/*
 * main.c: Stunt Race FX, native: the window, the controls, the sound, the
 * save and the pace, over the decomp's C and the machine layer.
 *
 *   stuntracefx                      play
 *   stuntracefx --headless --script FILE --out DIR [--frames N]
 *                                    run a script of sfxbox's form (wait,
 *                                    hold, press, shot, dump, repeat ...
 *                                    end) with no window and no pacing:
 *                                    shots as DIR/NAME.ppm, dumps as
 *                                    DIR/NAME.wram and DIR/NAME.gsuram
 *   stuntracefx --headless --frames N [--out DIR]
 *                                    N frames with no input
 *
 * Time is the console's: a frame each 1/60.0988 s against the wall clock,
 * the sound drained at the DSP's rate (32,040 Hz) and resampled to the
 * device's, the rate trimmed by up to 0.5% to keep 50 ms buffered.
 *
 * Controls (a game controller in port 1 and 2; the keyboard is port 1):
 *   SNES    keyboard          controller
 *   D-pad   arrows            d-pad or left stick
 *   B, A    Z, X              A, B (the SNES's layout: B below, A right)
 *   Y, X    A, S              X, Y
 *   L, R    Q, W              shoulders
 *   START   Return            menu
 *   SELECT  Right Shift, Tab  view
 *
 * The save is the cartridge's battery RAM (the Super FX's 64 KB), kept in
 * the host's save folder as stuntracefx.srm and written when it changes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "host.h"
#include "snes.h"
#include "video.h"
#include "rr.h"
#include "rr_size.h"
#include "dispatch.h"

/* the ROM, built in (romdata.S: the builder's own ROM) */
extern const uint8_t srfx_rom[];
extern const uint8_t srfx_rom_end[];

const uint8_t *port_rom(void) { return srfx_rom; }

static int headless;
static long max_frames = -1;
static const char *out_dir = ".";
static long frame_no;

/* ---- scripts ---- */
typedef struct {
    int kind;               /* 0 buttons for frames; 1 shot; 2 dump */
    uint16_t buttons[2];
    long frames;
    char name[128];
} Step;

static Step *steps;
static int nsteps, step_at;
static long step_left;

static void add_step(int kind, uint16_t b0, uint16_t b1, long frames, const char *name)
{
    Step *s;
    steps = (Step *)realloc(steps, sizeof *steps * (size_t)(nsteps + 1));
    s = &steps[nsteps++];
    memset(s, 0, sizeof *s);
    s->kind = kind;
    s->buttons[0] = b0;
    s->buttons[1] = b1;
    s->frames = frames;
    if (name)
        snprintf(s->name, sizeof s->name, "%s", name);
}

static int parse_buttons(const char *t, uint16_t *b0, uint16_t *b1)
{
    static const struct { const char *n; uint16_t m; } k[] = {
        { "B", SNES_B }, { "Y", SNES_Y }, { "SELECT", SNES_SELECT }, { "START", SNES_START },
        { "UP", SNES_UP }, { "DOWN", SNES_DOWN }, { "LEFT", SNES_LEFT }, { "RIGHT", SNES_RIGHT },
        { "A", SNES_A }, { "X", SNES_X }, { "L", SNES_L }, { "R", SNES_R },
    };
    char buf[256], *p, *next;
    *b0 = *b1 = 0;
    snprintf(buf, sizeof buf, "%s", t);
    for (p = buf; p; p = next) {
        int pad = 0;
        size_t i;
        next = strchr(p, '+');
        if (next)
            *next++ = 0;
        if (!strncmp(p, "2.", 2)) {
            pad = 1;
            p += 2;
        }
        for (i = 0; i < sizeof k / sizeof k[0]; i++)
            if (!strcmp(p, k[i].n))
                break;
        if (i == sizeof k / sizeof k[0])
            return 0;
        if (pad)
            *b1 |= k[i].m;
        else
            *b0 |= k[i].m;
    }
    return 1;
}

/* lines [lo, hi) of the script, expanded */
static void script_range(char **lines, int lo, int hi, const char *path)
{
    int i;
    for (i = lo; i < hi; i++) {
        char cmd[64] = "", a[256] = "", b[64] = "";
        uint16_t b0, b1;
        if (sscanf(lines[i], "%63s %255s %63s", cmd, a, b) < 1)
            continue;
        if (!strcmp(cmd, "wait")) {
            add_step(0, 0, 0, atol(a), NULL);
        } else if (!strcmp(cmd, "hold") || !strcmp(cmd, "press")) {
            if (!parse_buttons(a, &b0, &b1))
                snes_fatal("%s:%d: buttons '%s'", path, i + 1, a);
            add_step(0, b0, b1, b[0] ? atol(b) : (cmd[0] == 'p' ? 2 : 1), NULL);
            if (cmd[0] == 'p')
                add_step(0, 0, 0, 2, NULL);
        } else if (!strcmp(cmd, "shot")) {
            add_step(1, 0, 0, 0, a);
        } else if (!strcmp(cmd, "dump")) {
            add_step(2, 0, 0, 0, a);
        } else if (!strcmp(cmd, "repeat")) {
            int depth = 0, j, n = atoi(a);
            for (j = i + 1; j < hi; j++) {
                char c2[64] = "";
                sscanf(lines[j], "%63s", c2);
                if (!strcmp(c2, "repeat"))
                    depth++;
                else if (!strcmp(c2, "end") && depth-- == 0)
                    break;
            }
            while (n-- > 0)
                script_range(lines, i + 1, j, path);
            i = j;
        } else {
            snes_fatal("%s:%d: '%s' is not something the port's runner does (load, save, poke, set and call need the emulator)",
                       path, i + 1, cmd);
        }
    }
}

static void script_load(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[512], **lines = NULL;
    int n = 0;
    if (!f)
        snes_fatal("cannot read %s", path);
    while (fgets(line, sizeof line, f)) {
        char *h = strchr(line, '#');
        if (h)
            *h = 0;
        lines = (char **)realloc(lines, sizeof *lines * (size_t)(n + 1));
        lines[n++] = strdup(line);
    }
    fclose(f);
    script_range(lines, 0, n, path);
}

static void write_file(const char *name, const char *ext, const void *p, size_t n)
{
    char path[1024];
    FILE *f;
    snprintf(path, sizeof path, "%s/%s%s", out_dir, name, ext);
    f = fopen(path, "wb");
    if (!f)
        snes_fatal("cannot write %s", path);
    fwrite(p, 1, n, f);
    fclose(f);
}

static void shot(const char *name)
{
    const uint32_t *px = vid_picture();
    char path[1024];
    FILE *f;
    int i;
    snprintf(path, sizeof path, "%s/%s.ppm", out_dir, name);
    f = fopen(path, "wb");
    if (!f)
        snes_fatal("cannot write %s", path);
    fprintf(f, "P6\n%d %d\n255\n", SNES_WIDTH, SNES_HEIGHT);
    for (i = 0; i < SNES_WIDTH * SNES_HEIGHT; i++) {
        uint8_t rgb[3] = { (uint8_t)(px[i] >> 16), (uint8_t)(px[i] >> 8), (uint8_t)px[i] };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

/* the shots and dumps due now, at the end of a frame: those after the
   frames a step has just run out, as the harness takes them */
static void script_takes(void)
{
    while (step_at < nsteps && steps[step_at].kind) {
        Step *s = &steps[step_at];
        if (s->kind == 1)
            shot(s->name);
        else {
            write_file(s->name, ".wram", snes_wram(), 0x20000);
            write_file(s->name, ".gsuram", snes_sfx_ram(), 0x10000);
        }
        step_at++;
    }
}

/* this frame's buttons from the script; 0 when it has ended */
static int script_frame(uint16_t *b0, uint16_t *b1)
{
    while (step_at < nsteps) {
        Step *s = &steps[step_at];
        if (s->kind) {
            script_takes();
            continue;
        }
        if (!step_left)
            step_left = s->frames;
        if (step_left <= 0) {
            step_at++;
            step_left = 0;
            continue;
        }
        *b0 = s->buttons[0];
        *b1 = s->buttons[1];
        if (--step_left == 0)
            step_at++;
        return 1;
    }
    return 0;
}

/* ---- the save ---- */
static uint8_t saved[0x10000];
static char save_path[1024];

static void save_load(void)
{
    FILE *f;
    /* a headless run (a test) starts with no save and keeps none: the
       player's save is never read or written by one */
    if (headless)
        return;
    snprintf(save_path, sizeof save_path, "%s/stuntracefx.srm", host_save_dir());
    f = fopen(save_path, "rb");
    if (!f)
        return;
    if (fread(snes_sfx_ram(), 1, 0x10000, f) == 0x10000)
        memcpy(saved, snes_sfx_ram(), sizeof saved);
    fclose(f);
}

/* the records live at $70:FC00 up; the rest of the RAM is the Super FX's
   work, cleared at each start: written when the records changed */
static void save_write(int force)
{
    char tmp[1100];
    FILE *f;
    if (headless)
        return;
    if (!force && !memcmp(saved + 0xFC00, snes_sfx_ram() + 0xFC00, 0x400))
        return;
    memcpy(saved, snes_sfx_ram(), sizeof saved);
    snprintf(tmp, sizeof tmp, "%s.new", save_path);
    f = fopen(tmp, "wb");
    if (!f)
        return;
    fwrite(saved, 1, sizeof saved, f);
    fclose(f);
    remove(save_path);
    rename(tmp, save_path);
}

/* ---- input ---- */
static uint16_t keys;

static void key(int vk, int down)
{
    uint16_t m = 0;
    switch (vk) {
    case 0x26: m = SNES_UP; break;
    case 0x28: m = SNES_DOWN; break;
    case 0x25: m = SNES_LEFT; break;
    case 0x27: m = SNES_RIGHT; break;
    case 'Z': m = SNES_B; break;
    case 'X': m = SNES_A; break;
    case 'A': m = SNES_Y; break;
    case 'S': m = SNES_X; break;
    case 'Q': m = SNES_L; break;
    case 'W': m = SNES_R; break;
    case 0x0D: m = SNES_START; break;
    case 0x10: case 0x09: m = SNES_SELECT; break;
    }
    if (down)
        keys |= m;
    else
        keys &= (uint16_t)~m;
}

static uint16_t pad_buttons(int index)
{
    host_pad p;
    uint16_t b = 0;
    if (!host_pad_read(index, &p))
        return 0;
    if (p.buttons & 1u << 0) b |= SNES_B;
    if (p.buttons & 1u << 1) b |= SNES_A;
    if (p.buttons & 1u << 2) b |= SNES_Y;
    if (p.buttons & 1u << 3) b |= SNES_X;
    if (p.buttons & 1u << 4) b |= SNES_L;
    if (p.buttons & 1u << 5) b |= SNES_R;
    if (p.buttons & 1u << 6) b |= SNES_SELECT;
    if (p.buttons & 1u << 7) b |= SNES_START;
    if ((p.buttons & 1u << 12) || p.y < -0.5f) b |= SNES_UP;
    if ((p.buttons & 1u << 13) || p.x > 0.5f) b |= SNES_RIGHT;
    if ((p.buttons & 1u << 14) || p.y > 0.5f) b |= SNES_DOWN;
    if ((p.buttons & 1u << 15) || p.x < -0.5f) b |= SNES_LEFT;
    return b;
}

/* ---- sound: the DSP's 32,040 Hz to the device's rate ---- */
#define OUT_RATE 48000
static double src_pos;
static int16_t src_buf[4096 * 2];
static int src_n, src_at;

static void audio_cb(float *lr, int frames, void *user)
{
    int q = snes_audio_queued(), i;
    double target = 32040 * 0.05, step;
    (void)user;
    step = 32040.0 / OUT_RATE * (1.0 + 0.005 * ((q - target) / target > 1 ? 1 : (q - target) / target < -1 ? -1 : (q - target) / target));
    for (i = 0; i < frames; i++) {
        double f;
        int16_t *a, *b;
        while (src_at + 1 >= src_n) {
            int keep = src_n - src_at, got;
            if (keep > 0)
                memmove(src_buf, src_buf + src_at * 2, (size_t)keep * 4);
            else
                keep = 0;
            src_n = keep;
            src_at = 0;
            got = snes_audio_take(src_buf + src_n * 2, 4096 - src_n);
            if (got <= 0)
                break;
            src_n += got;
        }
        if (src_at + 1 >= src_n) {
            lr[2 * i] = lr[2 * i + 1] = 0;
            continue;
        }
        f = src_pos;
        a = src_buf + src_at * 2;
        b = a + 2;
        lr[2 * i] = (float)((a[0] + (b[0] - a[0]) * f) / 32768.0);
        lr[2 * i + 1] = (float)((a[1] + (b[1] - a[1]) * f) / 32768.0);
        src_pos += step;
        while (src_pos >= 1.0) {
            src_pos -= 1.0;
            src_at++;
        }
    }
}

/* ---- the pacing record (--pacing FILE) ----
   At each race frame's start, where the frames since the last one ($0306)
   go to $06D4: the video frame, the count, and hashes of work RAM (the
   stack, $0100-$02FF, left out) and of the Super FX's RAM: the lines
   sfxbox --pacing writes (its second field the original's STA; here 0). */
static FILE *pacing_f;

static uint32_t fnv(const uint8_t *p, size_t n, uint32_t h)
{
    while (n--)
        h = (h ^ *p++) * 16777619u;
    return h;
}

static void pacing_write(uint32_t addr, uint8_t value)
{
    uint8_t bank = (uint8_t)(addr >> 16);
    const uint8_t *w = snes_wram();
    (void)value;
    if ((addr & 0xFFFF) != 0x06D4 || !(bank == 0x7E || (bank & 0x7F) < 0x40))
        return;
    fprintf(pacing_f, "%ld %06X %u %08x %08x\n", frame_no, 0u, w[0x306],
            fnv(w + 0x300, 0x20000 - 0x300, fnv(w, 0x100, 2166136261u)),
            fnv(snes_sfx_ram(), 0x10000, 2166136261u));
}

/* ---- each frame ---- */
static uint64_t next_due;
#define FRAME_NS 16639267ull          /* 1 / 60.0988 s */

static void quit(void)
{
    save_write(0);
    host_shutdown();
    exit(0);
}

/* PORT_RRCHECK=N: at frame N, the frame the renderer presented, read back,
   against the reference drawing of the same picture (rr_size.h); the
   largest difference in any channel printed, then the port ends (exit 1
   when it is over 2) */
static void rr_self_check(int tw, int th, const int d[4])
{
    static long at = -2;
    uint32_t *got, *want;
    size_t i, n = (size_t)tw * (size_t)th;
    int maxd = 0;
    if (at == -2)
        at = getenv("PORT_RRCHECK") ? atol(getenv("PORT_RRCHECK")) : -1;
    if (at < 0 || frame_no != at)
        return;
    got = (uint32_t *)malloc(n * 4);
    want = (uint32_t *)malloc(n * 4);
    if (!got || !want || !rr_read(got))
        exit(2);
    for (i = 0; i < n; i++)
        want[i] = 0xFF000000u;
    rr_ref_image(want, tw, th, vid_picture(), VID_WIDTH, VID_HEIGHT, d);
    for (i = 0; i < n; i++) {
        int sh;
        for (sh = 0; sh < 24; sh += 8) {
            int a = (int)((got[i] >> sh) & 0xFF) - (int)((want[i] >> sh) & 0xFF);
            if (a < 0)
                a = -a;
            if (a > maxd)
                maxd = a;
        }
    }
    fprintf(stderr, "rrcheck: %s, frame %ld, %dx%d: largest difference %d\n", rr_name(), frame_no, tw, th, maxd);
    exit(maxd > 2);
}

/* the frame on the window: the picture at the original's 4:3 (its 256 x
   224 pixels, 8:7 each), as large as the window holds, centred, sharp
   bilinear */
static void show(void)
{
    int tw, th, w, h, d[4];
    rr_target(&tw, &th);
    w = th * 4 / 3;
    h = th;
    if (w > tw) {
        w = tw;
        h = tw * 3 / 4;
    }
    d[0] = (tw - w) / 2;
    d[1] = (th - h) / 2;
    d[2] = d[0] + w;
    d[3] = d[1] + h;
    rr_begin(0xFF000000u);
    rr_image(vid_picture(), VID_WIDTH, VID_HEIGHT, d);
    rr_present();
    rr_self_check(tw, th, d);
}

static void frame(void)
{
    uint16_t b0 = 0, b1 = 0;
    frame_no++;
    if (headless) {
        int more = script_frame(&b0, &b1);
        snes_set_joypad(0, b0);
        snes_set_joypad(1, b1);
        if ((max_frames >= 0 && frame_no >= max_frames) || (!more && nsteps && max_frames < 0)) {
            save_write(0);
            exit(0);
        }
        return;
    }
    show();
    {
        host_event e;
        while (host_poll_event(&e, 0)) {
            if (e.type == HOST_EV_KEY)
                key(e.vk, e.down);
            else if (e.type == HOST_EV_CLOSE)
                quit();
            else if (e.type == HOST_EV_FOCUS && !e.down)
                keys = 0;
        }
    }
    snes_set_joypad(0, (uint16_t)(keys | pad_buttons(0)));
    snes_set_joypad(1, pad_buttons(1));
    if (frame_no % 120 == 0)
        save_write(0);
    /* the pace: each frame due 1/60.0988 s after the last */
    {
        uint64_t now = host_ticks_ns();
        if (!next_due || now > next_due + 100000000ull)
            next_due = now;
        next_due += FRAME_NS;
        while ((now = host_ticks_ns()) < next_due) {
            uint64_t left = next_due - now;
            if (left > 2000000ull)
                host_sleep_ms((uint32_t)(left / 1000000ull) - 1);
        }
    }
}

int main(int argc, char **argv)
{
    static SnesCart cart;
    const char *script = NULL;
    int i;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--headless"))
            headless = 1;
        else if (!strcmp(argv[i], "--script") && i + 1 < argc)
            script = argv[++i];
        else if (!strcmp(argv[i], "--out") && i + 1 < argc)
            out_dir = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc)
            max_frames = atol(argv[++i]);
        else if (!strcmp(argv[i], "--pacing") && i + 1 < argc) {
            if (!(pacing_f = fopen(argv[++i], "w"))) {
                fprintf(stderr, "port: cannot write %s\n", argv[i]);
                return 2;
            }
            setvbuf(pacing_f, NULL, _IOLBF, 0);
            snes_write_hook = pacing_write;
        }
    }
    host_set_app_name("Stunt Race FX", "Stunt Race FX");
    host_init(argc, argv);
    if (script)
        script_load(script);
    cart.rom = srfx_rom;
    cart.rom_size = (uint32_t)(srfx_rom_end - srfx_rom);
    cart.superfx = 1;
    cart.sfx_ram_size = 0x10000;
    snes_power(&cart);
    save_load();
    if (headless && script) {
        /* The harness gives a step's first frame its buttons before that
           frame runs; here they are set at the end of the frame before.
           The first frame has none before it: it is taken now (its buttons
           reach no joypad read), with the shots and dumps before it. */
        uint16_t b0, b1;
        script_takes();
        script_frame(&b0, &b1);
    }
    snes_frame_hook = frame;
    if (!headless) {
        host_thread_interactive();
        host_window_open(SNES_WIDTH, SNES_HEIGHT, "Stunt Race FX");
        if (!rr_open()) {
            host_message_box("The renderer could not start on this computer.", "Stunt Race FX");
            return 1;
        }
        host_audio_open(OUT_RATE, audio_cb, NULL);
    }
    port_start();
    return 0;
}
