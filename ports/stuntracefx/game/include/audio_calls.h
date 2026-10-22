/*
 * audio_calls.h: the S-CPU's sound routines (game/audio) as C functions,
 * for the C that calls them.
 */
#ifndef STUNTRACEFX_AUDIO_CALLS_H
#define STUNTRACEFX_AUDIO_CALLS_H

#include <stdint.h>

/* Where an upload left the stream, and the last value it put on port 0. */
typedef struct {
    uint16_t offset;
    uint8_t kick;
} ApuUploadEnd;

/* The next byte of the sound stream; *offset (Y) steps on (apu_upload.c). */
uint8_t ApuUploadNextByte(uint16_t *offset);
/* The block list at bank:addr sent to the SPC700 (apu_upload.c). */
ApuUploadEnd ApuUpload(uint8_t bank, uint16_t addr);
void ApuPort1Send05(void);
void ApuSettleDelay(void);
/* The list at $19:C27C uploaded; the stream offset after it. */
uint16_t ApuUploadBank19(void);

/* The sound command queues (sound_queue.c): a command appended to the
   port 0 or port 1 ring. `wide` is the caller's push and pull time over
   M and X 8-bit: 8 with A 16-bit, 16 with X and Y 16-bit; the forms
   without it are for M 8-bit, X 16-bit. */
void SoundPort0QueuePushRaw(uint8_t command);
void SoundPort0QueuePushRawX8(uint8_t command);
void SoundPort0QueuePushRawWide(uint8_t command, unsigned wide);
void SoundPort0QueuePush(uint8_t command);
void SoundPort0QueuePushW(uint8_t command, unsigned wide);
void SoundPort1QueuePush(uint8_t command);
void SoundPort1QueuePushW(uint8_t command, unsigned wide);
void SoundReset(void);

/* The frame's sound output (SoundPortsSend): the low bytes of A and X. */
typedef struct {
    uint8_t a, x;
} SoundPortsEnd;

SoundPortsEnd SoundPortsSend(uint8_t bank, uint8_t x);

#endif
