/*
 * apu_upload.c: sending the sound program and its data to the SPC700.
 *
 * The S-CPU streams bytes to the sound CPU's boot loader through the four
 * APU ports. The stream is read from ROM through a long pointer in direct
 * page plus an offset in Y.
 */
#include "machine.h"

/* Direct page $FB-$FD: the upload stream's long pointer (low, high, bank). */
#define APU_STREAM_PTR 0x0000FBu
#define APU_STREAM_BANK 0x0000FDu

/*
 * @routine 04:D788 ApuUploadNextByte
 * WHAT IT DOES: Returns the next byte of the sound data being uploaded to the
 * SPC700 and advances the stream past it.
 * HOW: Reads the byte at stream pointer + offset, then steps the offset.
 * When pointer + offset runs past the end of the 64 KB bank, the stream goes
 * on at $8000 of the next bank, which is the next 32 KB of LoROM: the
 * pointer's low word becomes 0, the offset becomes $8000, and the 16-bit word
 * holding the bank byte is incremented.
 * Entry: M 8-bit, X 16-bit, D = $0000; Y = offset.
 * Exit: A low byte = the byte read, A high byte unchanged; Y = new offset;
 * C set when the stream moved to the next bank; V from the same 16-bit add;
 * N and Z from the 16-bit A; M 8-bit; X, I, D unchanged.
 */
uint8_t ApuUploadNextByte(uint16_t *offset)
{
    uint32_t base = rd24(APU_STREAM_PTR);
    uint8_t value = rd8((base + *offset) & 0xFFFFFF);

    *offset += 1;
    if ((uint32_t)(base & 0xFFFF) + *offset > 0xFFFF) {
        wr16(APU_STREAM_PTR, 0);
        *offset = 0x8000;
        wr16(APU_STREAM_BANK, (uint16_t)(rd16(APU_STREAM_BANK) + 1));
    }
    return value;
}

void abi_ApuUploadNextByte(void)
{
    /* The carry and overflow the original's 16-bit add leaves behind. */
    uint16_t low = rd16(APU_STREAM_PTR);
    uint16_t next = (uint16_t)(cpu.y + 1);
    uint32_t sum = (uint32_t)low + next;
    uint8_t value = ApuUploadNextByte(&cpu.y);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | value);
    cpu.p = nz16(cpu.p, cpu.a);
    cpu.p = (uint8_t)((cpu.p & ~(P_C | P_V)) | (sum > 0xFFFF ? P_C : 0)
                      | ((~(next ^ low) & (next ^ sum) & 0x8000) ? P_V : 0));
    cpu.p |= P_M;
}
