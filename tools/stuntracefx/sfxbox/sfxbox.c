/*
 * sfxbox: run the original ROM headless and record what it executes.
 *
 *   sfxbox --rom ROM --script FILE --out DIR [--core DYLIB] [--trace]
 *          [--check LIB | --lockstep LIB] [--only ADDR,ADDR...]
 *
 * Script, one command per line ('#' starts a comment):
 *   wait N               N frames, no buttons
 *   hold B[+B...] N      hold buttons for N frames
 *   press B[+B...] [N]   hold for N frames (default 2), then 2 frames released
 *   shot NAME            write the current frame to DIR/NAME.ppm
 *   dump NAME            write WRAM and GSU RAM to DIR/NAME.wram, DIR/NAME.gsuram
 *   save NAME            write a state to DIR/NAME.state
 *   load PATH            read a state
 *   poke ADDR BYTE[,BYTE...]
 *                        write bytes from S-CPU address ADDR (hex, 24-bit) into
 *                        work RAM ($7E-$7F, or the low 8K mirror in banks
 *                        $00-$3F) or Super FX RAM ($70-$71), between frames;
 *                        for scripts that start from save data the game itself
 *                        could hold (see each script's header)
 *   call ADDR [long] [a=HEX] [x=] [y=] [p=] [db=] [d=] [frames=N]
 *                        run the routine at ADDR once as a unit test: entered
 *                        by a JSR (by a JSL with long) from a stub in work RAM
 *                        with those registers, the game's state as the script
 *                        left it; then the game goes on where it was
 *   repeat N ... end     run the enclosed lines N times (nests)
 * Buttons: B Y SELECT START UP DOWN LEFT RIGHT A X L R, player 1; prefix
 * with 2. for player 2 (2.START). Joined with +.
 *
 * With --trace, DIR receives rommap.bin (per ROM byte: who touched it and
 * how), wram_exec.bin, gsuram_exec.bin and edges.csv (control transfers).
 *
 * With --check, every call of a routine that LIB (the hybrid build of the C
 * decomp) implements is checked against its C; see check.c. The report goes
 * to DIR/check.txt. With --lockstep, the C replaces those routines in a
 * second copy of the game run in step with the original; see check.c. The
 * report goes to DIR/lockstep.txt.
 */
#include <dlfcn.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libretro.h"
#include "check.h"

static void die(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap);
    fputc('\n', stderr); exit(1);
}

static void *core;
void sfxbox_trace_begin(void);
int sfxbox_trace_write(const char *dir);
uint8_t *sfxbox_memory(const char *name, size_t *size);
#define SYM(name) static __typeof__(&name) name##_p;
SYM(retro_init) SYM(retro_set_environment) SYM(retro_set_video_refresh) SYM(retro_set_audio_sample)
SYM(retro_set_audio_sample_batch) SYM(retro_set_input_poll) SYM(retro_set_input_state) SYM(retro_load_game)
SYM(retro_run) SYM(retro_serialize_size) SYM(retro_serialize) SYM(retro_unserialize)
SYM(sfxbox_trace_begin) SYM(sfxbox_trace_write) SYM(sfxbox_memory)

static const char *outdir;
static char sysdir[4096];
static unsigned pixfmt = RETRO_PIXEL_FORMAT_0RGB1555;
static uint32_t frame[512 * 512];
static unsigned fw, fh;
static unsigned buttons;
static long frames;

static bool env(unsigned cmd, void *data) {
    switch (cmd) {
    case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
    case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        *(const char **)data = sysdir; return true;
    case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        pixfmt = *(const unsigned *)data; return true;
    case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        *(bool *)data = true; return true;
    case RETRO_ENVIRONMENT_GET_VARIABLE: {
        /* The core's speed shortcuts are all off: scanline PPU, fast DSP and
           delayed coprocessor sync trade accuracy for speed. No overclock. */
        struct retro_variable *v = data;
        static const char *fixed[][2] = {
            {"bsnes_ppu_fast", "OFF"}, {"bsnes_dsp_fast", "OFF"}, {"bsnes_coprocessor_delayed_sync", "OFF"},
            {"bsnes_cpu_overclock", "100"}, {"bsnes_cpu_sfx_overclock", "100"}, {"bsnes_cpu_sa1_overclock", "100"},
        };
        for (unsigned i = 0; i < sizeof fixed / sizeof fixed[0]; i++)
            if (!strcmp(v->key, fixed[i][0])) { v->value = fixed[i][1]; return true; }
        return false;
    }
    default:
        return false;
    }
}

static void video(const void *data, unsigned w, unsigned h, size_t pitch) {
    if (!data) return;
    if (w > 512) w = 512;
    if (h > 512) h = 512;
    fw = w; fh = h;
    for (unsigned y = 0; y < h; y++) {
        const uint8_t *row = (const uint8_t *)data + y * pitch;
        for (unsigned x = 0; x < w; x++) {
            uint32_t c;
            if (pixfmt == RETRO_PIXEL_FORMAT_XRGB8888) c = ((const uint32_t *)row)[x];
            else {
                uint16_t p = ((const uint16_t *)row)[x];
                if (pixfmt == RETRO_PIXEL_FORMAT_RGB565)
                    c = ((p >> 11 & 31) << 19) | ((p >> 5 & 63) << 10) | ((p & 31) << 3);
                else
                    c = ((p >> 10 & 31) << 19) | ((p >> 5 & 31) << 11) | ((p & 31) << 3);
            }
            frame[y * 512 + x] = c;
        }
    }
}
static void audio(int16_t l, int16_t r) { (void)l; (void)r; }
static size_t audio_batch(const int16_t *d, size_t n) { (void)d; return n; }
static void poll(void) {}
static int16_t input(unsigned port, unsigned device, unsigned index, unsigned id) {
    (void)index;
    if (port > 1 || device != RETRO_DEVICE_JOYPAD) return 0;
    unsigned b = buttons >> (port * 16) & 0xffff;
    if (id == RETRO_DEVICE_ID_JOYPAD_MASK) return (int16_t)b;
    return (b >> id) & 1;
}

static int lockstep;
static void run(long n)
{
    while (n-- > 0) {
        if (lockstep) lockstep_before_frame(frames);
        retro_run_p();
        if (lockstep) lockstep_after_frame(frames);
        frames++;
    }
}

static unsigned parse_buttons(const char *s) {
    static const char *names[] = {"B", "Y", "SELECT", "START", "UP", "DOWN", "LEFT", "RIGHT", "A", "X", "L", "R"};
    unsigned m = 0;
    char buf[256]; snprintf(buf, sizeof buf, "%s", s);
    char *save = NULL;
    for (char *t = strtok_r(buf, "+", &save); t; t = strtok_r(NULL, "+", &save)) {
        unsigned i, shift = 0;
        if (!strncmp(t, "2.", 2)) { t += 2; shift = 16; }
        for (i = 0; i < 12 && strcmp(t, names[i]); i++) {}
        if (i == 12) die("unknown button: %s", t);
        m |= 1u << (i + shift);
    }
    return m;
}

static void write_file(const char *name, const char *ext, const void *p, size_t n) {
    if (lockstep && lockstep_role() == 2) return;  /* the original writes the files */
    char path[4096]; snprintf(path, sizeof path, "%s/%s%s", outdir, name, ext);
    FILE *f = fopen(path, "wb"); if (!f) die("cannot write %s", path);
    fwrite(p, 1, n, f); fclose(f);
}

static void shot(const char *name) {
    if (lockstep && lockstep_role() == 2) return;
    char path[4096]; snprintf(path, sizeof path, "%s/%s.ppm", outdir, name);
    FILE *f = fopen(path, "wb"); if (!f) die("cannot write %s", path);
    fprintf(f, "P6\n%u %u\n255\n", fw, fh);
    for (unsigned y = 0; y < fh; y++)
        for (unsigned x = 0; x < fw; x++) {
            uint32_t c = frame[y * 512 + x];
            fputc(c >> 16 & 255, f); fputc(c >> 8 & 255, f); fputc(c & 255, f);
        }
    fclose(f);
}

static void *load_file(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb"); if (!f) die("cannot read %s", path);
    fseek(f, 0, SEEK_END); *n = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    void *p = malloc(*n); if (fread(p, 1, *n, f) != *n) die("short read %s", path);
    fclose(f); return p;
}

static char lines[20000][256];
static int nlines;
static const char *script_path;

/* poke ADDR BYTE[,BYTE...]: both lockstep processes run the script, so a
   poke lands the same way on each side. */
static void poke(const char *line, int lineno) {
    char addr[64] = "", bytes[256] = "";
    if (sscanf(line, "%*s %63s %255s", addr, bytes) != 2) die("%s:%d: poke ADDR BYTE[,BYTE...]", script_path, lineno);
    unsigned long a = strtoul(addr[0] == '$' ? addr + 1 : addr, NULL, 16);
    unsigned bank = a >> 16 & 0xFF, off = a & 0xFFFF;
    const char *view = NULL; size_t base = 0;
    if (bank == 0x7E || bank == 0x7F) { view = "wram"; base = (size_t)(bank - 0x7E) << 16 | off; }
    else if (bank <= 0x3F && off < 0x2000) { view = "wram"; base = off; }
    else if (bank == 0x70 || bank == 0x71) { view = "gsuram"; base = (size_t)(bank - 0x70) << 16 | off; }
    else die("%s:%d: poke: $%06lX is neither work RAM nor Super FX RAM", script_path, lineno, a);
    size_t n; uint8_t *p = sfxbox_memory_p(view, &n);
    if (!p) die("%s:%d: poke: no %s view", script_path, lineno, view);
    for (char *s = bytes, *e; *s; s = *e ? e + 1 : e) {
        unsigned long v = strtoul(s, &e, 16);
        if (e == s || v > 0xFF || (*e && *e != ',')) die("%s:%d: poke: bad byte list", script_path, lineno);
        if (base >= n) die("%s:%d: poke: past the end of %s", script_path, lineno, view);
        p[base++] = (uint8_t)v;
    }
}

/* call: run one routine as a unit test, from the state the script has
   reached. At the next instruction boundary the CPU's registers are saved
   and it runs a stub in low work RAM ($1FE0, its bytes saved and put back):
   JSR or JSL to the routine, then BRA to itself. The routine is entered by
   a real JSR/JSL, so --check and --lockstep take it as a call like any
   other. When the stub's loop is reached the registers are restored and
   the game goes on from where it was. Interrupts run as usual meanwhile.
   It starts only while the Super FX is stopped (GO, bit 5 of $3030, clear)
   and outside the vertical blank ($4212 bit 7). */
struct SfxboxRegs { uint16_t a, x, y, s, d; uint8_t db, p, e; uint32_t pc; };
static void (*sfxbox_core_regs_p)(struct SfxboxRegs *);
static void (*sfxbox_core_set_regs_p)(const struct SfxboxRegs *);
static uint8_t (*sfxbox_core_bus_read_p)(uint32_t);
static int (**cpu_exec_hook)(uint32_t);
static int (*exec_before)(uint32_t);
static struct SfxboxRegs call_from, call_regs;
static uint32_t call_stub, call_loop;
static int call_state;   /* 1 to start at the next boundary, 2 running, 0 done */

static int call_exec(uint32_t pc)
{
    /* Not while the Super FX runs: it may hold the ROM and its RAM, and the
       S-CPU reads neither then (the game waits in work RAM). Nor in the
       vertical blank, where the NMI would land between the JSR and the
       routine's first instruction (a script line runs at a frame's end). */
    if (call_state == 1 && !(sfxbox_core_bus_read_p(0x003030) & 0x20) && !(sfxbox_core_bus_read_p(0x004212) & 0x80)) {
        sfxbox_core_regs_p(&call_from);
        struct SfxboxRegs r = call_regs;
        r.s = call_from.s; r.e = 0; r.pc = call_stub;
        sfxbox_core_set_regs_p(&r);
        call_state = 2;
        return 1;
    }
    if (call_state == 2 && pc == call_loop) {
        sfxbox_core_set_regs_p(&call_from);
        call_state = 0;
        return 1;
    }
    return exec_before ? exec_before(pc) : 0;
}

static void call(const char *line, int lineno) {
    char tok[16][64]; int k = sscanf(line, "%*s %63s %63s %63s %63s %63s %63s %63s %63s %63s", tok[0], tok[1], tok[2], tok[3], tok[4], tok[5], tok[6], tok[7], tok[8]);
    if (k < 1) die("%s:%d: call ADDR [long] [a=] [x=] [y=] [p=] [db=] [d=] [frames=]", script_path, lineno);
    char ab[64]; int j = 0;
    for (const char *c = tok[0]; *c; c++) if (*c != '$' && *c != ':') ab[j++] = *c;
    ab[j] = 0;
    uint32_t addr = (uint32_t)strtoul(ab, NULL, 16), bank = addr >> 16;
    int lng = 0; long frames_max = 600;
    struct SfxboxRegs cur; sfxbox_core_regs_p(&cur);
    call_regs = cur;
    for (int i = 1; i < k; i++) {
        unsigned long v = strtoul(strchr(tok[i], '=') ? strchr(tok[i], '=') + 1 : "0", NULL, 16);
        if (!strcmp(tok[i], "long")) lng = 1;
        else if (!strncmp(tok[i], "a=", 2)) call_regs.a = (uint16_t)v;
        else if (!strncmp(tok[i], "x=", 2)) call_regs.x = (uint16_t)v;
        else if (!strncmp(tok[i], "y=", 2)) call_regs.y = (uint16_t)v;
        else if (!strncmp(tok[i], "p=", 2)) call_regs.p = (uint8_t)v;
        else if (!strncmp(tok[i], "db=", 3)) call_regs.db = (uint8_t)v;
        else if (!strncmp(tok[i], "d=", 2)) call_regs.d = (uint16_t)v;
        else if (!strncmp(tok[i], "frames=", 7)) frames_max = strtol(tok[i] + 7, NULL, 10);
        else die("%s:%d: call: bad option %s", script_path, lineno, tok[i]);
    }
    if (call_regs.p & 0x10) { call_regs.x &= 0x00FF; call_regs.y &= 0x00FF; }  /* 8-bit index */
    size_t n; uint8_t *w = sfxbox_memory_p("wram", &n);
    size_t at = 0; uint32_t sb = 0;
    if (lng) { at = 0x1FE0; sb = 0x00; }
    else if (bank == 0x7F) { at = 0x11FE0; sb = 0x7F; }
    else if (bank == 0x7E || bank < 0x40 || (bank >= 0x80 && bank < 0xC0)) { at = 0x1FE0; sb = bank; }
    else die("%s:%d: call: a JSR can't reach bank %02X from work RAM; use long", script_path, lineno, bank);
    uint8_t keep[6]; memcpy(keep, w + at, 6);
    uint8_t stub[6]; int m = 0;
    if (lng) { stub[m++] = 0x22; stub[m++] = (uint8_t)addr; stub[m++] = (uint8_t)(addr >> 8); stub[m++] = (uint8_t)bank; }
    else { stub[m++] = 0x20; stub[m++] = (uint8_t)addr; stub[m++] = (uint8_t)(addr >> 8); }
    stub[m++] = 0x80; stub[m++] = 0xFE;
    memcpy(w + at, stub, (size_t)m);
    call_stub = sb << 16 | 0x1FE0;
    call_loop = call_stub + (uint32_t)(m - 2);
    exec_before = *cpu_exec_hook;
    *cpu_exec_hook = call_exec;
    call_state = 1;
    long f = 0;
    while (call_state && f < frames_max) { run(1); f++; }
    *cpu_exec_hook = exec_before;
    memcpy(w + at, keep, 6);
    if (call_state) die("%s:%d: call %06X did not return in %ld frames", script_path, lineno, addr, frames_max);
}

static int block_end(int i) {  /* index of the matching end for a repeat at i */
    int depth = 0;
    for (int j = i + 1; j < nlines; j++) {
        char a[64] = "";
        sscanf(lines[j], "%63s", a);
        if (!strcmp(a, "repeat")) depth++;
        else if (!strcmp(a, "end")) { if (depth-- == 0) return j; }
    }
    die("%s:%d: repeat without end", script_path, i + 1);
    return -1;
}

static void exec_range(int from, int to) {
    for (int i = from; i < to; i++) {
        char a[64] = "", b[256] = "", c[64] = "";
        int k = sscanf(lines[i], "%63s %255s %63s", a, b, c);
        if (k <= 0) continue;
        if (!strcmp(a, "repeat") && k >= 2) {
            int e = block_end(i);
            for (long n = atol(b); n > 0; n--) exec_range(i + 1, e);
            i = e;
        }
        else if (!strcmp(a, "end")) die("%s:%d: end without repeat", script_path, i + 1);
        else if (!strcmp(a, "wait") && k >= 2) { buttons = 0; run(atol(b)); }
        else if (!strcmp(a, "hold") && k >= 3) { buttons = parse_buttons(b); run(atol(c)); buttons = 0; }
        else if (!strcmp(a, "press") && k >= 2) { buttons = parse_buttons(b); run(k >= 3 ? atol(c) : 2); buttons = 0; run(2); }
        else if (!strcmp(a, "shot") && k >= 2) shot(b);
        else if (!strcmp(a, "dump") && k >= 2) {
            size_t n; uint8_t *p = sfxbox_memory_p("wram", &n); write_file(b, ".wram", p, n);
            p = sfxbox_memory_p("gsuram", &n); write_file(b, ".gsuram", p, n);
        }
        else if (!strcmp(a, "save") && k >= 2) {
            size_t n = retro_serialize_size_p(); void *p = malloc(n);
            if (!retro_serialize_p(p, n)) die("serialize failed");
            write_file(b, ".state", p, n); free(p);
        }
        else if (!strcmp(a, "poke") && k >= 3) poke(lines[i], i + 1);
        else if (!strcmp(a, "call") && k >= 2) call(lines[i], i + 1);
        else if (!strcmp(a, "load") && k >= 2) {
            size_t n; void *p = load_file(b, &n);
            if (!retro_unserialize_p(p, n)) die("unserialize failed: %s", b);
            free(p);
        }
        else die("%s:%d: bad command", script_path, i + 1);
    }
}

static void script(const char *path) {
    FILE *f = fopen(path, "r"); if (!f) die("cannot read script %s", path);
    script_path = path;
    while (nlines < 20000 && fgets(lines[nlines], sizeof lines[0], f)) {
        char *h = strchr(lines[nlines], '#'); if (h) *h = 0;
        nlines++;
    }
    fclose(f);
    exec_range(0, nlines);
}

int main(int argc, char **argv) {
    const char *rom = NULL, *scr = NULL, *corepath = "tools/toolchains/bsnes/bsnes_libretro.dylib";
    int trace = 0;
    const char *checklib = NULL, *only = NULL, *locklib = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--rom") && i + 1 < argc) rom = argv[++i];
        else if (!strcmp(argv[i], "--script") && i + 1 < argc) scr = argv[++i];
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) outdir = argv[++i];
        else if (!strcmp(argv[i], "--core") && i + 1 < argc) corepath = argv[++i];
        else if (!strcmp(argv[i], "--trace")) trace = 1;
        else if (!strcmp(argv[i], "--check") && i + 1 < argc) checklib = argv[++i];
        else if (!strcmp(argv[i], "--only") && i + 1 < argc) only = argv[++i];
        else if (!strcmp(argv[i], "--lockstep") && i + 1 < argc) locklib = argv[++i];
        else die("usage: sfxbox --rom ROM --script FILE --out DIR [--core DYLIB] [--trace]");
    }
    if (!rom || !scr || !outdir) die("usage: sfxbox --rom ROM --script FILE --out DIR [--core DYLIB] [--trace]");
    snprintf(sysdir, sizeof sysdir, "%s", outdir);

    core = dlopen(corepath, RTLD_NOW | RTLD_LOCAL);
    if (!core) die("dlopen %s: %s", corepath, dlerror());
#define LOAD(name) if (!(*(void **)&name##_p = dlsym(core, #name))) die("missing symbol %s", #name);
    LOAD(retro_init) LOAD(retro_set_environment) LOAD(retro_set_video_refresh) LOAD(retro_set_audio_sample)
    LOAD(retro_set_audio_sample_batch) LOAD(retro_set_input_poll) LOAD(retro_set_input_state) LOAD(retro_load_game)
    LOAD(retro_run) LOAD(retro_serialize_size) LOAD(retro_serialize) LOAD(retro_unserialize)
    LOAD(sfxbox_trace_begin) LOAD(sfxbox_trace_write) LOAD(sfxbox_memory)
    LOAD(sfxbox_core_regs) LOAD(sfxbox_core_set_regs) LOAD(sfxbox_core_bus_read)
    if (!(cpu_exec_hook = dlsym(core, "sfxbox_cpu_exec"))) die("missing symbol sfxbox_cpu_exec");

    retro_set_environment_p(env);
    retro_init_p();
    retro_set_video_refresh_p(video);
    retro_set_audio_sample_p(audio);
    retro_set_audio_sample_batch_p(audio_batch);
    retro_set_input_poll_p(poll);
    retro_set_input_state_p(input);

    size_t n; void *data = load_file(rom, &n);
    struct retro_game_info gi = {rom, data, n, NULL};
    if (!retro_load_game_p(&gi)) die("load failed: %s", rom);
    if (trace) sfxbox_trace_begin_p();
    if (checklib) {
        char rp[4096]; snprintf(rp, sizeof rp, "%s/check.txt", outdir);
        if (check_begin(core, checklib, only, rp)) die("check setup failed");
    }
    if (locklib) {
        char rp[4096]; snprintf(rp, sizeof rp, "%s/lockstep.txt", outdir);
        if (lockstep_begin(core, locklib, only, rp) < 0) die("lockstep setup failed");
        lockstep = 1;
    }
    script(scr);
    if (checklib) check_end();
    if (lockstep) lockstep_end(frames);
    if (trace && sfxbox_trace_write_p(outdir)) die("trace write failed");
    printf("frames %ld\n", frames);
    return 0;
}
