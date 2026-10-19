/*
 * sound_data.c: getting a block of sound data into the SPC700. The block is
 * first put in Super FX RAM at $70:6000, either copied from ROM bank $1E or
 * unpacked there by Super FX program $01:CF42, then uploaded from there.
 */
#include "machine.h"
#include "cpu_ops.h"

uint8_t GsuRun(uint8_t bank, uint16_t start);
void ApuUpload(uint8_t bank, uint16_t addr);
void abi_ApuUpload(void);

/* Copy `bytes` from $1E:src to $70:6000, a word at a time. */
static void copy_to_6000(uint16_t src, uint16_t bytes)
{
    uint16_t x;

    for (x = 0; x < bytes; x = (uint16_t)(x + 2))
        { cyc(154); wr16(0x706000 + x, rd16(0x1E0000 + (uint16_t)(src + x))); }
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
    cyc(204);
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
 * yet.)
 */
void SoundDataLoad(uint16_t which, uint16_t input)
{
    put_block(which, input);
    ApuUpload(0x70, 0x6000);
    cyc(180);
    delay_turns(0x4000);
}

void abi_SoundDataLoad(void)
{
    put_block(cpu.a, cpu.y);
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.a = 0x0070;
    cpu.y = 0x6000;
    abi_ApuUpload();
    cyc(90);
    delay_turns(0x4000);
    cpu.a = 0x0000;
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_M)) | P_Z);
}

void ApuPort1Send05(void);
void ApuSettleDelay(void);
void abi_ApuPort1Send05(void);
void abi_ApuSettleDelay(void);

/* CMP #imm in 16 bits: N, Z and C. */
static uint8_t cmp16(uint8_t p, uint16_t a, uint16_t m)
{
    return (uint8_t)((nz16(p, (uint16_t)(a - m)) & ~P_C) | (a >= m ? P_C : 0));
}

/* The table index for music `n`: n * 3, through direct page $F1. */
static uint16_t music_index(uint16_t n)
{
    cyc(40);
    wr16(0x0000F1, n);
    cyc(44);
    return (uint16_t)((uint16_t)(n << 1) + rd16(0x0000F1));
}

/* One block list from the table entry at $04:D41E + `entry` (an address
   word, $FFFF for none, and a bank byte): ApuPort1Send05, then ApuUpload. */
static void upload_entry(uint16_t entry)
{
    cyc(118);
    uint16_t addr = rd16(0x040000 | entry);

    cpu.a = addr;
    cpu.p = cmp16(cpu.p, addr, 0xFFFF);
    if (addr == 0xFFFF)
        return;
    cpu.y = addr;
    abi_ApuPort1Send05();
    cyc(150);
    cpu.a = (uint16_t)(rd16(0x040000 | (uint16_t)(entry + 2)) & 0x00FF);
    abi_ApuUpload();
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
 * M 16-bit; Z set, N clear; X, Y, C and V as the last call or compare
 * leaves them. The register-level steps are in the abi_ wrapper; this is
 * the same work without the registers.
 */
void MusicLoadForCourse(uint8_t bank, uint16_t music)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t i = music_index((uint16_t)(music & 0x000F)), addr, src, k, v;
    static const uint16_t patch[4][2] = {{0x0002, 0x0010}, {0x0004, 0x0D30}, {0x0006, 0x0D30}, {0x0016, 0x0D30}};
    unsigned n;

    ApuPort1Send05();
    addr = rd16(0x040000 | (uint16_t)(0xD41E + i));
    if (addr != 0xFFFF) {
        ApuPort1Send05();
        ApuUpload((uint8_t)rd16(0x040000 | (uint16_t)(0xD420 + i)), addr);
    }
    ApuSettleDelay();
    if ((rd16(b | 0x0D2B) & 0x00FF) != 0x0002) {
        addr = rd16(0x040000 | (uint16_t)(0xD42D + i));
        if (addr != 0xFFFF) {
            ApuPort1Send05();
            ApuUpload((uint8_t)rd16(0x040000 | (uint16_t)(0xD42F + i)), addr);
        }
        ApuSettleDelay();
        return;
    }
    ApuPort1Send05();
    i = music_index((uint16_t)(rd16(b | 0x0D8A) & 0x000F));
    src = rd16(0x040000 | (uint16_t)(0xD41E + i));
    for (k = 0; k < 0x1000; k++)
        wr8(0x703000 + k, rd8(0x190000 | (uint16_t)(src + k)));
    for (n = 0; n < 4; n++) {
        v = rd16(0x703000 + patch[n][0]);
        wr16(0x703000 + patch[n][0], (uint16_t)(v + patch[n][1]));
    }
    ApuUpload(0x70, 0x3000);
    ApuSettleDelay();
}

void abi_MusicLoadForCourse(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t i, src, k, v;
    static const struct { uint16_t off, add; } patch[4] = {
        {0x0002, 0x0010}, {0x0004, 0x0D30}, {0x0006, 0x0D30}, {0x0016, 0x0D30}};
    unsigned n;

    cyc(46);
    i = music_index((uint16_t)(cpu.a & 0x000F));
    cpu.x = i;
    (cyc(22), abi_ApuPort1Send05());
    cpu.p &= (uint8_t)~(P_M | P_X);
    upload_entry((uint16_t)(0xD41E + i));
    cpu.p &= (uint8_t)~(P_M | P_X);
    abi_ApuSettleDelay();
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.x = i;
    cyc(158);
    v = (uint16_t)(rd16(b | 0x0D2B) & 0x00FF);
    cyc(48);
    cpu.p = cmp16(cpu.p, v, 0x0002);
    if (v != 0x0002) {
        upload_entry((uint16_t)(0xD42D + i));
        cpu.p &= (uint8_t)~(P_M | P_X);
        (cyc(44), abi_ApuSettleDelay());
        return;
    }
    abi_ApuPort1Send05();
    cpu.p &= (uint8_t)~P_M;
    cyc(40);
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
    cpu.x = (uint16_t)(src + 0x1000);
    cpu.y = 0x3000;
    cpu.a = 0x0070;
    abi_ApuUpload();
    abi_ApuSettleDelay();
}

void abi_SoundReset(void);
void abi_MusicLoadForCourse(void);

/* LDA #$4000, DEC A until 0 (16-bit A): A = 0, Z set. */
static void delay_4000(void)
{
    delay_turns(0x4000);
    cpu.a = 0x0000;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
}

/* 5 on APU port 1 (a word store), then the delay. */
static void port1_5_delay(void)
{
    cpu.a = 0x0005;
    cyc(116);
    wr16(0x002141, 0x0005);
    (cyc(134), delay_4000());
}

/* PHP, REP #$30, port1_5_delay, the upload or load with A and Y, (REP
   #$20 and the delay after an upload), PLP. */
static void apu_step(void (*load)(void), uint16_t a, uint16_t y, int delay_after)
{
    uint8_t p = cpu.p;

    cpu.p &= (uint8_t)~(P_M | P_X);
    port1_5_delay();
    cpu.a = a;
    cpu.y = y;
    cpu.p = nz16(cpu.p, y);
    load();
    if (delay_after) {
        cyc(94);
        cpu.p &= (uint8_t)~P_M;
        delay_4000();
    }
    op_plp(p);
}

/* The upload of the word list entry at $04:list + X when it is not $FFFF:
   Y = its address, A = the bank byte after it, after ApuPort1Send05. */
static void upload_listed(uint16_t list)
{
    uint16_t w;

    cpu.p &= (uint8_t)~(P_M | P_X);
    cyc(266);
    w = rd16(0x040000 + list + cpu.x);
    cpu.a = w;
    cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(w - 0xFFFF)) & ~P_C) | (w >= 0xFFFF ? P_C : 0));
    if (w == 0xFFFF)
        return;
    cpu.y = w;
    cpu.p = nz16(cpu.p, w);
    abi_ApuPort1Send05();
    cyc(150);
    cpu.a = (uint16_t)(rd16(0x040000 + list + 2 + cpu.x) & 0x00FF);
    cpu.p = nz16(cpu.p, cpu.a);
    abi_ApuUpload();
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
void abi_CourseSoundLoad(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p, v;
    uint16_t x, a;

    op_sep(P_M);
    cyc(202);
    op_lda8(b | 0x10DA);
    if ((uint8_t)cpu.a) {
        cyc(16);
        op_plp(p0);
        return;
    }
    cyc(46);
    op_lda8(b | 0x10DC);
    if ((uint8_t)cpu.a) {
        cyc(46);
        op_lda8(b | 0x10DE);
        if ((uint8_t)cpu.a) {
            cyc(46);
            op_plp(p0);
            return;
        }
        cyc(46);
        v = (uint8_t)(rd8(b | 0x10DE) - 1);
        cyc(6);
        wr8(b | 0x10DE, v);
        cpu.p = nz8(cpu.p, v);
        cpu.p &= (uint8_t)~(P_M | P_X);
        abi_ApuPort1Send05();
        apu_step(abi_ApuUpload, 0x0019, 0x8D4C, 1);
        delay_4000();
        cpu.a = 0x0005;
        cyc(18);
        wr16(0x002141, 0x0005);
        cpu.p &= (uint8_t)~P_M;
        delay_4000();
        for (x = 0x0000; x < 0x1518; x = (uint16_t)(x + 2)) {
            cyc(154);
            cpu.a = rd16(0x1E9B2C + x);
            wr16(0x706000 + x, cpu.a);
        }
        cyc(178);
        cpu.x = x;
        cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z | P_C);
        cpu.y = 0x6000;
        cpu.a = 0x0070;
        cpu.p = nz16(cpu.p, 0x0070);
        abi_ApuUpload();
        cpu.p &= (uint8_t)~P_M;
        (cyc(30), delay_4000());
        apu_step(abi_ApuUpload, 0x001B, 0xB8A0, 1);
        abi_SoundReset();
        op_sep(P_M);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x04);
        cpu.p = nz8(cpu.p, 0x04);
        cyc(134);
        wr8(b | 0x2140, 0x04);
        op_plp(p0);
        return;
    }
    cpu.p &= (uint8_t)~(P_M | P_X);
    cyc(92);
    a = rd16(b | 0x0E3D);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 15));
    a = (uint16_t)(a << 1);
    cpu.p &= (uint8_t)~P_C;
    cyc(52);
    a = op_adc16(a, rd16(b | 0x0E3D), &cpu.p);
    x = (uint16_t)(a & 0x00FF);
    cpu.a = x;
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
    (cyc(68), abi_ApuPort1Send05());
    cyc(212);
    cpu.y = (uint16_t)(rd16(0x04D324 + x) & 0x00FF);
    cpu.a = rd16(0x04D322 + x);
    cpu.p = nz16(cpu.p, cpu.a);
    abi_SoundDataLoad();
    cpu.p &= (uint8_t)~(P_M | P_X);
    abi_ApuSettleDelay();
    cpu.x = x;
    upload_listed(0xD376);
    cpu.p &= (uint8_t)~(P_M | P_X);
    abi_ApuSettleDelay();
    cpu.p &= (uint8_t)~P_M;
    cyc(122);
    op_lda16(b | 0x0D89);
    abi_MusicLoadForCourse();
    abi_ApuSettleDelay();
    cpu.x = x;
    upload_listed(0xD3CA);
    cpu.p &= (uint8_t)~(P_M | P_X);
    abi_ApuSettleDelay();
    op_sep(P_M);
    cyc(122);
    op_lda8(b | 0x0D2B);
    if ((uint8_t)cpu.a == 0x01) {
        cyc(56);
        op_lda8(b | 0x0D4E);
        if (!(uint8_t)cpu.a)
            apu_step(abi_SoundDataLoad, 0xF9B8, 0x001D, 0);
    } else { cyc(52); }
    op_sep(P_M);
    do {
        cyc(32);
        op_lda8(b | 0x2140);
        if ((uint8_t)cpu.a)
            { cyc(14); continue; }
        cyc(40);
        op_lda8(b | 0x2141);
        if ((uint8_t)cpu.a)
            continue;
        cyc(40);
        op_lda8(b | 0x2142);
        if ((uint8_t)cpu.a)
            continue;
        cyc(40);
        op_lda8(b | 0x2143);
    } while ((uint8_t)cpu.a);
    cyc(16);
    op_plp(p0);
}
