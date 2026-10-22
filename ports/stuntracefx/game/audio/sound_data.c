/*
 * sound_data.c: getting a block of sound data into the SPC700. The block is
 * first put in Super FX RAM at $70:6000, either copied from ROM bank $1E or
 * unpacked there by Super FX program $01:CF42, then uploaded from there.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"

#include "audio_calls.h"

uint8_t GsuRun(uint8_t bank, uint16_t start);
void ApuPort1Send05(void);
void ApuSettleDelay(void);

/* Copy `bytes` from $1E:src to $70:6000, a word at a time. */
static void copy_to_6000(uint16_t src, uint16_t bytes)
{
    uint16_t first = (src == 0x8000) ? 144 : 154;   /* CMP, BNE, LDX lead in */
    uint16_t x;

    for (x = 0; x < bytes; x = (uint16_t)(x + 2))
        { cyc(x == 0 ? first : 154); wr16(0x706000 + x, rd16(0x1E0000 + (uint16_t)(src + x))); }
}

/* Put sound block `which` at $70:6000: $8000, $91BC and $AFC6 are copied
   from bank $1E ($11F8, $11F8 and $0DAC bytes); anything else is unpacked
   by Super FX program $01:CF42 with inputs `which` and `input`. */
static void put_block(uint16_t which, uint16_t input)
{
    if (which == 0x8000) {
        copy_to_6000(0x8000, 0x11F8);
    } else if (which == 0x91BC) {
        cyc(36);
        copy_to_6000(0x91BC, 0x11F8);
    } else if (which == 0xAFC6) {
        cyc(82);
        copy_to_6000(0xAFC6, 0x0DAC);
    } else {
        cyc(170);
        wr16(0x700068, which);
        cyc(56);
        wr16(0x70002C, 0x6000);
        cyc(56);
        wr16(0x7000A2, 0x0000);
        cyc(46);
        wr16(0x70006A, input);
        cyc(124);
        GsuRun(0x01, 0xCF42);
    }
    cyc(which == 0x8000 || which == 0x91BC || which == 0xAFC6 ? 200 : 204);
}

/* The last upload's end, for the adapters: Y = its stream offset (X = 0
   after it). */
static uint16_t up_y;

static void upload(uint8_t bank, uint16_t addr)
{
    up_y = ApuUpload(bank, addr).offset;
}

/*
 * @routine 03:F05E SoundDataLoad
 * WHAT IT DOES: Loads sound block A into the SPC700: puts it at $70:6000
 * (put_block: copied from bank $1E for $8000, $91BC or $AFC6, otherwise
 * unpacked by the Super FX with Y as its second input), uploads it with
 * ApuUpload, then waits a moment ($4000 turns of a countdown).
 * HOW: Word copies or GsuRun; ApuUpload from $70:6000; a DEC loop.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = block, Y = Super FX input.
 * Exit: A = 0; X = 0; Y as ApuUpload leaves it; M 16-bit; Z, C set, N, V
 * clear. (The copies for $91BC and $AFC6 are not reached by any script
 * yet.) Returns Y.
 */
uint16_t SoundDataLoad(uint16_t which, uint16_t input)
{
    put_block(which, input);
    upload(0x70, 0x6000);
    cyc(90);
    delay_turns(0x4000);
    return up_y;
}

void abi_SoundDataLoad(void)
{
    cpu.y = SoundDataLoad(cpu.a, cpu.y);
    cpu.a = 0x0000;
    cpu.x = 0x0000;
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_V | P_M | P_X)) | P_Z | P_C);
}

/* The table index for music `n`: n * 3, through direct page $F1. */
static uint16_t music_index(uint16_t n)
{
    cyc(40);
    wr16(0x0000F1, n);
    cyc(44);
    return (uint16_t)((uint16_t)(n << 1) + rd16(0x0000F1));
}

/* What MusicLoadForCourse and CourseSoundLoad leave, for their adapters:
   X (ml_x; 0 after an upload), and whether an upload set Y and cleared V. */
static uint16_t ml_x;
static int ml_uploaded;

/* One block list from the table entry at $04:D41E + `entry` (an address
   word, $FFFF for none, and a bank byte): ApuPort1Send05, then ApuUpload.
   The compare with $FFFF leaves C set either way. */
static void upload_entry(uint16_t entry)
{
    cyc(118);
    uint16_t addr = rd16(0x040000 | entry);

    cyc(42);
    if (addr == 0xFFFF)
        return;
    ApuPort1Send05();
    cyc(134);
    upload((uint8_t)rd16(0x040000 | (uint16_t)(entry + 2)), addr);
    ml_x = 0x0000;
    ml_uploaded = 1;
    cyc(62);
}

/*
 * @routine 04:D251 MusicLoadForCourse
 * WHAT IT DOES: Loads music A's sound data into the SPC700: the first
 * block list from the table at $04:D41E (three bytes per music: an address,
 * $FFFF for none, and a bank); then, in HUD mode 2, $1000 bytes of the
 * block for the course in $0D8A, copied from bank $19 to Super FX RAM at
 * $70:3000 and patched (+$10 to its word at 2, +$D30 to its words at 4, 6
 * and 22) before uploading it from there; otherwise the second list from
 * the table at $04:D42D. Each upload is preceded by ApuPort1Send05 and
 * followed by ApuSettleDelay.
 * HOW: The index is the music times 3 through direct page $F1; the copy is
 * MVN (byte by byte, each read then written). X is saved on the stack
 * around the first upload.
 * Entry: M 16-bit, X 16-bit, D = $0000; A low nibble = music. Exit: A = 0;
 * M, X 16-bit; Z, C set, N clear; X, Y and V as the last upload leaves
 * them (X = the index when the last list was empty).
 */
void MusicLoadForCourse(uint8_t bank, uint16_t music)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t i, src, k, v;
    static const struct { uint16_t off, add; } patch[4] = {
        {0x0002, 0x0010}, {0x0004, 0x0D30}, {0x0006, 0x0D30}, {0x0016, 0x0D30}};
    unsigned n;

    cyc(46);
    i = music_index((uint16_t)(music & 0x000F));
    ml_x = i;
    (cyc(106), ApuPort1Send05());
    cyc(26);
    upload_entry((uint16_t)(0xD41E + i));
    (cyc(44), ApuSettleDelay());
    ml_x = i;
    cyc(82);
    v = (uint16_t)(rd16(b | 0x0D2B) & 0x00FF);
    cyc(48);
    cyc(42);                            /* CMP #imm */
    if (v != 0x0002) {
        upload_entry((uint16_t)(0xD42D + i));
        (cyc(44), ApuSettleDelay());
        cyc(42);
        return;
    } else { cyc(42); }
    ApuPort1Send05();
    cyc(24);
    i = music_index((uint16_t)(rd16(b | 0x0D8A) & 0x000F));
    cyc(178);
    src = rd16(0x040000 | (uint16_t)(0xD41E + i));
    for (k = 0; k < 0x1000; k++)
        { cyc(44); wr8(0x703000 + k, rd8(0x190000 | (uint16_t)(src + k))); }
    cyc(84);
    for (n = 0; n < 4; n++) {
        cyc(32);
        v = rd16(0x703000 + patch[n].off);
        cyc(70);
        wr16(0x703000 + patch[n].off, (uint16_t)(v + patch[n].add));
    }
    cyc(110);
    upload(0x70, 0x3000);
    ml_x = 0x0000;
    ml_uploaded = 1;
    (cyc(62), ApuSettleDelay());
    cyc(42);
}

/* The registers MusicLoadForCourse leaves. */
static void music_exit(void)
{
    cpu.a = 0x0000;
    cpu.x = ml_x;
    if (ml_uploaded) {
        cpu.y = up_y;
        cpu.p &= (uint8_t)~P_V;
    }
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_M | P_X)) | P_Z | P_C);
}

void abi_MusicLoadForCourse(void)
{
    ml_uploaded = 0;
    MusicLoadForCourse(cpu.db, cpu.a);
    music_exit();
}

/* LDA #$4000, DEC A until 0 (16-bit A). */
static void delay_4000(void)
{
    delay_turns(0x4000);
}

/* 5 on APU port 1 (a word store), then the delay. */
static void port1_5_delay(void)
{
    cyc(100);
    hw_wr16(0x002141, 0x0005);
    (cyc(134), delay_4000());
}

/* PHP, REP #$30, port1_5_delay, an upload (bank, addr) or a sound data
   load (block, input), the delay after an upload, PLP. */
static void apu_step(int sound_data, uint16_t a, uint16_t y)
{
    port1_5_delay();
    if (sound_data) {
        SoundDataLoad(a, y);
    } else {
        upload((uint8_t)a, y);
        cyc(94);
        delay_4000();
    }
    cyc(42);                            /* PLP */
}

/* The upload of the word list entry at $04:list + x when it is not $FFFF:
   Y = its address, A = the bank byte after it, after ApuPort1Send05. */
static void upload_listed(uint16_t list, uint16_t x)
{
    uint16_t w;

    cyc(244);
    w = rd16(0x040000 + list + x);
    if (w == 0xFFFF)
        return;
    ApuPort1Send05();
    cyc(134);
    upload((uint8_t)rd16(0x040000 + list + 2 + x), w);
    ml_x = 0x0000;
    cyc(106);
}

/*
 * @routine 04:D0DB CourseSoundLoad
 * WHAT IT DOES: Loads the course's sound unless $10DA is set. With $10DC
 * set it is the other set, loaded once ($10DE counted down from 0 first):
 * block $19:8D4C, the $1518 bytes from $1E:9B2C through $70:6000, block
 * $1B:B8A0, then SoundReset and 4 on APU port 0. Otherwise, by the course
 * $0E3D (entry course * 3 of the tables in bank 04): the samples
 * (SoundDataLoad, block and input from $04:D322), the effects block from
 * $04:D376 and, after the music for the car $0D89 (MusicLoadForCourse),
 * the block from $04:D3CA (each skipped at $FFFF); in HUD mode 1 with
 * $0D4E clear also block $F9B8 (input $1D). Last it waits until all four
 * APU ports read 0.
 * HOW: P pushed; each upload after 5 on APU port 1 or ApuPort1Send05,
 * then a settling delay; X (the table offset) kept on the stack. The
 * $10DC set is not reached in the traces so far.
 * Entry: any widths; the data bank where work RAM and the APU ports are
 * mapped, D = $0000. Exit: P restored; A low byte 0 after the wait.
 */
/* How it ended, for the adapter: early (A's low byte the flag read), the
   other set (A = 4, X = 0, Y = the last offset's low byte) or the course's
   (A = 0, X as left, Y = the last upload's offset). */
enum { CSL_EARLY, CSL_OTHER, CSL_COURSE };
static int csl_kind;
static uint8_t csl_a;

void CourseSoundLoad(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v;
    uint16_t x, a;

    cyc(68);
    v = reg_rd(b | 0x10DA);
    if (v) {
        cyc(16);
        cyc(42);                        /* PLP */
        cyc(30);
        csl_kind = CSL_EARLY;
        csl_a = v;
        return;
    }
    cyc(46);
    v = reg_rd(b | 0x10DC);
    if (v) {
        cyc(46);
        v = reg_rd(b | 0x10DE);
        if (v) {
            cyc(46);                    /* BEQ, BRL */
            cyc(42);
            cyc(30);
            csl_kind = CSL_EARLY;
            csl_a = v;
            return;
        }
        cyc(46);
        v = (uint8_t)(rd8(b | 0x10DE) - 1);
        cyc(6);
        wr8(b | 0x10DE, v);
        (cyc(84), ApuPort1Send05());
        apu_step(0, 0x0019, 0x8D4C);
        delay_4000();
        cyc(18);
        hw_wr16(0x002141, 0x0005);
        delay_4000();
        for (x = 0x0000; x < 0x1518; x = (uint16_t)(x + 2)) {
            cyc(150);
            wr16(0x706000 + x, rd16(0x1E9B2C + x));
            cyc(4);
        }
        cyc(174);
        upload(0x70, 0x6000);
        (cyc(30), delay_4000());
        cyc(16);
        apu_step(0, 0x001B, 0xB8A0);
        SoundReset();
        cyc(62);
        hw_wr(b | 0x2140, 0x04);
        cyc(42);                        /* PLP */
        cyc(30);
        csl_kind = CSL_OTHER;
        return;
    }
    cyc(92);
    a = (uint16_t)(rd16(b | 0x0E3D) << 1);
    cyc(52);
    a = (uint16_t)(a + rd16(b | 0x0E3D));
    x = (uint16_t)(a & 0x00FF);
    (cyc(152), ApuPort1Send05());
    cyc(196);
    {
        uint16_t input = (uint16_t)(rd16(0x04D324 + x) & 0x00FF);

        SoundDataLoad(rd16(0x04D322 + x), input);
    }
    (cyc(106), ApuSettleDelay());
    ml_x = x;
    cyc(30);
    upload_listed(0xD376, x);
    ApuSettleDelay();
    cyc(46);
    MusicLoadForCourse(bank, rd16(b | 0x0D89));
    (cyc(62), ApuSettleDelay());
    ml_x = x;
    upload_listed(0xD3CA, x);
    ApuSettleDelay();
    cyc(46);
    if (reg_rd(b | 0x0D2B) == 0x01) {
        cyc(56);
        if (!reg_rd(b | 0x0D4E))
            { cyc(16); apu_step(1, 0xF9B8, 0x001D); ml_x = 0x0000; }
    } else { cyc(52); }
    do {
        cyc(32);
        if ((v = reg_rd(b | 0x2140)))
            { cyc(14); continue; }
        cyc(40);
        if ((v = reg_rd(b | 0x2141)))
            continue;
        cyc(40);
        if ((v = reg_rd(b | 0x2142)))
            continue;
        cyc(40);
        v = reg_rd(b | 0x2143);
    } while (v);                        /* each test is of the byte just loaded */
    cyc(46);
    cyc(42);                            /* PLP */
    csl_kind = CSL_COURSE;
}

void abi_CourseSoundLoad(void)
{
    CourseSoundLoad(cpu.db);
    switch (csl_kind) {
    case CSL_EARLY:
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | csl_a);
        break;
    case CSL_OTHER:
        cpu.a = 0x0004;
        cpu.x = 0x0000;
        cpu.y = (uint8_t)up_y;
        break;
    default:
        cpu.a = 0x0000;
        cpu.x = ml_x;
        cpu.y = up_y;
        break;
    }
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}
