/*
 * race_frame.c: a race frame on the S-CPU, and the race's start, which
 * sets up the Super FX, runs the course script and steps two frames.
 */
#include "machine.h"
#include "hw.h"
#include "cpu_ops.h"

void abi_ObjectsRunHandlers(void);
void abi_CameraFollowStep(void);
void abi_ObjectFaceTied(void);
void abi_ViewFromRecord(void);
void abi_FrameRunGsu8FCA(void);
void abi_ViewUpdateAndGsuAEF8(void);
void abi_GsuAB13WithParams(void);
void abi_GsuScreenFlip(void);
void abi_ObjectPoolInit(void);
void abi_ViewDefaults(void);
void abi_GsuRun(void);
void abi_CourseScriptRun(void);

/*
 * @routine 03:B7C5 RaceFrameStep
 * WHAT IT DOES: One race frame: the frames since the last one ($0306) go to
 * $06D4 and $0306 starts again; the near side is set ($1F07 = 0) and
 * $70:246C = $FFFF; the objects run (ObjectsRunHandlers); the camera
 * record at $1A64 moves on (+$37 += $400, CameraFollowStep) and the tied
 * objects face it (ObjectFaceTied); then the view (ViewFromRecord), the
 * Super FX frame with the object work (FrameRunGsu8FCA) and the view
 * update (ViewUpdateAndGsuAEF8). With A = 0 it then waits for the vertical
 * blank to take the frame ($0357), draws the screen (GsuAB13WithParams)
 * and flips it (GsuScreenFlip).
 * HOW: Data bank $03 (the program bank), $7E around the camera calls; A is
 * kept on the stack; P and the data bank saved and restored.
 * Entry: A = 0 to show the frame, else not. Exit: A = the entry A, or as
 * GsuScreenFlip leaves it; X, Y as the last call leaves them; P and the
 * data bank unchanged.
 */
void abi_RaceFrameStep(void)
{
    uint8_t p = cpu.p, db = cpu.db;
    uint16_t arg = cpu.a, w;
    uint8_t v;

    cpu.p |= P_M;
    cpu.p &= (uint8_t)~P_X;
    cpu.db = 0x03;
    v = rd8(0x030306);
    wr8(0x0306D4, v);
    wr8(0x030306, 0x00);
    wr8(0x031F07, 0x00);
    wr16(0x70246C, 0xFFFF);
    abi_ObjectsRunHandlers();
    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    cpu.db = 0x7E;
    cpu.x = 0x1A64;
    cpu.p &= (uint8_t)~P_M;
    w = rd16(0x000037 + 0x1A64);
    wr16(0x000037 + 0x1A64, (uint16_t)(w + 0x0400));
    abi_CameraFollowStep();
    abi_ObjectFaceTied();
    cpu.db = 0x03;
    cpu.p &= (uint8_t)~P_X;
    cpu.x = 0x1A64;
    abi_ViewFromRecord();
    abi_FrameRunGsu8FCA();
    abi_ViewUpdateAndGsuAEF8();
    cpu.a = arg;
    if (!arg) {
        cpu.p |= P_M;
        while (!rd8(0x030357))
            ;
        cpu.p &= (uint8_t)~(P_M | P_X);
        abi_GsuAB13WithParams();
        abi_GsuScreenFlip();
        wr8(0x030357, 0x00);
    }
    cpu.db = db;
    cpu.p = p;
}

/*
 * @routine 03:B5DF RaceStart
 * WHAT IT DOES: Starts a race: the Super FX's screen mode ($0374 = 3),
 * clock (CLSR = 1), config (CFGR = $20) and screen base (SCBR = $0B); not
 * the menu ($0D62 = 0); the object pools (ObjectPoolInit) and view
 * (ViewDefaults); the screen start $2C00 ($70:021A and direct page $48),
 * $70:01C4 = 2, $70:108C = 0; Super FX program $04:8800 (GsuRun); $116A =
 * 0; the course script at $05:8000 (CourseScriptRun); two race frames not
 * shown (RaceFrameStep with A = 1); the camera's target ($1A64 + 2) from
 * $116C; $0357 = 1; and Super FX RAM $70:2C00-$5FFF cleared.
 * HOW: Through the data bank as the caller has it, with long stores to
 * Super FX RAM. P saved and restored.
 * Entry: D = $0000. Exit: A = 0; X = $3400; Y = $1A64; P unchanged.
 */
void abi_RaceStart(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p = cpu.p;
    uint16_t x;

    cpu.p |= P_M;
    wr8(b | 0x0374, 0x03);
    wr8(b | GSU_CLSR, 0x01);
    wr8(b | GSU_CFGR, 0x20);
    wr8(b | GSU_SCBR, 0x0B);
    wr8(b | 0x0D62, 0x00);
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.a = 0xA545;
    wr16(0x70004E, 0xA545);
    abi_ObjectPoolInit();
    abi_ViewDefaults();
    cpu.p &= (uint8_t)~P_M;
    wr16(0x70021A, 0x2C00);
    wr16(0x000048, 0x2C00);
    wr16(0x7001C4, 0x0002);
    wr16(0x70108C, 0x0000);
    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x04);
    cpu.x = 0x8800;
    abi_GsuRun();
    cpu.p &= (uint8_t)~P_M;
    wr16(b | 0x116A, 0x0000);
    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x05);
    cpu.x = 0x0000;
    abi_CourseScriptRun();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    abi_RaceFrameStep();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    abi_RaceFrameStep();
    cpu.p &= (uint8_t)~(P_M | P_X);
    wr16(b | 0x1A66, rd16(0x00116C));
    cpu.p |= P_M;
    wr8(b | 0x0357, 0x01);
    cpu.p &= (uint8_t)~(P_M | P_X);
    for (x = 0; x != 0x3400; x = (uint16_t)(x + 2))
        wr16(0x702C00 + x, 0x0000);
    cpu.a = 0x0000;
    cpu.x = 0x3400;
    cpu.y = 0x1A64;
    cpu.p = p;
}

void abi_SoundPort0QueuePush(void);
void abi_SoundPort0QueuePushRaw(void);

/* The race phases after a finish: sound $37 for the current player, then
   the two players' phases ($0DD6, $0DD7): `p2` for player 2's finish (Y
   not 0), else by bit 0 of $0DED: set, both $14 after $07:E944; clear,
   player 1 `p1` and player 2 $12. Then $0E58 = $30, $0E5B = $0E5F = 0 and
   sound 2 on port 0. */
static void finish_phases(uint8_t p1, uint8_t p2)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t y = cpu.y;
    uint8_t lo, hi;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x37);
    abi_SoundPort0QueuePush();
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)y) & ~P_C) | P_C);     /* CPY #0 */
    if ((uint8_t)y != 0x00) {
        cpu.p |= P_M;
        wr8(b | 0x0DD6, 0x12);
        wr8(b | 0x0DD7, p2);
    } else {
        cpu.p |= P_M;
        hi = rd8(b | 0x0DEF);
        lo = rd8(b | 0x0DED);
        cpu.a = (uint16_t)(hi << 8 | lo);
        cpu.p &= (uint8_t)~P_M;
        if (lo & 0x01) {
            cpu.p |= P_M;
            call_routine(0x07E944, 0);
            cpu.p |= P_M;
            wr8(b | 0x0DD6, 0x14);
            wr8(b | 0x0DD7, 0x14);
        } else {
            cpu.p |= P_M;
            wr8(b | 0x0DD7, 0x12);
            wr8(b | 0x0DD6, p1);
        }
    }
    wr8(b | 0x0E58, 0x30);
    wr8(b | 0x0E5B, 0x00);
    wr8(b | 0x0E5F, 0x00);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x02);
    abi_SoundPort0QueuePushRaw();
}

/*
 * @routine 07:A9D7 RaceFinishPhases0E
 * WHAT IT DOES: Sets the race phases after a finish: sound $37; with Y not
 * 0 (player 2), player 1 $12 and player 2 $0E; with Y = 0, both $14 (after
 * $07:E944) when bit 0 of $0DED is set, else player 1 $0E and player 2
 * $12. Then $0E58 = $30, $0E5B = $0E5F = 0 and sound 2.
 * HOW: Shares its tail with RaceFinishPhases10 ($07:AA4E). The $0DED case
 * is not reached in the traces so far.
 * Entry: M 8-bit, X 8-bit; Y = player. Exit: A low byte = 2; M 8-bit; N, Z
 * from the data bank (the sound queue's last pull).
 */
void abi_RaceFinishPhases0E(void)
{
    finish_phases(0x0E, 0x0E);
}

/*
 * @routine 07:AA0B RaceFinishPhases10
 * WHAT IT DOES: As RaceFinishPhases0E, with $10 in place of $0E: player 2
 * $10 when Y is not 0, player 1 $10 otherwise.
 * HOW: As RaceFinishPhases0E. The Y-not-0 and $0DED cases are not reached
 * in the traces so far.
 * Entry: as RaceFinishPhases0E. Exit: as RaceFinishPhases0E.
 */
void abi_RaceFinishPhases10(void)
{
    finish_phases(0x10, 0x10);
}

/* RaceVarsReset's stores in order: kind 0 through the data bank, 1 long,
   2 direct page; width in bytes; value. */
static const struct { uint8_t kind; uint32_t addr; uint8_t width; uint16_t v; } race_reset[] = {
    {0, 0x0D60, 2, 0x0000},
    {0, 0x0D29, 2, 0x0166},
    {0, 0x0D7F, 2, 0x0000},
    {1, 0x70021A, 2, 0x2C00},
    {2, 0x0048, 2, 0x2C00},
    {0, 0x0D81, 2, 0x0000},
    {0, 0x0E62, 2, 0x0000},
    {0, 0x0E64, 2, 0x0000},
    {0, 0x0E66, 2, 0x0000},
    {0, 0x10A2, 2, 0x0000},
    {0, 0x10A4, 2, 0x009A},
    {0, 0x10A6, 2, 0x006A},
    {0, 0x10A8, 2, 0x0000},
    {0, 0x0DA4, 2, 0x0000},
    {0, 0x0DA6, 2, 0x0000},
    {0, 0x0D9E, 2, 0x0000},
    {0, 0x0DA0, 2, 0x0000},
    {0, 0x117E, 2, 0x0000},
    {0, 0x1180, 2, 0x0000},
    {0, 0x0D85, 2, 0x37D7},
    {0, 0x0D85, 2, 0x37D7},
    {1, 0x7EFD5B, 2, 0x0000},
    {1, 0x7EFD5D, 2, 0x0000},
    {1, 0x7EFD5F, 2, 0x0000},
    {1, 0x7EFD61, 2, 0x0000},
    {1, 0x7EFD63, 2, 0x0000},
    {1, 0x7F3460, 2, 0x0000},
    {1, 0x7F3462, 2, 0x0000},
    {1, 0x7F346C, 2, 0x0000},
    {1, 0x7F346E, 2, 0x0000},
    {1, 0x7F3464, 2, 0x0000},
    {1, 0x7F3466, 2, 0x0000},
    {1, 0x7F3474, 2, 0xFFFF},
    {1, 0x7F3476, 2, 0xFFFF},
    {1, 0x7F3478, 2, 0xFFFF},
    {1, 0x7F345C, 2, 0x7FFF},
    {1, 0x7F345E, 2, 0x7FFF},
    {1, 0x7F3468, 2, 0x001F},
    {1, 0x7F346A, 2, 0x001F},
    {0, 0x0DE4, 2, 0x0000},
    {0, 0x0DE6, 2, 0x0000},
    {0, 0x0DDC, 2, 0x0000},
    {0, 0x0DDE, 2, 0x0000},
    {0, 0x0DE0, 2, 0x0000},
    {0, 0x0DE2, 2, 0x0000},
    {0, 0x0E5D, 2, 0x0000},
    {0, 0x0D65, 2, 0x0000},
    {0, 0x10BA, 1, 0x0000},
    {0, 0x0D54, 1, 0x0000},
    {0, 0x0E47, 1, 0x0000},
    {0, 0x0E49, 1, 0x0000},
    {0, 0x0D5C, 1, 0x0000},
    {0, 0x0D5D, 1, 0x0000},
    {0, 0x0EDB, 1, 0x0000},
    {0, 0x0EDC, 1, 0x0000},
    {0, 0x0EDD, 1, 0x0000},
    {0, 0x0E1A, 1, 0x0000},
    {0, 0x0DD6, 1, 0x0000},
    {0, 0x0DD7, 1, 0x0000},
    {0, 0x0DD8, 1, 0x0000},
    {0, 0x0DD9, 1, 0x0000},
    {0, 0x0DDA, 1, 0x0000},
    {0, 0x0DDB, 1, 0x0000},
    {0, 0x0DE8, 1, 0x0000},
    {0, 0x0DE9, 1, 0x0000},
    {0, 0x1A2D, 1, 0x0000},
    {0, 0x0DEA, 1, 0x0000},
    {0, 0x0DEB, 1, 0x0000},
    {0, 0x0DF6, 1, 0x0000},
    {0, 0x0DF7, 1, 0x0000},
    {0, 0x0DF9, 1, 0x0000},
    {0, 0x0DFA, 1, 0x0000},
    {0, 0x0DEC, 1, 0x0000},
    {0, 0x0DED, 1, 0x0000},
    {0, 0x0DEE, 1, 0x0000},
    {0, 0x0DEF, 1, 0x0000},
    {0, 0x0DF4, 1, 0x0001},
    {0, 0x0DF5, 1, 0x0001},
    {0, 0x0E07, 1, 0x0000},
    {0, 0x0E08, 1, 0x0000},
    {2, 0x0000, 1, 0x0000},
    {0, 0x0D22, 1, 0x0000},
    {0, 0x0D23, 1, 0x0000},
    {0, 0x0D2F, 1, 0x0000},
    {0, 0x0D2D, 1, 0x0000},
    {0, 0x0D2E, 1, 0x0000},
    {0, 0x0D3D, 1, 0x0000},
    {0, 0x0D55, 1, 0x0000},
    {0, 0x0E57, 1, 0x0000},
    {0, 0x0E5C, 1, 0x0000},
    {0, 0x0E59, 1, 0x0000},
    {0, 0x0E3A, 1, 0x0000},
    {0, 0x0E54, 1, 0x0000},
    {0, 0x0E3F, 1, 0x0000},
    {0, 0x0E40, 1, 0x0000},
    {0, 0x0E41, 1, 0x0000},
    {0, 0x0E42, 1, 0x0000},
    {0, 0x1A5D, 1, 0x0000},
    {0, 0x0E4B, 1, 0x0000},
    {0, 0x0E4F, 1, 0x0000},
    {0, 0x0E50, 1, 0x0000},
    {0, 0x0E51, 1, 0x0000},
    {0, 0x0E52, 1, 0x0000},
    {0, 0x0E53, 1, 0x0000},
    {0, 0x0E90, 1, 0x0000},
    {0, 0x0E01, 1, 0x0000},
    {0, 0x0E02, 1, 0x0000},
    {0, 0x0E5B, 1, 0x0000},
    {0, 0x0D5B, 1, 0x0000},
    {0, 0x0D5A, 1, 0x0000},
    {0, 0x0E5F, 1, 0x0017},
    {0, 0x4209, 1, 0x0017},
    {0, 0x420A, 1, 0x0000},
    {0, 0x4207, 1, 0x0040},
    {0, 0x4208, 1, 0x0000},
    {0, 0x0D53, 1, 0x00FF},
    {0, 0x0D45, 1, 0x001C},
    {0, 0x4200, 1, 0x0031},
    {0, 0x210F, 1, 0x0000},
    {0, 0x210F, 1, 0x0000},
    {0, 0x2110, 1, 0x0080},
    {0, 0x2110, 1, 0x0000},
};

/*
 * @routine 03:E838 RaceVarsReset
 * WHAT IT DOES: Resets the race's work variables before a race: $0DA2 =
 * -$10 on course $0A, else 0; $0D29 = $166; the Super FX screen start
 * $2C00 ($70:021A and direct page $48); the HUD, time, lap and gauge
 * values cleared; $10A4 = $9A, $10A6 = $6A; $0D85 = $37D7; the fade
 * values at $7F:345C-$7F:3478 ($7FFF, 0, $1F, $FFFF); $0DF4/$0DF5 = 1,
 * $0E5F = $17, $0D53 = $FF, $0D45 = $1C; then the V/H IRQ timer
 * ($17, $40), NMITIMEN = $31 and BG3 scroll 0 / 0.
 * HOW: 16-bit stores, then 8-bit from $10BA on, in the original's order;
 * $0D85 is stored twice. The course $0A value is not reached in the
 * traces so far.
 * Entry: any widths; DB a bank where work RAM and the PPU registers are
 * mapped, D = $0000. Exit: A = $0080; M 8-bit; N set, Z clear; C from
 * comparing the course with $0A.
 */
void abi_RaceVarsReset(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    unsigned i;

    cpu.p &= (uint8_t)~P_M;
    cpu.a = (uint16_t)(rd16(b | 0x0E3D) & 0x00FF);
    op_cmp16(0x000A);
    cpu.a = cpu.a == 0x000A ? 0xFFF0 : 0x0000;
    wr16(b | 0x0DA2, cpu.a);
    for (i = 0; i < sizeof race_reset / sizeof race_reset[0]; i++) {
        uint32_t a = race_reset[i].kind == 0 ? (b | race_reset[i].addr)
                   : race_reset[i].kind == 1 ? race_reset[i].addr
                   : (uint16_t)(cpu.d + race_reset[i].addr);

        if (race_reset[i].width == 2)
            wr16(a, race_reset[i].v);
        else
            wr8(a, (uint8_t)race_reset[i].v);
    }
    cpu.a = 0x0080;
    cpu.p = (uint8_t)(nz8(cpu.p, 0x80) | P_M);
}

void abi_HandlerSoundIndex(void);
void abi_ApuUpload(void);
void abi_RecordsUnpackCourseRunning(void);

/* The pick's sound: $0D8C = the handler at $18DB, $0D8B its sound index
   (HandlerSoundIndex), leaving M and X 8-bit. */
static void pick_sound(uint32_t b)
{
    cpu.p &= (uint8_t)~P_M;
    cpu.a = rd16(0x0018DB);
    wr16(b | 0x0D8C, cpu.a);
    abi_HandlerSoundIndex();
    op_sep(P_M | P_X);
    wr8(b | 0x0D8B, (uint8_t)cpu.a);
}

/* LDA #$4000, DEC A until 0: A = 0, Z set. */
static void delay_4000(void)
{
    cpu.a = 0x0000;
    cpu.p = nz16(cpu.p, 0);
}

/*
 * @routine 03:D8B3 RaceModeSetup
 * WHAT IT DOES: Sets the race up for mode $0D2B. Mode 3 with bits 0-2 of
 * $70:FC0B clear only marks $10CD ($FF), takes course $1B and the pick's
 * sound ($18DB into $0D8C, HandlerSoundIndex into $0D8B). Otherwise:
 * $0D4A = 0 (or, in mode 3, 1 unless bit 0 of $70:FC0B is set), $10CD and
 * $0E37 cleared, the pick's sound, the race flags at $0D4C, $0D4E, $10CD,
 * $0D48, $0E3E, $0D63, $0D44, $0EED-$0EEF, $0D24, $0D25 cleared, the twelve
 * bytes at $0F05 set to 4 in mode 0 else 0, $0E6C/$0E6E cleared; then mode
 * 0 unpacks the course's records as running totals
 * (RecordsUnpackCourseRunning) with $0DF8 = 2, others $0DF8 = $FF, mode 2
 * first sending 5 on APU port 1 and uploading sound block $1A:F091
 * (ApuUpload) between two delay loops.
 * HOW: 8-bit with word stores where the original is 16-bit; the $0F05
 * bytes are stored from the last down. Mode 3 with any of bits 0-2 set is
 * not reached in the traces so far.
 * Entry: any widths; DB a bank where work RAM is mapped, D = $0000. Exit:
 * P restored; X and Y high bytes cleared; A as the last step left it.
 */
void abi_RaceModeSetup(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p, mode, v;
    static const uint16_t clear[] = {0x0D4C, 0x0D4E, 0x10CD, 0x0D48, 0x0E3E, 0x0D63, 0x0D4C,
                                     0x0D44, 0x0EED, 0x0EEE, 0x0EEF, 0x0D24, 0x0D25};
    unsigned i;

    cpu.p |= P_M;
    mode = rd8(b | 0x0D2B);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | mode);
    if (mode == 0x03) {
        v = (uint8_t)(rd8(0x70FC0B) & 0x07);
        if (!v) {
            wr8(b | 0x10CD, 0xFF);
            wr8(b | 0x0E3D, 0x1B);
            wr8(b | 0x0E3E, 0x00);
            pick_sound(b);
            op_plp(p0);
            return;
        }
        v = rd8(0x70FC0B);
        wr8(b | 0x0D4A, (v & 0x01) ? 0x00 : 0x01);
    } else
        wr8(b | 0x0D4A, 0x00);
    wr8(b | 0x10CD, 0x00);
    cpu.p &= (uint8_t)~P_M;
    wr16(b | 0x0E37, 0x0000);
    pick_sound(b);
    for (i = 0; i < sizeof clear / sizeof clear[0]; i++)
        wr8(b | clear[i], 0x00);
    v = rd8(b | 0x0D2B) ? 0x00 : 0x04;
    for (i = 0x0C; i-- > 0; )
        wr8(b | (uint16_t)(0x0F05 + i), v);
    cpu.x = 0xFF;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p &= (uint8_t)~P_M;
    wr16(b | 0x0E6C, 0x0000);
    wr16(b | 0x0E6E, 0x0000);
    cpu.p |= P_M;
    mode = rd8(b | 0x0D2B);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | mode);
    if (mode == 0x00) {
        abi_RecordsUnpackCourseRunning();
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x02);
        wr8(b | 0x0DF8, 0x02);
        op_plp(p0);
        return;
    }
    if (mode == 0x02) {
        uint8_t p1 = cpu.p;

        cpu.p &= (uint8_t)~(P_M | P_X);
        wr16(0x002141, 0x0005);
        delay_4000();
        cpu.a = 0x001A;
        cpu.y = 0xF091;
        abi_ApuUpload();
        cpu.p &= (uint8_t)~P_M;
        delay_4000();
        op_plp(p1);
    }
    cpu.p |= P_M;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xFF);
    wr8(b | 0x0DF8, 0xFF);
    op_plp(p0);
}

/* The view's direct page parameters $2A-$35 and $CE-$D1, in the order the
   original stores them (8-bit). */
static void view_params(const uint8_t *v)
{
    static const uint8_t at[16] = {0x2A, 0x2B, 0x2E, 0x2F, 0x2C, 0x2D, 0x30, 0x31,
                                   0xCE, 0xCF, 0xD0, 0xD1, 0x34, 0x35, 0x32, 0x33};
    int i;

    for (i = 0; i < 16; i++)
        wr8((uint16_t)(cpu.d + at[i]), v[i]);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v[15]);
    cpu.p = nz8(cpu.p, v[15]);
}

/*
 * @routine 7E:F6E9 ViewParamsByPlayers
 * WHAT IT DOES: Sets the view's direct page parameters for one player or,
 * with $0D62 set, for the two-player split: $2A-$2F, $30/$31, $CE-$D1,
 * $34/$35 and $32/$33 ($CF, $80, $68, $40, $4000, $4EC4 for one; $8F,
 * $50, $48, $28, $6666, $71C7 for two).
 * HOW: Byte stores to direct page.
 * Entry: any widths, D = $0000. Exit: M 8-bit; A low byte = the last byte
 * ($4E or $71), N, Z from it.
 */
void abi_ViewParamsByPlayers(void)
{
    static const uint8_t one[16] = {0x00, 0x00, 0xCF, 0x00, 0x00, 0x00, 0x80, 0x00,
                                    0x68, 0x00, 0x40, 0x00, 0x00, 0x40, 0xC4, 0x4E};
    static const uint8_t two[16] = {0x00, 0x00, 0x8F, 0x00, 0x00, 0x00, 0x50, 0x00,
                                    0x48, 0x00, 0x28, 0x00, 0x66, 0x66, 0xC7, 0x71};
    uint8_t v;

    cpu.p |= P_M;
    v = rd8((uint32_t)cpu.db << 16 | 0x0D62);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    view_params(v ? two : one);
}

void abi_ObjectLinkSpeedSet(void);
void abi_ObjectLinkBobStart(void);

/*
 * @routine 7E:F6F2 ViewParamsSpeedLong
 * WHAT IT DOES: A long way into ObjectLinkEventTrigger2's tail, which
 * nothing reaches: stores A at $1A21.
 * HOW: JSR ObjectLinkSpeedSet, RTL.
 * Entry: any widths; data bank $7E. Exit: M and X 8-bit (X and Y high
 * bytes 0); A unchanged.
 */
void abi_ViewParamsSpeedLong(void)
{
    abi_ObjectLinkSpeedSet();
}

/*
 * @routine 7E:F737 ViewParamsBobLong
 * WHAT IT DOES: A long way into ObjectLinkEventTrigger2's tail, which
 * nothing reaches: starts bob 1 with A as its kind ($1A1D = A, $1A1F =
 * $1A23 = 0, $1A21 = $FE).
 * HOW: JSR ObjectLinkBobStart, RTL.
 * Entry: M 8-bit (the code is 8-bit); data bank $7E. Exit: M and X 8-bit
 * (X and Y high bytes 0); A low byte = $FE; N set, Z clear.
 */
void abi_ViewParamsBobLong(void)
{
    abi_ObjectLinkBobStart();
}

/*
 * @routine 02:E33D LoopLeaveWhenEnded
 * WHAT IT DOES: Ends the frame loop at $02:E289 once its fade-out has run
 * its course ($0E3A set and $0D61 negative): interrupts and the vertical
 * blank interrupt off, and a return from the loop itself with its saved
 * data bank and P. Otherwise a plain return.
 * HOW: PLA drops this routine's return address; then STZ $4200, SEI, PLB,
 * PLP and RTL take the loop's frame (P and the data bank pushed after its
 * JSL).
 * Entry: D = $0000. Exit: M 8-bit; on the way out as the loop's caller
 * left P and the data bank, A = this routine's return address.
 */
void abi_LoopLeaveWhenEnded(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t lo;

    op_sep(P_M);
    op_lda8(b | 0x0E3A);
    if (!(uint8_t)cpu.a)
        return;
    op_lda8(b | 0x0D61);
    if (!(cpu.a & 0x80))
        return;
    op_rep(P_M);
    cpu.a = rd16((uint16_t)(cpu.s + 1));
    cpu.p = nz16(cpu.p, cpu.a);
    cpu.s = (uint16_t)(cpu.s + 2);
    op_sep(P_M | P_X);
    wr8(NMITIMEN, 0x00);
    cpu.p |= P_I;
    cpu.db = rd8((uint16_t)(cpu.s + 1));
    cpu.s = (uint16_t)(cpu.s + 1);
    cpu.p = nz8(cpu.p, cpu.db);
    op_plp(rd8((uint16_t)(cpu.s + 1)));
    cpu.s = (uint16_t)(cpu.s + 1);
    lo = (uint16_t)(rd16((uint16_t)(cpu.s + 1)) + 1);
    cpu.pc = (uint32_t)rd8((uint16_t)(cpu.s + 3)) << 16 | lo;
    cpu.s = (uint16_t)(cpu.s + 3);
    cpu.unwound = 1;
}

void abi_VideoRegsReset(void);
void abi_SaveUnlocksCheck(void);
void abi_SoundIndicesFromHandlers(void);
void abi_SoundDataLoad(void);
void abi_SoundReset(void);
void abi_VramNoiseTiles(void);
void abi_CgramDmaAll(void);
void abi_OamBufferClear(void);
void abi_ReturnOnly(void);
void abi_RaceVarsReset(void);
void abi_GsuCF42ToVram(void);
void abi_RaceScreenSetup(void);

/* PHP, REP #$30, 5 on APU port 1 (a word store), a delay, then the load
   with A and Y, PLP. */
static void race_load_sound(void (*load)(void), uint16_t a, uint16_t y, int delay_after)
{
    uint8_t p = cpu.p;

    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.a = 0x0005;
    wr16(0x002141, 0x0005);
    delay_4000();
    cpu.a = a;
    cpu.y = y;
    cpu.p = nz16(cpu.p, y);
    load();
    if (delay_after) {
        cpu.p &= (uint8_t)~P_M;
        delay_4000();
    }
    op_plp(p);
}

/* REP #$30, LDY, LDX, JSL GsuCF42ToVram. */
static void race_load_gsu(uint16_t y, uint16_t x)
{
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.y = y;
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
    abi_GsuCF42ToVram();
}

/* PHB, PHP, REP #$30, MVN $7F,$70 of $1000 bytes from $70:3000 to $7F:dst,
   PLP, PLB. */
static void race_load_copy(uint16_t dst)
{
    uint8_t p = cpu.p;
    unsigned i;

    cpu.p &= (uint8_t)~(P_M | P_X);
    for (i = 0; i < 0x1000; i++)
        wr8(0x7F0000 | (uint16_t)(dst + i), rd8(0x703000 + i));
    cpu.a = 0xFFFF;
    cpu.x = 0x4000;
    cpu.y = (uint16_t)(dst + 0x1000);
    op_plp(p);
}

/* The views for the mode: the GSU block (Y $F868, X x) and the two copies
   of $70:3000 into $7F:93D7 and $7F:BBD7. */
static void race_load_views(uint16_t x)
{
    race_load_gsu(0xF868, x);
    race_load_copy(0x93D7);
    race_load_copy(0xBBD7);
}

/* REP #$20, then $0E35, $0E37 (= scroll), $0E31 and $0E33 (word stores). */
static void race_load_scroll(uint32_t b, uint16_t scroll)
{
    cpu.p &= (uint8_t)~P_M;
    wr16(b | 0x0E35, 0x0000);
    if (scroll) {
        cpu.a = scroll;
        cpu.p = nz16(cpu.p, scroll);
    }
    wr16(b | 0x0E37, scroll);
    wr16(b | 0x0E31, 0x0000);
    wr16(b | 0x0E33, 0x0000);
}

/* SEP #$20, LDA $0D63: Z when it is clear. */
static uint8_t race_load_0d63(uint32_t b)
{
    op_sep(P_M);
    op_lda8(b | 0x0D63);
    return (uint8_t)cpu.a;
}

/*
 * @routine 03:F0E6 RaceLoad
 * WHAT IT DOES: Loads a race: video registers reset (twice), the unlocks
 * checked, the sounds chosen (SoundIndicesFromHandlers) and loaded
 * (SoundDataLoad $E049/$1E, ApuUpload $1B:B8A0 and $1B:D5FC, each after 5
 * on APU port 1 and a delay), SoundReset, 1 on APU port 0, the Super FX
 * screen mode shadow $0374 = 3, the noise tiles (VramNoiseTiles), the
 * palette from $1F:8200 into $7E:FD65 and to CGRAM, the OAM buffer
 * cleared, the race variables reset and two Super FX blocks put in VRAM
 * (GsuCF42ToVram, Y/X $F837/$F7E6 and $F822/$F801). Then by HUD mode $0D2B
 * through the table at $03:F1B2: modes 0 and 3 take their views (Y $F868
 * with X $F7F2 or $F7FB; $70:3000 copied twice into $7F:93D7 and
 * $7F:BBD7), then block $F83E/$F7E9 and the scroll $0E37 = $50 ($0 with
 * $0D63 set); modes 1 and 2 their views ($F7F8, $F7F5) and blocks
 * $F83E/$F7EC or $F83E/$F7EF with the scroll 0, mode 2 and mode 1 without
 * $0D63 also clearing $0D48 (mode 1 then with scroll $50); mode 4 (the
 * fifth entry) none of these. $0E35, $0E31 and $0E33 are cleared. Last,
 * RaceScreenSetup.
 * HOW: P and the data bank ($03) pushed; delay loops of $4000; MVN for the
 * copies. Modes 3 and 4, and mode 1 with $0D63 set, are not reached in the
 * traces so far. Modes above 4 would take code bytes as addresses and are
 * not modelled.
 * Entry: any widths. Exit: P and the data bank restored; A, X, Y as
 * RaceScreenSetup leaves them.
 */
void abi_RaceLoad(void)
{
    const uint32_t b = 0x030000;
    uint8_t p0 = cpu.p, db0 = cpu.db, v;
    uint16_t w;

    op_sep(P_M | P_X);
    cpu.db = 0x03;
    cpu.p = nz8(cpu.p, 0x03);
    abi_VideoRegsReset();
    abi_SaveUnlocksCheck();
    abi_SoundIndicesFromHandlers();
    race_load_sound(abi_SoundDataLoad, 0xE049, 0x001E, 0);
    race_load_sound(abi_ApuUpload, 0x001B, 0xB8A0, 1);
    race_load_sound(abi_ApuUpload, 0x001B, 0xD5FC, 1);
    abi_SoundReset();
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    wr8(b | 0x2140, 0x01);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x03);
    cpu.p = nz8(cpu.p, 0x03);
    wr8(b | 0x0374, 0x03);
    abi_VramNoiseTiles();
    cpu.p &= (uint8_t)~(P_M | P_X);
    for (w = 0x01FE;; w = (uint16_t)(w - 2)) {
        cpu.a = rd16(0x1F8200 + w);
        wr16(0x7EFD65 + w, cpu.a);
        if (!w)
            break;
    }
    cpu.x = 0xFFFE;
    cpu.p = nz16(cpu.p, 0xFFFE);
    abi_CgramDmaAll();
    abi_OamBufferClear();
    abi_ReturnOnly();
    abi_VideoRegsReset();
    abi_RaceVarsReset();
    race_load_gsu(0xF837, 0xF7E6);
    race_load_gsu(0xF822, 0xF801);
    op_sep(P_M | P_X);
    v = rd8(b | 0x0D2B);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >> 7));
    v = (uint8_t)(v << 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = v;
    cpu.p = nz8(cpu.p, v);
    w = rd16(b | (uint16_t)(0xF1B2 + v));
    switch (w) {
    case 0xF1EE:
    case 0xF1BC:
        race_load_views(w == 0xF1EE ? 0xF7F2 : 0xF7FB);
        race_load_gsu(0xF83E, 0xF7E9);
        race_load_scroll(b, race_load_0d63(b) ? 0x0000 : 0x0050);
        break;
    case 0xF256:
        race_load_views(0xF7F5);
        race_load_gsu(0xF83E, 0xF7EF);
        race_load_scroll(b, 0x0000);
        op_sep(P_M);
        wr8(b | 0x0D48, 0x00);
        break;
    case 0xF2A7:
        race_load_views(0xF7F8);
        race_load_gsu(0xF83E, 0xF7EC);
        race_load_scroll(b, 0x0000);
        if (race_load_0d63(b))
            race_load_scroll(b, 0x0000);
        else {
            race_load_scroll(b, 0x0050);
            op_sep(P_M);
            wr8(b | 0x0D48, 0x00);
        }
        break;
    }
    abi_RaceScreenSetup();
    op_sep(P_M);
    cpu.db = db0;
    cpu.p = nz8(cpu.p, db0);
    op_plp(p0);
}

void abi_PlayerControlsFrame(void);
void abi_PaletteFadeStep(void);
void abi_WordCopyStep(void);
void abi_BackdropAnglesFromView(void);
void abi_FrameRunGsuAB13B(void);
void abi_DebugShowErrors(void);
void abi_SpeedGaugeUpdate(void);
void abi_NearCarSoundUpdate(void);
void abi_GsuScreenToggle(void);

/* CMP # (8-bit): N, Z and C. */
static void frame_cmp8(uint8_t m)
{
    uint8_t a = (uint8_t)cpu.a;

    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? P_C : 0));
}

/* LDA # (8-bit): the low byte of A. */
static void frame_lda8_imm(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* The frame's start: data bank $02 (PHK, PLB), the frames counted since
   the last ($0306) taken into $06D4, and $00 = 2. */
static void frame_start(void)
{
    uint32_t b = 0x020000;

    op_sep(P_M);
    op_rep(P_X);
    cpu.db = 0x02;
    cpu.p = nz8(cpu.p, 0x02);
    op_lda8(b | 0x0306);
    wr8(b | 0x06D4, (uint8_t)cpu.a);
    wr8(b | 0x0306, 0x00);
    frame_lda8_imm(0x02);
    wr8(cpu.d, 0x02);
}

/* ViewFromRecord on the object two links (+$4F) on from the car at
   `slot`. */
static void frame_view_from(uint16_t slot)
{
    op_rep(P_M | P_X);
    cpu.x = rd16((uint32_t)cpu.db << 16 | slot);
    cpu.p = nz16(cpu.p, cpu.x);
    op_lda16(0x7E204Fu + cpu.x);
    op_tax16();
    op_lda16(0x7E204Fu + cpu.x);
    op_tax16();
    abi_ViewFromRecord();
}

/* Side 0: $1F07 = 0, $70:246C-D = $FF, programs $8FCA and $01:BBC7. */
static void frame_side0(void)
{
    op_sep(P_M);
    wr8((uint32_t)cpu.db << 16 | 0x1F07, 0x00);
    frame_lda8_imm(0xFF);
    wr8(0x70246C, 0xFF);
    wr8(0x70246D, 0xFF);
    abi_FrameRunGsu8FCA();
    op_sep(P_M);
    op_rep(P_X);
    frame_lda8_imm(0x01);
    cpu.x = 0xBBC7;
    cpu.p = nz16(cpu.p, cpu.x);
    abi_GsuRun();
}

/* The frame's end: the time counts, $0306 to $70:108E, then the data
   bank and P put back. */
static void frame_time(uint8_t db, uint8_t p0)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;

    op_lda8(b | 0x0306);
    wr8(b | 0x1F3E, (uint8_t)cpu.a);
    op_lda8(b | 0x06D5);
    cpu.p &= (uint8_t)~P_C;
    v = op_adc8((uint8_t)cpu.a, rd8(b | 0x06D4), &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    wr8(b | 0x06D5, v);
    v = (uint8_t)(rd8(b | 0x06D6) + 1);
    wr8(b | 0x06D6, v);
    cpu.p = nz8(cpu.p, v);
    op_lda8(b | 0x06D5);
    frame_cmp8(0x3C);
    if (cpu.p & P_C) {
        op_lda8(b | 0x06D6);
        wr8(b | 0x06D7, (uint8_t)cpu.a);
        wr8(b | 0x06D6, 0x00);
        op_lda8(b | 0x06D5);
        cpu.p |= P_C;
        v = op_sbc8((uint8_t)cpu.a, 0x3C, &cpu.p);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        wr8(b | 0x06D5, v);
    }
    op_sep(P_M);
    op_lda8(b | 0x0306);
    wr8(0x70108E, (uint8_t)cpu.a);
    frame_lda8_imm(0x00);
    wr8(0x70108F, 0x00);
    cpu.db = db;
    cpu.p = nz8(cpu.p, db);
    op_plp(p0);
}

/*
 * @routine 02:D7CD RaceFrameRun
 * WHAT IT DOES: One frame of a race: takes the frames counted since the last
 * ($0306, into $06D4, cleared), points the view from player 1's car
 * (ViewFromRecord on the object two links on from $0346), reads the
 * players' controls (PlayerControlsFrame), draws side 0 ($1F07) with Super FX
 * programs $8FCA (FrameRunGsu8FCA, $70:246C-D = $FF first) and $01:BBC7
 * (GsuRun), runs the objects, the view and program $AEF8, the palette fade,
 * the word copy and the backdrop angles; then side 1 with the HUD
 * (FrameRunGsuAB13B, $1AE8 cleared first), DebugShowErrors, the speed gauge
 * (A = $70:2476, X = $2143) and the near-car sound; keeps $1AE8 at $1AEA;
 * runs program $01:D307 when $70:2732 is set, else 07:DAA7; switches the
 * screen buffers (GsuScreenToggle), clears $0357 and keeps time: $1F3E =
 * $0306, $06D5 adds $06D4 and $06D6 counts frames, each $3C of $06D5 moving
 * $06D6 to $06D7; $0306 also goes to $70:108E, with 0 at $70:108F.
 * HOW: Data bank $02, its own (the low addresses reach work RAM); $00 = 2;
 * 8-bit A for the counts; P and the data bank put back at the end.
 * Entry: any widths; D = $0000. Exit: P and the data bank unchanged; A low
 * byte 0; X, Y as the last call left them.
 */
void abi_RaceFrameRun(void)
{
    uint8_t p0 = cpu.p, db = cpu.db;
    uint32_t b;

    frame_start();
    frame_view_from(0x0346);
    abi_PlayerControlsFrame();
    frame_side0();
    op_sep(P_M);
    op_rep(P_X);
    abi_ObjectsRunHandlers();
    abi_ViewUpdateAndGsuAEF8();
    abi_PaletteFadeStep();
    abi_WordCopyStep();
    abi_BackdropAnglesFromView();
    op_sep(P_M);
    b = (uint32_t)cpu.db << 16;
    frame_lda8_imm(0x01);
    wr8(b | 0x1F07, 0x01);
    op_rep(P_M);
    wr16(b | 0x1AE8, 0x0000);
    op_sep(P_M);
    abi_FrameRunGsuAB13B();
    op_sep(P_M);
    abi_DebugShowErrors();
    op_rep(P_M | P_X);
    cpu.x = 0x2143;
    cpu.p = nz16(cpu.p, cpu.x);
    op_lda16(0x702476);
    abi_SpeedGaugeUpdate();
    abi_NearCarSoundUpdate();
    op_rep(P_M);
    b = (uint32_t)cpu.db << 16;
    op_lda16(b | 0x1AE8);
    wr16(b | 0x1AEA, cpu.a);
    op_lda16(0x702732);
    if (cpu.a) {
        op_sep(P_M);
        frame_lda8_imm(0x01);
        cpu.x = 0xD307;
        cpu.p = nz16(cpu.p, cpu.x);
        abi_GsuRun();
        op_rep(P_M | P_X);
    } else {
        op_sep(P_M);
        call_routine(0x07DAA7, 1);
    }
    abi_GsuScreenToggle();
    op_sep(P_M);
    b = (uint32_t)cpu.db << 16;
    wr8(b | 0x0357, 0x00);
    frame_time(db, p0);
}

void abi_GsuScreenFlipMenu(void);
void abi_RaceSoundsPlayer1(void);
void abi_RaceSoundsPlayer2(void);
void abi_FrameRunGsuAB13A(void);
void abi_SpeedGauge2Update(void);

/*
 * @routine 0B:FB26 RaceFrameRunTwoPlayers
 * WHAT IT DOES: One frame of a two-player race: takes the frames counted
 * since the last ($0306, into $06D4, cleared), runs the objects, then for
 * player 1 points the view from the car (ViewFromRecord on the object two
 * links on from $0346), reads both players' controls (PlayerControlsFrame)
 * and draws side 0 ($1F07) with Super FX programs $8FCA (FrameRunGsu8FCA,
 * $70:246C-D = $FF first) and $01:BBC7, the view and program $AEF8, the
 * palette fade, the word copy and the backdrop angles; for player 2 the
 * view from the car at $034A, side 1 with the HUD (FrameRunGsuAB13B, $1AE8
 * cleared first) and the speed gauge (A = $70:2476, X = $2143), 07:DA02,
 * a screen switch (GsuScreenFlipMenu) and $0357 cleared; then the view and
 * program $AEF8 again, the backdrop angles, $06CF less $140, HUD panel A
 * (FrameRunGsuAB13A), the second gauge (A = $70:2478, X = $2142), 07:DA65,
 * another switch and $0357 cleared; and keeps time as RaceFrameRun does.
 * HOW: As RaceFrameRun: data bank $02, its own; $00 = 2; P and the data bank
 * put back at the end.
 * Entry: any widths; D = $0000. Exit: P and the data bank unchanged; A low
 * byte 0; X, Y as the last call left them.
 */
void abi_RaceFrameRunTwoPlayers(void)
{
    uint8_t p0 = cpu.p, db = cpu.db;
    uint32_t b;

    frame_start();
    op_sep(P_M);
    op_rep(P_X);
    abi_ObjectsRunHandlers();
    frame_view_from(0x0346);
    abi_PlayerControlsFrame();
    frame_side0();
    op_sep(P_M);
    op_rep(P_X);
    abi_ViewUpdateAndGsuAEF8();
    op_rep(P_M);
    abi_PaletteFadeStep();
    abi_WordCopyStep();
    abi_BackdropAnglesFromView();
    frame_view_from(0x034A);
    op_sep(P_M);
    b = (uint32_t)cpu.db << 16;
    frame_lda8_imm(0x01);
    wr8(b | 0x1F07, 0x01);
    op_rep(P_M);
    wr16(b | 0x1AE8, 0x0000);
    op_sep(P_M);
    abi_FrameRunGsuAB13B();
    op_rep(P_M | P_X);
    cpu.x = 0x2143;
    cpu.p = nz16(cpu.p, cpu.x);
    op_lda16(0x702476);
    abi_SpeedGaugeUpdate();
    op_sep(P_M);
    abi_RaceSoundsPlayer1();
    abi_GsuScreenFlipMenu();
    op_sep(P_M);
    wr8((uint32_t)cpu.db << 16 | 0x0357, 0x00);
    abi_ViewUpdateAndGsuAEF8();
    abi_BackdropAnglesFromView();
    op_rep(P_M);
    b = (uint32_t)cpu.db << 16;
    op_lda16(b | 0x06CF);
    op_sub16(0x0140);
    wr16(b | 0x06CF, cpu.a);
    op_sep(P_M);
    abi_FrameRunGsuAB13A();
    op_rep(P_M | P_X);
    cpu.x = 0x2142;
    cpu.p = nz16(cpu.p, cpu.x);
    op_lda16(0x702478);
    abi_SpeedGauge2Update();
    abi_RaceSoundsPlayer2();
    op_sep(P_M);
    abi_GsuScreenFlipMenu();
    op_sep(P_M);
    wr8((uint32_t)cpu.db << 16 | 0x0357, 0x00);
    frame_time(db, p0);
}

void abi_GsuEE88Ramp(void);
void abi_SoftResetLong(void);
void abi_ScrollHdmaSet(void);
void abi_OamBufferHideLargeLong(void);
void abi_RaceLoad(void);
void abi_LoopLeaveWhenEnded(void);

/* A byte pushed on the real stack (bank 0). */
static void loop_push8(uint8_t v)
{
    wr8((uint16_t)cpu.s, v);
    cpu.s--;
}

/*
 * @routine 02:E289 RaceLoop
 * WHAT IT DOES: The race's frame loop (unless $10CD is set: nothing): packs
 * the three colour settings $1184-$1186 into a BGR word at $0F11, sets the
 * players $0D62 from the HUD mode (the table after the loop: 2 for mode 2),
 * loads the race (RaceLoad), sets $0D3F = $10, clears the fade, the fade
 * out and $0D40, $0D43 and $0D50, turns the NMI, the IRQ and the joypad
 * read on ($4200 = $31) and interrupts on; then each frame, after the
 * vertical blank ($0D52) and the Super FX's frame ($0D81): GsuEE88Ramp, every
 * eighth frame the next of four sprite pieces ($10BB), $10C1 = $10C5 = 0,
 * $0D7F = 1, the soft reset check, the scroll HDMA, the large sprites hidden,
 * LoopLeaveWhenEnded (which leaves the loop when the race's fade out is done)
 * and 02:E356.
 * HOW: The packing through direct page $F1; then P and the data bank pushed
 * (the data bank $02) for LoopLeaveWhenEnded to pull on the way out.
 * Entry: any widths; D = $0000; called by JSL. Exit: through
 * LoopLeaveWhenEnded: as the caller left P and the data bank; or, with
 * $10CD set, at once.
 */
void abi_RaceLoop(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;

    op_sep(P_M);
    op_lda8(b | 0x10CD);
    if ((uint8_t)cpu.a)
        return;
    op_rep(P_M);
    op_lda16(b | 0x1184);
    op_and16(0x001F);
    wr16(0x0000F1, cpu.a);
    op_lda16(b | 0x1185);
    op_asl16();
    op_asl16();
    op_asl16();
    op_asl16();
    op_asl16();
    op_and16(0x03E0);
    cpu.a |= rd16(0x0000F1);
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(0x0000F1, cpu.a);
    op_lda16(b | 0x1186);
    op_xba();
    op_asl16();
    op_asl16();
    op_and16(0x7C00);
    cpu.a |= rd16(0x0000F1);
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(b | 0x0F11, cpu.a);
    loop_push8(cpu.p);
    op_sep(P_M | P_X);
    loop_push8(cpu.db);
    cpu.db = 0x02;
    cpu.p = nz8(cpu.p, 0x02);
    b = 0x020000;
    cpu.x = rd8(b | 0x0D2B);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    v = rd8(0x02E338 + cpu.x);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    wr8(b | 0x0D62, v);
    abi_RaceLoad();
    op_sep(P_M);
    b = (uint32_t)cpu.db << 16;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x10);
    cpu.p = nz8(cpu.p, 0x10);
    wr8(b | 0x0D3F, 0x10);
    wr8(b | 0x0D61, 0x00);
    wr8(b | 0x0E3A, 0x00);
    wr8(b | 0x0D43, 0x00);
    wr8(b | 0x0D40, 0x00);
    wr8(b | 0x0D50, 0x00);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x31);
    cpu.p = nz8(cpu.p, 0x31);
    wr8(b | 0x4200, 0x31);
    cpu.p &= (uint8_t)~P_I;
    for (;;) {
        op_sep(P_M);
        b = (uint32_t)cpu.db << 16;
        do
            op_lda8(b | 0x0D52);
        while (!(uint8_t)cpu.a);
        wr8(b | 0x0D52, 0x00);
        op_rep(P_M);
        do
            op_lda16(b | 0x0D81);
        while (!cpu.a);
        wr16(b | 0x0D81, 0x0000);
        abi_GsuEE88Ramp();
        op_sep(P_M);
        b = (uint32_t)cpu.db << 16;
        op_lda8(b | 0x0E3B);
        v = (uint8_t)(cpu.a & 0x07);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 7)) & ~P_C) | (v >= 7 ? P_C : 0));
        if (v == 7) {
            op_lda8(b | 0x10BB);
            v = (uint8_t)(cpu.a + 1);
            cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 4)) & ~P_C) | (v >= 4 ? P_C : 0));
            if (v >= 4)
                v = 0;
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
            cpu.p = nz8(cpu.p, v);
            wr8(b | 0x10BB, v);
        }
        cpu.a = (uint16_t)(cpu.a & 0xFF00);
        cpu.p = nz8(cpu.p, 0x00);
        wr8(b | 0x10C5, 0x00);
        wr8(b | 0x10C1, 0x00);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
        cpu.p = nz8(cpu.p, 0x01);
        wr8(b | 0x0D7F, 0x01);
        abi_SoftResetLong();
        abi_ScrollHdmaSet();
        abi_OamBufferHideLargeLong();
        /* JSR LoopLeaveWhenEnded: its return address (02:E332) on the
           stack, which it drops on the way out of the loop. */
        loop_push8(0xE3);
        loop_push8(0x32);
        abi_LoopLeaveWhenEnded();
        if (cpu.unwound)
            return;
        cpu.s = (uint16_t)(cpu.s + 2);
        call_routine(0x02E356, 0);
    }
}

void abi_Set7EBC2BTrio(void);
void abi_WaitScanline(void);
void abi_SpriteSlotFirstLong(void);
void abi_RaceHudLoad(void);
void abi_ViewParamsByPlayers(void);

/*
 * @routine 7E:F29F AttractViewStart
 * @alias 02:CFC6
 * WHAT IT DOES: Starts the attract race's view: $70:01D0 = $1234, $70:01C4 =
 * 1, $70:108C = the players ($0D62), direct page $A0 = $80, $05EF = $FF and
 * $00 = 0, the view's parameters for the players (ViewParamsByPlayers), then
 * on at 03:AE73, which returns to this routine's caller.
 * HOW: Run from its ROM copy at $02:CFC6 (the JSR reaches $02:D410, the
 * copy of ViewParamsByPlayers); P pushed first, a long jump to finish.
 * Entry: any widths; D = $0000. Exit: as 03:AE73 leaves it.
 */
void abi_AttractViewStart(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t pushed[1] = {cpu.p};

    op_rep(P_M);
    op_lda16_imm(0x1234);
    wr16(0x7001D0, 0x1234);
    op_lda16_imm(0x0001);
    wr16(0x7001C4, 0x0001);
    op_sep(P_M);
    op_lda8(b | 0x0D62);
    wr8(0x70108C, (uint8_t)cpu.a);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x80);
    cpu.p = nz8(cpu.p, 0x80);
    wr8((uint16_t)(cpu.d + 0xA0), 0x80);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xFF);
    cpu.p = nz8(cpu.p, 0xFF);
    wr8(b | 0x05EF, 0xFF);
    wr8((uint16_t)cpu.d, 0x00);
    op_sep(P_M | P_X);
    abi_ViewParamsByPlayers();
    /* 03:AE73 ends with the PLP of this routine's PHP. */
    jump_routine_pushed(0x03AE73, pushed, 1);
}

/*
 * @routine 7E:F21E AttractCourseSetup
 * @alias 02:CF45
 * WHAT IT DOES: Sets up the attract race's course: data bank 0, interrupts
 * and HDMA off, the screen blanked (A = $80 after Set7EBC2BTrio), $084B and
 * $084C cleared, waits for lines $6E and $64 (WaitScanline), clears the held
 * buttons $0309, $1168, $08EF and $05D5, starts the view (AttractViewStart),
 * the first sprite slot and the HUD (RaceHudLoad), then Super FX program
 * $01:D1EB; P and the data bank put back and interrupts on.
 * HOW: Run from its ROM copy at $02:CF45; the clears as the original makes
 * them, $1168 seven times.
 * Entry: any widths. Exit: P and the data bank as they were, then I clear;
 * A, X, Y as GsuRun leaves them.
 */
void abi_AttractCourseSetup(void)
{
    uint8_t p0 = cpu.p, db0 = cpu.db;
    static const uint32_t clear16[] = {0x0309, 0x1168, 0x1168, 0x1168};
    static const uint32_t clear8[] = {0x08EF, 0x1168, 0x1168, 0x1168, 0x05D5, 0x1168, 0x1168, 0x1168, 0x1168};
    unsigned i;

    op_sep(P_M);
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    cpu.p = nz8(cpu.p, 0x00);
    cpu.db = 0x00;
    cpu.p = nz8(cpu.p, 0x00);
    cpu.p |= P_I;
    op_sep(P_M);
    wr8(HDMAEN, 0x00);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x80);
    cpu.p = nz8(cpu.p, 0x80);
    abi_Set7EBC2BTrio();
    wr8(INIDISP, (uint8_t)cpu.a);
    wr8(0x00084B, 0x00);
    wr8(0x00084C, 0x00);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x6E);
    cpu.p = nz8(cpu.p, 0x6E);
    wr8(0x0006E3, 0x6E);
    abi_WaitScanline();
    op_sep(P_M);
    op_rep(P_X);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x64);
    cpu.p = nz8(cpu.p, 0x64);
    wr8(0x0006E3, 0x64);
    abi_WaitScanline();
    op_sep(P_M);
    op_rep(P_X);
    op_rep(P_M | P_X);
    for (i = 0; i < sizeof clear16 / sizeof clear16[0]; i++)
        wr16(clear16[i], 0x0000);
    op_sep(P_M);
    for (i = 0; i < sizeof clear8 / sizeof clear8[0]; i++)
        wr8(clear8[i], 0x00);
    abi_AttractViewStart();
    abi_SpriteSlotFirstLong();
    abi_RaceHudLoad();
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.p = nz8(cpu.p, 0x01);
    cpu.x = 0xD1EB;
    cpu.p = nz16(cpu.p, cpu.x);
    abi_GsuRun();
    op_sep(P_M);
    op_rep(P_X);
    cpu.db = db0;
    cpu.p = nz8(cpu.p, db0);
    op_plp(p0);
    cpu.p &= (uint8_t)~P_I;
}
