/*
 * resample.h: stereo sample-rate conversion (resample.c): the mixer's
 * 32,000 frames a second to the host's rate, by a windowed sinc (32 taps,
 * Kaiser window, cut off below the lower rate's Nyquist), its ratio
 * adjustable a little at a time so the output can follow the game's pace.
 */
#ifndef PORT_RESAMPLE_H
#define PORT_RESAMPLE_H

#include <stdint.h>

typedef struct Resampler Resampler;

/* in_rate frames a second in, out_rate out */
Resampler *resample_new(double in_rate, double out_rate);
void resample_free(Resampler *r);
/* the input frames per output frame, scaled by (1 + adjust); adjust small
   (a fraction of a percent), to keep a queue level */
void resample_adjust(Resampler *r, double adjust);
/* frames more input taken; returns how many of the n given it kept (it
   keeps what it has room for) */
int resample_push(Resampler *r, const int16_t *lr, int n);
/* up to n output frames made into lr (floats, -1..1); returns how many
   (fewer when the input runs out) */
int resample_pull(Resampler *r, float *lr, int n);
/* input frames waiting */
int resample_queued(const Resampler *r);

#endif
