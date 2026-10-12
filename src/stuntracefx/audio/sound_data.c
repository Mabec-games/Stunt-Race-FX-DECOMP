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
