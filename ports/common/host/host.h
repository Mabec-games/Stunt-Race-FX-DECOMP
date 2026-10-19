/* host.h: what each operating system supplies under a native port.
 *
 * A port's game code and its machine layer (ports/common/snes) are written
 * once, for every OS, against this small interface. Each OS implements only
 * this:
 *
 *   host/posix    time, files (macOS, Linux, iOS)
 *   host/win32    the same on Windows
 *   host/null     a headless window, input and audio (tests, comparisons)
 *   host/macos    a Cocoa window, keyboard and game controllers, Core Audio
 *   host/windows  a Win32 window, keyboard, XInput controllers, WASAPI
 *
 * An app that brings its own host (an iOS app, a builder) links none of the
 * window hosts and supplies these functions itself.
 *
 * The same interface as the Boss Rally and Top Gear Rally ports' hosts, less
 * what only those games need (a CD, the network, decoded music files).
 */
#ifndef PORT_HOST_H
#define PORT_HOST_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- time ----------------------------------------------------------------- */
uint64_t host_ticks_ns(void);                 /* monotonic, arbitrary origin */
void     host_sleep_ms(uint32_t ms);
/* the calling thread keeps time for the game (paces frames): ask the OS to
   wake it on time (no timer coalescing) */
void     host_thread_interactive(void);

/* ---- files ------------------------------------------------------------------ */
int         host_mkdir(const char *path);
/* where the player's saves and settings go (made by host_init) */
const char *host_save_dir(void);
/* where the program's own files are: a macOS app's Resources, else the
 * executable's folder (PORT_RESDIR overrides) */
const char *host_resource_dir(void);

/* ---- window and input --------------------------------------------------------- */
enum {
    HOST_EV_NONE,
    HOST_EV_KEY,          /* vk (a Win32 virtual key code on every OS), down */
    HOST_EV_FOCUS,        /* down: 1 gained, 0 lost */
    HOST_EV_CLOSE
};
typedef struct host_event {
    int type;
    int vk, down;
} host_event;

/* the window, sized for a picture of width x height (shown larger, the
   picture's shape kept) */
int  host_window_open(int width, int height, const char *title);
void host_window_close(void);
/* 1 while any of the window can be seen (not minimised, hidden or covered) */
int  host_window_visible(void);
/* the next event, waiting up to wait_ms for one; 1 if *ev was filled */
int  host_poll_event(host_event *ev, uint32_t wait_ms);
void host_message_box(const char *text, const char *caption);
/* a finished picture of ARGB pixels (0xAARRGGBB), top row first, shown
   scaled to the window; aspect is the shape it is shown at (width over
   height: 4/3 for a television's picture) */
void host_present(const uint32_t *argb, int w, int h, float aspect);

/* ---- audio out ------------------------------------------------------------------ */
/* the callback fills frames of interleaved stereo float, from the host's
   own audio thread */
typedef void (*host_audio_fn)(float *lr, int frames, void *user);
/* opened at rate frames a second; 0 when there is no audio device */
int  host_audio_open(int rate, host_audio_fn fn, void *user);
void host_audio_close(void);

/* ---- game controllers ------------------------------------------------------------- */
/* game controller `index` (0 the first), in an XInput pad's layout:
 *   x, y     left stick, -1..1, y positive downward
 *   buttons  0 A  1 B  2 X  3 Y  4 LB  5 RB  6 view  7 menu  8 L3  9 R3
 *            10 LT  11 RT (past half)  12-15 d-pad up/right/down/left
 * 0 when none is connected there (the state then reads centred) */
typedef struct host_pad {
    float    x, y;
    unsigned buttons;
} host_pad;
int host_pad_read(int index, host_pad *p);

/* ---- process ---------------------------------------------------------------------- */
/* the program's name, before host_init: dir names the save folder, title
   the window and dialogs */
void host_set_app_name(const char *dir, const char *title);
void host_init(int argc, char **argv);
void host_shutdown(void);

#ifdef __cplusplus
}
#endif
#endif
