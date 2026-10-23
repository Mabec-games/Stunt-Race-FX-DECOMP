/*
 * crash.c: the stop screen. Each entry stores a backdrop colour in $1A2E
 * and a code in X, then all of them share one body: interrupts and HDMA
 * off (stores through the caller's data bank, so only from banks $00-$3F
 * and $80-$BF), the screen blanked and rebuilt as BG1 alone with that
 * colour as colour 0, HDMA channel 1 feeding BG1HOFS from the table at
 * $00:812E; then BG1 is scrolled down step by step until its vertical
 * scroll ($0719) reaches the code ($0717), and the CPU stops in a loop
 * forever.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "cpu_ops.h"
#include "system_calls.h"
#include "carried.h"


#define CRASH_COLOUR 0x1A2Eu
#define CRASH_CODE 0x000717u
#define CRASH_SCROLL 0x000719u

/* The shared body ($00:805C). The data bank is the caller's until the PLB,
   then 0. Never returns. */
static void crash_body(uint8_t bank, uint16_t code)
{
    uint16_t a, step;

    cyc(70);
    wr16((uint32_t)bank << 16 | (uint16_t)CRASH_CODE, code);
    /* Through the caller's data bank: from bank $7E or $7F these four land
       in work RAM, and interrupts and HDMA stay on. */
    cyc(46);
    hw_wr((uint32_t)bank << 16 | (uint16_t)NMITIMEN, 0x00);
    cyc(24);
    hdmaen_wr((uint32_t)bank << 16 | (uint16_t)HDMAEN, 0x00);
    cpu.p |= P_I;
    cyc(38);
    ppu_wr((uint32_t)bank << 16 | (uint16_t)TM, 0x00);
    cyc(24);
    ppu_wr((uint32_t)bank << 16 | (uint16_t)TS, 0x00);
    cyc(150);
    ppu_wr(INIDISP, 0x80);
    cyc(24);
    ppu_wr(CGADD, 0x00);
    cyc(24);
    ppu_wr(CGDATA, cyc_then(24, rd8(CRASH_COLOUR)));
    cyc(24);
    ppu_wr(CGDATA, cyc_then(24, rd8(CRASH_COLOUR + 1)));
    cyc(40);
    ppu_wr(BG1SC, 0x40);
    cyc(40);
    ppu_wr(BG12NBA, 0x00);
    cyc(24);
    ppu_wr(W12SEL, 0x00);
    cyc(24);
    ppu_wr(W34SEL, 0x00);
    cyc(24);
    ppu_wr(WOBJSEL, 0x00);
    cyc(24);
    ppu_wr(WBGLOG, 0x00);
    cyc(24);
    ppu_wr(WOBJLOG, 0x00);
    cyc(24);
    ppu_wr(TMW, 0x00);
    cyc(24);
    ppu_wr(TSW, 0x00);
    cyc(24);
    ppu_wr(WH0, 0x00);
    cyc(24);
    ppu_wr(WH1, 0x00);
    cyc(40);
    ppu_wr(BGMODE, 0x01);
    cyc(40);
    ppu_wr(TM, 0x01);
    cyc(24);
    ppu_wr(TS, 0x00);
    cyc(24);
    ppu_wr(CGADSUB, 0x00);
    cyc(24);
    ppu_wr(COLDATA, 0x00);
    cyc(24);
    dma_wr(DMAP1, 0x00);
    cyc(40);
    dma_wr(BBAD1, 0x00);
    cyc(40);
    dma_wr(A1B1, 0x00);
    cyc(48);
    dma_wr16(A1T1L, 0x812E);
    cyc(24);
    dma_wr(DASB1, 0x00);
    cyc(24);
    ppu_wr(BG1VOFS, 0x00);
    cyc(24);
    ppu_wr(BG1VOFS, 0x00);
    cyc(40);
    ppu_wr(BG1HOFS, 0xF9);
    cyc(40);
    ppu_wr(BG1HOFS, 0xFF);
    cyc(40);
    ppu_wr(INIDISP, 0x0F);
    cyc(40);
    hdmaen_wr(HDMAEN, 0x02);
    for (;;) {
        cyc(92);
        delay_turns(20000);
        a = rd16(CRASH_CODE);
        cyc(24);
        if (a == rd16(CRASH_SCROLL)) {
            /* As the loop leaves them: 16-bit, IRQs masked, Z and C from
               the compare, X the HDMA table, Y the delay's end. */
            cpu.a = a;
            cpu.x = 0x812E;
            cpu.y = 0x0000;
            cpu.p = (uint8_t)((cpu.p & (P_V | P_D)) | P_I | P_Z | P_C);
            cpu_loop_forever(0x00812C);
        }
        cyc(54);
        step = (uint16_t)((uint16_t)(a - rd16(CRASH_SCROLL)) >> 3);
        if (!step)
            step = 0x0001;
        cyc(102);
        wr16(CRASH_SCROLL, cyc_then(24, (uint16_t)(step + rd16(CRASH_SCROLL))));
        cyc(46);
        ppu_wr(BG1VOFS, cyc_then(24, rd8(CRASH_SCROLL)));
        cyc(24);
        ppu_wr(BG1VOFS, cyc_then(24, rd8(CRASH_SCROLL + 1)));
        cyc(40);
        hw_wr(APUIO0, 0x03);
    }
}

/*
 * @routine 00:8014 CrashWhite
 * WHAT IT DOES: Stops the game on the stop screen with a white backdrop,
 * code $AA.
 * HOW: $1A2E = $7FFF, X = $AA, then the shared body at $00:805C (which
 * scrolls BG1 to the code between delays of 20000 loop counts, writes 3 to
 * APU port 0 at each step, and ends in a loop to itself).
 * Entry: any widths; D = $0000. Exit: never returns.
 */
void CrashWhite(uint8_t bank)
{
    cyc(70);
    wr16((uint32_t)bank << 16 | CRASH_COLOUR, 0x7FFF);
    crash_body(bank, 0x00AA);
}

void CrashGreen_cr(Carried *cr);
void CrashWhite_cr(Carried *cr);

void CrashWhite_cr(Carried *cr)
{
    CrashWhite(cr->db);
}

void abi_CrashWhite(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CrashWhite_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 00:8048 CrashGreen
 * WHAT IT DOES: Stops the game on the stop screen with a green backdrop,
 * code $E2: what ObjectSpawnFromScript does when no object is free. Not
 * reached in the traces so far.
 * HOW: As CrashWhite, with $1A2E = $03E0 and X = $E2.
 * Entry: any widths; D = $0000. Exit: never returns.
 */
void CrashGreen(uint8_t bank)
{
    cyc(70);
    wr16((uint32_t)bank << 16 | CRASH_COLOUR, 0x03E0);
    crash_body(bank, 0x00E2);
}

void CrashGreen_cr(Carried *cr)
{
    CrashGreen(cr->db);
}

void abi_CrashGreen(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CrashGreen_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

