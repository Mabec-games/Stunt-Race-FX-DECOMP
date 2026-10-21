/*
 * mixer_regs.c: the sound drivers' writes and reads of the sound chip's
 * 128 registers ($00-$7F, through the SPC700's $F2/$F3), as the mixer's
 * calls (mixer.h). A step on the way: it goes when the drivers' C calls
 * the mixer itself. The registers with no part (xA, xB, xE, $1D) and the
 * values the mixer does not report keep what was written.
 */
#include "mixer.h"
#include "mixer_regs.h"

static uint8_t r[128];

void mix_regs_reset(void)
{
    int i;
    for (i = 0; i < 128; i++)
        r[i] = 0;
    r[0x6C] = 0xE0;
}

void mix_reg_write(uint8_t reg, uint8_t v)
{
    int n = reg >> 4;
    reg &= 0x7F;
    r[reg] = v;
    switch (reg & 0x0F) {
    case 0x0: case 0x1: mix_voice_volume(n, (int8_t)r[n << 4], (int8_t)r[n << 4 | 1]); return;
    case 0x2: case 0x3: mix_voice_pitch(n, (uint16_t)(r[n << 4 | 2] | r[n << 4 | 3] << 8)); return;
    case 0x4: mix_voice_sample(n, v); return;
    case 0x5: case 0x6: {
        uint8_t a1 = r[n << 4 | 5], a2 = r[n << 4 | 6];
        mix_voice_envelope(n, a1 & 0x80, a1 & 0x0F, (a1 >> 4) & 7, a2 >> 5, a2 & 0x1F);
        return;
    }
    case 0x7: mix_voice_gain(n, v); return;
    case 0xF: mix_echo_filter(n, (int8_t)v); return;
    }
    switch (reg) {
    case 0x0C: case 0x1C: mix_master_volume((int8_t)r[0x0C], (int8_t)r[0x1C]); return;
    case 0x2C: case 0x3C: mix_echo_volume((int8_t)r[0x2C], (int8_t)r[0x3C]); return;
    case 0x4C: mix_key_on(v); return;
    case 0x5C: mix_key_off(v); return;
    case 0x6C: mix_control(v & 0x80, v & 0x40, v & 0x20, v & 0x1F); return;
    case 0x7C: mix_clear_ended(); return;
    case 0x0D: mix_echo_feedback((int8_t)v); return;
    case 0x2D: mix_pitch_modulation(v); return;
    case 0x3D: mix_noise(v); return;
    case 0x4D: mix_echo_voices(v); return;
    case 0x5D: mix_directory(v); return;
    case 0x6D: case 0x7D: mix_echo_buffer(r[0x6D], r[0x7D]); return;
    }
}

uint8_t mix_reg_read(uint8_t reg)
{
    reg &= 0x7F;
    if (reg == 0x7C)
        return mix_voices_ended();
    if ((reg & 0x0F) == 0x8)
        return mix_voice_level(reg >> 4);
    if ((reg & 0x0F) == 0x9)
        return (uint8_t)mix_voice_output(reg >> 4);
    return r[reg];
}
