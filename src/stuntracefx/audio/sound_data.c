/*
 * sound_data.c: getting a block of sound data into the SPC700. The block is
 * first put in Super FX RAM at $70:6000, either copied from ROM bank $1E or
 * unpacked there by Super FX program $01:CF42, then uploaded from there.
 */
#include "machine.h"

uint8_t GsuRun(uint8_t bank, uint16_t start);
void ApuUpload(uint8_t bank, uint16_t addr);
void abi_ApuUpload(void);

/* Copy `bytes` from $1E:src to $70:6000, a word at a time. */
static void copy_to_6000(uint16_t src, uint16_t bytes)
{
    uint16_t x;

    for (x = 0; x < bytes; x = (uint16_t)(x + 2))
        wr16(0x706000 + x, rd16(0x1E0000 + (uint16_t)(src + x)));
}

/* Put sound block `which` at $70:6000: $8000, $91BC and $AFC6 are copied
   from bank $1E ($11F8, $11F8 and $0DAC bytes); anything else is unpacked
   by Super FX program $01:CF42 with inputs `which` and `input`. */
static void put_block(uint16_t which, uint16_t input)
{
    if (which == 0x8000) {
        copy_to_6000(0x8000, 0x11F8);
    } else if (which == 0x91BC) {
        copy_to_6000(0x91BC, 0x11F8);
    } else if (which == 0xAFC6) {
        copy_to_6000(0xAFC6, 0x0DAC);
    } else {
        wr16(0x700068, which);
        wr16(0x70002C, 0x6000);
        wr16(0x7000A2, 0x0000);
        wr16(0x70006A, input);
        GsuRun(0x01, 0xCF42);
    }
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
    uint16_t wait = 0x4000;

    put_block(which, input);
    ApuUpload(0x70, 0x6000);
    while (--wait)
        ;
}

void abi_SoundDataLoad(void)
{
    put_block(cpu.a, cpu.y);
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.a = 0x0070;
    cpu.y = 0x6000;
    abi_ApuUpload();
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
    wr16(0x0000F1, n);
    return (uint16_t)((uint16_t)(n << 1) + rd16(0x0000F1));
}

/* One block list from the table entry at $04:D41E + `entry` (an address
   word, $FFFF for none, and a bank byte): ApuPort1Send05, then ApuUpload. */
static void upload_entry(uint16_t entry)
{
    uint16_t addr = rd16(0x040000 | entry);

    cpu.a = addr;
    cpu.p = cmp16(cpu.p, addr, 0xFFFF);
    if (addr == 0xFFFF)
        return;
    cpu.y = addr;
    abi_ApuPort1Send05();
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

    i = music_index((uint16_t)(cpu.a & 0x000F));
    cpu.x = i;
    abi_ApuPort1Send05();
    cpu.p &= (uint8_t)~(P_M | P_X);
    upload_entry((uint16_t)(0xD41E + i));
    cpu.p &= (uint8_t)~(P_M | P_X);
    abi_ApuSettleDelay();
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.x = i;
    v = (uint16_t)(rd16(b | 0x0D2B) & 0x00FF);
    cpu.p = cmp16(cpu.p, v, 0x0002);
    if (v != 0x0002) {
        upload_entry((uint16_t)(0xD42D + i));
        cpu.p &= (uint8_t)~(P_M | P_X);
        abi_ApuSettleDelay();
        return;
    }
    abi_ApuPort1Send05();
    cpu.p &= (uint8_t)~P_M;
    i = music_index((uint16_t)(rd16(b | 0x0D8A) & 0x000F));
    src = rd16(0x040000 | (uint16_t)(0xD41E + i));
    for (k = 0; k < 0x1000; k++)
        wr8(0x703000 + k, rd8(0x190000 | (uint16_t)(src + k)));
    for (n = 0; n < 4; n++) {
        v = rd16(0x703000 + patch[n].off);
        wr16(0x703000 + patch[n].off, (uint16_t)(v + patch[n].add));
    }
    cpu.x = (uint16_t)(src + 0x1000);
    cpu.y = 0x3000;
    cpu.a = 0x0070;
    abi_ApuUpload();
    abi_ApuSettleDelay();
}
