/*
 * mix_replay.c: the native mixer (../mixer.c) given what bsnes's DSP was given, held to
 * bsnes's own output, sample for sample.
 *
 *   mix_replay LOG SOUND
 *
 * LOG is sfxbox --apulog's record (the APU RAM at power-on, then each
 * SPC700 write with the DSP samples made before it and the DSP's cycle in
 * the sample it was making); SOUND is sfxbox --sound's output of the same
 * run. Each write lands in the replay's APU RAM and, for $F3, in the DSP
 * register $F2 names, from the frame after the one it came in (PHASE_AT=N:
 * a write before cycle N lands in its own frame). The
 * chip's output comes a sample after the mixer's (OUT_DELAY=0 compares
 * them unshifted).
 * Prints the first sample that differs and the count; exit 1 on any.
 * VOX=FILE (sfxbox --voices of the same run): also the first frame each
 * voice's envelope level or stage differs (the chip wraps its play
 * position and decodes a frame later than the mixer, from the same values,
 * so those are not compared).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mixer.h"
#include "mixer_regs.h"

int main(int argc, char **argv)
{
    FILE *lf, *sf;
    static uint8_t ram[65536];
    uint8_t rec[8], f2 = 0;
    int have = 0, cut = getenv("PHASE_AT") ? atoi(getenv("PHASE_AT")) : 0;
    uint32_t s = 0, ws = 0, first = UINT32_MAX;
    unsigned long bad = 0, n = 0;
    int16_t want[2], prev[2] = { 0, 0 };
    int vcut[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
    FILE *vf = getenv("VOX") ? fopen(getenv("VOX"), "rb") : NULL;
    int32_t chip[2][40], mine[2][40];
    int vk = 0, vreported[2] = { 0, 0 };
    int delay = getenv("OUT_DELAY") ? atoi(getenv("OUT_DELAY")) : 1;
    if (argc < 3 || !(lf = fopen(argv[1], "rb")) || !(sf = fopen(argv[2], "rb"))) {
        fprintf(stderr, "usage: mix_replay LOG SOUND\n");
        return 2;
    }
    if (getenv("VCUT"))
        sscanf(getenv("VCUT"), "%d,%d,%d,%d,%d,%d,%d,%d", &vcut[0], &vcut[1], &vcut[2], &vcut[3], &vcut[4], &vcut[5],
               &vcut[6], &vcut[7]);
    if (fread(ram, 1, sizeof ram, lf) != sizeof ram)
        return 2;
    mix_init(ram);
    mix_regs_reset();
    while (fread(want, 2, 2, sf) == 2) {
        int16_t l, r;
        /* the writes that land before this sample */
        for (;;) {
            uint16_t a;
            if (!have) {
                if (fread(rec, 1, 8, lf) != 8)
                    break;
                have = 1;
            }
            ws = rec[0] | rec[1] << 8 | rec[2] << 16 | (uint32_t)rec[3] << 24;
            {
                /* the cycle from which this write waits for the next frame:
                   VCUT=c0,..,c7 for each voice's registers, else PHASE_AT */
                int c = cut;
                uint16_t wa = (uint16_t)(rec[5] | rec[6] << 8);
                if (wa == 0xF3 && !(f2 & 0x80) && (f2 & 0x0F) < 0x0A && vcut[f2 >> 4] >= 0)
                    c = vcut[f2 >> 4];
                if (ws > s || (ws == s && rec[4] >= c))
                    break;
            }
            a = (uint16_t)(rec[5] | rec[6] << 8);
            ram[a] = rec[7];
            if (a == 0xF2)
                f2 = rec[7];
            else if (a == 0xF3 && !(f2 & 0x80))
                mix_reg_write(f2, rec[7]);
            have = 0;
        }
        mix_frame(&l, &r);
        if (vf && fread(chip[vk], 4, 40, vf) == 40) {
            int v, k;
            mix_voice_states(mine[vk]);
            for (v = 0; v < 8; v++)
                for (k = 0; k < 2; k++) {
                    const int32_t *c = chip[vk], *m = mine[vk];
                    if (vreported[k] || s < 2 || (!c[5 * v] && !m[5 * v]))
                        continue;
                    if (c[5 * v + k] != m[5 * v + k]) {
                        fprintf(stderr, "voice state first differs at frame %u: voice %d %s, chip %d, mixer %d\n", s, v,
                                (const char *[]){ "level", "stage", "position", "block", "byte" }[k], c[5 * v + k], m[5 * v + k]);
                        vreported[k] = 1;
                    }
                }
            vk ^= 1;
        }
        if (delay) {                         /* the output a sample later */
            int16_t t0 = prev[0], t1 = prev[1];
            prev[0] = l;
            prev[1] = r;
            l = t0;
            r = t1;
        }
        if (l != want[0] || r != want[1]) {
            if (first == UINT32_MAX) {
                first = s;
                fprintf(stderr, "first difference at sample %u: bsnes %d %d, mixer %d %d\n", s, want[0], want[1], l, r);
            }
            bad++;
        }
        n++;
        s++;
    }
    printf("%lu of %lu samples differ%s\n", bad, n, first == UINT32_MAX ? "" : "");
    return bad != 0;
}
