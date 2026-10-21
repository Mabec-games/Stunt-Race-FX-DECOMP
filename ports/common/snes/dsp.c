/*
 * dsp.c: the S-DSP, one sample at a time (32 SPC700 cycles each).
 *
 * Eight voices play BRR-compressed samples from the APU's RAM through the
 * sample directory (DIR): each block of nine bytes holds a header (shift,
 * filter, loop and end flags) and sixteen 4-bit samples, rebuilt with the
 * block's prediction filter. A voice steps through its samples at its pitch
 * (14 bits, 0x1000 = the sample's own rate; pitch modulation scales it by
 * the voice before's output), interpolating between four with a bell-shaped
 * kernel, and scales them by its envelope (ADSR, or GAIN's direct, linear,
 * exponential and bent modes, each at its rate from the shared counter) and
 * its two volumes. A voice can play the noise generator instead. The echo
 * unit keeps a delay line in the APU's RAM (ESA, EDL x 2 KB), filters what
 * comes back through an 8-tap FIR, feeds it back (EFB) and mixes it in
 * (EVOL); the master volume (MVOL) and FLG's mute, reset and echo-write
 * bits apply last.
 *
 * The arithmetic (15-bit samples, clamping, the counter's rates and
 * offsets, the order of the FIR's sums) follows the chip as documented.
 * The interpolation kernel is computed (gauss_init): a Gaussian bell, the
 * chip's width, tapered to zero at two samples and normalised so each
 * position's four weights sum to 2048, rather than the chip's own table.
 */
#include <math.h>
#include <string.h>

#include "snes_internal.h"

enum { ENV_RELEASE, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN };

typedef struct {
    int buf[24];             /* the last 12 decoded samples, twice over */
    int buf_pos;             /* where the next four go (0, 4, 8) */
    int interp_pos;          /* 12-bit fraction, 2 bits of sample */
    uint16_t brr_addr;
    int brr_offset;          /* 1, 3, 5, 7: the next pair of bytes */
    uint8_t brr_header;
    int kon_delay;
    int env_mode;
    int env, hidden_env;
    int out;                 /* the last output, for the next voice's modulation */
} Voice;

static uint8_t *ram;
static uint8_t regs[128];
static Voice voice[8];
static int counter;
static int noise;
static int every_other;
static uint8_t new_kon;
static int echo_hist[8][2];
static int echo_hist_pos;
static int echo_offset, echo_length;
static int gauss[512];

#define R_MVOLL 0x0C
#define R_MVOLR 0x1C
#define R_EVOLL 0x2C
#define R_EVOLR 0x3C
#define R_KON   0x4C
#define R_KOFF  0x5C
#define R_FLG   0x6C
#define R_ENDX  0x7C
#define R_EFB   0x0D
#define R_PMON  0x2D
#define R_NON   0x3D
#define R_EON   0x4D
#define R_DIR   0x5D
#define R_ESA   0x6D
#define R_EDL   0x7D

#define SIMPLE_COUNTER_RANGE (2048 * 5 * 3)

static const unsigned counter_rates[32] = {
    SIMPLE_COUNTER_RANGE + 1, 2048, 1536, 1280, 1024, 768, 640, 512, 384, 320, 256, 192, 160, 128, 96, 80,
    64, 48, 40, 32, 24, 20, 16, 12, 10, 8, 6, 5, 4, 3, 2, 1,
};

static const unsigned counter_offsets[32] = {
    1, 0, 1040, 536, 0, 1040, 536, 0, 1040, 536, 0, 1040, 536, 0, 1040, 536,
    0, 1040, 536, 0, 1040, 536, 0, 1040, 536, 0, 1040, 536, 0, 1040, 0, 0,
};

/* 1 when the rate's event is not due this sample */
static int counter_wait(int rate)
{
    return ((unsigned)counter + counter_offsets[rate]) % counter_rates[rate] != 0;
}

static int clamp16(int v) { return v < -32768 ? -32768 : v > 32767 ? 32767 : v; }

/* the interpolation kernel, the chip's own table (as Nocash's fullsnes
   gives it): gauss[j] weighs a sample (511 - j) / 256 samples away (j =
   511 the nearest); each position's four weights sum to 2047-2049 */
static const int gauss_table[512] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
    2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5,
    6, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10,
    11, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 15, 16, 16, 17, 17,
    18, 19, 19, 20, 20, 21, 21, 22, 23, 23, 24, 24, 25, 26, 27, 27,
    28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 36, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
    58, 59, 60, 61, 62, 64, 65, 66, 67, 69, 70, 71, 73, 74, 76, 77,
    78, 80, 81, 83, 84, 86, 87, 89, 90, 92, 94, 95, 97, 99, 100, 102,
    104, 106, 107, 109, 111, 113, 115, 117, 118, 120, 122, 124, 126, 128, 130, 132,
    134, 137, 139, 141, 143, 145, 147, 150, 152, 154, 156, 159, 161, 163, 166, 168,
    171, 173, 175, 178, 180, 183, 186, 188, 191, 193, 196, 199, 201, 204, 207, 210,
    212, 215, 218, 221, 224, 227, 230, 233, 236, 239, 242, 245, 248, 251, 254, 257,
    260, 263, 267, 270, 273, 276, 280, 283, 286, 290, 293, 297, 300, 304, 307, 311,
    314, 318, 321, 325, 328, 332, 336, 339, 343, 347, 351, 354, 358, 362, 366, 370,
    374, 378, 381, 385, 389, 393, 397, 401, 405, 410, 414, 418, 422, 426, 430, 434,
    439, 443, 447, 451, 456, 460, 464, 469, 473, 477, 482, 486, 491, 495, 499, 504,
    508, 513, 517, 522, 527, 531, 536, 540, 545, 550, 554, 559, 563, 568, 573, 577,
    582, 587, 592, 596, 601, 606, 611, 615, 620, 625, 630, 635, 640, 644, 649, 654,
    659, 664, 669, 674, 678, 683, 688, 693, 698, 703, 708, 713, 718, 723, 728, 732,
    737, 742, 747, 752, 757, 762, 767, 772, 777, 782, 787, 792, 797, 802, 806, 811,
    816, 821, 826, 831, 836, 841, 846, 851, 855, 860, 865, 870, 875, 880, 884, 889,
    894, 899, 904, 908, 913, 918, 923, 927, 932, 937, 941, 946, 951, 955, 960, 965,
    969, 974, 978, 983, 988, 992, 997, 1001, 1005, 1010, 1014, 1019, 1023, 1027, 1032, 1036,
    1040, 1045, 1049, 1053, 1057, 1061, 1066, 1070, 1074, 1078, 1082, 1086, 1090, 1094, 1098, 1102,
    1106, 1109, 1113, 1117, 1121, 1125, 1128, 1132, 1136, 1139, 1143, 1146, 1150, 1153, 1157, 1160,
    1164, 1167, 1170, 1174, 1177, 1180, 1183, 1186, 1190, 1193, 1196, 1199, 1202, 1205, 1207, 1210,
    1213, 1216, 1219, 1221, 1224, 1227, 1229, 1232, 1234, 1237, 1239, 1241, 1244, 1246, 1248, 1251,
    1253, 1255, 1257, 1259, 1261, 1263, 1265, 1267, 1269, 1270, 1272, 1274, 1275, 1277, 1279, 1280,
    1282, 1283, 1284, 1286, 1287, 1288, 1290, 1291, 1292, 1293, 1294, 1295, 1296, 1297, 1297, 1298,
    1299, 1300, 1300, 1301, 1302, 1302, 1303, 1303, 1303, 1304, 1304, 1304, 1304, 1304, 1305, 1305,
};

static void gauss_init(void)
{
    memcpy(gauss, gauss_table, sizeof gauss);
}

void dsp_reset(uint8_t *aram)
{
    static int made;
    if (!made) {
        gauss_init();
        made = 1;
    }
    ram = aram;
    memset(regs, 0, sizeof regs);
    memset(voice, 0, sizeof voice);
    regs[R_FLG] = 0xE0;
    counter = 0;
    noise = 0x4000;
    every_other = 0;
    new_kon = 0;
    memset(echo_hist, 0, sizeof echo_hist);
    echo_hist_pos = 0;
    echo_offset = 0;
    echo_length = 0;
}

uint8_t dsp_read(uint8_t reg) { return regs[reg & 0x7F]; }

void dsp_write(uint8_t reg, uint8_t v)
{
    reg &= 0x7F;
    regs[reg] = v;
    if (reg == R_KON)
        new_kon = v;
    else if (reg == R_ENDX)
        regs[R_ENDX] = 0;     /* any write clears it */
}

/* four more samples of the voice's BRR stream into its buffer */
static void brr_decode(Voice *vc, int ch)
{
    uint8_t header = ram[vc->brr_addr];
    int shift = header >> 4, filter = header & 0x0C, i;
    uint16_t a = (uint16_t)(vc->brr_addr + vc->brr_offset);
    unsigned nyb = (unsigned)ram[a] << 8 | ram[(uint16_t)(a + 1)];
    vc->brr_header = header;
    for (i = 0; i < 4; i++) {
        int s = (int16_t)(nyb << (i * 4)) >> 12;
        int pos = vc->buf_pos + i;
        int p1 = vc->buf[(pos + 11) % 12];
        int p2 = vc->buf[(pos + 10) % 12] >> 1;
        s = (s << shift) >> 1;
        if (shift >= 0x0D)
            s = (s >> 25) << 11;
        if (filter >= 8) {
            s += p1;
            s -= p2;
            if (filter == 8) {
                s += p2 >> 4;
                s += (p1 * -3) >> 6;
            } else {
                s += (p1 * -13) >> 7;
                s += (p2 * 3) >> 4;
            }
        } else if (filter) {
            s += p1 >> 1;
            s += (-p1) >> 5;
        }
        s = clamp16(s);
        s = (int16_t)(s * 2);
        vc->buf[pos] = s;
        vc->buf[pos + 12] = s;
    }
    vc->buf_pos = (vc->buf_pos + 4) % 12;
    vc->brr_offset += 2;
    if (vc->brr_offset >= 9) {
        /* the block's end: on to the next block, or to the loop point */
        if (header & 1) {
            uint16_t dir = (uint16_t)(regs[R_DIR] << 8) + regs[(ch << 4) | 4] * 4;
            vc->brr_addr = (uint16_t)(ram[(uint16_t)(dir + 2)] | ram[(uint16_t)(dir + 3)] << 8);
            regs[R_ENDX] |= (uint8_t)(1u << ch);
            if (!(header & 2)) {
                vc->env_mode = ENV_RELEASE;
                vc->env = 0;
            }
        } else {
            vc->brr_addr = (uint16_t)(vc->brr_addr + 9);
        }
        vc->brr_offset = 1;
    }
}

static void run_envelope(Voice *vc, int ch)
{
    int env = vc->env, rate, env_data;
    uint8_t adsr1 = regs[(ch << 4) | 5];
    if (vc->env_mode == ENV_RELEASE) {
        env -= 8;
        if (env < 0)
            env = 0;
        vc->env = env;
        return;
    }
    env_data = regs[(ch << 4) | 6];
    if (adsr1 & 0x80) {
        if (vc->env_mode >= ENV_DECAY) {
            env--;
            env -= env >> 8;
            rate = env_data & 0x1F;
            if (vc->env_mode == ENV_DECAY)
                rate = ((adsr1 >> 3) & 0x0E) + 0x10;
        } else {
            rate = (adsr1 & 0x0F) * 2 + 1;
            env += rate < 31 ? 0x20 : 0x400;
        }
    } else {
        int mode;
        env_data = regs[(ch << 4) | 7];
        mode = env_data >> 5;
        if (mode < 4) {
            env = env_data * 0x10;
            rate = 31;
        } else {
            rate = env_data & 0x1F;
            if (mode == 4)
                env -= 0x20;
            else if (mode < 6) {
                env--;
                env -= env >> 8;
            } else {
                env += 0x20;
                if (mode > 6 && (unsigned)vc->hidden_env >= 0x600)
                    env += 0x8 - 0x20;
            }
        }
    }
    if ((env >> 8) == (env_data >> 5) && vc->env_mode == ENV_DECAY)
        vc->env_mode = ENV_SUSTAIN;
    vc->hidden_env = env;
    if ((unsigned)env > 0x7FF) {
        env = env < 0 ? 0 : 0x7FF;
        if (vc->env_mode == ENV_ATTACK)
            vc->env_mode = ENV_DECAY;
    }
    if (!counter_wait(rate))
        vc->env = env;
}

static int interpolate(const Voice *vc)
{
    int off = (vc->interp_pos >> 4) & 0xFF;
    const int *fwd = gauss + 255 - off, *rev = gauss + off;
    const int *in = &vc->buf[((vc->interp_pos >> 12) + vc->buf_pos) % 12];
    int out;
    out = (fwd[0] * in[0]) >> 11;
    out += (fwd[256] * in[1]) >> 11;
    out += (rev[256] * in[2]) >> 11;
    out = (int16_t)out;
    out += (rev[0] * in[3]) >> 11;
    out = clamp16(out);
    return out & ~1;
}

void dsp_sample(int16_t *lo, int16_t *ro)
{
    int main_l = 0, main_r = 0, echo_l = 0, echo_r = 0, ch;
    uint8_t flg = regs[R_FLG];
    if (--counter < 0)
        counter = SIMPLE_COUNTER_RANGE - 1;
    if (!counter_wait(flg & 0x1F)) {
        int fb = (noise << 13) ^ (noise << 14);
        noise = (fb & 0x4000) ^ (noise >> 1);
    }
    every_other ^= 1;
    if (every_other) {
        /* key on and key off are taken every other sample */
        uint8_t kon = new_kon;
        new_kon = 0;
        for (ch = 0; ch < 8; ch++) {
            Voice *vc = &voice[ch];
            if (kon & (1u << ch)) {
                uint16_t dir = (uint16_t)(regs[R_DIR] << 8) + regs[(ch << 4) | 4] * 4;
                vc->brr_addr = (uint16_t)(ram[dir] | ram[(uint16_t)(dir + 1)] << 8);
                vc->brr_offset = 1;
                vc->buf_pos = 0;
                memset(vc->buf, 0, sizeof vc->buf);
                vc->interp_pos = 0;
                vc->env = vc->hidden_env = 0;
                vc->env_mode = ENV_ATTACK;
                vc->kon_delay = 5;
                regs[R_ENDX] &= (uint8_t)~(1u << ch);
            }
            if ((regs[R_KOFF] & (1u << ch)) || (flg & 0x80))
                if (!(kon & (1u << ch)))
                    vc->env_mode = ENV_RELEASE;
            if (flg & 0x80)
                vc->env = 0;
        }
    }
    for (ch = 0; ch < 8; ch++) {
        Voice *vc = &voice[ch];
        int pitch = (regs[(ch << 4) | 2] | regs[(ch << 4) | 3] << 8) & 0x3FFF;
        int out, l, r;
        if (ch > 0 && (regs[R_PMON] & (1u << ch)))
            pitch += ((voice[ch - 1].out >> 5) * pitch) >> 10;
        if (vc->kon_delay) {
            /* the first samples decoded while the voice starts silent */
            vc->kon_delay--;
            if (vc->kon_delay == 2 || vc->kon_delay == 1 || vc->kon_delay == 0)
                brr_decode(vc, ch);
            vc->out = 0;
            regs[(ch << 4) | 8] = 0;
            regs[(ch << 4) | 9] = 0;
            continue;
        }
        if (regs[R_NON] & (1u << ch))
            out = (int16_t)(noise * 2);
        else
            out = interpolate(vc);
        run_envelope(vc, ch);
        out = ((out * vc->env) >> 11) & ~1;
        vc->out = out;
        regs[(ch << 4) | 8] = (uint8_t)(vc->env >> 4);
        regs[(ch << 4) | 9] = (uint8_t)(out >> 8);
        l = (out * (int8_t)regs[(ch << 4) | 0]) >> 7;
        r = (out * (int8_t)regs[(ch << 4) | 1]) >> 7;
        main_l = clamp16(main_l + l);
        main_r = clamp16(main_r + r);
        if (regs[R_EON] & (1u << ch)) {
            echo_l = clamp16(echo_l + l);
            echo_r = clamp16(echo_r + r);
        }
        vc->interp_pos = (vc->interp_pos & 0x3FFF) + pitch;
        if (vc->interp_pos > 0x7FFF)
            vc->interp_pos = 0x7FFF;
        if (vc->interp_pos >= 0x4000) {
            brr_decode(vc, ch);
            vc->interp_pos -= 0x4000;
        }
    }
    /* the echo: read the delay line, filter, mix, write back */
    {
        uint16_t base = (uint16_t)(regs[R_ESA] << 8);
        uint16_t at = (uint16_t)(base + echo_offset);
        int in_l = (int16_t)(ram[at] | ram[(uint16_t)(at + 1)] << 8) >> 1;
        int in_r = (int16_t)(ram[(uint16_t)(at + 2)] | ram[(uint16_t)(at + 3)] << 8) >> 1;
        int fl, fr, i, out_l, out_r;
        echo_hist_pos = (echo_hist_pos + 1) & 7;
        echo_hist[echo_hist_pos][0] = in_l;
        echo_hist[echo_hist_pos][1] = in_r;
        fl = fr = 0;
        for (i = 0; i < 7; i++) {
            int h = (echo_hist_pos + 1 + i) & 7;
            fl += (echo_hist[h][0] * (int8_t)regs[(i << 4) | 0x0F]) >> 6;
            fr += (echo_hist[h][1] * (int8_t)regs[(i << 4) | 0x0F]) >> 6;
        }
        fl = (int16_t)fl;
        fr = (int16_t)fr;
        fl += (echo_hist[echo_hist_pos][0] * (int8_t)regs[0x7F]) >> 6;
        fr += (echo_hist[echo_hist_pos][1] * (int8_t)regs[0x7F]) >> 6;
        fl = clamp16(fl) & ~1;
        fr = clamp16(fr) & ~1;
        out_l = clamp16(((main_l * (int8_t)regs[R_MVOLL]) >> 7) + ((fl * (int8_t)regs[R_EVOLL]) >> 7));
        out_r = clamp16(((main_r * (int8_t)regs[R_MVOLR]) >> 7) + ((fr * (int8_t)regs[R_EVOLR]) >> 7));
        if (flg & 0x40)
            out_l = out_r = 0;
        *lo = (int16_t)out_l;
        *ro = (int16_t)out_r;
        if (!(flg & 0x20)) {
            int el = clamp16(echo_l + ((fl * (int8_t)regs[R_EFB]) >> 7)) & ~1;
            int er = clamp16(echo_r + ((fr * (int8_t)regs[R_EFB]) >> 7)) & ~1;
            ram[at] = (uint8_t)el;
            ram[(uint16_t)(at + 1)] = (uint8_t)(el >> 8);
            ram[(uint16_t)(at + 2)] = (uint8_t)er;
            ram[(uint16_t)(at + 3)] = (uint8_t)(er >> 8);
        }
        if (!echo_offset)
            echo_length = (regs[R_EDL] & 0x0F) * 0x800;
        echo_offset += 4;
        if (echo_offset >= echo_length)
            echo_offset = 0;
    }
}
