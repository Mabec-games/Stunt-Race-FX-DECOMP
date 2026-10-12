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
