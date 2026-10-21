/* host_null.c: a host with no window, no input and no audio device.
 *
 * The game runs exactly as it would on screen; nothing is shown and no key
 * is ever pressed except what a script injects. For tests and for
 * comparisons with the original.
 *
 * PORT_SAVEDIR names the save folder (default build/<dir>/save).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

static char s_save[1024];
static const char *s_app_dir = "game";

void host_set_app_name(const char *dir, const char *title)
{
    (void)title;
    if (dir)
        s_app_dir = dir;
}

void host_init(int argc, char **argv)
{
    const char *e = getenv("PORT_SAVEDIR");
    (void)argc;
    (void)argv;
    if (e) {
        snprintf(s_save, sizeof s_save, "%s", e);
    } else {
        snprintf(s_save, sizeof s_save, "build/%s", s_app_dir);
        host_mkdir("build");
        host_mkdir(s_save);
        snprintf(s_save, sizeof s_save, "build/%s/save", s_app_dir);
    }
    host_mkdir(s_save);
}

void host_shutdown(void) {}

const char *host_resource_dir(void)
{
    const char *e = getenv("PORT_RESDIR");
    return e ? e : ".";
}

const char *host_save_dir(void) { return s_save; }

int host_window_open(int width, int height, const char *title)
{
    (void)width;
    (void)height;
    (void)title;
    return 1;
}

void host_window_close(void) {}
int host_window_visible(void) { return 0; }
/* no window: a renderer draws offscreen */
void *host_window_handle(void) { return NULL; }
void host_window_pixels(int *w, int *h) { *w = *h = 0; }
#ifdef _WIN32
void *host_win32_instance(void) { return NULL; }
#endif

void host_present(const uint32_t *argb, int w, int h, float aspect)
{
    (void)argb;
    (void)w;
    (void)h;
    (void)aspect;
}

int host_poll_event(host_event *ev, uint32_t wait_ms)
{
    (void)ev;
    if (wait_ms && wait_ms != 0xFFFFFFFFu)
        host_sleep_ms(wait_ms);
    return 0;
}

void host_message_box(const char *text, const char *caption)
{
    fprintf(stderr, "%s: %s\n", caption ? caption : "message", text ? text : "");
}

int host_audio_open(int rate, host_audio_fn fn, void *user)
{
    (void)rate;
    (void)fn;
    (void)user;
    return 0;
}

void host_audio_close(void) {}

int host_pad_read(int index, host_pad *p)
{
    (void)index;
    memset(p, 0, sizeof *p);
    return 0;
}
