/* host_windows.c: the Windows host -- a window, the keyboard and game
 * controllers, sound out. The counterpart of host/macos/host_macos.m;
 * host/win32 under it supplies time and files.
 *
 * The game keeps its own loop on the main thread, so this host never runs a
 * message loop of its own: host_poll_event drains the real queue. Frames
 * are drawn on the window by the renderer (render/gl or render/vulkan,
 * host_window_handle), or arrive as ARGB pixels through host_present (the
 * software renderer), shown scaled with StretchDIBits.
 *
 *   keys      a key message's virtual key
 *   sound     WASAPI, shared mode, 32-bit float, on its own thread
 *   pads      XInput's controllers
 *
 * PORT_SAVEDIR names the save folder (default %APPDATA%\<app>). A GUI exe
 * has no console: what the game reports goes to log.txt in the save folder
 * (PORT_LOGFILE names another file; 0 keeps stderr).
 */
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#define CINTERFACE
#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <shlobj.h>
#include <xinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"

static char s_save[MAX_PATH];
static const char *s_app_dir = "Game", *s_app_title = "Game";

void host_set_app_name(const char *dir, const char *title)
{
    if (dir)
        s_app_dir = dir;
    if (title)
        s_app_title = title;
}

/* ---- process -------------------------------------------------------------------- */
static void exe_dir(char *out, size_t n)
{
    char *s;
    GetModuleFileNameA(NULL, out, (DWORD)n);
    s = strrchr(out, '\\');
    if (s)
        *s = 0;
}

static void log_open(void)
{
    char path[MAX_PATH + 16];
    const char *e = getenv("PORT_LOGFILE");
    if (e && e[0] == '0')
        return;
    if (e)
        snprintf(path, sizeof path, "%s", e);
    else
        snprintf(path, sizeof path, "%s\\log.txt", s_save);
    if (freopen(path, "w", stderr))
        setvbuf(stderr, NULL, _IONBF, 0);
}

void host_init(int argc, char **argv)
{
    const char *e;
    char here[MAX_PATH];
    (void)argc;
    (void)argv;
    exe_dir(here, sizeof here);
    e = getenv("PORT_SAVEDIR");
    if (e) {
        snprintf(s_save, sizeof s_save, "%s", e);
    } else {
        char app[MAX_PATH];
        if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, app) == S_OK)
            snprintf(s_save, sizeof s_save, "%s\\%s", app, s_app_dir);
        else
            snprintf(s_save, sizeof s_save, "%s\\save", here);
    }
    host_mkdir(s_save);
    log_open();
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
}

void host_shutdown(void)
{
    host_audio_close();
    host_window_close();
}

const char *host_save_dir(void) { return s_save; }

const char *host_resource_dir(void)
{
    static char here[MAX_PATH];
    const char *e = getenv("PORT_RESDIR");
    if (e)
        return e;
    if (!here[0])
        exe_dir(here, sizeof here);
    return here;
}

/* ---- the event queue the window fills and host_poll_event drains ------------------- */
#define QMAX 256
static host_event s_q[QMAX];
static int s_qh, s_qt;
static uint8_t s_held[256];          /* by virtual key: held */

static void qpush(const host_event *e)
{
    int n = (s_qt + 1) % QMAX;
    if (n != s_qh) {
        s_q[s_qt] = *e;
        s_qt = n;
    }
}

static void key_event(WPARAM vk, int down)
{
    host_event e;
    if (vk >= 256 || s_held[vk] == down)
        return;                       /* auto-repeat */
    s_held[vk] = (uint8_t)down;
    memset(&e, 0, sizeof e);
    e.type = HOST_EV_KEY;
    e.vk = (int)vk;
    e.down = down;
    qpush(&e);
}

/* ---- the window ----------------------------------------------------------------- */
static HWND s_win;

static LRESULT CALLBACK wnd_proc(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
    host_event e;
    memset(&e, 0, sizeof e);
    switch (m) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        key_event(wp, 1);
        if (m == WM_SYSKEYDOWN && wp != VK_F4)
            return 0;                 /* Alt alone must not open the window menu */
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        key_event(wp, 0);
        return 0;
    case WM_SETFOCUS:
    case WM_KILLFOCUS: {
        int i;
        if (m == WM_KILLFOCUS)        /* keys held when focus leaves go up */
            for (i = 0; i < 256; i++)
                if (s_held[i])
                    key_event((WPARAM)i, 0);
        e.type = HOST_EV_FOCUS;
        e.down = m == WM_SETFOCUS;
        qpush(&e);
        return 0;
    }
    case WM_CLOSE:
        e.type = HOST_EV_CLOSE;
        qpush(&e);
        return 0;                     /* the game decides */
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProcA(h, m, wp, lp);
}

int host_window_open(int width, int height, const char *title)
{
    WNDCLASSA wc;
    RECT r;
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (s_win)
        return 1;
    memset(&wc, 0, sizeof wc);
    wc.style = CS_OWNDC;              /* (an OpenGL context keeps its DC) */
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hIcon = LoadIconA(wc.hInstance, MAKEINTRESOURCEA(1));
    wc.lpszClassName = "PortGameWindow";
    RegisterClassA(&wc);
    SetProcessDPIAware();
    r.left = r.top = 0;
    r.right = width * 3;
    r.bottom = height * 3;
    AdjustWindowRect(&r, style, FALSE);
    s_win = CreateWindowA("PortGameWindow", title ? title : s_app_title, style, CW_USEDEFAULT, CW_USEDEFAULT,
                          r.right - r.left, r.bottom - r.top, NULL, NULL, wc.hInstance, NULL);
    if (!s_win)
        return 0;
    ShowWindow(s_win, SW_SHOW);
    SetForegroundWindow(s_win);
    return 1;
}

void host_window_close(void)
{
    if (s_win) {
        DestroyWindow(s_win);
        s_win = NULL;
    }
}

int host_window_visible(void)
{
    return s_win && IsWindowVisible(s_win) && !IsIconic(s_win);
}

/* the window for a renderer that draws on it (render/gl, render/vulkan):
   its HWND, its client area in pixels, the module */
void *host_window_handle(void) { return (void *)s_win; }
void *host_win32_instance(void) { return (void *)GetModuleHandleA(NULL); }

void host_window_pixels(int *w, int *h)
{
    RECT r;
    *w = *h = 0;
    if (s_win && GetClientRect(s_win, &r)) {
        *w = r.right - r.left;
        *h = r.bottom - r.top;
    }
}

void host_present(const uint32_t *argb, int w, int h, float aspect)
{
    BITMAPINFO bi;
    RECT r;
    HDC dc;
    int dw, dh, ox, oy;
    if (!s_win || !argb)
        return;
    if (aspect <= 0)
        aspect = (float)w / (float)h;
    memset(&bi, 0, sizeof bi);
    bi.bmiHeader.biSize = sizeof bi.bmiHeader;
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;        /* top row first */
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    GetClientRect(s_win, &r);
    /* letterboxed at the picture's shape */
    dw = r.right;
    dh = (int)(r.right / aspect + 0.5f);
    if (dh > r.bottom) {
        dh = r.bottom;
        dw = (int)(r.bottom * aspect + 0.5f);
    }
    ox = (r.right - dw) / 2;
    oy = (r.bottom - dh) / 2;
    dc = GetDC(s_win);
    if (ox > 0) {
        PatBlt(dc, 0, 0, ox, r.bottom, BLACKNESS);
        PatBlt(dc, ox + dw, 0, r.right - ox - dw, r.bottom, BLACKNESS);
    }
    if (oy > 0) {
        PatBlt(dc, 0, 0, r.right, oy, BLACKNESS);
        PatBlt(dc, 0, oy + dh, r.right, r.bottom - oy - dh, BLACKNESS);
    }
    SetStretchBltMode(dc, COLORONCOLOR);
    StretchDIBits(dc, ox, oy, dw, dh, 0, 0, w, h, argb, &bi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(s_win, dc);
}

int host_poll_event(host_event *ev, uint32_t wait_ms)
{
    MSG m;
    while (s_qh == s_qt) {
        if (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&m);
            DispatchMessageA(&m);
            continue;
        }
        if (wait_ms == 0)
            return 0;
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, wait_ms == 0xFFFFFFFFu ? INFINITE : wait_ms, QS_ALLINPUT) == WAIT_TIMEOUT)
            return 0;
        wait_ms = 0;
    }
    *ev = s_q[s_qh];
    s_qh = (s_qh + 1) % QMAX;
    return 1;
}

void host_message_box(const char *text, const char *caption)
{
    MessageBoxA(s_win, text ? text : "", caption ? caption : s_app_title, MB_OK);
}

/* ---- audio out: WASAPI ------------------------------------------------------------ */
/* KSDATAFORMAT_SUBTYPE_IEEE_FLOAT (ksmedia.h), spelled here */
static const GUID k_subtype_float = { 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };
static IAudioClient        *s_ac;
static IAudioRenderClient  *s_rc;
static HANDLE               s_aev, s_athread;
static volatile LONG        s_astop;
static host_audio_fn        s_afn;
static void                *s_auser;
static UINT32               s_abuf;
static int                  s_ach;

static DWORD WINAPI audio_main(LPVOID p)
{
    float tmp[2 * 4096];
    (void)p;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    while (!s_astop) {
        UINT32 pad = 0, n, k;
        BYTE *buf;
        WaitForSingleObject(s_aev, 100);
        if (FAILED(IAudioClient_GetCurrentPadding(s_ac, &pad)))
            continue;
        n = s_abuf - pad;
        while (n > 0) {
            UINT32 m = n > 4096 ? 4096 : n;
            if (FAILED(IAudioRenderClient_GetBuffer(s_rc, m, &buf)))
                break;
            s_afn(tmp, (int)m, s_auser);
            /* the mixer is stereo; the device may have more channels */
            for (k = 0; k < m; k++) {
                float *o = (float *)buf + (size_t)k * (size_t)s_ach;
                int c;
                o[0] = tmp[2 * k];
                if (s_ach > 1)
                    o[1] = tmp[2 * k + 1];
                for (c = 2; c < s_ach; c++)
                    o[c] = 0;
            }
            IAudioRenderClient_ReleaseBuffer(s_rc, m, 0);
            n -= m;
        }
    }
    return 0;
}

int host_audio_open(int rate, host_audio_fn fn, void *user)
{
    IMMDeviceEnumerator *en = NULL;
    IMMDevice *dev = NULL;
    WAVEFORMATEXTENSIBLE wf;
    WAVEFORMATEX *mix = NULL;
    if (s_ac)
        return 1;
    if (FAILED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void **)&en)))
        return 0;
    if (FAILED(IMMDeviceEnumerator_GetDefaultAudioEndpoint(en, eRender, eConsole, &dev))) {
        IMMDeviceEnumerator_Release(en);
        return 0;
    }
    IMMDeviceEnumerator_Release(en);
    if (FAILED(IMMDevice_Activate(dev, &IID_IAudioClient, CLSCTX_ALL, NULL, (void **)&s_ac))) {
        IMMDevice_Release(dev);
        return 0;
    }
    IMMDevice_Release(dev);
    IAudioClient_GetMixFormat(s_ac, &mix);
    /* float at the mixer's rate; the shared-mode engine converts */
    memset(&wf, 0, sizeof wf);
    wf.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    wf.Format.nChannels = (WORD)(mix && mix->nChannels ? mix->nChannels : 2);
    wf.Format.nSamplesPerSec = (DWORD)rate;
    wf.Format.wBitsPerSample = 32;
    wf.Format.nBlockAlign = (WORD)(wf.Format.nChannels * 4);
    wf.Format.nAvgBytesPerSec = wf.Format.nSamplesPerSec * wf.Format.nBlockAlign;
    wf.Format.cbSize = 22;
    wf.Samples.wValidBitsPerSample = 32;
    wf.dwChannelMask = wf.Format.nChannels == 2 ? (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT)
                       : mix && mix->wFormatTag == WAVE_FORMAT_EXTENSIBLE ? ((WAVEFORMATEXTENSIBLE *)mix)->dwChannelMask : 0;
    wf.SubFormat = k_subtype_float;
    if (mix)
        CoTaskMemFree(mix);
    s_ach = wf.Format.nChannels;
    if (FAILED(IAudioClient_Initialize(s_ac, AUDCLNT_SHAREMODE_SHARED,
                                       AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
                                       AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
                                       500000, 0, (WAVEFORMATEX *)&wf, NULL)) ||
        FAILED(IAudioClient_GetBufferSize(s_ac, &s_abuf)) ||
        FAILED(IAudioClient_GetService(s_ac, &IID_IAudioRenderClient, (void **)&s_rc))) {
        IAudioClient_Release(s_ac);
        s_ac = NULL;
        return 0;
    }
    s_aev = CreateEventA(NULL, FALSE, FALSE, NULL);
    IAudioClient_SetEventHandle(s_ac, s_aev);
    s_afn = fn;
    s_auser = user;
    s_astop = 0;
    IAudioClient_Start(s_ac);
    s_athread = CreateThread(NULL, 0, audio_main, NULL, 0, NULL);
    return 1;
}

void host_audio_close(void)
{
    if (!s_ac)
        return;
    InterlockedExchange(&s_astop, 1);
    SetEvent(s_aev);
    WaitForSingleObject(s_athread, INFINITE);
    CloseHandle(s_athread);
    IAudioClient_Stop(s_ac);
    IAudioRenderClient_Release(s_rc);
    IAudioClient_Release(s_ac);
    CloseHandle(s_aev);
    s_ac = NULL;
    s_rc = NULL;
}

/* ---- game controllers: XInput -------------------------------------------------- */
int host_pad_read(int index, host_pad *o)
{
    XINPUT_STATE st;
    const XINPUT_GAMEPAD *g;
    unsigned b = 0;
    DWORD i;
    int n = 0;
    memset(o, 0, sizeof *o);
    for (i = 0; i < XUSER_MAX_COUNT; i++)
        if (XInputGetState(i, &st) == ERROR_SUCCESS && n++ == index)
            break;
    if (i == XUSER_MAX_COUNT)
        return 0;
    g = &st.Gamepad;
    o->x = g->sThumbLX / 32767.0f;
    o->y = -g->sThumbLY / 32767.0f;
    if (o->x < -1) o->x = -1;
    if (o->y < -1) o->y = -1;
    if (g->wButtons & XINPUT_GAMEPAD_A) b |= 1u << 0;
    if (g->wButtons & XINPUT_GAMEPAD_B) b |= 1u << 1;
    if (g->wButtons & XINPUT_GAMEPAD_X) b |= 1u << 2;
    if (g->wButtons & XINPUT_GAMEPAD_Y) b |= 1u << 3;
    if (g->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) b |= 1u << 4;
    if (g->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) b |= 1u << 5;
    if (g->wButtons & XINPUT_GAMEPAD_BACK) b |= 1u << 6;
    if (g->wButtons & XINPUT_GAMEPAD_START) b |= 1u << 7;
    if (g->wButtons & XINPUT_GAMEPAD_LEFT_THUMB) b |= 1u << 8;
    if (g->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) b |= 1u << 9;
    if (g->bLeftTrigger > 127) b |= 1u << 10;
    if (g->bRightTrigger > 127) b |= 1u << 11;
    if (g->wButtons & XINPUT_GAMEPAD_DPAD_UP) b |= 1u << 12;
    if (g->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) b |= 1u << 13;
    if (g->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) b |= 1u << 14;
    if (g->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) b |= 1u << 15;
    o->buttons = b;
    return 1;
}
