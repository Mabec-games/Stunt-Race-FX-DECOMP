/*
 * apu_upload.c: sending the sound program and its data to the SPC700.
 *
 * At boot the SPC700 runs its built-in loader. The S-CPU sends it blocks of
 * bytes through the four APU ports with the loader's handshake, then tells it
 * where to start. The data is a stream in ROM: a list of blocks, each a
 * 16-bit length and a 16-bit SPC700 address followed by that many bytes; a
 * block of length 0 ends the list and its address is where the SPC700 starts.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hw.h"
#include "wram.h"

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
/* The byte, then the pointer's low word read again for the add (in *low). */
static uint8_t apu_upload_next_byte(uint16_t *offset, uint16_t *low)
{
    uint32_t base = rd24(APU_STREAM_PTR);
    uint8_t value = rd8((base + *offset) & 0xFFFFFF);
    uint16_t bank;

    *offset += 1;
    *low = rd16(APU_STREAM_PTR);
    if ((uint32_t)*low + *offset > 0xFFFF) {
        wr16(APU_STREAM_PTR, 0);
        *offset = 0x8000;
        bank = rd16(APU_STREAM_BANK);
        wr16(APU_STREAM_BANK, (uint16_t)(bank + 1));
    }
    return value;
}

uint8_t ApuUploadNextByte(uint16_t *offset)
{
    uint16_t low;

    cyc(46);
    return apu_upload_next_byte(offset, &low);
}

void abi_ApuUploadNextByte(void)
{
    /* The carry and overflow the original's 16-bit add leaves behind. */
    uint16_t low, next = (uint16_t)(cpu.y + 1);
    uint8_t value = apu_upload_next_byte(&cpu.y, &low);
    uint32_t sum = (uint32_t)low + next;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | value);
    cpu.p = nz16(cpu.p, cpu.a);
    cpu.p = (uint8_t)((cpu.p & ~(P_C | P_V)) | (sum > 0xFFFF ? P_C : 0)
                      | ((~(next ^ low) & (next ^ sum) & 0x8000) ? P_V : 0));
    cpu.p |= P_M;
}

/* Where an upload left the stream, and the last value it put on port 0. */
typedef struct {
    uint16_t offset;
    uint8_t kick;
} ApuUploadEnd;

static ApuUploadEnd apu_upload(uint8_t bank, uint16_t addr)
{
    ApuUploadEnd end;
    uint16_t offset = 0;
    uint8_t kick = 0xCC;

    wr16(APU_STREAM_PTR, addr);
    wr8(APU_STREAM_BANK, bank);
    while (rd16(APUIO0) != 0xBBAA) {
    }
    for (;;) {
        uint8_t lo = ApuUploadNextByte(&offset);
        uint8_t hi = ApuUploadNextByte(&offset);
        uint16_t length = (uint16_t)(lo | hi << 8);
        uint8_t more = length != 0;
        uint8_t index = 0;
        unsigned carry = 1;

        wr8(APUIO2, ApuUploadNextByte(&offset));
        wr8(APUIO3, ApuUploadNextByte(&offset));
        wr8(APUIO1, more);
        wr8(APUIO0, kick);
        while (rd8(APUIO0) != kick) {
        }
        if (!more)
            break;
        wr16(APUIO0, (uint16_t)(ApuUploadNextByte(&offset) << 8 | index));
        while (--length) {
            uint8_t data = ApuUploadNextByte(&offset);
            while (rd8(APUIO0) != index) {
            }
            index++;
            wr16(APUIO0, (uint16_t)(data << 8 | index));
        }
        while (rd8(APUIO0) != index) {
        }
        /* The next block's kick is past the last index, and never zero. */
        kick = index;
        do {
            unsigned sum = kick + 3 + carry;
            carry = sum > 0xFF;
            kick = (uint8_t)sum;
        } while (kick == 0);
    }
    end.offset = offset;
    end.kick = kick;
    return end;
}

/*
 * @routine 04:D720 ApuUpload
 * WHAT IT DOES: Uploads a block list from ROM to the SPC700's boot loader and
 * starts the SPC700 at the address the list ends with.
 * HOW: Points the upload stream at bank:address, waits for the loader's ready
 * signature $BBAA on ports 0-1, then for each block sends the SPC700 address
 * on ports 2-3, a flag on port 1 (1: data follows, 0: start here) and a kick
 * value on port 0, and waits for the loader to echo the kick. The block's
 * bytes go out on port 1 with a running index on port 0, each waiting for the
 * loader to echo the previous index. The first kick is $CC; each later one
 * is the last index plus 4 (plus 3 more while that wraps to zero).
 * Entry: M 16-bit, X 16-bit, D = $0000; A low byte = ROM bank, Y = address.
 * Exit: A = the final kick (high byte 0); X = 0; Y = the stream offset after
 * the list; N, V clear; Z, C set; M 8-bit.
 */
void ApuUpload(uint8_t bank, uint16_t addr)
{
    apu_upload(bank, addr);
}

void abi_ApuUpload(void)
{
    ApuUploadEnd end = apu_upload((uint8_t)cpu.a, cpu.y);

    cpu.a = end.kick;
    cpu.x = 0;
    cpu.y = end.offset;
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_V)) | P_Z | P_C | P_M);
}

/*
 * @routine 04:D43C ApuPort1Send05
 * WHAT IT DOES: Puts $05 on APU port 1, then waits while the sound CPU
 * responds (ApuSettleDelay).
 * HOW: An 8-bit store to port 1, then falls into ApuSettleDelay.
 * Entry: M 16-bit. Exit: as ApuSettleDelay.
 */
void ApuSettleDelay(void);

void ApuPort1Send05(void)
{
    cyc(146);
    wr8(APUIO1, 0x05);
    ApuSettleDelay();
}

void abi_ApuPort1Send05(void)
{
    ApuPort1Send05();
    cpu.a = 0;
    cpu.p = (uint8_t)((cpu.p & ~(P_M | P_N)) | P_Z);
}

/*
 * @routine 04:D443 ApuSettleDelay
 * WHAT IT DOES: Waits a fixed time for the sound CPU to act on a command.
 * HOW: Counts a 16-bit register down from $4000 to zero; it has no other
 * effect.
 * Entry: any widths. Exit: A = 0; M 16-bit; Z set, N clear.
 */
void ApuSettleDelay(void)
{
    cyc(589832);
    delay_turns(0x4000);
}

void abi_ApuSettleDelay(void)
{
    (cyc(177), ApuSettleDelay());
    cpu.a = 0;
    cpu.p = (uint8_t)((cpu.p & ~(P_M | P_N)) | P_Z);
}

/*
 * @routine 04:D44C ApuUploadBank19
 * WHAT IT DOES: Uploads the sound block list at ROM $19:C27C to the SPC700,
 * then waits for it to settle.
 * HOW: ApuUpload with bank $19 and address $C27C, then ApuSettleDelay.
 * Entry: any widths, D = $0000. Exit: A = 0, X = 0, Y = the stream offset
 * after the list; P unchanged.
 */
void ApuUploadBank19(void)
{
    ApuUpload(0x19, 0xC27C);
    ApuSettleDelay();
}

void abi_ApuUploadBank19(void)
{
    cyc(154);
    ApuUploadEnd end = apu_upload(0x19, 0xC27C);

    cpu.a = 0;
    cpu.x = 0;
    cpu.y = end.offset;
}
