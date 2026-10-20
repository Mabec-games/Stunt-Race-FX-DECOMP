/*
 * vblank_updates.c: the work the frame code does in the vertical blank
 * (run from work RAM bank $7F): the Super FX picture copied to VRAM in
 * stages, the window that opens and closes over the screen, the pending
 * DMA and register updates (frame_updates.c), a blinking colour and the
 * brightness step.
 */
#include "machine.h"
#include "hw.h"

uint8_t VramDmaWideRowsFromGsuRam(uint8_t rows_less_one, uint16_t src, uint16_t vram, uint16_t *next);
void CgramDmaIfPending(void);
void VramDmaQueueFlush(void);
void OamDmaIfPending(void);
void ByteWritesIfPending(uint8_t bank);
uint8_t HalfRateApproach(void);
void abi_CgramDmaIfPending(void);
void abi_VramDmaQueueFlush(void);
void abi_OamDmaIfPending(void);
void abi_ByteWritesIfPending(void);
void abi_HalfRateApproach(void);

static uint32_t dbw(uint8_t bank, uint16_t a)
{
    return (uint32_t)bank << 16 | a;
}

/* One stage of the upload: the next stage, then VramDmaWideRowsFromGsuRam
   (rows - 1, source, VRAM). Returns A as that routine leaves it: the
   source's high byte after the last row, and the screen mode value. */
static uint16_t upload_rows(uint8_t bank, uint16_t next, uint8_t rows_less_one, uint16_t src, uint16_t vram)
{
    uint8_t mode;
    uint16_t end;

    wr16(dbw(bank, 0x0D7D), cyc_then(116, next));
    mode = VramDmaWideRowsFromGsuRam(rows_less_one, src, vram, &end);
    return (uint16_t)((end & 0xFF00) | mode);
}

/* The last stage: $0357 = 1, BG1's tilemap at `sc`, and bit 5 of $11D7
   (the picture is complete). Returns A's low byte, the new flags. */
static uint8_t upload_done(uint8_t bank, uint8_t sc)
{
    uint8_t f;

    wr8(dbw(bank, 0x0357), cyc_then(62, 0x01));
    wr8(dbw(bank, BG1SC), cyc_then(40, sc));
    f = (uint8_t)(rd8(cyc_then(24, dbw(bank, 0x11D7))) | 0x20);
    wr8(dbw(bank, 0x11D7), cyc_then(40, f));
    return f;
}

/*
 * @routine 7F:0800 GsuPictureUploadStep
 * WHAT IT DOES: Copies the Super FX's finished picture to VRAM in stages,
 * one a frame, the stage being $0D7D: stage 0 copies 4 rows from $70:2C00
 * to VRAM 0, stage 2 4 rows from $3C00 to $0580, stage 4 5 rows from
 * $4C00 to $0B00 and completes the picture with BG1's tilemap at $6000;
 * stage 6 does nothing; stage 8 copies 4 rows from $5000 to $1760, stage
 * $0A 4 rows from $4000 to $11E0, and stage $0C 5 rows from $2C00 to $0B00
 * and completes it with the tilemap at $6400. Completing sets $0357 = 1
 * and bit 5 of $11D7; each stage sets the next (2, 4, 6, -, $0A, $0C, 6).
 * HOW: A jump table at $7F:0809 on $0D7D; VramDmaWideRowsFromGsuRam for
 * the rows. P is saved and restored.
 * Entry: any widths, D = $0000. Exit: stage 6: A, Y unchanged; X = 6.
 * Otherwise as VramDmaWideRowsFromGsuRam (X = $FF, Y = 1), with A's low
 * byte = $11D7 after a completing stage. P unchanged.
 */
uint16_t GsuPictureUploadStep(uint8_t bank, int *copied)
{
    uint16_t stage = rd16(cyc_then(68, dbw(bank, 0x0D7D))), a = 0;

    *copied = 1;
    switch (stage) {
    case 0x0:
        return cyc_then(70, upload_rows(bank, 0x0002, 3, 0x2C00, 0x0000));
    case 0x2:
        return cyc_then(70, upload_rows(bank, 0x0004, 3, 0x3C00, 0x0580));
    case 0x4:
        a = upload_rows(bank, 0x0006, 4, 0x4C00, 0x0B00);
        return cyc_then(70, (uint16_t)((a & 0xFF00) | upload_done(bank, 0x60)));
    case 0x8:
        return cyc_then(70, upload_rows(bank, 0x000A, 3, 0x5000, 0x1760));
    case 0xA:
        return cyc_then(70, upload_rows(bank, 0x000C, 3, 0x4000, 0x11E0));
    case 0xC:
        a = upload_rows(bank, 0x0006, 4, 0x2C00, 0x0B00);
        return cyc_then(70, (uint16_t)((a & 0xFF00) | upload_done(bank, 0x64)));
    }
    cyc(138);
    *copied = 0;
    return a;
}

void abi_GsuPictureUploadStep(void)
{
    int copied;
    uint16_t a = GsuPictureUploadStep(cpu.db, &copied);

    if (!copied) {
        cpu.x = 0x0006;
        return;
    } else { cyc(0); }
    cpu.a = a;
    cpu.x = 0x00FF;
    cpu.y = 0x0001;
}

#define WINDOW_MODE 0x001905u    /* 0 idle, 1 opening, 2 closing, 3 set */
#define WINDOW_SIZE 0x001907u    /* 0-$2C, in steps of 4 */

/* The window HDMA table's entries for size s: line counts at $7F:134A
   and $7F:134E ($2C - s), and at $7F:134C/$7F:134D either s * 2 and 0 or,
   when s * 2 is a multiple of 256, 2 and 3 with $7F:134E two lines
   shorter. */
static void window_table(void)
{
    uint16_t s = rd16(WINDOW_SIZE);
    uint8_t c = (uint8_t)(0x2C - s), d;

    wr8(0x7F134A, c);
    wr8(0x7F134E, c);
    s = rd16(WINDOW_SIZE);
    d = (uint8_t)(s << 1);
    if (d) {
        wr8(0x7F134C, d);
        wr8(0x7F134D, 0x00);
        return;
    }
    wr8(0x7F134C, 0x02);
    wr8(0x7F134D, 0x03);
    wr8(0x7F134E, (uint8_t)(rd8(0x7F134E) - 2));
}

/*
 * @routine 7F:0DAC WindowOpenCloseStep
 * WHAT IT DOES: Steps the window over the screen ($1905 its mode, $1907 its
 * size): opening (1) shrinks the size by 4 a frame until it is negative,
 * closing (2) grows it by 4 until it is $2C, then clears colour 0 (in
 * the palette copy $7E:FD65 and CGRAM); either then sets the mode to 0.
 * Each step rewrites the window HDMA table at $7F:134A from the size
 * before the step. Mode 3 (not reached in the traces so far) sets the line
 * counts at $7F:12DB, $12E8, $12F6 and $1301 to $1921 + 1, and $191B = $16
 * - $1921 + $1925, $191D = $45 - $1921 + $1927.
 * HOW: 16-bit arithmetic with 8-bit stores into the table; CGRAM through
 * the data bank.
 * Entry: any widths. Exit: X, Y unchanged; mode 0: A = 0, Z set; mode 1:
 * A = the new size, or 0 once negative (then Z set), N, Z, C, V from the
 * subtraction; mode 2: A = the new size and P from its compare with $2C,
 * or A = 0 with Z set; M 16-bit, X 16-bit. Mode 3: A = $191D; N, Z, C, V
 * from its add; M 8-bit, X 16-bit.
 */
uint16_t WindowOpenCloseStep(uint8_t bank, uint8_t *p)
{
    cyc(54);
    uint16_t mode = rd16(WINDOW_MODE), s, t;
    unsigned r;

    *p &= (uint8_t)~(P_M | P_X);
    if (!mode) {
        cyc(24);
        *p = nz16(*p, mode);
        return mode;
    }
    if (mode == 0x0003) {
        cyc(146);
        uint8_t n = (uint8_t)(rd8(0x001921) + 1);

        cyc(46);
        wr8(0x7F12DB, n);
        cyc(32);
        wr8(0x7F12E8, n);
        cyc(32);
        wr8(0x7F12F6, n);
        cyc(32);
        wr8(0x7F1301, n);
        cyc(92);
        t = (uint16_t)(0x0016 - rd16(0x001921));
        cyc(46);
        t = (uint16_t)(t + rd16(0x001925));
        cyc(32);
        wr16(0x00191B, t);
        cyc(70);
        s = (uint16_t)(0x0045 - rd16(0x001921));
        cyc(46);
        t = rd16(0x001927);
        r = (unsigned)s + t;
        *p = (uint8_t)((nz16(*p, (uint16_t)r) & ~(P_C | P_V)) | (r > 0xFFFF ? P_C : 0)
                       | ((~(s ^ t) & (s ^ (uint16_t)r) & 0x8000) ? P_V : 0));
        cyc(32);
        wr16(0x00191D, (uint16_t)r);
        *p |= P_M;
        return (uint16_t)r;
    }
    if (mode != 0x0002) {
        window_table();
        s = rd16(WINDOW_SIZE);
        t = (uint16_t)(s - 4);
        *p = (uint8_t)((nz16(*p, t) & ~(P_C | P_V)) | (s >= 4 ? P_C : 0)
                       | (((s ^ 4) & (s ^ t) & 0x8000) ? P_V : 0));
        wr16(WINDOW_SIZE, t);
        if (!(t & 0x8000))
            return t;
        t = 0x0000;
        *p = nz16(*p, t);
        wr16(WINDOW_MODE, t);
        return t;
    }
    window_table();
    s = rd16(WINDOW_SIZE);
    t = (uint16_t)(s + 4);
    *p = (uint8_t)((*p & ~P_V) | ((~(s ^ 4) & (s ^ t) & 0x8000) ? P_V : 0));
    wr16(WINDOW_SIZE, t);
    *p = (uint8_t)((nz16(*p, (uint16_t)(t - 0x2C)) & ~P_C) | (t >= 0x2C ? P_C : 0));
    if (t != 0x002C)
        return t;
    t = 0x0000;
    wr16(WINDOW_MODE, t);
    wr16(0x7EFD65, t);
    wr8(dbw(bank, CGADD), 0x00);
    wr8(dbw(bank, CGDATA), 0x00);
    wr8(dbw(bank, CGDATA), 0x00);
    *p = nz16(*p, t);
    return t;
}

void abi_WindowOpenCloseStep(void)
{
    uint8_t p = cpu.p;

    cpu.a = WindowOpenCloseStep(cpu.db, &p);
    cpu.p = p;
    cyc(64);
}

void abi_WindowOpenCloseStep(void);

/* The two colours the blinking colour $D9 alternates between. */
#define BLINK_COLOURS 0x7F0DA8u

/* Every fifth call toggles $18FF; then CGRAM colour $D9 gets entry $18FF
   of the two colours. Returns X as the code leaves it ($18FF * 2), and the
   last byte written in *last. */
static uint16_t blink_colour(uint8_t bank, uint8_t *last)
{
    cyc(48);
    uint16_t c = (uint16_t)(rd16(0x001901) + 1), x;

    cyc(46);
    wr16(0x001901, c);
    if (c == 0x0005) {
        cyc(96);
        wr16(0x001901, 0x0000);
        cyc(32);
        wr16(0x0018FF, cyc_then(70, (uint16_t)((rd16(0x0018FF) + 1) & 0x0001)));
    } else { cyc(46); }
    cyc(32);
    x = (uint16_t)(rd16(0x0018FF) << 1);
    wr8(dbw(bank, CGADD), cyc_then(90, 0xD9));
    wr8(dbw(bank, CGDATA), cyc_then(24, rd8(cyc_then(32, BLINK_COLOURS + x))));
    cyc(32);
    *last = rd8(BLINK_COLOURS + 1 + x);
    wr8(dbw(bank, CGDATA), cyc_then(24, *last));
    return x;
}

/* The end of the vertical blank work: clears bit 5 of $11D7 and waits for
   the joypad auto-read to finish. */
static uint8_t vblank_end(uint8_t bank)
{
    uint8_t v;

    wr8(dbw(bank, 0x11D7), cyc_then(40, (uint8_t)(rd8(cyc_then(46, dbw(bank, 0x11D7))) & 0xDF)));
    do
        v = (uint8_t)(rd8(cyc_then(24, dbw(bank, HVBJOY))) & 0x01);
    while (cyc_loop(38, v));
    cyc(104);
    return v;
}

/*
 * @routine 7F:0D2B VblankUpdates
 * WHAT IT DOES: The vertical blank work of the main loop: clears $11D8,
 * copies the next stage of the Super FX picture (GsuPictureUploadStep),
 * steps the screen window (WindowOpenCloseStep); unless a picture was just
 * completed (bit 5 of $11D7) applies the pending palette, VRAM and sprite
 * transfers (CgramDmaIfPending, VramDmaQueueFlush, OamDmaIfPending); then
 * the pending byte writes (ByteWritesIfPending); when $18B9 is set blinks
 * colour $D9 between the two colours at $7F:0DA8 (toggling every fifth
 * frame); steps the brightness (HalfRateApproach); clears bit 5 of $11D7
 * and waits for the joypad auto-read to end.
 * HOW: Work RAM and registers through the data bank, $1901/$18FF/$18B9
 * with long addressing. P is saved and restored.
 * Entry: any widths, D = $0000. Exit: A low byte = 0, high byte from the
 * last step; X, Y as the last routine called left them (X = $18FF * 2
 * after the blink); P unchanged.
 */
void VblankUpdates(uint8_t bank)
{
    int copied;
    uint8_t p = 0;

    wr16(dbw(bank, 0x11D8), 0x0000);
    GsuPictureUploadStep(bank, &copied);
    WindowOpenCloseStep(bank, &p);
    if (!(rd8(dbw(bank, 0x11D7)) & 0x20)) {
        CgramDmaIfPending();
        VramDmaQueueFlush();
        OamDmaIfPending();
    }
    ByteWritesIfPending(bank);
    if (rd16(0x0018B9)) {
        uint8_t last;

        blink_colour(bank, &last);
    }
    HalfRateApproach();
    vblank_end(bank);
}

void abi_VblankUpdates(void)
{
    uint8_t p0 = cpu.p;
    uint16_t v;

    cpu.p &= (uint8_t)~(P_M | P_X);
    wr16(dbw(cpu.db, 0x11D8), cyc_then(68, 0x0000));
    (cyc(46), abi_GsuPictureUploadStep());
    (cyc(46), abi_WindowOpenCloseStep());
    cpu.p |= P_M;
    v = rd8(cyc_then(46, dbw(cpu.db, 0x11D7))) & 0x20;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, (uint8_t)v);
    if (!v) {
        cyc(78);
        abi_CgramDmaIfPending();
        (cyc(46), abi_VramDmaQueueFlush());
        (cyc(46), abi_OamDmaIfPending());
    } else { cyc(38); }
    cyc(46);
    abi_ByteWritesIfPending();
    cpu.p &= (uint8_t)~P_M;
    cyc(54);
    v = rd16(0x0018B9);
    cpu.a = v;
    cpu.p = nz16(cpu.p, v);
    if (v) {
        uint8_t last;

        cpu.x = blink_colour(cpu.db, &last);
        cpu.a = (uint16_t)((cpu.x & 0xFF00) | last);
    } else { cyc(22); }
    cyc(68);
    cpu.p &= (uint8_t)~P_M;
    abi_HalfRateApproach();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | vblank_end(cpu.db));
    cpu.p = p0;
}
