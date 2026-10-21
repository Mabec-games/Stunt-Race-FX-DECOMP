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
    cyc(38);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* LDA #v, STA addr, 8-bit. */
static void st8i(uint32_t addr, uint8_t v)
{
    lda8(v);
    cyc(2);
    wr8(addr, v);
}

/* REP #$20, LDA #v, STA addr, SEP #$20. */
static void st16i(uint32_t addr, uint16_t v)
{
    cpu.p &= (uint8_t)~P_M;
    op_lda16_imm(v);
    cyc(38);
    wr16(addr, v);
    cpu.p |= P_M;
}

/* REP #$20, LDA src, STA dst, SEP #$20 (16-bit copy): `pre` clocks to the
   load's read, `mid` from there to the store. */
static void copy16x(uint32_t dst, uint32_t src, unsigned pre, unsigned mid)
{
    cpu.p &= (uint8_t)~P_M;
    cyc(pre);
    op_lda16(src);
    cyc(mid);
    wr16(dst, cpu.a);
    cpu.p |= P_M;
}

static void copy16(uint32_t dst, uint32_t src)
{
    copy16x(dst, src, 46, 30);
}

/* LDA addr, AND #m, STA addr (8-bit). */
static void and8(uint32_t addr, uint8_t m)
{
    cyc(30);
    uint8_t v = (uint8_t)(rd8(addr) & m);

    lda8(v);
    cyc(8);
    wr8(addr, v);
}

/* The handler of object o: +$1A = h, +$1C = $08. */
static void handler(uint16_t o, uint16_t h)
{
    st16i(F(0x201A, o), h);
    st8i(F(0x201C, o), cyc_then(28, 0x08));
}

/* LDY +$4F: Y = the director's camera. */
static void link(void)
{
    cpu.y = rd16(cyc_then(30, F(0x204F, cpu.x)));
    cpu.p = nz16(cpu.p, cpu.y);
}

/* LDY +$06: Y = the director's car; `jsl` is 62 when a JSL follows it. */
static void car_y(int jsl)
{
    cpu.y = rd16(cyc_then(30, F(0x2006, cpu.x)));
    cpu.p = nz16(cpu.p, cpu.y);
    if (jsl)
        cyc(62);
}

static void car(void)
{
    car_y(1);
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
    (cyc(62), abi_CameraFollowLink());
    st8i(F(0x2036, cpu.x), cyc_then(6, 0x03));
    cyc(44);
}

/* 08:8EC1: back to the director's table: the camera's +$04 and +$34
   cleared, +$25 bits 0 and 1 cleared, frame $08:8EF5
   (CameraDirectorTable), then that frame. */
static void table_start(void)
{
    (cyc(32), link());
    st16i(cam(0x0004), 0x0000);
    (cyc(22), link());
    cyc(6);
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
    (cyc(22), link());
    st16i(cam(0x0026), 0x0000);
    cyc(22);
    st16i(cam(0x0028), 0x0000);
    cyc(22);
    st16i(cam(0x002A), 0x0000);
    (cyc(22), link());
    st16i(cam(0x002C), 0x0000);
    cyc(22);
    st16i(cam(0x002E), 0x0000);
    cyc(22);
    st16i(cam(0x0030), 0x0000);
    (cyc(22), link());
    cyc(6);
    copy16(cam(0x001A), cam(0x0020));
    cyc(28);
    copy16(cam(0x001C), cam(0x0022));
    cyc(28);
    copy16(cam(0x001E), cam(0x0024));
    (cyc(84), abi_CameraFollowLink());
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
    op_lda16(cyc_then(52, cam(0x0000)));
    m = cpu.a;
    cpu.p |= P_M;
    (cyc(52), snap());
    link();
    cpu.p &= (uint8_t)~P_M;
    cpu.a = m;
    cpu.p = nz16(cpu.p, m);
    wr16(cam(0x0000), cyc_then(88, m));
    cpu.p |= P_M;
    cyc(64);
}

/* (a - b) halved toward zero, plus b: the move half way (16-bit). */
static uint16_t half_way(uint16_t a, uint32_t at)
{
    uint8_t c;

    cpu.p |= P_C;
    cyc(44);
    cpu.a = op_sbc16(a, rd16(at), &cpu.p);
    op_cmp16(0x8000);
    c = (uint8_t)(cpu.a & 1);
    cpu.a = (uint16_t)((cpu.a >> 1) | ((cpu.p & P_C) ? (cyc(18), 0x8000) : 0));
    cpu.p = (uint8_t)(nz16((uint8_t)(cpu.p & ~P_C), cpu.a) | c);
    if (cpu.a & 0x8000)
        cpu.a = op_adc16(cpu.a, 0x0000, &cpu.p);
    cpu.p &= (uint8_t)~P_C;
    cyc(98);
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
    (cyc(52), car_y(0));
    op_lda16(cyc_then(30, F(0x2022, cpu.y)));
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
    at = (0x700042 + cpu.x) & 0xFFFFFF;
    cyc(46);
    op_lda16(at);
    cpu.a = op ? (uint16_t)(cpu.a | 0x1010) : (uint16_t)(cpu.a & 0xEFFF);
    cpu.p = nz16(cpu.p, cpu.a);
    cyc(56);
    wr16(at, cpu.a);
    cpu.p |= P_M;
    cpu.x = sx;
    cpu.p = nz16(cpu.p, sx);
    cyc(op ? 80 : 58); /* SEP, PLX (, BRA) */
}

/* 08:8FFE: the overhead shot's frame (see CameraOverheadStep). */
static void overhead_frame(void)
{
    uint16_t sx, t;
    uint8_t v;
    unsigned pre = 78; /* to the load of +$0E: SEP (, BRA), REP, LDA */

    op_lda8(cyc_then(30, F(0x2048, cpu.x)));
    if ((uint8_t)cpu.a) {
        v = (uint8_t)(rd8(cyc_then(52, F(0x2048, cpu.x))) - 1);
        wr8(F(0x2048, cpu.x), cyc_then(6, v));
        cpu.p = nz8(cpu.p, v);
        car_rec42(1);
    } else {
        cyc(48);
        car_rec42(0);
    }
    link();
    lda8((uint8_t)(rd8(cyc_then(30, F(0x2025, cpu.x))) & 0x01));
    if ((uint8_t)cpu.a) {
        and8(F(0x2025, cpu.x), 0xFE);
        op_lda8(cyc_then(30, cam(0x0034)));
        v = (uint8_t)(cpu.a + 1);
        lda8(v);
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 2)) & ~P_C) | (v >= 2 ? P_C : 0));
        if (!((uint8_t)(v - 2) & 0x80)) {
            lda8(0x00);
            wr8(cam(0x0034), cyc_then(16, 0x00));
            cyc(62);
            lda8(0x2F);
            abi_SoundPort0QueuePush();
            st16i(F(0x200E, cpu.x), 0xFC18);
            pre = 74;
        } else {
            wr8(cam(0x0034), cyc_then(44, v));
            cyc(56);
            lda8(0x24);
            abi_SoundPort0QueuePush();
            st16i(F(0x200E, cpu.x), 0xF830);
            pre = 96;
        }
    }
    cpu.p &= (uint8_t)~P_M;
    op_lda16(cyc_then(pre, F(0x200E, cpu.x)));
    t = half_way(cpu.a, cam(0x000A));
    wr16(cam(0x000A), cyc_then(30, t));
    op_lda16(cyc_then(30, F(0x200C, cpu.x)));
    wr16(db() | 0x1F4C, cyc_then(24, cpu.a));
    op_lda16(cyc_then(30, F(0x2010, cpu.x)));
    wr16(db() | 0x1F50, cyc_then(24, cpu.a));
    sx = cpu.x;
    (cyc(30), link());
    op_lda16(cyc_then(30, cam(0x0002)));
    cpu.x = cpu.y;
    cpu.p = nz16(cpu.p, cpu.x);
    cpu.y = cpu.a;
    cpu.p = nz16(cpu.p, cpu.y);
    op_lda16(cyc_then(58, F(0x200C, cpu.y)));
    cpu.p |= P_C;
    cpu.a = op_sbc16(cpu.a, rd16(cyc_then(38, db() | 0x1F4C)), &cpu.p);
    (cyc(56), abi_ScaleThreeQuarters());       /* JSL, less the SBC's 6 */
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, rd16(cyc_then(82, db() | 0x1F4C)), &cpu.p);
    cyc(22);
    wr16((uint16_t)(cpu.d + 0x08 + cpu.x), cpu.a);
    op_lda16(cyc_then(30, F(0x2010, cpu.y)));
    cpu.p |= P_C;
    cpu.a = op_sbc16(cpu.a, rd16(cyc_then(38, db() | 0x1F50)), &cpu.p);
    (cyc(56), abi_ScaleThreeQuarters());       /* JSL, less the SBC's 6 */
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, rd16(cyc_then(82, db() | 0x1F50)), &cpu.p);
    cyc(22);
    wr16((uint16_t)(cpu.d + 0x0C + cpu.x), cpu.a);
    cpu.p |= P_M;
    cpu.x = sx;
    cpu.p = nz16(cpu.p, sx);
    (cyc(82), tail());
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
    op_lda16(cyc_then(52, cam(0x0002)));
    cpu.y = cpu.a;
    cpu.p = nz16(cpu.p, cpu.y);
    op_lda16(cyc_then(44, F(0x200C, cpu.y)));
    wr16(db() | 0x1F4C, cyc_then(24, cpu.a));
    wr16(F(0x200C, cpu.x), cyc_then(30, cpu.a));
    op_lda16_imm(0xFC18);
    wr16(db() | 0x1F4E, cyc_then(10, cpu.a));
    wr16(F(0x200E, cpu.x), cyc_then(30, cpu.a));
    op_lda16(cyc_then(30, F(0x2010, cpu.y)));
    op_add16(0xFED4);
    wr16(db() | 0x1F50, cyc_then(62, cpu.a));
    wr16(F(0x2010, cpu.x), cyc_then(30, cpu.a));
    cpu.p |= P_M;
    (cyc(22), link());
    copy16(cam(0x0008), db() | 0x1F4C);
    cyc(22);
    copy16(cam(0x000A), db() | 0x1F4E);
    cyc(22);
    copy16(cam(0x000C), db() | 0x1F50);
    cyc(22);
    mode(0x0003);
    (cyc(84), abi_CameraFollowLink());
    handler(cpu.x, 0x8FFE);
    st8i(F(0x2048, cpu.x), cyc_then(6, 0x0A));
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
    (cyc(62), abi_CameraPointFromTable());
    (cyc(24), tail());
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
    cyc(22);
    mode(0x0755);
    cyc(22);
    handler(cpu.x, 0x9176);
    abi_CameraTableStep();
}

/* 08:91D0: the path shot's frame: CameraApproachTarget; arrived (carry),
   $70:273C bit 0 set and back to the table; else the tail. */
static void path_frame(void)
{
    abi_CameraApproachTarget();
    if (!(cpu.p & P_C)) {
        cyc(46);
        tail();
        return;
    }
    cpu.p &= (uint8_t)~P_M;
    cyc(76);
    op_lda16(0x70273C);
    cpu.a |= 0x0001;
    cpu.p = nz16(cpu.p, cpu.a);
    cyc(56);
    wr16(0x70273C, cpu.a);
    cpu.p |= P_M;
    (cyc(14), table_start());
}

/* A path shot: frame $08:91D0, mode 1, the path at Y (ObjectPathStart),
   CameraFollowLink, then the frame. */
static void path_shot(uint16_t path)
{
    cpu.y = path;
    cpu.p = nz16(cpu.p, cpu.y);
    (cyc(100), abi_ObjectPathStart());
    (cyc(62), abi_CameraFollowLink());
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
    cyc(32);
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
    op_lda8(cyc_then(70, db() | 0x0D2B));
    h = (uint8_t)cpu.a;
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(h - 3)) & ~P_C) | (h >= 3 ? (cyc(18), P_C) : 0));
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
    (cyc(22), link());
    for (i = 0; i < 3; i++) {
        op_lda16(cyc_then(30, F(v[i], cpu.x)));
        op_add16(rd16(cyc_then(44, cam(c[i]))));
        wr16(cam(c[i]), cyc_then(30, cpu.a));
    }
    cpu.p |= P_M;
    for (i = 0; i < 3; i++) {
        cpu.p &= (uint8_t)~P_M;
        op_lda16(cyc_then(74, F(v[i], cpu.x)));
        cyc(16);
        wr16(0x00003E, cpu.a);
        op_lda16_imm(t[i]);
        abi_ApproachEighthMin8();
        wr16(F(v[i], cpu.x), cyc_then(30, cpu.a));
        cpu.p |= P_M;
    }
    op_lda8(cyc_then(52, F(0x2048, cpu.x)));
    if (!(uint8_t)cpu.a) {
        cyc(48);
        done();
        return;
    }
    {
        uint8_t n = (uint8_t)(rd8(cyc_then(52, F(0x2048, cpu.x))) - 1);

        wr8(F(0x2048, cpu.x), cyc_then(6, n));
        cpu.p = nz8(cpu.p, n);
    }
    (cyc(24), tail());
}

/* A drift shot: frame f, mode 3, +$48 = n, the director's velocity an
   eighth of its car's record velocity (Y cleared with `flat`). */
static void drift_shot(uint16_t f, uint8_t n, int flat)
{
    handler(cpu.x, f);
    mode(0x0003);
    st8i(F(0x2048, cpu.x), cyc_then(28, n));
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
    (cyc(22), abi_CameraDriftStep());
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
    (cyc(22), abi_CameraDriftLongStep());
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
    (cyc(22), abi_CameraZoomInStep());
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
    (cyc(52), car_y(0));
    cpu.p &= (uint8_t)~P_M;
    op_lda16(cyc_then(52, F(0x2022, cpu.y)));
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
    cyc(46);
    op_lda16((0x700058 + cpu.x) & 0xFFFFFF);
    cpu.x = sx;
    cpu.p = nz16(cpu.p, sx);
    cyc(36);
    while (n--)
        { cyc(62); abi_GsuTableEntryNext(); }
    (cyc(62), abi_GsuTableEntryTo1F4C());
    link();
    op_lda16(cyc_then(24, db() | 0x1F4C));
    if (dx)
        { cyc(38); op_add16(dx); }
    wr16(cam(0x0008), cyc_then(30, cpu.a));
    op_lda16(cyc_then(24, db() | 0x1F4E));
    op_add16(0xFF9C);
    wr16(cam(0x000A), cyc_then(68, cpu.a));
    op_lda16(cyc_then(24, db() | 0x1F50));
    if (dz)
        { cyc(38); op_add16(dz); }
    wr16(cam(0x000C), cyc_then(30, cpu.a));
    cpu.p |= P_M;
    (cyc(46), tail());
}

/*
 * @routine 08:92DC CameraHoldStep
 * WHAT IT DOES: A held shot's frame: the director's tail only.
 * HOW: JMP to 08:8EF9.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void abi_CameraHoldStep(void)
{
    (cyc(24), tail());
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
    (cyc(24), tail());
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
    op_lda16(cyc_then(52, cam(0x000E)));
    cyc(16);
    wr16(0x00003E, cpu.a);
    op_lda16_imm(to);
    (cyc(48), fn());
    wr16(cam(0x000E), cyc_then(30, cpu.a));
    cpu.p |= P_M;
    (cyc(46), tail());
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
    (cyc(22), abi_CameraZoomOutStep());
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
        cyc(22);
        abi_CameraZoomOut();
        return;
    } else { cyc(40); }
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
    (cyc(108), abi_ObjectPathStart());
    sx = cpu.x;
    cpu.p &= (uint8_t)~P_M;
    op_lda16(cyc_then(58, F(0x2006, cpu.x)));
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
    op_lda16(cyc_then(44, F(0x2022, cpu.x)));
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
    at = (0x700040 + cpu.x) & 0xFFFFFF;
    cyc(46);
    op_lda16(at);
    cpu.a |= 0x0004;
    cpu.p = nz16(cpu.p, cpu.a);
    cyc(56);
    wr16(at, cpu.a);
    cpu.p |= P_M;
    cpu.x = sx;
    cpu.p = nz16(cpu.p, sx);
    (cyc(58), abi_CameraOrbitStep());
}

/* LDY +$06, PHX, TYX, REP #$20, the car's record (through X = +$22): the
   word at `field` = v (read first with `read`), SEP #$20, PLX. */
static void car_rec_set(uint32_t field, uint16_t v, int read)
{
    uint16_t sx;
    uint32_t at;

    car_y(0);
    sx = cpu.x;
    cpu.x = cpu.y;
    cpu.p = nz16(cpu.p, cpu.x);
    cpu.p &= (uint8_t)~P_M;
    op_lda16(cyc_then(96, F(0x2022, cpu.x)));
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
    at = (field + cpu.x) & 0xFFFFFF;
    if (read)
        { cyc(46); op_lda16(at); } else { cyc(14); }
    op_lda16_imm(v);
    cyc(18);
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
    (cyc(82), tail());
}

/*
 * @routine 08:958C CameraEndStep
 * WHAT IT DOES: The end shot's frame: the director's tail only.
 * HOW: JMP to 08:8EF9. Reached only by unit_replay_camera_08.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void abi_CameraEndStep(void)
{
    (cyc(24), tail());
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
    op_lda8(cyc_then(88, F(0x2048, cpu.x)));
    if (!(uint8_t)cpu.a) {
        cyc(48);
        abi_CameraEnd();
        return;
    }
    {
        uint8_t n = (uint8_t)(rd8(cyc_then(52, F(0x2048, cpu.x))) - 1);

        wr8(F(0x2048, cpu.x), cyc_then(6, n));
        cpu.p = nz8(cpu.p, n);
    }
    (cyc(62), abi_CameraPointFromTable());
    (cyc(24), tail());
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
    st8i(F(0x2048, cpu.x), cyc_then(6, 0x0A));
    link();
    st16i(cam(0x0037), 0x8000);
    (cyc(22), abi_CameraStillStep());
}

/* ---- 08:95A0-9A4E: the camera following a car ---- */

void abi_GsuRecordNewCamera(void);
void abi_ObjectSyncToRecord(void);
void abi_ObjectSyncAngles(void);
void abi_RecordContactSummary(void);
void abi_CameraFollowStep(void);
void abi_ObjectFaceTied(void);
void abi_ObjectGsu81E3(void);
void abi_ObjectGsu80BCThen81E3(void);

/* Direct page word at off (bank 0), no time. */
static uint32_t dpa(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/* Direct page word at off (bank 0). */
static uint32_t dpw(uint8_t off)
{
    cyc(14);
    return (uint16_t)(cpu.d + off);
}

/* PHX, PHB, CameraFollowStep, SEP #$20, PLB, PLX. */
static void follow_step(void)
{
    uint16_t sx = cpu.x;
    uint8_t sb = cpu.db;

    (cyc(114), abi_CameraFollowStep());
    cpu.p |= P_M;
    cpu.db = sb;
    cpu.p = nz8(cpu.p, sb);
    cpu.x = sx;
    cpu.p = nz16(cpu.p, sx);
}

/* LDA #v, STA $05FA (the camera's view kind). */
static void view(uint8_t v)
{
    st8i(db() | 0x05FA, v);
}

/* A word divided by 2 or 4 keeping its sign (CMP #$8000, ROR, n times). */
static void asr_n(int n)
{
    while (n--) {
        uint8_t c;

        op_cmp16(0x8000);
        c = (uint8_t)(cpu.a & 1);
        cpu.a = (uint16_t)((cpu.a >> 1) | ((cpu.p & P_C) ? 0x8000 : 0));
        cpu.p = (uint8_t)(nz16((uint8_t)(cpu.p & ~P_C), cpu.a) | c);
    }
}

/* 08:95C7: the follower's frame (see CameraFollowCarStep). */
static void follower_frame(void)
{
    uint32_t r;
    static const uint8_t dpo[3] = {0xC5, 0xC7, 0xC9};
    static const uint16_t keep[3] = {0x1F64, 0x1F66, 0x1F68};
    static const uint16_t to[3] = {0x1F4C, 0x1F4E, 0x1F50};
    int i;

    car_y(0);
    wr8(db() | 0x1F41, cyc_then(24, 0x00));
    for (i = 0; i < 3; i++)
        copy16x(db() | keep[i], dpa(dpo[i]), i ? 60 : 38, 24);
    cyc(22);
    follow_step();
    cpu.p &= (uint8_t)~P_M;
    cpu.y = cpu.x;
    cpu.p = nz16(cpu.p, cpu.y);
    op_lda16(cyc_then(152, F(0x2022, cpu.x)));
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
    r = cpu.x;
    cyc(14);
    for (i = 0; i < 3; i++) {
        cyc(32);
        op_lda16((0x70001E + 2 * i + r) & 0xFFFFFF);
        wr16(db() | to[i], cyc_then(24, cpu.a));
    }
    op_lda16(cyc_then(2, dpw(0xB7)));
    asr_n(2);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, rd16(cyc_then(120, F(0x200C, cpu.y))), &cpu.p);
    cyc(32);
    wr16((0x70001E + r) & 0xFFFFFF, cpu.a);
    op_lda16_imm(0x00C8);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, rd16(cyc_then(30, F(0x200E, cpu.y))), &cpu.p);
    cyc(32);
    wr16((0x700020 + r) & 0xFFFFFF, cpu.a);
    op_lda16(cyc_then(2, dpw(0xBB)));
    asr_n(2);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, rd16(cyc_then(120, F(0x2010, cpu.y))), &cpu.p);
    cyc(32);
    wr16((0x700022 + r) & 0xFFFFFF, cpu.a);
    cpu.x = cpu.y;
    cpu.p = nz16(cpu.p, cpu.x);
    cpu.p |= P_M;
    st8i((cyc(36), db()) | 0x1F41, 0xFF);
    (cyc(62), abi_ObjectSyncAngles());
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | (cpu.a & 0x01));
    cpu.p = nz8(cpu.p, (uint8_t)cpu.a);
    cyc(16);                            /* AND #$01 */
    if ((uint8_t)cpu.a) {
        cyc(16);                        /* BEQ not taken */
        (cyc(62), abi_RecordContactSummary());
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | (cpu.a & 0x18));
        cpu.p = nz8(cpu.p, (uint8_t)cpu.a);
        if ((uint8_t)cpu.a) {
            cyc(32); /* AND, BEQ not taken */
            copy16x(dpa(0xC5), db() | 0x1F4C, 46, 16);
            copy16x(dpa(0xC7), db() | 0x1F4E, 68, 16);
            cpu.p &= (uint8_t)~P_M;
            op_lda16(cyc_then(46, dpw(0xC7)));
            op_add16(0xFF38);
            wr16(dpw(0xC7), cyc_then(40, cpu.a));
            cpu.p |= P_M;
            copy16x(dpa(0xC9), db() | 0x1F50, 68, 16);
            cyc(22);
        } else { cyc(16); } /* AND, BEQ taken */
        } else {
        for (i = 0; i < 3; i++)
            { cyc(8); copy16x(dpa(dpo[i]), db() | keep[i], 60, 16); }
    }
    cyc(84);
    abi_ObjectFaceTied();
    st8i(F(0x2036, cpu.x), cyc_then(6, 0x03));
    cyc(44); /* RTL */
}

/*
 * @routine 08:95A0 CameraFollowCar
 * WHAT IT DOES: The follow shot: view $05FA = 5, frame $08:95C7
 * (CameraFollowCarStep), a record of its own (GsuRecordNewCamera), one
 * CameraFollowStep, then synced to its record.
 * HOW: PHX/PHB around CameraFollowStep. Reached only by
 * unit_replay_camera_08.
 * Entry: M 8-bit, X 16-bit; X = the director; D = $0000. Exit: as
 * ObjectSyncToRecord leaves it.
 */
void abi_CameraFollowCar(void)
{
    view(0x05);
    handler(cpu.x, 0x95C7);
    (cyc(62), abi_GsuRecordNewCamera());
    follow_step();
    (cyc(148), abi_ObjectSyncToRecord());
    cyc(44); /* RTL */
}

/*
 * @routine 08:95C7 CameraFollowCarStep
 * WHAT IT DOES: The follower's frame: the camera point (direct page
 * $C5-$C9) kept at $1F64-$1F68, CameraFollowStep; its record placed a
 * quarter of $B7/$BB from the car (+$06) and $C8 up (the old place kept
 * at $1F4C-$1F50), synced (ObjectSyncAngles, $1F41 = $FF); touching
 * something (contact flags 3-4) puts the camera point at the old place
 * $C8 higher, no contacts puts it back; ObjectFaceTied, +$36 = 3.
 * HOW: 16-bit long record accesses; CMP #$8000/ROR twice for the
 * quarters. Reached only by unit_replay_camera_08.
 * Entry: M 8-bit, X 16-bit; X = the director; D = $0000. Exit: A low byte
 * = 3.
 */
void abi_CameraFollowCarStep(void)
{
    follower_frame();
}

void abi_CameraReturnStep(void);

/*
 * @routine 08:9833 CameraReturn
 * WHAT IT DOES: The director heads back to its table: frame $08:9847
 * (CameraReturnStep), +$48 = $0F; then that frame.
 * HOW: Word and byte stores; entered by JMLs from the shots below.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraReturn(void)
{
    handler(cpu.x, 0x9847);
    st8i(F(0x2048, cpu.x), cyc_then(6, 0x0F));
    abi_CameraReturnStep();
}

/*
 * @routine 08:9847 CameraReturnStep
 * WHAT IT DOES: The return's frame: view $05FA = 9; +$48 counts down with
 * the director's table shot (CameraDirectorTable) each frame; at 0 the
 * car's +$48 = 0 and the director goes back to its table (08:8EC1).
 * HOW: DEC, JML CameraDirectorTable; JML to 08:8EC1.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void abi_CameraReturnStep(void)
{
    uint8_t n;

    view(0x09);
    n = (uint8_t)(rd8(cyc_then(30, F(0x2048, cpu.x))) - 1);
    wr8(F(0x2048, cpu.x), cyc_then(6, n));
    cpu.p = nz8(cpu.p, n);
    if (n) {
        cyc(48);
        abi_CameraDirectorTable();
        return;
    } else { cyc(22); }
    car_y(0);
    st8i(F(0x2048, cpu.y), cyc_then(6, 0x00));
    table_start();
}


/* (a - b)/2 + b with the sign kept (no rounding). */
static uint16_t mid(uint16_t a, uint32_t at)
{
    cpu.p |= P_C;
    cyc(38);
    cpu.a = op_sbc16(a, rd16(at), &cpu.p);
    asr_n(1);
    cpu.p &= (uint8_t)~P_C;
    cyc(70);
    cpu.a = op_adc16(cpu.a, rd16(at), &cpu.p);
    return cpu.a;
}

/* 08:9825: the track shot's frame: a cue (+$25 bit 0) sends the director
   back (CameraReturn), else the tail. */
static void track_half_frame(void)
{
    lda8((uint8_t)(rd8(cyc_then(30, F(0x2025, cpu.x))) & 0x01));
    if ((uint8_t)cpu.a) {
        cyc(26);
        abi_CameraReturn();
        return;
    }
    (cyc(24), tail());
}

/*
 * @routine 08:9797 CameraTrackHalf
 * WHAT IT DOES: The track shot: view 2, frame $08:9825 (CameraTrackHalfStep);
 * the director half way between the car's track table entry (+$29) and
 * the next one ($1F4 higher), turned by the Super FX (ObjectGsu81E3,
 * ObjectGsu80BCThen81E3, the result negated); then that frame.
 * HOW: GsuTableEntryNext, GsuTableEntryTo1F4C through direct page
 * $04/$0A and $05F2; CMP #$8000/ROR for the halves.
 * Entry: M 8-bit, X 16-bit; X = the director; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_CameraTrackHalf(void)
{
    view(0x02);
    handler(cpu.x, 0x9825);
    cpu.p &= (uint8_t)~P_M;
    op_lda16(cyc_then(52, F(0x2029, cpu.x)));
    (cyc(62), abi_GsuTableEntryNext());
    (cyc(62), abi_GsuTableEntryTo1F4C());
    copy16x(dpa(0x04), db() | 0x1F4C, 24, 16);
    cpu.p &= (uint8_t)~P_M;
    op_lda16(cyc_then(24, db() | 0x1F4E));
    wr16(dpw(0x0A), cyc_then(2, cpu.a));
    op_lda16(cyc_then(24, db() | 0x1F50));
    wr16(db() | 0x05F2, cyc_then(24, cpu.a));
    op_lda16(cyc_then(30, F(0x2029, cpu.x)));
    (cyc(62), abi_GsuTableEntryTo1F4C());
    op_lda16(cyc_then(24, db() | 0x1F4E));
    op_add16(0xFE0C);
    wr16(db() | 0x1F4E, cyc_then(62, cpu.a));
    op_lda16(cyc_then(2, dpw(0x04)));
    wr16(F(0x200C, cpu.x), cyc_then(30, mid(cpu.a, db() | 0x1F4C)));
    op_lda16(cyc_then(2, dpw(0x0A)));
    op_add16(0xFE0C);
    wr16(F(0x200E, cpu.x), cyc_then(30, mid(cpu.a, (cyc(38), db()) | 0x1F4E)));
    op_lda16(cyc_then(24, db() | 0x05F2));
    wr16(F(0x2010, cpu.x), cyc_then(30, mid(cpu.a, db() | 0x1F50)));
    (cyc(62), abi_ObjectGsu81E3());
    (cyc(62), abi_ObjectGsu80BCThen81E3());
    cpu.a ^= 0xFFFF;
    cpu.p = nz16(cpu.p, cpu.a);
    cpu.a++;
    cpu.p = nz16(cpu.p, cpu.a);
    cpu.p |= P_M;
    (cyc(38), car_y(0));
    cpu.p = nz16(cpu.p, cpu.x);
    (cyc(80), track_half_frame());
}

/*
 * @routine 08:9825 CameraTrackHalfStep
 * WHAT IT DOES: The track shot's frame: a cue (+$25 bit 0) sends the
 * director back (CameraReturn), else the director's tail.
 * HOW: AND #1; JML CameraReturn.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void abi_CameraTrackHalfStep(void)
{
    track_half_frame();
}

/*
 * @routine 08:9787 CameraBesideWait
 * WHAT IT DOES: The side shot's frame: while +$48 counts down the
 * director's tail, then the track shot (CameraTrackHalf).
 * HOW: DEC; JMLs.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void abi_CameraBesideWait(void)
{
    op_lda8(cyc_then(30, F(0x2048, cpu.x)));
    if (!(uint8_t)cpu.a) {
        cyc(48);
        abi_CameraTrackHalf();
        return;
    }
    {
        uint8_t n = (uint8_t)(rd8(cyc_then(52, F(0x2048, cpu.x))) - 1);

        wr8(F(0x2048, cpu.x), cyc_then(6, n));
        cpu.p = nz8(cpu.p, n);
    }
    (cyc(32), tail());
}

/*
 * @routine 08:9711 CameraBesideStep
 * WHAT IT DOES: The side shot's first frame: the director at the car's
 * (+$06) X and Z plus $258 each, height -$C8; frame $08:9787
 * (CameraBesideWait); the car's record velocity and +$34-$38 cleared;
 * +$48 = $3C; then that frame.
 * HOW: 16-bit copies and ADCs; the record words through X (PHX/PLX).
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraBesideStep(void)
{
    uint16_t sx;
    uint32_t r;
    static const uint8_t f[6] = {0x2E, 0x30, 0x32, 0x36, 0x34, 0x38};
    int i;

    car_y(0);
    copy16(F(0x200C, cpu.x), cyc_then(6, F(0x200C, cpu.y)));
    copy16(F(0x2010, cpu.x), cyc_then(28, F(0x2010, cpu.y)));
    st16i(F(0x200E, cpu.x), cyc_then(22, 0xFF38));
    {
        cpu.p &= (uint8_t)~P_M;
        op_lda16(cyc_then(74, F(0x200C, cpu.x)));
        op_add16(0x0258);
        wr16(F(0x200C, cpu.x), cyc_then(68, cpu.a));
        cpu.p |= P_M;
        cpu.p &= (uint8_t)~P_M;
        op_lda16(cyc_then(74, F(0x2010, cpu.x)));
        op_add16(0x0258);
        wr16(F(0x2010, cpu.x), cyc_then(68, cpu.a));
        cpu.p |= P_M;
    }
    cyc(22);
    handler(cpu.x, 0x9787);
    sx = cpu.x;
    cpu.p &= (uint8_t)~P_M;
    op_lda16(cyc_then(82, F(0x2022, cpu.y)));
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
    r = cpu.x;
    op_lda16_imm(0x0000);
    for (i = 0; i < 6; i++)
        { cyc(32); wr16((0x700000 + f[i] + r) & 0xFFFFFF, 0x0000); }
    cyc(64);
    cpu.p |= P_M;
    cpu.x = sx;
    cpu.p = nz16(cpu.p, sx);
    st8i(F(0x2048, cpu.x), 0x3C);
    abi_CameraBesideWait();
}

/*
 * @routine 08:9702 CameraBeside
 * WHAT IT DOES: The side shot: frame $08:9711 (CameraBesideStep); then
 * that frame.
 * HOW: Word and byte stores; entered by a JML from CameraLiftOffStep.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraBeside(void)
{
    handler(cpu.x, 0x9711);
    abi_CameraBesideStep();
}

/* 08:96D1: the lift-off's frame (see CameraLiftOffStep). */
static void lift_frame(void)
{
    op_lda8(cyc_then(30, F(0x2048, cpu.x)));
    if (!(uint8_t)cpu.a) {
        cyc(48);
        abi_CameraReturn();
        return;
    }
    {
        uint8_t n = (uint8_t)(rd8(cyc_then(52, F(0x2048, cpu.x))) - 1);

        wr8(F(0x2048, cpu.x), cyc_then(6, n));
        cpu.p = nz8(cpu.p, n);
    }
    cpu.p &= (uint8_t)~P_M;
    op_lda16(cyc_then(52, F(0x200E, cpu.x)));
    op_add16(0xFFCE);
    wr16(F(0x200E, cpu.x), cyc_then(68, cpu.a));
    cpu.p |= P_M;
    (cyc(22), car_y(0));
    cpu.p &= (uint8_t)~P_M;
    op_lda16(cyc_then(52, F(0x200E, cpu.y)));
    op_cmp16(0xFC18);
    cpu.p |= P_M;
    if (cpu.p & P_N) {
        cyc(100);
        tail();
        return;
    } else { cyc(94); }
    abi_CameraBeside();
}

/*
 * @routine 08:96D1 CameraLiftOffStep
 * WHAT IT DOES: The lift-off's frame: while +$48 counts down the director
 * rises $32; once the car (+$06) is no higher than -$3E8 the side shot
 * (CameraBeside), else the tail; at 0 back (CameraReturn).
 * HOW: 16-bit ADC and CMP; JMLs.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void abi_CameraLiftOffStep(void)
{
    lift_frame();
}

/*
 * @routine 08:96A3 CameraLiftOff
 * WHAT IT DOES: The lift-off: view 3, the car's track entry +$29 taken,
 * frame $08:96D1 (CameraLiftOffStep), +$48 = $96, +$25 bit 0 cleared; then
 * that frame.
 * HOW: 16-bit copy; word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraLiftOff(void)
{
    view(0x03);
    car_y(0);
    copy16(F(0x2029, cpu.x), cyc_then(6, F(0x2029, cpu.y)));
    cyc(22);
    handler(cpu.x, 0x96D1);
    st8i(F(0x2048, cpu.x), cyc_then(6, 0x96));
    and8(F(0x2025, cpu.x), 0xFE);
    lift_frame();
}

/*
 * @routine 08:9888 CameraPathFollowStep
 * WHAT IT DOES: The followed path's frame: +$36 = 3, CameraApproachTarget,
 * CameraFollowLink.
 * HOW: Two JSLs.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraFollowLink
 * leaves it.
 */
void abi_CameraPathFollowStep(void)
{
    st8i(F(0x2036, cpu.x), cyc_then(6, 0x03));
    abi_CameraApproachTarget();
    (cyc(62), abi_CameraFollowLink());
    cyc(44); /* RTL */
}

/*
 * @routine 08:9861 CameraPathFollow
 * WHAT IT DOES: The director on path $85C3 (ObjectPathStart), frame
 * $08:9888 (CameraPathFollowStep), mode 1, CameraFollowLink; then that
 * frame.
 * HOW: JSLs and word stores.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraPathFollow(void)
{
    cpu.y = 0x85C3;
    cpu.p = nz16(cpu.p, cpu.y);
    (cyc(86), abi_ObjectPathStart());
    handler(cpu.x, 0x9888);
    mode(0x0001);
    (cyc(84), abi_CameraFollowLink());
    abi_CameraPathFollowStep();
}

/*
 * @routine 08:998B CameraPathGoStep
 * WHAT IT DOES: A path shot's frame: CameraApproachTarget, then the
 * director's tail.
 * HOW: JSL, JMP to 08:8EF9.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void abi_CameraPathGoStep(void)
{
    abi_CameraApproachTarget();
    (cyc(24), tail());
}

/*
 * @routine 08:997C CameraPathGo
 * WHAT IT DOES: The path shots' shared end: frame $08:998B
 * (CameraPathGoStep); then that frame.
 * HOW: Word and byte stores; entered by JMPs.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraPathGo(void)
{
    handler(cpu.x, 0x998B);
    abi_CameraPathGoStep();
}

/* A path shot: the director on path p, mode 1 (unless `free`),
   CameraFollowLink, then CameraPathGo. */
static void path_go(uint16_t p, int set_mode)
{
    cyc(86);
    cpu.y = p;
    cpu.p = nz16(cpu.p, cpu.y);
    abi_ObjectPathStart();
    if (set_mode)
        { cyc(0); mode(0x0001); cyc(84); } else { cyc(62); }
    abi_CameraFollowLink();
    (cyc(24), abi_CameraPathGo());
}

/*
 * @routine 08:9896 CameraPath8603
 * WHAT IT DOES: A path shot: the director on path $8603 (ObjectPathStart), mode 1,
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void abi_CameraPath8603(void)
{
    path_go(0x8603, 1);
}

/*
 * @routine 08:98B1 CameraPath8643
 * WHAT IT DOES: A path shot: the director on path $8643 (ObjectPathStart), mode 1,
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void abi_CameraPath8643(void)
{
    path_go(0x8643, 1);
}

/*
 * @routine 08:98CC CameraPath8679
 * WHAT IT DOES: A path shot: the director on path $8679 (ObjectPathStart), mode 1,
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void abi_CameraPath8679(void)
{
    path_go(0x8679, 1);
}

/*
 * @routine 08:98E7 CameraPath869B
 * WHAT IT DOES: A path shot: the director on path $869B (ObjectPathStart), mode 1,
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void abi_CameraPath869B(void)
{
    path_go(0x869B, 1);
}

/*
 * @routine 08:9902 CameraPath86B3
 * WHAT IT DOES: A path shot: the director on path $86B3 (ObjectPathStart), mode 1,
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void abi_CameraPath86B3(void)
{
    path_go(0x86B3, 1);
}

/*
 * @routine 08:991D CameraPath86CB
 * WHAT IT DOES: A path shot: the director on path $86CB (ObjectPathStart), mode 1,
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void abi_CameraPath86CB(void)
{
    path_go(0x86CB, 1);
}

/*
 * @routine 08:9938 CameraPath8709
 * WHAT IT DOES: A path shot: the director on path $8709 (ObjectPathStart),
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void abi_CameraPath8709(void)
{
    path_go(0x8709, 0);
}

/*
 * @routine 08:9946 CameraPath86E3
 * WHAT IT DOES: A path shot: the director on path $86E3 (ObjectPathStart), mode 1,
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void abi_CameraPath86E3(void)
{
    path_go(0x86E3, 1);
}

/*
 * @routine 08:9961 CameraPath86FB
 * WHAT IT DOES: A path shot: the director on path $86FB (ObjectPathStart), mode 1,
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void abi_CameraPath86FB(void)
{
    path_go(0x86FB, 1);
}

/* 08:9A25: the low shot's frame: the camera's +$1A-$1E = (0, -$64, 0);
   the tail. */
static void low_frame(void)
{
    link();
    st16i(cam(0x001A), 0x0000);
    (cyc(22), link());
    st16i(cam(0x001C), 0xFF9C);
    (cyc(22), link());
    st16i(cam(0x001E), 0x0000);
}

/*
 * @routine 08:9A25 CameraLowHoldStep
 * WHAT IT DOES: The low shot's frame: the camera's offset +$1A-$1E = (0,
 * -$64, 0), then the director's tail.
 * HOW: Word stores; JMP to 08:8EF9.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void abi_CameraLowHoldStep(void)
{
    low_frame();
    (cyc(46), tail());
}

/*
 * @routine 08:9992 CameraLowBehind
 * WHAT IT DOES: The low shot from behind: mode $C080, frame $08:9A25
 * (CameraLowHoldStep), the camera's offset (0, -$64, 0), CameraFollowLink;
 * then that frame.
 * HOW: Word stores; a JMP into the frame.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraLowBehind(void)
{
    mode(0xC080);
    cyc(22);
    handler(cpu.x, 0x9A25);
    low_frame();
    (cyc(84), abi_CameraFollowLink());
    (cyc(24), abi_CameraLowHoldStep());
}

/*
 * @routine 08:9A16 CameraLowHold
 * WHAT IT DOES: The low shot held: frame $08:9A25 (CameraLowHoldStep);
 * then that frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraLowHold(void)
{
    handler(cpu.x, 0x9A25);
    abi_CameraLowHoldStep();
}

/*
 * @routine 08:9A0D CameraPathLowStep
 * WHAT IT DOES: The low path shot's frame: CameraApproachTarget; once there
 * the low hold (CameraLowHold), else the tail.
 * HOW: Carry from CameraApproachTarget; BRL to 08:8EF9.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void abi_CameraPathLowStep(void)
{
    abi_CameraApproachTarget();
    if (cpu.p & P_C) {
        cyc(22);
        abi_CameraLowHold();
        return;
    } else { cyc(46); }
    tail();
}

/* The low path shots: the director on path p, mode 1, CameraFollowLink,
   frame $08:9A0D (CameraPathLowStep), then that frame. */
static void path_low(uint16_t p, int jmp)
{
    cpu.y = p;
    cpu.p = nz16(cpu.p, cpu.y);
    (cyc(86), abi_ObjectPathStart());
    if (jmp)
        cyc(24); /* JMP into the shared part */
    mode(0x0001);
    (cyc(84), abi_CameraFollowLink());
    handler(cpu.x, 0x9A0D);
    abi_CameraPathLowStep();
}

/*
 * @routine 08:99DC CameraPathLow87B7
 * WHAT IT DOES: A low path shot on path $87B7: ObjectPathStart, mode 1,
 * CameraFollowLink, frame $08:9A0D (CameraPathLowStep); then that frame.
 * HOW: JSLs; a JMP into the shared part at 08:99ED.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraPathLow87B7(void)
{
    path_low(0x87B7, 1);
}

/*
 * @routine 08:99E6 CameraPathLow875B
 * WHAT IT DOES: As CameraPathLow87B7 on path $875B.
 * HOW: As CameraPathLow87B7.
 * Entry: as CameraPathLow87B7. Exit: as CameraPathLow87B7.
 */
void abi_CameraPathLow875B(void)
{
    path_low(0x875B, 0);
}
