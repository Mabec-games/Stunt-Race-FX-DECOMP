/* rb.h: the release builder's core, shared by the macOS, Windows and Linux front ends.
 *
 * The builder turns the player's own copy of Stunt Race FX into a build of
 * the native game (native/stuntracefx on nativeshvc). The code ships inside
 * the builder, built with none of the cartridge's data in it; the data comes
 * from the player's ROM, checked by MD5: the No-Intro dump of Stunt Race FX
 * (USA) (Rev 1). The build carries the bytes the game reads
 * (native/stuntracefx/tools/assets.csv) as cart.dat: an image of the ROM
 * with each range at its offset and nothing where the ROM's code was. The
 * result needs no ROM once built. It is for the system the builder runs on
 * (Windows: Windows; Linux: Linux; macOS: the Mac, or the iPhone and iPad as an
 * .ipa).
 *
 * Paths are UTF-8 everywhere. Every long call reports progress through an
 * rb_progress and stops early (with an error) when *cancel becomes nonzero.
 */
#ifndef RB_H
#define RB_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* what the build is for */
enum { RB_MACOS, RB_IOS, RB_WINDOWS, RB_LINUX };

typedef void (*rb_progress)(void *ctx, double fraction, const char *status);

/* the dump the builder accepts: No-Intro's Stunt Race FX (USA) (Rev 1) */
#define RB_ROM_MD5  "128b316a74caf17fdc216b3ab46d4a9a"
#define RB_ROM_NAME "Stunt Race FX (USA) (Rev 1)"
#define RB_ROM_SIZE 0x100000u

#define RB_PRODUCT "Stunt Race FX"
#define RB_BUILDER "Stunt Race FX Builder"
const char *rb_target_name(int target);       /* "Mac" / "iPhone" / "Windows" / "Linux" */
/* what the build is called in the destination folder: "Stunt Race FX.app"
 * (Mac), "Stunt Race FX.ipa" (iPhone), the folder "Stunt Race FX" (Windows, Linux) */
const char *rb_output_name(int target);

/* ---- checking what the player provides ---------------------------------------- */
typedef struct rb_check {
    int  ok;                  /* the file is the dump the builder reads */
    char md5[33];             /* the file's MD5, as it is */
    char note[256];           /* why it is not, or how it was taken (a copier header) */
} rb_check;

/* the ROM: its MD5 as it is, and without a copier's 512-byte header when it
 * has one, against No-Intro's */
int rb_check_rom(const char *path, rb_check *out, rb_progress cb, void *ctx, volatile int *cancel);

/* ---- building ---------------------------------------------------------------------- */
typedef struct rb_job {
    int         target;       /* RB_MACOS / RB_IOS / RB_WINDOWS / RB_LINUX */
    const char *rom;          /* the cartridge's ROM */
    const char *dest_dir;     /* the folder the build goes into */
} rb_job;

/* 1 when dest_dir already holds this target's output; *ours is 1 when this
 * builder made it (it then may be replaced) */
int rb_output_exists(const rb_job *job, int *ours);
/* the whole build; 1 on success, else 0 with the reason in err */
int rb_build(const rb_job *job, rb_progress cb, void *ctx, volatile int *cancel, char *err, size_t errlen);
/* where the build went (dest_dir/rb_output_name) */
void rb_output_path(const rb_job *job, char *out, size_t n);

/* the iPhone build's signing: the player's own Xcode signing when this Mac
 * has it (a certificate with a provisioning profile for it): 1 and its
 * description ("Apple Development: Name (ID)") in desc; 0 when the build will
 * be an unsigned .ipa. macOS only. */
int rb_ios_signing(char *desc, size_t n);

/* ---- the game's data (srfx.c) ---------------------------------------------------- */
/* cart.dat from the ROM (RB_ROM_SIZE bytes, No-Intro's), the same for every
 * system: malloc'd, its size in *n; NULL with the reason in err. */
uint8_t *rb_cart(const uint8_t *rom, size_t *n, char *err, size_t errlen);

/* ---- supplied by each front end -------------------------------------------------- */
/* where a piece shipped inside the builder is (a file, or a folder):
 * "game" (the game for this system), "shaders" (its shaders' folder), "icon"
 * (the Mac app's AppIcon.icns), "ios-app" (the iPhone app's folder, its
 * shaders in it), "ios-sign", "icon-png" and "icon-argb" (the Linux game's
 * icon for its desktop file, and its window's); 1 on success */
int rb_host_payload(const char *name, char *path, size_t n, char *err, size_t errlen);

/* ---- utilities the core shares (util.c, md5.c) ---------------------------------- */
typedef struct rb_md5 { uint32_t a, b, c, d; uint64_t len; uint8_t buf[64]; } rb_md5;
void rb_md5_init(rb_md5 *m);
void rb_md5_update(rb_md5 *m, const void *p, size_t n);
void rb_md5_final(rb_md5 *m, uint8_t out[16]);
void rb_md5_hex(const uint8_t d[16], char out[33]);

#ifdef __cplusplus
}
#endif
#endif
