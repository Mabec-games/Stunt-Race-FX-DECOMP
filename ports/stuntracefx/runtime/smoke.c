/* Optional headless diagnostics. Direct memory reads never add SNES cycles.
 * PORT_DRIVE_SCRIPT: ascending pairs of race-relative video frame and hex
 * joypad mask. Last input is held; the race clock's first rising edge is t=0.
 * PORT_SMOKE_TRACE: JSONL, version 1. Does not change game state or saves.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "snes.h"
#include "video.h"
#include "smoke.h"

extern int world_frame;
static FILE *trace;
static struct { long frame; unsigned buttons; } inputs[256];
static int input_count, input_at;
static long race_start = -1, tick;
static unsigned buttons;
static long stop_after = -1;

static unsigned word(const uint8_t *p, unsigned a)
{
    return p[a & 65535] | (unsigned)p[(a + 1) & 65535] << 8;
}

static void close_trace(void)
{
    if (trace && (ferror(trace) || fclose(trace))) {
        fprintf(stderr, "smoke: trace write failed\n");
        _Exit(2);
    }
}

void smoke_init(int headless)
{
    const char *path = getenv("PORT_DRIVE_SCRIPT"), *out = getenv("PORT_SMOKE_TRACE");
    if (!headless || (!path && !out))
        return;
    if (getenv("PORT_SMOKE_FRAMES")) {
        char *end;
        stop_after = strtol(getenv("PORT_SMOKE_FRAMES"), &end, 10);
        if (*end || stop_after < 1)
            snes_fatal("smoke: PORT_SMOKE_FRAMES must be positive");
    }
    if (path) {
        FILE *f = fopen(path, "r");
        char line[256];
        if (!f)
            snes_fatal("smoke: cannot read drive script %s", path);
        while (fgets(line, sizeof line, f)) {
            long at;
            unsigned mask;
            char extra, *comment = strchr(line, '#');
            if (comment) *comment = 0;
            if (sscanf(line, " %c", &extra) != 1) continue;
            if (sscanf(line, "%ld %x %c", &at, &mask, &extra) != 2 || at < 0 || mask > 65535 ||
                input_count == 256 || (input_count && at <= inputs[input_count-1].frame))
                snes_fatal("smoke: invalid drive script row: %s", line);
            inputs[input_count].frame = at;
            inputs[input_count++].buttons = mask;
        }
        fclose(f);
        if (!input_count || inputs[0].frame != 0)
            snes_fatal("smoke: drive script must start at frame 0");
    }
    if (out) {
        trace = fopen(out, "w");
        if (!trace) snes_fatal("smoke: cannot write %s", out);
        fprintf(trace, "{\"event\":\"schema\",\"version\":1,\"refresh_hz\":60.0988}\n");
        atexit(close_trace);
    }
}

void smoke_input(long frame, uint16_t *p0, uint16_t *p1)
{
    if (!trace && !input_count) return;
    if (race_start < 0 && snes_wram()[0x0E5B]) {
        race_start = frame;
        if (trace) fprintf(trace, "{\"event\":\"race_start\",\"frame\":%ld}\n", frame);
    }
    if (race_start >= 0 && input_count) {
        while (input_at + 1 < input_count && inputs[input_at + 1].frame <= frame - race_start)
            input_at++;
        *p0 = (uint16_t)inputs[input_at].buttons;
        *p1 = 0;
    }
    buttons = *p0;
    if (race_start >= 0 && stop_after > 0 && frame - race_start >= stop_after) {
        if (trace) fprintf(trace, "{\"event\":\"complete\",\"frame\":%ld,\"elapsed\":%ld}\n", frame, frame-race_start);
        exit(0);
    }
}

static void vector(const uint8_t *p, unsigned a)
{
    fprintf(trace, "[%d,%d,%d]", (int16_t)word(p, a), (int16_t)word(p, a+2), (int16_t)word(p, a+4));
}

void smoke_gsu(uint32_t program, int done, const uint16_t *objects, int count)
{
    const uint8_t *w, *s;
    unsigned o, r, cam, i;
    const char *event;
    if (!trace) return;
    if (program == 0x048FCA) {
        if (!done) tick++;
        event = done ? "physics_end" : "physics_begin";
    } else if (program == 0x01AEF8 && !done) event = "draw";
    else return;
    w = snes_wram(); s = snes_sfx_ram();
    o = word(w, 0x0346);
    r = word(w, 0x2022 + o);
    if (!o || !r || word(s, r + 0x0A) != 0xB1A1) return;
    cam = word(w, 0x204F + word(w, 0x204F + o));
    fprintf(trace, "{\"event\":\"%s\",\"frame\":%u,\"t\":%ld,\"tick\":%ld,\"world\":%d,"
            "\"phase\":%u,\"clock\":%u,\"buttons\":%u,\"object\":%u,\"record\":%u,\"boost\":%u,"
            "\"controls\":%u,\"speed\":%u,\"damage\":%u,\"car\":",
            event, snes_frame(), race_start < 0 ? -1 : (long)snes_frame()-race_start, tick, world_frame,
            w[0x0DD6], w[0x0E5B], buttons, o, r, word(s,r+0x6A), word(s,r+0x42), word(s,r+0x4A), word(s,r+0x82));
    vector(s,r+0x1E);
    fprintf(trace, ",\"pose\":"); vector(w,0x200C+o);
    fprintf(trace, ",\"angles\":"); vector(s,r+0x24);
    fprintf(trace, ",\"view\":"); vector(w,0xC5);
    fprintf(trace, ",\"view_angles\":"); vector(w,0x664);
    fprintf(trace, ",\"camera\":"); vector(w,cam+8);
    fprintf(trace, ",\"camera_angles\":"); vector(w,cam+14);
    if (program == 0x01AEF8) {
        unsigned n = word(s,0x1C8);
        int mapped = n <= 64 && n == (unsigned)count && !memcmp(s+0xAAA,w+0xF77F,n*30);
        fprintf(trace, ",\"mapped\":%s,\"gsu_view\":", mapped ? "true" : "false"); vector(s,0xD8);
        fprintf(trace, ",\"matrix\":[");
        for (i=0; i<9; i++) fprintf(trace, "%s%d", i ? "," : "", (int16_t)word(s,0xE4+2*i));
        fprintf(trace, "],\"draws\":[");
        for (i=0; mapped && i<n; i++) {
            unsigned b = 0xAAA+30*i, obj = objects[i], fx = word(w,0x2022+obj);
            fprintf(trace, "%s{\"object\":%u,\"model\":%u,\"flags\":%u,\"handler\":%u,\"record\":%u,\"relative\":[%d,%d,%d],\"pose\":",
                    i ? "," : "", obj, word(s,b+8), s[b+7], word(w,0x201A+obj) | (unsigned)w[(0x201C+obj)&65535]<<16,
                    fx, (int16_t)word(s,b+18), (int16_t)word(s,b+16), (int16_t)word(s,b+20));
            vector(w,0x200C+obj);
            fprintf(trace, ",\"angles\":[%u,%u,%u]}",s[b+4],s[b+5],s[b+6]);
        }
        fprintf(trace,"]");
    }
    fprintf(trace,"}\n");
}

void smoke_present(long frame, const uint32_t *pixels)
{
    uint32_t hash = 2166136261u;
    int x, y;
    if (!trace) return;
    if (!pixels) pixels = vid_picture();
    /* Interior of gameplay view, excluding top-row hold and bottom HUD. */
    for (y=40;y<144;y++) for (x=24;x<232;x++) {
        uint32_t rgb = pixels[y*VID_WIDTH+x] & 0xFFFFFF;
        hash = (hash ^ rgb) * 16777619u;
    }
    fprintf(trace,"{\"event\":\"present\",\"frame\":%ld,\"t\":%ld,\"hash\":%u",
            frame, race_start < 0 ? -1 : frame-race_start, hash);
    if (race_start < 0) {
        const uint8_t *w = snes_wram();
        fprintf(trace,",\"lamps\":[[%u,%u],[%u,%u],[%u,%u]]",
                w[0x0406], w[0x0407], w[0x040A], w[0x040B], w[0x040E], w[0x040F]);
    }
    fprintf(trace,"}\n");
}
