/* build.c: checking the player's ROM and assembling the build.
 *
 * Mac: an app bundle
 *   Stunt Race FX.app/Contents/MacOS/Stunt Race FX      the game
 *                    /Contents/Resources/cart.dat      the cartridge's data (srfx.c)
 *                    /Contents/Resources/shaders/      its shaders
 *                    /Contents/Resources/AppIcon.icns  its icon
 * iPhone: Stunt Race FX.ipa, the app (Payload/Stunt Race FX.app, its shaders
 *   and icon in it) with cart.dat inside, signed with the player's Xcode
 *   signing when this Mac has it (rb_ios_signing), else unsigned for the
 *   player to sign
 * Windows: a folder
 *   Stunt Race FX\Stunt Race FX.exe (its icon built in), shaders\, cart.dat
 * Linux: a folder
 *   Stunt Race FX/Stunt Race FX     the game
 *                /shaders/, cart.dat
 *                /icon.png, icon.argb      its icon: the desktop file's, the window's
 *                /Stunt Race FX.desktop    a launcher for the folder as built (copy it to
 *                                          ~/.local/share/applications to list the game)
 *
 * The build is made in a hidden folder beside its destination and renamed
 * into place when complete; an earlier build is replaced only when it carries
 * this builder's marker file. */
#include <stdlib.h>
#include <string.h>
#include "rb_int.h"

#ifdef __APPLE__
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
#endif

#ifndef RB_VERSION
#define RB_VERSION "0.0"
#endif

#define MARKER ".stuntracefx-builder"
#define GAME_EXE RB_PRODUCT                /* the Mac app's and iPhone app's executable */
#define IOS_APP RB_PRODUCT ".app"
#define COPIER_HEADER 512u                 /* a copier's header before the ROM (.smc) */

const char *rb_target_name(int t)
{
    return t == RB_MACOS ? "Mac" : t == RB_IOS ? "iPhone" : t == RB_LINUX ? "Linux" : "Windows";
}

const char *rb_output_name(int t)
{
    return t == RB_IOS ? RB_PRODUCT ".ipa" : t == RB_MACOS ? RB_PRODUCT ".app" : RB_PRODUCT;
}

void rb_output_path(const rb_job *job, char *out, size_t n)
{
    rb_join(out, n, job->dest_dir, rb_output_name(job->target));
}

/* ---- checks --------------------------------------------------------------------------- */
static void md5_hex(const uint8_t *p, size_t n, char hex[33])
{
    rb_md5 m;
    uint8_t d[16];
    rb_md5_init(&m);
    rb_md5_update(&m, p, n);
    rb_md5_final(&m, d);
    rb_md5_hex(d, hex);
}

/* the ROM, read: the cartridge's bytes (a copier's header left out) when they
 * are No-Intro's; else NULL, with what it is in out */
static uint8_t *load_rom(const char *path, rb_check *out)
{
    size_t n;
    uint8_t *r = rb_read_file(path, &n);
    char md5[33];
    memset(out, 0, sizeof *out);
    if (!r) {
        snprintf(out->note, sizeof out->note, "The file could not be read.");
        return NULL;
    }
    md5_hex(r, n, out->md5);
    if (n == RB_ROM_SIZE && !strcmp(out->md5, RB_ROM_MD5)) {
        out->ok = 1;
        snprintf(out->note, sizeof out->note, RB_ROM_NAME ".");
        return r;
    }
    if (n == RB_ROM_SIZE + COPIER_HEADER) {
        md5_hex(r + COPIER_HEADER, RB_ROM_SIZE, md5);
        if (!strcmp(md5, RB_ROM_MD5)) {
            out->ok = 1;
            memmove(r, r + COPIER_HEADER, RB_ROM_SIZE);
            snprintf(out->note, sizeof out->note, RB_ROM_NAME ", with a copier's 512-byte header: without it, its MD5 is %s.", md5);
            return r;
        }
    }
    free(r);
    snprintf(out->note, sizeof out->note,
             n == RB_ROM_SIZE || n == RB_ROM_SIZE + COPIER_HEADER
                 ? "This is not " RB_ROM_NAME ". The first release (Rev 0) and the European and Japanese cartridges are not supported yet."
                 : "This is not " RB_ROM_NAME " (a 1 MB SNES ROM).");
    return NULL;
}

int rb_check_rom(const char *path, rb_check *out, rb_progress cb, void *ctx, volatile int *cancel)
{
    uint8_t *r;
    (void)cancel;
    if (cb)
        cb(ctx, 0, NULL);
    r = load_rom(path, out);
    free(r);
    if (cb)
        cb(ctx, 1, NULL);
    return out->ok;
}

/* ---- the output ------------------------------------------------------------------------- */
#ifdef __APPLE__
/* a tool, waited for: its exit status (-1 when it could not start); its
 * output to out (n bytes) when out is given */
static int run(char *const argv[], char *out, size_t n)
{
    pid_t pid;
    int st = 0, fd[2] = { -1, -1 };
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    if (out) {
        if (pipe(fd) != 0)
            return -1;
        posix_spawn_file_actions_adddup2(&fa, fd[1], 1);
        posix_spawn_file_actions_addclose(&fa, fd[0]);
    }
    if (posix_spawn(&pid, argv[0], &fa, NULL, argv, environ) != 0) {
        posix_spawn_file_actions_destroy(&fa);
        if (out) {
            close(fd[0]);
            close(fd[1]);
        }
        return -1;
    }
    posix_spawn_file_actions_destroy(&fa);
    if (out) {
        size_t got = 0;
        ssize_t r;
        close(fd[1]);
        while (got + 1 < n && (r = read(fd[0], out + got, n - 1 - got)) > 0)
            got += (size_t)r;
        out[got] = 0;
        close(fd[0]);
    }
    waitpid(pid, &st, 0);
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

int rb_ios_signing(char *desc, size_t n)
{
    char sh[2048], err[256], line[2048] = "";
    char *argv[] = { "/bin/sh", sh, "check", NULL };
    if (!rb_host_payload("ios-sign", sh, sizeof sh, err, sizeof err) || run(argv, line, sizeof line) != 0 || !line[0])
        return 0;
    if (strchr(line, '\t'))
        *strchr(line, '\t') = 0;                      /* the certificate's name */
    snprintf(desc, n, "%s", line);
    return 1;
}
#else
int rb_ios_signing(char *desc, size_t n)
{
    (void)desc, (void)n;
    return 0;
}
#endif

/* the marker sits in the app's Resources (Mac), the app inside the .ipa
 * (iPhone), or the folder (Windows, Linux) */
static void marker_path(int target, const char *root, char *out, size_t n)
{
    char r[2048], a[2048];
    if (target == RB_MACOS) {
        rb_join(r, sizeof r, root, "Contents/Resources");
        rb_join(out, n, r, MARKER);
    } else if (target == RB_IOS) {
        rb_join(a, sizeof a, root, "Payload");
        rb_join(r, sizeof r, a, IOS_APP);
        rb_join(out, n, r, MARKER);
    } else {
        rb_join(out, n, root, MARKER);
    }
}

/* an .ipa is a zip: this builder's has its marker among the names */
static int ipa_ours(const char *path)
{
    static const char name[] = "Payload/" IOS_APP "/" MARKER;
    size_t n, i;
    char *d = rb_read_file(path, &n);
    int ours = 0;
    for (i = 0; d && !ours && i + sizeof name - 1 <= n; i++)
        ours = d[i] == 'P' && !memcmp(d + i, name, sizeof name - 1);
    free(d);
    return ours;
}

int rb_output_exists(const rb_job *job, int *ours)
{
    char out[2048], mk[2048];
    rb_output_path(job, out, sizeof out);
    if (ours) {
        if (job->target == RB_IOS) {
            *ours = ipa_ours(out);
        } else {
            marker_path(job->target, out, mk, sizeof mk);
            *ours = rb_exists(mk);
        }
    }
    return rb_exists(out);
}

typedef struct ctx {
    const rb_job *job;
    rb_progress cb;
    void *cctx;
    volatile int *cancel;
    char *err;
    size_t errlen;
    char root[2048];           /* the staged build */
    char app[2048];            /* the app inside it (Mac, iPhone) */
    char res[2048];            /* where the data goes */
} ctx;

static void say(ctx *c, double f, const char *s)
{
    if (c->cb)
        c->cb(c->cctx, f, s);
}

static int cancelled(ctx *c)
{
    if (c->cancel && *c->cancel) {
        rb_err(c->err, c->errlen, "the build was cancelled");
        return 1;
    }
    return 0;
}

/* the Mac app's Info.plist */
static int mac_plist(ctx *c)
{
    char path[2048], buf[4096], contents[2048];
    snprintf(buf, sizeof buf,
             "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
             "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
             "<plist version=\"1.0\">\n<dict>\n"
             "    <key>CFBundleName</key><string>%s</string>\n"
             "    <key>CFBundleDisplayName</key><string>%s</string>\n"
             "    <key>CFBundleIdentifier</key><string>com.strutb.stuntracefx</string>\n"
             "    <key>CFBundleExecutable</key><string>%s</string>\n"
             "    <key>CFBundleIconFile</key><string>AppIcon</string>\n"
             "    <key>CFBundlePackageType</key><string>APPL</string>\n"
             "    <key>CFBundleShortVersionString</key><string>%s</string>\n"
             "    <key>CFBundleVersion</key><string>%s</string>\n"
             "    <key>LSApplicationCategoryType</key><string>public.app-category.racing-games</string>\n"
             "    <key>LSMinimumSystemVersion</key><string>12.0</string>\n"
             "    <key>NSHighResolutionCapable</key><true/>\n"
             "    <key>GCSupportsControllerUserInteraction</key><true/>\n"
             "</dict>\n</plist>\n",
             RB_PRODUCT, RB_PRODUCT, GAME_EXE, RB_VERSION, RB_VERSION);
    rb_join(contents, sizeof contents, c->root, "Contents");
    rb_join(path, sizeof path, contents, "Info.plist");
    if (!rb_write_file(path, buf, strlen(buf))) {
        rb_err(c->err, c->errlen, "cannot write the app's Info.plist");
        return 0;
    }
    rb_join(path, sizeof path, contents, "PkgInfo");
    return rb_write_file(path, "APPL????", 8);
}

/* a piece of the payload copied to `to` */
static int payload(ctx *c, const char *name, const char *to, int tree)
{
    char src[2048];
    if (!rb_host_payload(name, src, sizeof src, c->err, c->errlen))
        return 0;
    if (tree ? !rb_copy_tree(src, to) : !rb_copy_file(src, to)) {
        rb_err(c->err, c->errlen, "cannot copy the builder's %s to %s", name, to);
        return 0;
    }
    return 1;
}

/* the staged layout, with the game and its icon in it */
static int stage(ctx *c)
{
    char tmpname[512], path[2048], dir[2048];
    int t = c->job->target;
    snprintf(tmpname, sizeof tmpname, ".%s.building", rb_output_name(t));
    rb_join(c->root, sizeof c->root, c->job->dest_dir, tmpname);
    if (rb_exists(c->root) && !rb_rmtree(c->root)) {        /* a build that stopped partway */
        rb_err(c->err, c->errlen, "cannot clear %s", c->root);
        return 0;
    }
    if (t == RB_MACOS) {
        snprintf(c->app, sizeof c->app, "%s", c->root);
        rb_join(c->res, sizeof c->res, c->root, "Contents/Resources");
        rb_join(dir, sizeof dir, c->root, "Contents/MacOS");
        if (!rb_mkdirs(dir) || !rb_mkdirs(c->res))
            goto nodir;
        rb_join(path, sizeof path, dir, GAME_EXE);
        if (!payload(c, "game", path, 0))
            return 0;
        rb_set_exec(path);
        rb_join(path, sizeof path, c->res, "shaders");
        if (!payload(c, "shaders", path, 1))
            return 0;
        rb_join(path, sizeof path, c->res, "AppIcon.icns");
        return payload(c, "icon", path, 0);
    }
    if (t == RB_IOS) {
        rb_join(dir, sizeof dir, c->root, "Payload");
        rb_join(c->app, sizeof c->app, dir, IOS_APP);
        snprintf(c->res, sizeof c->res, "%s", c->app);
        if (!rb_mkdirs(dir))
            goto nodir;
        return payload(c, "ios-app", c->app, 1);
    }
    snprintf(c->app, sizeof c->app, "%s", c->root);
    snprintf(c->res, sizeof c->res, "%s", c->root);
    if (!rb_mkdirs(c->root))
        goto nodir;
    rb_join(path, sizeof path, c->root, t == RB_LINUX ? RB_PRODUCT : RB_PRODUCT ".exe");
    if (!payload(c, "game", path, 0))
        return 0;
    if (t == RB_LINUX) {
        rb_set_exec(path);
        rb_join(path, sizeof path, c->root, "icon.png");
        if (!payload(c, "icon-png", path, 0))
            return 0;
        rb_join(path, sizeof path, c->root, "icon.argb");
        if (!payload(c, "icon-argb", path, 0))
            return 0;
    }
    rb_join(path, sizeof path, c->root, "shaders");
    return payload(c, "shaders", path, 1);
nodir:
    rb_err(c->err, c->errlen, "cannot make folders in %s (is it writable?)", c->job->dest_dir);
    return 0;
}

/* the game's data, from the ROM */
static int data(ctx *c)
{
    rb_check ck;
    uint8_t *rom, *img;
    size_t n;
    char path[2048];
    int ok;
    say(c, 0.05, "Reading the ROM");
    if (!(rom = load_rom(c->job->rom, &ck))) {
        rb_err(c->err, c->errlen, "%s: %s", c->job->rom, ck.note);
        return 0;
    }
    if (cancelled(c)) {
        free(rom);
        return 0;
    }
    say(c, 0.3, "Taking the game's data from the ROM");
    img = rb_cart(rom, &n, c->err, c->errlen);
    free(rom);
    if (!img)
        return 0;
    say(c, 0.7, "Writing the game's data");
    rb_join(path, sizeof path, c->res, "cart.dat");
    ok = rb_write_file(path, img, n);
    free(img);
    if (!ok)
        rb_err(c->err, c->errlen, "cannot write %s", path);
    return ok && !cancelled(c);
}

/* Linux: a launcher for the game where it will be once moved into place */
static int linux_desktop(ctx *c)
{
    char out[2048], path[2048], buf[8192];
    rb_output_path(c->job, out, sizeof out);
    snprintf(buf, sizeof buf,
             "[Desktop Entry]\n"
             "Type=Application\n"
             "Name=%s\n"
             "Comment=Stunt Race FX, built from your cartridge\n"
             "Exec=\"%s/%s\"\n"
             "Path=%s\n"
             "Icon=%s/icon.png\n"
             "Categories=Game;\n"
             "Terminal=false\n",
             RB_PRODUCT, out, RB_PRODUCT, out, out);
    rb_join(path, sizeof path, c->root, RB_PRODUCT ".desktop");
    if (!rb_write_file(path, buf, strlen(buf))) {
        rb_err(c->err, c->errlen, "cannot write the game's desktop file");
        return 0;
    }
    rb_set_exec(path);
    return 1;
}

/* sealed and moved into place */
static int finish(ctx *c)
{
    char out[2048], mk[2048], staged[2048];
    int ours = 0, t = c->job->target;
    if (t == RB_MACOS && !mac_plist(c))
        return 0;
    if (t == RB_LINUX && !linux_desktop(c))
        return 0;
    marker_path(t, c->root, mk, sizeof mk);
    if (!rb_write_file(mk, RB_VERSION "\n", strlen(RB_VERSION "\n"))) {
        rb_err(c->err, c->errlen, "cannot write in %s", c->root);
        return 0;
    }
    snprintf(staged, sizeof staged, "%s", c->root);
#ifdef __APPLE__
    if (t == RB_MACOS) {       /* sign the bundle as a whole (ad hoc), so its parts are sealed together */
        char *argv[] = { "/usr/bin/codesign", "--force", "--sign", "-", c->root, NULL };
        say(c, 0.9, "Signing the app");
        if (run(argv, NULL, 0) != 0) {
            rb_err(c->err, c->errlen, "signing the app failed (codesign)");
            return 0;
        }
    } else if (t == RB_IOS) {
        char sh[2048], payload_dir[2048], dummy[16];
        char *sign[] = { "/bin/sh", sh, "sign", c->app, NULL };
        char *zip[] = { "/usr/bin/ditto", "-c", "-k", "--sequesterRsrc", "--keepParent", payload_dir, staged, NULL };
        if (rb_ios_signing(dummy, sizeof dummy)) {
            say(c, 0.85, "Signing the app with your Xcode signing");
            if (!rb_host_payload("ios-sign", sh, sizeof sh, c->err, c->errlen) || run(sign, NULL, 0) != 0) {
                rb_err(c->err, c->errlen, "signing the iPhone app failed (codesign); check your certificate in Xcode");
                return 0;
            }
        }
        say(c, 0.92, "Packing the .ipa");
        rb_join(payload_dir, sizeof payload_dir, c->root, "Payload");
        snprintf(staged, sizeof staged, "%s.zip", c->root);
        if (run(zip, NULL, 0) != 0) {
            rb_err(c->err, c->errlen, "cannot pack the .ipa");
            return 0;
        }
        rb_rmtree(c->root);
        snprintf(c->root, sizeof c->root, "%s", staged);       /* what is left to clear on failure */
    }
#endif
    rb_output_path(c->job, out, sizeof out);
    if (rb_output_exists(c->job, &ours)) {
        if (!ours) {
            rb_err(c->err, c->errlen, "%s already exists and was not made by this builder; move it away or pick another folder", out);
            return 0;
        }
        if (!rb_rmtree(out)) {
            rb_err(c->err, c->errlen, "cannot replace the earlier build at %s", out);
            return 0;
        }
    }
    if (!rb_rename(staged, out)) {
        rb_err(c->err, c->errlen, "cannot move the build into place at %s", out);
        return 0;
    }
    return 1;
}

int rb_build(const rb_job *job, rb_progress cb, void *cctx, volatile int *cancel, char *err, size_t errlen)
{
    ctx c;
    int ok;
    memset(&c, 0, sizeof c);
    c.job = job;
    c.cb = cb;
    c.cctx = cctx;
    c.cancel = cancel;
    c.err = err;
    c.errlen = errlen;
    if (err && errlen)
        err[0] = 0;
    if (!rb_is_dir(job->dest_dir) && !rb_mkdirs(job->dest_dir)) {
        rb_err(err, errlen, "cannot make the folder %s", job->dest_dir);
        return 0;
    }
    say(&c, 0, "Preparing");
    ok = stage(&c) && data(&c) && finish(&c);
    if (!ok) {
        if (c.root[0])
            rb_rmtree(c.root);                         /* leave nothing half-made behind */
        if (err && errlen && !err[0])
            rb_err(err, errlen, "the build failed");
    } else {
        say(&c, 1.0, "Done");
    }
    return ok;
}
