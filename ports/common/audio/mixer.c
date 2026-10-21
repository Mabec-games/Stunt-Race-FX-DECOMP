/*
 * mixer.c: the sound, native (mixer.h).
 *
 * Written from Nocash's fullsnes ("SNES APU DSP" chapters) and checked
 * against bsnes's sound chip by mix_replay (check/), which feeds this mixer
 * the chip's inputs and compares every frame. Where fullsnes is silent the
 * behaviour here is what that check measured, and says so.
 *
 * A frame: the shared rate counter steps; the noise source steps at its
 * rate; every other frame the key-ons and releases are taken; each voice
 * makes its sample (Gaussian interpolation of its last four decoded values,
 * or the noise), scales it by its envelope and adds it to the main and echo
 * sums at its volumes; then the echo's delay line is read, filtered, mixed
 * in and written back. Values are the chip's: decoded samples 15 bits
 * signed, envelope levels 11 bits, sums 16 bits.
 */
#include <string.h>

#include "mixer.h"

enum { RELEASE, ATTACK, DECAY, SUSTAIN };

typedef struct {
    /* what the game set */
    int8_t vol_l, vol_r;
    uint16_t pitch;
    uint8_t entry;
    int adsr, attack, decay, sustain_level, sustain_rate;
    uint8_t gain;
    /* playing */
    int16_t hist[24];        /* the last 12 decoded values, twice (no wrap when read) */
    int hist_next;           /* where the next four go: 0, 4 or 8 */
    int pos;                 /* the pitch counter: bits 12-13 the value, 4-11 the Gaussian index */
    uint16_t block;          /* the BRR block being decoded */
    int byte;                /* the next pair of bytes in it: 1, 3, 5, 7 */
    int start_wait;          /* frames until a voice keyed on plays (5) */
    int cut_in;              /* frames until a voice that reached an end-and-mute block is cut */
    int stage, level;
    int bent;                /* the last level computed, 11 bits kept (bent increase) */
    int out;                 /* the last output, 15 bits (pitch modulation, OUTX) */
} Voice;

static uint8_t *mem;
static Voice voice[8];
static uint8_t dir_page, echo_page, echo_len_set;
static int8_t mvol_l, mvol_r, evol_l, evol_r, efb, fir[8];
static uint8_t pmon, non, eon, released, pending_on, ended;
static int reset_held, muted, echo_off, noise_rate;
static int counter, odd_frame;
static int noise;                         /* 15 bits */
static int echo_hist[8][2], echo_at;
static int echo_index, echo_length;

/* The chip's Gaussian interpolation table, as fullsnes gives it: gauss[i]
   weighs a value (511 - i) / 256 values away; each position's four
   weights sum to 2047-2049. */
static const int16_t gauss[512] = {
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

/* ---- rates ---------------------------------------------------------------------------
   fullsnes: a 5-bit rate is a step every N frames, N one of 1, 3 or 5
   times a power of two (rate 0 never). The chip drives them from one
   counter of 30,720 frames; each family of rates steps at its own phase of
   it (the powers of two at 0, the 3s at 1,040 frames, the 5s at 536) and
   the counter starts at 1 (both measured against bsnes's chip by
   mix_replay: envelopes there step on other frames without them). */
#define COUNTER_PERIOD 30720
static const uint16_t rate_frames[32] = {
    0, 2048, 1536, 1280, 1024, 768, 640, 512, 384, 320, 256, 192, 160, 128, 96, 80,
    64, 48, 40, 32, 24, 20, 16, 12, 10, 8, 6, 5, 4, 3, 2, 1,
};

static int rate_due(int rate)
{
    unsigned n = rate_frames[rate], phase;
    if (!n)
        return 0;
    phase = n % 3 == 0 ? 1040 : n % 5 == 0 ? 536 : 0;
    return ((unsigned)counter + phase) % n == 0;
}

static int clamp16(int v) { return v < -32768 ? -32768 : v > 32767 ? 32767 : v; }
static int wrap15(int v) { return (int16_t)(v << 1) >> 1; }

/* ---- setup ----------------------------------------------------------------------------- */
void mix_init(uint8_t *memory)
{
    mem = memory;
    memset(voice, 0, sizeof voice);
    dir_page = echo_page = echo_len_set = 0;
    mvol_l = mvol_r = evol_l = evol_r = efb = 0;
    memset(fir, 0, sizeof fir);
    pmon = non = eon = released = pending_on = ended = 0;
    /* fullsnes: after reset, released, muted, echo writes off, noise stopped */
    reset_held = muted = echo_off = 1;
    noise_rate = 0;
    counter = 1;
    odd_frame = 0;
    noise = 0x4000;                        /* -4000h */
    memset(echo_hist, 0, sizeof echo_hist);
    echo_at = 0;
    echo_index = echo_length = 0;
}

void mix_voice_volume(int v, int8_t left, int8_t right) { voice[v & 7].vol_l = left; voice[v & 7].vol_r = right; }
void mix_voice_pitch(int v, uint16_t pitch) { voice[v & 7].pitch = pitch & 0x3FFF; }
void mix_voice_sample(int v, uint8_t entry) { voice[v & 7].entry = entry; }

void mix_voice_envelope(int v, int adsr, int attack, int decay, int sustain_level, int sustain_rate)
{
    Voice *vc = &voice[v & 7];
    vc->adsr = adsr != 0;
    vc->attack = attack & 15;
    vc->decay = decay & 7;
    vc->sustain_level = sustain_level & 7;
    vc->sustain_rate = sustain_rate & 31;
}

void mix_voice_gain(int v, uint8_t gain) { voice[v & 7].gain = gain; }
void mix_key_on(uint8_t voices) { pending_on = voices; }
void mix_key_off(uint8_t voices) { released = voices; }
void mix_pitch_modulation(uint8_t voices) { pmon = voices; }
void mix_noise(uint8_t voices) { non = voices; }
void mix_echo_voices(uint8_t voices) { eon = voices; }
void mix_master_volume(int8_t left, int8_t right) { mvol_l = left; mvol_r = right; }

void mix_control(int reset, int mute, int echo_writes_off, int rate)
{
    reset_held = reset != 0;
    muted = mute != 0;
    echo_off = echo_writes_off != 0;
    noise_rate = rate & 31;
}

void mix_directory(uint8_t page) { dir_page = page; }
void mix_echo_buffer(uint8_t page, uint8_t length) { echo_page = page; echo_len_set = length & 15; }
void mix_echo_volume(int8_t left, int8_t right) { evol_l = left; evol_r = right; }
void mix_echo_feedback(int8_t feedback) { efb = feedback; }
void mix_echo_filter(int tap, int8_t coefficient) { fir[tap & 7] = coefficient; }

uint8_t mix_voices_ended(void) { return ended; }
void mix_clear_ended(void) { ended = 0; }
uint8_t mix_voice_level(int v) { return (uint8_t)(voice[v & 7].level >> 4); }
int8_t mix_voice_output(int v) { return (int8_t)(voice[v & 7].out >> 7); }

void mix_voice_states(int32_t *o)
{
    int i;
    for (i = 0; i < 8; i++) {
        o[5 * i] = voice[i].level;
        o[5 * i + 1] = voice[i].stage;
        o[5 * i + 2] = voice[i].pos;
        o[5 * i + 3] = voice[i].block;
        o[5 * i + 4] = voice[i].byte;
    }
}

/* ---- samples ---------------------------------------------------------------------------- */
static uint16_t dir_entry(const Voice *vc, int loop)
{
    uint16_t at = (uint16_t)(dir_page << 8 | vc->entry << 2) + (loop ? 2 : 0);
    return (uint16_t)(mem[at] | mem[(uint16_t)(at + 1)] << 8);
}

/* four more values of the voice's sample: one byte pair of its block,
   rebuilt with the block's shift and prediction filter (fullsnes, BRR) */
static void decode(Voice *vc, int v)
{
    uint8_t header = mem[vc->block];
    int shift = header >> 4, filter = (header >> 2) & 3, i;
    uint16_t at = (uint16_t)(vc->block + vc->byte);
    unsigned bits = (unsigned)mem[at] << 8 | mem[(uint16_t)(at + 1)];
    for (i = 0; i < 4; i++) {
        int k = vc->hist_next + i;
        int old = vc->hist[k + 11], older = vc->hist[k + 10];
        int nib = (int16_t)(bits << (i * 4)) >> 12, s;
        s = shift <= 12 ? (nib << shift) >> 1 : (nib >> 3) << 11;
        switch (filter) {
        case 1: s += old + ((-old) >> 4); break;
        case 2: s += 2 * old + ((-old * 3) >> 5) - older + (older >> 4); break;
        case 3: s += 2 * old + ((-old * 13) >> 6) - older + ((older * 3) >> 4); break;
        }
        s = wrap15(clamp16(s));
        vc->hist[k] = (int16_t)s;
        vc->hist[k + 12] = (int16_t)s;
    }
    /* the history is read as twelve values ending before hist_next: the
       slot before index 0 is index 11 */
    vc->hist_next = (vc->hist_next + 4) % 12;
    vc->byte += 2;
    if (vc->byte >= 9) {
        if (header & 1) {                  /* the end: on at the loop address */
            vc->block = dir_entry(vc, 1);
            ended |= (uint8_t)(1u << v);
        } else {
            vc->block = (uint16_t)(vc->block + 9);
            /* measured: a voice reaching a block that ends with no loop is
               cut 3 frames later, before the block plays (fullsnes: ENDX is
               set as that block starts) */
            if ((mem[vc->block] & 3) == 1)
                vc->cut_in = 3;
        }
        vc->byte = 1;
    }
}

/* fullsnes, Gaussian interpolation: the newest four values at the pitch
   counter's fraction; the first three sums wrap at 16 bits, the last
   saturates; 15 bits out */
static int interpolate(const Voice *vc)
{
    int i = (vc->pos >> 4) & 0xFF;
    const int16_t *h = &vc->hist[((vc->pos >> 12) + vc->hist_next) % 12];
    int out = (gauss[0xFF - i] * h[0]) >> 10;
    out += (gauss[0x1FF - i] * h[1]) >> 10;
    out += (gauss[0x100 + i] * h[2]) >> 10;
    out = (int16_t)out;
    out += (gauss[i] * h[3]) >> 10;
    return clamp16(out) >> 1;
}

/* the envelope's next level (fullsnes, ADSR/Gain). The stage moves on
   from the level computed even on a frame its rate does not step (as
   measured): attack ends past 7FFh, decay at the sustain level's top byte
   (in gain mode the gain's top bits, fullsnes's noted quirk). */
static void envelope(Voice *vc)
{
    int level = vc->level, rate, top;
    if (vc->stage == RELEASE) {
        level -= 8;
        vc->level = level < 0 ? 0 : level;
        return;
    }
    if (vc->adsr) {
        top = vc->sustain_level;
        if (vc->stage == ATTACK) {
            rate = vc->attack * 2 + 1;
            level += rate == 31 ? 0x400 : 0x20;
        } else {
            rate = vc->stage == DECAY ? vc->decay * 2 + 16 : vc->sustain_rate;
            level -= ((level - 1) >> 8) + 1;
        }
    } else {
        int mode = vc->gain >> 5;
        top = vc->gain >> 5;
        if (!(vc->gain & 0x80)) {          /* direct: a fixed level */
            level = vc->gain * 16;
            rate = 31;
        } else {
            rate = vc->gain & 31;
            if (mode == 4)
                level -= 0x20;
            else if (mode == 5)
                level -= ((level - 1) >> 8) + 1;
            else
                level += mode == 7 && (vc->bent & 0x7FF) >= 0x600 ? 8 : 0x20;
        }
    }
    if (vc->stage == DECAY && (level >> 8) == top)
        vc->stage = SUSTAIN;
    vc->bent = level;
    if (level < 0 || level > 0x7FF) {
        level = level < 0 ? 0 : 0x7FF;
        if (vc->stage == ATTACK)
            vc->stage = DECAY;
    }
    if (rate_due(rate))
        vc->level = level;
}

static void key_on(Voice *vc, int v)
{
    vc->block = dir_entry(vc, 0);
    vc->byte = 1;
    vc->hist_next = 0;
    memset(vc->hist, 0, sizeof vc->hist);
    vc->pos = 0;
    vc->level = vc->bent = 0;
    vc->stage = ATTACK;
    vc->start_wait = 5;
    vc->cut_in = 0;
    ended &= (uint8_t)~(1u << v);
}

void mix_frame(int16_t *out_l, int16_t *out_r)
{
    int main_l = 0, main_r = 0, echo_l = 0, echo_r = 0, v;
    if (--counter < 0)
        counter = COUNTER_PERIOD - 1;
    if (rate_due(noise_rate))
        noise = ((noise >> 1) & 0x3FFF) | (((noise ^ (noise >> 1)) & 1) << 14);
    /* fullsnes: key-on and release are taken every other frame */
    odd_frame ^= 1;
    if (odd_frame) {
        uint8_t on = pending_on;
        pending_on = 0;
        for (v = 0; v < 8; v++) {
            Voice *vc = &voice[v];
            if (on >> v & 1)
                key_on(vc, v);
            else if ((released >> v & 1) || reset_held)
                vc->stage = RELEASE;
            if (reset_held)
                vc->level = 0;
        }
    }
    for (v = 0; v < 8; v++) {
        Voice *vc = &voice[v];
        int pitch = vc->pitch, s, l, r;
        if (v > 0 && (pmon >> v & 1))
            pitch += ((voice[v - 1].out >> 4) * pitch) >> 10;
        if (vc->start_wait) {
            /* the frames after a key-on: silent while the first three
               byte pairs are decoded */
            if (--vc->start_wait <= 2)
                decode(vc, v);
            vc->out = 0;
            continue;
        }
        if (vc->cut_in && !--vc->cut_in) {
            vc->stage = RELEASE;
            vc->level = 0;
        }
        s = (non >> v & 1) ? (int16_t)(noise << 1) >> 1 : interpolate(vc);
        envelope(vc);
        s = (s * vc->level) >> 11;
        vc->out = s;
        /* fullsnes, output mixer: each voice's sample times its volume,
           SAR 6, the sums saturating at 16 bits */
        l = (s * vc->vol_l) >> 6;
        r = (s * vc->vol_r) >> 6;
        main_l = clamp16(main_l + l);
        main_r = clamp16(main_r + r);
        if (eon >> v & 1) {
            echo_l = clamp16(echo_l + l);
            echo_r = clamp16(echo_r + r);
        }
        vc->pos = (vc->pos & 0x3FFF) + pitch;
        if (vc->pos > 0x7FFF)
            vc->pos = 0x7FFF;
        if (vc->pos >= 0x4000) {
            decode(vc, v);
            vc->pos -= 0x4000;
        }
    }
    /* fullsnes, echo: the oldest entry out of the delay line, through the
       8-tap filter (the oldest seven summed without saturation, the
       newest with), mixed in at the echo volume; the echo voices and the
       feedback written back in its place */
    {
        uint16_t at = (uint16_t)((echo_page << 8) + echo_index);
        int fl = 0, fr = 0, i, ol, or_;
        echo_at = (echo_at + 1) & 7;
        echo_hist[echo_at][0] = (int16_t)(mem[at] | mem[(uint16_t)(at + 1)] << 8) >> 1;
        echo_hist[echo_at][1] = (int16_t)(mem[(uint16_t)(at + 2)] | mem[(uint16_t)(at + 3)] << 8) >> 1;
        for (i = 0; i < 7; i++) {
            const int *h = echo_hist[(echo_at + 1 + i) & 7];
            fl += (h[0] * fir[i]) >> 6;
            fr += (h[1] * fir[i]) >> 6;
        }
        fl = clamp16((int16_t)fl + ((echo_hist[echo_at][0] * fir[7]) >> 6)) & ~1;
        fr = clamp16((int16_t)fr + ((echo_hist[echo_at][1] * fir[7]) >> 6)) & ~1;
        ol = clamp16(((main_l * mvol_l) >> 7) + ((fl * evol_l) >> 7));
        or_ = clamp16(((main_r * mvol_r) >> 7) + ((fr * evol_r) >> 7));
        *out_l = (int16_t)(muted ? 0 : ol);
        *out_r = (int16_t)(muted ? 0 : or_);
        if (!echo_off) {
            int el = clamp16(echo_l + ((fl * efb) >> 7)) & ~1;
            int er = clamp16(echo_r + ((fr * efb) >> 7)) & ~1;
            mem[at] = (uint8_t)el;
            mem[(uint16_t)(at + 1)] = (uint8_t)(el >> 8);
            mem[(uint16_t)(at + 2)] = (uint8_t)er;
            mem[(uint16_t)(at + 3)] = (uint8_t)(er >> 8);
        }
        /* the length is taken when the index comes round to 0 */
        if (!echo_index)
            echo_length = echo_len_set * 0x800;
        echo_index += 4;
        if (echo_index >= echo_length)
            echo_index = 0;
    }
}
