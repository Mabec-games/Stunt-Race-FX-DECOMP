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
