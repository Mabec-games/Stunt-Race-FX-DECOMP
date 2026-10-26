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
 * the sound drained at the mixer's rate (32,040 Hz) and resampled to the
 * device's (windowed sinc), the rate trimmed by up to 0.5% to keep 50 ms
 * buffered.
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
#include "resample.h"
#include "dispatch.h"
#include "smoke.h"
#include "hires.h"

/* the ROM, built in (romdata.S: the builder's own ROM) */
extern const uint8_t srfx_rom[];
extern const uint8_t srfx_rom_end[];

/* the ROM: in the executable, or (a build without it, ROM_EXTERNAL=1)
   the player's own, read at start (--rom PATH, else SRFX_ROM) */
static const uint8_t *rom_data;
static uint32_t rom_len;
const uint8_t *port_rom(void) { return rom_data ? rom_data : srfx_rom; }
uint32_t port_rom_size(void) { return rom_data ? rom_len : (uint32_t)(srfx_rom_end - srfx_rom); }

/* SHA-1 (the ROM's check) */
static void sha1(const uint8_t *m, size_t n, uint8_t out[20])
{
    uint32_t h[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 }, w[80];
    uint64_t bits = (uint64_t)n * 8;
    size_t total = ((n + 8) / 64 + 1) * 64, off;
    int i;
    for (off = 0; off < total; off += 64) {
        uint32_t a, b, c, d, e;
        for (i = 0; i < 64; i++) {
            size_t k = off + (size_t)i;
            uint8_t v = k < n ? m[k] : k == n ? 0x80 : k >= total - 8 ? (uint8_t)(bits >> (8 * (total - 1 - k))) : 0;
            if (!(i & 3))
                w[i >> 2] = 0;
            w[i >> 2] |= (uint32_t)v << (24 - 8 * (i & 3));
        }
        for (i = 16; i < 80; i++) {
            uint32_t x = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
            w[i] = x << 1 | x >> 31;
        }
        a = h[0]; b = h[1]; c = h[2]; d = h[3]; e = h[4];
        for (i = 0; i < 80; i++) {
            uint32_t f, k, tmp;
            if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6; }
            tmp = (a << 5 | a >> 27) + f + e + k + w[i];
            e = d; d = c; c = b << 30 | b >> 2; b = a; a = tmp;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
    }
    for (i = 0; i < 20; i++)
        out[i] = (uint8_t)(h[i >> 2] >> (24 - 8 * (i & 3)));
}

/* the player's ROM at path: read, a copier's 512-byte header left out,
   checked (Stunt Race FX (USA) (Rev 1)); 0 with *why set when not */
static int rom_load(const char *path, const char **why)
{
    static const uint8_t want[20] = { 0x78, 0x1e, 0x37, 0x6f, 0xf4, 0x53, 0xa8, 0x3c, 0x7b, 0xcf,
                                      0x72, 0x33, 0x9a, 0xbe, 0x6b, 0x84, 0x2e, 0xfd, 0x9c, 0x86 };
    FILE *f = fopen(path, "rb");
    long n;
    uint8_t *b, got[20];
    if (!f) {
        *why = "The ROM file could not be opened.";
        return 0;
    }
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0 || n > 16 * 1024 * 1024 || !(b = (uint8_t *)malloc((size_t)n)) || fread(b, 1, (size_t)n, f) != (size_t)n) {
        fclose(f);
        *why = "The ROM file could not be read.";
        return 0;
    }
    fclose(f);
    if (n % 1024 == 512) {
        memmove(b, b + 512, (size_t)n - 512);
        n -= 512;
    }
    sha1(b, (size_t)n, got);
    if (memcmp(got, want, 20)) {
        free(b);
        *why = "This is not Stunt Race FX (USA) (Rev 1). Its SHA-1 must be 781e376ff453a83c7bcf72339abe6b842efd9c86.";
        return 0;
    }
    rom_data = b;
    rom_len = (uint32_t)n;
    return 1;
}

static int headless;
static long max_frames = -1;
static const char *out_dir = ".";
static long frame_no;

/* ---- scripts ---- */
typedef struct {
    int kind;               /* 0 buttons for frames; 1 shot; 2 dump; 3 poke (frames: the address, name: the bytes) */
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
        } else if (!strcmp(cmd, "poke") && getenv("PORT_SCRIPT_POKE")) {
            /* poke ADDR BB,BB...: bytes into work RAM ($7E-$7F) or the
               Super FX's RAM ($70-$71), no time taken (the tests' own
               shortcuts, as sfxbox's poke; PORT_SCRIPT_POKE=1, the
               reference port having none: same.py runs as before) */
            long at = strtol(a, NULL, 16);
            if (!b[0] || !((at >> 16) == 0x70 || (at >> 16) == 0x71 || (at >> 16) == 0x7E || (at >> 16) == 0x7F))
                snes_fatal("%s:%d: poke ADDR BYTES (banks $70-$71, $7E-$7F)", path, i + 1);
            add_step(3, 0, 0, at, b);
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
   frames a step has just run out, as the harness takes them. PORT_IRQMASK:
   each dump also as DIR/NAME.irq, the first 8 KB's bytes whose last write
   was an interrupt's frame (1 each), which same.py leaves out */
static void script_takes(void)
{
    while (step_at < nsteps && steps[step_at].kind) {
        Step *s = &steps[step_at];
        if (s->kind == 1)
            shot(s->name);
        else if (s->kind == 3) {
            const char *p = s->name;
            long at = s->frames;
            while (*p) {
                uint8_t v = (uint8_t)strtol(p, (char **)&p, 16);
                if ((at >> 16) >= 0x7E)
                    ((uint8_t *)snes_wram())[at & 0x1FFFF] = v;
                else
                    snes_sfx_ram()[at & 0xFFFF] = v;
                at++;
                if (*p == ',')
                    p++;
                else
                    break;
            }
        } else {
            write_file(s->name, ".wram", snes_wram(), 0x20000);
            write_file(s->name, ".gsuram", snes_sfx_ram(), 0x10000);
            if (getenv("PORT_IRQMASK"))
                write_file(s->name, ".irq", snes_wram_irq(), 0x2000);
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

void interp_live_set(int on);
int interp_live(void);
void interp_speed_set(int ticks);
extern int port_steady_ticks;
void interp_log_open(const char *dir);
void interp_log_input(uint16_t buttons);
int interp_lookahead_child(void);       /* the look-ahead's copy of the game (interp.c) */
void interp_lookahead_child_frame(void);
void interp_lookahead_input(uint16_t b0, uint16_t b1);
void interp_lookahead_set(int on);
int interp_lookahead(void);
void interp_lookahead_wait(double ms);
void interp_hires_set(int scale);       /* the high-resolution 3D (interp.c): 0 off, else the scale */
int interp_hires(void);
const uint32_t *interp_picture_hr(int *w, int *h);
static int speed_idx = 1;               /* F3's place: natural 10 (with the look-ahead) first */

int interp_wide(void);
void interp_wide_set(int on);
extern uint16_t port_far_cars;          /* ObjectNearFocus's far distance (game/object/object.c) */

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
    case 0x72:                      /* F3: the game's speed: natural 60, natural 10, steady 15, 20, 30, the original */
        if (down) {
            int mode;
            static const int ticks[] = { -60, 6, 4, 3, 2, 0 };
            static const char *name[] = { "natural 60 (experimental: the cars' physics at 60 a second, the original's pace)",
                "natural 10 (the original game at its average pace, no slowdown; 60 fps picture with F2, look-ahead with F4)", "steady 15", "steady 20", "steady 30 (turbo)",
                "original (as on the console)" };
            mode = speed_idx = (speed_idx + 1) % 6;
            interp_speed_set(ticks[mode]);
            fprintf(stderr, "game speed: %s\n", name[mode]);
        }
        break;
    case 0x73:                      /* F4: the look-ahead (the next race frames run early on a copy): 3, 2, 1, off */
        if (down) {
            interp_lookahead_set((interp_lookahead() + 3) % 4);
            if (interp_lookahead())
                fprintf(stderr, "look-ahead: %d race frame%s ahead\n", interp_lookahead(), interp_lookahead() > 1 ? "s" : "");
            else
                fprintf(stderr, "look-ahead off\n");
        }
        break;
    case 0x77:                      /* F8: the wider view (the 3D over the bezel, to the picture's edges) */
        if (down) {
            interp_wide_set(!interp_wide());
            fprintf(stderr, "view: %s\n", interp_wide() ? "wide (the 3D over the bezel)" : "the original's window");
        }
        break;
    case 0x76:                      /* F7: the other cars in 3D farther out (4x the original's distance), or as the original */
        if (down) {
            port_far_cars = port_far_cars == 0x1194 ? 0x4650 : 0x1194;
            fprintf(stderr, "other cars: %s\n", port_far_cars == 0x1194 ? "sprites when far (original)" : "3D 4x farther out");
        }
        break;
    case 0x75:                      /* F6: small shapes (far fine detail) as they are, or widened */
        if (down) {
            hires_set_small(!hires_get_small());
            fprintf(stderr, "small 3D shapes: %s\n", hires_get_small() ? "as they are (fine detail averaged)" : "widened and grown (default)");
        }
        break;
    case 0x74:                      /* F5: the 3D at high resolution: off, 2x, 3x, 4x */
        if (down) {
            int s = interp_hires();
            interp_hires_set(s == 0 ? 2 : s >= 4 ? 0 : s + 1);
            if (interp_hires())
                fprintf(stderr, "3D at %dx resolution\n", interp_hires());
            else
                fprintf(stderr, "3D at the original resolution\n");
        }
        break;
    case 0x71:                      /* F2: the in-between frames on or off */
        if (down) {
            interp_live_set(!interp_live());
            fprintf(stderr, "in-between frames %s\n", interp_live() ? "on" : "off");
        }
        break;
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

/* ---- sound: the mixer's 32,040 Hz to the device's rate ----
   The windowed-sinc resampler (ports/common/audio/resample.c), its ratio
   trimmed by up to 0.5% to keep 50 ms of the mixer's output queued. */
#define OUT_RATE 48000
static Resampler *s_rs;

static void audio_cb(float *lr, int frames, void *user)
{
    int16_t in[2048 * 2];
    int got, made, q;
    double target = 32040 * 0.05, d;
    (void)user;
    while (resample_queued(s_rs) < 12000 && (got = snes_audio_take(in, 2048)) > 0)
        resample_push(s_rs, in, got);
    q = resample_queued(s_rs) + snes_audio_queued();
    d = (q - target) / target;
    resample_adjust(s_rs, 0.005 * (d > 1 ? 1 : d < -1 ? -1 : d));
    made = resample_pull(s_rs, lr, frames);
    if (made < frames)
        memset(lr + 2 * made, 0, (size_t)(frames - made) * 2 * sizeof *lr);
}

/* ---- the pacing record (--pacing FILE) ----
   At each race frame's start, where the frames since the last one ($0306)
   go to $06D4: the video frame, the count, and hashes of work RAM (the
   stack, $0100-$02FF, left out) and of the Super FX's RAM: the lines
   sfxbox --pacing writes (its second field the original's STA; here 0). */
static FILE *pacing_f;
/* --sound FILE: the DSP's output from power-on, 16-bit stereo, left first
   (raw, 32,040 frames a second), as sfxbox --sound writes it */
static FILE *sound_f;

static void sound_drain(void)
{
    int16_t b[2048 * 2];
    int n;
    while ((n = snes_audio_take(b, 2048)) > 0)
        fwrite(b, 4, (size_t)n, sound_f);
}

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
    if (interp_lookahead_child())
        return;
    if ((addr & 0xFFFF) != 0x06D4 || !(bank == 0x7E || (bank & 0x7F) < 0x40))
        return;
    if (getenv("PORT_PACING_GAME")) {
        /* (a check) the game's state without what only the picture
           follows: the draw list ($0CEE, $7E:F77F), the count the wait
           for the Super FX's draw makes ($1F21), and the Super FX RAM */
        static uint8_t c[0x20000];
        memcpy(c, w, sizeof c);
        memset(c + 0x0CEE, 0, 2);
        memset(c + 0x1F21, 0, 2);
        memset(c + 0xF77F, 0, 0x31 * 0x1E);
        fprintf(pacing_f, "%ld %06X %u %08x\n", frame_no, 0u, w[0x306],
                fnv(c + 0x300, 0x20000 - 0x300, fnv(c, 0x100, 2166136261u)));
        if (getenv("PORT_PACING_DUMP")) {
            static FILE *df;
            static int nd;
            if (!df)
                df = fopen(getenv("PORT_PACING_DUMP"), "wb");
            if (df && nd++ < 4000)
                fwrite(c, 1, sizeof c, df);
        }
        return;
    }
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

/* the frame's record as the renderer's compositor takes it */
static uint32_t pic_lines[(VID_HEIGHT + 1) * RR_LINE_WORDS];

static void picture_of(const VidFrame *f, RrPicture *p)
{
    unsigned line;
    for (line = 1; line <= VID_HEIGHT; line++)
        vid_pack_line(f, line, &pic_lines[line * RR_LINE_WORDS]);
    p->vram = &f->vram[0][0];
    p->external = &f->external[0][0];
    p->oam = &f->oam[0][0];
    p->versions = f->versions;
    p->palette = &f->palette[0][0];
    p->lines = pic_lines;
}

void canvas_gpu(void);
void canvas_gpu_apply(const VidFrame *f);
extern const uint8_t srfx_rom[], srfx_rom_end[];

/* the 3D canvases fed from the game's screen operations, where the
   renderer has them */
static void canvases_start(void)
{
    if (rr_fill_rom(port_rom(), port_rom_size()))
        canvas_gpu();
}

/* the renderer opened once for the headless checks, its target the
   picture's size: 0 when it cannot open */
int port_rr_headless(void)
{
    static int open = -1;
    if (open < 0) {
#ifdef _WIN32
        _putenv("RR_SIZE=256x224");
#else
        setenv("RR_SIZE", "256x224", 1);
#endif
        open = rr_open();
    }
    return open;
}

/* PORT_GPUCHECK=1 (headless): each finished frame composed by the
   renderer's GPU compositor, one to one, read back and compared with the
   picture drawn from the same record (video.c); at exit, the frames and
   pixels that differ */
static long gpu_frames = -1, gpu_bad, gpu_pixels, gpu_first = -1;

static void gpu_report(void)
{
    fprintf(stderr, "gpucheck: %s, %ld frames, %ld differ (first at frame %ld), %ld pixels\n", rr_name(), gpu_frames,
            gpu_bad, gpu_first, gpu_pixels);
}

static void gpu_check(void)
{
    static uint32_t *got, want[VID_WIDTH * VID_HEIGHT];
    static const int d[4] = { 0, 0, VID_WIDTH, VID_HEIGHT };
    RrPicture p;
    int tw, th, x, y;
    long bad = 0;
    if (gpu_frames < 0) {
        if (!port_rr_headless()) {
            fprintf(stderr, "gpucheck: the renderer cannot open\n");
            exit(2);
        }
        canvases_start();
        atexit(gpu_report);
        gpu_frames = 0;
    }
    rr_target(&tw, &th);
    if (!got)
        got = (uint32_t *)malloc((size_t)tw * (size_t)th * 4);
    picture_of(vid_frame(), &p);
    canvas_gpu_apply(vid_frame());
    rr_begin(0xFF000000u);
    rr_compose(&p, d);
    rr_present();
    rr_read(got);
    vid_draw_frame(vid_frame(), want);
    for (y = 0; y < VID_HEIGHT; y++)
        for (x = 0; x < VID_WIDTH; x++)
            if ((got[y * tw + x] | 0xFF000000u) != want[y * VID_WIDTH + x])
                bad++;
    gpu_frames++;
    if (bad) {
        gpu_bad++;
        gpu_pixels += bad;
        if (gpu_first < 0)
            gpu_first = frame_no;
    }
}

/* the frame on the window: the picture composed on the GPU from the
   frame's record, at the original's 4:3 (its 256 x 224 pixels, 8:7 each),
   as large as the window holds, centred, sharp bilinear */
const uint32_t *interp_picture(void);   /* the in-between frame to show now, or NULL (interp.c) */

/* PORT_BAR_LOG=1 (a check, the window): a picture with a thin black
   strip the one before did not have, saved with its lines' display
   registers to the session log's folder (bar-N.ppm, bar-N.txt), 20 at most */
static void bar_log(const uint32_t *px)
{
    static int on = -1, saved;
    static uint32_t last[VID_WIDTH * VID_HEIGHT];
    static int dark_last[VID_HEIGHT];
    int dark[VID_HEIGHT], y, x, hit = -1;
    if (on < 0)
        on = getenv("PORT_BAR_LOG") && getenv("PORT_LOG_DIR");
    if (!on || saved >= 20)
        return;
    for (y = 0; y < VID_HEIGHT; y++) {
        dark[y] = 0;
        for (x = 0; x < VID_WIDTH; x++) {
            uint32_t c = px[y * VID_WIDTH + x];
            dark[y] += ((c >> 16 & 0xFF) + (c >> 8 & 0xFF) + (c & 0xFF)) < 30;
        }
    }
    for (y = 4; y < VID_HEIGHT - 4 && hit < 0; y++)
        if (dark[y] - dark_last[y] > 40 && dark[y - 4] - dark_last[y - 4] < 10 && dark[y + 4] - dark_last[y + 4] < 10)
            hit = y;
    if (hit >= 0) {
        char path[1024];
        FILE *f;
        const VidFrame *fr = vid_frame();
        snprintf(path, sizeof path, "%s/bar-%ld.ppm", getenv("PORT_LOG_DIR"), frame_no);
        if ((f = fopen(path, "wb"))) {
            int i;
            fprintf(f, "P6\n%d %d\n255\n", VID_WIDTH, VID_HEIGHT * 2);
            for (i = 0; i < VID_WIDTH * VID_HEIGHT * 2; i++) {
                uint32_t c = i < VID_WIDTH * VID_HEIGHT ? last[i] : px[i - VID_WIDTH * VID_HEIGHT];
                uint8_t rgb[3] = { (uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c };
                fwrite(rgb, 1, 3, f);
            }
            fclose(f);
        }
        snprintf(path, sizeof path, "%s/bar-%ld.txt", getenv("PORT_LOG_DIR"), frame_no);
        if ((f = fopen(path, "w"))) {
            fprintf(f, "frame %ld, snes frame %u, strip at line %d (the picture before above, this one below)\n", frame_no, snes_frame(), hit);
            for (y = 1; y <= VID_HEIGHT; y++)
                fprintf(f, "line %3d: dark %3d (before %3d) inidisp %02X tm %02X bgmode %02X\n", y, dark[y - 1], dark_last[y - 1],
                        fr->line[y].inidisp, fr->line[y].tm, fr->line[y].bgmode);
            fclose(f);
        }
        saved++;
        fprintf(stderr, "bar log: a strip at line %d, frame %ld saved\n", hit, frame_no);
    }
    memcpy(last, px, sizeof last);
    memcpy(dark_last, dark, sizeof dark_last);
}

static void show(void)
{
    int tw, th, w, h, d[4];
    RrPicture p;
    if (!strcmp(rr_name(), "soft")) {
        /* no compositor of its own: the reference picture, whole, scaled by
           the host. The race blanks its top lines in the picture it uploads
           the screen's last third in (hidden by a television's overscan):
           a top row all black that was not, kept as it was */
        static uint32_t shown[VID_WIDTH * VID_HEIGHT];
        static uint8_t held[VID_HEIGHT], was_seen[VID_HEIGHT];
        const uint32_t *in = interp_picture();
        int y, x, hw, hh;
        const uint32_t *hr = interp_picture_hr(&hw, &hh);
        if (hr) {
            /* the 3D at high resolution (the picture S times larger) */
            host_present(hr, hw, hh, 4.0f / 3.0f);
            return;
        }
        if (!in)
            in = vid_picture();
        for (y = 0; y < VID_HEIGHT; y++) {
            const uint32_t *r = in + y * VID_WIDTH;
            int black = 1;
            for (x = 0; black && x < VID_WIDTH; x++)
                black = (r[x] & 0xFFFFFF) == 0;
            /* a row seen in the last picture and black now: held, two pictures at most */
            if (black && y < 40 && was_seen[y] && held[y] < 2) {
                held[y]++;
                continue;
            }
            memcpy(shown + y * VID_WIDTH, r, VID_WIDTH * 4);
            held[y] = 0;
            was_seen[y] = !black;
        }
        bar_log(shown);
        host_present(shown, VID_WIDTH, VID_HEIGHT, 4.0f / 3.0f);
        return;
    }
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
    picture_of(vid_frame(), &p);
    canvas_gpu_apply(vid_frame());
    rr_begin(0xFF000000u);
    rr_compose(&p, d);
    rr_present();
    rr_self_check(tw, th, d);
}

void interp_frame(long frame);   /* the in-between frames (interp.c) */
void hires_frame(void);          /* the high-resolution capture (hires.c) */
void interp_live_set(int on);
int interp_live(void);

/* the window's controls, to the joypads (and the look-ahead) when they
   changed, or always (each frame: the session log's) */
static void window_input(int always)
{
    static uint16_t was0, was1;
    uint16_t b0, b1;
    host_event e;
    if (interp_lookahead_child())
        return;     /* a copy (forked on a press below): the window is the game's alone */
    while (host_poll_event(&e, 0)) {
        if (e.type == HOST_EV_KEY)
            key(e.vk, e.down);
        else if (e.type == HOST_EV_CLOSE)
            quit();
        else if (e.type == HOST_EV_FOCUS && !e.down)
            keys = 0;
    }
    b0 = (uint16_t)(keys | pad_buttons(0));
    b1 = pad_buttons(1);
    if (!always && b0 == was0 && b1 == was1)
        return;
    was0 = b0;
    was1 = b1;
    snes_set_joypad(0, b0);
    snes_set_joypad(1, b1);
    if (always)
        interp_log_input(b0);
    interp_lookahead_input(b0, b1);
}

static void frame(void)
{
    uint16_t b0 = 0, b1 = 0;
    if (interp_lookahead_child()) {
        interp_lookahead_child_frame();     /* a copy: nothing shown, the controls held */
        return;
    }
    frame_no++;
    interp_frame(frame_no);
    hires_frame();
    if (headless) {
        int more;
        const uint32_t *picture = interp_picture();
        smoke_present(frame_no, picture);   /* NULL: the game's (drawn only if traced) */
        {
            /* PORT_HIRES_SHOT=F1,F2,...: frame Fn's picture as shown (the
               high-resolution one, else the in-between, else the game's) to
               <out>/hires-Fn.ppm (a check) */
            const char *e = getenv("PORT_HIRES_SHOT");
            int hw, hh;
            const uint32_t *hr;
            while (e && *e) {
                if (atol(e) == frame_no && ((hr = interp_picture_hr(&hw, &hh)) ||
                                            ((hr = interp_picture()) ? (hw = VID_WIDTH, hh = VID_HEIGHT) : 0) ||
                                            (hr = vid_picture(), hw = VID_WIDTH, hh = VID_HEIGHT))) {
                    char path[512];
                    FILE *f;
                    long i;
                    snprintf(path, sizeof path, "%s/hires-%ld.ppm", out_dir ? out_dir : ".", frame_no);
                    if ((f = fopen(path, "wb"))) {
                        fprintf(f, "P6\n%d %d\n255\n", hw, hh);
                        for (i = 0; i < (long)hw * hh; i++) {
                            uint8_t px[3] = { (uint8_t)(hr[i] >> 16), (uint8_t)(hr[i] >> 8), (uint8_t)hr[i] };
                            fwrite(px, 1, 3, f);
                        }
                        fclose(f);
                    }
                }
                e = strchr(e, ',');
                e = e ? e + 1 : NULL;
            }
        }
        static int gc = -1;
        if (gc < 0)
            gc = getenv("PORT_GPUCHECK") != NULL;
        if (gc && frame_no > 1)
            gpu_check();
        if (sound_f)
            sound_drain();
        more = script_frame(&b0, &b1);
        {
            /* PORT_AUTODRIVE=1 (a check): from the race clock's start ($0E5B
               set), the same driving every run whatever the game's speed:
               B 80 frames, B+LEFT 14, B 80, B+RIGHT 14, over and over */
            static int ad = -1;
            static long ad_start;
            if (ad < 0)
                ad = getenv("PORT_AUTODRIVE") != NULL;
            if (ad && snes_wram()[0x0E5B]) {
                long t;
                if (!ad_start)
                    ad_start = frame_no;
                t = (frame_no - ad_start) % 188;
                b0 = (uint16_t)(SNES_B | (t >= 80 && t < 94 ? SNES_LEFT : t >= 174 ? SNES_RIGHT : 0));
                if (getenv("PORT_AUTODRIVE")[0] == '2')
                    b0 = SNES_B;          /* 2: full throttle, straight on */
                b1 = 0;
            }
        }
        smoke_input(frame_no, &b0, &b1);
        snes_set_joypad(0, b0);
        snes_set_joypad(1, b1);
        interp_log_input(b0);
        interp_lookahead_input(b0, b1);
        if ((max_frames >= 0 && frame_no >= max_frames) || (!more && nsteps && max_frames < 0)) {
            save_write(0);
            exit(0);
        }
        {
            /* PORT_REALTIME=1 (a check): the window's pace and its wait for
               the look-ahead, with no window */
            static int rt = -1;
            if (rt < 0) {
                rt = getenv("PORT_REALTIME") != NULL;
                if (rt)
                    interp_lookahead_wait(6.0);
            }
            if (rt) {
                uint64_t now = host_ticks_ns();
                if (!next_due || now > next_due + 100000000ull)
                    next_due = now;
                next_due += FRAME_NS;
                while ((now = host_ticks_ns()) < next_due)
                    if (next_due - now > 2000000ull)
                        host_sleep_ms((uint32_t)((next_due - now) / 1000000ull) - 1);
            }
        }
        return;
    }
    show();
    window_input(1);
    if (interp_lookahead_child())
        return;     /* a copy forked just now: no saving, no waiting */
    if (frame_no % 120 == 0)
        save_write(0);
    /* the pace: each frame due 1/60.0988 s after the last; the controls
       read all the while (a press during the wait starts the look-ahead's
       copy on it at once, not at the next frame) */
    {
        uint64_t now = host_ticks_ns();
        if (!next_due || now > next_due + 100000000ull)
            next_due = now;
        next_due += FRAME_NS;
        while ((now = host_ticks_ns()) < next_due && !interp_lookahead_child()) {
            uint64_t left = next_due - now;
            if (left > 2000000ull)
                host_sleep_ms(1);
            window_input(0);
        }
    }
}

/* PORT_VIDCHECK=1: the frames drawn from their records against the
   picture drawn a line at a time (video.c), at exit */
extern unsigned long vid_check_frames, vid_check_bad, vid_version_overflow;
extern long port_far_lists, port_far_cars_drawn, port_far_refused, port_far_full;
static void far_report(void)
{
    if (port_far_lists && !interp_lookahead_child())
        fprintf(stderr, "far cars: %ld draw lists rebuilt with %ld cars in 3D; %ld kept as the game's (a register touched), %ld with the farthest left out (the list full)\n",
                port_far_lists, port_far_cars_drawn, port_far_refused, port_far_full);
}

static void vidcheck_report(void)
{
    if (getenv("PORT_VIDCHECK"))
        fprintf(stderr, "vidcheck: %lu frames, %lu differ; %lu needed more copies than kept\n", vid_check_frames,
                vid_check_bad, vid_version_overflow);
}

int main(int argc, char **argv)
{
    atexit(vidcheck_report);
    atexit(far_report);
    static SnesCart cart;
    const char *script = NULL, *rom_path = NULL;
    int i;
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--headless"))
            headless = 1;
        else if (!strcmp(argv[i], "--rom") && i + 1 < argc)
            rom_path = argv[++i];
        else if (!strcmp(argv[i], "--script") && i + 1 < argc)
            script = argv[++i];
        else if (!strcmp(argv[i], "--out") && i + 1 < argc)
            out_dir = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc)
            max_frames = atol(argv[++i]);
        else if (!strcmp(argv[i], "--sound") && i + 1 < argc) {
            if (!(sound_f = fopen(argv[++i], "wb"))) {
                fprintf(stderr, "port: cannot write %s\n", argv[i]);
                return 2;
            }
        }
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
    {
        const char *rp = rom_path ? rom_path : getenv("SRFX_ROM"), *why = NULL;
        if (!port_rom_size() || rp) {
            if (!rp || !rom_load(rp, &why)) {
                char msg[512];
                snprintf(msg, sizeof msg, "%s\n\nStart the game from the launcher and choose your Stunt Race FX ROM.",
                         rp ? why : "No ROM was given.");
                fprintf(stderr, "port: %s\n", msg);
                host_message_box(msg, "Stunt Race FX");
                return 1;
            }
        }
    }
    cart.rom = port_rom();
    cart.rom_size = port_rom_size();
    cart.superfx = 1;
    cart.sfx_ram_size = 0x10000;
    snes_power(&cart);
    save_load();
    smoke_init(headless);
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
    /* PORT_FAR_CARS: the other cars in 3D out to this distance (|dX| +
       |dZ|; 4500 the original's, off); the window's default 4x that */
    if (getenv("PORT_FAR_CARS"))
        port_far_cars = (uint16_t)strtol(getenv("PORT_FAR_CARS"), NULL, 0);
    else if (!headless)
        port_far_cars = 0x4650;
    if (headless && getenv("PORT_INTERP_LIVE") && !strcmp(getenv("PORT_INTERP_LIVE"), "1"))
        interp_live_set(1);       /* a check of the live in-between frames */
    if (headless && getenv("PORT_HIRES"))
        interp_hires_set(atoi(getenv("PORT_HIRES")));   /* a check of the high-resolution 3D */
    if (headless && getenv("PORT_LOG_DIR"))
        interp_log_open(getenv("PORT_LOG_DIR"));    /* a check of the session log */
    if (headless && getenv("PORT_SPEED")) {
        const char *e = getenv("PORT_SPEED");
        interp_speed_set(!strcmp(e, "natural60") ? -60 : !strcmp(e, "original") ? 0 : !strcmp(e, "max") ? -1 : atoi(e) > 0 ? 60 / atoi(e) : 4);
        if (getenv("PORT_GSU_SPEED"))
            gsu_set_speed(atof(getenv("PORT_GSU_SPEED")));
        if (getenv("PORT_CPU_SPEED"))
            snes_set_cpu_speed(atof(getenv("PORT_CPU_SPEED")));
    }
    if (!headless) {
        interp_live_set(!getenv("PORT_INTERP_LIVE") || strcmp(getenv("PORT_INTERP_LIVE"), "0"));
        interp_lookahead_wait(6.0);
        /* PORT_HIRES: the 3D at S times the resolution (default 3; 0 the original's) */
        interp_hires_set(getenv("PORT_HIRES") ? atoi(getenv("PORT_HIRES")) : 3);
        {
            /* PORT_SPEED: 10 (natural 10, the default), natural60, 15, 20, 30, original */
            const char *e = getenv("PORT_SPEED");
            int t = !e ? 6 : !strcmp(e, "natural60") ? -60 : !strcmp(e, "original") ? 0 : !strcmp(e, "max") ? -1 :
                    atoi(e) > 0 ? 60 / atoi(e) : 6;
            static const int ticks[] = { -60, 6, 4, 3, 2, 0 };
            int k;
            for (k = 0; k < 6; k++)
                if (ticks[k] == t)
                    speed_idx = k;
            interp_speed_set(t);
            if (getenv("PORT_GSU_SPEED"))
                gsu_set_speed(atof(getenv("PORT_GSU_SPEED")));
            if (getenv("PORT_LOG_DIR"))
                interp_log_open(getenv("PORT_LOG_DIR"));
        }
        host_thread_interactive();
        host_window_open(SNES_WIDTH, SNES_HEIGHT, "Stunt Race FX");
        if (!rr_open()) {
            host_message_box("The renderer could not start on this computer.", "Stunt Race FX");
            return 1;
        }
        canvases_start();
        s_rs = resample_new(32040, OUT_RATE);
        host_audio_open(OUT_RATE, audio_cb, NULL);
    }
    port_start();
    return 0;
}
