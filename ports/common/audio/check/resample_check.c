/*
 * resample_check.c: the resampler against the exact signal. Tones at
 * 32,000 Hz go in; what comes out at 48,000 Hz is compared with the same
 * tone computed at the output's times (output frame i is the input at
 * 15 + i * 2/3: the filter's centre). The error, the images above 16 kHz
 * included, must be 70 dB or more below a tone up to 12 kHz.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "resample.h"

static double run(double hz, double *level)
{
    Resampler *r = resample_new(32000, 48000);
    static int16_t in[2 * 32000];
    static float out[2 * 48000];
    double err = 0, sig = 0, peak = 0, amp = 30000.0 / 32768.0;
    int i, n = 0, got;
    for (i = 0; i < 32000; i++)
        in[2 * i] = in[2 * i + 1] = (int16_t)lrint(30000 * sin(2 * M_PI * hz * i / 32000.0));
    for (i = 0; i < 32000; i += 500) {
        resample_push(r, in + 2 * i, 500);
        while ((got = resample_pull(r, out + 2 * n, 48000 - n)) > 0)
            n += got;
    }
    for (i = 4800; i < n - 100; i++) {        /* past the start */
        double t = i * (32000.0 / 48000.0) + 15;   /* input frames */
        double want = amp * sin(2 * M_PI * hz * t / 32000.0);
        err += (out[2 * i] - want) * (out[2 * i] - want);
        sig += want * want;
        if (fabs(out[2 * i]) > peak) peak = fabs(out[2 * i]);
    }
    resample_free(r);
    *level = 20 * log10(peak / amp);
    return 10 * log10(sig / err);
}

int main(void)
{
    static const double band[] = { 100, 1000, 5000, 10000, 12000, 14000 };
    int i, bad = 0;
    double lv;
    for (i = 0; i < 6; i++) {
        double snr = run(band[i], &lv);
        printf("%6.0f Hz: error %.1f dB below the tone, level %+.2f dB\n", band[i], snr, lv);
        bad += band[i] <= 12000 && snr < 70;
    }
    return bad != 0;
}
