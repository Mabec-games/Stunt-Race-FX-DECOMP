/*
 * programs.c: S-CPU wrappers that set a Super FX program's inputs in Super
 * FX RAM, run it with GsuRun, and collect what it leaves there.
 *
 * Program $01:CF42 draws into a buffer whose address it takes at $70:002C
 * (with inputs at $70:0068, $70:006A and a zero at $70:00A2); $01:80BC and
 * $01:82CE take a vector and return results; $01:8325 fills nine words at
 * $70:00E4; $07:EE88 reads an eight-word ramp at $70:3C00.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "cpu_ops.h"
#include "gsu_calls.h"
#include "audio_calls.h"
#include "gsu_programs_calls.h"
#include "carried.h"
#include "wram_names.h"
#include "wram_struct.h"


uint8_t GsuRun(uint8_t bank, uint16_t start);
void abi_GsuRun(void);
void abi_GsuBytes200_BFB3BE(void);
void abi_GsuBytes200_BFBCBD(void);
void abi_GsuBytes200_BFB7B6(void);
uint16_t GsuParam216B(uint8_t bank, uint16_t index);
void GsuBytes200_0F050E(void);
void GsuBytes200_0F080E(void);

#define GSU_IN_68 0x700068u
#define GSU_IN_6A 0x70006Au
#define GSU_DEST 0x70002Cu
#define GSU_IN_A2 0x7000A2u

static void cf42_inputs(uint16_t in68, uint16_t dest)
{
    cyc(32);
    wr16(GSU_IN_68, in68);
    cyc(dest == 0x3000 ? 56 : 78);      /* LDA #, STA long: $C000 and $4000 BRA there */
    wr16(GSU_DEST, dest);
    cyc(56);
    wr16(GSU_IN_A2, 0x0000);
}

static uint8_t cf42(uint16_t in68, uint16_t dest, uint16_t in6a)
{
    cf42_inputs(in68, dest);
    cyc(46);
    wr16(GSU_IN_6A, in6a);
    cyc(124);
    return GsuRun(0x01, 0xCF42);
}

void BigSpritesCountDraw_cr(Carried *cr);
void BigSpritesNine_cr(Carried *cr);
void BigSpritesRow5_cr(Carried *cr);
void BigSpritesRow6_cr(Carried *cr);
void BigSpritesRow6B_cr(Carried *cr);
void GlyphAtY60_cr(Carried *cr);
void GlyphGrid1A_cr(Carried *cr);
void GlyphGrid1D_cr(Carried *cr);
void GlyphListDraw_cr(Carried *cr);
void GlyphListMessageAndTime_cr(Carried *cr);
void GlyphRowY60From0_cr(Carried *cr);
void GlyphRowY60From1_cr(Carried *cr);
void GlyphRowY60From2_cr(Carried *cr);
void GlyphRowY60From3_cr(Carried *cr);
void GlyphRowY60From4_cr(Carried *cr);
void GlyphRowY60From5_cr(Carried *cr);
void GlyphRowsAt32_cr(Carried *cr);
void GlyphRowsAt56_cr(Carried *cr);
void GlyphSlideThree_cr(Carried *cr);
void GlyphSpriteDraw_cr(Carried *cr);
void GlyphSpriteDrawLeft8_cr(Carried *cr);
void GlyphSpritesDrawAll_cr(Carried *cr);
void GlyphSpritesGrowAndTimes_cr(Carried *cr);
void GlyphSpritesInit_cr(Carried *cr);
void GlyphSpritesSlideIn_cr(Carried *cr);
void GlyphSpritesSlideOut_cr(Carried *cr);
void GlyphTimesSequence_cr(Carried *cr);
void GlyphWordAt1C28_cr(Carried *cr);
void Gsu81E3_cr(Carried *cr);
void Gsu81E3Long_cr(Carried *cr);
void Gsu82CE_cr(Carried *cr);
void Gsu8325_cr(Carried *cr);
void GsuCF42To3000_cr(Carried *cr);
void GsuCF42To4000_cr(Carried *cr);
void GsuCF42ToC000_cr(Carried *cr);
void GsuCF42ToVram_cr(Carried *cr);
void GsuClear6000IfSet_cr(Carried *cr);
void GsuClearScreenIfSet_cr(Carried *cr);
void GsuDigitPair0E74_cr(Carried *cr);
void GsuEE88Ramp_cr(Carried *cr);
void GsuEEF5OrEFCD_cr(Carried *cr);
void GsuF0A5OrF0B4_cr(Carried *cr);
void GsuF15BFive_cr(Carried *cr);
void GsuF15BFourA_cr(Carried *cr);
void GsuF15BFourB_cr(Carried *cr);
void GsuF15BThree_cr(Carried *cr);
void GsuF15BTwoLines_cr(Carried *cr);
void GsuF329Entry_cr(Carried *cr);
void GsuF329Sequence_cr(Carried *cr);
void GsuF441Player1_cr(Carried *cr);
void GsuF441Player2_cr(Carried *cr);
void GsuFA1BFromTables_cr(Carried *cr);
void GsuFA35FromTables_cr(Carried *cr);
void GsuFA5AFromTables_cr(Carried *cr);
void GsuFA5ASix_cr(Carried *cr);
void GsuFA5ASixLeft_cr(Carried *cr);
void GsuFA5AThree_cr(Carried *cr);
void GsuFA5AThreeShifted_cr(Carried *cr);
void GsuPieceOffsetFromView_cr(Carried *cr);
void GsuPieceScaleFromView_cr(Carried *cr);
void GsuPiecesFromBytes_cr(Carried *cr);
void GsuPiecesFromRecord_cr(Carried *cr);
void GsuPiecesListF6DF_cr(Carried *cr);
void GsuPiecesListFAF2_cr(Carried *cr);
void GsuPiecesStep10CB_cr(Carried *cr);
void GsuReciprocal_cr(Carried *cr);
void HudBlankDraw_cr(Carried *cr);
void HudHoldDraw_cr(Carried *cr);
void HudLapDigitsDraw_cr(Carried *cr);
void HudLeadWordDraw_cr(Carried *cr);
void HudSixGlyphs_cr(Carried *cr);
void HudTimesDraw_cr(Carried *cr);
void HudTimesDrawEven_cr(Carried *cr);
void HudTimesPackedDraw_cr(Carried *cr);
void HudTimesPackedEven_cr(Carried *cr);
void ObjectGsu80BC_cr(Carried *cr);
void ObjectGsu80BCThen81E3_cr(Carried *cr);
void ObjectGsu81E3_cr(Carried *cr);
void TimeDigitsStore_cr(Carried *cr);
void ViewDefaults_cr(Carried *cr);
void ViewMatrixUpdateAndSave_cr(Carried *cr);
void ViewReset_cr(Carried *cr);
void ViewSetup_cr(Carried *cr);
void ViewUpdateAndGsuAEF8_cr(Carried *cr);
void GlyphSpritesGrowAndTimes(Carried *cr);
void GlyphTimesSequence(Carried *cr, uint8_t bank);
static void abi_cf42(Carried *cr, uint16_t dest);
static void glyph_times_exit(Carried *cr);
void ChampionshipFinish(Carried *cr);

static void abi_cf42(Carried *cr, uint16_t dest)
{
    uint16_t x = cr->x;
    uint8_t mode = cf42(cr->a, dest, x);

    cr->a = (uint16_t)((x & 0xFF00) | mode);
    cr->x = 0xCF42;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) | P_M));
}

/*
 * @routine 03:EBEF GsuCF42ToC000
 * WHAT IT DOES: Runs Super FX program $01:CF42 drawing to $C000, with inputs
 * A and X.
 * HOW: $70:0068 = A, $70:002C = $C000, $70:00A2 = 0, $70:006A = X, then
 * GsuRun.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = X's high byte and the
 * screen mode value; X = $CF42; M 8-bit; N, Z from the data bank.
 */
uint8_t GsuCF42ToC000(uint16_t in68, uint16_t in6a)
{
    return cf42(in68, 0xC000, in6a);
}

void GsuCF42ToC000_cr(Carried *cr)
{
    abi_cf42(cr, 0xC000);
}

void abi_GsuCF42ToC000(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuCF42ToC000_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:EBF8 GsuCF42To4000
 * WHAT IT DOES: As GsuCF42ToC000, drawing to $4000.
 * HOW: As GsuCF42ToC000.
 * Entry: as GsuCF42ToC000. Exit: as GsuCF42ToC000.
 */
uint8_t GsuCF42To4000(uint16_t in68, uint16_t in6a)
{
    return cf42(in68, 0x4000, in6a);
}

void GsuCF42To4000_cr(Carried *cr)
{
    abi_cf42(cr, 0x4000);
    cyc(44);
}

void abi_GsuCF42To4000(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuCF42To4000_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:EC01 GsuCF42To3000
 * WHAT IT DOES: As GsuCF42ToC000, drawing to $3000.
 * HOW: As GsuCF42ToC000.
 * Entry: as GsuCF42ToC000. Exit: as GsuCF42ToC000.
 */
uint8_t GsuCF42To3000(uint16_t in68, uint16_t in6a)
{
    cyc(0);
    return cyc_then(98, cf42(in68, 0x3000, in6a));
}

void GsuCF42To3000_cr(Carried *cr)
{
    abi_cf42(cr, 0x3000);
}

void abi_GsuCF42To3000(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuCF42To3000_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:EB83 GsuCF42ToVram
 * WHAT IT DOES: Runs program $01:CF42 drawing to $70:3000 and copies the
 * result to VRAM by DMA.
 * HOW: The inputs come from the record at X in the data bank (word at +0 to
 * $70:0068, byte at +2 to $70:006A); after GsuRun, the transfer comes from
 * the record at Y: VMAIN, the VRAM address, DMA mode and B-bus register, and
 * the size, with the source fixed at $70:3000.
 * Entry: M 16-bit, X 16-bit; X = input record, Y = transfer record. Exit: A =
 * 1; X = Y; M 8-bit, X 16-bit; N, Z clear.
 */
void GsuCF42ToVram(uint8_t bank, uint16_t in, uint16_t out)
{
    uint32_t b = (uint32_t)bank << 16;

    cyc(76);
    cf42_inputs(rd16(b | in), 0x3000);
    cyc(92);
    wr8(GSU_IN_6A, rd8(b | (uint16_t)(in + 2)));
    cyc(102);
    GsuRun(0x01, 0xCF42);
    cyc(214);
    ppu_wr(VMAIN, rd8(b | out));
    cyc(62);
    ppu_wr(VMADDL, rd8(b | (uint16_t)(out + 1)));
    cyc(62);
    ppu_wr(VMADDH, rd8(b | (uint16_t)(out + 2)));
    cyc(62);
    dma_wr(DMAP0, rd8(b | (uint16_t)(out + 3)));
    cyc(62);
    dma_wr(BBAD0, rd8(b | (uint16_t)(out + 4)));
    cyc(40);
    dma_wr(A1T0L, 0x00);
    cyc(40);
    dma_wr(A1T0H, 0x30);
    cyc(40);
    dma_wr(A1B0, 0x70);
    cyc(62);
    dma_wr(DAS0L, rd8(b | (uint16_t)(out + 5)));
    cyc(62);
    dma_wr(DAS0H, rd8(b | (uint16_t)(out + 6)));
    cyc(40);
    mdmaen_wr(MDMAEN, 0x01);
    cyc(44);
}

void GsuCF42ToVram_cr(Carried *cr)
{
    GsuCF42ToVram(cr->db, cr->x, cr->y);
    cr->a = 0x0001;
    cr->x = cr->y;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_X)) | P_M));
}

void abi_GsuCF42ToVram(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuCF42ToVram_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:C9EE ObjectGsu80BC
 * WHAT IT DOES: Runs Super FX program $01:80BC on object X's offset from a
 * point and returns its result.
 * HOW: $70:0068 = direct page $04 - the object's +$0C; $70:002E = $05F2 -
 * its +$10 (both through the data bank); GsuRun; the result is $70:006A.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = object. Exit: A = the result;
 * X unchanged; M 16-bit, X 16-bit; N, Z from A; C, V from the second
 * subtract.
 */
static uint16_t sub16(uint16_t a, uint16_t b, uint8_t *p)
{
    uint16_t r = (uint16_t)(a - b);

    *p = (uint8_t)((*p & ~(P_C | P_V)) | (a >= b ? P_C : 0) | (((a ^ b) & (a ^ r) & 0x8000) ? P_V : 0));
    return r;
}

uint16_t ObjectGsu80BC(uint8_t bank, uint16_t obj, uint8_t *p)
{
    (void)bank;
    uint16_t m, s;

    cyc(16);
    m = rd16(W(dp_04));
    cyc(44);
    s = RD16I(obj_pos_x, obj);
    wr16(GSU_IN_68, cyc_then(32, sub16(m, s, p)));
    cyc(24);
    m = RD16(wram_05F2);
    cyc(44);
    s = RD16I(obj_pos_z, obj);
    wr16(0x70002E, cyc_then(32, sub16(m, s, p)));
    cyc(154);
    GsuRun(0x01, 0x80BC);
    cyc(184);
    return cyc_then(44, rd16(GSU_IN_6A));
}

void ObjectGsu80BC_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ObjectGsu80BC(cr->db, cr->x, &p);
    set_p(cr, (uint8_t)(nz16(p, cr->a) & ~(P_M | P_X)));
}

void abi_ObjectGsu80BC(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectGsu80BC_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:AFD7 Gsu82CE
 * WHAT IT DOES: Runs Super FX program $01:82CE on the vector in direct page
 * $02, $08, $94 and puts its three results in direct page $B7, $B9, $BB.
 * HOW: Inputs to $70:0068, $70:002C, $70:002E; GsuRun; results from
 * $70:0026, $70:0028, $70:002A.
 * Entry: any widths, D = $0000. Exit: A = the third result; X, Y, P
 * unchanged.
 */
uint16_t Gsu82CE(void)
{
    uint16_t r = 0;
    unsigned i;

    cyc(104);
    wr16(GSU_IN_68, cyc_then(32, rd16(W(dp_02))));
    cyc(16);
    wr16(0x70002C, cyc_then(32, rd16(W(dp_08))));
    cyc(16);
    wr16(0x70002E, cyc_then(32, rd16(W(dp_94))));
    cyc(124);
    GsuRun(0x01, 0x82CE);
    cyc(94);
    for (i = 0; i < 3; i++) {
        cyc(32);
        r = rd16(0x700026 + 2 * i);
        cyc(16);
        wr16(W(dp_b7) + 2 * i, r);
    }
    cyc(128);
    return r;
}

void Gsu82CE_cr(Carried *cr)
{
    cr->a = Gsu82CE();
}

void abi_Gsu82CE(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Gsu82CE_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:B011 Gsu8325
 * WHAT IT DOES: Runs Super FX program $01:8325 on the three words at $065E
 * and copies the nine words it leaves at $70:00E4 to $0608.
 * HOW: Inputs to $70:0020-$70:0024; GsuRun; nine word copies. Work RAM is
 * reached through the data bank.
 * Entry: X 16-bit, D = $0000. Exit: A = the last word; X = $25, Y high byte
 * clear; M 8-bit, X 8-bit; N, Z from A (16-bit).
 */
uint16_t Gsu8325(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t v = 0;
    unsigned i;

    cyc(22);
    for (i = 0; i < 3; i++)
        { cyc(24); wr16(0x700020 + 2 * i, cyc_then(32, rd16(b | (0x065E + 2 * i)))); }
    cyc(146);
    GsuRun(0x01, 0x8325);
    cyc(94);
    for (i = 0; i < 9; i++) {
        cyc(32);
        v = rd16(0x7000E4 + 2 * i);
        cyc(24);
        wr16(b | (0x0608 + 2 * i), v);
    }
    cyc(66);
    return v;
}

void Gsu8325_cr(Carried *cr)
{
    cr->a = Gsu8325(cr->db);
    cr->x = 0x0025;
    cr->y &= 0x00FF;
    set_p(cr, (uint8_t)(nz16(cr->p, cr->a) | P_M | P_X));
}

void abi_Gsu8325(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Gsu8325_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:FE8D GsuEE88Ramp
 * WHAT IT DOES: Writes the ramp $80, $90, ... $F0 to $70:3C00 and runs Super
 * FX program $07:EE88.
 * HOW: Eight word stores, then GsuRun.
 * Entry: any widths. Exit: A = the screen mode value (high byte clear); X =
 * $88, Y high byte clear; M 16-bit, X 8-bit; N, Z from the data bank.
 */
uint8_t GsuEE88Ramp(void)
{
    unsigned i;

    cyc(22);
    for (i = 0; i < 8; i++)
        { cyc(56); wr16(0x703C00 + 2 * i, (uint16_t)(0x80 + 0x10 * i)); }
    cyc(146);
    return GsuRun(0x07, 0xEE88);
}

void GsuEE88Ramp_cr(Carried *cr)
{
    cr->a = GsuEE88Ramp();
    cr->x = 0x0088;
    cr->y &= 0x00FF;
    set_p(cr, (uint8_t)((nz8(cr->p, cr->db) & ~P_M) | P_X));
    cyc(160);                           /* after GsuRun: its return */
}

void abi_GsuEE88Ramp(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuEE88Ramp_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 07:8B9B GsuF329Entry
 * WHAT IT DOES: Runs Super FX program $07:F329 with the renderer input at
 * $70:0216 set to entry A (low four bits) of the table at $8E52.
 * HOW: GsuParam216B, then GsuRun.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = entry. Exit: A = the table
 * word's high byte and the screen mode value; X = $F329; C clear; N, Z
 * from the data bank.
 */
uint8_t GsuF329Entry(uint8_t bank, uint16_t entry)
{
    cyc(70);
    GsuParam216B(bank, (uint16_t)((entry & 0x000F) << 1));
    cyc(188);
    return cyc_then(86, GsuRun(0x07, 0xF329));
}

void GsuF329Entry_cr(Carried *cr)
{
    uint16_t index = (uint16_t)((cr->a & 0x000F) << 1);
    uint16_t hi = (uint16_t)(rd16((uint32_t)cr->db << 16 | (uint16_t)(0x8E52 + index)) & 0xFF00);
    uint8_t mode = GsuF329Entry(cr->db, cr->a);

    cr->a = (uint16_t)(hi | mode);
    cr->x = 0xF329;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~P_C));
}

void abi_GsuF329Entry(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuF329Entry_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Program $07:F441 for a player: its kind decides the bytes at $70:0200
 * and the word at $70:0216; $70:020A = $74 and $70:020C = $3C. */
static uint8_t f441(uint8_t kind)
{
    if ((kind & 0x0F) == 1) {
        cyc(140);
        GsuBytes200_0F050E();
        cyc(56);
        wr16(0x700216, 0x080E);
        cyc(22);                        /* BRA */
    } else {
        cyc(156);
        GsuBytes200_0F080E();
        cyc(56);
        wr16(0x700216, 0x0A0E);
    }
    cyc(56);
    wr16(0x70020A, 0x0074);
    cyc(56);
    wr16(0x70020C, 0x003C);
    cyc(146);
    return GsuRun(0x07, 0xF441);
}

/*
 * @routine 07:A684 GsuF441Player1
 * WHAT IT DOES: Runs Super FX program $07:F441 for player 1, with the
 * inputs chosen by the low four bits of $0DF4 (1, or anything else).
 * HOW: Kind 1: GsuBytes200_0F050E and $70:0216 = $080E; otherwise
 * GsuBytes200_0F080E and $0A0E. Then $70:020A = $74, $70:020C = $3C and
 * GsuRun.
 * Entry: any widths, D = $0000. Exit: A = the screen mode value (high byte
 * clear); X = $F441 ($41 when X is 8-bit); Y high byte clear when X is
 * 8-bit; P unchanged.
 */
uint8_t GsuF441Player1(uint8_t bank)
{
    (void)bank;
    cyc(68);
    return cyc_then(114, f441(RD8(wram_0DF4)));
}

void GsuF441Player1_cr(Carried *cr)
{
    cr->a = GsuF441Player1(cr->db);
    if (cr->p & P_X)
        cr->y &= 0x00FF;
    cr->x = cr->p & P_X ? 0x0041 : 0xF441;  /* the final pull restores X's width */
}

void abi_GsuF441Player1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuF441Player1_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:A694 GsuF441Player2
 * WHAT IT DOES: As GsuF441Player1, for player 2 ($0DF5).
 * HOW: The same tail as GsuF441Player1.
 * Entry: any widths, D = $0000. Exit: A = the screen mode value (high byte
 * clear); X = $F441 ($41 when X is 8-bit); Y high byte clear when X is
 * 8-bit; P unchanged.
 */
uint8_t GsuF441Player2(uint8_t bank)
{
    (void)bank;
    cyc(68);
    return f441(RD8(wram_0DF5));
}

void GsuF441Player2_cr(Carried *cr)
{
    cr->a = GsuF441Player2(cr->db);
    if (cr->p & P_X)
        { cyc(114); cr->y &= 0x00FF; }
    cr->x = cr->p & P_X ? 0x0041 : 0xF441;  /* the final pull restores X's width */
}

void abi_GsuF441Player2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuF441Player2_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:AF05 ViewReset
 * WHAT IT DOES: Puts the view at the origin facing straight ahead: clears
 * the position at $0664-$0669 and copies it to the Super FX inputs at
 * $065E; runs program $01:8325 (Gsu8325) and keeps its nine words at
 * $064C-$065D; sets the nine words at $063A to the identity matrix ($7F00
 * on the diagonal).
 * HOW: Work RAM through the data bank; the matrix is written a byte at a
 * time (high bytes of the diagonal, then the low bytes).
 * Entry: any widths. Exit: A = $7F with the high byte of the last result
 * word; X = $25; Y high byte clear; P unchanged.
 */
uint16_t ViewReset(uint8_t bank)
{
    static const uint16_t high[9] = {0x063B, 0x063D, 0x063F, 0x0641, 0x0643, 0x0645, 0x0647, 0x0649, 0x064B};
    uint32_t b = (uint32_t)bank << 16;
    uint16_t v = 0;
    unsigned i;

    cyc(106);
    for (i = 0; i < 3; i++)
        { cyc(24); wr16(b | (0x0664 + 2 * i), 0x0000); }
    for (i = 0; i < 3; i++)
        { cyc(24); wr16(b | (0x065E + 2 * i), cyc_then(24, rd16(b | (0x0664 + 2 * i)))); }
    cyc(84);
    Gsu8325(bank);
    cyc(22);
    for (i = 0; i < 9; i++) {
        cyc(24);
        v = rd16(b | (0x0608 + 2 * i));
        cyc(24);
        wr16(b | (0x064C + 2 * i), v);
    }
    cyc(38);
    for (i = 0; i < 9; i++)
        { cyc(24); wr8(b | high[i], (i % 4) == 0 ? 0x7F : 0x00); }
    for (i = 0; i < 9; i++)
        { cyc(24); WR8I(view_matrix, 2 * i, 0x00); }
    cyc(72);
    return (uint16_t)((v & 0xFF00) | 0x7F);
}

void ViewReset_cr(Carried *cr)
{
    cr->a = ViewReset(cr->db);
    cr->x = 0x0025;
    cr->y &= 0x00FF;
}

void abi_ViewReset(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ViewReset_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

uint16_t GsuParamsFromDp(uint8_t bank);
uint8_t ReadTable9022(uint8_t bank);

/* The setup tail at $03:AE73 (with its own entry at $03:AE70, PHP and 8-bit
   registers, that no traced code calls): seeds the random number state
   ($E6-$E9), clears the view state, then ViewReset, ReadTable9022 and
   GsuParamsFromDp. Returns GsuParamsFromDp's A. */
static uint16_t view_setup_tail(uint8_t bank)
{
    static const uint8_t dp[8] = {0x9C, 0xBD, 0x99, 0x9F, 0xA0, 0xA1, 0x9D, 0xD3};
    static const uint16_t clear[5] = {0x0665, 0x0667, 0x0669, 0x035D, 0x035E};
    uint32_t b = (uint32_t)bank << 16;
    unsigned i;

    cyc(46);
    wr8(W(random_e6), cyc_then(16, RD8(wram_1A6C)));
    cyc(32);
    wr8(W(random_e7), 0xA7);
    cyc(32);
    wr8(W(random_e8), 0x55);
    cyc(32);
    wr8(W(random_e9), 0x7F);
    for (i = 0; i < 8; i++)
        { cyc(16); wr8(dp[i], 0x00); }
    for (i = 0; i < 5; i++)
        { cyc(24); wr8(b | clear[i], 0x00); }
    cyc(40);
    WR8(wram_05FA, 0x00);
    ViewReset(bank);
    cyc(62);
    ReadTable9022(bank);
    return cyc_then(72, GsuParamsFromDp(bank));
}

/*
 * @routine 03:AE70 ViewSetup
 * WHAT IT DOES: The view's setup with P kept: the random number state
 * seeded, the view state cleared, then ViewReset, ReadTable9022 and
 * GsuParamsFromDp (ViewSetupTail). Nothing in the game calls it.
 * HOW: PHP, SEP #$30, then the tail, whose PLP restores P.
 * Entry: D = $0000; any widths. Exit: A = GsuParamsFromDp's A; X = $80;
 * Y = 0; P unchanged.
 */
uint16_t ViewSetup(uint8_t bank)
{
    cyc(22);                            /* SEP #$30; the tail's PLP and RTL are stated */
    return view_setup_tail(bank);
}

void ViewSetup_cr(Carried *cr)
{
    cr->a = ViewSetup(cr->db);
    cr->x = 0x0080;
    cr->y = 0x0000;
}

void abi_ViewSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ViewSetup_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ViewSetup from inside (03:AE73), its P already pushed: the long jumps at
   the ends of ViewDefaults and AttractViewStart come here; the callers'
   C does the PLP of their own P. */
uint16_t ViewSetupTail(uint8_t bank)
{
    cyc(10);
    return view_setup_tail(bank);
}

/*
 * @routine 03:B67B ViewDefaults
 * WHAT IT DOES: Sets the default view: Super FX words $70:01D0 = $1234,
 * $70:01C4 = 1, $70:108C = 0; $A0 = $80, $05EF = $FF, $00 = 0; the
 * direct-page view parameters $2A = 0, $2E = $67, $2C = 0, $30 = $58,
 * $CE = $34, $D0 = $2C, $34 = $5D17, $32 = $9D89; then the setup tail at
 * $03:AE73 (random seed, cleared state, ViewReset, ReadTable9022,
 * GsuParamsFromDp).
 * HOW: Byte stores in the original's order ($A0 is written here and cleared
 * again in the tail), then a long jump into the tail, which returns through
 * this routine's PLP.
 * Entry: D = $0000; any widths. Exit: A = $5D17; X = $80; Y = 0; P
 * unchanged.
 */
uint16_t ViewDefaults(uint8_t bank)
{
    static const uint8_t dp[16][2] = {
        {0x2A, 0x00}, {0x2B, 0x00}, {0x2E, 0x67}, {0x2F, 0x00}, {0x2C, 0x00}, {0x2D, 0x00},
        {0x30, 0x58}, {0x31, 0x00}, {0xCE, 0x34}, {0xCF, 0x00}, {0xD0, 0x2C}, {0xD1, 0x00},
        {0x34, 0x17}, {0x35, 0x5D}, {0x32, 0x89}, {0x33, 0x9D}};
    unsigned i;

    cyc(178);
    wr16(0x7001D0, 0x1234);
    cyc(56);
    wr16(0x7001C4, 0x0001);
    cyc(56);
    wr16(0x70108C, 0x0000);
    cyc(54);
    wr8(W(dp_a0), 0x80);
    cyc(40);
    WR8(wram_05EF, 0xFF);
    cyc(16);
    wr8(W(dp_00), 0x00);
    cyc(22);
    for (i = 0; i < 16; i++)
        { cyc(32); wr8(dp[i][0], dp[i][1]); }
    cyc(10);
    return view_setup_tail(bank);
}

void ViewDefaults_cr(Carried *cr)
{
    cr->a = ViewDefaults(cr->db);
    cr->x = 0x0080;
    cr->y = 0x0000;
}

void abi_ViewDefaults(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ViewDefaults_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void GsuBytes200_0F090A(void);
void GsuBytes200_BFBCBD(void);
void GsuBytes200_0F0405(void);
void GsuBytes200_BFB7B6(void);

/*
 * @routine 07:85E1 GsuF0A5OrF0B4
 * WHAT IT DOES: Sets the renderer input at $70:020C to A, then runs Super FX
 * program $07:F0A5 with the bytes $0F, $09, $0A at $70:0200 when $0D62 is
 * nonzero, else program $07:F0B4 with $BF, $BC, $BD.
 * HOW: GsuBytes200_0F090A or GsuBytes200_BFBCBD, then GsuRun.
 * Entry: M 16-bit, X 8-bit, D = $0000; A = the input. Exit: A = the screen
 * mode value (high byte clear); X = the program address's low byte; Y high
 * byte clear; M 16-bit, X 8-bit; N, Z from the data bank.
 */
uint8_t GsuF0A5OrF0B4(uint8_t bank, uint16_t input, uint8_t *set)
{
    (void)bank;
    wr16(0x70020C, input);
    cyc(24);
    *set = RD8(wram_0D62);
    if (*set) {
        cyc(110);
        GsuBytes200_0F090A();
        cyc(218);
        return GsuRun(0x07, 0xF0A5);
    }
    (cyc(116), GsuBytes200_BFBCBD());
    cyc(218);
    return GsuRun(0x07, 0xF0B4);
}

void GsuF0A5OrF0B4_cr(Carried *cr)
{
    cyc(32);
    uint8_t set;

    cr->a = GsuF0A5OrF0B4(cr->db, cr->a, &set);
    cr->x = set ? 0x00A5 : 0x00B4;
    cr->y &= 0x00FF;
    set_p(cr, (uint8_t)((nz8(cr->p, cr->db) & ~P_M) | P_X));
    cyc(86);
}

void abi_GsuF0A5OrF0B4(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuF0A5OrF0B4_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:8619 GsuEEF5OrEFCD
 * WHAT IT DOES: Sets the renderer input at $70:020C to A, then runs Super FX
 * program $07:EEF5 with the bytes $0F, $04, $05 at $70:0200 when $0D62 is
 * nonzero, else program $07:EFCD with $BF, $B7, $B6.
 * HOW: GsuBytes200_0F0405 or GsuBytes200_BFB7B6, then GsuRun.
 * Entry: M 16-bit, X 8-bit, D = $0000; A = the input. Exit: A = the screen
 * mode value (high byte clear); X = the program address; M 16-bit, X
 * 16-bit; N, Z from the data bank.
 */
uint8_t GsuEEF5OrEFCD(uint8_t bank, uint16_t input, uint8_t *set)
{
    (void)bank;
    wr16(0x70020C, input);
    cyc(46);
    *set = RD8(wram_0D62);
    if (*set) {
        cyc(110);
        GsuBytes200_0F0405();
        cyc(218);
        return GsuRun(0x07, 0xEEF5);
    } else { cyc(116); }
    GsuBytes200_BFB7B6();
    cyc(218);
    return GsuRun(0x07, 0xEFCD);
}

void GsuEEF5OrEFCD_cr(Carried *cr)
{
    cyc(32);
    uint8_t set;

    cr->a = GsuEEF5OrEFCD(cr->db, cr->a, &set);
    cr->x = set ? 0xEEF5 : 0xEFCD;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
    cyc(86);
}

void abi_GsuEEF5OrEFCD(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuEEF5OrEFCD_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

uint16_t GsuParam216A(uint8_t bank, uint16_t index);

/* Runs program $07:F15B, which draws with the inputs at $70:020A-$70:0216. */
static uint8_t f15b(void)
{
    cyc(146);
    return GsuRun(0x07, 0xF15B);
}

/*
 * @routine 07:9252 GsuDigitPair0E74
 * WHAT IT DOES: Draws the two digits of the byte at $0E74 (high digit, then
 * low) with Super FX program $07:F15B: $70:020E = $10, $70:020A = 4,
 * $70:020C = 8, and $70:0216 = the digit's entry in the table at $8E3E.
 * HOW: GsuParam216A and GsuRun for each digit.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the low digit's table
 * word's high byte and the screen mode value; X = $F15B; C clear; N, Z
 * from the data bank.
 */
static uint8_t gsu_digit_pair_0e74(uint8_t bank, uint16_t *index)
{
    uint16_t v;

    cyc(56);
    wr16(0x70020E, 0x0010);
    cyc(56);
    wr16(0x70020A, 0x0004);
    cyc(80);
    wr16(0x70020C, 0x0008);
    cyc(24);
    v = RD16(wram_0E74);
    cyc(126);
    GsuParam216A(bank, (uint16_t)(((v >> 4) & 0x000F) << 1));
    (cyc(42), f15b());
    cyc(162);
    v = RD16(wram_0E74);
    *index = (uint16_t)((v & 0x000F) << 1);
    cyc(70);
    GsuParam216A(bank, *index);
    return (cyc(42), f15b());
}

static uint16_t digit_index;            /* the adapter's: the low digit's table offset */

uint8_t GsuDigitPair0E74(uint8_t bank)
{
    return gsu_digit_pair_0e74(bank, &digit_index);
}

/* GsuDigitPair0E74 with the A it leaves: the low digit's table word's
   high byte and the screen mode value (X = $F15B). */
uint16_t GsuDigitPair0E74A(uint8_t bank)
{
    uint8_t mode = GsuDigitPair0E74(bank);
    cyc(158);
    uint16_t hi = (uint16_t)(rd16((uint32_t)bank << 16 | (uint16_t)(0x8E3E + digit_index)) & 0xFF00);

    return (uint16_t)(hi | mode);
}

void GsuDigitPair0E74_cr(Carried *cr)
{
    cr->a = GsuDigitPair0E74A(cr->db);
    cr->x = 0xF15B;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_C | P_M | P_X)));
}

void abi_GsuDigitPair0E74(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuDigitPair0E74_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:9883 GsuF15BThree
 * WHAT IT DOES: Runs Super FX program $07:F15B three times, with $70:0216
 * = $9000, $8808 and $881C, after setting $70:0210 = $20, $70:020E = $10,
 * $70:020A = $30, $70:020C = $18 and the bytes $0F, $04, $05 at $70:0200.
 * HOW: GsuBytes200_0F0405, then GsuRun for each value.
 * Entry: any widths, D = $0000. Exit: A = $88 and the screen mode value; X
 * = $F15B; M 16-bit, X 16-bit; N, Z from the data bank.
 */
uint8_t GsuF15BThree(void)
{
    cyc(78);
    wr16(0x700210, 0x0020);
    cyc(56);
    wr16(0x70020E, 0x0010);
    cyc(56);
    wr16(0x70020A, 0x0030);
    cyc(56);
    wr16(0x70020C, 0x0018);
    (cyc(62), GsuBytes200_0F0405());
    cyc(150);
    wr16(0x700216, 0x9000);
    f15b();
    cyc(194);
    wr16(0x700216, 0x8808);
    f15b();
    cyc(194);
    wr16(0x700216, 0x881C);
    return cyc_then(86, f15b());
}

void GsuF15BThree_cr(Carried *cr)
{
    cr->a = (uint16_t)(0x8800 | GsuF15BThree());
    cr->x = 0xF15B;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

void abi_GsuF15BThree(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuF15BThree_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:976B GsuFA1BFromTables
 * WHAT IT DOES: Sets the renderer inputs $70:020A = $0D6D, $70:020C = $0D6F
 * + 8, $70:0210 = $70:0214 = $100 and the bytes $BF, $BC, $BD at $70:0200;
 * then, when entry $0D6B of the word table at $07:97D6 is nonzero, runs
 * Super FX program $07:FA1B with $70:020E = that word, $70:0212 = the same
 * entry of the table at $07:97E2 and $70:0216 = $1E00.
 * HOW: GsuBytes200_BFBCBD, word copies, GsuRun.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: when the entry is 0, A = 0, X
 * = $0D6B, Z set and N clear; otherwise A = $1E and the screen mode value,
 * X = $FA1B, N and Z from the data bank. C and V from the add of 8.
 */
int GsuFA1BFromTables(uint8_t bank, uint8_t *mode, uint8_t *p, uint16_t *index)
{
    (void)bank;
    uint16_t y = RD16(wram_0D6F), s = (uint16_t)(y + 8), x, v;

    GsuBytes200_BFBCBD();
    cyc(96);
    wr16(0x70020A, cyc_then(32, RD16(wram_0D6D)));
    *p = (uint8_t)((*p & ~(P_C | P_V)) | (y > 0xFFF7 ? P_C : 0) | ((~y & s & 0x8000) ? P_V : 0));
    cyc(110);
    wr16(0x70020C, s);
    cyc(56);
    wr16(0x700210, 0x0100);
    cyc(56);
    wr16(0x700214, 0x0100);
    cyc(24);
    x = RD16(wram_0D6B);
    *index = x;
    cyc(102);
    v = rd16(0x070000 | (uint16_t)(0x97D6 + x));
    if (!v)
        return 0;
    wr16(0x70020E, v);
    cyc(80);
    wr16(0x700212, rd16(0x070000 | (uint16_t)(0x97E2 + x)));
    cyc(56);
    wr16(0x700216, 0x1E00);
    cyc(146);
    *mode = GsuRun(0x07, 0xFA1B);
    return 1;
}

void GsuFA1BFromTables_cr(Carried *cr)
{
    cyc(62);
    uint16_t x;
    uint8_t p = cr->p, mode = 0;

    if (GsuFA1BFromTables(cr->db, &mode, &p, &x)) {
        cyc(154);
        cr->a = (uint16_t)(0x1E00 | mode);
        cr->x = 0xFA1B;
        set_p(cr, (uint8_t)(nz8(p, cr->db) & ~(P_M | P_X)));
    } else {
        cr->a = 0x0000;
        cr->x = x;
        set_p(cr, (uint8_t)((p & ~P_N) | P_Z));
    }
    cyc(4);
}

void abi_GsuFA1BFromTables(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuFA1BFromTables_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:E01C Gsu81E3
 * WHAT IT DOES: Runs Super FX program $01:81E3 with direct page $02 at
 * $70:0068 and direct page $08 at $70:002C, and returns the word it leaves
 * at $70:0040.
 * HOW: Two long word stores, GsuRun, one long word load; X and P are saved
 * around it.
 * Entry: any widths, D = $0000. Exit: A = $70:0040; X and P unchanged.
 */
uint16_t Gsu81E3(void)
{
    cyc(90);
    wr16(GSU_IN_68, cyc_then(32, rd16(W(dp_02))));
    cyc(16);
    wr16(GSU_DEST, cyc_then(32, rd16(W(dp_08))));
    cyc(146);
    GsuRun(0x01, 0x81E3);
    cyc(126);
    return cyc_then(106, rd16(0x700040));
}

void Gsu81E3_cr(Carried *cr)
{
    cr->a = Gsu81E3();
}

void abi_Gsu81E3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Gsu81E3_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:DF24 ViewMatrixUpdateAndSave
 * WHAT IT DOES: Saves the view position ($0664-$0669 to $065E-$0663), has
 * the Super FX recompute the nine-word view matrix at $0608 (Gsu8325) and
 * saves a copy of it at $064C-$065D.
 * HOW: Word copies through the data bank around Gsu8325.
 * Entry: D = $0000. Exit: A = the last matrix word (16-bit); X = $25, Y
 * high byte clear; M 8-bit, X 8-bit; N, Z from A as a 16-bit value.
 */
uint16_t ViewMatrixUpdateAndSave(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t v = 0;
    unsigned i;

    cyc(22);
    for (i = 0; i < 3; i++)
        { cyc(24); wr16(b | (0x065E + 2 * i), cyc_then(24, rd16(b | (0x0664 + 2 * i)))); }
    cyc(84);
    Gsu8325(bank);
    cyc(22);
    for (i = 0; i < 9; i++) {
        cyc(24);
        v = rd16(b | (0x0608 + 2 * i));
        cyc(24);
        wr16(b | (0x064C + 2 * i), v);
    }
    cyc(66);
    return v;
}

void ViewMatrixUpdateAndSave_cr(Carried *cr)
{
    cr->a = ViewMatrixUpdateAndSave(cr->db);
    cr->x = 0x0025;
    cr->y &= 0x00FF;
    set_p(cr, (uint8_t)(nz16(cr->p, cr->a) | P_M | P_X));
}

void abi_ViewMatrixUpdateAndSave(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ViewMatrixUpdateAndSave_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:E016 Gsu81E3Long
 * WHAT IT DOES: Gsu81E3 for callers in other banks.
 * HOW: A near call to Gsu81E3, then 16-bit A and a long return.
 * Entry: as Gsu81E3. Exit: as Gsu81E3, with M 16-bit.
 */
uint16_t Gsu81E3Long(void)
{
    uint16_t v;

    cyc(46);
    v = Gsu81E3();
    cyc(66);
    return v;
}

void Gsu81E3Long_cr(Carried *cr)
{
    cr->a = Gsu81E3Long();
    set_p(cr, (uint8_t)(cr->p & ~P_M));
}

void abi_Gsu81E3Long(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Gsu81E3Long_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:DB59 ViewUpdateAndGsuAEF8
 * WHAT IT DOES: Updates the view matrix (ViewMatrixUpdateAndSave), then
 * runs Super FX program $01:AEF8.
 * HOW: Two long calls; GsuRun with A = 1, X = $AEF8.
 * Entry: D = $0000. Exit: A low byte = the screen mode value; X = $AEF8;
 * Y high byte clear; M 8-bit, X 16-bit; N, Z from the data bank.
 */
static uint8_t aef8_high;               /* the adapter's: A's high byte from the matrix */

uint8_t ViewUpdateAndGsuAEF8(uint8_t bank)
{
    uint8_t mode;

    cyc(62);
    aef8_high = (uint8_t)(ViewMatrixUpdateAndSave(bank) >> 8);
    mode = (cyc(168), GsuRun(0x01, 0xAEF8));
    cyc(72);                            /* GsuRun's PLB, RTL */
    cyc(88);
    return mode;
}

void ViewUpdateAndGsuAEF8_cr(Carried *cr)
{
    uint8_t mode = ViewUpdateAndGsuAEF8(cr->db);

    cr->a = (uint16_t)(aef8_high << 8 | mode);
    cr->x = 0xAEF8;
    cr->y &= 0x00FF;
    set_p(cr, (uint8_t)((nz8(cr->p, cr->db) | P_M) & ~P_X));
}

void abi_ViewUpdateAndGsuAEF8(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ViewUpdateAndGsuAEF8_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 07:8AA6 GsuF329Sequence
 * WHAT IT DOES: Runs Super FX program $07:F329 seven times: for each of the
 * five bytes at Y (through the data bank) via GsuF329Entry, with two runs
 * between them that set $70:0216 = $980A (after the first byte) and $980C
 * (after the third).
 * HOW: GsuF329Entry and GsuRun; each byte is a word load (M 16-bit), of
 * which GsuF329Entry keeps the low four bits.
 * Entry: M 16-bit, X 16-bit, D = $0000; Y = the bytes. Exit: as
 * GsuF329Entry for the fifth byte; Y unchanged.
 */
static uint8_t gsu_f329_sequence(uint8_t bank, uint16_t y, uint16_t *last)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t e;

    cyc(30);
    e = rd16(b | y);
    cyc(46);
    GsuF329Entry(bank, e);
    cyc(128);
    wr16(0x700216, 0x980A);
    cyc(146);
    GsuRun(0x07, 0xF329);
    cyc(146);
    e = rd16(b | (uint16_t)(y + 1));
    cyc(46);
    GsuF329Entry(bank, e);
    cyc(102);
    e = rd16(b | (uint16_t)(y + 2));
    cyc(46);
    GsuF329Entry(bank, e);
    cyc(128);
    wr16(0x700216, 0x980C);
    cyc(146);
    GsuRun(0x07, 0xF329);
    cyc(146);
    e = rd16(b | (uint16_t)(y + 3));
    cyc(46);
    GsuF329Entry(bank, e);
    cyc(102);
    e = rd16(b | (uint16_t)(y + 4));
    *last = e;
    cyc(46);
    return GsuF329Entry(bank, e);
}

static uint16_t f329_last;              /* the adapter's: the fifth byte's word */

uint8_t GsuF329Sequence(uint8_t bank, uint16_t y)
{
    return gsu_f329_sequence(bank, y, &f329_last);
}

/* What GsuF329Sequence's exit adds to A: the high byte of the fifth
   byte's entry in the table at $8E52. */
static uint16_t f329_high(uint8_t bank)
{
    uint16_t index = (uint16_t)((f329_last & 0x000F) << 1);

    cyc(114);
    return (uint16_t)(rd16((uint32_t)bank << 16 | (uint16_t)(0x8E52 + index)) & 0xFF00);
}

void GsuF329Sequence_cr(Carried *cr)
{
    uint8_t mode = GsuF329Sequence(cr->db, cr->y);

    cr->a = (uint16_t)(f329_high(cr->db) | mode);
    cr->x = 0xF329;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~P_C));
}

void abi_GsuF329Sequence(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuF329Sequence_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:997F GsuF15BFive
 * WHAT IT DOES: Runs Super FX program $07:F15B five times, with $70:0216
 * = $8410, $8C0C, $8408, $8C10 and $8804, after setting $70:0210 = $20,
 * $70:020E = $10, $70:020A = $18, $70:020C = $18 and the bytes $0F, $09,
 * $0A at $70:0200.
 * HOW: GsuBytes200_0F090A, then GsuRun for each value.
 * Entry: any widths, D = $0000. Exit: A = $88 and the screen mode value; X
 * = $F15B; M 16-bit, X 16-bit; N, Z from the data bank.
 */
uint8_t GsuF15BFive(void)
{
    static const uint16_t in216[5] = {0x8410, 0x8C0C, 0x8408, 0x8C10, 0x8804};
    uint8_t mode = 0;
    unsigned i;

    cyc(124);
    wr16(0x700210, 0x0020);
    cyc(56);
    wr16(0x70020E, 0x0010);
    cyc(56);
    wr16(0x70020A, 0x0018);
    cyc(56);
    wr16(0x70020C, 0x0018);
    (cyc(62), GsuBytes200_0F090A());
    for (i = 0; i < 5; i++) {
        cyc(i ? 194 : 150);             /* the first follows GsuBytes200_0F090A */
        wr16(0x700216, in216[i]);
        mode = f15b();
    }
    cyc(200);
    return mode;
}

void GsuF15BFive_cr(Carried *cr)
{
    cr->a = (uint16_t)(0x8800 | GsuF15BFive());
    cr->x = 0xF15B;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

void abi_GsuF15BFive(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuF15BFive_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:92B5 GsuFA5AThree
 * WHAT IT DOES: Runs Super FX program $07:FA5A three times around the point
 * ($0D6D, $0D6F): at (X - 8, Y) with $70:0216 = $1A10 and (X - 8, Y + $18)
 * with $1B10, both with $70:020E/0210/0212/0214 = $100; then at (X - 8,
 * Y + 8) with $1A18, $70:0210 = $200 and $70:0214 = $80. The point goes to
 * $70:020A and $70:020C.
 * HOW: Word stores and GsuRun.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = $1A and the screen mode
 * value; X = $FA5A; N, Z from the data bank; C, V from the add of 8.
 */
uint8_t GsuFA5AThree(uint8_t bank, uint8_t *p)
{
    (void)bank;
    uint16_t y;

    cyc(56);
    wr16(0x70020E, 0x0100);
    cyc(56);
    wr16(0x700212, 0x0100);
    cyc(56);
    wr16(0x700210, 0x0100);
    cyc(56);
    wr16(0x700214, 0x0100);
    cyc(24);
    wr16(0x70020A, cyc_then(70, (uint16_t)(RD16(wram_0D6D) - 8)));
    cyc(24);
    wr16(0x70020C, cyc_then(32, RD16(wram_0D6F)));
    cyc(56);
    wr16(0x700216, 0x1A10);
    cyc(146);
    GsuRun(0x07, 0xFA5A);
    cyc(140);
    wr16(0x70020A, cyc_then(70, (uint16_t)(RD16(wram_0D6D) - 8)));
    cyc(24);
    wr16(0x70020C, cyc_then(70, (uint16_t)(RD16(wram_0D6F) + 0x18)));
    cyc(56);
    wr16(0x700216, 0x1B10);
    cyc(146);
    GsuRun(0x07, 0xFA5A);
    cyc(172);
    wr16(0x700210, 0x0200);
    cyc(56);
    wr16(0x700214, 0x0080);
    cyc(24);
    wr16(0x70020A, cyc_then(70, (uint16_t)(RD16(wram_0D6D) - 8)));
    cyc(24);
    y = RD16(wram_0D6F);
    *p = (uint8_t)((*p & ~(P_C | P_V)) | (y > 0xFFF7 ? P_C : 0) | ((~y & (uint16_t)(y + 8) & 0x8000) ? P_V : 0));
    cyc(70);
    wr16(0x70020C, (uint16_t)(y + 8));
    cyc(56);
    wr16(0x700216, 0x1A18);
    cyc(146);
    return cyc_then(158, GsuRun(0x07, 0xFA5A));
}

void GsuFA5AThree_cr(Carried *cr)
{
    uint8_t p = cr->p, mode = GsuFA5AThree(cr->db, &p);

    cr->a = (uint16_t)(0x1A00 | mode);
    cr->x = 0xFA5A;
    set_p(cr, nz8(p, cr->db));
}

void abi_GsuFA5AThree(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuFA5AThree_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A 16-bit ADC with carry clear: the sum, with C and V set as the 65816 sets them. */
static uint16_t add16(uint16_t a, uint16_t m, uint8_t *p)
{
    uint16_t r = (uint16_t)(a + m);

    *p = (uint8_t)((*p & ~(P_C | P_V)) | ((unsigned)a + m > 0xFFFF ? P_C : 0)
                   | ((~(a ^ m) & (a ^ r) & 0x8000) ? P_V : (cyc(16), 0)));
    return r;
}

/* Program $07:FA5A at ($70:020A, $70:020C) with piece $70:0216. */
static uint8_t fa5a(uint16_t x, uint16_t y, uint16_t piece)
{
    wr16(0x70020A, x);
    cyc(56);
    wr16(0x700216, piece);
    cyc(72);
    wr16(0x70020C, y);
    cyc(146);
    return GsuRun(0x07, 0xFA5A);
}

/*
 * @routine 07:9660 GsuFA5AFromTables
 * WHAT IT DOES: Runs Super FX program $07:FA5A three times at the point
 * ($0D6D, $0D6F): piece $1B18 at (X, Y) with $70:020E and $70:0212 from
 * entry $0D6B of the word tables at $07:97BE and $07:97CA; piece $1D18 at
 * (X, Y + $18); piece $1A55 at (X, Y + 8) with $70:0210 = $200 and
 * $70:0214 = $80 (the first two with both $100).
 * HOW: Word stores and GsuRun.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the screen mode value
 * (high byte clear); X = $FA5A; N, Z from the data bank; C, V from the add
 * of 8.
 */
uint8_t GsuFA5AFromTables(uint8_t bank, uint8_t *p)
{
    (void)bank;
    cyc(24);
    uint16_t i = RD16(wram_0D6B), y;

    cyc(80);
    wr16(0x70020E, rd16(0x070000 | (uint16_t)(0x97BE + i)));
    cyc(80);
    wr16(0x700212, rd16(0x070000 | (uint16_t)(0x97CA + i)));
    cyc(56);
    wr16(0x700216, 0x1B18);
    cyc(24);
    wr16(0x70020A, cyc_then(32, RD16(wram_0D6D)));
    cyc(24);
    wr16(0x70020C, cyc_then(32, RD16(wram_0D6F)));
    cyc(56);
    wr16(0x700210, 0x0100);
    cyc(56);
    wr16(0x700214, 0x0100);
    cyc(146);
    GsuRun(0x07, 0xFA5A);
    cyc(140);
    wr16(0x70020A, cyc_then(32, RD16(wram_0D6D)));
    cyc(24);
    wr16(0x70020C, cyc_then(54, add16(RD16(wram_0D6F), 0x0018, p)));
    cyc(56);
    wr16(0x700210, 0x0100);
    cyc(56);
    wr16(0x700214, 0x0100);
    cyc(56);
    wr16(0x700216, 0x1D18);
    cyc(146);
    GsuRun(0x07, 0xFA5A);
    cyc(140);
    y = add16(RD16(wram_0D6F), 0x0008, p);
    cyc(54);
    wr16(0x70020C, y);
    cyc(24);
    wr16(0x70020A, cyc_then(32, RD16(wram_0D6D)));
    cyc(56);
    wr16(0x700216, 0x1A55);
    cyc(56);
    wr16(0x700210, 0x0200);
    cyc(56);
    wr16(0x700214, 0x0080);
    cyc(146);
    return GsuRun(0x07, 0xFA5A);
}

void GsuFA5AFromTables_cr(Carried *cr)
{
    uint8_t p = cr->p, mode = GsuFA5AFromTables(cr->db, &p);

    cr->a = mode;
    cr->x = 0xFA5A;
    set_p(cr, nz8(p, cr->db));
}

void abi_GsuFA5AFromTables(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuFA5AFromTables_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:9366 GsuFA5ASix
 * WHAT IT DOES: Runs Super FX program $07:FA5A six times, all with
 * $70:020E/0210/0212/0214 = $100, from X = $0D6D + entry $0D6B of the word
 * table at $07:9806 (kept in direct page $F1) and Y = $0D6F: pieces $1A0E,
 * $1A16, $1B16, $1B0E at Y, Y + 8, Y + $10, Y + $18; then at X + 8, pieces
 * $1A0E and $1B0E at Y + 8 and Y + $10.
 * HOW: Word stores and GsuRun; $F1 ends as X + 8.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = $1B and the screen mode
 * value; X = $FA5A; N, Z from the data bank; C, V from the add of $10.
 */
uint8_t GsuFA5ASix(uint8_t bank, uint8_t *p)
{
    (void)bank;
    static const uint16_t dy[6] = {0x0000, 0x0008, 0x0010, 0x0018, 0x0008, 0x0010};
    static const uint16_t piece[6] = {0x1A0E, 0x1A16, 0x1B16, 0x1B0E, 0x1A0E, 0x1B0E};
    uint16_t x, y;
    uint8_t mode = 0;
    unsigned i;

    cyc(260);
    wr16(0x70020E, 0x0100);
    cyc(56);
    wr16(0x700212, 0x0100);
    cyc(56);
    wr16(0x700210, 0x0100);
    cyc(56);
    wr16(0x700214, 0x0100);
    cyc(24);
    x = add16(rd16(cyc_then(86, 0x070000 | (uint16_t)(0x9806 + RD16(wram_0D6B)))), RD16(wram_0D6D), p);
    wr16(W(recip_divisor), x);
    for (i = 0; i < 6; i++) {
        if (i == 4) {
            x = add16(rd16(W(recip_divisor)), 0x0008, p);
            cyc(38);
            wr16(W(recip_divisor), x);
        } else { cyc(16); }
        cyc(16);
        y = RD16(wram_0D6F);
        if (dy[i])
            y = add16(y, dy[i], p);
        if (i == 0) {
            mode = fa5a(x, y, piece[i]);
        } else {
            wr16(0x70020A, x);
            cyc(110);
            wr16(0x70020C, y);
            cyc(56);
            wr16(0x700216, piece[i]);
            cyc(146);
            mode = GsuRun(0x07, 0xFA5A);
        }
        cyc(132);
    }
    cyc(26);
    return mode;
}

void GsuFA5ASix_cr(Carried *cr)
{
    uint8_t p = cr->p, mode = GsuFA5ASix(cr->db, &p);

    cr->a = (uint16_t)(0x1B00 | mode);
    cr->x = 0xFA5A;
    set_p(cr, nz8(p, cr->db));
}

void abi_GsuFA5ASix(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuFA5ASix_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:8653 GsuF15BTwoLines
 * WHAT IT DOES: Draws sixteen pieces with Super FX program $07:F15B (a run
 * per piece, $70:0216 = the piece): three from ($50, 8) and 13 from
 * (0, $2C), with $70:0210 = A, $70:020E = $10 and the bytes $BF, $B7,
 * $B6 at $70:0200. A piece of 0 is a space.
 * HOW: GsuBytes200_BFB7B6, then GsuRun per piece; the program steps the
 * position itself.
 * Entry: M 16-bit, X 8-bit, D = $0000; A = the input for $70:0210. Exit:
 * A = $8C and the screen mode value; X = $F15B; M 16-bit, X 16-bit; N, Z
 * from the data bank.
 */
uint8_t GsuF15BTwoLines(uint16_t input)
{
    static const uint16_t first[3] = {0x8800, 0x8418, 0x8C14};
    static const uint16_t second[13] = {0x8408, 0x8814, 0x8814, 0x0000, 0x8C14, 0x8804, 0x8418,
                                        0x0000, 0x8C10, 0x8C14, 0x8408, 0x8C0C, 0x8C10};
    uint8_t mode = 0;
    unsigned i;

    cyc(64);
    wr16(0x700210, input);
    cyc(56);
    wr16(0x70020E, 0x0010);
    cyc(56);
    wr16(0x70020A, 0x0050);
    cyc(56);
    wr16(0x70020C, 0x0008);
    (cyc(62), GsuBytes200_BFB7B6());
    for (i = 0; i < 3; i++) {
        cyc(i ? 194 : 150);             /* the first follows GsuBytes200_BFB7B6 */
        wr16(0x700216, first[i]);
        f15b();
    }
    cyc(194);
    wr16(0x70020A, 0x0000);
    cyc(56);
    wr16(0x70020C, 0x002C);
    for (i = 0; i < 13; i++) {
        cyc(78);
        wr16(0x700216, second[i]);
        mode = f15b();
        cyc(116);
    }
    return mode;
}

void GsuF15BTwoLines_cr(Carried *cr)
{
    cr->a = (uint16_t)(0x8C00 | GsuF15BTwoLines(cr->a));
    cr->x = 0xF15B;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

void abi_GsuF15BTwoLines(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuF15BTwoLines_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* $70:0210 = input, $70:020E = $10, the position (x, $28), the bytes
   $BF, $B7, $B6 at $70:0200, then each piece through program $07:F15B. */
static uint8_t glyph_rows_at(uint16_t input, uint16_t x, const uint16_t *piece, unsigned n)
{
    uint8_t mode = 0;
    unsigned i;

    cyc(64);
    wr16(0x700210, input);
    cyc(56);
    wr16(0x70020E, 0x0010);
    cyc(56);
    wr16(0x70020A, x);
    cyc(56);
    wr16(0x70020C, 0x0028);
    cyc(62);
    GsuBytes200_BFB7B6();
    for (i = 0; i < n; i++) {
        cyc(i ? 194 : 150);
        wr16(0x700216, piece[i]);
        mode = f15b();
    }
    cyc(22);
    return mode;
}

/*
 * @routine 07:8821 GlyphRowsAt32
 * WHAT IT DOES: Draws nine pieces with Super FX program $07:F15B from (32,
 * 40): $881C, $8418, $9004, $8C14, space, $8408, $8C0C, $8418, $8408,
 * with $70:0210 = A, $70:020E = $10 and the bytes $BF, $B7, $B6 at
 * $70:0200 (StuntPanelShow's line).
 * HOW: As GsuF15BTwoLines.
 * Entry: M 16-bit, D = $0000; A = the input for $70:0210. Exit: A = $84
 * and the screen mode value; X = $F15B; M 16-bit, X 16-bit; N, Z from the
 * data bank.
 */
uint8_t GlyphRowsAt32(uint16_t input)
{
    static const uint16_t piece[9] = {0x881C, 0x8418, 0x9004, 0x8C14, 0x0000,
                                      0x8408, 0x8C0C, 0x8418, 0x8408};
    uint8_t mode = glyph_rows_at(input, 0x0020, piece, 9);

    cyc(136);                           /* GsuRun's PLB, RTL; the second REP, RTS */
    return mode;
}

void GlyphRowsAt32_cr(Carried *cr)
{
    cr->a = (uint16_t)(0x8400 | GlyphRowsAt32(cr->a));
    cr->x = 0xF15B;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

void abi_GlyphRowsAt32(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphRowsAt32_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:8929 GlyphRowsAt56
 * WHAT IT DOES: Draws six pieces with Super FX program $07:F15B from (56,
 * 40): $841C, $8808, $881C, $8808, $8C10, $8804, with $70:0210 = A,
 * $70:020E = $10 and the bytes $BF, $B7, $B6 at $70:0200
 * (StuntPanelDone's line).
 * HOW: As GsuF15BTwoLines.
 * Entry: M 16-bit, D = $0000; A = the input for $70:0210. Exit: A = $88
 * and the screen mode value; X = $F15B; M 16-bit, X 16-bit; N, Z from the
 * data bank.
 */
uint8_t GlyphRowsAt56(uint16_t input)
{
    static const uint16_t piece[6] = {0x841C, 0x8808, 0x881C, 0x8808, 0x8C10, 0x8804};

    return glyph_rows_at(input, 0x0038, piece, 6);
}

void GlyphRowsAt56_cr(Carried *cr)
{
    cr->a = (uint16_t)(0x8800 | GlyphRowsAt56(cr->a));
    cr->x = 0xF15B;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

void abi_GlyphRowsAt56(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphRowsAt56_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void GsuBytes200_BFBAB9(void);

/* DB:X + k for an absolute,X load (the sum carries into the next bank). */
static uint32_t absx(uint8_t bank, uint16_t x, uint16_t k)
{
    return (((uint32_t)bank << 16) + x + k) & 0xFFFFFF;
}

/* The piece list loop of 07:81C2 and 07:81ED: each word of the list at X
   up to $FFFF to $70:0216, then GsuBytes200_BFB7B6 and the program (X and
   P kept across the calls). Returns the end marker's offset; the adapters
   set N, Z, C as CMP #$FFFF leaves them. */
static uint16_t pieces_list(uint8_t bank, uint16_t x, uint16_t program)
{
    uint16_t a;

    for (;;) {
        a = rd16(cyc_then(30, absx(bank, x, 0)));
        if (a == 0xFFFF)
            { cyc(88); break; }
        cyc(72);
        wr16(0x700216, a);
        (cyc(114), GsuBytes200_BFB7B6());
        cyc(218);
        GsuRun(0x07, program);
        cyc(program == 0xF6DF ? 252 : 230);     /* RTL, the rest of the loop */
        x = (uint16_t)(x + 2);
    }
    return x;
}

/*
 * @routine 07:8188 GsuPiecesFromRecord
 * WHAT IT DOES: Copies a piece record at X (through the data bank) to the
 * Super FX inputs: its words +$00 to +$0A to $70:020A-$70:0214 and +$0E to
 * $70:0216 (+$0C is read and dropped), then draws the pieces of the list
 * after it (GsuPiecesListFAF2). Nothing calls it (no JSR, JSL, pointer or table reaches it), so a
 * direct call is its only way in.
 * HOW: Seven loads and stores, the X step past the record with CLC, ADC
 * #$000E; then as GsuPiecesListFAF2.
 * Entry: M 16-bit, X 16-bit; X = the record. Exit: A = $FFFF; X = the end
 * marker's offset; N clear, Z and C set (CMP #$FFFF); V from the ADC.
 */
static int record_v;                    /* the adapter's: V from the ADC #$000E */

uint16_t GsuPiecesFromRecord(uint8_t bank, uint16_t x)
{
    static const uint16_t to[6] = {0x020A, 0x020C, 0x020E, 0x0210, 0x0212, 0x0214};
    unsigned i;
    uint16_t list;

    for (i = 0; i < 6; i++)
        wr16(0x700000 | to[i], cyc_then(32, rd16(cyc_then(30, absx(bank, x, (uint16_t)(2 * i))))));
    rd16(cyc_then(30, absx(bank, x, 0x000C)));
    wr16(0x700216, cyc_then(32, rd16(cyc_then(30, absx(bank, x, 0x000E)))));
    list = (uint16_t)(x + 0x000E);
    record_v = (~x & list & 0x8000) != 0;
    cyc(66);
    return pieces_list(bank, list, 0xFAF2);
}

void GsuPiecesFromRecord_cr(Carried *cr)
{
    cr->x = GsuPiecesFromRecord(cr->db, cr->x);
    cr->a = 0xFFFF;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_V | P_Z | P_C)) | (record_v ? P_V : 0) | P_Z | P_C));
}

void abi_GsuPiecesFromRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuPiecesFromRecord_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:81C2 GsuPiecesListFAF2
 * WHAT IT DOES: Draws each piece of the list at X (words through the data
 * bank, ended by $FFFF): the word to $70:0216, the renderer bytes 0, $BF,
 * $B7, $B6 (GsuBytes200_BFB7B6) and Super FX program $07:FAF2 (GsuRun).
 * Entered from GsuPiecesFromRecord and GsuPiecesFromBytes; nothing else calls it (no jsr, jsl, pointer or table reaches it), so a
 * direct call is its only way in.
 * HOW: X and P pushed around the two calls (PHX, PHP; PLP, PLX), X
 * stepped by 2.
 * Entry: M 16-bit, X 16-bit; X = the list. Exit: A = $FFFF; X = the end
 * marker's offset; N clear, Z and C set (CMP #$FFFF); the other flags as at
 * entry.
 */
uint16_t GsuPiecesListFAF2(uint8_t bank, uint16_t list)
{
    return pieces_list(bank, list, 0xFAF2);
}

void GsuPiecesListFAF2_cr(Carried *cr)
{
    cr->x = GsuPiecesListFAF2(cr->db, cr->x);
    cr->a = 0xFFFF;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C)) | P_Z | P_C));
}

void abi_GsuPiecesListFAF2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuPiecesListFAF2_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:81EC GsuPiecesReturn
 * WHAT IT DOES: A lone RTS after GsuPiecesFromRecord's own. Nothing calls it (no JSR, JSL, pointer or table reaches it), so a
 * direct call is its only way in.
 * HOW: RTS.
 * Entry: any. Exit: nothing changed.
 */
void abi_GsuPiecesReturn(void)
{
    cyc(42);
}

/*
 * @routine 07:81ED GsuPiecesListF6DF
 * WHAT IT DOES: As GsuPiecesListFAF2 with Super FX program $07:F6DF: each
 * piece of the list at X (ended by $FFFF) to $70:0216, the bytes 0, $BF,
 * $B7, $B6 and the program. Nothing calls it (no JSR, JSL, pointer or table reaches it), so a
 * direct call is its only way in.
 * HOW: REP #$30 at the top of each piece; X and P pushed around the calls.
 * Entry: X = the list. Exit: A = $FFFF; X = the end marker's offset; M and X
 * 16-bit; N clear, Z and C set (CMP #$FFFF); the other flags as at entry.
 */
uint16_t GsuPiecesListF6DF(uint8_t bank, uint16_t list)
{
    cyc(22);
    return pieces_list(bank, list, 0xF6DF);
}

void GsuPiecesListF6DF_cr(Carried *cr)
{
    cr->x = GsuPiecesListF6DF(cr->db, cr->x);
    cr->a = 0xFFFF;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_M | P_X)) | P_Z | P_C));
}

void abi_GsuPiecesListF6DF(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuPiecesListF6DF_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:8219 GsuPieceOffsetFromView
 * WHAT IT DOES: Halves the Super FX words $70:003E and $70:003A into $0E84
 * and $0E82, then sets the piece position $70:020A to $0E82 less half the
 * PPU product of $70:020E and $0E8E (negated), and $70:020C to $0E84 plus
 * the product of $70:0210 and $FFF0. Nothing calls it (no JSR, JSL, pointer or table reaches it), so a
 * direct call is its only way in.
 * HOW: The PPU multiplier: 16-bit stores to M7A ($211B, its high byte to
 * M7B) and M7B ($211C, its high byte to M7C), the product's middle word
 * read at MPYM ($2135); SEC, SBC, EOR #$FFFF, INC; CLC, ADC. Registers and
 * $0Exx through the data bank.
 * Entry: M 16-bit; data bank with the PPU registers. Exit: A = the new
 * $70:020C; X, Y unchanged; N, V, Z, C from the ADC.
 */
static uint8_t offset_flags;            /* the adapter's: N, V, Z, C from the ADC */

uint16_t GsuPieceOffsetFromView(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t a, m, r;

    cyc(32);
    WR16(wram_0E84, cyc_then(38, (uint16_t)(rd16(0x70003E) >> 1)));
    cyc(32);
    WR16(wram_0E82, cyc_then(38, (uint16_t)(rd16(0x70003A) >> 1)));
    cyc(32);
    io_wr16(b | 0x211B, m = cyc_then(24, rd16(0x70020E)));
    ppu_mul_factor = (int16_t)m;
    cyc(24);
    io_wr16(b | 0x211C, m = cyc_then(24, RD16(wram_0E8E)));   /* M7B, then M7C */
    cyc(24);
    a = (uint16_t)(io_rd16(b | 0x2135, (uint16_t)(ppu_mul((uint8_t)m) >> 8)) >> 1);
    cyc(52);
    a = (uint16_t)(a - RD16(wram_0E82));
    cyc(6);                             /* the SBC's time, as op_sbc16 states it */
    a = (uint16_t)((a ^ 0xFFFF) + 1);
    cyc(64);
    wr16(0x70020A, a);
    cyc(32);
    io_wr16(b | 0x211B, m = cyc_then(24, rd16(0x700210)));
    ppu_mul_factor = (int16_t)m;
    cyc(48);
    io_wr16(b | 0x211C, 0xFFF0);
    cyc(24);
    a = io_rd16(b | 0x2135, (uint16_t)(ppu_mul(0xF0) >> 8));
    cyc(38);
    m = RD16(wram_0E84);
    r = (uint16_t)(a + m);
    offset_flags = (uint8_t)(nz16(0, r) | ((unsigned)a + m > 0xFFFF ? P_C : 0)
                             | ((~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0));
    cyc(32);
    wr16(0x70020C, r);
    cyc(42);                            /* RTS */
    return r;
}

void GsuPieceOffsetFromView_cr(Carried *cr)
{
    cr->a = GsuPieceOffsetFromView(cr->db);
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_V | P_Z | P_C)) | offset_flags));
}

void abi_GsuPieceOffsetFromView(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuPieceOffsetFromView_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:82B4 GsuReciprocal
 * WHAT IT DOES: $FFFF divided by the word at $F1, both halved first when it
 * is above $FF (so the divisor fits the CPU divider's 8 bits).
 * HOW: $F3 = $FFFF; CMP #$00FF with $F1, LSR of both words; then WRDIVL =
 * $F3, WRDIVB = $F1 (16-bit: the high byte to HTIMEL), four NOPs, the
 * quotient read at RDDIVL.
 * Entry: M 16-bit, D = $0000; $F1 = the divisor; data bank with the
 * divider. Exit: A = the quotient; N, Z from it; C set; $F3 = $FFFF or
 * $7FFF.
 */
uint16_t GsuReciprocal(uint8_t bank, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t a, n, d;

    cyc(40);
    wr16(W(recip_dividend), 0xFFFF);
    cyc(40);
    if (0x00FF < rd16(W(recip_divisor))) {
        cyc(32);
        wr16(W(recip_divisor), cyc_then(14, (uint16_t)(rd16(W(recip_divisor)) >> 1)));
        cyc(16);
        wr16(W(recip_dividend), cyc_then(14, (uint16_t)(rd16(W(recip_dividend)) >> 1)));
    } else { cyc(22); }
    cyc(16);
    io_wr16(b | 0x4204, n = cyc_then(24, rd16(W(recip_dividend))));
    cyc(16);
    d = cyc_then(24, rd16(W(recip_divisor)));
    io_wr8(b | 0x4206, (uint8_t)d);
    hw_wr(b | 0x4207, (uint8_t)(d >> 8));      /* the high byte to HTIMEL */
    cyc(80);
    a = io_rd16(b | 0x4214, cpu_div(n, (uint8_t)d));
    *p = (uint8_t)(nz16(*p, a) | P_C);
    return a;
}

void GsuReciprocal_cr(Carried *cr)
{
    cr->a = GsuReciprocal(cr->db, &cr->p);
    cyc(42);                            /* RTS */
}

void abi_GsuReciprocal(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuReciprocal_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:825F GsuPieceScaleFromView
 * WHAT IT DOES: Sets the Super FX inputs from the view: $70:020E = (the
 * PPU product of $0E86 and $70:003A) / $0E8E, $70:0210 = (the product of
 * $0E8E and $70:003E) / $0E8E, and $70:0214 and $70:0212 the reciprocals
 * ($FFFF / the value, GsuReciprocal) of those two. Nothing calls it (no JSR, JSL, pointer or table reaches it), so a
 * direct call is its only way in.
 * HOW: The PPU multiplier as in GsuPieceOffsetFromView, the CPU divider
 * (16-bit stores to WRDIVL and WRDIVB, whose high byte goes to HTIMEL;
 * the quotient read at RDDIVL), with NOPs to wait for it; $0E8E kept on
 * the stack (PHA, PLX), the first quotient too (PHA, PLA).
 * Entry: M 16-bit, X 16-bit, D = $0000; data bank with the PPU registers.
 * Exit: A = the second reciprocal; X = $0E8E; N, Z from A; C set.
 */
static uint16_t scale_e;                /* the adapter's: $0E8E, left in X */

uint16_t GsuPieceScaleFromView(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t e, q1, q2, m, n, r;
    uint8_t p = 0;

    cyc(24);
    io_wr16(b | 0x211B, m = cyc_then(24, RD16(wram_0E86)));
    ppu_mul_factor = (int16_t)m;
    cyc(32);
    io_wr16(b | 0x211C, m = cyc_then(24, rd16(0x70003A)));    /* M7B, then M7C */
    cyc(24);
    io_wr16(b | 0x4204, n = cyc_then(24, io_rd16(b | 0x2135, (uint16_t)(ppu_mul((uint8_t)m) >> 8))));
    cyc(24);
    e = RD16(wram_0E8E);
    cyc(54);
    io_wr8(b | 0x4206, (uint8_t)e);
    hw_wr(b | 0x4207, (uint8_t)(e >> 8));      /* the high byte to HTIMEL */
    q1 = cpu_div(n, (uint8_t)e);
    cyc(24);
    io_wr16(b | 0x211B, e);
    ppu_mul_factor = (int16_t)e;
    cyc(32);
    io_wr16(b | 0x211C, m = cyc_then(24, rd16(0x70003E)));
    cyc(24);
    q1 = io_rd16(b | 0x4214, q1);
    cyc(24);
    io_wr16(b | 0x4204, n = cyc_then(24, io_rd16(b | 0x2135, (uint16_t)(ppu_mul((uint8_t)m) >> 8))));
    cyc(60);
    io_wr8(b | 0x4206, (uint8_t)e);
    hw_wr(b | 0x4207, (uint8_t)(e >> 8));
    q2 = cpu_div(n, (uint8_t)e);
    cyc(32);
    wr16(0x70020E, q1);
    cyc(82);
    q2 = io_rd16(b | 0x4214, q2);
    cyc(32);
    wr16(0x700210, q2);
    cyc(16);
    wr16(W(recip_divisor), q2);
    cyc(16);
    wr16(W(recip_divisor), q2);
    cyc(46);
    wr16(0x700214, cyc_then(74, GsuReciprocal(bank, &p)));
    cyc(52);
    wr16(W(recip_divisor), q1);
    cyc(46);
    r = GsuReciprocal(bank, &p);
    cyc(74);
    wr16(0x700212, r);
    scale_e = e;
    cyc(42);                            /* RTS */
    return r;
}

void GsuPieceScaleFromView_cr(Carried *cr)
{
    cr->a = GsuPieceScaleFromView(cr->db);
    cr->x = scale_e;
    set_p(cr, (uint8_t)(nz16(cr->p, cr->a) | P_C));
}

void abi_GsuPieceScaleFromView(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuPieceScaleFromView_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:82D6 GsuReciprocalReturn
 * WHAT IT DOES: A lone RTS after GsuReciprocal's. Nothing calls it (no JSR, JSL, pointer or table reaches it), so a
 * direct call is its only way in.
 * HOW: RTS.
 * Entry: any. Exit: nothing changed.
 */
void abi_GsuReciprocalReturn(void)
{
    cyc(42);
}

/*
 * @routine 07:82D7 GsuPiecesFromBytes
 * WHAT IT DOES: As GsuPiecesFromRecord for a record of bytes and words at
 * X: +$00 and +$01 (bytes) to $70:020A and $70:020C, the words +$02 to +$08
 * to $70:020E-$70:0214, then the pieces of the list the word at +$0B
 * points to (GsuPiecesListFAF2; +$0A is read and dropped). Nothing calls it (no JSR, JSL, pointer or table reaches it), so a
 * direct call is its only way in.
 * HOW: Loads through the data bank (AND #$00FF for the bytes), TAX of the
 * list address; then as GsuPiecesListFAF2.
 * Entry: M 16-bit, X 16-bit; X = the record. Exit: A = $FFFF; X = the end
 * marker's offset; N clear, Z and C set (CMP #$FFFF); the other flags as at
 * entry.
 */
uint16_t GsuPiecesFromBytes(uint8_t bank, uint16_t x)
{
    static const uint16_t to[4] = {0x020E, 0x0210, 0x0212, 0x0214};
    unsigned i;

    wr16(0x70020A, cyc_then(56, (uint16_t)(rd16(cyc_then(30, absx(bank, x, 0))) & 0x00FF)));
    wr16(0x70020C, cyc_then(56, (uint16_t)(rd16(cyc_then(30, absx(bank, x, 1))) & 0x00FF)));
    for (i = 0; i < 4; i++)
        wr16(0x700000 | to[i], cyc_then(32, rd16(cyc_then(30, absx(bank, x, (uint16_t)(2 + 2 * i))))));
    rd16(cyc_then(30, absx(bank, x, 0x000A)));
    x = rd16(cyc_then(54, absx(bank, x, 0x000B)));
    cyc(14);
    return pieces_list(bank, x, 0xFAF2);
}

void GsuPiecesFromBytes_cr(Carried *cr)
{
    cr->x = GsuPiecesFromBytes(cr->db, cr->x);
    cr->a = 0xFFFF;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C)) | P_Z | P_C));
}

void abi_GsuPiecesFromBytes(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuPiecesFromBytes_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:833B GsuPiecesStep10CB
 * WHAT IT DOES: Steps the counter at $10CB (up to $14) and draws eleven
 * pieces from ($10, $28): $800C, $8814, $8408, $8C04, $8C10, space, $8C14,
 * $8C00, space, $8800, $8C00, with $70:0210 = the counter's byte in the
 * table at $85CC (through the data bank), $70:020E = $10 and the bytes
 * $BF, $BA, $B9 at $70:0200. While the counter is below 8 the pieces go
 * through Super FX program $07:F15B, after that through $07:F121.
 * HOW: GsuBytes200_BFBAB9, then GsuRun per piece; after the first piece
 * the X position the program left at $70:020A is moved on by 4.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = $8C and the screen mode
 * value; X = the program address; N, Z from the data bank; C, V from the
 * add of 4.
 */
uint8_t GsuPiecesStep10CB(uint8_t bank, uint16_t *program, uint8_t *p)
{
    static const uint16_t piece[11] = {0x800C, 0x8814, 0x8408, 0x8C04, 0x8C10, 0x0000,
                                       0x8C14, 0x8C00, 0x0000, 0x8800, 0x8C00};
    uint32_t b = (uint32_t)bank << 16;
    cyc(92);
    uint16_t n = (uint16_t)(RD16(piece_counter) + 1);
    uint8_t mode = 0;
    unsigned i;

    if (n >= 0x0014)
        { cyc(18); n = 0x0014; }
    cyc(84);
    WR16(piece_counter, n);
    cyc(70);
    wr16(0x70020E, 0x0010);
    cyc(56);
    wr16(0x70020A, 0x0010);
    cyc(56);
    wr16(0x70020C, 0x0028);
    (cyc(62), GsuBytes200_BFBAB9());
    cyc(102);
    wr16(0x700210, rd8(b | (uint16_t)(0x85CC + n)));
    *program = n < 8 ? 0xF15B : (cyc(18), 0xF121);
    cyc(46);
    for (i = 0; i < 11; i++) {
        cyc(78);
        wr16(0x700216, piece[i]);
        cyc(146);
        mode = GsuRun(0x07, *program);
        if (i == 0)
            { cyc(148); wr16(0x70020A, cyc_then(54, add16(rd16(0x70020A), 0x0004, p))); } else { cyc(116); }
    }
    cyc(42);
    return mode;
}

void GsuPiecesStep10CB_cr(Carried *cr)
{
    uint8_t p = cr->p, mode = GsuPiecesStep10CB(cr->db, &cr->x, &p);

    cr->a = (uint16_t)(0x8C00 | mode);
    set_p(cr, nz8(p, cr->db));
}

void abi_GsuPiecesStep10CB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuPiecesStep10CB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

static uint8_t object_cv;               /* the adapters': C and V of the last subtract */

/*
 * @routine 03:C998 ObjectGsu80BCThen81E3
 * WHAT IT DOES: For object X, runs program $01:80BC (ObjectGsu80BC) and
 * keeps its result in direct page $08, puts $000A (data bank) - the
 * object's +$0E in $02, then runs Gsu81E3Long.
 * HOW: 16-bit subtract; X and Y are saved and restored.
 * Entry: X 16-bit, D = $0000; X = object. Exit: A as Gsu81E3Long leaves it;
 * X, Y unchanged; M 16-bit; N, Z from X.
 */
uint16_t ObjectGsu80BCThen81E3(uint8_t bank, uint16_t x)
{
    uint8_t p = 0;
    uint16_t v;

    cyc(144);
    v = ObjectGsu80BC(bank, x, &p);
    cyc(16);
    wr16(W(dp_08), v);
    cyc(24);
    object_cv = 0;
    v = sub16(RD16(dp_0a), rd16(cyc_then(44, WAI(obj_pos_y, x))), &object_cv);
    cyc(16);
    wr16(W(dp_02), v);
    v = (cyc(62), Gsu81E3Long());
    cyc(62);
    cyc(98);
    return v;
}

void ObjectGsu80BCThen81E3_cr(Carried *cr)
{
    cr->a = ObjectGsu80BCThen81E3(cr->db, cr->x);
    set_p(cr, nz16((uint8_t)((cr->p & ~(P_M | P_X | P_C | P_V)) | object_cv), cr->x));
}

void abi_ObjectGsu80BCThen81E3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectGsu80BCThen81E3_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:C9B4 ObjectGsu81E3
 * WHAT IT DOES: For object X, puts direct page $04 - its +$0C in $02 and
 * $05F2 - its +$10 in $08, then runs Gsu81E3Long.
 * HOW: 16-bit subtracts through the data bank; X and Y saved and restored.
 * Entry: X 16-bit, D = $0000; X = object. Exit: as ObjectGsu80BCThen81E3.
 */
uint16_t ObjectGsu81E3(uint8_t bank, uint16_t x)
{
    (void)bank;
    uint16_t m, s, v;

    object_cv = 0;
    cyc(98);
    m = rd16(W(dp_04));
    cyc(44);
    s = RD16I(obj_pos_x, x);
    wr16(W(dp_02), cyc_then(16, sub16(m, s, &object_cv)));
    cyc(24);
    m = RD16(wram_05F2);
    cyc(44);
    s = RD16I(obj_pos_z, x);
    v = sub16(m, s, &object_cv);
    cyc(16);
    wr16(W(dp_08), v);
    v = (cyc(62), Gsu81E3Long());
    cyc(138);                           /* REP, PLY, PLX, RTL */
    return v;
}

void ObjectGsu81E3_cr(Carried *cr)
{
    cr->a = ObjectGsu81E3(cr->db, cr->x);
    set_p(cr, nz16((uint8_t)((cr->p & ~(P_M | P_C | P_V)) | object_cv), cr->x));
}

void abi_ObjectGsu81E3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectGsu81E3_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Program $07:FA5A with all four of $70:020E/0210/0212/0214 = $100. */
static void fa5a_unit_scale(void)
{
    cyc(56);
    wr16(0x70020E, 0x0100);
    cyc(56);
    wr16(0x700212, 0x0100);
    cyc(56);
    wr16(0x700210, 0x0100);
    cyc(56);
    wr16(0x700214, 0x0100);
}

/*
 * @routine 07:9486 GsuFA5AThreeShifted
 * WHAT IT DOES: As GsuFA5AThree, from X = $0D6D + entry $0D6B of the word
 * table at $07:9806 (kept in direct page $F1) instead of $0D6D - 8, with
 * pieces $1A12, $1B12 and $1C18.
 * HOW: Word stores and GsuRun. Not reached in the traces so far.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = $1C and the screen mode
 * value; X = $FA5A; N, Z from the data bank; C, V from the add of 8.
 */
uint8_t GsuFA5AThreeShifted(uint8_t bank, uint8_t *p)
{
    (void)bank;
    uint16_t x;

    fa5a_unit_scale();
    cyc(24);
    x = add16(rd16(cyc_then(86, 0x079806 + RD16(wram_0D6B))), RD16(wram_0D6D), p);
    wr16(W(recip_divisor), x);
    cyc(32);
    wr16(0x70020A, x);
    cyc(24);
    wr16(0x70020C, cyc_then(32, RD16(wram_0D6F)));
    cyc(56);
    wr16(0x700216, 0x1A12);
    cyc(146);
    GsuRun(0x07, 0xFA5A);
    cyc(132);
    wr16(0x70020A, cyc_then(32, rd16(W(recip_divisor))));
    cyc(24);
    wr16(0x70020C, cyc_then(54, add16(RD16(wram_0D6F), 0x0018, p)));
    cyc(56);
    wr16(0x700216, 0x1B12);
    cyc(146);
    GsuRun(0x07, 0xFA5A);
    cyc(172);
    wr16(0x700210, 0x0200);
    cyc(56);
    wr16(0x700214, 0x0080);
    cyc(16);
    wr16(0x70020A, cyc_then(32, rd16(W(recip_divisor))));
    cyc(24);
    wr16(0x70020C, cyc_then(54, add16(RD16(wram_0D6F), 0x0008, p)));
    cyc(56);
    wr16(0x700216, 0x1C18);
    cyc(146);
    return cyc_then(158, GsuRun(0x07, 0xFA5A));
}

void GsuFA5AThreeShifted_cr(Carried *cr)
{
    uint8_t p = cr->p, mode = GsuFA5AThreeShifted(cr->db, &p);

    cr->a = (uint16_t)(0x1C00 | mode);
    cr->x = 0xFA5A;
    set_p(cr, nz8(p, cr->db));
}

void abi_GsuFA5AThreeShifted(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuFA5AThreeShifted_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:9533 GsuFA5ASixLeft
 * WHAT IT DOES: Runs Super FX program $07:FA5A six times, all with
 * $70:020E/0210/0212/0214 = $100, around the point ($0D6D, $0D6F): at X -
 * 8, pieces $1A0C, $1A14, $1B14, $1B0C at Y, Y + 8, Y + $10, Y + $18; then
 * at X - $10, pieces $1A0C and $1B0C at Y + 8 and Y + $10. The mirror of
 * GsuFA5ASix.
 * HOW: Word stores and GsuRun; each run reads the point again. Not reached
 * in the traces so far.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = $1B and the screen mode
 * value; X = $FA5A; N, Z from the data bank; C, V from the add of -$10.
 */
uint8_t GsuFA5ASixLeft(uint8_t bank, uint8_t *p)
{
    (void)bank;
    cyc(118);
    static const uint16_t dy[6] = {0x0000, 0x0008, 0x0010, 0x0018, 0x0008, 0x0010};
    static const uint16_t dx[6] = {0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF0, 0xFFF0};
    static const uint16_t piece[6] = {0x1A0C, 0x1A14, 0x1B14, 0x1B0C, 0x1A0C, 0x1B0C};
    uint8_t mode = 0;
    unsigned i;

    (cyc(86), fa5a_unit_scale());
    for (i = 0; i < 6; i++) {
        cyc(i ? 140 : 24);              /* after the last piece's GsuRun: its return, then on */
        uint16_t y = RD16(wram_0D6F);

        wr16(0x70020C, cyc_then(32, dy[i] ? (cyc(22), add16(y, dy[i], p)) : y));
        cyc(24);
        wr16(0x70020A, cyc_then(54, add16(RD16(wram_0D6D), dx[i], p)));
        cyc(56);
        wr16(0x700216, piece[i]);
        cyc(146);
        mode = GsuRun(0x07, 0xFA5A);
    }
    cyc(158);
    return mode;
}

void GsuFA5ASixLeft_cr(Carried *cr)
{
    uint8_t p = cr->p, mode = GsuFA5ASixLeft(cr->db, &p);

    cr->a = (uint16_t)(0x1B00 | mode);
    cr->x = 0xFA5A;
    set_p(cr, nz8(p, cr->db));
}

void abi_GsuFA5ASixLeft(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuFA5ASixLeft_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:9718 GsuFA35FromTables
 * WHAT IT DOES: Sets the renderer inputs $70:020A = $0D6D, $70:020C = $0D6F
 * + 8, $70:0210 = $70:0214 = $100 and the bytes $BF, $BC, $BD at $70:0200;
 * then, when entry $0D6B of the word table at $07:97EE is nonzero, runs
 * Super FX program $07:FA35 with $70:020E = that word, $70:0212 = the same
 * entry of the table at $07:97FA and $70:0216 = $1C00. The counterpart of
 * GsuFA1BFromTables.
 * HOW: GsuBytes200_BFBCBD, word copies, GsuRun. Not reached in the traces
 * so far.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: when the entry is 0, A = 0, X
 * = $0D6B, Z set and N clear; otherwise A = $1C and the screen mode value,
 * X = $FA35, N and Z from the data bank. C and V from the add of 8.
 */
int GsuFA35FromTables(uint8_t bank, uint8_t *mode, uint8_t *p, uint16_t *index)
{
    (void)bank;
    uint16_t x, v;

    GsuBytes200_BFBCBD();
    cyc(96);
    wr16(0x70020A, cyc_then(32, RD16(wram_0D6D)));
    cyc(24);
    wr16(0x70020C, cyc_then(54, add16(RD16(wram_0D6F), 0x0008, p)));
    cyc(56);
    wr16(0x700210, 0x0100);
    cyc(56);
    wr16(0x700214, 0x0100);
    cyc(24);
    x = RD16(wram_0D6B);
    *index = x;
    cyc(102);
    v = rd16(0x070000 | (uint16_t)(0x97EE + x));
    if (!v)
        return 0;
    wr16(0x70020E, v);
    cyc(80);
    wr16(0x700212, rd16(0x070000 | (uint16_t)(0x97FA + x)));
    cyc(56);
    wr16(0x700216, 0x1C00);
    cyc(146);
    *mode = GsuRun(0x07, 0xFA35);
    return 1;
}

void GsuFA35FromTables_cr(Carried *cr)
{
    cyc(62);
    uint16_t x;
    uint8_t p = cr->p, mode = 0;

    if (GsuFA35FromTables(cr->db, &mode, &p, &x)) {
        cyc(154);
        cr->a = (uint16_t)(0x1C00 | mode);
        cr->x = 0xFA35;
        set_p(cr, (uint8_t)(nz8(p, cr->db) & ~(P_M | P_X)));
    } else {
        cr->a = 0x0000;
        cr->x = x;
        set_p(cr, (uint8_t)((p & ~P_N) | P_Z));
    }
    cyc(4);
}

void abi_GsuFA35FromTables(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuFA35FromTables_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 07:9AB0 HudSixGlyphs
 * WHAT IT DOES: Draws six HUD glyphs with Super FX program $07:F15B: the
 * input bytes 0, $0F, $06, $0E (GsuBytes200_0F060E), $20 rows, an advance
 * of $10, starting at X $10, Y $18; the glyph words $8C10, $8C04, $8814,
 * $8408, $8C10 and $8804 in turn at $70:0216.
 * HOW: Long word stores to Super FX RAM, then GsuRun per glyph.
 * Entry: D = $0000. Exit: as the last GsuRun leaves it, with M and X 16-bit.
 */
uint8_t HudSixGlyphs(void)
{
    static const uint16_t glyph[6] = {0x8C10, 0x8C04, 0x8814, 0x8408, 0x8C10, 0x8804};
    uint8_t mode = 0;
    unsigned i;

    cyc(84);
    GsuBytes200_0F060E();
    cyc(56);
    wr16(0x700210, 0x0020);
    cyc(56);
    wr16(0x70020E, 0x0010);
    cyc(56);
    wr16(0x70020A, 0x0010);
    cyc(56);
    wr16(0x70020C, 0x0018);
    for (i = 0; i < 6; i++) {
        cyc(78);
        wr16(0x700216, glyph[i]);
        mode = f15b();
        cyc(72);                        /* GsuRun's PLB, RTL */
        cyc(44);
    }
    cyc(42);
    return mode;
}

void HudSixGlyphs_cr(Carried *cr)
{
    cr->a = (uint16_t)(0x8800 | HudSixGlyphs());
    cr->x = 0xF15B;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

void abi_HudSixGlyphs(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudSixGlyphs_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* $70:0216 = src, then Super FX program $07:FA88 (FxSprite24x40) through
   GsuRun. Returns the screen mode value. */
static uint8_t sprite24x40(uint16_t src)
{
    uint8_t mode;

    cyc(78);
    wr16(0x700216, src);
    mode = (cyc(146), GsuRun(0x07, 0xFA88));
    cyc(72);                            /* GsuRun's PLB, RTL */
    return mode;
}

/* SEP #$20, LDA $1160, CMP #n: 1 when the count is n or more (carry). */
static int count_at_least(uint8_t n)
{
    uint8_t v;

    cyc(46);
    v = RD8(wram_1160);
    if (v >= n)
        cyc(32);
    return v >= n;
}

/*
 * @routine 07:9EE9 BigSpritesCountDraw
 * WHAT IT DOES: The big sprite rows' draw when $0E5C is set: 24 x 40
 * sprites (FxSprite24x40) as many as the count $1160 allows, from (48, 16)
 * sources $A018 (always), $A000 (count 1 or more), $A518 (2), $A00C (3),
 * then from (64, 64) $AA06 (6), $B00C (7), $A00C (8), $AA12 (9); then the
 * count goes up by one, to at most 12. The sprite rows branch here (BRL)
 * and its RTS returns from them.
 * HOW: Both steps $100 and the place at $70:0212-$70:020C first; GsuRun
 * for each sprite with $70:0216 = the source; the count compared through
 * 8-bit A after each; INC A, CMP #$0C at the end.
 * Entry: any widths, D = $0000. Exit: M 8-bit, X 16-bit; X = $FA88; A low
 * byte = the new count; below 12: N, Z, C from the compare with 12, else
 * 12 with C set.
 */
static uint8_t count_high;              /* the adapters': A's high byte, the last source's */
static uint8_t count_flags;             /* N, Z, C from the compare with 12 */

uint8_t BigSpritesCountDraw(uint8_t bank)
{
    (void)bank;
    static const uint16_t near_[4] = {0xA018, 0xA000, 0xA518, 0xA00C}, far_[4] = {0xAA06, 0xB00C, 0xA00C, 0xAA12};
    static const uint8_t need_far[3] = {7, 8, 9};
    unsigned i;
    uint8_t v;

    cyc(78);
    wr16(0x700212, 0x0100);
    cyc(32);
    wr16(0x700214, 0x0100);
    cyc(56);
    wr16(0x70020A, 0x0030);
    cyc(56);
    wr16(0x70020C, 0x0010);
    for (i = 0; i < 4; i++) {
        sprite24x40(near_[i]);
        count_high = (uint8_t)(near_[i] >> 8);
        if (i < 3 && (cyc(44), !count_at_least((uint8_t)(i + 1))))
            { cyc(24); goto count; } else { cyc(6); }
    }
    count_high = 0x00;                  /* LDA #$0040 */
    cyc(116);
    wr16(0x70020A, 0x0040);
    cyc(56);
    wr16(0x70020C, 0x0040);
    if (!count_at_least(6))
        { cyc(24); goto count; } else { cyc(6); }
    for (i = 0; i < 4; i++) {
        sprite24x40(far_[i]);
        count_high = (uint8_t)(far_[i] >> 8);
        if (i < 3 && (cyc(44), !count_at_least(need_far[i])))
            goto count;
    }
cyc(6);
count:
    cyc(84);
    v = (uint8_t)(RD8(wram_1160) + 1);
    if (v >= 0x0C) {
        cyc(10);
        v = 0x0C;
        count_flags = P_C;
    } else {
        count_flags = nz8(0, (uint8_t)(v - 0x0C));
    }
    cyc(76);
    WR8(wram_1160, v);
    cyc(42);                            /* RTS */
    return v;
}

void BigSpritesCountDraw_cr(Carried *cr)
{
    uint8_t v = BigSpritesCountDraw(cr->db);

    cr->a = (uint16_t)(count_high << 8 | v);
    cr->x = 0xFA88;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_X)) | P_M | count_flags));
}

void abi_BigSpritesCountDraw(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BigSpritesCountDraw_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

static int rows_counted;                /* the adapters': the rows went to BigSpritesCountDraw */
static uint8_t rows_count;              /* its new count */
static uint8_t row_cv;                  /* C and V from BigSpritesRow6's add of the gap */

/* The common start: the renderer bytes (setter), then, with $0E5C set,
   BigSpritesCountDraw instead (a branch, so its RTS returns from the
   caller). Returns 0 when the branch was taken. */
static int big_sprites_start(uint8_t bank, void (*setter)(void))
{
    cyc(68);
    setter();
    cyc(118);                           /* SEP, LDA */
    if (RD8(wram_0E5C)) {
        cyc(46);
        rows_counted = 1;
        rows_count = BigSpritesCountDraw(bank);
        return 0;
    }
    rows_counted = 0;
    return 1;
}

/* M and X 16-bit; x, y, and both steps $100 at $70:020A-$70:0214. */
static void big_sprites_at(uint16_t x, uint16_t y)
{
    cyc(100);
    wr16(0x70020A, x);
    cyc(56);
    wr16(0x70020C, y);
    cyc(56);
    wr16(0x700212, 0x0100);
    cyc(32);
    wr16(0x700214, 0x0100);
}

/*
 * @routine 07:9D67 BigSpritesRow6
 * WHAT IT DOES: Unless $0E5C is set, draws six 24 x 40 sprites (Super FX
 * program FxSprite24x40) from (24, 40): sources $B000, $A506, $A518 and
 * $A00C, then 16 further on $B006 and $AA0C. With $0E5C set it goes to
 * 07:9EE9 instead.
 * HOW: GsuBytes200_BFB5BB first; GsuRun for each sprite; the x the
 * program leaves is read back for the gap. The jump is not reached in the
 * traces so far.
 * Entry: any widths, D = $0000. Exit: M, X 16-bit; X = $FA88; A = the
 * last source's high byte and the screen mode value; N, Z from the data
 * bank; C, V from adding the gap.
 */
uint8_t BigSpritesRow6(uint8_t bank)
{
    static const uint16_t first[4] = {0xB000, 0xA506, 0xA518, 0xA00C}, second[2] = {0xB006, 0xAA0C};
    uint16_t w, r;
    uint8_t mode = 0;
    unsigned i;

    if (!(cyc(16), big_sprites_start(bank, GsuBytes200_BFB5BB)))
        return rows_count;
    big_sprites_at(0x0018, 0x0028);
    for (i = 0; i < 4; i++)
        { sprite24x40(first[i]); if (i < 3) cyc(44); }
    cyc(76);
    w = rd16(0x70020A);
    r = (uint16_t)(w + 0x0010);
    row_cv = (uint8_t)((w > 0xFFEF ? P_C : 0) | ((~w & r & 0x8000) ? P_V : 0));
    cyc(70);
    wr16(0x70020A, r);
    for (i = 0; i < 2; i++)
        { mode = sprite24x40(second[i]); cyc(44); }
    cyc(42);
    return mode;
}

void BigSpritesRow6_cr(Carried *cr)
{
    uint8_t v = BigSpritesRow6(cr->db);

    cr->x = 0xFA88;
    if (rows_counted) {
        cr->a = (uint16_t)(count_high << 8 | v);
        set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_X)) | P_M | count_flags));
    } else {
        cr->a = (uint16_t)(0xAA00 | v);
        set_p(cr, (uint8_t)((nz8(cr->p, cr->db) & ~(P_M | P_X | P_C | P_V)) | row_cv));
    }
}

void abi_BigSpritesRow6(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BigSpritesRow6_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 07:9E3B BigSpritesRow5
 * WHAT IT DOES: Unless $0E5C is set, draws five 24 x 40 sprites
 * (FxSprite24x40) from (32, 40): sources $A006, $AA12, $A000, $AA18 and
 * $A500. With $0E5C set it goes to 07:9EE9 instead.
 * HOW: GsuBytes200_BFBCBD first; GsuRun for each sprite. The jump is not
 * reached in the traces so far.
 * Entry: any widths, D = $0000. Exit: as BigSpritesRow6, C and V as the
 * entry left them.
 */
uint8_t BigSpritesRow5(uint8_t bank)
{
    static const uint16_t src[5] = {0xA006, 0xAA12, 0xA000, 0xAA18, 0xA500};
    uint8_t mode = 0;
    unsigned i;

    if (!(cyc(16), big_sprites_start(bank, GsuBytes200_BFBCBD)))
        return rows_count;
    big_sprites_at(0x0020, 0x0028);
    for (i = 0; i < 5; i++)
        { if (i) cyc(44); mode = sprite24x40(src[i]); }
    cyc(86);
    return mode;
}

void BigSpritesRow5_cr(Carried *cr)
{
    uint8_t v = BigSpritesRow5(cr->db);

    cr->x = 0xFA88;
    if (rows_counted) {
        cr->a = (uint16_t)(count_high << 8 | v);
        set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_X)) | P_M | count_flags));
    } else {
        cr->a = (uint16_t)(0xA500 | v);
        set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
    }
}

void abi_BigSpritesRow5(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BigSpritesRow5_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}



/*
 * @routine 07:89E4 HudHoldDraw
 * WHAT IT DOES: When the HUD hold flag $0E5D is set, sets the renderer
 * bytes 0, $BF, $B3, $BE (GsuBytes200_BFB3BE) and draws the sequence at
 * $0F69 at (147, 108) (GsuF329Sequence, through HudTimesSecondRow, the
 * tail of HudTimesDraw).
 * HOW: P saved and restored; 16-bit.
 * Entry: any widths. Exit: P restored; A, X, Y as the draw leaves them
 * (A = $0E5D when not set).
 */
static uint16_t hold_a;                 /* the adapter's: A as the draw leaves it */

int HudHoldDraw(uint8_t bank)
{
    uint8_t mode;

    cyc(22);
    cyc(46);
    if (!RD16(hud_hold)) {
        cyc(44);
        cyc(42);                        /* PLP */
        hold_a = 0x0000;
        return 0;
    }
    cyc(84);
    GsuBytes200_BFB3BE();
    cyc(38);
    cyc(114);
    wr16(0x70020A, 0x0093);
    cyc(38);
    cyc(18);
    wr16(0x70020C, 0x006C);
    mode = (cyc(70), GsuF329Sequence(bank, 0x0F69));
    hold_a = (uint16_t)(f329_high(bank) | mode);
    cyc(42);                            /* PLP */
    cyc(28);
    return 1;
}

void HudHoldDraw_cr(Carried *cr)
{
    if (HudHoldDraw(cr->db)) {
        cr->x = 0xF329;
        cr->y = 0x0F69;
        if (cr->p & P_X) {
            cr->x &= 0x00FF;
            cr->y &= 0x00FF;
        }
    }
    cr->a = hold_a;
}

void abi_HudHoldDraw(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudHoldDraw_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/* SEP #$20, REP #$10, LDX #$F329, LDA #$07, JSL GsuRun, REP #$20, REP #$10:
   Super FX program $07:F329 (the glyph at $70:0216 drawn at $70:020A,
   $70:020C). */
static uint8_t run_f329(void)
{
    uint8_t mode;

    cyc(22);
    mode = (cyc(154), GsuRun(0x07, 0xF329));
    cyc(72);                            /* GsuRun's PLB, RTL */
    cyc(22);
    return mode;
}


/*
 * @routine 04:DDB0 GlyphAtY60
 * WHAT IT DOES: Draws glyph A at ($70:020A, 96) with the renderer bytes 0,
 * $BF, $B5, $BB (GsuBytes200_BFB5BB) and program $07:F329.
 * HOW: Long stores to the program's parameters, then GsuRun.
 * Entry: M 16-bit, X 16-bit; A = the glyph. Exit: M 16-bit, X 16-bit; X =
 * $F329; A as GsuRun leaves it.
 */
uint8_t GlyphAtY60(uint16_t glyph)
{
    uint8_t mode;

    cyc(32);
    wr16(0x700216, glyph);
    cyc(38);
    cyc(18);
    wr16(0x70020C, 0x0060);
    (cyc(62), GsuBytes200_BFB5BB());
    mode = (cyc(42), run_f329());
    cyc(64);
    return mode;
}

void GlyphAtY60_cr(Carried *cr)
{
    cr->a = GlyphAtY60(cr->a);
    cr->x = 0xF329;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

void abi_GlyphAtY60(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphAtY60_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* One step of the row at y 96: LDA #x, STA.L $70:020A, LDA #glyph, JSR
   GlyphAtY60. */
static uint8_t glyph_step(uint16_t x, uint16_t glyph)
{
    cyc(38);
    cyc(18);
    wr16(0x70020A, x);
    cyc(38);
    return (cyc(32), GlyphAtY60(glyph));
}

/* The row of six glyphs from 04:DD61, each entry running the rest. */
static uint8_t glyph_row(int from)
{
    static const uint16_t xs[6] = {0x0078, 0x0070, 0x0068, 0x0060, 0x0058, 0x0050};
    static const uint16_t gs[6] = {0x9808, 0x9808, 0x9808, 0x941A, 0x960E, 0x9414};
    uint8_t mode = 0;
    int i;

    for (i = from; i < 6; i++)
        mode = glyph_step(xs[i], gs[i]);
    cyc(42);
    return mode;
}

/*
 * @routine 04:DD61 GlyphRowY60From0
 * WHAT IT DOES: Draws a row of six glyphs at y 96 from x $78 down by 8:
 * $9808 three times, then $941A, $960E and $9414 (GlyphAtY60).
 * HOW: Six steps of LDA #x, STA.L $70:020A, LDA #glyph, JSR; the entries
 * at $04:DD6E, $DD7B, $DD88, $DD95 and $DDA2 start further along.
 * Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
 */
uint8_t GlyphRowY60From0(void)
{
    return glyph_row(0);
}

void GlyphRowY60From0_cr(Carried *cr)
{
    cr->a = GlyphRowY60From0();
    cr->x = 0xF329;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

void abi_GlyphRowY60From0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphRowY60From0_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:DD6E GlyphRowY60From1
 * WHAT IT DOES: As GlyphRowY60From0 from its second glyph (x $70).
 * HOW: The same code, entered further along.
 * Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
 */
uint8_t GlyphRowY60From1(void)
{
    return glyph_row(1);
}

void GlyphRowY60From1_cr(Carried *cr)
{
    cr->a = GlyphRowY60From1();
    cr->x = 0xF329;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

void abi_GlyphRowY60From1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphRowY60From1_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:DD7B GlyphRowY60From2
 * WHAT IT DOES: As GlyphRowY60From0 from its third glyph (x $68).
 * HOW: The same code, entered further along.
 * Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
 */
uint8_t GlyphRowY60From2(void)
{
    return glyph_row(2);
}

void GlyphRowY60From2_cr(Carried *cr)
{
    cr->a = GlyphRowY60From2();
    cr->x = 0xF329;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

void abi_GlyphRowY60From2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphRowY60From2_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:DD88 GlyphRowY60From3
 * WHAT IT DOES: As GlyphRowY60From0 from its fourth glyph (x $60).
 * HOW: The same code, entered further along.
 * Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
 */
uint8_t GlyphRowY60From3(void)
{
    return glyph_row(3);
}

void GlyphRowY60From3_cr(Carried *cr)
{
    cr->a = GlyphRowY60From3();
    cr->x = 0xF329;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

void abi_GlyphRowY60From3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphRowY60From3_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:DD95 GlyphRowY60From4
 * WHAT IT DOES: As GlyphRowY60From0 from its fifth glyph (x $58).
 * HOW: The same code, entered further along.
 * Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
 */
uint8_t GlyphRowY60From4(void)
{
    return glyph_row(4);
}

void GlyphRowY60From4_cr(Carried *cr)
{
    cr->a = GlyphRowY60From4();
    cr->x = 0xF329;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

void abi_GlyphRowY60From4(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphRowY60From4_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:DDA2 GlyphRowY60From5
 * WHAT IT DOES: Draws the row's last glyph, $9414 at x $50 (GlyphAtY60).
 * HOW: The same code, entered at its last step.
 * Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
 */
uint8_t GlyphRowY60From5(void)
{
    return glyph_row(5);
}

void GlyphRowY60From5_cr(Carried *cr)
{
    cr->a = GlyphRowY60From5();
    cr->x = 0xF329;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

void abi_GlyphRowY60From5(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphRowY60From5_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:DDD1 GlyphListDraw
 * WHAT IT DOES: Draws the glyph list: for each of the $10E6 entries (2
 * bytes apart) whose word at $110E is set, glyph $10E8 at x and y the bytes
 * at $1134 and $1135, the first $10E4 entries with the renderer bytes 0,
 * $BF, $BC, $BD and the rest with 0, $BF, $B3, $BE; program $07:F329.
 * HOW: The limits doubled into direct page $F1 and $F3; X and P kept on the
 * stack around GsuRun.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: M 16-bit, X 16-bit; X = the
 * end; N, Z, C from its compare.
 */
static uint16_t list_a;                 /* the adapter's: A as the last entry leaves it */
static uint8_t list_flags;              /* N, Z, C from the last compare */

uint16_t GlyphListDraw(uint8_t bank)
{
    (void)bank;
    uint16_t x = 0x0000, v, lim;

    cyc(48);
    v = (uint16_t)(RD16(glyph_list_first) << 1);
    cyc(30);
    wr16(W(recip_divisor), v);
    cyc(24);
    v = (uint16_t)(RD16(glyph_list_count) << 1);
    cyc(30);
    wr16(W(recip_dividend), v);
    do {
        cyc(22);                        /* REP #$20 */
        cyc(30);
        list_a = RD16I(glyph_list_on, x);
        if (list_a) {
            cyc(46);
            v = RD16I(glyph_list_glyph, x);
            cyc(32);
            wr16(0x700216, v);
            cyc(30);
            v = (uint16_t)(RD16I(glyph_list_x, x) & 0x00FF);
            cyc(2);
            cyc(54);
            wr16(0x70020A, v);
            cyc(30);
            v = (uint16_t)(RD16I(glyph_list_y, x) & 0x00FF);
            cyc(2);
            cyc(54);
            wr16(0x70020C, v);
            cyc(16);
            lim = rd16(W(recip_divisor));
            if (x >= lim)
                { cyc(84); GsuBytes200_BFB3BE(); }
            else
                { cyc(78); GsuBytes200_BFBCBD(); cyc(22); }
            cyc(94);
            list_a = run_f329();
            cyc(42);                    /* PLP */
            /* the second REP, PLX, INX, INX, CPX (GsuRun states its PLP and RTL) */
            cyc(88);
        } else { cyc(66); }
        x = (uint16_t)(x + 2);
        lim = rd16(W(recip_dividend));
        if (x >= lim)
            cyc(58);
    } while (cyc_loop(22, x < lim));
    list_flags = (uint8_t)(nz16(0, (uint16_t)(x - lim)) | P_C);
    return x;
}

void GlyphListDraw_cr(Carried *cr)
{
    cr->x = GlyphListDraw(cr->db);
    cr->a = list_a;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_M)) | list_flags));
}

void abi_GlyphListDraw(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphListDraw_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:A426 GlyphSpriteDraw
 * WHAT IT DOES: Draws glyph sprite X/2: glyph $1010 + X at ($0FB0 + X,
 * $0FE0 + X) with program $07:F329, then steps X by 2.
 * HOW: Long stores to the program's parameters; X kept on the stack around
 * GsuRun.
 * Entry: M 16-bit, X 16-bit; X = the sprite * 2. Exit: X + 2; M 16-bit, X
 * 16-bit; N, Z from X; A as GsuRun leaves it.
 */
/* GlyphSpriteDraw from its first store: sprite X/2 drawn at x = A. */
static uint16_t sprite_glyph;           /* the adapters': the glyph word drawn */

static uint8_t sprite_at(uint16_t x, uint16_t at)
{
    uint16_t v;

    cyc(32);
    wr16(0x70020A, at);
    cyc(30);
    v = RD16I(glyph_spr_y, x);
    cyc(32);
    wr16(0x70020C, v);
    cyc(30);
    sprite_glyph = RD16I(glyph_spr_glyph, x);
    cyc(32);
    wr16(0x700216, sprite_glyph);
    return run_f329();
}


uint8_t GlyphSpriteDraw(uint8_t bank, uint16_t x)
{
    (void)bank;
    cyc(30);
    return sprite_at(x, RD16I(glyph_spr_x, x));
}

void GlyphSpriteDraw_cr(Carried *cr)
{
    uint8_t mode = GlyphSpriteDraw(cr->db, cr->x);

    cr->a = (uint16_t)((sprite_glyph & 0xFF00) | mode);
    cr->x = (uint16_t)(cr->x + 2);
    set_p(cr, nz16((uint8_t)(cr->p & ~(P_M | P_X)), cr->x));
}

void abi_GlyphSpriteDraw(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphSpriteDraw_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 04:DA3C GlyphSpritesInit
 * WHAT IT DOES: Sets the 16 glyph sprites up: x from the table at
 * $04:DA7A, y 8, glyph from the table at $04:DA8E, $1040 + X = 0; then
 * $10D8 = 0, $115A = $1E, and $0D5B and the HUD hold $0E5D cleared.
 * HOW: A 16-bit loop over X = 0 to $1E by 2; P saved and restored.
 * Entry: any widths. Exit: P restored; A = $001E; X = $20; Y unchanged.
 */
void GlyphSpritesInit(uint8_t bank)
{
    (void)bank;
    uint16_t x = 0x0000;

    cyc(22);
    do {
        cyc(124);
        WR16I(glyph_spr_x, x, rd16(0x04DA7A + (uint32_t)x));
        cyc(38);
        cyc(16);
        WR16I(glyph_spr_y, x, 0x0008);
        cyc(78);
        WR16I(glyph_spr_glyph, x, rd16(0x04DA8E + (uint32_t)x));
        cyc(30);
        WR16I(glyph_spr_1040, x, 0x0000);
        x = (uint16_t)(x + 2);
    } while (cyc_loop(28, x < 0x0020));
    cyc(92);
    WR16(wram_10D8, 0x0000);
    cyc(38);
    cyc(10);
    WR16(wram_115A, 0x001E);
    cyc(46);
    WR8(wram_0D5B, 0x00);
    cyc(24);
    WR8(hud_hold, 0x00);
    cyc(42);                            /* PLP */
    cyc(30);
}

void GlyphSpritesInit_cr(Carried *cr)
{
    GlyphSpritesInit(cr->db);
    cr->a = 0x001E;
    cr->x = 0x0020;
}

void abi_GlyphSpritesInit(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphSpritesInit_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* One glyph of the time row: the glyph word, on (+$110E = 0, 16-bit), x,
   y $68, then the next entry (not after the last). */
static uint16_t time_glyph(uint16_t y, uint16_t glyph, uint8_t x, int last)
{
    cyc(22);
    cyc(70);
    WR16I(glyph_list_glyph, y, glyph);
    cyc(38);
    cyc(16);
    WR16I(glyph_list_on, y, 0x0000);
    cyc(68);
    WR8I(glyph_list_x, y, x);
    cyc(46);
    WR8I(glyph_list_y, y, 0x68);
    if (!last)
        y = (uint16_t)(y + 2); else { cyc(30); }
    cyc(12);
    return y;
}

/* A digit glyph: $9400 + digit * 2, the digit read from direct page. */
static uint16_t digit_glyph(uint16_t addr, int high)
{
    uint16_t v;

    cyc(32);
    v = rd16(addr);
    if (high)
        cyc(56);
    cyc(2);
    v = high ? (uint16_t)((v & 0x00FF) >> 4) : (uint16_t)(v & 0x000F);
    cyc(12);
    return (uint16_t)((v << 1) + 0x9400);
}

/*
 * @routine 04:DE2C GlyphListMessageAndTime
 * WHAT IT DOES: Fills the glyph list with message X/2 (the string its word
 * in the table at $04:DF6D points to: a length byte, then a glyph word per
 * letter, 0 for a space) centred on x $68 at y $58, then the time row at y
 * $68 from x $4C by 8: the digits of $F7 (low), $F6 and $F5 with the
 * separator glyphs $980A and $980C; $10E4 = the letters - 1 and $10E6 =
 * that + 7.
 * HOW: The string through the data bank (bank $04); the letter count in
 * direct page $F1 and the x in $F3; spaces take a place but no entry. The
 * letters' on-flags are cleared as bytes, the time's as words.
 * Entry: M 16-bit, X 16-bit, D = $0000, DB = $04; X = the message * 2.
 * Exit: M 8-bit; Y = the last entry; A low byte = $68; N, Z from it.
 */
static uint16_t message_end;            /* the adapters': X at the end, past the string */

uint16_t GlyphListMessageAndTime(uint8_t bank, uint16_t message)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t x, y, v;

    cyc(114);
    x = rd16(b | (uint16_t)(0xDF6D + message));
    cyc(30);                            /* TAX */
    v = (uint16_t)(rd16(b | x) & 0x00FF);
    cyc(2);
    wr16(W(recip_divisor), v);
    v = (uint16_t)(v << 2);
    cyc(6);                             /* the SBC */
    v = (uint16_t)(0x0068 - v);         /* SBC #$68, negated */
    cyc(114);
    wr16(W(recip_dividend), v);
    cyc(16);
    v = (uint16_t)(rd16(W(recip_divisor)) - 1);
    cyc(38);
    WR16(glyph_list_first, v);
    cyc(62);
    WR16(glyph_list_count, (uint16_t)(v + 7));
    y = 0x0000;
    cyc(2);
    do {
        cyc(62);
        v = rd16(b | (uint16_t)(0x0001 + x));
        if (v) {
            cyc(52);
            WR16I(glyph_list_glyph, y, v);
            cyc(38);
            v = rd8(W(recip_dividend));
            cyc(30);
            WR8I(glyph_list_x, y, (uint8_t)v);
            cyc(46);
            WR8I(glyph_list_y, y, 0x58);
            cyc(46);
            WR8I(glyph_list_on, y, 0x00);
            y = (uint16_t)(y + 2);
        }
        cyc(22);
        cyc(44);
        v = (uint16_t)(rd16(W(recip_dividend)) + 8);
        cyc(54);
        wr16(W(recip_dividend), v);
        x = (uint16_t)(x + 2);
        cyc(44);
        v = (uint16_t)(rd16(W(recip_divisor)) - 1);
        cyc(14);
        wr16(W(recip_divisor), v);
    } while (v);
    message_end = x;
    cyc(22);
    y = time_glyph(y, digit_glyph(W(glyph_time_low), 0), 0x4C, 0);
    y = time_glyph(y, 0x980A, 0x54, 0);
    cyc(22);
    y = time_glyph(y, digit_glyph(W(glyph_time_mid), 1), 0x5C, 0);
    cyc(22);
    y = time_glyph(y, digit_glyph(W(glyph_time_mid), 0), 0x64, 0);
    y = time_glyph(y, 0x980C, 0x6C, 0);
    cyc(22);
    y = time_glyph(y, digit_glyph(W(glyph_time_high), 1), 0x74, 0);
    cyc(22);
    return time_glyph(y, digit_glyph(W(glyph_time_high), 0), 0x7C, 1);
}

void GlyphListMessageAndTime_cr(Carried *cr)
{
    cr->y = GlyphListMessageAndTime(cr->db, cr->x);
    cr->x = message_end;
    cr->a = 0x0068;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_V)) | P_M));
}

void abi_GlyphListMessageAndTime(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphListMessageAndTime_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_GsuBytes200_BFB7B6_D8C4(void);
void abi_GsuBytes200_BFBAB9_D8E1(void);
void abi_GsuBytes200_BFB3BE_D8FE(void);
void abi_GsuBytes200_BFBBBE(void);

/* LDA #glyph, STA.L $70:0216, then Super FX program $07:prog (SEP #$20,
   REP #$10, LDX #prog, LDA #$07, JSL GsuRun, REP #$20, REP #$10). */
static uint8_t gprog(uint16_t prog, uint16_t glyph)
{
    uint8_t mode;

    cyc(38);
    cyc(18);
    wr16(0x700216, glyph);
    cyc(22);
    mode = (cyc(124), GsuRun(0x07, prog));
    cyc(72);                            /* GsuRun's PLB, RTL */
    cyc(22);
    cyc(22);
    return mode;
}


/* LDA.L $70:020A or $020C, CLC/SEC, ADC/SBC #d, STA.L: move the draw
   position. */
static uint8_t pos_cv;                  /* C and V of the last move */

static uint16_t pos_add(uint32_t at, uint16_t d, int sub)
{
    uint16_t a, r;

    cyc(32);
    a = rd16(at);
    if (sub) {
        r = (uint16_t)(a - d);
        cyc(6);                         /* the SBC */
        pos_cv = (uint8_t)((a >= d ? P_C : 0) | (((a ^ d) & (a ^ r) & 0x8000) ? P_V : 0));
    } else {
        cyc(6);
        r = (uint16_t)(a + d);
        pos_cv = (uint8_t)(((unsigned)a + d > 0xFFFF ? P_C : 0) | ((~(a ^ d) & (a ^ r) & 0x8000) ? P_V : 0));
    }
    cyc(64);
    wr16(at, r);
    return r;
}


/*
 * @routine 07:A03F GlyphWordAt1C28
 * WHAT IT DOES: Draws a nine-glyph word with program $07:F15B at (28, 40),
 * 32 rows, advancing 16, with the renderer bytes 0, $BF, $BC, $BD: glyphs
 * $840C, $8C00, $881C, $8C18, $8C10, then 4 further on $8C14, $8C0C, $8408
 * and $9004.
 * HOW: Long stores to the program's parameters; GsuRun per glyph.
 * Entry: any widths. Exit: M 16-bit, X 16-bit; X = $F15B; A as GsuRun
 * leaves it.
 */
uint8_t GlyphWordAt1C28(void)
{
    static const uint16_t g1[5] = {0x840C, 0x8C00, 0x881C, 0x8C18, 0x8C10};
    static const uint16_t g2[4] = {0x8C14, 0x8C0C, 0x8408, 0x9004};
    uint8_t mode = 0;
    int i;

    cyc(22);
    cyc(38);
    cyc(64);
    wr16(0x70020A, 0x001C);
    cyc(38);
    cyc(18);
    wr16(0x70020C, 0x0028);
    cyc(38);
    cyc(18);
    wr16(0x700210, 0x0020);
    cyc(38);
    cyc(18);
    wr16(0x70020E, 0x0010);
    (cyc(62), GsuBytes200_BFBCBD());
    cyc(22);
    for (i = 0; i < 5; i++) {
        if (!i)
            cyc(50);
        cyc(22);
        gprog(0xF15B, g1[i]);
    }
    pos_add(0x70020A, 0x0004, 0);
    for (i = 0; i < 4; i++) {
        cyc(22);
        mode = gprog(0xF15B, g2[i]);
    }
    cyc(42);
    return mode;
}

void GlyphWordAt1C28_cr(Carried *cr)
{
    cr->a = (uint16_t)(0x9000 | GlyphWordAt1C28());
    cr->x = 0xF15B;
    set_p(cr, (uint8_t)((nz8(cr->p, cr->db) & ~(P_M | P_X | P_C | P_V)) | pos_cv));
}

void abi_GlyphWordAt1C28(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphWordAt1C28_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/* Three rows of three 8 x 8 glyphs (program $07:F280), each row 8 lower
   and back 24; the glyphs are row byte * $100 + $1A, $1C, $1E. `mid` (when
   not 0) sets the renderer bytes before the first row's third glyph,
   `row2` after the step to the second row. */
static uint8_t glyph_grid3(uint16_t first, uint8_t (*mid)(void), uint8_t (*row2)(void))
{
    uint8_t mode = 0;
    int r, c;

    for (r = 0; r < 3; r++) {
        if (r) {
            pos_add(0x70020C, 0x0008, 0);
            pos_add(0x70020A, 0x0018, 1);
            if (r == 1 && row2)
                { cyc(62); row2(); }
        }
        for (c = 0; c < 3; c++) {
            if (r == 0 && c == 2 && mid)
                { cyc(62); mid(); }
            mode = gprog(0xF280, (uint16_t)((first + r) << 8 | (0x1A + 2 * c)));
        }
        if (r == 2)
            cyc(42);
    }
    return mode;
}

/*
 * @routine 07:A158 GlyphGrid1A
 * WHAT IT DOES: Draws the 3 x 3 glyph picture $1A1A-$1C1E at (56, 48) with
 * program $07:F280 (8 x 8 glyphs): its first two glyphs with the renderer
 * bytes 0, $BF, $B7, $B6, the third with 0, $BF, $BA, $B9 and the rest
 * with 0, $BF, $B3, $BE.
 * HOW: Each row of three, then 8 down and 24 back; the renderer setters'
 * long entries ($04:D8C4, $D8E1, $D8FE).
 * Entry: any widths. Exit: M 16-bit, X 16-bit; X = $F280; A as GsuRun
 * leaves it.
 */
uint8_t GlyphGrid1A(void)
{
    cyc(22);
    cyc(38);
    cyc(18);
    wr16(0x70020A, 0x0038);
    cyc(38);
    cyc(18);
    wr16(0x70020C, 0x0030);
    (cyc(62), GsuBytes200_BFB7B6_D8C4());
    return glyph_grid3(0x1A, GsuBytes200_BFBAB9_D8E1, GsuBytes200_BFB3BE_D8FE);
}

void GlyphGrid1A_cr(Carried *cr)
{
    cr->a = (uint16_t)(0x1C00 | GlyphGrid1A());
    cr->x = 0xF280;
    set_p(cr, (uint8_t)((nz8(cr->p, cr->db) & ~(P_M | P_X | P_C | P_V)) | pos_cv));
}

void abi_GlyphGrid1A(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphGrid1A_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:A27D GlyphGrid1D
 * WHAT IT DOES: Draws the 3 x 3 glyph picture $1D1A-$1F1E at (56, 72) with
 * program $07:F280, the renderer bytes 0, $BF, $BB, $BE.
 * HOW: As GlyphGrid1A, one setter first.
 * Entry: any widths. Exit: as GlyphGrid1A.
 */
uint8_t GlyphGrid1D(void)
{
    cyc(22);
    cyc(38);
    cyc(18);
    wr16(0x70020A, 0x0038);
    cyc(38);
    cyc(18);
    wr16(0x70020C, 0x0048);
    (cyc(62), GsuBytes200_BFBBBE());
    return glyph_grid3(0x1D, 0, 0);
}

void GlyphGrid1D_cr(Carried *cr)
{
    cr->a = (uint16_t)(0x1F00 | GlyphGrid1D());
    cr->x = 0xF280;
    set_p(cr, (uint8_t)((nz8(cr->p, cr->db) & ~(P_M | P_X | P_C | P_V)) | pos_cv));
}

void abi_GlyphGrid1D(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphGrid1D_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void SoundPort1QueuePush(uint8_t command);

/* GlyphTimesSequence's exit, for the adapters: whose registers it leaves
   (its own: A, X, N and Z from the values here; or the message's, the
   list draw's or a glyph row's, from their own statics). */
enum { SEQ_OWN, SEQ_MESSAGE, SEQ_LIST, SEQ_ROW };
static int seq_exit;
static uint16_t seq_a, seq_x, seq_nz, seq_y, seq_end;
/* What ChampionshipFinish is entered with that the C does not carry: Y
   (when GlyphSpritesGrowAndTimes changed it) and V. */
static int times_y_valid;
static uint16_t times_y;
static uint8_t times_v;

/* DEC $115A (16-bit); at 0, $115A = reload (unless 0) and the step $10D8
   moves on; 1 when it ran out. */
static int glyph_wait(uint16_t reload)
{
    uint16_t v = (uint16_t)(RD16(wram_115A) - 1);

    cyc(14);
    WR16(wram_115A, v);
    seq_nz = v;
    if (v)
        { cyc(22); return 0; }
    if (reload) {
        cyc(38);
        cyc(26);
        WR16(wram_115A, reload);
        seq_a = reload;
    } else { cyc(16); }
    cyc(24);
    v = (uint16_t)(RD16(wram_10D8) + 1);
    cyc(14);
    WR16(wram_10D8, v);
    seq_nz = v;
    return 1;
}

/* INC $10D8 (16-bit). */
static void glyph_next(void)
{
    cyc(24);
    uint16_t v = (uint16_t)(RD16(wram_10D8) + 1);

    cyc(14);
    WR16(wram_10D8, v);
    seq_nz = v;
}

/* A time's three bytes to direct page $F5-$F7, $115C = 0, the next step,
   then message $0D4A * 8 + extra (GlyphListMessageAndTime). */
static void glyph_load_time(uint32_t b, uint16_t from, uint16_t extra)
{
    uint16_t v;
    int i;

    for (i = 0; i < 3; i++) {
        cyc(22);
        v = rd8(b | (uint16_t)(from + i));
        cyc(16);
        wr8((uint32_t)(0xF5 + i), (uint8_t)v);
        cyc(2);
    }
    cyc(22);
    cyc(22);
    WR16(wram_115C, 0x0000);
    glyph_next();
    cyc(24);
    v = (uint16_t)(RD16(wram_0D4A) & 0x00FF);
    cyc(2);
    v = (uint16_t)(v << 3);
    if (extra)
        { cyc(38); v = (uint16_t)(v + extra); }
    cyc(94);
    cyc(30);                            /* TAX */
    seq_y = GlyphListMessageAndTime((uint8_t)(b >> 16), v);
    seq_end = message_end;
    seq_exit = SEQ_MESSAGE;
}

/* One more of the list's entries gets on-flag `flag` ($115C the entry * 2);
   after the last, the next step and $115A = reload. Then the list is
   drawn. */
static void glyph_flag_step(uint32_t b, uint16_t flag, uint16_t reload)
{
    uint16_t v;

    v = RD16(wram_115C);
    cyc(30);                            /* TAX */
    cyc(38);
    WR16I(glyph_list_on, v, flag);
    cyc(24);
    v = (uint16_t)(RD16(wram_115C) + 1);
    cyc(14);
    WR16(wram_115C, v);
    cyc(24);
    v = (uint16_t)(RD16(wram_115C) + 1);
    cyc(14);
    WR16(wram_115C, v);
    cyc(24);
    v = (uint16_t)(RD16(wram_115C) >> 1);
    cyc(38);
    if (v >= RD16(glyph_list_count)) {
        cyc(16);
        glyph_next();
        cyc(38);
        cyc(10);
        WR16(wram_115A, reload);
    } else { cyc(22); }
    cyc(46);
    seq_end = GlyphListDraw((uint8_t)(b >> 16));
    seq_exit = SEQ_LIST;
}

void abi_ChampionshipFinish(void);

/*
 * @routine 04:DB87 GlyphTimesSequence
 * WHAT IT DOES: One frame of the three best times' display, by its step
 * $10D8 (the table at $04:DB94): two waits; then for each of the course
 * $0D4A's three times ($0EF9, $0EFC, $0EFF): its message and time as a
 * glyph list (GlyphListMessageAndTime), shown a glyph a frame
 * (GlyphListDraw with the entries' flags set to $FF one by one), a wait of
 * 10, then cleared a glyph a frame, a wait of 4; then the row of six glyphs
 * at y 96 drawn from its last glyph back to its first a step at a time
 * with waits of 4 and 8; at the end the split window on ($19D8 = 1, $19DF
 * cleared) with sound $04 on port 1, and after $14 frames the screen at
 * $07:EE26 (with data bank $07).
 * HOW: JMP (table,X) on the step; $115A counts frames, $115C the entry.
 * Entry: any widths, D = $0000. Exit: M 16-bit, X 16-bit; A, X, Y as the
 * step leaves them.
 */
void GlyphTimesSequence(Carried *cr, uint8_t bank)
{
    static uint8_t (*const row[6])(void) = {GlyphRowY60From0, GlyphRowY60From1, GlyphRowY60From2,
                                             GlyphRowY60From3, GlyphRowY60From4, GlyphRowY60From5};
    uint32_t b = (uint32_t)bank << 16;
    uint16_t step, v;

    cyc(22);
    cyc(116);
    v = (uint16_t)(RD16(wram_10D8) & 0x00FF);
    cyc(2);
    seq_a = seq_x = (uint16_t)(v << 1);
    cyc(30);                            /* TAX */
    step = v;
    seq_exit = SEQ_OWN;
    cyc(90);
    rd16(0x040000 | (uint16_t)(0xDB94 + seq_x));
    switch (step) {
    case 0:
        glyph_wait(0x001E);
        break;
    case 1:
        glyph_wait(0);
        break;
    case 2:
        glyph_load_time(b, W(wram_0EF9), 0);
        break;
    case 6:
        glyph_load_time(b, W(wram_0EFC), 2);
        break;
    case 10:
        glyph_load_time(b, W(wram_0EFF), 4);
        break;
    case 3: case 7: case 11:
        glyph_flag_step(b, 0x00FF, 0x000A);
        break;
    case 4: case 8: case 12:
        v = (uint16_t)(RD16(wram_115A) - 1);
        cyc(14);
        WR16(wram_115A, v);
        if (!v) {
            cyc(40);
            WR16(wram_115C, 0x0000);
            glyph_next();
        } else { cyc(22); }
        cyc(46);
        seq_end = GlyphListDraw(bank);
        seq_exit = SEQ_LIST;
        break;
    case 5: case 9: case 13:
        glyph_flag_step(b, 0x0000, 0x0004);
        break;
    case 14:
        cyc(24);
        seq_a = 0x0004;
        WR16(wram_115A, 0x0004);
        glyph_next();
        break;
    case 15: case 16: case 17: case 18: case 19: case 20: {
        static const uint16_t reload[6] = {0x0004, 0x0004, 0x0008, 0x0008, 0x0008, 0x0004};
        glyph_wait(reload[step - 15]);
        seq_a = (cyc(46), row[5 - (step - 15)]());
        seq_exit = SEQ_ROW;
        break;
    }
    case 21:
        if (glyph_wait(0x0014)) {
            cyc(62);
            WR8(window_hdma_off, 0x01);
            cyc(24);
            WR8(wram_19DF, 0x00);
            (cyc(78), SoundPort1QueuePush(0x04));
            cyc(22);
        }
        cyc(46);
        seq_a = GlyphRowY60From0();
        seq_exit = SEQ_ROW;
        break;
    default:
        v = (uint16_t)(RD16(wram_115A) - 1);
        cyc(14);
        WR16(wram_115A, v);
        if (!v) {
            cyc(136);
            /* ChampionshipFinish is still in registers: A low byte 7, X the
               step * 2, M 8-bit, X 16-bit, N, Z, C clear, data bank 7. */
            cr->a = (uint16_t)((seq_x & 0xFF00) | 0x07);
            cr->x = seq_x;
            if (times_y_valid)
                cr->y = times_y;
            set_p(cr, (uint8_t)((cr->p & (P_D | P_I)) | P_M | times_v));
            cr->db = 0x07;
            ChampionshipFinish(cr);
            return;
        } else { cyc(68); }
        cyc(22);
        seq_a = GlyphRowY60From0();
        seq_exit = SEQ_ROW;
        break;
    }
    cyc(42);
}

/* The registers GlyphTimesSequence leaves, from what it recorded. */
static void glyph_times_exit(Carried *cr)
{
    switch (seq_exit) {
    case SEQ_OWN:
        cr->a = seq_a;
        cr->x = seq_x;
        set_p(cr, nz16((uint8_t)(cr->p & ~(P_M | P_X | P_C)), seq_nz));
        break;
    case SEQ_MESSAGE:
        cr->a = 0x0068;
        cr->x = seq_end;
        cr->y = seq_y;
        set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_V | P_X)) | P_M));
        break;
    case SEQ_LIST:
        cr->a = list_a;
        cr->x = seq_end;
        set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_M | P_X)) | list_flags));
        break;
    default:
        cr->a = seq_a;
        cr->x = 0xF329;
        set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X | P_C)));
        break;
    }
}

void GlyphTimesSequence_cr(Carried *cr)
{
    times_y_valid = 0;
    times_v = (uint8_t)(cr->p & P_V);
    GlyphTimesSequence(cr, cr->db);
    glyph_times_exit(cr);
}

void abi_GlyphTimesSequence(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphTimesSequence_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* SEP #$20, REP #$10, LDX #prog, LDA #$07, JSL GsuRun, REP #$20, REP #$10. */
static uint8_t gprog_run(uint16_t prog)
{
    uint8_t mode;

    cyc(22);
    mode = (cyc(124), GsuRun(0x07, prog));
    cyc(72);                            /* GsuRun's PLB, RTL */
    cyc(22);
    cyc(22);
    return mode;
}


/*
 * @routine 04:DAE2 GlyphSpritesGrowAndTimes
 * WHAT IT DOES: Once the best times' display has started ($10D8 not 0):
 * each glyph sprite's size step $1040 (0-7) grows by one once the sprite
 * before it has reached 3 (the first always), and every sprite with a
 * step is drawn scaled (program $07:FAF2, the renderer bytes 0, $BF, $B7,
 * $B6) at its x and y plus the step's offsets, with the step's size and
 * scale (tables at $04:DAA2, $DAB2, $DAC2, $DAD2). Then GlyphTimesSequence.
 * HOW: Data bank $04 (the program's) for the tables; P and the data bank
 * saved and restored.
 * Entry: any widths, D = $0000. Exit: P and the data bank restored; A, X,
 * Y as GlyphTimesSequence leaves them.
 */
void GlyphSpritesGrowAndTimes(Carried *cr)
{
    uint32_t b = 0x040000;
    uint16_t x, v, s;

    cyc(22);
    cyc(140);
    if (RD16(wram_10D8)) {
        x = 0x0002;
        do {
            cyc(76);
            if (rd16(0x7E0000 | (uint16_t)(W(glyph_spr_1040) - 2 + x)) >= 0x0003) {
                cyc(70);
                v = RD16I(glyph_spr_1040, x);
                if (v < 0x0007) {
                    cyc(84);
                    WR16I(glyph_spr_1040, x, (uint16_t)(v + 1));
                } else { cyc(46); }
            } else { cyc(46); }
            cyc(28);
            x = (uint16_t)(x + 2);
        } while (x < 0x0014);
        cyc(64);
        if (RD16(glyph_spr_1040) < 0x0007) {
            cyc(64);
            v = (uint16_t)(RD16(glyph_spr_1040) + 1);
            cyc(14);
            WR16(glyph_spr_1040, v);
        } else { cyc(46); }
        cyc(62);
        GsuBytes200_BFB7B6();
        x = 0x0000;
        cyc(44);
        do {
            cyc(82);
            s = RD16I(glyph_spr_1040, x);
            if (s) {
                s = (uint16_t)(s << 1);
                times_y = s;
                times_y_valid = 1;
                cyc(122);
                v = rd16(b | (uint16_t)(0xDAA2 + s));
                wr16(0x70020E, v);
                cyc(32);
                wr16(0x700210, v);
                cyc(78);
                v = rd16(b | (uint16_t)(0xDAB2 + s));
                wr16(0x700212, v);
                cyc(32);
                wr16(0x700214, v);
                cyc(30);
                v = RD16I(glyph_spr_x, x);
                cyc(92);
                v = (uint16_t)(v + rd16(b | (uint16_t)(0xDAC2 + s)));
                wr16(0x70020A, v);
                cyc(38);
                cyc(18);
                wr16(0x70020C, 0x0006);
                cyc(30);
                v = RD16I(glyph_spr_y, x);
                cyc(92);
                {
                    uint16_t m = rd16(b | (uint16_t)(0xDAD2 + s)), r = (uint16_t)(v + m);

                    times_v = (~(v ^ m) & (v ^ r) & 0x8000) ? P_V : 0;
                    v = r;
                }
                wr16(0x70020C, v);
                cyc(30);
                v = RD16I(glyph_spr_glyph, x);
                cyc(32);
                wr16(0x700216, v);
                cyc(52);
                gprog_run(0xFAF2);
                cyc(42);                /* PLP */
            }
            cyc(44);
            x = (uint16_t)(x + 2);
        } while (x != 0x0020);
    }
    GlyphTimesSequence(cr, 0x04);
    cyc(42);                            /* PLP */
    cyc(122);
}

void GlyphSpritesGrowAndTimes_cr(Carried *cr)
{
    uint8_t p0 = cr->p;

    times_y_valid = 0;
    times_v = (uint8_t)(cr->p & P_V);
    GlyphSpritesGrowAndTimes(cr);
    if (times_y_valid)
        cr->y = times_y;
    glyph_times_exit(cr);
    set_p(cr, p0);
    if (p0 & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_GlyphSpritesGrowAndTimes(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphSpritesGrowAndTimes_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_GsuBytes200_BFBAB9(void);

/*
 * @routine 07:9814 GlyphSlideThree
 * WHAT IT DOES: Slides the first three glyph sprites left: each one's x
 * goes down by its step $1040 but not below its target $1070 (nor past
 * 0), and it is drawn at y $50 with program $07:F3D2, the renderer bytes
 * by its glyph: 0, $BF, $B7, $B6 for $8000, $BC, $BD for $8006, $BA, $B9
 * for $800C, $B3, $BE for the others.
 * HOW: 16-bit; X and P kept on the stack around GsuRun; P saved and
 * restored.
 * Entry: any widths. Exit: P restored; X = 6; A as the last draw left it.
 */
static uint16_t slide_glyph;            /* the adapter's: the last glyph word */

uint8_t GlyphSlideThree(uint8_t bank)
{
    (void)bank;
    uint16_t x = 0x0000, a, m;
    uint8_t mode = 0;

    cyc(22);
    do {
        cyc(76);
        a = RD16I(glyph_spr_x, x);
        cyc(44);
        m = RD16I(glyph_spr_1040, x);
        cyc(6);                         /* the SBC */
        if (a >= m) {
            a = (uint16_t)(a - m);
            cyc(40);
            if (a < RD16I(glyph_spr_1040, 48 + x))
                { cyc(46); a = RD16I(glyph_spr_1040, 48 + x); } else { cyc(22); }
        } else {
            cyc(46);
            a = RD16I(glyph_spr_1040, 48 + x);
        }
        cyc(30);
        WR16I(glyph_spr_x, x, a);
        cyc(32);
        wr16(0x70020A, a);
        cyc(38);
        cyc(18);
        wr16(0x70020C, 0x0050);
        cyc(30);
        slide_glyph = RD16I(glyph_spr_glyph, x);
        cyc(32);
        wr16(0x700216, slide_glyph);
        if (slide_glyph == 0x8000)
            { cyc(108); GsuBytes200_BFB7B6(); cyc(124); }
        else if (slide_glyph == 0x8006)
            { cyc(148); GsuBytes200_BFBCBD(); cyc(22); cyc(124); }
        else if (slide_glyph == 0x800C)
            { cyc(188); GsuBytes200_BFBAB9(); cyc(22 + 124 - 72); }  /* the setter states 72 of these */
        else
            { cyc(182); GsuBytes200_BFB3BE(); cyc(22); cyc(124); }
        mode = gprog_run(0xF3D2);
        cyc(42);                        /* PLP */
        x = (uint16_t)(x + 2);
        cyc(x >= 0x0006 ? 118 : 50);
    } while (x < 0x0006);
    cyc(42);                            /* PLP */
    return mode;
}

void GlyphSlideThree_cr(Carried *cr)
{
    uint8_t mode = GlyphSlideThree(cr->db);

    cr->a = (uint16_t)((slide_glyph & 0xFF00) | mode);
    cr->x = 0x0006;
}

void abi_GlyphSlideThree(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphSlideThree_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


void abi_GsuBytes200_BF12B6(void);

/*
 * @routine 07:A41D GlyphSpriteDrawLeft8
 * WHAT IT DOES: As GlyphSpriteDraw, 8 pixels to the left.
 * HOW: Subtracts 8 from the sprite's x and joins GlyphSpriteDraw after its
 * first load. Only GlyphSpritesDrawAll calls it, with $0FAE 0 (unit_cpu).
 * Entry: M 16-bit, X 16-bit; X = the sprite * 2. Exit: as GlyphSpriteDraw.
 */
static uint8_t left8_cv;                /* the adapter's: C and V of the subtract */

uint8_t GlyphSpriteDrawLeft8(uint8_t bank, uint16_t x)
{
    (void)bank;
    uint16_t v, r;

    cyc(148);
    v = RD16I(glyph_spr_x, x);
    r = (uint16_t)(v - 8);
    cyc(6);                             /* the SBC */
    left8_cv = (uint8_t)((v >= 8 ? P_C : 0) | (((v ^ 8) & (v ^ r) & 0x8000) ? P_V : 0));
    cyc(54);
    return sprite_at(x, r);
}

void GlyphSpriteDrawLeft8_cr(Carried *cr)
{
    uint8_t mode = GlyphSpriteDrawLeft8(cr->db, cr->x);

    cr->a = (uint16_t)((sprite_glyph & 0xFF00) | mode);
    cr->x = (uint16_t)(cr->x + 2);
    set_p(cr, nz16((uint8_t)((cr->p & ~(P_M | P_X | P_C | P_V)) | left8_cv), cr->x));
}

void abi_GlyphSpriteDrawLeft8(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphSpriteDrawLeft8_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:A39A GlyphSpritesDrawAll
 * WHAT IT DOES: Draws glyph sprites 1 to 11 (GlyphSpriteDraw) with the
 * renderer bytes switched between them (GsuBytes200_BFB3BE before 1 and
 * 4, GsuBytes200_BFBCBD before 2 and 9); then, with $0FAE set, sprites 12
 * to 24 (BFB3BE before 12, BF12B6 before 16), or with $0FAE 0 only 12 to
 * 15, each 8 pixels to the left; then the glyph pictures GlyphGrid1A and
 * GlyphGrid1D.
 * HOW: X steps by 2 through the sprites; the left-shifted ones by
 * GlyphSpriteDrawLeft8 (only from unit_cpu).
 * Entry: D = $0000. Exit: as GlyphGrid1D leaves it.
 */
uint8_t GlyphSpritesDrawAll(uint8_t bank)
{
    uint16_t x = 0x0002;
    int i;

    cyc(22);
    (cyc(86), GsuBytes200_BFB3BE());
    (cyc(118), GlyphSpriteDraw(bank, x));
    x = (uint16_t)(x + 2);
    (cyc(190), GsuBytes200_BFBCBD());
    cyc(22);
    (cyc(96), GlyphSpriteDraw(bank, x));
    x = (uint16_t)(x + 2);
    (cyc(174), GlyphSpriteDraw(bank, x));
    x = (uint16_t)(x + 2);
    (cyc(190), GsuBytes200_BFB3BE());
    cyc(72);
    for (i = 0; i < 5; i++) {
        (cyc(46), GlyphSpriteDraw(bank, x));
        x = (uint16_t)(x + 2);
        cyc(128);
    }
    cyc(62);                            /* JSL; the loop's 128 is the rest of the last draw */
    GsuBytes200_BFBCBD();
    cyc(22);
    cyc(50);
    for (i = 0; cyc_loop(46, i < 3); i++) {
        GlyphSpriteDraw(bank, x);
        x = (uint16_t)(x + 2);
        cyc(128);
    }
    cyc(24);
    if (!RD16(wram_0FAE)) {
        (cyc(78), GsuBytes200_BFB3BE());          /* BNE, JSL */
        for (i = 0; i < 4; i++) {
            if (i)
                cyc(56);
            GlyphSpriteDrawLeft8(bank, x);
            x = (uint16_t)(x + 2);
        }
        cyc(196);                       /* the draw's rest, BRA, JSR */
    } else {
        cyc(84);
        GsuBytes200_BFB3BE();
        cyc(72);
        for (i = 0; i < 4; i++) {
            (cyc(46), GlyphSpriteDraw(bank, x));
            x = (uint16_t)(x + 2);
            cyc(128);
        }
        cyc(62);
        GsuBytes200_BF12B6();
        for (i = 0; i < 9; i++) {
            (cyc(46), GlyphSpriteDraw(bank, x));
            x = (uint16_t)(x + 2);
            cyc(128);
        }
        cyc(46);                        /* JSR */
    }
    GlyphGrid1A();
    {
        uint8_t mode = (cyc(46), GlyphGrid1D());

        cyc(42);
        return mode;
    }
}

void GlyphSpritesDrawAll_cr(Carried *cr)
{
    cr->a = (uint16_t)(0x1F00 | GlyphSpritesDrawAll(cr->db));
    cr->x = 0xF280;
    set_p(cr, (uint8_t)((nz8(cr->p, cr->db) & ~(P_M | P_X | P_C | P_V)) | pos_cv));
}

void abi_GlyphSpritesDrawAll(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphSpritesDrawAll_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


void TimeUnpackBcd(uint16_t packed, uint16_t *min, uint16_t *sec, uint16_t *hund, uint8_t *p);

/* What the HUD times' adapters rebuild: A as the last step leaves it, and
   X and Y when it changed them (bit 0, bit 1 of hud_xy). */
static uint16_t hud_a, hud_x, hud_y;
static int hud_xy;
/* V from the last TimeUnpackBcd that computed one (packed_v_set), for
   HudTimesPackedDraw's exit. */
static int packed_v_set;
static uint8_t packed_v;

/* REP #$30, then the renderer bytes by player 1's lead ($0FA3, signed):
   negative, positive or 0. */
static int lead_bytes(int8_t lead, uint8_t (*neg)(void), uint8_t (*pos)(void), uint8_t (*zero)(void))
{
    cyc(22);
    if (lead < 0)
        { cyc(84); neg(); }
    else if (lead > 0)
        { cyc(100); pos(); return 1; }
    else
        { cyc(94); zero(); }
    return 0;
}

/* SEP #$20, LDA $0FA3 (8-bit): player 1's lead, the flags from it. */
static int8_t lead_read(void)
{
    cyc(18);
    return (int8_t)RD8(p1_lead);
}

/* LDA #$0093, STA.L $70:020A, LDA #y, STA.L $70:020C: the draw position. */
static void times_at(uint16_t y, uint32_t less)
{
    cyc(38);
    cyc(112 - less);                    /* less: 22 after the setters BFB3BE and BF11BD, 72 after those that state their return */
    wr16(0x70020A, 0x0093);
    cyc(38);
    cyc(18);
    wr16(0x70020C, y);
}

/* LDY #seq, JSR GsuF329Sequence. */
static uint16_t times_seq_tail(uint32_t b, uint16_t seq, uint32_t tail)
{
    uint8_t bank = (uint8_t)(b >> 16), mode = (cyc(70), GsuF329Sequence(bank, seq));  /* LDY #, JSR */
    uint16_t a = (uint16_t)(f329_high(bank) | mode);

    cyc(tail);
    hud_a = a;
    hud_x = 0xF329;
    hud_y = seq;
    hud_xy = 3;
    return a;
}

static uint16_t times_seq(uint32_t b, uint16_t seq)
{
    return times_seq_tail(b, seq, 42);
}

/* 07:8A5D on: the first row ($0F64 at y 88), then the second's renderer
   bytes by the lead (negative B2B3, positive 11BD, 0 10B9) and the second
   row ($0F69 at y 108); then P pulled. */
static void times_rows(uint32_t b, uint32_t less)
{
    int pos;

    times_at(0x0058, less);
    times_seq_tail(b, 0x0F64, 28);
    pos = lead_bytes(lead_read(), GsuBytes200_BFB2B3, GsuBytes200_BF11BD, GsuBytes200_BF10B9);
    times_at(0x006C, (pos ? 22 : 0) + 72);
    times_seq_tail(b, 0x0F69, 28);
    cyc(42);                            /* PLP */
}

static uint8_t lap_hi;                  /* HudLapDigitsDraw's: A's high byte, the last digit's word */
static uint8_t lead_cv;                 /* HudLeadWordDraw's: C and V of the move */

/*
 * @routine 07:89F5 HudTimesDraw
 * WHAT IT DOES: Unless the low byte of $10CD is set, and only while the
 * HUD hold flag $0E5D is set: in HUD mode 3 with $0D4C set, the two packed
 * times (HudTimesPackedDraw); with no time at $0F2B ($FFFF), the sequence
 * at $0F69 alone at (147, 108); with $0E57 set, the lap digits
 * (HudLapDigitsDraw) and, when player 1 leads, the word at (60, 72)
 * (HudLeadWordDraw); otherwise the sequences at $0F64 and $0F69 at (147,
 * 88) and (147, 108), coloured by player 1's lead $0FA3.
 * HOW: P saved and restored; 16-bit tests, 8-bit for the lead. The first
 * row's colour test is BMI then BPL, so its zero case (07:8A47) is never
 * reached; 07:8A90 is the second row's tail HudHoldDraw jumps to.
 * Entry: any widths. Exit: P restored; A, X, Y as the last step leaves
 * them.
 */
void HudTimesDraw(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    int8_t lead;
    uint16_t v;
    uint8_t mode;

    hud_xy = 0;
    cyc(22);
    cyc(46);
    v = (uint16_t)(RD16(wram_10CD) & 0x00FF);
    cyc(2);
    if (v) {
        cyc(72);
        cyc(42);                        /* PLP */
        hud_a = v;
        return;
    }
    cyc(62);
    if (!RD16(hud_hold)) {
        cyc(44);
        cyc(42);
        hud_a = 0x0000;
        return;
    }
    cyc(46);
    if (RD16(hud_mode) == 0x0003) {
        cyc(64);
        if (RD16(digit62_override)) {
            cyc(90);
            HudTimesPackedDraw(bank);
            cyc(28);
            cyc(42);
            return;
        }
    }
    cyc(70);
    if (RD16(wram_0F2B) == 0xFFFF) {
        cyc(102);
        GsuBytes200_BFB3BE();
        cyc(2);
        times_at(0x006C, 0);
        times_seq_tail(b, 0x0F69, 28);
        cyc(42);
        return;
    }
    cyc(92);
    if (RD8(wram_0E57)) {
        mode = (cyc(62), HudLapDigitsDraw(bank));
        cyc(28);
        lead = lead_read();
        if (lead > 0) {
            cyc(140);
            mode = HudLeadWordDraw();
            cyc(28);
            hud_a = (uint16_t)(0x9400 | mode);
            hud_x = 0xF329;
            hud_xy = 1;
        } else {
            cyc(50);
            hud_a = (uint16_t)(lap_hi << 8 | (uint8_t)lead);
            hud_x = 0xF6DF;
            hud_xy = 1;
        }
        cyc(42);
        return;
    }
    cyc(46);
    lead = (int8_t)RD8(p1_lead);
    cyc(22);
    if (lead < 0)
        { cyc(84); GsuBytes200_BFB3BE(); }
    else
        { cyc(100); GsuBytes200_BFB2B3(); }
    times_rows(b, lead < 0 ? 22 : 72);
}

void HudTimesDraw_cr(Carried *cr)
{
    HudTimesDraw(cr->db);
    cr->a = hud_a;
    if (hud_xy & 1)
        cr->x = hud_x;
    if (hud_xy & 2)
        cr->y = hud_y;
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_HudTimesDraw(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudTimesDraw_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 07:8A47 HudTimesDrawEven
 * WHAT IT DOES: HudTimesDraw's first row for no lead, which nothing
 * reaches (the test before it branches on both signs): the renderer
 * bytes 0, $BF, $10, $B9 (GsuBytes200_BF10B9), then both rows as
 * HudTimesDraw.
 * HOW: As inside HudTimesDraw; ends pulling the P its start pushed.
 * Entry: M 8-bit, X 16-bit; P on the stack above the return address.
 * Exit: as HudTimesDraw.
 */
uint8_t HudTimesDrawEven(uint8_t bank)
{
    uint8_t p0;

    cyc(22);
    (cyc(62), GsuBytes200_BF10B9());
    cpu.s++;
    p0 = rd8((uint16_t)cpu.s);
    times_rows((uint32_t)bank << 16, 72);
    return p0;
}

void HudTimesDrawEven_cr(Carried *cr)
{
    set_p(cr, HudTimesDrawEven(cr->db));
    cr->a = hud_a;
    cr->x = hud_x;
    cr->y = hud_y;
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_HudTimesDrawEven(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudTimesDrawEven_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:8AF5 TimeDigitsStore
 * WHAT IT DOES: Stores a time unpacked by TimeUnpackBcd as five digits at
 * $0F64: the minutes A, then the seconds X and the hundredths Y, each as
 * its high and low nibble.
 * HOW: 8-bit; four LSRs for each high nibble, AND #$0F for each low.
 * Entry: A, X, Y as TimeUnpackBcd leaves them. Exit: M and X 8-bit (X and
 * Y high bytes 0); A = Y AND $0F; N, Z from it, C bit 3 of Y.
 */
void TimeDigitsStore(uint8_t bank, uint8_t min, uint8_t sec, uint8_t hund)
{
    (void)bank;

    cyc(22);
    cyc(70);
    WR8(lap_record_a, min);
    cyc(94);
    WR8(wram_0F65, (uint8_t)(sec >> 4));
    cyc(54);
    WR8(wram_0F66, (uint8_t)(sec & 0x0F));
    cyc(94);
    WR8(wram_0F67, (uint8_t)(hund >> 4));
    cyc(54);
    WR8(wram_0F68, (uint8_t)(hund & 0x0F));
    cyc(42);
}

void TimeDigitsStore_cr(Carried *cr)
{
    uint8_t y = (uint8_t)cr->y;

    TimeDigitsStore(cr->db, (uint8_t)cr->a, (uint8_t)cr->x, y);
    cr->x &= 0x00FF;
    cr->y = y;
    cr->a = (uint16_t)((cr->a & 0xFF00) | (y & 0x0F));
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(y & 0x0F)) & ~P_C) | P_M | P_X | ((y >> 3) & 1 ? P_C : 0)));
}

void abi_TimeDigitsStore(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeDigitsStore_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* LDA time, JSR TimeUnpackBcd, JSR TimeDigitsStore, REP #$30, then the
   sequence at $0F64. */
/* TimeUnpackBcd (binary: D is clear here), then TimeDigitsStore. */
static void packed_unpack(uint32_t b, uint16_t packed)
{
    uint16_t min, sec, hund;
    uint8_t p = 0;

    TimeUnpackBcd(packed, &min, &sec, &hund, &p);
    if (packed != 0xFFFF) {
        packed_v_set = 1;
        packed_v = (uint8_t)(p & P_V);
    }
    TimeDigitsStore((uint8_t)(b >> 16), (uint8_t)min, (uint8_t)sec, (uint8_t)hund);
}

static void packed_time_draw(uint32_t b, uint16_t at)
{
    uint16_t v;

    cyc(24);
    v = rd16(b | at);
    cyc(46);                            /* JSR */
    packed_unpack(b, v);
    cyc(22);
    times_seq(b, 0x0F64);
}

/* 07:8B38 on: the first packed time ($1163, unless $FFFF) at (147, 88),
   then the second's colour by the lead and the second ($1161) at (147,
   108). */
static void packed_rows(uint32_t b, uint32_t less)
{
    uint16_t v;
    int pos;

    times_at(0x0058, less);
    cyc(24);
    v = RD16(packed_time_a);
    if (v != 0xFFFF) {
        cyc(86);
        packed_unpack(b, v);
        cyc(22);
        times_seq_tail(b, 0x0F64, 28);
    } else { cyc(74); }
    pos = lead_bytes(lead_read(), GsuBytes200_BFB2B3, GsuBytes200_BF11BD, GsuBytes200_BF10B9);
    times_at(0x006C, (pos ? 22 : 0) + 72);
    packed_time_draw(b, W(packed_time_b));
}

/*
 * @routine 07:8B17 HudTimesPackedDraw
 * WHAT IT DOES: HudTimesDraw's rows from the packed times: $1163 (unless
 * $FFFF, no time) at (147, 88) and $1161 at (147, 108), each unpacked to
 * the digits at $0F64 (TimeUnpackBcd, TimeDigitsStore) and drawn from
 * there, coloured by player 1's lead $0FA3.
 * HOW: 8-bit for the lead, else 16-bit. The first colour test is BMI then
 * BPL, so its zero case (07:8B22) is never reached.
 * Entry: any widths. Exit: M 16-bit, X 16-bit; A, X, Y as
 * GsuF329Sequence leaves them.
 */
void HudTimesPackedDraw(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    int8_t lead;

    cyc(22);
    lead = lead_read();
    cyc(22);
    if (lead < 0)
        { cyc(84); GsuBytes200_BFB3BE(); }
    else
        { cyc(100); GsuBytes200_BFB2B3(); }
    packed_rows(b, lead < 0 ? 22 : 72);
}

void HudTimesPackedDraw_cr(Carried *cr)
{
    packed_v_set = 0;
    HudTimesPackedDraw(cr->db);
    cr->a = hud_a;
    cr->x = hud_x;
    cr->y = hud_y;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X | P_C)));
    if (packed_v_set)
        set_p(cr, (uint8_t)((cr->p & ~P_V) | packed_v));
}

void abi_HudTimesPackedDraw(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudTimesPackedDraw_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:8B22 HudTimesPackedEven
 * WHAT IT DOES: HudTimesPackedDraw's first row for no lead, which nothing
 * reaches: the renderer bytes 0, $BF, $10, $B9 (GsuBytes200_BF10B9), then
 * both rows as HudTimesPackedDraw.
 * HOW: As inside HudTimesPackedDraw.
 * Entry: M 8-bit, X 16-bit. Exit: as HudTimesPackedDraw.
 */
void HudTimesPackedEven(uint8_t bank)
{
    cyc(22);
    (cyc(62), GsuBytes200_BF10B9());
    packed_rows((uint32_t)bank << 16, 72);
}

void HudTimesPackedEven_cr(Carried *cr)
{
    packed_v_set = 0;
    HudTimesPackedEven(cr->db);
    cr->a = hud_a;
    cr->x = hud_x;
    cr->y = hud_y;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X | P_C)));
    if (packed_v_set)
        set_p(cr, (uint8_t)((cr->p & ~P_V) | packed_v));
}

void abi_HudTimesPackedEven(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudTimesPackedEven_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* REP #$20, LDA #glyph, STA.L $70:0216, then Super FX program $07:F329. */
static uint8_t word_glyph(uint16_t glyph)
{
    cyc(22);
    return gprog(0xF329, glyph);
}

/*
 * @routine 07:A7AE HudLeadWordDraw
 * WHAT IT DOES: Draws a word of glyphs at (60, 72) with the renderer bytes
 * 0, $BF, $11, $BD (GsuBytes200_BF11BD): four glyphs, the position moved 8
 * right, then six more.
 * HOW: Each glyph by a long store to $70:0216 and Super FX program
 * $07:F329.
 * Entry: any widths. Exit: M 16-bit, X 16-bit; X = $F329; A as GsuRun
 * leaves it.
 */
uint8_t HudLeadWordDraw(void)
{
    static const uint16_t first[4] = {0x9416, 0x941C, 0x9618, 0x961A};
    static const uint16_t rest[6] = {0x9616, 0x941C, 0x9418, 0x9610, 0x9616, 0x941A};
    uint16_t v, r;
    uint8_t mode = 0;
    int i;

    cyc(22);
    GsuBytes200_BF11BD();
    cyc(38);
    cyc(18);
    wr16(0x70020A, 0x003C);
    cyc(38);
    cyc(18);
    wr16(0x70020C, 0x0048);
    for (i = 0; i < 4; i++)
        word_glyph(first[i]);
    cyc(32);
    v = rd16(0x70020A);
    r = (uint16_t)(v + 0x0008);
    lead_cv = (uint8_t)((v > 0xFFF7 ? P_C : 0) | ((~v & r & 0x8000) ? P_V : 0));
    cyc(70);
    wr16(0x70020A, r);
    for (i = 0; i < 6; i++)
        mode = word_glyph(rest[i]);
    cyc(42);                            /* RTS */
    return mode;
}

void HudLeadWordDraw_cr(Carried *cr)
{
    cr->a = (uint16_t)(0x9400 | HudLeadWordDraw());
    cr->x = 0xF329;
    set_p(cr, (uint8_t)((nz8(cr->p, cr->db) & ~(P_M | P_X | P_C | P_V)) | lead_cv));
}

void abi_HudLeadWordDraw(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudLeadWordDraw_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/* LDA digit, AND #$00FF, JSR GsuParam216A, then Super FX program
   $07:F6DF. */
static uint8_t lap_digit(uint32_t b, uint16_t at)
{
    uint16_t v;

    cyc(24);
    v = (uint16_t)(rd16(b | at) & 0x00FF);
    cyc(2);
    cyc(68);                            /* AND, JSR */
    lap_hi = (uint8_t)(GsuParam216A((uint8_t)(b >> 16), (uint16_t)(v << 1)) >> 8);
    cyc(42);                            /* its BCS not taken: the index's top bit is 0 */
    return gprog_run(0xF6DF);
}

/*
 * @routine 07:A8D3 HudLapDigitsDraw
 * WHAT IT DOES: Draws the five digits at $0F69 at (48, 40) with the
 * renderer bytes 0, $BF, $B3, $BE (GsuBytes200_BFB3BE), as M:SS:HH: the
 * separators glyphs $9014 and $9018 by program $07:FAB0.
 * HOW: Each digit through the table GsuParam216A reads and Super FX
 * program $07:F6DF.
 * Entry: any widths; data bank $07 (GsuParam216A's table). Exit: M 16-bit,
 * X 16-bit; X = $F6DF; A as GsuRun leaves it.
 */
uint8_t HudLapDigitsDraw(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;

    cyc(22);
    (cyc(62), GsuBytes200_BFB3BE());
    cyc(38);
    cyc(90);
    wr16(0x70020A, 0x0030);
    cyc(38);
    cyc(18);
    wr16(0x70020C, 0x0028);
    lap_digit(b, W(lap_digits));
    gprog(0xFAB0, 0x9014);
    lap_digit(b, W(wram_0F6A));
    lap_digit(b, W(wram_0F6B));
    gprog(0xFAB0, 0x9018);
    lap_digit(b, W(wram_0F6C));
    {
        uint8_t mode = lap_digit(b, W(wram_0F6D));

        cyc(42);                        /* RTS */
        return mode;
    }
}

void HudLapDigitsDraw_cr(Carried *cr)
{
    uint8_t mode = HudLapDigitsDraw(cr->db);

    cr->a = (uint16_t)(lap_hi << 8 | mode);
    cr->x = 0xF6DF;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X | P_C)));
}

void abi_HudLapDigitsDraw(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudLapDigitsDraw_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/* REP #$30, LDA #x, STA.L $70:020A, then Super FX program $07:F5C9. */
static uint8_t blank_mark_at(uint16_t x, uint32_t t)
{
    cyc(22);
    cyc(38);
    cyc(t);                             /* 62 from a branch, 18 after a run's REPs */
    wr16(0x70020A, x);
    return gprog_run(0xF5C9);
}

/*
 * @routine 07:A6DD HudBlankDraw
 * WHAT IT DOES: Stunt Trax with $0D4E set: three marks ($0C08, bytes 0,
 * $BF, $BA, $BD) at (4, 108), (22, 108) and (40, 108), then over them,
 * with the bytes 0, $BF, $B7, $B6, the mark $0E08 for each of the low
 * nibble of $1F71 used: none, at 40, 22 and 4 for 0; at 22 and 4 for 1;
 * at 4 for 2.
 * HOW: Super FX program $07:F5C9 per mark; P saved and restored.
 * Entry: any widths; data bank $07. Exit: P restored; A, X, Y as the last
 * run leaves them.
 */
static uint16_t blank_a;                /* the adapter's: A as the draw leaves it */

void HudBlankDraw(uint8_t bank)
{
    (void)bank;
    uint16_t n;

    cyc(22);
    GsuBytes200_BFBABD();
    cyc(22);
    cyc(38);
    cyc(90);
    wr16(0x700216, 0x0C08);
    cyc(38);
    cyc(18);
    wr16(0x70020A, 0x0004);
    cyc(38);
    cyc(18);
    wr16(0x70020C, 0x006C);
    gprog_run(0xF5C9);
    blank_mark_at(0x0016, 18);
    blank_mark_at(0x0028, 18);
    (cyc(62), GsuBytes200_BFB7B6());
    cyc(22);
    cyc(38);
    cyc(90);
    wr16(0x700216, 0x0E08);
    cyc(24);
    n = (uint16_t)(RD16(script_byte_1f71) & 0x000F);
    cyc(2);
    if (n) {
        if (n < 2)
            cyc(6);
        if (n > 2) {
            cyc(122);
            cyc(42);                    /* PLP */
            blank_a = n;
            return;
        }
    }
    if (n == 0)
        blank_mark_at(0x0028, 62);
    if (n <= 1) {
        if (n == 0)
            blank_mark_at(0x0016, 18);
        else
            { cyc(50); blank_mark_at(0x0016, 62); }
    }
    if (n <= 1)
        blank_a = blank_mark_at(0x0004, 18);
    else
        { cyc(50); blank_a = blank_mark_at(0x0004, n == 2 ? 52 : 62); }
    cyc(28);
    cyc(42);                            /* PLP */
}

void HudBlankDraw_cr(Carried *cr)
{
    HudBlankDraw(cr->db);
    cr->a = blank_a;
    cr->x = (uint16_t)(cr->p & P_X ? 0x00C9 : 0xF5C9);
}

void abi_HudBlankDraw(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HudBlankDraw_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}



/* GlyphSpritesSlideIn's tail (07:A4F5): the two glyph words drawn at the
   sprites' positions ($0FB0/$0FE0 with the bytes 0, $BF, $B7, $B6;
   $0FB2/$0FE2 with 0, $BF, $BC, $BD), program $07:FAB0 per glyph, the
   position moved 8 on after each word's third glyph. */
static uint8_t glyph_slide_draw(void)
{
    static const uint16_t one[6] = {0x8804, 0x8808, 0x8C14, 0x8408, 0x8814, 0x8814};
    static const uint16_t two[8] = {0x8C14, 0x8804, 0x8418, 0x8410, 0x8408, 0x8C0C, 0x8C10, 0x901C};
    uint8_t mode = 0;
    unsigned i;

    cyc(22);
    (cyc(62), GsuBytes200_BFB7B6());
    cyc(96);
    wr16(0x70020A, cyc_then(32, RD16(glyph_spr_x)));
    cyc(24);
    wr16(0x70020C, cyc_then(32, RD16(glyph_spr_y)));
    for (i = 0; i < 6; i++) {
        if (i == 3)
            pos_add(0x70020A, 0x0008, 0);
        gprog(0xFAB0, one[i]);
    }
    cyc(62);
    GsuBytes200_BFBCBD();
    cyc(22);
    cyc(74);
    wr16(0x70020A, cyc_then(32, RD16(wram_0FB2)));
    cyc(24);
    wr16(0x70020C, cyc_then(32, RD16(wram_0FE2)));
    for (i = 0; i < 8; i++) {
        if (i == 3)
            pos_add(0x70020A, 0x0008, 0);
        mode = gprog(0xFAB0, two[i]);
    }
    cyc(42);                            /* RTS */
    return mode;
}

/* CMP #m (16-bit): N, Z, C as the compare sets them. */
static void slide_cmp16(uint16_t a, uint16_t m)
{
    if (a >= m)
        cyc(18);
}

/*
 * @routine 07:A451 GlyphSpritesSlideOut
 * WHAT IT DOES: Moves the two glyph sprites out by $16: the first left to
 * no further than -$68 ($0FB0), the second right to no further than $D0
 * ($0FB2); once both are there, phase $0DD6 = 6 (a 16-bit store) and
 * nothing is drawn; otherwise they are drawn (GlyphSpritesSlideIn's
 * tail).
 * HOW: 16-bit, through the data bank; the tail is a branch.
 * Entry: any widths; data bank $07. Exit: M 16-bit, X 16-bit; A = 6; or
 * as the drawing leaves them.
 */
static uint8_t slide_v;                 /* the adapter's: V of the add to the second sprite */

/* Returns 1 when both sprites were out (A = 6, nothing drawn); else the
   draw ran and slide_mode is its screen mode value. */
static uint8_t slide_mode;

int GlyphSpritesSlideOut(uint8_t bank)
{
    (void)bank;
    uint16_t a, v;

    cyc(22);
    cyc(24);
    a = (uint16_t)(RD16(glyph_spr_x) - 0x0016);
    cyc(6);                             /* the SBC */
    if (a & 0x8000) {
        slide_cmp16(a, 0xFF98);
        if (a < 0xFF98)
            { cyc(36); a = 0xFF98; }
    }
    cyc(100);
    WR16(glyph_spr_x, a);
    cyc(24);
    v = RD16(wram_0FB2);
    a = (uint16_t)(v + 0x0016);
    slide_v = (~v & a & 0x8000) ? P_V : 0;
    slide_cmp16(a, 0x00D0);
    if (a >= 0x00D0)
        a = 0x00D0;
    cyc(108);
    WR16(wram_0FB2, a);
    cyc(24);
    a = RD16(glyph_spr_x);
    slide_cmp16(a, 0xFF98);
    if (a == 0xFF98) {
        cyc(46);
        a = RD16(wram_0FB2);
        slide_cmp16(a, 0x00D0);
        if (a == 0x00D0) {
            cyc(70);
            WR16(wram_0DD6, 0x0006);
            cyc(42);                    /* RTS */
            return 1;
        }
    } else { cyc(50); }
    slide_mode = glyph_slide_draw();
    return 0;
}

void GlyphSpritesSlideOut_cr(Carried *cr)
{
    if (GlyphSpritesSlideOut(cr->db)) {
        cr->a = 0x0006;
        set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_M | P_X | P_V)) | P_C | slide_v));
    } else {
        cr->a = (uint16_t)(0x9000 | slide_mode);
        cr->x = 0xFAB0;
        set_p(cr, (uint8_t)((nz8(cr->p, cr->db) & ~(P_M | P_X | P_C | P_V)) | pos_cv));
    }
}

void abi_GlyphSpritesSlideOut(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphSpritesSlideOut_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 07:A492 GlyphSpritesSlideIn
 * WHAT IT DOES: Moves the two glyph sprites in by $16: the first left to
 * no less than $34 ($0FB0), the second right to no more than $24 ($0FB2,
 * unless negative); once both are there, counts $0E55 down from $10 (at
 * $10, $0E5B = $10 and sounds 1 and $3A; at 0, phase $0DD6 = 4),
 * otherwise sets it to $10; then draws them: the first word's six glyphs
 * at the first sprite, the second word's eight at the second.
 * HOW: 16-bit moves through the data bank; program $07:FAB0 per glyph.
 * Entry: any widths; data bank $07. Exit: M 16-bit, X 16-bit; A, X as the
 * last run leaves them.
 */
uint8_t GlyphSpritesSlideIn(uint8_t bank)
{
    (void)bank;
    uint16_t a;
    uint8_t v;
    int there;

    cyc(22);
    cyc(24);
    a = (uint16_t)(RD16(glyph_spr_x) - 0x0016);
    cyc(6);                             /* the SBC */
    slide_cmp16(a, 0x0034);
    if (a < 0x0034)
        { cyc(36); a = 0x0034; }
    cyc(84);
    WR16(glyph_spr_x, a);
    cyc(24);
    a = (uint16_t)(RD16(wram_0FB2) + 0x0016);
    if (!(a & 0x8000)) {
        slide_cmp16(a, 0x0024);
        if (a >= 0x0024)
            a = 0x0024;
    }
    cyc(124);
    WR16(wram_0FB2, a);
    cyc(24);
    a = RD16(glyph_spr_x);
    slide_cmp16(a, 0x0034);
    there = 0;
    if (a == 0x0034) {
        cyc(46);
        a = RD16(wram_0FB2);
        slide_cmp16(a, 0x0024);
        there = a == 0x0024;
    }
    if (there) {
        cyc(68);
        v = RD8(wram_0E55);
        if (v < 0x10)
            cyc(38);
        if (v == 0x10) {
            cyc(56);
            WR8(wram_0E5B, v);
            (cyc(40), SoundPort0QueuePushRaw(0x01));
            cyc(8);
            (cyc(86), SoundPort0QueuePush(0x3A));
        }
        cyc(24);
        v = (uint8_t)(RD8(wram_0E55) - 1);
        cyc(6);
        WR8(wram_0E55, v);
        if (!v) {
            cyc(56);
            WR8(wram_0DD6, 0x04);
        }
        cyc(22);
    } else {
        cyc(90);
        WR8(wram_0E55, 0x10);
    }
    return glyph_slide_draw();
}

void GlyphSpritesSlideIn_cr(Carried *cr)
{
    cr->a = (uint16_t)(0x9000 | GlyphSpritesSlideIn(cr->db));
    cr->x = 0xFAB0;
    set_p(cr, (uint8_t)((nz8(cr->p, cr->db) & ~(P_M | P_X | P_C | P_V)) | pos_cv));
}

void abi_GlyphSpritesSlideIn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GlyphSpritesSlideIn_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}




/* $70:0210 = $20, $70:020E = $10, $70:020A = $28, $70:020C = $18, the
   renderer bytes (setter), then program $07:F15B for each $70:0216. */
static uint8_t f15b_four(void (*setter)(void), const uint16_t *in216)
{
    uint8_t mode = 0;
    unsigned i;

    cyc(124);
    wr16(0x700210, 0x0020);
    cyc(56);
    wr16(0x70020E, 0x0010);
    cyc(56);
    wr16(0x70020A, 0x0028);
    cyc(56);
    wr16(0x70020C, 0x0018);
    (cyc(62), setter());
    for (i = 0; i < 4; i++) {
        cyc(i ? 194 : 150);
        wr16(0x700216, in216[i]);
        mode = f15b();
    }
    cyc(158);
    return mode;
}

uint8_t GsuF15BFourA(void)
{
    static const uint16_t in216[4] = {0x8814, 0x8C00, 0x8C10, 0x8418};

    return f15b_four(GsuBytes200_0F0708, in216);
}

uint8_t GsuF15BFourB(void)
{
    static const uint16_t in216[4] = {0x8414, 0x8C0C, 0x8408, 0x9000};

    return f15b_four(GsuBytes200_0F0D03, in216);
}

/*
 * @routine 07:98F4 GsuF15BFourA
 * WHAT IT DOES: Runs Super FX program $07:F15B four times, with $70:0216 =
 * $8814, $8C00, $8C10 and $8418, after setting $70:0210 = $20, $70:020E =
 * $10, $70:020A = $28, $70:020C = $18 and the bytes $0F, $07, $08 at
 * $70:0200.
 * HOW: GsuBytes200_0F0708, then GsuRun for each value.
 * Entry: any widths, D = $0000. Exit: A = $84 and the screen mode value; X
 * = $F15B; M 16-bit, X 16-bit; N, Z from the data bank.
 */
void GsuF15BFourA_cr(Carried *cr)
{
    cr->a = (uint16_t)(0x8400 | GsuF15BFourA());
    cr->x = 0xF15B;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

void abi_GsuF15BFourA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuF15BFourA_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:9A24 GsuF15BFourB
 * WHAT IT DOES: As GsuF15BFourA with $70:0216 = $8414, $8C0C, $8408 and
 * $9000 and the bytes $0F, $0D, $03.
 * HOW: GsuBytes200_0F0D03, then GsuRun for each value.
 * Entry: any widths, D = $0000. Exit: as GsuF15BFourA, A = $90 and the
 * screen mode value.
 */
void GsuF15BFourB_cr(Carried *cr)
{
    cr->a = (uint16_t)(0x9000 | GsuF15BFourB());
    cr->x = 0xF15B;
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
}

void abi_GsuF15BFourB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuF15BFourB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:9B6F BigSpritesNine
 * WHAT IT DOES: Unless $0E5C is set, draws nine 24 x 40 sprites
 * (FxSprite24x40): from (41, 33) $AA06, $B006, $B000, then 6 further on
 * $AA06 and $A012; from (56, 80) $AA12, $A000, $AA00 and $A50C. With $0E5C
 * set it goes to 07:9EE9 instead.
 * HOW: GsuBytes200_BFB7B6 first; GsuRun for each sprite; the x the program
 * leaves is read back for the gap.
 * Entry: any widths, D = $0000. Exit: as BigSpritesRow6.
 */
uint8_t BigSpritesNine(uint8_t bank)
{
    static const uint16_t first[3] = {0xAA06, 0xB006, 0xB000}, second[2] = {0xAA06, 0xA012};
    static const uint16_t third[4] = {0xAA12, 0xA000, 0xAA00, 0xA50C};
    uint16_t w, r;
    uint8_t mode = 0;
    unsigned i;

    if (!(cyc(16), big_sprites_start(bank, GsuBytes200_BFB7B6)))
        return rows_count;
    big_sprites_at(0x0029, 0x0021);
    for (i = 0; i < 3; i++)
        { if (i) cyc(44); sprite24x40(first[i]); }
    cyc(76);
    w = rd16(0x70020A);
    r = (uint16_t)(w + 0x0006);
    row_cv = (uint8_t)((w > 0xFFF9 ? P_C : 0) | ((~w & r & 0x8000) ? P_V : 0));
    cyc(70);
    wr16(0x70020A, r);
    for (i = 0; i < 2; i++)
        { if (i) cyc(44); sprite24x40(second[i]); }
    cyc(100);
    wr16(0x70020A, 0x0038);
    cyc(56);
    wr16(0x70020C, 0x0050);
    for (i = 0; i < 4; i++)
        { if (i) cyc(44); mode = sprite24x40(third[i]); }
    return mode;
}

void BigSpritesNine_cr(Carried *cr)
{
    uint8_t v = BigSpritesNine(cr->db);

    cr->x = 0xFA88;
    if (rows_counted) {
        cr->a = (uint16_t)(count_high << 8 | v);
        set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_X)) | P_M | count_flags));
    } else {
        cr->a = (uint16_t)(0xA500 | v);
        set_p(cr, (uint8_t)((nz8(cr->p, cr->db) & ~(P_M | P_X | P_C | P_V)) | row_cv));
    }
}

void abi_BigSpritesNine(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BigSpritesNine_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 07:9C9F BigSpritesRow6B
 * WHAT IT DOES: Unless $0E5C is set, draws six 24 x 40 sprites
 * (FxSprite24x40) from (24, 40): $AA18, $AA0C, $A512, $A000, $AA18 and
 * $A500. With $0E5C set it goes to 07:9EE9 instead.
 * HOW: GsuBytes200_BFB3BE first; GsuRun for each sprite.
 * Entry: any widths, D = $0000. Exit: as BigSpritesRow5.
 */
static void bytes_bfb3be(void)
{
    GsuBytes200_BFB3BE();
}

uint8_t BigSpritesRow6B(uint8_t bank)
{
    static const uint16_t src[6] = {0xAA18, 0xAA0C, 0xA512, 0xA000, 0xAA18, 0xA500};
    uint8_t mode = 0;
    unsigned i;

    if (!(cyc(16), big_sprites_start(bank, bytes_bfb3be)))
        return rows_count;
    big_sprites_at(0x0018, 0x0028);
    for (i = 0; i < 6; i++)
        { if (i) cyc(44); mode = sprite24x40(src[i]); }
    cyc(128);
    return mode;
}

void BigSpritesRow6B_cr(Carried *cr)
{
    uint8_t v = BigSpritesRow6B(cr->db);

    cr->x = 0xFA88;
    if (rows_counted) {
        cr->a = (uint16_t)(count_high << 8 | v);
        set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_X)) | P_M | count_flags));
    } else {
        cr->a = (uint16_t)(0xA500 | v);
        set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~(P_M | P_X)));
    }
}

void abi_BigSpritesRow6B(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BigSpritesRow6B_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 02:DA14 GsuClearScreenIfSet
 * WHAT IT DOES: Runs Super FX program $01:AECA (FxClearScreenIfSet), which
 * clears the picture from $2C00 when $01C4 is set. Nothing in the game
 * calls it.
 * HOW: 8-bit A, 16-bit index registers; GsuRun with A = $01, X = $AECA.
 * P is saved and restored.
 * Entry: any widths. Exit: A low byte = GsuRun's screen mode value; X =
 * $AECA (its low byte when X is 8-bit); P unchanged.
 */
uint8_t GsuClearScreenIfSet(void)
{
    cyc(168);
    return GsuRun(0x01, 0xAECA);
}

void GsuClearScreenIfSet_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | GsuClearScreenIfSet());
    cr->x = 0xAECA;
    cyc(102);                           /* GsuRun's PLB, RTL, then the PLP and RTL left */
    r_plp(cr, cr->p);
}

void abi_GsuClearScreenIfSet(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuClearScreenIfSet_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:DA24 GsuClear6000IfSet
 * WHAT IT DOES: Runs Super FX program $01:AEE1 (FxClear6000IfSet), which
 * clears the picture from $6000 when $01C4 is set. Nothing in the game
 * calls it.
 * HOW: 8-bit A, 16-bit index registers; GsuRun with A = $01, X = $AEE1.
 * P is saved and restored.
 * Entry: any widths. Exit: A low byte = GsuRun's screen mode value; X =
 * $AEE1 (its low byte when X is 8-bit); P unchanged.
 */
uint8_t GsuClear6000IfSet(void)
{
    cyc(168);
    return GsuRun(0x01, 0xAEE1);
}

void GsuClear6000IfSet_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | GsuClear6000IfSet());
    cr->x = 0xAEE1;
    cyc(102);                           /* GsuRun's PLB, RTL, then the PLP and RTL left */
    r_plp(cr, cr->p);
}

void abi_GsuClear6000IfSet(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GsuClear6000IfSet_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

