/*
 * mix_replay.c: the port's mixer given what bsnes's DSP was given, held to
 * bsnes's own output, sample for sample.
 *
 *   mix_replay LOG SOUND
 *
 * LOG is sfxbox --apulog's record (the APU RAM at power-on, then each
 * SPC700 write with the DSP samples made before it and the DSP's cycle in
 * the sample it was making); SOUND is sfxbox --sound's output of the same
 * run. Each write lands in the replay's APU RAM and, for $F3, in the DSP
 * register $F2 names, before the sample it came in is made (PHASE_AT, the
 * cycle from which a write waits for the next sample, sets otherwise). The
 * chip's output comes a sample after the mixer's (OUT_DELAY=0 compares
 * them unshifted).
 * Prints the first sample that differs and the count; exit 1 on any.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "snes_internal.h"

int main(int argc, char **argv)
{
    FILE *lf, *sf;
    static uint8_t ram[65536];
    uint8_t rec[8], f2 = 0;
    int have = 0, cut = getenv("PHASE_AT") ? atoi(getenv("PHASE_AT")) : 32;
    uint32_t s = 0, ws = 0, first = UINT32_MAX;
    unsigned long bad = 0, n = 0;
    int16_t want[2], prev[2] = { 0, 0 };
    int delay = getenv("OUT_DELAY") ? atoi(getenv("OUT_DELAY")) : 1;
    if (argc < 3 || !(lf = fopen(argv[1], "rb")) || !(sf = fopen(argv[2], "rb"))) {
        fprintf(stderr, "usage: mix_replay LOG SOUND\n");
        return 2;
    }
    if (fread(ram, 1, sizeof ram, lf) != sizeof ram)
        return 2;
    dsp_reset(ram);
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
            if (ws > s || (ws == s && rec[4] >= cut))
                break;
            a = (uint16_t)(rec[5] | rec[6] << 8);
            ram[a] = rec[7];
            if (a == 0xF2)
                f2 = rec[7];
            else if (a == 0xF3 && !(f2 & 0x80))
                dsp_write(f2, rec[7]);
            have = 0;
        }
        dsp_sample(&l, &r);
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
