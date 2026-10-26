/*
 * host_sdl3.c: a window, keyboard, game controllers and audio through SDL3
 * (Linux, and any OS SDL3 runs on), beside host/posix for time and files.
 * As host_sdl.c (SDL2): the picture comes whole (host_present), shown
 * scaled to the window with its shape kept; keys reach the game as Win32
 * virtual key codes. PORT_FULLSCREEN=1 opens it full screen; Alt+Enter
 * switches.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <SDL3/SDL.h>

#include "host.h"

static char s_dir[256] = "port", s_title[256] = "port";
static char s_save[1024], s_res[1024];
static SDL_Window *s_win;
static SDL_Renderer *s_ren;
static SDL_Texture *s_tex;
static int s_tex_w, s_tex_h;
static SDL_AudioStream *s_audio;
static host_audio_fn s_audio_fn;
static void *s_audio_user;
static SDL_Gamepad *s_pads[4];
static int s_alt;

void host_set_app_name(const char *dir, const char *title)
{
    snprintf(s_dir, sizeof s_dir, "%s", dir);
    snprintf(s_title, sizeof s_title, "%s", title);
}

void host_init(int argc, char **argv)
{
    const char *x = getenv("XDG_DATA_HOME"), *home = getenv("HOME");
    char exe[1024];
    ssize_t n;
    (void)argc;
    (void)argv;
    SDL_SetHint(SDL_HINT_APP_ID, "stuntracefx");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD))
        fprintf(stderr, "host: SDL could not start: %s\n", SDL_GetError());
    if (x && *x)
        snprintf(s_save, sizeof s_save, "%s/%s", x, s_dir);
    else {
        char base[1024];
        snprintf(base, sizeof base, "%s/.local/share", home ? home : ".");
        host_mkdir(base);
        snprintf(s_save, sizeof s_save, "%s/%s", base, s_dir);
    }
    host_mkdir(s_save);
    if (getenv("PORT_RESDIR"))
        snprintf(s_res, sizeof s_res, "%s", getenv("PORT_RESDIR"));
    else if ((n = readlink("/proc/self/exe", exe, sizeof exe - 1)) > 0) {
        char *slash;
        exe[n] = 0;
        if ((slash = strrchr(exe, '/')))
            *slash = 0;
        snprintf(s_res, sizeof s_res, "%s", exe);
    } else
        snprintf(s_res, sizeof s_res, ".");
}

void host_shutdown(void)
{
    host_audio_close();
    host_window_close();
    SDL_Quit();
}

const char *host_save_dir(void) { return s_save; }
const char *host_resource_dir(void) { return s_res; }

/* ---- window ---- */
int host_window_open(int width, int height, const char *title)
{
    int scale = 3;
    s_win = SDL_CreateWindow(title ? title : s_title, width * scale * 7 / 6, height * scale,
                             SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!s_win)
        return 0;
    s_ren = SDL_CreateRenderer(s_win, NULL);
    if (!s_ren)
        return 0;
    /* the game paces its own frames (60.0988 a second): the display's
       vertical sync only on request (PORT_VSYNC=1) */
    SDL_SetRenderVSync(s_ren, getenv("PORT_VSYNC") && !strcmp(getenv("PORT_VSYNC"), "1") ? 1 : 0);
    if (getenv("PORT_FULLSCREEN") && !strcmp(getenv("PORT_FULLSCREEN"), "1"))
        SDL_SetWindowFullscreen(s_win, true);
    return 1;
}

void host_window_close(void)
{
    if (s_tex)
        SDL_DestroyTexture(s_tex);
    if (s_ren)
        SDL_DestroyRenderer(s_ren);
    if (s_win)
        SDL_DestroyWindow(s_win);
    s_tex = NULL;
    s_ren = NULL;
    s_win = NULL;
}

int host_window_visible(void)
{
    return s_win && !(SDL_GetWindowFlags(s_win) & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED));
}

void *host_window_handle(void) { return s_win; }

void host_window_pixels(int *w, int *h)
{
    if (!s_ren || !SDL_GetCurrentRenderOutputSize(s_ren, w, h))
        *w = *h = 0;
}

void host_present(const uint32_t *argb, int w, int h, float aspect)
{
    int ow, oh, dw, dh;
    SDL_FRect d;
    if (!s_ren)
        return;
    if (!s_tex || s_tex_w != w || s_tex_h != h) {
        if (s_tex)
            SDL_DestroyTexture(s_tex);
        s_tex = SDL_CreateTexture(s_ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
        /* a picture larger than the console's (the high-resolution 3D):
           smoothed to the window's size; the console's own: sharp */
        if (s_tex)
            SDL_SetTextureScaleMode(s_tex, w > 256 ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
        s_tex_w = w;
        s_tex_h = h;
    }
    SDL_UpdateTexture(s_tex, NULL, argb, w * 4);
    SDL_GetCurrentRenderOutputSize(s_ren, &ow, &oh);
    if (aspect <= 0)
        aspect = (float)w / (float)h;
    dh = oh;
    dw = (int)(oh * aspect + 0.5f);
    if (dw > ow) {
        dw = ow;
        dh = (int)(ow / aspect + 0.5f);
    }
    d.x = (float)((ow - dw) / 2);
    d.y = (float)((oh - dh) / 2);
    d.w = (float)dw;
    d.h = (float)dh;
    SDL_SetRenderDrawColor(s_ren, 0, 0, 0, 255);
    SDL_RenderClear(s_ren);
    SDL_RenderTexture(s_ren, s_tex, NULL, &d);
    SDL_RenderPresent(s_ren);
}

/* ---- events ---- */
static int vk_of(SDL_Keycode k)
{
    if (k >= SDLK_A && k <= SDLK_Z)
        return 'A' + (int)(k - SDLK_A);
    if (k >= SDLK_0 && k <= SDLK_9)
        return '0' + (int)(k - SDLK_0);
    if (k >= SDLK_F1 && k <= SDLK_F12)
        return 0x70 + (int)(k - SDLK_F1);
    switch (k) {
    case SDLK_LEFT: return 0x25;
    case SDLK_UP: return 0x26;
    case SDLK_RIGHT: return 0x27;
    case SDLK_DOWN: return 0x28;
    case SDLK_RETURN: case SDLK_KP_ENTER: return 0x0D;
    case SDLK_LSHIFT: case SDLK_RSHIFT: return 0x10;
    case SDLK_TAB: return 0x09;
    case SDLK_ESCAPE: return 0x1B;
    case SDLK_SPACE: return 0x20;
    case SDLK_BACKSPACE: return 0x08;
    }
    return 0;
}

static void pad_added(SDL_JoystickID which)
{
    int i;
    if (!SDL_IsGamepad(which))
        return;
    for (i = 0; i < 4; i++)
        if (!s_pads[i]) {
            s_pads[i] = SDL_OpenGamepad(which);
            return;
        }
}

static void pad_removed(SDL_JoystickID id)
{
    int i;
    for (i = 0; i < 4; i++)
        if (s_pads[i] && SDL_GetGamepadID(s_pads[i]) == id) {
            SDL_CloseGamepad(s_pads[i]);
            s_pads[i] = NULL;
        }
}

int host_poll_event(host_event *ev, uint32_t wait_ms)
{
    SDL_Event e;
    bool got = wait_ms ? SDL_WaitEventTimeout(&e, (Sint32)wait_ms) : SDL_PollEvent(&e);
    while (got) {
        switch (e.type) {
        case SDL_EVENT_QUIT:
            ev->type = HOST_EV_CLOSE;
            ev->vk = ev->down = 0;
            return 1;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            if (e.key.key == SDLK_LALT || e.key.key == SDLK_RALT)
                s_alt = e.key.down;
            if (e.key.repeat)
                break;
            if (e.key.down && s_alt && (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER)) {
                SDL_SetWindowFullscreen(s_win, !(SDL_GetWindowFlags(s_win) & SDL_WINDOW_FULLSCREEN));
                break;
            }
            ev->type = HOST_EV_KEY;
            ev->vk = vk_of(e.key.key);
            ev->down = e.key.down;
            if (ev->vk)
                return 1;
            break;
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            ev->type = HOST_EV_FOCUS;
            ev->vk = 0;
            ev->down = e.type == SDL_EVENT_WINDOW_FOCUS_GAINED;
            if (!ev->down)
                s_alt = 0;
            return 1;
        case SDL_EVENT_GAMEPAD_ADDED:
            pad_added(e.gdevice.which);
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            pad_removed(e.gdevice.which);
            break;
        }
        got = SDL_PollEvent(&e);
    }
    return 0;
}

void host_message_box(const char *text, const char *caption)
{
    if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, caption, text, s_win))
        fprintf(stderr, "%s: %s\n", caption, text);
}

/* ---- audio ---- */
static void audio_cb(void *user, SDL_AudioStream *stream, int additional, int total)
{
    static float buf[4096];
    (void)user;
    (void)total;
    while (additional > 0) {
        int bytes = additional < (int)sizeof buf ? additional : (int)sizeof buf;
        int frames = bytes / (int)(2 * sizeof(float));
        if (frames <= 0)
            break;
        if (s_audio_fn)
            s_audio_fn(buf, frames, s_audio_user);
        else
            memset(buf, 0, (size_t)frames * 2 * sizeof(float));
        SDL_PutAudioStreamData(stream, buf, frames * (int)(2 * sizeof(float)));
        additional -= frames * (int)(2 * sizeof(float));
    }
}

int host_audio_open(int rate, host_audio_fn fn, void *user)
{
    SDL_AudioSpec want;
    want.freq = rate;
    want.format = SDL_AUDIO_F32;
    want.channels = 2;
    s_audio_fn = fn;
    s_audio_user = user;
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, "512");
    s_audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want, audio_cb, NULL);
    if (!s_audio)
        return 0;
    SDL_ResumeAudioStreamDevice(s_audio);
    return 1;
}

void host_audio_close(void)
{
    if (s_audio)
        SDL_DestroyAudioStream(s_audio);
    s_audio = NULL;
}

/* ---- game controllers, in an XInput pad's layout ---- */
int host_pad_read(int index, host_pad *p)
{
    SDL_Gamepad *c;
    static const SDL_GamepadButton map[] = {
        SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_WEST, SDL_GAMEPAD_BUTTON_NORTH,
        SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_GAMEPAD_BUTTON_BACK,
        SDL_GAMEPAD_BUTTON_START, SDL_GAMEPAD_BUTTON_LEFT_STICK, SDL_GAMEPAD_BUTTON_RIGHT_STICK
    };
    unsigned i;
    p->x = p->y = 0;
    p->buttons = 0;
    if (index < 0 || index >= 4 || !(c = s_pads[index]))
        return 0;
    p->x = SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0f;
    p->y = SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0f;
    for (i = 0; i < sizeof map / sizeof map[0]; i++)
        if (SDL_GetGamepadButton(c, map[i]))
            p->buttons |= 1u << i;
    if (SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > 16384)
        p->buttons |= 1u << 10;
    if (SDL_GetGamepadAxis(c, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 16384)
        p->buttons |= 1u << 11;
    if (SDL_GetGamepadButton(c, SDL_GAMEPAD_BUTTON_DPAD_UP)) p->buttons |= 1u << 12;
    if (SDL_GetGamepadButton(c, SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) p->buttons |= 1u << 13;
    if (SDL_GetGamepadButton(c, SDL_GAMEPAD_BUTTON_DPAD_DOWN)) p->buttons |= 1u << 14;
    if (SDL_GetGamepadButton(c, SDL_GAMEPAD_BUTTON_DPAD_LEFT)) p->buttons |= 1u << 15;
    return 1;
}
