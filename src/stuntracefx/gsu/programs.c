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
#include "hw.h"
#include "cpu_ops.h"

uint8_t GsuRun(uint8_t bank, uint16_t start);
uint16_t GsuParam216B(uint8_t bank, uint16_t index);
void GsuBytes200_0F050E(void);
void GsuBytes200_0F080E(void);

#define GSU_IN_68 0x700068u
#define GSU_IN_6A 0x70006Au
#define GSU_DEST 0x70002Cu
#define GSU_IN_A2 0x7000A2u

static void cf42_inputs(uint16_t in68, uint16_t dest)
{
    wr16(GSU_IN_68, in68);
    wr16(GSU_DEST, dest);
    wr16(GSU_IN_A2, 0x0000);
}

static uint8_t cf42(uint16_t in68, uint16_t dest, uint16_t in6a)
{
    cf42_inputs(in68, dest);
    wr16(GSU_IN_6A, in6a);
    return GsuRun(0x01, 0xCF42);
}

static void abi_cf42(uint16_t dest)
{
    uint16_t x = cpu.x;
    uint8_t mode = cf42(cpu.a, dest, x);

    cpu.a = (uint16_t)((x & 0xFF00) | mode);
    cpu.x = 0xCF42;
    cpu.p = (uint8_t)(nz8(cpu.p, cpu.db) | P_M);
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

void abi_GsuCF42ToC000(void)
{
    abi_cf42(0xC000);
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

void abi_GsuCF42To4000(void)
{
    abi_cf42(0x4000);
}

/*
 * @routine 03:EC01 GsuCF42To3000
 * WHAT IT DOES: As GsuCF42ToC000, drawing to $3000.
 * HOW: As GsuCF42ToC000.
 * Entry: as GsuCF42ToC000. Exit: as GsuCF42ToC000.
 */
uint8_t GsuCF42To3000(uint16_t in68, uint16_t in6a)
{
    return cf42(in68, 0x3000, in6a);
}

void abi_GsuCF42To3000(void)
{
    abi_cf42(0x3000);
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

    cf42_inputs(rd16(b | in), 0x3000);
    wr8(GSU_IN_6A, rd8(b | (uint16_t)(in + 2)));
    GsuRun(0x01, 0xCF42);
    wr8(VMAIN, rd8(b | out));
    wr8(VMADDL, rd8(b | (uint16_t)(out + 1)));
    wr8(VMADDH, rd8(b | (uint16_t)(out + 2)));
    wr8(DMAP0, rd8(b | (uint16_t)(out + 3)));
    wr8(BBAD0, rd8(b | (uint16_t)(out + 4)));
    wr8(A1T0L, 0x00);
    wr8(A1T0H, 0x30);
    wr8(A1B0, 0x70);
    wr8(DAS0L, rd8(b | (uint16_t)(out + 5)));
    wr8(DAS0H, rd8(b | (uint16_t)(out + 6)));
    wr8(MDMAEN, 0x01);
}

void abi_GsuCF42ToVram(void)
{
    GsuCF42ToVram(cpu.db, cpu.x, cpu.y);
    cpu.a = 0x0001;
    cpu.x = cpu.y;
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z | P_X)) | P_M);
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
    uint32_t b = (uint32_t)bank << 16;

    wr16(GSU_IN_68, sub16(rd16(0x000004), rd16(b | (uint16_t)(0x200C + obj)), p));
    wr16(0x70002E, sub16(rd16(b | 0x05F2), rd16(b | (uint16_t)(0x2010 + obj)), p));
    GsuRun(0x01, 0x80BC);
    return rd16(GSU_IN_6A);
}

void abi_ObjectGsu80BC(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectGsu80BC(cpu.db, cpu.x, &p);
    cpu.p = (uint8_t)(nz16(p, cpu.a) & ~(P_M | P_X));
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

    wr16(GSU_IN_68, rd16(0x000002));
    wr16(0x70002C, rd16(0x000008));
    wr16(0x70002E, rd16(0x000094));
    GsuRun(0x01, 0x82CE);
    for (i = 0; i < 3; i++) {
        r = rd16(0x700026 + 2 * i);
        wr16(0x0000B7 + 2 * i, r);
    }
    return r;
}

void abi_Gsu82CE(void)
{
    cpu.a = Gsu82CE();
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

    for (i = 0; i < 3; i++)
        wr16(0x700020 + 2 * i, rd16(b | (0x065E + 2 * i)));
    GsuRun(0x01, 0x8325);
    for (i = 0; i < 9; i++) {
        v = rd16(0x7000E4 + 2 * i);
        wr16(b | (0x0608 + 2 * i), v);
    }
    return v;
}

void abi_Gsu8325(void)
{
    cpu.a = Gsu8325(cpu.db);
    cpu.x = 0x0025;
    cpu.y &= 0x00FF;
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) | P_M | P_X);
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

    for (i = 0; i < 8; i++)
        wr16(0x703C00 + 2 * i, (uint16_t)(0x80 + 0x10 * i));
    return GsuRun(0x07, 0xEE88);
}

void abi_GsuEE88Ramp(void)
{
    cpu.a = GsuEE88Ramp();
    cpu.x = 0x0088;
    cpu.y &= 0x00FF;
    cpu.p = (uint8_t)((nz8(cpu.p, cpu.db) & ~P_M) | P_X);
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
    GsuParam216B(bank, (uint16_t)((entry & 0x000F) << 1));
    return GsuRun(0x07, 0xF329);
}

void abi_GsuF329Entry(void)
{
    uint16_t index = (uint16_t)((cpu.a & 0x000F) << 1);
    uint16_t hi = (uint16_t)(rd16((uint32_t)cpu.db << 16 | (uint16_t)(0x8E52 + index)) & 0xFF00);
    uint8_t mode = GsuF329Entry(cpu.db, cpu.a);

    cpu.a = (uint16_t)(hi | mode);
    cpu.x = 0xF329;
    cpu.p = (uint8_t)(nz8(cpu.p, cpu.db) & ~P_C);
}

/* Program $07:F441 for a player: its kind decides the bytes at $70:0200
 * and the word at $70:0216; $70:020A = $74 and $70:020C = $3C. */
static uint8_t f441(uint8_t kind)
{
    if ((kind & 0x0F) == 1) {
        GsuBytes200_0F050E();
        wr16(0x700216, 0x080E);
    } else {
        GsuBytes200_0F080E();
        wr16(0x700216, 0x0A0E);
    }
    wr16(0x70020A, 0x0074);
    wr16(0x70020C, 0x003C);
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
    return f441(rd8((uint32_t)bank << 16 | 0x0DF4));
}

void abi_GsuF441Player1(void)
{
    cpu.a = GsuF441Player1(cpu.db);
    if (cpu.p & P_X)
        cpu.y &= 0x00FF;
    cpu.x = cpu.p & P_X ? 0x0041 : 0xF441;  /* the final pull restores X's width */
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
    return f441(rd8((uint32_t)bank << 16 | 0x0DF5));
}

void abi_GsuF441Player2(void)
{
    cpu.a = GsuF441Player2(cpu.db);
    if (cpu.p & P_X)
        cpu.y &= 0x00FF;
    cpu.x = cpu.p & P_X ? 0x0041 : 0xF441;  /* the final pull restores X's width */
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

    for (i = 0; i < 3; i++)
        wr16(b | (0x0664 + 2 * i), 0x0000);
    for (i = 0; i < 3; i++)
        wr16(b | (0x065E + 2 * i), rd16(b | (0x0664 + 2 * i)));
    Gsu8325(bank);
    for (i = 0; i < 9; i++) {
        v = rd16(b | (0x0608 + 2 * i));
        wr16(b | (0x064C + 2 * i), v);
    }
    for (i = 0; i < 9; i++)
        wr8(b | high[i], (i % 4) == 0 ? 0x7F : 0x00);
    for (i = 0; i < 9; i++)
        wr8(b | (uint16_t)(0x063A + 2 * i), 0x00);
    return (uint16_t)((v & 0xFF00) | 0x7F);
}

void abi_ViewReset(void)
{
    cpu.a = ViewReset(cpu.db);
    cpu.x = 0x0025;
    cpu.y &= 0x00FF;
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

    wr8(0x0000E6, rd8(b | 0x1A6C));
    wr8(0x0000E7, 0xA7);
    wr8(0x0000E8, 0x55);
    wr8(0x0000E9, 0x7F);
    for (i = 0; i < 8; i++)
        wr8(dp[i], 0x00);
    for (i = 0; i < 5; i++)
        wr8(b | clear[i], 0x00);
    wr8(b | 0x05FA, 0x00);
    ViewReset(bank);
    ReadTable9022(bank);
    return GsuParamsFromDp(bank);
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

    wr16(0x7001D0, 0x1234);
    wr16(0x7001C4, 0x0001);
    wr16(0x70108C, 0x0000);
    wr8(0x0000A0, 0x80);
    wr8((uint32_t)bank << 16 | 0x05EF, 0xFF);
    wr8(0x000000, 0x00);
    for (i = 0; i < 16; i++)
        wr8(dp[i][0], dp[i][1]);
    return view_setup_tail(bank);
}

void abi_ViewDefaults(void)
{
    cpu.a = ViewDefaults(cpu.db);
    cpu.x = 0x0080;
    cpu.y = 0x0000;
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
uint8_t GsuF0A5OrF0B4(uint8_t bank, uint16_t input)
{
    wr16(0x70020C, input);
    if (rd8((uint32_t)bank << 16 | 0x0D62)) {
        GsuBytes200_0F090A();
        return GsuRun(0x07, 0xF0A5);
    }
    GsuBytes200_BFBCBD();
    return GsuRun(0x07, 0xF0B4);
}

void abi_GsuF0A5OrF0B4(void)
{
    uint8_t set = rd8((uint32_t)cpu.db << 16 | 0x0D62);

    cpu.a = GsuF0A5OrF0B4(cpu.db, cpu.a);
    cpu.x = set ? 0x00A5 : 0x00B4;
    cpu.y &= 0x00FF;
    cpu.p = (uint8_t)((nz8(cpu.p, cpu.db) & ~P_M) | P_X);
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
uint8_t GsuEEF5OrEFCD(uint8_t bank, uint16_t input)
{
    wr16(0x70020C, input);
    if (rd8((uint32_t)bank << 16 | 0x0D62)) {
        GsuBytes200_0F0405();
        return GsuRun(0x07, 0xEEF5);
    }
    GsuBytes200_BFB7B6();
    return GsuRun(0x07, 0xEFCD);
}

void abi_GsuEEF5OrEFCD(void)
{
    uint8_t set = rd8((uint32_t)cpu.db << 16 | 0x0D62);

    cpu.a = GsuEEF5OrEFCD(cpu.db, cpu.a);
    cpu.x = set ? 0xEEF5 : 0xEFCD;
    cpu.p = (uint8_t)(nz8(cpu.p, cpu.db) & ~(P_M | P_X));
}

uint16_t GsuParam216A(uint8_t bank, uint16_t index);

/* Runs program $07:F15B, which draws with the inputs at $70:020A-$70:0216. */
static uint8_t f15b(void)
{
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
uint8_t GsuDigitPair0E74(uint8_t bank)
{
    uint16_t v = rd16((uint32_t)bank << 16 | 0x0E74);

    wr16(0x70020E, 0x0010);
    wr16(0x70020A, 0x0004);
    wr16(0x70020C, 0x0008);
    GsuParam216A(bank, (uint16_t)(((v >> 4) & 0x000F) << 1));
    f15b();
    GsuParam216A(bank, (uint16_t)((v & 0x000F) << 1));
    return f15b();
}

void abi_GsuDigitPair0E74(void)
{
    uint16_t index = (uint16_t)((rd8((uint32_t)cpu.db << 16 | 0x0E74) & 0x0F) << 1);
    uint16_t hi = (uint16_t)(rd16((uint32_t)cpu.db << 16 | (uint16_t)(0x8E3E + index)) & 0xFF00);
    uint8_t mode = GsuDigitPair0E74(cpu.db);

    cpu.a = (uint16_t)(hi | mode);
    cpu.x = 0xF15B;
    cpu.p = (uint8_t)(nz8(cpu.p, cpu.db) & ~(P_C | P_M | P_X));
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
    wr16(0x700210, 0x0020);
    wr16(0x70020E, 0x0010);
    wr16(0x70020A, 0x0030);
    wr16(0x70020C, 0x0018);
    GsuBytes200_0F0405();
    wr16(0x700216, 0x9000);
    f15b();
    wr16(0x700216, 0x8808);
    f15b();
    wr16(0x700216, 0x881C);
    return f15b();
}

void abi_GsuF15BThree(void)
{
    cpu.a = (uint16_t)(0x8800 | GsuF15BThree());
    cpu.x = 0xF15B;
    cpu.p = (uint8_t)(nz8(cpu.p, cpu.db) & ~(P_M | P_X));
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
int GsuFA1BFromTables(uint8_t bank, uint8_t *mode, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t y = rd16(b | 0x0D6F), s = (uint16_t)(y + 8), x, v;

    GsuBytes200_BFBCBD();
    wr16(0x70020A, rd16(b | 0x0D6D));
    *p = (uint8_t)((*p & ~(P_C | P_V)) | (y > 0xFFF7 ? P_C : 0) | ((~y & s & 0x8000) ? P_V : 0));
    wr16(0x70020C, s);
    wr16(0x700210, 0x0100);
    wr16(0x700214, 0x0100);
    x = rd16(b | 0x0D6B);
    v = rd16(0x070000 | (uint16_t)(0x97D6 + x));
    if (!v)
        return 0;
    wr16(0x70020E, v);
    wr16(0x700212, rd16(0x070000 | (uint16_t)(0x97E2 + x)));
    wr16(0x700216, 0x1E00);
    *mode = GsuRun(0x07, 0xFA1B);
    return 1;
}

void abi_GsuFA1BFromTables(void)
{
    uint16_t x = rd16((uint32_t)cpu.db << 16 | 0x0D6B);
    uint8_t p = cpu.p, mode = 0;

    if (GsuFA1BFromTables(cpu.db, &mode, &p)) {
        cpu.a = (uint16_t)(0x1E00 | mode);
        cpu.x = 0xFA1B;
        cpu.p = (uint8_t)(nz8(p, cpu.db) & ~(P_M | P_X));
    } else {
        cpu.a = 0x0000;
        cpu.x = x;
        cpu.p = (uint8_t)((p & ~P_N) | P_Z);
    }
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
    wr16(GSU_IN_68, rd16(0x000002));
    wr16(GSU_DEST, rd16(0x000008));
    GsuRun(0x01, 0x81E3);
    return rd16(0x700040);
}

void abi_Gsu81E3(void)
{
    cpu.a = Gsu81E3();
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

    for (i = 0; i < 3; i++)
        wr16(b | (0x065E + 2 * i), rd16(b | (0x0664 + 2 * i)));
    Gsu8325(bank);
    for (i = 0; i < 9; i++) {
        v = rd16(b | (0x0608 + 2 * i));
        wr16(b | (0x064C + 2 * i), v);
    }
    return v;
}

void abi_ViewMatrixUpdateAndSave(void)
{
    cpu.a = ViewMatrixUpdateAndSave(cpu.db);
    cpu.x = 0x0025;
    cpu.y &= 0x00FF;
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) | P_M | P_X);
}

void abi_Gsu81E3(void);

/*
 * @routine 02:E016 Gsu81E3Long
 * WHAT IT DOES: Gsu81E3 for callers in other banks.
 * HOW: A near call to Gsu81E3, then 16-bit A and a long return.
 * Entry: as Gsu81E3. Exit: as Gsu81E3, with M 16-bit.
 */
uint16_t Gsu81E3Long(void)
{
    return Gsu81E3();
}

void abi_Gsu81E3Long(void)
{
    abi_Gsu81E3();
    cpu.p = (uint8_t)(cpu.p & ~P_M);
}

void abi_ViewMatrixUpdateAndSave(void);
void abi_GsuRun(void);

/*
 * @routine 02:DB59 ViewUpdateAndGsuAEF8
 * WHAT IT DOES: Updates the view matrix (ViewMatrixUpdateAndSave), then
 * runs Super FX program $01:AEF8.
 * HOW: Two long calls; GsuRun with A = 1, X = $AEF8.
 * Entry: D = $0000. Exit: A low byte = the screen mode value; X = $AEF8;
 * Y high byte clear; M 8-bit, X 16-bit; N, Z from the data bank.
 */
uint8_t ViewUpdateAndGsuAEF8(uint8_t bank)
{
    ViewMatrixUpdateAndSave(bank);
    return GsuRun(0x01, 0xAEF8);
}

void abi_ViewUpdateAndGsuAEF8(void)
{
    abi_ViewMatrixUpdateAndSave();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.x = 0xAEF8;
    cpu.p = (uint8_t)(nz16((uint8_t)((cpu.p | P_M) & ~P_X), 0xAEF8));
    abi_GsuRun();
    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
}

/*
 * @routine 07:8AA6 GsuF329Sequence
 * WHAT IT DOES: Runs Super FX program $07:F329 seven times: for each of the
 * five bytes at Y (through the data bank) via GsuF329Entry, with two runs
 * between them that set $70:0216 = $980A (after the first byte) and $980C
 * (after the third).
 * HOW: GsuF329Entry and GsuRun.
 * Entry: M 16-bit, X 16-bit, D = $0000; Y = the bytes. Exit: as
 * GsuF329Entry for the fifth byte; Y unchanged.
 */
uint8_t GsuF329Sequence(uint8_t bank, uint16_t y)
{
    uint32_t b = (uint32_t)bank << 16;

    GsuF329Entry(bank, rd8(b | y));
    wr16(0x700216, 0x980A);
    GsuRun(0x07, 0xF329);
    GsuF329Entry(bank, rd8(b | (uint16_t)(y + 1)));
    GsuF329Entry(bank, rd8(b | (uint16_t)(y + 2)));
    wr16(0x700216, 0x980C);
    GsuRun(0x07, 0xF329);
    GsuF329Entry(bank, rd8(b | (uint16_t)(y + 3)));
    return GsuF329Entry(bank, rd8(b | (uint16_t)(y + 4)));
}

void abi_GsuF329Sequence(void)
{
    uint16_t index = (uint16_t)((rd8((uint32_t)cpu.db << 16 | (uint16_t)(cpu.y + 4)) & 0x0F) << 1);
    uint16_t hi = (uint16_t)(rd16((uint32_t)cpu.db << 16 | (uint16_t)(0x8E52 + index)) & 0xFF00);
    uint8_t mode = GsuF329Sequence(cpu.db, cpu.y);

    cpu.a = (uint16_t)(hi | mode);
    cpu.x = 0xF329;
    cpu.p = (uint8_t)(nz8(cpu.p, cpu.db) & ~P_C);
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

    wr16(0x700210, 0x0020);
    wr16(0x70020E, 0x0010);
    wr16(0x70020A, 0x0018);
    wr16(0x70020C, 0x0018);
    GsuBytes200_0F090A();
    for (i = 0; i < 5; i++) {
        wr16(0x700216, in216[i]);
        mode = f15b();
    }
    return mode;
}

void abi_GsuF15BFive(void)
{
    cpu.a = (uint16_t)(0x8800 | GsuF15BFive());
    cpu.x = 0xF15B;
    cpu.p = (uint8_t)(nz8(cpu.p, cpu.db) & ~(P_M | P_X));
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
    uint32_t b = (uint32_t)bank << 16;
    uint16_t y;

    wr16(0x70020E, 0x0100);
    wr16(0x700212, 0x0100);
    wr16(0x700210, 0x0100);
    wr16(0x700214, 0x0100);
    wr16(0x70020A, (uint16_t)(rd16(b | 0x0D6D) - 8));
    wr16(0x70020C, rd16(b | 0x0D6F));
    wr16(0x700216, 0x1A10);
    GsuRun(0x07, 0xFA5A);
    wr16(0x70020A, (uint16_t)(rd16(b | 0x0D6D) - 8));
    wr16(0x70020C, (uint16_t)(rd16(b | 0x0D6F) + 0x18));
    wr16(0x700216, 0x1B10);
    GsuRun(0x07, 0xFA5A);
    wr16(0x700210, 0x0200);
    wr16(0x700214, 0x0080);
    wr16(0x70020A, (uint16_t)(rd16(b | 0x0D6D) - 8));
    y = rd16(b | 0x0D6F);
    *p = (uint8_t)((*p & ~(P_C | P_V)) | (y > 0xFFF7 ? P_C : 0) | ((~y & (uint16_t)(y + 8) & 0x8000) ? P_V : 0));
    wr16(0x70020C, (uint16_t)(y + 8));
    wr16(0x700216, 0x1A18);
    return GsuRun(0x07, 0xFA5A);
}

void abi_GsuFA5AThree(void)
{
    uint8_t p = cpu.p, mode = GsuFA5AThree(cpu.db, &p);

    cpu.a = (uint16_t)(0x1A00 | mode);
    cpu.x = 0xFA5A;
    cpu.p = nz8(p, cpu.db);
}

/* A 16-bit ADC with carry clear: the sum, with C and V set as the 65816 sets them. */
static uint16_t add16(uint16_t a, uint16_t m, uint8_t *p)
{
    uint16_t r = (uint16_t)(a + m);

    *p = (uint8_t)((*p & ~(P_C | P_V)) | ((unsigned)a + m > 0xFFFF ? P_C : 0)
                   | ((~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0));
    return r;
}

/* Program $07:FA5A at ($70:020A, $70:020C) with piece $70:0216. */
static uint8_t fa5a(uint16_t x, uint16_t y, uint16_t piece)
{
    wr16(0x70020A, x);
    wr16(0x700216, piece);
    wr16(0x70020C, y);
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
    uint32_t b = (uint32_t)bank << 16;
    uint16_t i = rd16(b | 0x0D6B), y;

    wr16(0x70020E, rd16(0x070000 | (uint16_t)(0x97BE + i)));
    wr16(0x700212, rd16(0x070000 | (uint16_t)(0x97CA + i)));
    wr16(0x700216, 0x1B18);
    wr16(0x70020A, rd16(b | 0x0D6D));
    wr16(0x70020C, rd16(b | 0x0D6F));
    wr16(0x700210, 0x0100);
    wr16(0x700214, 0x0100);
    GsuRun(0x07, 0xFA5A);
    wr16(0x70020A, rd16(b | 0x0D6D));
    wr16(0x70020C, add16(rd16(b | 0x0D6F), 0x0018, p));
    wr16(0x700210, 0x0100);
    wr16(0x700214, 0x0100);
    wr16(0x700216, 0x1D18);
    GsuRun(0x07, 0xFA5A);
    y = add16(rd16(b | 0x0D6F), 0x0008, p);
    wr16(0x70020C, y);
    wr16(0x70020A, rd16(b | 0x0D6D));
    wr16(0x700216, 0x1A55);
    wr16(0x700210, 0x0200);
    wr16(0x700214, 0x0080);
    return GsuRun(0x07, 0xFA5A);
}

void abi_GsuFA5AFromTables(void)
{
    uint8_t p = cpu.p, mode = GsuFA5AFromTables(cpu.db, &p);

    cpu.a = mode;
    cpu.x = 0xFA5A;
    cpu.p = nz8(p, cpu.db);
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
    static const uint16_t dy[6] = {0x0000, 0x0008, 0x0010, 0x0018, 0x0008, 0x0010};
    static const uint16_t piece[6] = {0x1A0E, 0x1A16, 0x1B16, 0x1B0E, 0x1A0E, 0x1B0E};
    uint32_t b = (uint32_t)bank << 16;
    uint16_t x, y;
    uint8_t mode = 0;
    unsigned i;

    wr16(0x70020E, 0x0100);
    wr16(0x700212, 0x0100);
    wr16(0x700210, 0x0100);
    wr16(0x700214, 0x0100);
    x = add16(rd16(0x070000 | (uint16_t)(0x9806 + rd16(b | 0x0D6B))), rd16(b | 0x0D6D), p);
    wr16(0x0000F1, x);
    for (i = 0; i < 6; i++) {
        if (i == 4) {
            x = add16(rd16(0x0000F1), 0x0008, p);
            wr16(0x0000F1, x);
        }
        y = rd16(b | 0x0D6F);
        if (dy[i])
            y = add16(y, dy[i], p);
        if (i == 0) {
            mode = fa5a(x, y, piece[i]);
        } else {
            wr16(0x70020A, x);
            wr16(0x70020C, y);
            wr16(0x700216, piece[i]);
            mode = GsuRun(0x07, 0xFA5A);
        }
    }
    return mode;
}

void abi_GsuFA5ASix(void)
{
    uint8_t p = cpu.p, mode = GsuFA5ASix(cpu.db, &p);

    cpu.a = (uint16_t)(0x1B00 | mode);
    cpu.x = 0xFA5A;
    cpu.p = nz8(p, cpu.db);
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

    wr16(0x700210, input);
    wr16(0x70020E, 0x0010);
    wr16(0x70020A, 0x0050);
    wr16(0x70020C, 0x0008);
    GsuBytes200_BFB7B6();
    for (i = 0; i < 3; i++) {
        wr16(0x700216, first[i]);
        f15b();
    }
    wr16(0x70020A, 0x0000);
    wr16(0x70020C, 0x002C);
    for (i = 0; i < 13; i++) {
        wr16(0x700216, second[i]);
        mode = f15b();
    }
    return mode;
}

void abi_GsuF15BTwoLines(void)
{
    cpu.a = (uint16_t)(0x8C00 | GsuF15BTwoLines(cpu.a));
    cpu.x = 0xF15B;
    cpu.p = (uint8_t)(nz8(cpu.p, cpu.db) & ~(P_M | P_X));
}

void GsuBytes200_BFBAB9(void);

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
    uint16_t n = (uint16_t)(rd16(b | 0x10CB) + 1);
    uint8_t mode = 0;
    unsigned i;

    if (n >= 0x0014)
        n = 0x0014;
    wr16(b | 0x10CB, n);
    wr16(0x70020E, 0x0010);
    wr16(0x70020A, 0x0010);
    wr16(0x70020C, 0x0028);
    GsuBytes200_BFBAB9();
    wr16(0x700210, rd8(b | (uint16_t)(0x85CC + n)));
    *program = n < 8 ? 0xF15B : 0xF121;
    for (i = 0; i < 11; i++) {
        wr16(0x700216, piece[i]);
        mode = GsuRun(0x07, *program);
        if (i == 0)
            wr16(0x70020A, add16(rd16(0x70020A), 0x0004, p));
    }
    return mode;
}

void abi_GsuPiecesStep10CB(void)
{
    uint8_t p = cpu.p, mode = GsuPiecesStep10CB(cpu.db, &cpu.x, &p);

    cpu.a = (uint16_t)(0x8C00 | mode);
    cpu.p = nz8(p, cpu.db);
}

void abi_Gsu81E3Long(void);
void abi_ObjectGsu80BC(void);

/* A - B as SEC / SBC does it, leaving C and V in cpu.p. */
static uint16_t sbc16(uint16_t a, uint16_t b)
{
    uint16_t r = (uint16_t)(a - b);

    cpu.p = (uint8_t)((cpu.p & ~(P_C | P_V)) | (a >= b ? P_C : 0) | (((a ^ b) & (a ^ r) & 0x8000) ? P_V : 0));
    return r;
}

/* The exit of the two routines below: X, Y restored (N, Z from X), M
   16-bit. */
static void restore_xy(uint16_t x, uint16_t y)
{
    cpu.y = y;
    cpu.x = x;
    cpu.p = (uint8_t)(nz16(cpu.p & ~P_M, x));
}

/*
 * @routine 03:C998 ObjectGsu80BCThen81E3
 * WHAT IT DOES: For object X, runs program $01:80BC (ObjectGsu80BC) and
 * keeps its result in direct page $08, puts $000A (data bank) - the
 * object's +$0E in $02, then runs Gsu81E3Long.
 * HOW: 16-bit subtract; X and Y are saved and restored.
 * Entry: X 16-bit, D = $0000; X = object. Exit: A as Gsu81E3Long leaves it;
 * X, Y unchanged; M 16-bit; N, Z from X.
 */
void ObjectGsu80BCThen81E3(void)
{
    uint16_t x = cpu.x, y = cpu.y;
    uint32_t b = (uint32_t)cpu.db << 16;

    cpu.p &= (uint8_t)~P_M;
    abi_ObjectGsu80BC();
    wr16(0x000008, cpu.a);
    cpu.a = sbc16(rd16(b | 0x000A), rd16(b | (uint16_t)(0x200E + x)));
    wr16(0x000002, cpu.a);
    abi_Gsu81E3Long();
    restore_xy(x, y);
}

void abi_ObjectGsu80BCThen81E3(void)
{
    ObjectGsu80BCThen81E3();
}

/*
 * @routine 03:C9B4 ObjectGsu81E3
 * WHAT IT DOES: For object X, puts direct page $04 - its +$0C in $02 and
 * $05F2 - its +$10 in $08, then runs Gsu81E3Long.
 * HOW: 16-bit subtracts through the data bank; X and Y saved and restored.
 * Entry: X 16-bit, D = $0000; X = object. Exit: as ObjectGsu80BCThen81E3.
 */
void ObjectGsu81E3(void)
{
    uint16_t x = cpu.x, y = cpu.y;
    uint32_t b = (uint32_t)cpu.db << 16;

    cpu.p &= (uint8_t)~P_M;
    wr16(0x000002, sbc16(rd16(0x000004), rd16(b | (uint16_t)(0x200C + x))));
    cpu.a = sbc16(rd16(b | 0x05F2), rd16(b | (uint16_t)(0x2010 + x)));
    wr16(0x000008, cpu.a);
    abi_Gsu81E3Long();
    restore_xy(x, y);
}

void abi_ObjectGsu81E3(void)
{
    ObjectGsu81E3();
}

/* Program $07:FA5A with all four of $70:020E/0210/0212/0214 = $100. */
static void fa5a_unit_scale(void)
{
    wr16(0x70020E, 0x0100);
    wr16(0x700212, 0x0100);
    wr16(0x700210, 0x0100);
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
    uint32_t b = (uint32_t)bank << 16;
    uint16_t x;

    fa5a_unit_scale();
    x = add16(rd16(0x079806 + rd16(b | 0x0D6B)), rd16(b | 0x0D6D), p);
    wr16(0x0000F1, x);
    wr16(0x70020A, x);
    wr16(0x70020C, rd16(b | 0x0D6F));
    wr16(0x700216, 0x1A12);
    GsuRun(0x07, 0xFA5A);
    wr16(0x70020A, rd16(0x0000F1));
    wr16(0x70020C, add16(rd16(b | 0x0D6F), 0x0018, p));
    wr16(0x700216, 0x1B12);
    GsuRun(0x07, 0xFA5A);
    wr16(0x700210, 0x0200);
    wr16(0x700214, 0x0080);
    wr16(0x70020A, rd16(0x0000F1));
    wr16(0x70020C, add16(rd16(b | 0x0D6F), 0x0008, p));
    wr16(0x700216, 0x1C18);
    return GsuRun(0x07, 0xFA5A);
}

void abi_GsuFA5AThreeShifted(void)
{
    uint8_t p = cpu.p, mode = GsuFA5AThreeShifted(cpu.db, &p);

    cpu.a = (uint16_t)(0x1C00 | mode);
    cpu.x = 0xFA5A;
    cpu.p = nz8(p, cpu.db);
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
    static const uint16_t dy[6] = {0x0000, 0x0008, 0x0010, 0x0018, 0x0008, 0x0010};
    static const uint16_t dx[6] = {0xFFF8, 0xFFF8, 0xFFF8, 0xFFF8, 0xFFF0, 0xFFF0};
    static const uint16_t piece[6] = {0x1A0C, 0x1A14, 0x1B14, 0x1B0C, 0x1A0C, 0x1B0C};
    uint32_t b = (uint32_t)bank << 16;
    uint8_t mode = 0;
    unsigned i;

    fa5a_unit_scale();
    for (i = 0; i < 6; i++) {
        uint16_t y = rd16(b | 0x0D6F);

        wr16(0x70020C, dy[i] ? add16(y, dy[i], p) : y);
        wr16(0x70020A, add16(rd16(b | 0x0D6D), dx[i], p));
        wr16(0x700216, piece[i]);
        mode = GsuRun(0x07, 0xFA5A);
    }
    return mode;
}

void abi_GsuFA5ASixLeft(void)
{
    uint8_t p = cpu.p, mode = GsuFA5ASixLeft(cpu.db, &p);

    cpu.a = (uint16_t)(0x1B00 | mode);
    cpu.x = 0xFA5A;
    cpu.p = nz8(p, cpu.db);
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
int GsuFA35FromTables(uint8_t bank, uint8_t *mode, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t x, v;

    GsuBytes200_BFBCBD();
    wr16(0x70020A, rd16(b | 0x0D6D));
    wr16(0x70020C, add16(rd16(b | 0x0D6F), 0x0008, p));
    wr16(0x700210, 0x0100);
    wr16(0x700214, 0x0100);
    x = rd16(b | 0x0D6B);
    v = rd16(0x070000 | (uint16_t)(0x97EE + x));
    if (!v)
        return 0;
    wr16(0x70020E, v);
    wr16(0x700212, rd16(0x070000 | (uint16_t)(0x97FA + x)));
    wr16(0x700216, 0x1C00);
    *mode = GsuRun(0x07, 0xFA35);
    return 1;
}

void abi_GsuFA35FromTables(void)
{
    uint16_t x = rd16((uint32_t)cpu.db << 16 | 0x0D6B);
    uint8_t p = cpu.p, mode = 0;

    if (GsuFA35FromTables(cpu.db, &mode, &p)) {
        cpu.a = (uint16_t)(0x1C00 | mode);
        cpu.x = 0xFA35;
        cpu.p = (uint8_t)(nz8(p, cpu.db) & ~(P_M | P_X));
    } else {
        cpu.a = 0x0000;
        cpu.x = x;
        cpu.p = (uint8_t)((p & ~P_N) | P_Z);
    }
}

void abi_GsuBytes200_0F060E(void);
void abi_GsuRun(void);

/*
 * @routine 07:9AB0 HudSixGlyphs
 * WHAT IT DOES: Draws six HUD glyphs with Super FX program $07:F15B: the
 * input bytes 0, $0F, $06, $0E (GsuBytes200_0F060E), $20 rows, an advance
 * of $10, starting at X $10, Y $18; the glyph words $8C10, $8C04, $8814,
 * $8408, $8C10 and $8804 in turn at $70:0216.
 * HOW: Long word stores to Super FX RAM, then GsuRun per glyph.
 * Entry: D = $0000. Exit: as the last GsuRun leaves it, with M and X 16-bit.
 */
void abi_HudSixGlyphs(void)
{
    static const uint16_t glyph[6] = {0x8C10, 0x8C04, 0x8814, 0x8408, 0x8C10, 0x8804};
    unsigned i;

    cpu.p &= (uint8_t)~(P_M | P_X);
    abi_GsuBytes200_0F060E();
    wr16(0x700210, 0x0020);
    wr16(0x70020E, 0x0010);
    wr16(0x70020A, 0x0010);
    wr16(0x70020C, 0x0018);
    for (i = 0; i < 6; i++) {
        cpu.p &= (uint8_t)~(P_M | P_X);
        cpu.a = glyph[i];
        wr16(0x700216, glyph[i]);
        cpu.p |= P_M;
        cpu.x = 0xF15B;
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x07);
        abi_GsuRun();
    }
    cpu.p &= (uint8_t)~(P_M | P_X);
}

void abi_GsuBytes200_BFB5BB(void);
void abi_GsuBytes200_BFBCBD(void);

/* $70:0216 = src, then Super FX program $07:FA88 (FxSprite24x40) through
   GsuRun; M and X 16-bit after. */
static void sprite24x40(uint16_t src)
{
    cpu.p &= (uint8_t)~P_M;
    cpu.a = src;
    wr16(0x700216, src);
    cpu.p = (uint8_t)((nz16(cpu.p, src) | P_M) & ~P_X);
    cpu.x = 0xFA88;
    cpu.a = (uint16_t)((src & 0xFF00) | 0x07);
    abi_GsuRun();
    cpu.p &= (uint8_t)~(P_M | P_X);
}

/* The common start: the renderer bytes (setter), then, with $0E5C set,
   07:9EE9 instead (a jump; not reached in the traces so far). Returns 0
   when the jump was taken. */
static int big_sprites_start(void (*setter)(void))
{
    uint8_t v;

    cpu.p &= (uint8_t)~P_M;
    setter();
    cpu.p |= P_M;
    v = rd8((uint32_t)cpu.db << 16 | 0x0E5C);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (v) {
        call_routine(0x079EE9, 0);
        return 0;
    }
    return 1;
}

/* M and X 16-bit; x, y, and both steps $100 at $70:020A-$70:0214. */
static void big_sprites_at(uint16_t x, uint16_t y)
{
    cpu.p &= (uint8_t)~(P_M | P_X);
    wr16(0x70020A, x);
    wr16(0x70020C, y);
    wr16(0x700212, 0x0100);
    wr16(0x700214, 0x0100);
    cpu.a = 0x0100;
    cpu.p = nz16(cpu.p, 0x0100);
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
void abi_BigSpritesRow6(void)
{
    static const uint16_t first[4] = {0xB000, 0xA506, 0xA518, 0xA00C}, second[2] = {0xB006, 0xAA0C};
    uint16_t w;
    unsigned i;

    if (!big_sprites_start(abi_GsuBytes200_BFB5BB))
        return;
    big_sprites_at(0x0018, 0x0028);
    for (i = 0; i < 4; i++)
        sprite24x40(first[i]);
    w = rd16(0x70020A);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(w, 0x0010, &cpu.p);
    wr16(0x70020A, cpu.a);
    for (i = 0; i < 2; i++)
        sprite24x40(second[i]);
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
void abi_BigSpritesRow5(void)
{
    static const uint16_t src[5] = {0xA006, 0xAA12, 0xA000, 0xAA18, 0xA500};
    unsigned i;

    if (!big_sprites_start(abi_GsuBytes200_BFBCBD))
        return;
    big_sprites_at(0x0020, 0x0028);
    for (i = 0; i < 5; i++)
        sprite24x40(src[i]);
}

void abi_GsuBytes200_BFB3BE(void);
void abi_GsuF329Sequence(void);

/*
 * @routine 07:89E4 HudHoldDraw
 * WHAT IT DOES: When the HUD hold flag $0E5D is set, sets the renderer
 * bytes 0, $BF, $B3, $BE (GsuBytes200_BFB3BE) and draws the sequence at
 * $0F69 at (147, 108) (GsuF329Sequence, through the tail of the routine at
 * $07:89F5).
 * HOW: P saved and restored; 16-bit.
 * Entry: any widths. Exit: P restored; A, X, Y as the draw leaves them
 * (A = $0E5D when not set).
 */
void abi_HudHoldDraw(void)
{
    uint8_t p0 = cpu.p;

    op_rep(P_M | P_X);
    op_lda16((uint32_t)cpu.db << 16 | 0x0E5D);
    if (!cpu.a) {
        op_plp(p0);
        return;
    }
    abi_GsuBytes200_BFB3BE();
    op_lda16_imm(0x0093);
    wr16(0x70020A, 0x0093);
    op_lda16_imm(0x006C);
    wr16(0x70020C, 0x006C);
    cpu.y = 0x0F69;
    cpu.p = nz16(cpu.p, cpu.y);
    abi_GsuF329Sequence();
    op_plp(p0);
}

/* SEP #$20, REP #$10, LDX #$F329, LDA #$07, JSL GsuRun, REP #$20, REP #$10:
   Super FX program $07:F329 (the glyph at $70:0216 drawn at $70:020A,
   $70:020C). */
static void run_f329(void)
{
    op_sep(P_M);
    op_rep(P_X);
    cpu.x = 0xF329;
    cpu.p = nz16(cpu.p, cpu.x);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x07);
    cpu.p = nz8(cpu.p, 0x07);
    abi_GsuRun();
    op_rep(P_M | P_X);
}

/*
 * @routine 04:DDB0 GlyphAtY60
 * WHAT IT DOES: Draws glyph A at ($70:020A, 96) with the renderer bytes 0,
 * $BF, $B5, $BB (GsuBytes200_BFB5BB) and program $07:F329.
 * HOW: Long stores to the program's parameters, then GsuRun.
 * Entry: M 16-bit, X 16-bit; A = the glyph. Exit: M 16-bit, X 16-bit; X =
 * $F329; A as GsuRun leaves it.
 */
void abi_GlyphAtY60(void)
{
    wr16(0x700216, cpu.a);
    op_lda16_imm(0x0060);
    wr16(0x70020C, 0x0060);
    abi_GsuBytes200_BFB5BB();
    run_f329();
}

/* One step of the row at y 96: LDA #x, STA.L $70:020A, LDA #glyph, JSR
   GlyphAtY60. */
static void glyph_step(uint16_t x, uint16_t glyph)
{
    op_lda16_imm(x);
    wr16(0x70020A, x);
    op_lda16_imm(glyph);
    abi_GlyphAtY60();
}

/* The row of six glyphs from 04:DD61, each entry running the rest. */
static void glyph_row(int from)
{
    static const uint16_t xs[6] = {0x0078, 0x0070, 0x0068, 0x0060, 0x0058, 0x0050};
    static const uint16_t gs[6] = {0x9808, 0x9808, 0x9808, 0x941A, 0x960E, 0x9414};
    int i;

    for (i = from; i < 6; i++)
        glyph_step(xs[i], gs[i]);
}

/*
 * @routine 04:DD61 GlyphRowY60From0
 * WHAT IT DOES: Draws a row of six glyphs at y 96 from x $78 down by 8:
 * $9808 three times, then $941A, $960E and $9414 (GlyphAtY60).
 * HOW: Six steps of LDA #x, STA.L $70:020A, LDA #glyph, JSR; the entries
 * at $04:DD6E, $DD7B, $DD88, $DD95 and $DDA2 start further along.
 * Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
 */
void abi_GlyphRowY60From0(void)
{
    glyph_row(0);
}

/*
 * @routine 04:DD6E GlyphRowY60From1
 * WHAT IT DOES: As GlyphRowY60From0 from its second glyph (x $70).
 * HOW: The same code, entered further along.
 * Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
 */
void abi_GlyphRowY60From1(void)
{
    glyph_row(1);
}

/*
 * @routine 04:DD7B GlyphRowY60From2
 * WHAT IT DOES: As GlyphRowY60From0 from its third glyph (x $68).
 * HOW: The same code, entered further along.
 * Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
 */
void abi_GlyphRowY60From2(void)
{
    glyph_row(2);
}

/*
 * @routine 04:DD88 GlyphRowY60From3
 * WHAT IT DOES: As GlyphRowY60From0 from its fourth glyph (x $60).
 * HOW: The same code, entered further along.
 * Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
 */
void abi_GlyphRowY60From3(void)
{
    glyph_row(3);
}

/*
 * @routine 04:DD95 GlyphRowY60From4
 * WHAT IT DOES: As GlyphRowY60From0 from its fifth glyph (x $58).
 * HOW: The same code, entered further along.
 * Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
 */
void abi_GlyphRowY60From4(void)
{
    glyph_row(4);
}

/*
 * @routine 04:DDA2 GlyphRowY60From5
 * WHAT IT DOES: Draws the row's last glyph, $9414 at x $50 (GlyphAtY60).
 * HOW: The same code, entered at its last step.
 * Entry: M 16-bit, X 16-bit. Exit: as GlyphAtY60.
 */
void abi_GlyphRowY60From5(void)
{
    glyph_row(5);
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
void abi_GlyphListDraw(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t x;
    uint8_t p;

    cpu.x = 0x0000;
    cpu.p = nz16(cpu.p, 0);
    op_lda16(b | 0x10E4);
    op_asl16();
    wr16(0x0000F1, cpu.a);
    op_lda16(b | 0x10E6);
    op_asl16();
    wr16(0x0000F3, cpu.a);
    do {
        op_rep(P_M);
        op_lda16(b | (uint16_t)(0x110E + cpu.x));
        if (cpu.a) {
            op_lda16(b | (uint16_t)(0x10E8 + cpu.x));
            wr16(0x700216, cpu.a);
            op_lda16(b | (uint16_t)(0x1134 + cpu.x));
            op_and16(0x00FF);
            wr16(0x70020A, cpu.a);
            op_lda16(b | (uint16_t)(0x1135 + cpu.x));
            op_and16(0x00FF);
            wr16(0x70020C, cpu.a);
            {
                uint16_t lim = rd16(0x0000F1);

                cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(cpu.x - lim)) & ~P_C) | (cpu.x >= lim ? P_C : 0));
                if (cpu.x >= lim)
                    abi_GsuBytes200_BFB3BE();
                else
                    abi_GsuBytes200_BFBCBD();
            }
            x = cpu.x;
            p = cpu.p;
            run_f329();
            op_plp(p);
            cpu.x = x;
            cpu.p = nz16(cpu.p, x);
        }
        cpu.x = (uint16_t)(cpu.x + 2);
        {
            uint16_t lim = rd16(0x0000F3);

            cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(cpu.x - lim)) & ~P_C) | (cpu.x >= lim ? P_C : 0));
        }
    } while (!(cpu.p & P_C));
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
void abi_GlyphSpriteDraw(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t x = cpu.x;

    op_lda16(b | (uint16_t)(0x0FB0 + x));
    wr16(0x70020A, cpu.a);
    op_lda16(b | (uint16_t)(0x0FE0 + x));
    wr16(0x70020C, cpu.a);
    op_lda16(b | (uint16_t)(0x1010 + x));
    wr16(0x700216, cpu.a);
    run_f329();
    cpu.x = (uint16_t)(x + 2);
    cpu.p = nz16(cpu.p, cpu.x);
}

/*
 * @routine 04:DA3C GlyphSpritesInit
 * WHAT IT DOES: Sets the 16 glyph sprites up: x from the table at
 * $04:DA7A, y 8, glyph from the table at $04:DA8E, $1040 + X = 0; then
 * $10D8 = 0, $115A = $1E, and $0D5B and the HUD hold $0E5D cleared.
 * HOW: A 16-bit loop over X = 0 to $1E by 2; P saved and restored.
 * Entry: any widths. Exit: P restored; A = $001E; X = $20; Y unchanged.
 */
void abi_GlyphSpritesInit(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p;

    op_rep(P_M | P_X);
    cpu.x = 0x0000;
    do {
        op_lda16(0x04DA7A + (uint32_t)cpu.x);
        wr16(b | (uint16_t)(0x0FB0 + cpu.x), cpu.a);
        op_lda16_imm(0x0008);
        wr16(b | (uint16_t)(0x0FE0 + cpu.x), 0x0008);
        op_lda16(0x04DA8E + (uint32_t)cpu.x);
        wr16(b | (uint16_t)(0x1010 + cpu.x), cpu.a);
        wr16(b | (uint16_t)(0x1040 + cpu.x), 0x0000);
        cpu.x = (uint16_t)(cpu.x + 2);
    } while (cpu.x < 0x0020);
    wr16(b | 0x10D8, 0x0000);
    op_lda16_imm(0x001E);
    wr16(b | 0x115A, 0x001E);
    op_sep(P_M);
    wr8(b | 0x0D5B, 0x00);
    wr8(b | 0x0E5D, 0x00);
    op_plp(p0);
}

/* One glyph of the time row: the glyph word, on (+$110E = 0, 16-bit), x,
   y $68, then the next entry (not after the last). */
static void time_glyph(uint32_t b, uint16_t glyph, uint8_t x, int last)
{
    op_rep(P_M);
    cpu.a = glyph;
    cpu.p = nz16(cpu.p, glyph);
    wr16(b | (uint16_t)(0x10E8 + cpu.y), glyph);
    op_lda16_imm(0x0000);
    wr16(b | (uint16_t)(0x110E + cpu.y), 0x0000);
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | x);
    wr8(b | (uint16_t)(0x1134 + cpu.y), x);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x68);
    cpu.p = nz8(cpu.p, 0x68);
    wr8(b | (uint16_t)(0x1135 + cpu.y), 0x68);
    if (!last)
        cpu.y = (uint16_t)(cpu.y + 2);
}

/* A digit glyph: $9400 + digit * 2, the digit read from direct page. */
static uint16_t digit_glyph(uint16_t addr, int high)
{
    op_lda16(addr);
    op_and16(high ? 0x00FF : 0x000F);
    if (high) {
        op_lsr16();
        op_lsr16();
        op_lsr16();
        op_lsr16();
    }
    op_asl16();
    op_add16(0x9400);
    return cpu.a;
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
void abi_GlyphListMessageAndTime(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t v;

    op_lda16(b | (uint16_t)(0xDF6D + cpu.x));
    op_tax16();
    op_lda16(b | cpu.x);
    op_and16(0x00FF);
    wr16(0x0000F1, cpu.a);
    op_asl16();
    op_asl16();
    op_sub16(0x0068);
    cpu.a = (uint16_t)(~cpu.a + 1);
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(0x0000F3, cpu.a);
    op_lda16(0x0000F1);
    cpu.a = (uint16_t)(cpu.a - 1);
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(b | 0x10E4, cpu.a);
    op_add16(0x0007);
    wr16(b | 0x10E6, cpu.a);
    cpu.y = 0x0000;
    cpu.p = nz16(cpu.p, 0);
    do {
        op_lda16(b | (uint16_t)(0x0001 + cpu.x));
        if (cpu.a) {
            wr16(b | (uint16_t)(0x10E8 + cpu.y), cpu.a);
            op_sep(P_M);
            op_lda8(0x0000F3);
            wr8(b | (uint16_t)(0x1134 + cpu.y), (uint8_t)cpu.a);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x58);
            wr8(b | (uint16_t)(0x1135 + cpu.y), 0x58);
            cpu.a = (uint16_t)(cpu.a & 0xFF00);
            cpu.p = nz8(cpu.p, 0);
            wr8(b | (uint16_t)(0x110E + cpu.y), 0x00);
            cpu.y = (uint16_t)(cpu.y + 2);
        }
        op_rep(P_M);
        op_lda16(0x0000F3);
        op_add16(0x0008);
        wr16(0x0000F3, cpu.a);
        cpu.x = (uint16_t)(cpu.x + 2);
        v = (uint16_t)(rd16(0x0000F1) - 1);
        wr16(0x0000F1, v);
        cpu.p = nz16(cpu.p, v);
    } while (v);
    op_rep(P_M);
    time_glyph(b, digit_glyph(0x0000F7, 0), 0x4C, 0);
    time_glyph(b, 0x980A, 0x54, 0);
    op_rep(P_M);
    time_glyph(b, digit_glyph(0x0000F6, 1), 0x5C, 0);
    op_rep(P_M);
    time_glyph(b, digit_glyph(0x0000F6, 0), 0x64, 0);
    time_glyph(b, 0x980C, 0x6C, 0);
    op_rep(P_M);
    time_glyph(b, digit_glyph(0x0000F5, 1), 0x74, 0);
    op_rep(P_M);
    time_glyph(b, digit_glyph(0x0000F5, 0), 0x7C, 1);
}

void abi_GsuBytes200_BFB7B6_D8C4(void);
void abi_GsuBytes200_BFBAB9_D8E1(void);
void abi_GsuBytes200_BFB3BE_D8FE(void);
void abi_GsuBytes200_BFBBBE(void);

/* LDA #glyph, STA.L $70:0216, then Super FX program $07:prog (SEP #$20,
   REP #$10, LDX #prog, LDA #$07, JSL GsuRun, REP #$20, REP #$10). */
static void glyph_prog(uint16_t prog, uint16_t glyph)
{
    op_lda16_imm(glyph);
    wr16(0x700216, glyph);
    op_sep(P_M);
    op_rep(P_X);
    cpu.x = prog;
    cpu.p = nz16(cpu.p, prog);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x07);
    cpu.p = nz8(cpu.p, 0x07);
    abi_GsuRun();
    op_rep(P_M | P_X);
}

/* LDA.L $70:020A or $020C, CLC/SEC, ADC/SBC #d, STA.L: move the draw
   position. */
static void draw_pos_add(uint32_t at, uint16_t d, int sub)
{
    op_lda16(at);
    if (sub)
        op_sub16(d);
    else
        op_add16(d);
    wr16(at, cpu.a);
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
void abi_GlyphWordAt1C28(void)
{
    static const uint16_t g1[5] = {0x840C, 0x8C00, 0x881C, 0x8C18, 0x8C10};
    static const uint16_t g2[4] = {0x8C14, 0x8C0C, 0x8408, 0x9004};
    int i;

    op_rep(P_M | P_X);
    op_lda16_imm(0x001C);
    wr16(0x70020A, 0x001C);
    op_lda16_imm(0x0028);
    wr16(0x70020C, 0x0028);
    op_lda16_imm(0x0020);
    wr16(0x700210, 0x0020);
    op_lda16_imm(0x0010);
    wr16(0x70020E, 0x0010);
    abi_GsuBytes200_BFBCBD();
    for (i = 0; i < 5; i++) {
        op_rep(P_M);
        glyph_prog(0xF15B, g1[i]);
    }
    draw_pos_add(0x70020A, 0x0004, 0);
    for (i = 0; i < 4; i++) {
        op_rep(P_M);
        glyph_prog(0xF15B, g2[i]);
    }
}

/* Three rows of three 8 x 8 glyphs (program $07:F280), each row 8 lower
   and back 24; the glyphs are row byte * $100 + $1A, $1C, $1E. `mid` (when
   not 0) sets the renderer bytes before the first row's third glyph,
   `row2` after the step to the second row. */
static void glyph_grid3(uint16_t first, void (*mid)(void), void (*row2)(void))
{
    int r, c;

    for (r = 0; r < 3; r++) {
        if (r) {
            draw_pos_add(0x70020C, 0x0008, 0);
            draw_pos_add(0x70020A, 0x0018, 1);
            if (r == 1 && row2)
                row2();
        }
        for (c = 0; c < 3; c++) {
            if (r == 0 && c == 2 && mid)
                mid();
            glyph_prog(0xF280, (uint16_t)((first + r) << 8 | (0x1A + 2 * c)));
        }
    }
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
void abi_GlyphGrid1A(void)
{
    op_rep(P_M);
    op_lda16_imm(0x0038);
    wr16(0x70020A, 0x0038);
    op_lda16_imm(0x0030);
    wr16(0x70020C, 0x0030);
    abi_GsuBytes200_BFB7B6_D8C4();
    glyph_grid3(0x1A, abi_GsuBytes200_BFBAB9_D8E1, abi_GsuBytes200_BFB3BE_D8FE);
}

/*
 * @routine 07:A27D GlyphGrid1D
 * WHAT IT DOES: Draws the 3 x 3 glyph picture $1D1A-$1F1E at (56, 72) with
 * program $07:F280, the renderer bytes 0, $BF, $BB, $BE.
 * HOW: As GlyphGrid1A, one setter first.
 * Entry: any widths. Exit: as GlyphGrid1A.
 */
void abi_GlyphGrid1D(void)
{
    op_rep(P_M);
    op_lda16_imm(0x0038);
    wr16(0x70020A, 0x0038);
    op_lda16_imm(0x0048);
    wr16(0x70020C, 0x0048);
    abi_GsuBytes200_BFBBBE();
    glyph_grid3(0x1D, 0, 0);
}

void abi_GlyphListMessageAndTime(void);
void abi_GlyphListDraw(void);
void abi_SoundPort1QueuePush(void);

/* DEC $115A (16-bit); at 0, $115A = reload (unless 0) and the step $10D8
   moves on; 1 when it ran out. */
static int glyph_wait(uint32_t b, uint16_t reload)
{
    uint16_t v = (uint16_t)(rd16(b | 0x115A) - 1);

    wr16(b | 0x115A, v);
    cpu.p = nz16(cpu.p, v);
    if (v)
        return 0;
    if (reload) {
        op_lda16_imm(reload);
        wr16(b | 0x115A, reload);
    }
    v = (uint16_t)(rd16(b | 0x10D8) + 1);
    wr16(b | 0x10D8, v);
    cpu.p = nz16(cpu.p, v);
    return 1;
}

/* INC $10D8 (16-bit). */
static void glyph_next(uint32_t b)
{
    uint16_t v = (uint16_t)(rd16(b | 0x10D8) + 1);

    wr16(b | 0x10D8, v);
    cpu.p = nz16(cpu.p, v);
}

/* A time's three bytes to direct page $F5-$F7, $115C = 0, the next step,
   then message $0D4A * 8 + extra (GlyphListMessageAndTime). */
static void glyph_load_time(uint32_t b, uint16_t from, uint16_t extra)
{
    int i;

    op_sep(P_M);
    for (i = 0; i < 3; i++) {
        op_lda8(b | (uint16_t)(from + i));
        wr8((uint32_t)(0xF5 + i), (uint8_t)cpu.a);
    }
    op_rep(P_M);
    wr16(b | 0x115C, 0x0000);
    glyph_next(b);
    op_lda16(b | 0x0D4A);
    op_and16(0x00FF);
    op_asl16();
    op_asl16();
    op_asl16();
    if (extra)
        op_add16(extra);
    op_tax16();
    abi_GlyphListMessageAndTime();
}

/* One more of the list's entries gets on-flag `flag` ($115C the entry * 2);
   after the last, the next step and $115A = reload. Then the list is
   drawn. */
static void glyph_flag_step(uint32_t b, uint16_t flag, uint16_t reload)
{
    uint16_t v;

    op_lda16(b | 0x115C);
    op_tax16();
    op_lda16_imm(flag);
    wr16(b | (uint16_t)(0x110E + cpu.x), flag);
    v = (uint16_t)(rd16(b | 0x115C) + 1);
    wr16(b | 0x115C, v);
    v = (uint16_t)(rd16(b | 0x115C) + 1);
    wr16(b | 0x115C, v);
    cpu.p = nz16(cpu.p, v);
    op_lda16(b | 0x115C);
    op_lsr16();
    op_cmp16(rd16(b | 0x10E6));
    if (cpu.p & P_C) {
        glyph_next(b);
        op_lda16_imm(reload);
        wr16(b | 0x115A, reload);
    }
    abi_GlyphListDraw();
}

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
void abi_GlyphTimesSequence(void)
{
    static void (*const row[6])(void) = {abi_GlyphRowY60From0, abi_GlyphRowY60From1, abi_GlyphRowY60From2,
                                          abi_GlyphRowY60From3, abi_GlyphRowY60From4, abi_GlyphRowY60From5};
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t step;

    op_rep(P_M | P_X);
    op_lda16(b | 0x10D8);
    op_and16(0x00FF);
    op_asl16();
    op_tax16();
    step = (uint16_t)(cpu.x >> 1);
    rd16(0x040000 | (uint16_t)(0xDB94 + cpu.x));
    switch (step) {
    case 0:
        glyph_wait(b, 0x001E);
        break;
    case 1:
        glyph_wait(b, 0);
        break;
    case 2:
        glyph_load_time(b, 0x0EF9, 0);
        break;
    case 6:
        glyph_load_time(b, 0x0EFC, 2);
        break;
    case 10:
        glyph_load_time(b, 0x0EFF, 4);
        break;
    case 3: case 7: case 11:
        glyph_flag_step(b, 0x00FF, 0x000A);
        break;
    case 4: case 8: case 12: {
        uint16_t v = (uint16_t)(rd16(b | 0x115A) - 1);

        wr16(b | 0x115A, v);
        cpu.p = nz16(cpu.p, v);
        if (!v) {
            wr16(b | 0x115C, 0x0000);
            glyph_next(b);
        }
        abi_GlyphListDraw();
        break;
    }
    case 5: case 9: case 13:
        glyph_flag_step(b, 0x0000, 0x0004);
        break;
    case 14:
        op_lda16_imm(0x0004);
        wr16(b | 0x115A, 0x0004);
        glyph_next(b);
        break;
    case 15: case 16: case 17: case 18: case 19: case 20: {
        static const uint16_t reload[6] = {0x0004, 0x0004, 0x0008, 0x0008, 0x0008, 0x0004};
        glyph_wait(b, reload[step - 15]);
        row[5 - (step - 15)]();
        break;
    }
    case 21:
        if (glyph_wait(b, 0x0014)) {
            op_sep(P_M);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
            cpu.p = nz8(cpu.p, 0x01);
            wr8(b | 0x19D8, 0x01);
            wr8(b | 0x19DF, 0x00);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x04);
            cpu.p = nz8(cpu.p, 0x04);
            abi_SoundPort1QueuePush();
            op_rep(P_M);
        }
        abi_GlyphRowY60From0();
        break;
    default: {
        uint16_t v = (uint16_t)(rd16(b | 0x115A) - 1);

        wr16(b | 0x115A, v);
        cpu.p = nz16(cpu.p, v);
        if (!v) {
            op_sep(P_M);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x07);
            cpu.p = nz8(cpu.p, 0x07);
            cpu.db = 0x07;
            call_routine(0x07EE26, 0);
            return;
        }
        op_rep(P_M | P_X);
        abi_GlyphRowY60From0();
        break;
    }
    }
}

void abi_GsuBytes200_BFB7B6(void);
void abi_GlyphTimesSequence(void);

/* SEP #$20, REP #$10, LDX #prog, LDA #$07, JSL GsuRun, REP #$20, REP #$10. */
static void glyph_prog_run(uint16_t prog)
{
    op_sep(P_M);
    op_rep(P_X);
    cpu.x = prog;
    cpu.p = nz16(cpu.p, prog);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x07);
    cpu.p = nz8(cpu.p, 0x07);
    abi_GsuRun();
    op_rep(P_M | P_X);
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
void abi_GlyphSpritesGrowAndTimes(void)
{
    uint8_t p0 = cpu.p, db0 = cpu.db;
    uint32_t b;
    uint16_t v;

    op_sep(P_M);
    cpu.db = 0x04;
    b = 0x040000;
    op_rep(P_M | P_X);
    op_lda16(b | 0x10D8);
    if (cpu.a) {
        cpu.x = 0x0002;
        do {
            op_lda16(b | (uint16_t)(0x103E + cpu.x));
            op_cmp16(0x0003);
            if (cpu.p & P_C) {
                op_lda16(b | (uint16_t)(0x1040 + cpu.x));
                op_cmp16(0x0007);
                if (!(cpu.p & P_C)) {
                    cpu.a = (uint16_t)(cpu.a + 1);
                    cpu.p = nz16(cpu.p, cpu.a);
                    wr16(b | (uint16_t)(0x1040 + cpu.x), cpu.a);
                }
            }
            cpu.x = (uint16_t)(cpu.x + 2);
            cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(cpu.x - 0x0014)) & ~P_C) | (cpu.x >= 0x0014 ? P_C : 0));
        } while (cpu.x < 0x0014);
        op_lda16(b | 0x1040);
        op_cmp16(0x0007);
        if (!(cpu.p & P_C)) {
            v = (uint16_t)(rd16(b | 0x1040) + 1);
            wr16(b | 0x1040, v);
            cpu.p = nz16(cpu.p, v);
        }
        abi_GsuBytes200_BFB7B6();
        cpu.x = 0x0000;
        do {
            op_lda16(b | (uint16_t)(0x1040 + cpu.x));
            if (cpu.a) {
                uint16_t x = cpu.x;
                uint8_t p;

                op_asl16();
                op_tay16();
                op_lda16(b | (uint16_t)(0xDAA2 + cpu.y));
                wr16(0x70020E, cpu.a);
                wr16(0x700210, cpu.a);
                op_lda16(b | (uint16_t)(0xDAB2 + cpu.y));
                wr16(0x700212, cpu.a);
                wr16(0x700214, cpu.a);
                op_lda16(b | (uint16_t)(0x0FB0 + cpu.x));
                op_add16(rd16(b | (uint16_t)(0xDAC2 + cpu.y)));
                wr16(0x70020A, cpu.a);
                op_lda16_imm(0x0006);
                wr16(0x70020C, 0x0006);
                op_lda16(b | (uint16_t)(0x0FE0 + cpu.x));
                op_add16(rd16(b | (uint16_t)(0xDAD2 + cpu.y)));
                wr16(0x70020C, cpu.a);
                op_lda16(b | (uint16_t)(0x1010 + cpu.x));
                wr16(0x700216, cpu.a);
                p = cpu.p;
                glyph_prog_run(0xFAF2);
                op_plp(p);
                cpu.x = x;
                cpu.p = nz16(cpu.p, x);
            }
            cpu.x = (uint16_t)(cpu.x + 2);
            cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(cpu.x - 0x0020)) & ~P_C) | (cpu.x >= 0x0020 ? P_C : 0));
        } while (cpu.x != 0x0020);
    }
    abi_GlyphTimesSequence();
    op_sep(P_M);
    cpu.db = db0;
    cpu.p = nz8(cpu.p, db0);
    op_plp(p0);
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
void abi_GlyphSlideThree(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p, p;
    uint16_t x;

    op_rep(P_M | P_X);
    cpu.x = 0x0000;
    do {
        op_lda16(b | (uint16_t)(0x0FB0 + cpu.x));
        op_sub16(rd16(b | (uint16_t)(0x1040 + cpu.x)));
        if (cpu.p & P_C) {
            op_cmp16(rd16(b | (uint16_t)(0x1070 + cpu.x)));
            if (!(cpu.p & P_C))
                op_lda16(b | (uint16_t)(0x1070 + cpu.x));
        } else {
            op_lda16(b | (uint16_t)(0x1070 + cpu.x));
        }
        wr16(b | (uint16_t)(0x0FB0 + cpu.x), cpu.a);
        wr16(0x70020A, cpu.a);
        op_lda16_imm(0x0050);
        wr16(0x70020C, 0x0050);
        op_lda16(b | (uint16_t)(0x1010 + cpu.x));
        wr16(0x700216, cpu.a);
        op_cmp16(0x8000);
        if (cpu.p & P_Z)
            abi_GsuBytes200_BFB7B6();
        else if (op_cmp16(0x8006), cpu.p & P_Z)
            abi_GsuBytes200_BFBCBD();
        else if (op_cmp16(0x800C), cpu.p & P_Z)
            abi_GsuBytes200_BFBAB9();
        else
            abi_GsuBytes200_BFB3BE();
        x = cpu.x;
        p = cpu.p;
        glyph_prog_run(0xF3D2);
        op_plp(p);
        cpu.x = (uint16_t)(x + 2);
        cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(cpu.x - 0x0006)) & ~P_C) | (cpu.x >= 0x0006 ? P_C : 0));
    } while (cpu.x < 0x0006);
    op_plp(p0);
}
