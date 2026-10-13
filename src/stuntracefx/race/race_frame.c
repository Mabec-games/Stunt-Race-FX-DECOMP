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
            cpu.p = p0;
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
        cpu.p = p0;
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
        cpu.p = p1;
        if (p1 & P_X) {
            cpu.x &= 0x00FF;
            cpu.y &= 0x00FF;
        }
    }
    cpu.p |= P_M;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xFF);
    wr8(b | 0x0DF8, 0xFF);
    cpu.p = p0;
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
