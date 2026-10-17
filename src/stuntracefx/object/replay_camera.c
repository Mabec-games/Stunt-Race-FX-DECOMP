/*
 * replay_camera.c: the replay camera director's handlers of bank $08
 * (08:8F03-958E). A director object (X) moves the camera actor its +$4F
 * links to ($1A64) around its car (+$06): the camera's mode word at +$00
 * ($0001 follow, $0002/$0003 free, $065x on a path), its point at
 * +$08/+$0A/+$0C, its zoom +$0E. Each handler sets the next frame and
 * falls into it; frames end in the director's tail (CameraFollowLink,
 * +$36 = 3, 08:8EF9) or go back to the director's table (08:8EC1).
 *
 * Objects are at $2000 + X and the camera at $0000 + Y through the data
 * bank (X 16-bit, M 8-bit).
 */
#include "machine.h"
#include "cpu_ops.h"

void abi_CameraFollowLink(void);
void abi_CameraPointFromTable(void);
void abi_CameraDirectorTable(void);
void abi_CameraDirectorSkip2(void);
void abi_CameraApproachTarget(void);
void abi_ObjectPathStart(void);
void abi_ObjectVelocityFromRecordOf(void);
void abi_ApproachEighthMin8(void);
void abi_ApproachThirtySecondMin32(void);
void abi_ApproachSixtyFourthMin64(void);
void abi_ScaleThreeQuarters(void);
void abi_GsuTableEntryNext(void);
void abi_GsuTableEntryTo1F4C(void);
void abi_SoundPort0QueuePush(void);

static uint32_t db(void)
{
    return (uint32_t)cpu.db << 16;
}

/* Object field `f` ($20xx) of object `o`, through the data bank. */
#define F(f, o) (db() | (uint16_t)((f) + (o)))

/* Camera field `o` of the camera in Y, through the data bank. */
static uint32_t cam(uint16_t o)
{
    return db() | (uint16_t)(cpu.y + o);
}

/* LDA #v with M 8-bit. */
static void lda8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* LDA #v, STA addr, 8-bit. */
static void st8i(uint32_t addr, uint8_t v)
{
    lda8(v);
    wr8(addr, v);
}

/* REP #$20, LDA #v, STA addr, SEP #$20. */
static void st16i(uint32_t addr, uint16_t v)
{
    cpu.p &= (uint8_t)~P_M;
    op_lda16_imm(v);
    wr16(addr, v);
    cpu.p |= P_M;
}

/* REP #$20, LDA src, STA dst, SEP #$20 (16-bit copy). */
static void copy16(uint32_t dst, uint32_t src)
{
    cpu.p &= (uint8_t)~P_M;
    op_lda16(src);
    wr16(dst, cpu.a);
    cpu.p |= P_M;
}

/* LDA addr, AND #m, STA addr (8-bit). */
static void and8(uint32_t addr, uint8_t m)
{
    uint8_t v = (uint8_t)(rd8(addr) & m);

    lda8(v);
    wr8(addr, v);
}

/* The handler of object o: +$1A = h, +$1C = $08. */
static void handler(uint16_t o, uint16_t h)
{
    st16i(F(0x201A, o), h);
    st8i(F(0x201C, o), 0x08);
}

/* LDY +$4F: Y = the director's camera. */
static void link(void)
{
    cpu.y = rd16(F(0x204F, cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
}

/* LDY +$06: Y = the director's car. */
static void car(void)
{
    cpu.y = rd16(F(0x2006, cpu.x));
    cpu.p = nz16(cpu.p, cpu.y);
}

/* The camera's mode word +$00 = v (LDY +$4F first). */
static void mode(uint16_t v)
{
    link();
    st16i(cam(0x0000), v);
}

/* 08:8EF9: the director's tail: CameraFollowLink, +$36 = 3. */
static void tail(void)
{
    abi_CameraFollowLink();
    st8i(F(0x2036, cpu.x), 0x03);
}

/* 08:8EC1: back to the director's table: the camera's +$04 and +$34
   cleared, +$25 bits 0 and 1 cleared, frame $08:8EF5
   (CameraDirectorTable), then that frame. */
static void table_start(void)
{
    link();
    st16i(cam(0x0004), 0x0000);
    link();
    st8i(cam(0x0034), 0x00);
    and8(F(0x2025, cpu.x), 0xFE);
    and8(F(0x2025, cpu.x), 0xFD);
    handler(cpu.x, 0x8EF5);
    abi_CameraDirectorTable();
}

/* The camera put back on its car: mode 1, its turn words +$26-$30
   cleared, its +$1A-$1E from +$20-$24, CameraFollowLink. */
static void snap(void)
{
    mode(0x0001);
    link();
    st16i(cam(0x0026), 0x0000);
    st16i(cam(0x0028), 0x0000);
    st16i(cam(0x002A), 0x0000);
    link();
    st16i(cam(0x002C), 0x0000);
    st16i(cam(0x002E), 0x0000);
    st16i(cam(0x0030), 0x0000);
    link();
    copy16(cam(0x001A), cam(0x0020));
    copy16(cam(0x001C), cam(0x0022));
    copy16(cam(0x001E), cam(0x0024));
    abi_CameraFollowLink();
}

/*
 * @routine 08:8F03 CameraSnapToCar
 * WHAT IT DOES: Puts the camera back on its car at once: mode 1 for the
 * moment, its turn words +$26-$30 cleared, +$1A-$1E from +$20-$24, one
 * CameraFollowLink; then its mode word +$00 as it was.
 * HOW: The mode word kept on the stack (PHA/PLA, 16-bit).
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: M 8-bit; A = the mode
 * word; N, Z from it.
 */
void abi_CameraSnapToCar(void)
{
    uint16_t m;

    link();
    cpu.p &= (uint8_t)~P_M;
    op_lda16(cam(0x0000));
    m = cpu.a;
    cpu.p |= P_M;
    snap();
    link();
    cpu.p &= (uint8_t)~P_M;
    cpu.a = m;
    cpu.p = nz16(cpu.p, m);
    wr16(cam(0x0000), m);
    cpu.p |= P_M;
}

/* (a - b) halved toward zero, plus b: the move half way (16-bit). */
static uint16_t half_way(uint16_t a, uint32_t at)
{
    uint8_t c;

    cpu.p |= P_C;
    cpu.a = op_sbc16(a, rd16(at), &cpu.p);
    op_cmp16(0x8000);
    c = (uint8_t)(cpu.a & 1);
    cpu.a = (uint16_t)((cpu.a >> 1) | ((cpu.p & P_C) ? 0x8000 : 0));
    cpu.p = (uint8_t)(nz16((uint8_t)(cpu.p & ~P_C), cpu.a) | c);
    if (cpu.a & 0x8000)
        cpu.a = op_adc16(cpu.a, 0x0000, &cpu.p);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, rd16(at), &cpu.p);
    return cpu.a;
}

/* REP #$20, PHX, the car's record word +$42 (long, through X) set by
   `op` (1: ORA #$1010, 0: AND #$EFFF), SEP #$20, PLX. */
static void car_rec42(int op)
{
    uint16_t sx = cpu.x;
    uint32_t at;

    cpu.p &= (uint8_t)~P_M;
    car();
    op_lda16(F(0x2022, cpu.y));
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
    at = (0x700042 + cpu.x) & 0xFFFFFF;
    op_lda16(at);
    cpu.a = op ? (uint16_t)(cpu.a | 0x1010) : (uint16_t)(cpu.a & 0xEFFF);
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(at, cpu.a);
    cpu.p |= P_M;
    cpu.x = sx;
    cpu.p = nz16(cpu.p, sx);
}

/* 08:8FFE: the overhead shot's frame (see CameraOverheadStep). */
static void overhead_frame(void)
{
    uint16_t sx, t;
    uint8_t v;

    op_lda8(F(0x2048, cpu.x));
    if ((uint8_t)cpu.a) {
        v = (uint8_t)(rd8(F(0x2048, cpu.x)) - 1);
        wr8(F(0x2048, cpu.x), v);
        cpu.p = nz8(cpu.p, v);
        car_rec42(1);
    } else {
        car_rec42(0);
    }
    link();
    lda8((uint8_t)(rd8(F(0x2025, cpu.x)) & 0x01));
    if ((uint8_t)cpu.a) {
        and8(F(0x2025, cpu.x), 0xFE);
        op_lda8(cam(0x0034));
        v = (uint8_t)(cpu.a + 1);
        lda8(v);
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 2)) & ~P_C) | (v >= 2 ? P_C : 0));
        if (!((uint8_t)(v - 2) & 0x80)) {
            lda8(0x00);
            wr8(cam(0x0034), 0x00);
            lda8(0x2F);
            abi_SoundPort0QueuePush();
            st16i(F(0x200E, cpu.x), 0xFC18);
        } else {
            wr8(cam(0x0034), v);
            lda8(0x24);
            abi_SoundPort0QueuePush();
            st16i(F(0x200E, cpu.x), 0xF830);
        }
    }
    cpu.p &= (uint8_t)~P_M;
    op_lda16(F(0x200E, cpu.x));
    t = half_way(cpu.a, cam(0x000A));
    wr16(cam(0x000A), t);
    op_lda16(F(0x200C, cpu.x));
    wr16(db() | 0x1F4C, cpu.a);
    op_lda16(F(0x2010, cpu.x));
    wr16(db() | 0x1F50, cpu.a);
    sx = cpu.x;
    link();
    op_lda16(cam(0x0002));
    cpu.x = cpu.y;
    cpu.p = nz16(cpu.p, cpu.x);
    cpu.y = cpu.a;
    cpu.p = nz16(cpu.p, cpu.y);
    op_lda16(F(0x200C, cpu.y));
    cpu.p |= P_C;
    cpu.a = op_sbc16(cpu.a, rd16(db() | 0x1F4C), &cpu.p);
    abi_ScaleThreeQuarters();
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, rd16(db() | 0x1F4C), &cpu.p);
    wr16((uint16_t)(cpu.d + 0x08 + cpu.x), cpu.a);
    op_lda16(F(0x2010, cpu.y));
    cpu.p |= P_C;
    cpu.a = op_sbc16(cpu.a, rd16(db() | 0x1F50), &cpu.p);
    abi_ScaleThreeQuarters();
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, rd16(db() | 0x1F50), &cpu.p);
    wr16((uint16_t)(cpu.d + 0x0C + cpu.x), cpu.a);
    cpu.p |= P_M;
    cpu.x = sx;
    cpu.p = nz16(cpu.p, sx);
    tail();
}

/*
 * @routine 08:8FFE CameraOverheadStep
 * WHAT IT DOES: The overhead shot's frame: while +$48 counts down the car's
 * record +$42 gets bits 4 and 12, after it bit 12 goes; on a cue (+$25 bit
 * 0, cleared) the camera's +$34 flips between 1 (sound $24, height -$7D0)
 * and 0 (sound $2F, height -$3E8); the camera's height +$0A moves half way
 * to the director's (rounded toward it) and its X/Z three quarters of the
 * way from the director's toward the car's (ScaleThreeQuarters); then the
 * director's tail.
 * HOW: 16-bit SBC, CMP #$8000 and ROR for the signed half; the X and Z
 * stored through the direct page with X = the camera.
 * Entry: M 8-bit, X 16-bit; X = the director; D = $0000. Exit: as the tail
 * leaves it.
 */
void abi_CameraOverheadStep(void)
{
    overhead_frame();
}

/*
 * @routine 08:8F8E CameraOverhead
 * WHAT IT DOES: The overhead shot: the director at the car's X and Z ($12C
 * nearer) at height -$3E8, the camera's point there too, mode 3, one
 * CameraFollowLink; frame $08:8FFE (CameraOverheadStep), +$48 = $0A; then
 * that frame.
 * HOW: The positions through $1F4C-$1F50 (16-bit). Reached only by
 * unit_replay_camera_08.
 * Entry: M 8-bit, X 16-bit; X = the director; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_CameraOverhead(void)
{
    link();
    cpu.p &= (uint8_t)~P_M;
    op_lda16(cam(0x0002));
    cpu.y = cpu.a;
    cpu.p = nz16(cpu.p, cpu.y);
    op_lda16(F(0x200C, cpu.y));
    wr16(db() | 0x1F4C, cpu.a);
    wr16(F(0x200C, cpu.x), cpu.a);
    op_lda16_imm(0xFC18);
    wr16(db() | 0x1F4E, cpu.a);
    wr16(F(0x200E, cpu.x), cpu.a);
    op_lda16(F(0x2010, cpu.y));
    op_add16(0xFED4);
    wr16(db() | 0x1F50, cpu.a);
    wr16(F(0x2010, cpu.x), cpu.a);
    cpu.p |= P_M;
    link();
    copy16(cam(0x0008), db() | 0x1F4C);
    copy16(cam(0x000A), db() | 0x1F4E);
    copy16(cam(0x000C), db() | 0x1F50);
    mode(0x0003);
    abi_CameraFollowLink();
    handler(cpu.x, 0x8FFE);
    st8i(F(0x2048, cpu.x), 0x0A);
    overhead_frame();
}

/*
 * @routine 08:9176 CameraTableStep
 * WHAT IT DOES: A frame: the camera's point from the director's table
 * (CameraPointFromTable), then the director's tail.
 * HOW: JSL, JMP to 08:8EF9.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void abi_CameraTableStep(void)
{
    abi_CameraPointFromTable();
    tail();
}

/*
 * @routine 08:90D9 CameraSnapToTable
 * WHAT IT DOES: The camera put back on its car (mode 1, turn words cleared,
 * CameraFollowLink), its +$04 cleared, mode $0755, frame $08:9176
 * (CameraTableStep); then that frame.
 * HOW: Word stores. Reached only by unit_replay_camera_08.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraSnapToTable(void)
{
    snap();
    link();
    st16i(cam(0x0004), 0x0000);
    mode(0x0755);
    handler(cpu.x, 0x9176);
    abi_CameraTableStep();
}

/* 08:91D0: the path shot's frame: CameraApproachTarget; arrived (carry),
   $70:273C bit 0 set and back to the table; else the tail. */
static void path_frame(void)
{
    abi_CameraApproachTarget();
    if (!(cpu.p & P_C)) {
        tail();
        return;
    }
    cpu.p &= (uint8_t)~P_M;
    op_lda16(0x70273C);
    cpu.a |= 0x0001;
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(0x70273C, cpu.a);
    cpu.p |= P_M;
    table_start();
}

/* A path shot: frame $08:91D0, mode 1, the path at Y (ObjectPathStart),
   CameraFollowLink, then the frame. */
static void path_shot(uint16_t path)
{
    cpu.y = path;
    cpu.p = nz16(cpu.p, cpu.y);
    abi_ObjectPathStart();
    abi_CameraFollowLink();
    path_frame();
}

/*
 * @routine 08:917D CameraPathA
 * WHAT IT DOES: A path shot: frame $08:91D0 (CameraPathStep), mode 1, the
 * director on path $885F (ObjectPathStart), CameraFollowLink; then that
 * frame.
 * HOW: Word stores; a JMP into the shared start at 08:91C8.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraPathA(void)
{
    handler(cpu.x, 0x91D0);
    mode(0x0001);
    path_shot(0x885F);
}

/*
 * @routine 08:919F CameraPathB
 * WHAT IT DOES: As CameraPathA on path $882D, or $85B9 in HUD mode 3.
 * HOW: CMP #3 on $0D2B.
 * Entry: as CameraPathA. Exit: as CameraPathA.
 */
void abi_CameraPathB(void)
{
    uint8_t h;

    handler(cpu.x, 0x91D0);
    mode(0x0001);
    cpu.y = 0x882D;
    cpu.p = nz16(cpu.p, cpu.y);
    op_lda8(db() | 0x0D2B);
    h = (uint8_t)cpu.a;
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(h - 3)) & ~P_C) | (h >= 3 ? P_C : 0));
    path_shot(h == 3 ? 0x85B9 : 0x882D);
}

/*
 * @routine 08:91D0 CameraPathStep
 * WHAT IT DOES: The path shot's frame: the director moves on along its path
 * (CameraApproachTarget); once there, $70:273C bit 0 is set and the
 * director goes back to its table (08:8EC1); else the director's tail.
 * HOW: Carry from CameraApproachTarget; a 16-bit long OR.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void abi_CameraPathStep(void)
{
    path_frame();
}

/* The drift frames: the camera's point moved by the director's velocity
   (+$2E/+$30/+$32), the velocity eased toward (0, ty, 0)
   (ApproachEighthMin8); while +$48 counts down the tail, then `done`. */
static void drift_frame(uint16_t ty, void (*done)(void))
{
    static const uint16_t v[3] = {0x202E, 0x2030, 0x2032};
    static const uint16_t c[3] = {0x0008, 0x000A, 0x000C};
    uint16_t t[3];
    int i;

    t[0] = 0x0000;
    t[1] = ty;
    t[2] = 0x0000;
    cpu.p &= (uint8_t)~P_M;
    link();
    for (i = 0; i < 3; i++) {
        op_lda16(F(v[i], cpu.x));
        op_add16(rd16(cam(c[i])));
        wr16(cam(c[i]), cpu.a);
    }
    cpu.p |= P_M;
    for (i = 0; i < 3; i++) {
        cpu.p &= (uint8_t)~P_M;
        op_lda16(F(v[i], cpu.x));
        wr16(0x00003E, cpu.a);
        op_lda16_imm(t[i]);
        abi_ApproachEighthMin8();
        wr16(F(v[i], cpu.x), cpu.a);
        cpu.p |= P_M;
    }
    op_lda8(F(0x2048, cpu.x));
    if (!(uint8_t)cpu.a) {
        done();
        return;
    }
    {
        uint8_t n = (uint8_t)(rd8(F(0x2048, cpu.x)) - 1);

        wr8(F(0x2048, cpu.x), n);
        cpu.p = nz8(cpu.p, n);
    }
    tail();
}

/* A drift shot: frame f, mode 3, +$48 = n, the director's velocity an
   eighth of its car's record velocity (Y cleared with `flat`). */
static void drift_shot(uint16_t f, uint8_t n, int flat)
{
    handler(cpu.x, f);
    mode(0x0003);
    st8i(F(0x2048, cpu.x), n);
    car();
    abi_ObjectVelocityFromRecordOf();
    if (flat)
        st16i(F(0x2030, cpu.x), 0x0000);
}

/*
 * @routine 08:921D CameraDriftStep
 * WHAT IT DOES: The drift shot's frame: the camera's point moves by the
 * director's velocity, which eases toward 0 (ApproachEighthMin8); while
 * +$48 counts down the director's tail, then CameraDirectorSkip2.
 * HOW: 16-bit ADCs; ApproachEighthMin8 through $3E; JML.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void abi_CameraDriftStep(void)
{
    drift_frame(0x0000, abi_CameraDirectorSkip2);
}

/*
 * @routine 08:91EB CameraDrift
 * WHAT IT DOES: The drift shot: frame $08:921D (CameraDriftStep), mode 3,
 * +$48 = $1E, the director's velocity from its car's record
 * (ObjectVelocityFromRecordOf) with no Y; then that frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraDrift(void)
{
    drift_shot(0x921D, 0x1E, 1);
    abi_CameraDriftStep();
}

/*
 * @routine 08:9311 CameraDriftLongStep
 * WHAT IT DOES: As CameraDriftStep (its own copy of the code).
 * HOW: As CameraDriftStep. Reached only by unit_replay_camera_08.
 * Entry: as CameraDriftStep. Exit: as CameraDriftStep.
 */
void abi_CameraDriftLongStep(void)
{
    drift_frame(0x0000, abi_CameraDirectorSkip2);
}

/*
 * @routine 08:92DF CameraDriftLong
 * WHAT IT DOES: As CameraDrift with +$48 = $28 and frame $08:9311
 * (CameraDriftLongStep).
 * HOW: As CameraDrift. Reached only by unit_replay_camera_08.
 * Entry: as CameraDrift. Exit: as CameraDrift.
 */
void abi_CameraDriftLong(void)
{
    drift_shot(0x9311, 0x28, 1);
    abi_CameraDriftLongStep();
}

/*
 * @routine 08:946C CameraZoomIn
 * WHAT IT DOES: The zoom-in: frame $08:9488 (CameraZoomInStep), mode 2;
 * then that frame.
 * HOW: Word stores.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraZoomInStep(void);

void abi_CameraZoomIn(void)
{
    handler(cpu.x, 0x9488);
    mode(0x0002);
    abi_CameraZoomInStep();
}

/*
 * @routine 08:93FF CameraRiseStep
 * WHAT IT DOES: The rising drift's frame: as CameraDriftStep with the Y
 * velocity eased toward -$0A; when +$48 runs out, CameraZoomIn.
 * HOW: As CameraDriftStep; a JML into CameraZoomIn.
 * Entry: as CameraDriftStep. Exit: as the path leaves it.
 */
void abi_CameraRiseStep(void)
{
    drift_frame(0xFFF6, abi_CameraZoomIn);
}

/*
 * @routine 08:93D7 CameraRise
 * WHAT IT DOES: The rising drift: frame $08:93FF (CameraRiseStep), mode 3,
 * +$48 = $28, the director's velocity from its car's record; then that
 * frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraRise(void)
{
    drift_shot(0x93FF, 0x28, 0);
    abi_CameraRiseStep();
}

/* The fixed shots: frame f (a JMP to the tail), mode 3, the point from the
   car's table entry (GsuTableEntryNext `n` times, GsuTableEntryTo1F4C)
   moved by (dx, -$64, dz); then the tail. */
static void fixed_shot(uint16_t f, int n, uint16_t dx, uint16_t dz)
{
    uint16_t sx;

    handler(cpu.x, f);
    mode(0x0003);
    sx = cpu.x;
    car();
    cpu.p &= (uint8_t)~P_M;
    op_lda16(F(0x2022, cpu.y));
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
    op_lda16((0x700058 + cpu.x) & 0xFFFFFF);
    cpu.x = sx;
    cpu.p = nz16(cpu.p, sx);
    while (n--)
        abi_GsuTableEntryNext();
    abi_GsuTableEntryTo1F4C();
    link();
    op_lda16(db() | 0x1F4C);
    if (dx)
        op_add16(dx);
    wr16(cam(0x0008), cpu.a);
    op_lda16(db() | 0x1F4E);
    op_add16(0xFF9C);
    wr16(cam(0x000A), cpu.a);
    op_lda16(db() | 0x1F50);
    if (dz)
        op_add16(dz);
    wr16(cam(0x000C), cpu.a);
    cpu.p |= P_M;
    tail();
}

/*
 * @routine 08:92DC CameraHoldStep
 * WHAT IT DOES: A held shot's frame: the director's tail only.
 * HOW: JMP to 08:8EF9.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void abi_CameraHoldStep(void)
{
    tail();
}

/*
 * @routine 08:928A CameraFixedAhead
 * WHAT IT DOES: A fixed shot: frame $08:92DC (CameraHoldStep), mode 3, the
 * camera's point at the car's track table entry two on (GsuTableEntryNext
 * twice, GsuTableEntryTo1F4C), $64 up; then the tail.
 * HOW: The table pointer from the car's record +$58. Reached only by
 * unit_replay_camera_08.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void abi_CameraFixedAhead(void)
{
    fixed_shot(0x92DC, 2, 0x0000, 0x0000);
}

/*
 * @routine 08:93D4 CameraHoldStep2
 * WHAT IT DOES: As CameraHoldStep (its own JMP).
 * HOW: JMP to 08:8EF9. Reached only by unit_replay_camera_08.
 * Entry: as CameraHoldStep. Exit: as CameraHoldStep.
 */
void abi_CameraHoldStep2(void)
{
    tail();
}

/*
 * @routine 08:937E CameraFixedSide
 * WHAT IT DOES: A fixed shot: frame $08:93D4 (CameraHoldStep2), mode 3, the
 * camera's point at the car's next track table entry moved ($3E8, -$64,
 * $3E8); then the tail.
 * HOW: As CameraFixedAhead with one GsuTableEntryNext. Reached only by
 * unit_replay_camera_08.
 * Entry: as CameraFixedAhead. Exit: as CameraFixedAhead.
 */
void abi_CameraFixedSide(void)
{
    fixed_shot(0x93D4, 1, 0x03E8, 0x03E8);
}

/* A zoom frame: the camera's zoom +$0E eased toward `to` by `fn`; the tail. */
static void zoom_frame(uint16_t to, void (*fn)(void))
{
    link();
    cpu.p &= (uint8_t)~P_M;
    op_lda16(cam(0x000E));
    wr16(0x00003E, cpu.a);
    op_lda16_imm(to);
    fn();
    wr16(cam(0x000E), cpu.a);
    cpu.p |= P_M;
    tail();
}

/*
 * @routine 08:9488 CameraZoomInStep
 * WHAT IT DOES: The zoom-in's frame: the camera's zoom +$0E eased toward
 * $1000 (ApproachThirtySecondMin32); the tail.
 * HOW: Through $3E (16-bit).
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void abi_CameraZoomInStep(void)
{
    zoom_frame(0x1000, abi_ApproachThirtySecondMin32);
}

/*
 * @routine 08:9502 CameraZoomOutStep
 * WHAT IT DOES: The zoom-out's frame: the zoom +$0E eased toward $2000
 * (ApproachSixtyFourthMin64); the tail.
 * HOW: Through $3E (16-bit).
 * Entry: as CameraZoomInStep. Exit: as CameraZoomInStep.
 */
void abi_CameraZoomOutStep(void)
{
    zoom_frame(0x2000, abi_ApproachSixtyFourthMin64);
}

/*
 * @routine 08:94E6 CameraZoomOut
 * WHAT IT DOES: The zoom-out: frame $08:9502 (CameraZoomOutStep), mode
 * $0654; then that frame.
 * HOW: Word stores.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraZoomOut(void)
{
    handler(cpu.x, 0x9502);
    mode(0x0654);
    abi_CameraZoomOutStep();
}

/*
 * @routine 08:94DD CameraOrbitStep
 * WHAT IT DOES: The orbit shot's frame: the director moves along its path
 * (CameraApproachTarget); once there it runs into CameraZoomOut, else the
 * tail.
 * HOW: Carry from CameraApproachTarget; the fall into 08:94E6.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void abi_CameraOrbitStep(void)
{
    abi_CameraApproachTarget();
    if (cpu.p & P_C) {
        abi_CameraZoomOut();
        return;
    }
    tail();
}

/*
 * @routine 08:94A1 CameraOrbit
 * WHAT IT DOES: The orbit shot: frame $08:94DD (CameraOrbitStep), mode
 * $0655, the director on path $88BB (ObjectPathStart), the car's record
 * +$40 bit 2 set; then that frame.
 * HOW: Word stores; the record word through X (PHX/PLX, 16-bit).
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraOrbit(void)
{
    uint16_t sx;
    uint32_t at;

    handler(cpu.x, 0x94DD);
    mode(0x0655);
    cpu.y = 0x88BB;
    cpu.p = nz16(cpu.p, cpu.y);
    abi_ObjectPathStart();
    sx = cpu.x;
    cpu.p &= (uint8_t)~P_M;
    op_lda16(F(0x2006, cpu.x));
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
    op_lda16(F(0x2022, cpu.x));
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
    at = (0x700040 + cpu.x) & 0xFFFFFF;
    op_lda16(at);
    cpu.a |= 0x0004;
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(at, cpu.a);
    cpu.p |= P_M;
    cpu.x = sx;
    cpu.p = nz16(cpu.p, sx);
    abi_CameraOrbitStep();
}

/* LDY +$06, PHX, TYX, REP #$20, the car's record (through X = +$22): the
   word at `field` = v (read first with `read`), SEP #$20, PLX. */
static void car_rec_set(uint32_t field, uint16_t v, int read)
{
    uint16_t sx;
    uint32_t at;

    car();
    sx = cpu.x;
    cpu.x = cpu.y;
    cpu.p = nz16(cpu.p, cpu.x);
    cpu.p &= (uint8_t)~P_M;
    op_lda16(F(0x2022, cpu.x));
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
    at = (field + cpu.x) & 0xFFFFFF;
    if (read)
        op_lda16(at);
    op_lda16_imm(v);
    wr16(at, v);
    cpu.p |= P_M;
    cpu.x = sx;
    cpu.p = nz16(cpu.p, sx);
}

/*
 * @routine 08:9564 CameraEnd
 * WHAT IT DOES: The end of the replay shot: frame $08:958C (CameraEndStep),
 * the car's record +$1C = $10 (read first); then the tail.
 * HOW: The record word through X (PHX, TYX, PLX). Reached only by
 * unit_replay_camera_08.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void abi_CameraEnd(void)
{
    handler(cpu.x, 0x958C);
    car_rec_set(0x70001C, 0x0010, 1);
    tail();
}

/*
 * @routine 08:958C CameraEndStep
 * WHAT IT DOES: The end shot's frame: the director's tail only.
 * HOW: JMP to 08:8EF9. Reached only by unit_replay_camera_08.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void abi_CameraEndStep(void)
{
    tail();
}

/*
 * @routine 08:953C CameraStillStep
 * WHAT IT DOES: The still shot's frame: the car's record +$42 = $0410;
 * while +$48 counts down, the camera's point from the table
 * (CameraPointFromTable) and the tail; then CameraEnd.
 * HOW: The record word through X (PHX, TYX, PLX); a JML into CameraEnd.
 * Reached only by unit_replay_camera_08.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void abi_CameraStillStep(void)
{
    car_rec_set(0x700042, 0x0410, 0);
    op_lda8(F(0x2048, cpu.x));
    if (!(uint8_t)cpu.a) {
        abi_CameraEnd();
        return;
    }
    {
        uint8_t n = (uint8_t)(rd8(F(0x2048, cpu.x)) - 1);

        wr8(F(0x2048, cpu.x), n);
        cpu.p = nz8(cpu.p, n);
    }
    abi_CameraPointFromTable();
    tail();
}

/*
 * @routine 08:951B CameraStill
 * WHAT IT DOES: The still shot: frame $08:953C (CameraStillStep), +$48 =
 * $0A, the camera's +$37 word = $8000; then that frame.
 * HOW: Word and byte stores. Reached only by unit_replay_camera_08.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraStill(void)
{
    handler(cpu.x, 0x953C);
    st8i(F(0x2048, cpu.x), 0x0A);
    link();
    st16i(cam(0x0037), 0x8000);
    abi_CameraStillStep();
}
