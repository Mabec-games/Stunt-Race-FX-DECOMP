/* main.c: the release builder's Linux front end (GTK 3, loaded while it runs).
 * It builds Stunt Race FX for Linux.
 *
 *   1 what it makes
 *   2 the player's own ROM, checked by MD5, and the folder to build into
 *   3 the build (core/build.c), with progress
 *   4 the result: the game's folder, or what went wrong
 *
 * GTK is opened with dlopen rather than linked, as nativeshvc opens X11, ALSA
 * and Vulkan: the builder then needs only the C library to start, so one
 * build runs on any desktop Linux that has GTK 3, and says so plainly on one
 * that has not. The game it copies is a plain file in the game folder beside
 * it.
 *
 * Arguments fill in the choices for a scripted run, and --build runs it in
 * the terminal, with no window:
 *   --dest DIR  --rom FILE  --build
 * A scripted run prints its result, its exit code 0 on success. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <limits.h>
#include <pthread.h>
#include <spawn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rb.h"

extern char **environ;

/* ---- GTK 3, GLib and GObject, the calls this uses ------------------------------------- */
typedef void W;                          /* any GtkWidget, GtkWindow, GtkBox ... */
typedef int gboolean;
typedef int (*GSourceFunc)(void *);
typedef void (*GCallback)(void);

#define GTK_WINDOW_TOPLEVEL 0
#define GTK_ORIENTATION_HORIZONTAL 0
#define GTK_ORIENTATION_VERTICAL 1
#define GTK_FILE_CHOOSER_ACTION_OPEN 0
#define GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER 2
#define GTK_RESPONSE_ACCEPT (-3)
#define GTK_RESPONSE_CANCEL (-6)
#define GTK_RESPONSE_YES (-8)
#define GTK_DIALOG_MODAL 1
#define GTK_MESSAGE_WARNING 1
#define GTK_MESSAGE_QUESTION 2
#define GTK_BUTTONS_OK 1
#define GTK_BUTTONS_YES_NO 4
#define GTK_ALIGN_END 2

static struct {
    gboolean (*init_check)(int *, char ***);
    W *(*window_new)(int);
    void (*window_set_title)(W *, const char *);
    void (*window_set_default_size)(W *, int, int);
    void (*window_set_resizable)(W *, gboolean);
    gboolean (*window_set_icon_from_file)(W *, const char *, void **);
    void (*container_add)(W *, W *);
    void (*container_set_border_width)(W *, unsigned);
    W *(*box_new)(int, int);
    void (*box_pack_start)(W *, W *, gboolean, gboolean, unsigned);
    void (*box_pack_end)(W *, W *, gboolean, gboolean, unsigned);
    W *(*label_new)(const char *);
    void (*label_set_markup)(W *, const char *);
    void (*label_set_text)(W *, const char *);
    void (*label_set_line_wrap)(W *, gboolean);
    void (*label_set_xalign)(W *, float);
    void (*label_set_max_width_chars)(W *, int);
    void (*label_set_selectable)(W *, gboolean);
    W *(*button_new_with_label)(const char *);
    void (*widget_set_sensitive)(W *, gboolean);
    void (*widget_set_halign)(W *, int);
    void (*widget_show_all)(W *);
    void (*widget_destroy)(W *);
    void (*widget_grab_default)(W *);
    void (*widget_set_can_default)(W *, gboolean);
    W *(*progress_bar_new)(void);
    void (*progress_bar_set_fraction)(W *, double);
    void (*main)(void);
    void (*main_quit)(void);
    W *(*file_chooser_dialog_new)(const char *, W *, int, const char *, ...);
    char *(*file_chooser_get_filename)(W *);
    gboolean (*file_chooser_set_current_folder)(W *, const char *);
    void (*file_chooser_add_filter)(W *, void *);
    void *(*file_filter_new)(void);
    void (*file_filter_set_name)(void *, const char *);
    void (*file_filter_add_pattern)(void *, const char *);
    int (*dialog_run)(W *);
    W *(*message_dialog_new)(W *, int, int, int, const char *, ...);
    unsigned long (*signal_connect_data)(void *, const char *, GCallback, void *, void *, int);
    unsigned (*idle_add)(GSourceFunc, void *);
    void (*g_free)(void *);
    char *(*markup_escape_text)(const char *, long);
} gtk;

static int gtk_load(void)
{
    void *g = dlopen("libgtk-3.so.0", RTLD_NOW | RTLD_GLOBAL);
    void *glib = dlopen("libglib-2.0.so.0", RTLD_NOW | RTLD_GLOBAL);
    void *gobj = dlopen("libgobject-2.0.so.0", RTLD_NOW | RTLD_GLOBAL);
    int ok = 1;
    if (!g || !glib || !gobj)
        return 0;
#define F(field, lib, name) ok &= (*(void **)&gtk.field = dlsym(lib, name)) != NULL
    F(init_check, g, "gtk_init_check");
    F(window_new, g, "gtk_window_new");
    F(window_set_title, g, "gtk_window_set_title");
    F(window_set_default_size, g, "gtk_window_set_default_size");
    F(window_set_resizable, g, "gtk_window_set_resizable");
    F(window_set_icon_from_file, g, "gtk_window_set_icon_from_file");
    F(container_add, g, "gtk_container_add");
    F(container_set_border_width, g, "gtk_container_set_border_width");
    F(box_new, g, "gtk_box_new");
    F(box_pack_start, g, "gtk_box_pack_start");
    F(box_pack_end, g, "gtk_box_pack_end");
    F(label_new, g, "gtk_label_new");
    F(label_set_markup, g, "gtk_label_set_markup");
    F(label_set_text, g, "gtk_label_set_text");
    F(label_set_line_wrap, g, "gtk_label_set_line_wrap");
    F(label_set_xalign, g, "gtk_label_set_xalign");
    F(label_set_max_width_chars, g, "gtk_label_set_max_width_chars");
    F(label_set_selectable, g, "gtk_label_set_selectable");
    F(button_new_with_label, g, "gtk_button_new_with_label");
    F(widget_set_sensitive, g, "gtk_widget_set_sensitive");
    F(widget_set_halign, g, "gtk_widget_set_halign");
    F(widget_show_all, g, "gtk_widget_show_all");
    F(widget_destroy, g, "gtk_widget_destroy");
    F(widget_grab_default, g, "gtk_widget_grab_default");
    F(widget_set_can_default, g, "gtk_widget_set_can_default");
    F(progress_bar_new, g, "gtk_progress_bar_new");
    F(progress_bar_set_fraction, g, "gtk_progress_bar_set_fraction");
    F(main, g, "gtk_main");
    F(main_quit, g, "gtk_main_quit");
    F(file_chooser_dialog_new, g, "gtk_file_chooser_dialog_new");
    F(file_chooser_get_filename, g, "gtk_file_chooser_get_filename");
    F(file_chooser_set_current_folder, g, "gtk_file_chooser_set_current_folder");
    F(file_chooser_add_filter, g, "gtk_file_chooser_add_filter");
    F(file_filter_new, g, "gtk_file_filter_new");
    F(file_filter_set_name, g, "gtk_file_filter_set_name");
    F(file_filter_add_pattern, g, "gtk_file_filter_add_pattern");
    F(dialog_run, g, "gtk_dialog_run");
    F(message_dialog_new, g, "gtk_message_dialog_new");
    F(signal_connect_data, gobj, "g_signal_connect_data");
    F(idle_add, glib, "g_idle_add");
    F(g_free, glib, "g_free");
    F(markup_escape_text, glib, "g_markup_escape_text");
#undef F
    return ok;
}

#define on(w, sig, fn, data) gtk.signal_connect_data((w), (sig), (GCallback)(void (*)(void))(fn), (data), NULL, 0)

/* ---- where the builder and its game are --------------------------------------------------- */
static char s_here[PATH_MAX];            /* the builder's own folder */

/* the pieces in the game folder beside the builder */
int rb_host_payload(const char *name, char *path, size_t n, char *err, size_t errlen)
{
    const char *rel = !strcmp(name, "game") ? "game/stuntracefx" : !strcmp(name, "shaders") ? "game/shaders"
                    : !strcmp(name, "icon-png") ? "game/icon.png" : !strcmp(name, "icon-argb") ? "game/icon.argb" : NULL;
    struct stat st;
    if (rel)
        snprintf(path, n, "%s/%s", s_here, rel);
    if (!rel || stat(path, &st) != 0) {
        snprintf(err, errlen, "the builder's game folder is missing a file (%s); extract the whole download and run the "
                 "builder from that folder", name);
        return 0;
    }
    return 1;
}

/* ---- state ----------------------------------------------------------------------------------- */
static W *s_wnd, *s_page_box, *s_bar, *s_status, *s_build, *s_romtext, *s_destlabel;
static int s_page, s_ok, s_checking, s_result;
static volatile int s_cancel;
static char s_dest[PATH_MAX], s_rom[PATH_MAX];
static char s_err[1024], s_out[2048];
static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
static char s_status_text[256];
static double s_fraction;

/* text set as markup, escaped */
static void markup(W *label, const char *fmt, ...)
{
    char raw[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(raw, sizeof raw, fmt, ap);
    va_end(ap);
    gtk.label_set_markup(label, raw);
}

static char *esc(const char *s) { return gtk.markup_escape_text(s, -1); }

/* ---- layout ----------------------------------------------------------------------------------- */
static W *row_buttons;

static void clear_page(void)
{
    if (s_page_box)
        gtk.widget_destroy(s_page_box);
    s_page_box = gtk.box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk.container_add(s_wnd, s_page_box);
    row_buttons = gtk.box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk.widget_set_halign(row_buttons, GTK_ALIGN_END);
    gtk.box_pack_end(s_page_box, row_buttons, 0, 0, 0);
    s_bar = s_status = s_build = s_romtext = s_destlabel = NULL;
}

static W *text(const char *markup_text)
{
    W *l = gtk.label_new(NULL);
    gtk.label_set_markup(l, markup_text);
    gtk.label_set_line_wrap(l, 1);
    gtk.label_set_max_width_chars(l, 70);
    gtk.label_set_xalign(l, 0);
    gtk.box_pack_start(s_page_box, l, 0, 0, 0);
    return l;
}

static W *button(const char *label, void (*fn)(void), int def)
{
    W *b = gtk.button_new_with_label(label);
    on(b, "clicked", fn, NULL);
    gtk.box_pack_start(row_buttons, b, 0, 0, 0);
    if (def) {
        gtk.widget_set_can_default(b, 1);
        gtk.widget_grab_default(b);
    }
    return b;
}

static void show(void) { gtk.widget_show_all(s_wnd); }

/* ---- pages --------------------------------------------------------------------------------- */
static void page_rom(void);
static void pick_rom(void);
static void pick_dest(void);
static void build(void);
static void cancel(void);
static void quit(void) { gtk.main_quit(); }
static void open_folder(void);
static void play(void);

static void page_intro(void)
{
    s_page = 1;
    clear_page();
    text("<span size='x-large' weight='bold'>Stunt Race FX Builder</span>");
    text("Stunt Race FX Builder makes a native Linux build of Stunt Race FX (Super Nintendo, 1994) from your own "
         "copy of it: the game as C, compiled for your PC.");
    text("<b>Stunt Race FX</b>");
    text("A Linux program (from 2022 on: glibc 2.35 or later, X11 or XWayland, with a Vulkan graphics driver), played "
         "with the keyboard or a game controller. It needs your ROM of the cartridge, " RB_ROM_NAME ", once, to build.");
    button("Continue", page_rom, 1);
    show();
}

static void update_build(void)
{
    if (s_build)
        gtk.widget_set_sensitive(s_build, s_ok && !s_checking);
}

static void show_dest(void)
{
    if (s_destlabel) {
        char *e = esc(s_dest);
        markup(s_destlabel, "<tt>%s</tt>", e);
        gtk.g_free(e);
    }
}

static void show_rom(const rb_check *c)
{
    const char *name = strrchr(s_rom, '/') ? strrchr(s_rom, '/') + 1 : s_rom;
    char *en, *note;
    if (!s_romtext)
        return;
    en = esc(name);
    if (!s_rom[0]) {
        markup(s_romtext, "<tt>No file chosen</tt>");
    } else if (!c) {
        markup(s_romtext, "<tt>%s: checking...</tt>", en);
    } else {
        note = esc(c->note);
        markup(s_romtext, "<span foreground='%s'><tt>%s\nYour file's MD5  %s  %s%s%s</tt></span>",
               c->ok ? "#008000" : "#c00000", en, c->md5, c->ok ? "✓ matches" : "✗ does not match",
               c->note[0] ? "\n" : "", note);
        gtk.g_free(note);
    }
    gtk.g_free(en);
}

static rb_check s_last;
static int s_have_last;

static void page_rom(void)
{
    W *row;
    s_page = 2;
    clear_page();
    text("<span size='x-large' weight='bold'>Your copy of the game</span>");
    text("The game's data is copyrighted, so the builder cannot include it. Provide a ROM of the Stunt Race FX cartridge: "
         "the builder checks it by MD5 and takes only the data the game reads.");
    row = gtk.box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk.box_pack_start(s_page_box, row, 0, 0, 0);
    {
        W *l = gtk.label_new(NULL), *b = gtk.button_new_with_label("Choose...");
        gtk.label_set_markup(l, "<b>Stunt Race FX: cartridge ROM</b>");
        gtk.box_pack_start(row, l, 0, 0, 0);
        gtk.box_pack_end(row, b, 0, 0, 0);
        on(b, "clicked", pick_rom, NULL);
    }
    text("<tt>Expected MD5  " RB_ROM_MD5 "</tt>");
    s_romtext = text("");
    gtk.label_set_selectable(s_romtext, 1);
    show_rom(s_have_last ? &s_last : NULL);
    row = gtk.box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk.box_pack_start(s_page_box, row, 0, 0, 8);
    {
        W *l = gtk.label_new(NULL), *b = gtk.button_new_with_label("Choose...");
        gtk.label_set_markup(l, "<b>Build into:</b>");
        gtk.box_pack_start(row, l, 0, 0, 0);
        s_destlabel = gtk.label_new(NULL);
        gtk.box_pack_start(row, s_destlabel, 0, 0, 0);
        gtk.box_pack_end(row, b, 0, 0, 0);
        on(b, "clicked", pick_dest, NULL);
    }
    show_dest();
    button("Back", page_intro, 0);
    s_build = button("Build", build, 1);
    update_build();
    show();
}

static void page_build(void)
{
    s_page = 3;
    clear_page();
    text("<span size='x-large' weight='bold'>Building Stunt Race FX</span>");
    s_bar = gtk.progress_bar_new();
    gtk.box_pack_start(s_page_box, s_bar, 0, 0, 8);
    s_status = text("Starting");
    button("Cancel", cancel, 0);
    show();
}

static void page_result(void)
{
    s_page = 4;
    clear_page();
    if (s_result) {
        char *e = esc(s_out);
        text("<span size='x-large' weight='bold'>Stunt Race FX is ready</span>");
        text("The build is complete. It carries everything it needs, so your ROM is no longer required to play.");
        markup(text(""), "<tt>%s</tt>", e);
        text("To list the game with your other programs, copy <tt>Stunt Race FX.desktop</tt> from its folder to "
             "<tt>~/.local/share/applications</tt>.");
        gtk.g_free(e);
        button("Done", quit, 0);
        button("Open Folder", open_folder, 0);
        button("Play", play, 1);
    } else {
        char *e = esc(s_err[0] ? s_err : "The build failed.");
        text("<span size='x-large' weight='bold'>The build did not finish</span>");
        text(e);
        gtk.g_free(e);
        button("Quit", quit, 0);
        button("Back", page_rom, 1);
    }
    show();
}

/* ---- checks ------------------------------------------------------------------------------ */
typedef struct check_job { char path[PATH_MAX]; rb_check c; } check_job;

static int checked(void *p)
{
    check_job *j = p;
    s_checking--;
    if (!strcmp(j->path, s_rom)) {
        s_ok = j->c.ok;
        s_last = j->c;
        s_have_last = 1;
        show_rom(&j->c);
    }
    free(j);
    update_build();
    return 0;
}

static void *check_thread(void *p)
{
    check_job *j = p;
    rb_check_rom(j->path, &j->c, NULL, NULL, NULL);
    gtk.idle_add(checked, j);
    return NULL;
}

static void start_check(const char *path)
{
    check_job *j = calloc(1, sizeof *j);
    pthread_t t;
    snprintf(j->path, sizeof j->path, "%s", path);
    snprintf(s_rom, sizeof s_rom, "%s", path);
    s_ok = 0;
    s_have_last = 0;
    show_rom(NULL);
    s_checking++;
    update_build();
    if (pthread_create(&t, NULL, check_thread, j) == 0)
        pthread_detach(t);
}

/* ---- pickers ---------------------------------------------------------------------------------- */
static int pick(int folder, char *out, size_t n)
{
    W *d = gtk.file_chooser_dialog_new(folder ? "Build into" : "Your Stunt Race FX ROM", s_wnd,
                                       folder ? GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER : GTK_FILE_CHOOSER_ACTION_OPEN,
                                       "_Cancel", GTK_RESPONSE_CANCEL, folder ? "_Select" : "_Open", GTK_RESPONSE_ACCEPT,
                                       NULL);
    int ok = 0;
    if (folder) {
        gtk.file_chooser_set_current_folder(d, s_dest);
    } else {
        void *f = gtk.file_filter_new(), *all = gtk.file_filter_new();
        gtk.file_filter_set_name(f, "SNES ROM (*.sfc, *.smc)");
        gtk.file_filter_add_pattern(f, "*.sfc");
        gtk.file_filter_add_pattern(f, "*.smc");
        gtk.file_filter_add_pattern(f, "*.SFC");
        gtk.file_filter_add_pattern(f, "*.SMC");
        gtk.file_chooser_add_filter(d, f);
        gtk.file_filter_set_name(all, "All files");
        gtk.file_filter_add_pattern(all, "*");
        gtk.file_chooser_add_filter(d, all);
    }
    if (gtk.dialog_run(d) == GTK_RESPONSE_ACCEPT) {
        char *p = gtk.file_chooser_get_filename(d);
        if (p) {
            snprintf(out, n, "%s", p);
            gtk.g_free(p);
            ok = 1;
        }
    }
    gtk.widget_destroy(d);
    return ok;
}

static void pick_rom(void)
{
    char p[PATH_MAX];
    if (pick(0, p, sizeof p))
        start_check(p);
}

static void pick_dest(void)
{
    if (pick(1, s_dest, sizeof s_dest))
        show_dest();
}

/* ---- the build ------------------------------------------------------------------------------- */
static int progressed(void *p)
{
    (void)p;
    pthread_mutex_lock(&s_lock);
    if (s_bar) {
        gtk.progress_bar_set_fraction(s_bar, s_fraction);
        if (!s_cancel)
            gtk.label_set_text(s_status, s_status_text);
    }
    pthread_mutex_unlock(&s_lock);
    return 0;
}

static void progress(void *ctx, double f, const char *s)
{
    (void)ctx;
    pthread_mutex_lock(&s_lock);
    s_fraction = f;
    if (s)
        snprintf(s_status_text, sizeof s_status_text, "%s", s);
    pthread_mutex_unlock(&s_lock);
    gtk.idle_add(progressed, NULL);
}

static int built(void *p)
{
    (void)p;
    page_result();
    return 0;
}

static void *build_thread(void *p)
{
    rb_job job = { 0 };
    (void)p;
    job.target = RB_LINUX;
    job.dest_dir = s_dest;
    job.rom = s_rom;
    s_err[0] = 0;
    s_result = rb_build(&job, progress, NULL, &s_cancel, s_err, sizeof s_err);
    rb_output_path(&job, s_out, sizeof s_out);
    gtk.idle_add(built, NULL);
    return NULL;
}

static int ask(int question, const char *msg)
{
    W *d = gtk.message_dialog_new(s_wnd, GTK_DIALOG_MODAL, question ? GTK_MESSAGE_QUESTION : GTK_MESSAGE_WARNING,
                                  question ? GTK_BUTTONS_YES_NO : GTK_BUTTONS_OK, "%s", msg);
    int r = gtk.dialog_run(d);
    gtk.widget_destroy(d);
    return r == GTK_RESPONSE_YES;
}

static void build(void)
{
    rb_job job = { 0 };
    char out[2048], msg[4096];
    int ours = 0;
    pthread_t t;
    job.target = RB_LINUX;
    job.dest_dir = s_dest;
    if (rb_output_exists(&job, &ours)) {
        rb_output_path(&job, out, sizeof out);
        if (!ours) {
            snprintf(msg, sizeof msg, "%s already exists and was not made by Stunt Race FX Builder, so it will not be "
                     "replaced. Move it away or choose another folder.", out);
            ask(0, msg);
            return;
        }
        snprintf(msg, sizeof msg, "An earlier build is at %s.\n\nReplace it? Your records and settings are kept; they "
                 "live in ~/.local/share, not in the game's folder.", out);
        if (!ask(1, msg))
            return;
    }
    s_cancel = 0;
    page_build();
    if (pthread_create(&t, NULL, build_thread, NULL) == 0)
        pthread_detach(t);
}

static void cancel(void)
{
    s_cancel = 1;
    if (s_status)
        gtk.label_set_text(s_status, "Stopping...");
}

/* a program started and left to run */
static void launch(char *const argv[], const char *dir)
{
    pid_t pid;
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    if (dir)
        posix_spawn_file_actions_addchdir_np(&fa, dir);     /* glibc 2.29 and later */
    posix_spawnp(&pid, argv[0], &fa, NULL, argv, environ);
    posix_spawn_file_actions_destroy(&fa);
}

static void open_folder(void)
{
    char *argv[] = { "xdg-open", s_out, NULL };
    launch(argv, NULL);
}

static void play(void)
{
    char exe[PATH_MAX + 32];
    char *argv[] = { exe, NULL };
    snprintf(exe, sizeof exe, "%s/" RB_PRODUCT, s_out);
    launch(argv, s_out);
}

/* closing the window while it builds stops the build first; it cleans up after itself */
static gboolean on_delete(void)
{
    if (s_page == 3) {
        cancel();
        return 1;
    }
    return 0;
}

static void on_destroy(void)
{
    gtk.main_quit();
}

/* ---- a scripted run, in the terminal ------------------------------------------------------- */
static void say(void *ctx, double f, const char *s)
{
    (void)ctx;
    if (s)
        printf("%3d%%  %s\n", (int)(f * 100 + 0.5), s);
    fflush(stdout);
}

static int scripted(void)
{
    rb_job job = { 0 };
    int ours = 0;
    job.target = RB_LINUX;
    job.dest_dir = s_dest;
    job.rom = s_rom;
    if (rb_output_exists(&job, &ours) && !ours) {
        rb_output_path(&job, s_out, sizeof s_out);
        printf("failed: %s already exists and was not made by Stunt Race FX Builder\n", s_out);
        return 1;
    }
    s_result = rb_build(&job, say, NULL, &s_cancel, s_err, sizeof s_err);
    rb_output_path(&job, s_out, sizeof s_out);
    printf("%s: %s\n", s_result ? "built" : "failed", s_result ? s_out : s_err);
    return s_result ? 0 : 1;
}

int main(int argc, char **argv)
{
    int i, script = 0;
    ssize_t n;
    const char *home = getenv("HOME");
    if ((n = readlink("/proc/self/exe", s_here, sizeof s_here - 1)) > 0) {
        s_here[n] = 0;
        if (strrchr(s_here, '/'))
            *strrchr(s_here, '/') = 0;
    } else {
        snprintf(s_here, sizeof s_here, ".");
    }
    snprintf(s_dest, sizeof s_dest, "%s/Games", home ? home : ".");
    for (i = 1; i < argc; i++) {
        const char *k = argv[i], *v = i + 1 < argc ? argv[i + 1] : "";
        if (!strcmp(k, "--build")) { script = 1; continue; }
        if (!strcmp(k, "--dest")) snprintf(s_dest, sizeof s_dest, "%s", v);
        else if (!strcmp(k, "--rom")) snprintf(s_rom, sizeof s_rom, "%s", v);
        else continue;
        i++;
    }
    if (script)
        return scripted();
    if (!gtk_load() || !gtk.init_check(&argc, &argv)) {
        fprintf(stderr, "Stunt Race FX Builder needs GTK 3 and a desktop to show its window. Without them, build from the "
                "terminal:\n  %s/StuntRaceFXBuilder --rom YOUR_ROM.sfc --dest FOLDER --build\n", s_here);
        return 1;
    }
    s_wnd = gtk.window_new(GTK_WINDOW_TOPLEVEL);
    gtk.window_set_title(s_wnd, RB_BUILDER);
    gtk.window_set_default_size(s_wnd, 600, 440);
    gtk.window_set_resizable(s_wnd, 0);
    gtk.container_set_border_width(s_wnd, 20);
    {
        char icon[PATH_MAX + 32];
        snprintf(icon, sizeof icon, "%s/game/icon.png", s_here);
        gtk.window_set_icon_from_file(s_wnd, icon, NULL);
    }
    on(s_wnd, "delete-event", on_delete, NULL);
    on(s_wnd, "destroy", on_destroy, NULL);
    page_intro();
    if (s_rom[0]) {
        page_rom();
        start_check(s_rom);
    }
    gtk.main();
    return 0;
}
