/*
 * sfxbox: run the original ROM headless and record what it executes.
 *
 *   sfxbox --rom ROM --script FILE --out DIR [--core DYLIB] [--trace]
 *
 * Script, one command per line ('#' starts a comment):
 *   wait N               N frames, no buttons
 *   hold B[+B...] N      hold buttons for N frames
 *   press B[+B...] [N]   hold for N frames (default 2), then 2 frames released
 *   shot NAME            write the current frame to DIR/NAME.ppm
 *   dump NAME            write WRAM and GSU RAM to DIR/NAME.wram, DIR/NAME.gsuram
 *   save NAME            write a state to DIR/NAME.state
 *   load PATH            read a state
 * Buttons: B Y SELECT START UP DOWN LEFT RIGHT A X L R.
 *
 * With --trace, DIR receives rommap.bin (per ROM byte: who touched it and
 * how), wram_exec.bin, gsuram_exec.bin and edges.csv (control transfers).
 */
#include <dlfcn.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libretro.h"

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
    if (port != 0 || device != RETRO_DEVICE_JOYPAD) return 0;
    if (id == RETRO_DEVICE_ID_JOYPAD_MASK) return (int16_t)buttons;
    return (buttons >> id) & 1;
}

static void run(long n) { while (n-- > 0) { retro_run_p(); frames++; } }

static unsigned parse_buttons(const char *s) {
    static const char *names[] = {"B", "Y", "SELECT", "START", "UP", "DOWN", "LEFT", "RIGHT", "A", "X", "L", "R"};
    unsigned m = 0;
    char buf[256]; snprintf(buf, sizeof buf, "%s", s);
    for (char *t = strtok(buf, "+"); t; t = strtok(NULL, "+")) {
        unsigned i;
        for (i = 0; i < 12 && strcmp(t, names[i]); i++) {}
        if (i == 12) die("unknown button: %s", t);
        m |= 1u << i;
    }
    return m;
}

static void write_file(const char *name, const char *ext, const void *p, size_t n) {
    char path[4096]; snprintf(path, sizeof path, "%s/%s%s", outdir, name, ext);
    FILE *f = fopen(path, "wb"); if (!f) die("cannot write %s", path);
    fwrite(p, 1, n, f); fclose(f);
}

static void shot(const char *name) {
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

static void script(const char *path) {
    FILE *f = fopen(path, "r"); if (!f) die("cannot read script %s", path);
    char line[1024];
    int ln = 0;
    while (fgets(line, sizeof line, f)) {
        ln++;
        char *h = strchr(line, '#'); if (h) *h = 0;
        char a[64] = "", b[256] = "", c[64] = "";
        int k = sscanf(line, "%63s %255s %63s", a, b, c);
        if (k <= 0) continue;
        if (!strcmp(a, "wait") && k >= 2) { buttons = 0; run(atol(b)); }
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
        else if (!strcmp(a, "load") && k >= 2) {
            size_t n; void *p = load_file(b, &n);
            if (!retro_unserialize_p(p, n)) die("unserialize failed: %s", b);
            free(p);
        }
        else die("%s:%d: bad command", path, ln);
    }
    fclose(f);
}

int main(int argc, char **argv) {
    const char *rom = NULL, *scr = NULL, *corepath = "tools/toolchains/bsnes/bsnes_libretro.dylib";
    int trace = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--rom") && i + 1 < argc) rom = argv[++i];
        else if (!strcmp(argv[i], "--script") && i + 1 < argc) scr = argv[++i];
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) outdir = argv[++i];
        else if (!strcmp(argv[i], "--core") && i + 1 < argc) corepath = argv[++i];
        else if (!strcmp(argv[i], "--trace")) trace = 1;
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
    script(scr);
    if (trace && sfxbox_trace_write_p(outdir)) die("trace write failed");
    printf("frames %ld\n", frames);
    return 0;
}
