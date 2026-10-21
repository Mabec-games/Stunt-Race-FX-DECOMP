/*
 * mixer.h: the sound, native: eight voices, each playing a sample from
 * the game's sound memory at a pitch, through an envelope and two volumes,
 * with a noise source, pitch modulation and an echo, mixed to stereo at
 * 32,000 frames a second. Platform agnostic C.
 *
 * The samples are the game's own, in the compressed form it keeps them
 * (BRR: nine-byte blocks of sixteen 4-bit values, a shift, a prediction
 * filter, loop and end flags), found through a directory of start and loop
 * addresses; the echo's delay line lives in the same memory. The arithmetic
 * (decoding, Gaussian interpolation, envelopes, the echo's filter) is the
 * sound chip's as Nocash's fullsnes documents it, so the game sounds as it
 * did. A change takes effect from the next frame mixed.
 */
#ifndef PORT_MIXER_H
#define PORT_MIXER_H

#include <stdint.h>

/* the game's sound memory: 64 KB, the samples' directory and data and the
   echo's delay line */
void mix_init(uint8_t *memory);

/* ---- a voice ---- */
void mix_voice_volume(int v, int8_t left, int8_t right);
/* 14 bits: 0x1000 plays the sample at 32,000 Hz */
void mix_voice_pitch(int v, uint16_t pitch);
/* the directory entry its next key-on (and its loops) play */
void mix_voice_sample(int v, uint8_t entry);
/* the envelope: ADSR (attack 0-15, decay 0-7, sustain level 0-7 and rate
   0-31) when adsr is set, else the gain setting (a fixed level, or a
   linear or exponential fade or rise at a rate) */
void mix_voice_envelope(int v, int adsr, int attack, int decay, int sustain_level, int sustain_rate);
void mix_voice_gain(int v, uint8_t gain);
/* voices (a bit each) started from their sample, released */
void mix_key_on(uint8_t voices);
void mix_key_off(uint8_t voices);
/* voices whose pitch the voice before modulates; that play noise; that
   feed the echo */
void mix_pitch_modulation(uint8_t voices);
void mix_noise(uint8_t voices);
void mix_echo_voices(uint8_t voices);

/* ---- the mix ---- */
void mix_master_volume(int8_t left, int8_t right);
/* every voice released at once with its envelope at 0 (held while set);
   the output silenced; the echo's writes to memory off; the noise's rate
   (0-31, as an envelope rate) */
void mix_control(int reset, int mute, int echo_writes_off, int noise_rate);
/* the directory's page (its address / 256) */
void mix_directory(uint8_t page);
/* the echo: its delay line (page, and length in 2 KB steps, 0-15), the
   volume it is heard at, its feedback, its 8-tap filter */
void mix_echo_buffer(uint8_t page, uint8_t length);
void mix_echo_volume(int8_t left, int8_t right);
void mix_echo_feedback(int8_t feedback);
void mix_echo_filter(int tap, int8_t coefficient);

/* ---- what the game reads back ---- */
/* the voices that have reached their sample's end since last cleared */
uint8_t mix_voices_ended(void);
void mix_clear_ended(void);
/* a voice's envelope (its top 7 of 11 bits) and output (its top 8 bits) */
uint8_t mix_voice_level(int v);
int8_t mix_voice_output(int v);

/* one frame: left and right */
void mix_frame(int16_t *left, int16_t *right);

/* the replay check's view: each voice's envelope level and stage (0
   release, 1 attack, 2 decay, 3 sustain), play position, block address and
   byte within the block */
void mix_voice_states(int32_t *out);

#endif
