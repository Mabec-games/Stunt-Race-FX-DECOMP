/*
 * race_frame.c: a race frame on the S-CPU, and the race's start, which
 * sets up the Super FX, runs the course script and steps two frames.
 */
#include "machine.h"
#include "hw.h"

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
