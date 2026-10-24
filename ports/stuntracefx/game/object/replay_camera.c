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
#include "math_calls.h"
#include "cpu_ops.h"
#include "carried.h"
#include "wram_names.h"

int ObjectPathStart(uint8_t bank, uint16_t obj, uint16_t path, uint16_t *a, uint16_t *y, uint8_t *p);

void abi_CameraFollowLink(void);
void abi_CameraPointFromTable(void);
void abi_CameraDirectorTable(void);
void abi_CameraDirectorSkip2(void);
void abi_CameraApproachTarget(void);
void abi_ObjectPathStart(void);
void abi_ObjectVelocityFromRecordOf(void);
void abi_ScaleThreeQuarters(void);
void abi_GsuTableEntryNext(void);
void abi_GsuTableEntryTo1F4C(void);
void abi_SoundPort0QueuePush(void);

void CameraBeside(Carried *cr);
void CameraBesideStep(Carried *cr);
void CameraBesideWait(Carried *cr);
void CameraDrift(Carried *cr);
void CameraDriftLong(Carried *cr);
void CameraDriftLongStep(Carried *cr);
void CameraDriftStep(Carried *cr);
void CameraEnd(Carried *cr);
void CameraEndStep(Carried *cr);
void CameraFixedAhead(Carried *cr);
void CameraFixedSide(Carried *cr);
void CameraFollowCar(Carried *cr);
void CameraFollowCarStep(Carried *cr);
void CameraHoldStep(Carried *cr);
void CameraHoldStep2(Carried *cr);
void CameraLiftOff(Carried *cr);
void CameraLiftOffStep(Carried *cr);
void CameraLowBehind(Carried *cr);
void CameraLowHold(Carried *cr);
void CameraLowHoldStep(Carried *cr);
void CameraOrbit(Carried *cr);
void CameraOrbitStep(Carried *cr);
void CameraOverhead(Carried *cr);
void CameraOverheadStep(Carried *cr);
void CameraPath8603(Carried *cr);
void CameraPath8643(Carried *cr);
void CameraPath8679(Carried *cr);
void CameraPath869B(Carried *cr);
void CameraPath86B3(Carried *cr);
void CameraPath86CB(Carried *cr);
void CameraPath86E3(Carried *cr);
void CameraPath86FB(Carried *cr);
void CameraPath8709(Carried *cr);
void CameraPathA(Carried *cr);
void CameraPathB(Carried *cr);
void CameraPathFollow(Carried *cr);
void CameraPathFollowStep(Carried *cr);
void CameraPathGo(Carried *cr);
void CameraPathGoStep(Carried *cr);
void CameraPathLow875B(Carried *cr);
void CameraPathLow87B7(Carried *cr);
void CameraPathLowStep(Carried *cr);
void CameraPathStep(Carried *cr);
void CameraReturn(Carried *cr);
void CameraReturnStep(Carried *cr);
void CameraRise(Carried *cr);
void CameraRiseStep(Carried *cr);
void CameraSnapToCar(Carried *cr);
void CameraSnapToTable(Carried *cr);
void CameraStill(Carried *cr);
void CameraStillStep(Carried *cr);
void CameraTableStep(Carried *cr);
void CameraTrackHalf(Carried *cr);
void CameraTrackHalfStep(Carried *cr);
void CameraZoomIn(Carried *cr);
void CameraZoomInStep(Carried *cr);
void CameraZoomOut(Carried *cr);
void CameraZoomOutStep(Carried *cr);
static void and8(Carried *cr, uint32_t addr, uint8_t m);
static void asr_n(Carried *cr, int n);
static uint32_t cam(Carried *cr, uint16_t o);
static void car(Carried *cr);
static void car_rec42(Carried *cr, int op);
static void car_rec_set(Carried *cr, uint32_t field, uint16_t v, int read);
static void car_y(Carried *cr, int jsl);
static void copy16(Carried *cr, uint32_t dst, uint32_t src);
static void copy16x(Carried *cr, uint32_t dst, uint32_t src, unsigned pre, unsigned mid);
static uint32_t db(Carried *cr);
static uint32_t dpa(Carried *cr, uint8_t off);
static uint32_t dpw(Carried *cr, uint8_t off);
static void drift_frame(Carried *cr, uint16_t ty, void (*done)(Carried *));
static void drift_shot(Carried *cr, uint16_t f, uint8_t n, int flat);
static void fixed_shot(Carried *cr, uint16_t f, int n, uint16_t dx, uint16_t dz);
static void follow_step(Carried *cr);
static void follower_frame(Carried *cr);
static uint16_t half_way(Carried *cr, uint16_t a, uint32_t at);
static void handler(Carried *cr, uint16_t o, uint16_t h);
static void lda8(Carried *cr, uint8_t v);
static void lift_frame(Carried *cr);
static void link(Carried *cr);
static void low_frame(Carried *cr);
static uint16_t mid(Carried *cr, uint16_t a, uint32_t at);
static void mode(Carried *cr, uint16_t v);
static void overhead_frame(Carried *cr);
static void path_frame(Carried *cr);
static void path_go(Carried *cr, uint16_t p, int set_mode);
static void path_low(Carried *cr, uint16_t p, int jmp);
static void path_shot(Carried *cr, uint16_t path);
static void snap(Carried *cr);
static void st16i(Carried *cr, uint32_t addr, uint16_t v);
static void st8i(Carried *cr, uint32_t addr, uint8_t v);
static void table_start(Carried *cr);
static void tail(Carried *cr);
static void track_half_frame(Carried *cr);
static void view(Carried *cr, uint8_t v);
static void zoom_frame(Carried *cr, uint16_t to, uint16_t (*fn)(uint16_t, int *));
static void w_ObjectPathStart(Carried *cr);
void CameraApproachTarget(Carried *cr);
void CameraDirectorSkip2(Carried *cr);
void CameraDirectorTable(Carried *cr);
void CameraFollowLink(Carried *cr);
void CameraFollowStep_cr(Carried *cr);
void CameraPointFromTable(Carried *cr);
void GsuRecordNewCamera_cr(Carried *cr);
void GsuTableEntryNext_cr(Carried *cr);
void GsuTableEntryTo1F4C_cr(Carried *cr);
void ObjectFaceTied_cr(Carried *cr);
void ObjectGsu80BCThen81E3_cr(Carried *cr);
void ObjectGsu81E3_cr(Carried *cr);
void ObjectSyncAngles_cr(Carried *cr);
void ObjectSyncToRecord_cr(Carried *cr);
void ObjectVelocityFromRecordOf_cr(Carried *cr);
void RecordContactSummary_cr(Carried *cr);
void ScaleThreeQuarters_cr(Carried *cr);
void SoundPort0QueuePush_cr(Carried *cr);

static uint32_t db(Carried *cr)
{
    return (uint32_t)cr->db << 16;
}

/* Object field `f` ($20xx) of object `o`, through the data bank. */
#define F(f, o) (db(cr) | (uint16_t)((f) + (o)))

/* Camera field `o` of the camera in Y, through the data bank. */
static uint32_t cam(Carried *cr, uint16_t o)
{
    return db(cr) | (uint16_t)(cr->y + o);
}

/* LDA #v with M 8-bit. */
static void lda8(Carried *cr, uint8_t v)
{
    cyc(38);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
}

/* LDA #v, STA addr, 8-bit. */
static void st8i(Carried *cr, uint32_t addr, uint8_t v)
{
    lda8(cr, v);
    cyc(2);
    wr8(addr, v);
}

/* REP #$20, LDA #v, STA addr, SEP #$20. */
static void st16i(Carried *cr, uint32_t addr, uint16_t v)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16_imm(cr, v);
    cyc(38);
    wr16(addr, v);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

/* REP #$20, LDA src, STA dst, SEP #$20 (16-bit copy): `pre` clocks to the
   load's read, `mid` from there to the store. */
static void copy16x(Carried *cr, uint32_t dst, uint32_t src, unsigned pre, unsigned mid)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(pre);
    r_lda16(cr, src);
    cyc(mid);
    wr16(dst, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

static void copy16(Carried *cr, uint32_t dst, uint32_t src)
{
    copy16x(cr, dst, src, 46, 30);
}

/* LDA addr, AND #m, STA addr (8-bit). */
static void and8(Carried *cr, uint32_t addr, uint8_t m)
{
    cyc(30);
    uint8_t v = (uint8_t)(rd8(addr) & m);

    lda8(cr, v);
    cyc(8);
    wr8(addr, v);
}

/* The handler of object o: +$1A = h, +$1C = $08. */
static void handler(Carried *cr, uint16_t o, uint16_t h)
{
    st16i(cr, F(W(obj_1a_w), o), h);
    st8i(cr, F(W(obj_1c), o), cyc_then(28, 0x08));
}

/* LDY +$4F: Y = the director's camera. */
static void link(Carried *cr)
{
    cr->y = rd16(cyc_then(30, F(W(obj_link_w), cr->x)));
    set_p(cr, nz16(cr->p, cr->y));
}

/* LDY +$06: Y = the director's car; `jsl` is 62 when a JSL follows it. */
static void car_y(Carried *cr, int jsl)
{
    cr->y = rd16(cyc_then(30, F(W(obj_06), cr->x)));
    set_p(cr, nz16(cr->p, cr->y));
    if (jsl)
        cyc(62);
}

static void car(Carried *cr)
{
    car_y(cr, 1);
}

/* The camera's mode word +$00 = v (LDY +$4F first). */
static void mode(Carried *cr, uint16_t v)
{
    link(cr);
    st16i(cr, cam(cr, 0x0000), v);
}

/* 08:8EF9: the director's tail: CameraFollowLink, +$36 = 3. */
static void tail(Carried *cr)
{
    (cyc(62), CameraFollowLink(cr));
    st8i(cr, F(W(obj_36), cr->x), cyc_then(6, 0x03));
    cyc(44);
}

/* 08:8EC1: back to the director's table: the camera's +$04 and +$34
   cleared, +$25 bits 0 and 1 cleared, frame $08:8EF5
   (CameraDirectorTable), then that frame. */
static void table_start(Carried *cr)
{
    (cyc(32), link(cr));
    st16i(cr, cam(cr, 0x0004), 0x0000);
    (cyc(22), link(cr));
    cyc(6);
    st8i(cr, cam(cr, 0x0034), 0x00);
    and8(cr, F(W(obj_25), cr->x), 0xFE);
    and8(cr, F(W(obj_25), cr->x), 0xFD);
    handler(cr, cr->x, 0x8EF5);
    CameraDirectorTable(cr);
}

/* The camera put back on its car: mode 1, its turn words +$26-$30
   cleared, its +$1A-$1E from +$20-$24, CameraFollowLink. */
static void snap(Carried *cr)
{
    mode(cr, 0x0001);
    (cyc(22), link(cr));
    st16i(cr, cam(cr, 0x0026), 0x0000);
    cyc(22);
    st16i(cr, cam(cr, 0x0028), 0x0000);
    cyc(22);
    st16i(cr, cam(cr, 0x002A), 0x0000);
    (cyc(22), link(cr));
    st16i(cr, cam(cr, 0x002C), 0x0000);
    cyc(22);
    st16i(cr, cam(cr, 0x002E), 0x0000);
    cyc(22);
    st16i(cr, cam(cr, 0x0030), 0x0000);
    (cyc(22), link(cr));
    cyc(6);
    copy16(cr, cam(cr, 0x001A), cam(cr, 0x0020));
    cyc(28);
    copy16(cr, cam(cr, 0x001C), cam(cr, 0x0022));
    cyc(28);
    copy16(cr, cam(cr, 0x001E), cam(cr, 0x0024));
    (cyc(84), CameraFollowLink(cr));
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
void CameraSnapToCar(Carried *cr)
{
    uint16_t m;

    link(cr);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(52, cam(cr, 0x0000)));
    m = cr->a;
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(52), snap(cr));
    link(cr);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->a = m;
    set_p(cr, nz16(cr->p, m));
    wr16(cam(cr, 0x0000), cyc_then(88, m));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(64);
}

void abi_CameraSnapToCar(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraSnapToCar(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* (a - b) halved toward zero, plus b: the move half way (16-bit). */
static uint16_t half_way(Carried *cr, uint16_t a, uint32_t at)
{
    uint8_t c;

    set_p(cr, (uint8_t)(cr->p | (P_C)));
    cyc(44);
    cr->a = alu_sbc16(a, rd16(at), &cr->p);
    r_cmp16(cr, 0x8000);
    c = (uint8_t)(cr->a & 1);
    cr->a = (uint16_t)((cr->a >> 1) | ((cr->p & P_C) ? (cyc(18), 0x8000) : 0));
    set_p(cr, (uint8_t)(nz16((uint8_t)(cr->p & ~P_C), cr->a) | c));
    if (cr->a & 0x8000)
        cr->a = alu_adc16(cr->a, 0x0000, &cr->p);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    cyc(98);
    cr->a = alu_adc16(cr->a, rd16(at), &cr->p);
    return cr->a;
}

/* REP #$20, PHX, the car's record word +$42 (long, through X) set by
   `op` (1: ORA #$1010, 0: AND #$EFFF), SEP #$20, PLX. */
static void car_rec42(Carried *cr, int op)
{
    uint16_t sx = cr->x;
    uint32_t at;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    (cyc(52), car_y(cr, 0));
    r_lda16(cr, cyc_then(30, F(W(obj_22), cr->y)));
    cr->x = cr->a;
    set_p(cr, nz16(cr->p, cr->x));
    at = (0x700042 + cr->x) & 0xFFFFFF;
    cyc(46);
    r_lda16(cr, at);
    cr->a = op ? (uint16_t)(cr->a | 0x1010) : (uint16_t)(cr->a & 0xEFFF);
    set_p(cr, nz16(cr->p, cr->a));
    cyc(56);
    wr16(at, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->x = sx;
    set_p(cr, nz16(cr->p, sx));
    cyc(op ? 80 : 58); /* SEP, PLX (, BRA) */
}

/* 08:8FFE: the overhead shot's frame (see CameraOverheadStep). */
static void overhead_frame(Carried *cr)
{
    uint16_t sx, t;
    uint8_t v;
    unsigned pre = 78; /* to the load of +$0E: SEP (, BRA), REP, LDA */

    r_lda8(cr, cyc_then(30, F(W(obj_48), cr->x)));
    if ((uint8_t)cr->a) {
        v = (uint8_t)(rd8(cyc_then(52, F(W(obj_48), cr->x))) - 1);
        wr8(F(W(obj_48), cr->x), cyc_then(6, v));
        set_p(cr, nz8(cr->p, v));
        car_rec42(cr, 1);
    } else {
        cyc(48);
        car_rec42(cr, 0);
    }
    link(cr);
    lda8(cr, (uint8_t)(rd8(cyc_then(30, F(W(obj_25), cr->x))) & 0x01));
    if ((uint8_t)cr->a) {
        and8(cr, F(W(obj_25), cr->x), 0xFE);
        r_lda8(cr, cyc_then(30, cam(cr, 0x0034)));
        v = (uint8_t)(cr->a + 1);
        lda8(cr, v);
        set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - 2)) & ~P_C) | (v >= 2 ? P_C : 0)));
        if (!((uint8_t)(v - 2) & 0x80)) {
            lda8(cr, 0x00);
            wr8(cam(cr, 0x0034), cyc_then(16, 0x00));
            cyc(62);
            lda8(cr, 0x2F);
            SoundPort0QueuePush_cr(cr);
            st16i(cr, F(W(obj_pos_y), cr->x), 0xFC18);
            pre = 74;
        } else {
            wr8(cam(cr, 0x0034), cyc_then(44, v));
            cyc(56);
            lda8(cr, 0x24);
            SoundPort0QueuePush_cr(cr);
            st16i(cr, F(W(obj_pos_y), cr->x), 0xF830);
            pre = 96;
        }
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(pre, F(W(obj_pos_y), cr->x)));
    t = half_way(cr, cr->a, cam(cr, 0x000A));
    wr16(cam(cr, 0x000A), cyc_then(30, t));
    r_lda16(cr, cyc_then(30, F(W(obj_pos_x), cr->x)));
    wr16(db(cr) | 0x1F4C, cyc_then(24, cr->a));
    r_lda16(cr, cyc_then(30, F(W(obj_pos_z), cr->x)));
    wr16(db(cr) | 0x1F50, cyc_then(24, cr->a));
    sx = cr->x;
    (cyc(30), link(cr));
    r_lda16(cr, cyc_then(30, cam(cr, 0x0002)));
    cr->x = cr->y;
    set_p(cr, nz16(cr->p, cr->x));
    cr->y = cr->a;
    set_p(cr, nz16(cr->p, cr->y));
    r_lda16(cr, cyc_then(58, F(W(obj_pos_x), cr->y)));
    set_p(cr, (uint8_t)(cr->p | (P_C)));
    cr->a = alu_sbc16(cr->a, rd16(cyc_then(38, db(cr) | 0x1F4C)), &cr->p);
    (cyc(56), ScaleThreeQuarters_cr(cr));       /* JSL, less the SBC's 6 */
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    cr->a = alu_adc16(cr->a, rd16(cyc_then(82, db(cr) | 0x1F4C)), &cr->p);
    cyc(22);
    wr16((uint16_t)(cr->d + 0x08 + cr->x), cr->a);
    r_lda16(cr, cyc_then(30, F(W(obj_pos_z), cr->y)));
    set_p(cr, (uint8_t)(cr->p | (P_C)));
    cr->a = alu_sbc16(cr->a, rd16(cyc_then(38, db(cr) | 0x1F50)), &cr->p);
    (cyc(56), ScaleThreeQuarters_cr(cr));       /* JSL, less the SBC's 6 */
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    cr->a = alu_adc16(cr->a, rd16(cyc_then(82, db(cr) | 0x1F50)), &cr->p);
    cyc(22);
    wr16((uint16_t)(cr->d + 0x0C + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->x = sx;
    set_p(cr, nz16(cr->p, sx));
    (cyc(82), tail(cr));
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
void CameraOverheadStep(Carried *cr)
{
    overhead_frame(cr);
}

void abi_CameraOverheadStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraOverheadStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void CameraOverhead(Carried *cr)
{
    link(cr);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(52, cam(cr, 0x0002)));
    cr->y = cr->a;
    set_p(cr, nz16(cr->p, cr->y));
    r_lda16(cr, cyc_then(44, F(W(obj_pos_x), cr->y)));
    wr16(db(cr) | 0x1F4C, cyc_then(24, cr->a));
    wr16(F(W(obj_pos_x), cr->x), cyc_then(30, cr->a));
    r_lda16_imm(cr, 0xFC18);
    wr16(db(cr) | 0x1F4E, cyc_then(10, cr->a));
    wr16(F(W(obj_pos_y), cr->x), cyc_then(30, cr->a));
    r_lda16(cr, cyc_then(30, F(W(obj_pos_z), cr->y)));
    r_add16(cr, 0xFED4);
    wr16(db(cr) | 0x1F50, cyc_then(62, cr->a));
    wr16(F(W(obj_pos_z), cr->x), cyc_then(30, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(22), link(cr));
    copy16(cr, cam(cr, 0x0008), db(cr) | 0x1F4C);
    cyc(22);
    copy16(cr, cam(cr, 0x000A), db(cr) | 0x1F4E);
    cyc(22);
    copy16(cr, cam(cr, 0x000C), db(cr) | 0x1F50);
    cyc(22);
    mode(cr, 0x0003);
    (cyc(84), CameraFollowLink(cr));
    handler(cr, cr->x, 0x8FFE);
    st8i(cr, F(W(obj_48), cr->x), cyc_then(6, 0x0A));
    overhead_frame(cr);
}

void abi_CameraOverhead(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraOverhead(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9176 CameraTableStep
 * WHAT IT DOES: A frame: the camera's point from the director's table
 * (CameraPointFromTable), then the director's tail.
 * HOW: JSL, JMP to 08:8EF9.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void CameraTableStep(Carried *cr)
{
    (cyc(62), CameraPointFromTable(cr));
    (cyc(24), tail(cr));
}

void abi_CameraTableStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraTableStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:90D9 CameraSnapToTable
 * WHAT IT DOES: The camera put back on its car (mode 1, turn words cleared,
 * CameraFollowLink), its +$04 cleared, mode $0755, frame $08:9176
 * (CameraTableStep); then that frame.
 * HOW: Word stores. Reached only by unit_replay_camera_08.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void CameraSnapToTable(Carried *cr)
{
    snap(cr);
    link(cr);
    st16i(cr, cam(cr, 0x0004), 0x0000);
    cyc(22);
    mode(cr, 0x0755);
    cyc(22);
    handler(cr, cr->x, 0x9176);
    CameraTableStep(cr);
}

void abi_CameraSnapToTable(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraSnapToTable(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 08:91D0: the path shot's frame: CameraApproachTarget; arrived (carry),
   $70:273C bit 0 set and back to the table; else the tail. */
static void path_frame(Carried *cr)
{
    CameraApproachTarget(cr);
    if (!(cr->p & P_C)) {
        cyc(46);
        tail(cr);
        return;
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(76);
    r_lda16(cr, 0x70273C);
    cr->a |= 0x0001;
    set_p(cr, nz16(cr->p, cr->a));
    cyc(56);
    wr16(0x70273C, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(14), table_start(cr));
}

/* A path shot: frame $08:91D0, mode 1, the path at Y (ObjectPathStart),
   CameraFollowLink, then the frame. */
static void path_shot(Carried *cr, uint16_t path)
{
    cr->y = path;
    set_p(cr, nz16(cr->p, cr->y));
    (cyc(100), w_ObjectPathStart(cr));
    (cyc(62), CameraFollowLink(cr));
    path_frame(cr);
}

/*
 * @routine 08:917D CameraPathA
 * WHAT IT DOES: A path shot: frame $08:91D0 (CameraPathStep), mode 1, the
 * director on path $885F (ObjectPathStart), CameraFollowLink; then that
 * frame.
 * HOW: Word stores; a JMP into the shared start at 08:91C8.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void CameraPathA(Carried *cr)
{
    handler(cr, cr->x, 0x91D0);
    mode(cr, 0x0001);
    cyc(32);
    path_shot(cr, 0x885F);
}

void abi_CameraPathA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPathA(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:919F CameraPathB
 * WHAT IT DOES: As CameraPathA on path $882D, or $85B9 in HUD mode 3.
 * HOW: CMP #3 on $0D2B.
 * Entry: as CameraPathA. Exit: as CameraPathA.
 */
void CameraPathB(Carried *cr)
{
    uint8_t h;

    handler(cr, cr->x, 0x91D0);
    mode(cr, 0x0001);
    cr->y = 0x882D;
    set_p(cr, nz16(cr->p, cr->y));
    r_lda8(cr, cyc_then(70, db(cr) | 0x0D2B));
    h = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(h - 3)) & ~P_C) | (h >= 3 ? (cyc(18), P_C) : 0)));
    path_shot(cr, h == 3 ? 0x85B9 : 0x882D);
}

void abi_CameraPathB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPathB(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:91D0 CameraPathStep
 * WHAT IT DOES: The path shot's frame: the director moves on along its path
 * (CameraApproachTarget); once there, $70:273C bit 0 is set and the
 * director goes back to its table (08:8EC1); else the director's tail.
 * HOW: Carry from CameraApproachTarget; a 16-bit long OR.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void CameraPathStep(Carried *cr)
{
    path_frame(cr);
}

void abi_CameraPathStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPathStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The drift frames: the camera's point moved by the director's velocity
   (+$2E/+$30/+$32), the velocity eased toward (0, ty, 0)
   (ApproachEighthMin8); while +$48 counts down the tail, then `done`. */
static void drift_frame(Carried *cr, uint16_t ty, void (*done)(Carried *))
{
    static const uint16_t v[3] = {0x202E, 0x2030, 0x2032};
    static const uint16_t c[3] = {0x0008, 0x000A, 0x000C};
    uint16_t t[3];
    int i;

    t[0] = 0x0000;
    t[1] = ty;
    t[2] = 0x0000;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    (cyc(22), link(cr));
    for (i = 0; i < 3; i++) {
        r_lda16(cr, cyc_then(30, F(v[i], cr->x)));
        r_add16(cr, rd16(cyc_then(44, cam(cr, c[i]))));
        wr16(cam(cr, c[i]), cyc_then(30, cr->a));
    }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    for (i = 0; i < 3; i++) {
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        r_lda16(cr, cyc_then(74, F(v[i], cr->x)));
        cyc(16);
        wr16(W(dp_3e), cr->a);
        r_lda16_imm(cr, t[i]);
        r_approach(cr, ApproachEighthMin8);
        wr16(F(v[i], cr->x), cyc_then(30, cr->a));
        set_p(cr, (uint8_t)(cr->p | (P_M)));
    }
    r_lda8(cr, cyc_then(52, F(W(obj_48), cr->x)));
    if (!(uint8_t)cr->a) {
        cyc(48);
        done(cr);
        return;
    }
    {
        uint8_t n = (uint8_t)(rd8(cyc_then(52, F(W(obj_48), cr->x))) - 1);

        wr8(F(W(obj_48), cr->x), cyc_then(6, n));
        set_p(cr, nz8(cr->p, n));
    }
    (cyc(24), tail(cr));
}

/* A drift shot: frame f, mode 3, +$48 = n, the director's velocity an
   eighth of its car's record velocity (Y cleared with `flat`). */
static void drift_shot(Carried *cr, uint16_t f, uint8_t n, int flat)
{
    handler(cr, cr->x, f);
    mode(cr, 0x0003);
    st8i(cr, F(W(obj_48), cr->x), cyc_then(28, n));
    car(cr);
    ObjectVelocityFromRecordOf_cr(cr);
    if (flat)
        st16i(cr, F(W(obj_vel_y), cr->x), 0x0000);
}

/*
 * @routine 08:921D CameraDriftStep
 * WHAT IT DOES: The drift shot's frame: the camera's point moves by the
 * director's velocity, which eases toward 0 (ApproachEighthMin8); while
 * +$48 counts down the director's tail, then CameraDirectorSkip2.
 * HOW: 16-bit ADCs; ApproachEighthMin8 through $3E; JML.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void CameraDriftStep(Carried *cr)
{
    drift_frame(cr, 0x0000, CameraDirectorSkip2);
}

void abi_CameraDriftStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDriftStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:91EB CameraDrift
 * WHAT IT DOES: The drift shot: frame $08:921D (CameraDriftStep), mode 3,
 * +$48 = $1E, the director's velocity from its car's record
 * (ObjectVelocityFromRecordOf) with no Y; then that frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void CameraDrift(Carried *cr)
{
    drift_shot(cr, 0x921D, 0x1E, 1);
    (cyc(22), CameraDriftStep(cr));
}

void abi_CameraDrift(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDrift(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9311 CameraDriftLongStep
 * WHAT IT DOES: As CameraDriftStep (its own copy of the code).
 * HOW: As CameraDriftStep. Reached only by unit_replay_camera_08.
 * Entry: as CameraDriftStep. Exit: as CameraDriftStep.
 */
void CameraDriftLongStep(Carried *cr)
{
    drift_frame(cr, 0x0000, CameraDirectorSkip2);
}

void abi_CameraDriftLongStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDriftLongStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:92DF CameraDriftLong
 * WHAT IT DOES: As CameraDrift with +$48 = $28 and frame $08:9311
 * (CameraDriftLongStep).
 * HOW: As CameraDrift. Reached only by unit_replay_camera_08.
 * Entry: as CameraDrift. Exit: as CameraDrift.
 */
void CameraDriftLong(Carried *cr)
{
    drift_shot(cr, 0x9311, 0x28, 1);
    (cyc(22), CameraDriftLongStep(cr));
}

void abi_CameraDriftLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDriftLong(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:946C CameraZoomIn
 * WHAT IT DOES: The zoom-in: frame $08:9488 (CameraZoomInStep), mode 2;
 * then that frame.
 * HOW: Word stores.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void abi_CameraZoomInStep(void);

void CameraZoomIn(Carried *cr)
{
    handler(cr, cr->x, 0x9488);
    mode(cr, 0x0002);
    (cyc(22), CameraZoomInStep(cr));
}

void abi_CameraZoomIn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraZoomIn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:93FF CameraRiseStep
 * WHAT IT DOES: The rising drift's frame: as CameraDriftStep with the Y
 * velocity eased toward -$0A; when +$48 runs out, CameraZoomIn.
 * HOW: As CameraDriftStep; a JML into CameraZoomIn.
 * Entry: as CameraDriftStep. Exit: as the path leaves it.
 */
void CameraRiseStep(Carried *cr)
{
    drift_frame(cr, 0xFFF6, CameraZoomIn);
}

void abi_CameraRiseStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraRiseStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:93D7 CameraRise
 * WHAT IT DOES: The rising drift: frame $08:93FF (CameraRiseStep), mode 3,
 * +$48 = $28, the director's velocity from its car's record; then that
 * frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void CameraRise(Carried *cr)
{
    drift_shot(cr, 0x93FF, 0x28, 0);
    CameraRiseStep(cr);
}

void abi_CameraRise(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraRise(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The fixed shots: frame f (a JMP to the tail), mode 3, the point from the
   car's table entry (GsuTableEntryNext `n` times, GsuTableEntryTo1F4C)
   moved by (dx, -$64, dz); then the tail. */
static void fixed_shot(Carried *cr, uint16_t f, int n, uint16_t dx, uint16_t dz)
{
    uint16_t sx;

    handler(cr, cr->x, f);
    mode(cr, 0x0003);
    sx = cr->x;
    (cyc(52), car_y(cr, 0));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(52, F(W(obj_22), cr->y)));
    cr->x = cr->a;
    set_p(cr, nz16(cr->p, cr->x));
    cyc(46);
    r_lda16(cr, (0x700058 + cr->x) & 0xFFFFFF);
    cr->x = sx;
    set_p(cr, nz16(cr->p, sx));
    cyc(36);
    while (n--)
        { cyc(62); GsuTableEntryNext_cr(cr); }
    (cyc(62), GsuTableEntryTo1F4C_cr(cr));
    link(cr);
    r_lda16(cr, cyc_then(24, db(cr) | 0x1F4C));
    if (dx)
        { cyc(38); r_add16(cr, dx); }
    wr16(cam(cr, 0x0008), cyc_then(30, cr->a));
    r_lda16(cr, cyc_then(24, db(cr) | 0x1F4E));
    r_add16(cr, 0xFF9C);
    wr16(cam(cr, 0x000A), cyc_then(68, cr->a));
    r_lda16(cr, cyc_then(24, db(cr) | 0x1F50));
    if (dz)
        { cyc(38); r_add16(cr, dz); }
    wr16(cam(cr, 0x000C), cyc_then(30, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(46), tail(cr));
}

/*
 * @routine 08:92DC CameraHoldStep
 * WHAT IT DOES: A held shot's frame: the director's tail only.
 * HOW: JMP to 08:8EF9.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void CameraHoldStep(Carried *cr)
{
    (cyc(24), tail(cr));
}

void abi_CameraHoldStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraHoldStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void CameraFixedAhead(Carried *cr)
{
    fixed_shot(cr, 0x92DC, 2, 0x0000, 0x0000);
}

void abi_CameraFixedAhead(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraFixedAhead(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:93D4 CameraHoldStep2
 * WHAT IT DOES: As CameraHoldStep (its own JMP).
 * HOW: JMP to 08:8EF9. Reached only by unit_replay_camera_08.
 * Entry: as CameraHoldStep. Exit: as CameraHoldStep.
 */
void CameraHoldStep2(Carried *cr)
{
    (cyc(24), tail(cr));
}

void abi_CameraHoldStep2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraHoldStep2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void CameraFixedSide(Carried *cr)
{
    fixed_shot(cr, 0x93D4, 1, 0x03E8, 0x03E8);
}

void abi_CameraFixedSide(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraFixedSide(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A zoom frame: the camera's zoom +$0E eased toward `to` by `fn`; the tail. */
static void zoom_frame(Carried *cr, uint16_t to, uint16_t (*fn)(uint16_t, int *))
{
    link(cr);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(52, cam(cr, 0x000E)));
    cyc(16);
    wr16(W(dp_3e), cr->a);
    r_lda16_imm(cr, to);
    cyc(48);
    r_approach(cr, fn);
    wr16(cam(cr, 0x000E), cyc_then(30, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(46), tail(cr));
}

/*
 * @routine 08:9488 CameraZoomInStep
 * WHAT IT DOES: The zoom-in's frame: the camera's zoom +$0E eased toward
 * $1000 (ApproachThirtySecondMin32); the tail.
 * HOW: Through $3E (16-bit).
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void CameraZoomInStep(Carried *cr)
{
    zoom_frame(cr, 0x1000, ApproachThirtySecondMin32);
}

void abi_CameraZoomInStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraZoomInStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9502 CameraZoomOutStep
 * WHAT IT DOES: The zoom-out's frame: the zoom +$0E eased toward $2000
 * (ApproachSixtyFourthMin64); the tail.
 * HOW: Through $3E (16-bit).
 * Entry: as CameraZoomInStep. Exit: as CameraZoomInStep.
 */
void CameraZoomOutStep(Carried *cr)
{
    zoom_frame(cr, 0x2000, ApproachSixtyFourthMin64);
}

void abi_CameraZoomOutStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraZoomOutStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:94E6 CameraZoomOut
 * WHAT IT DOES: The zoom-out: frame $08:9502 (CameraZoomOutStep), mode
 * $0654; then that frame.
 * HOW: Word stores.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void CameraZoomOut(Carried *cr)
{
    handler(cr, cr->x, 0x9502);
    mode(cr, 0x0654);
    (cyc(22), CameraZoomOutStep(cr));
}

void abi_CameraZoomOut(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraZoomOut(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:94DD CameraOrbitStep
 * WHAT IT DOES: The orbit shot's frame: the director moves along its path
 * (CameraApproachTarget); once there it runs into CameraZoomOut, else the
 * tail.
 * HOW: Carry from CameraApproachTarget; the fall into 08:94E6.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void CameraOrbitStep(Carried *cr)
{
    CameraApproachTarget(cr);
    if (cr->p & P_C) {
        cyc(22);
        CameraZoomOut(cr);
        return;
    } else { cyc(40); }
    tail(cr);
}

void abi_CameraOrbitStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraOrbitStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:94A1 CameraOrbit
 * WHAT IT DOES: The orbit shot: frame $08:94DD (CameraOrbitStep), mode
 * $0655, the director on path $88BB (ObjectPathStart), the car's record
 * +$40 bit 2 set; then that frame.
 * HOW: Word stores; the record word through X (PHX/PLX, 16-bit).
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void CameraOrbit(Carried *cr)
{
    uint16_t sx;
    uint32_t at;

    handler(cr, cr->x, 0x94DD);
    mode(cr, 0x0655);
    cr->y = 0x88BB;
    set_p(cr, nz16(cr->p, cr->y));
    (cyc(108), w_ObjectPathStart(cr));
    sx = cr->x;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(58, F(W(obj_06), cr->x)));
    cr->x = cr->a;
    set_p(cr, nz16(cr->p, cr->x));
    r_lda16(cr, cyc_then(44, F(W(obj_22), cr->x)));
    cr->x = cr->a;
    set_p(cr, nz16(cr->p, cr->x));
    at = (0x700040 + cr->x) & 0xFFFFFF;
    cyc(46);
    r_lda16(cr, at);
    cr->a |= 0x0004;
    set_p(cr, nz16(cr->p, cr->a));
    cyc(56);
    wr16(at, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->x = sx;
    set_p(cr, nz16(cr->p, sx));
    (cyc(58), CameraOrbitStep(cr));
}

void abi_CameraOrbit(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraOrbit(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* LDY +$06, PHX, TYX, REP #$20, the car's record (through X = +$22): the
   word at `field` = v (read first with `read`), SEP #$20, PLX. */
static void car_rec_set(Carried *cr, uint32_t field, uint16_t v, int read)
{
    uint16_t sx;
    uint32_t at;

    car_y(cr, 0);
    sx = cr->x;
    cr->x = cr->y;
    set_p(cr, nz16(cr->p, cr->x));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(96, F(W(obj_22), cr->x)));
    cr->x = cr->a;
    set_p(cr, nz16(cr->p, cr->x));
    at = (field + cr->x) & 0xFFFFFF;
    if (read)
        { cyc(46); r_lda16(cr, at); } else { cyc(14); }
    r_lda16_imm(cr, v);
    cyc(18);
    wr16(at, v);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->x = sx;
    set_p(cr, nz16(cr->p, sx));
}

/*
 * @routine 08:9564 CameraEnd
 * WHAT IT DOES: The end of the replay shot: frame $08:958C (CameraEndStep),
 * the car's record +$1C = $10 (read first); then the tail.
 * HOW: The record word through X (PHX, TYX, PLX). Reached only by
 * unit_replay_camera_08.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void CameraEnd(Carried *cr)
{
    handler(cr, cr->x, 0x958C);
    car_rec_set(cr, 0x70001C, 0x0010, 1);
    (cyc(82), tail(cr));
}

void abi_CameraEnd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraEnd(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:958C CameraEndStep
 * WHAT IT DOES: The end shot's frame: the director's tail only.
 * HOW: JMP to 08:8EF9. Reached only by unit_replay_camera_08.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void CameraEndStep(Carried *cr)
{
    (cyc(24), tail(cr));
}

void abi_CameraEndStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraEndStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void CameraStillStep(Carried *cr)
{
    car_rec_set(cr, 0x700042, 0x0410, 0);
    r_lda8(cr, cyc_then(88, F(W(obj_48), cr->x)));
    if (!(uint8_t)cr->a) {
        cyc(48);
        CameraEnd(cr);
        return;
    }
    {
        uint8_t n = (uint8_t)(rd8(cyc_then(52, F(W(obj_48), cr->x))) - 1);

        wr8(F(W(obj_48), cr->x), cyc_then(6, n));
        set_p(cr, nz8(cr->p, n));
    }
    (cyc(62), CameraPointFromTable(cr));
    (cyc(24), tail(cr));
}

void abi_CameraStillStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraStillStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:951B CameraStill
 * WHAT IT DOES: The still shot: frame $08:953C (CameraStillStep), +$48 =
 * $0A, the camera's +$37 word = $8000; then that frame.
 * HOW: Word and byte stores. Reached only by unit_replay_camera_08.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void CameraStill(Carried *cr)
{
    handler(cr, cr->x, 0x953C);
    st8i(cr, F(W(obj_48), cr->x), cyc_then(6, 0x0A));
    link(cr);
    st16i(cr, cam(cr, 0x0037), 0x8000);
    (cyc(22), CameraStillStep(cr));
}

void abi_CameraStill(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraStill(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
static uint32_t dpa(Carried *cr, uint8_t off)
{
    return (uint16_t)(cr->d + off);
}

/* Direct page word at off (bank 0). */
static uint32_t dpw(Carried *cr, uint8_t off)
{
    cyc(14);
    return (uint16_t)(cr->d + off);
}

/* PHX, PHB, CameraFollowStep, SEP #$20, PLB, PLX. */
static void follow_step(Carried *cr)
{
    uint16_t sx = cr->x;
    uint8_t sb = cr->db;

    (cyc(114), CameraFollowStep_cr(cr));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->db = sb;
    set_p(cr, nz8(cr->p, sb));
    cr->x = sx;
    set_p(cr, nz16(cr->p, sx));
}

/* LDA #v, STA $05FA (the camera's view kind). */
static void view(Carried *cr, uint8_t v)
{
    st8i(cr, db(cr) | 0x05FA, v);
}

/* A word divided by 2 or 4 keeping its sign (CMP #$8000, ROR, n times). */
static void asr_n(Carried *cr, int n)
{
    while (n--) {
        uint8_t c;

        r_cmp16(cr, 0x8000);
        c = (uint8_t)(cr->a & 1);
        cr->a = (uint16_t)((cr->a >> 1) | ((cr->p & P_C) ? 0x8000 : 0));
        set_p(cr, (uint8_t)(nz16((uint8_t)(cr->p & ~P_C), cr->a) | c));
    }
}

/* 08:95C7: the follower's frame (see CameraFollowCarStep). */
static void follower_frame(Carried *cr)
{
    uint32_t r;
    static const uint8_t dpo[3] = {0xC5, 0xC7, 0xC9};
    static const uint16_t keep[3] = {0x1F64, 0x1F66, 0x1F68};
    static const uint16_t to[3] = {0x1F4C, 0x1F4E, 0x1F50};
    int i;

    car_y(cr, 0);
    wr8(db(cr) | 0x1F41, cyc_then(24, 0x00));
    for (i = 0; i < 3; i++)
        copy16x(cr, db(cr) | keep[i], dpa(cr, dpo[i]), i ? 60 : 38, 24);
    cyc(22);
    follow_step(cr);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->y = cr->x;
    set_p(cr, nz16(cr->p, cr->y));
    r_lda16(cr, cyc_then(152, F(W(obj_22), cr->x)));
    cr->x = cr->a;
    set_p(cr, nz16(cr->p, cr->x));
    r = cr->x;
    cyc(14);
    for (i = 0; i < 3; i++) {
        cyc(32);
        r_lda16(cr, (0x70001E + 2 * i + r) & 0xFFFFFF);
        wr16(db(cr) | to[i], cyc_then(24, cr->a));
    }
    r_lda16(cr, cyc_then(2, dpw(cr, 0xB7)));
    asr_n(cr, 2);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    cr->a = alu_adc16(cr->a, rd16(cyc_then(120, F(W(obj_pos_x), cr->y))), &cr->p);
    cyc(32);
    wr16((0x70001E + r) & 0xFFFFFF, cr->a);
    r_lda16_imm(cr, 0x00C8);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    cr->a = alu_adc16(cr->a, rd16(cyc_then(30, F(W(obj_pos_y), cr->y))), &cr->p);
    cyc(32);
    wr16((0x700020 + r) & 0xFFFFFF, cr->a);
    r_lda16(cr, cyc_then(2, dpw(cr, 0xBB)));
    asr_n(cr, 2);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    cr->a = alu_adc16(cr->a, rd16(cyc_then(120, F(W(obj_pos_z), cr->y))), &cr->p);
    cyc(32);
    wr16((0x700022 + r) & 0xFFFFFF, cr->a);
    cr->x = cr->y;
    set_p(cr, nz16(cr->p, cr->x));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    st8i(cr, (cyc(36), db(cr)) | 0x1F41, 0xFF);
    (cyc(62), ObjectSyncAngles_cr(cr));
    cr->a = (uint16_t)((cr->a & 0xFF00) | (cr->a & 0x01));
    set_p(cr, nz8(cr->p, (uint8_t)cr->a));
    cyc(16);                            /* AND #$01 */
    if ((uint8_t)cr->a) {
        cyc(16);                        /* BEQ not taken */
        (cyc(62), RecordContactSummary_cr(cr));
        cr->a = (uint16_t)((cr->a & 0xFF00) | (cr->a & 0x18));
        set_p(cr, nz8(cr->p, (uint8_t)cr->a));
        if ((uint8_t)cr->a) {
            cyc(32); /* AND, BEQ not taken */
            copy16x(cr, dpa(cr, 0xC5), db(cr) | 0x1F4C, 46, 16);
            copy16x(cr, dpa(cr, 0xC7), db(cr) | 0x1F4E, 68, 16);
            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
            r_lda16(cr, cyc_then(46, dpw(cr, 0xC7)));
            r_add16(cr, 0xFF38);
            wr16(dpw(cr, 0xC7), cyc_then(40, cr->a));
            set_p(cr, (uint8_t)(cr->p | (P_M)));
            copy16x(cr, dpa(cr, 0xC9), db(cr) | 0x1F50, 68, 16);
            cyc(22);
        } else { cyc(16); } /* AND, BEQ taken */
        } else {
        for (i = 0; i < 3; i++)
            { cyc(8); copy16x(cr, dpa(cr, dpo[i]), db(cr) | keep[i], 60, 16); }
    }
    cyc(84);
    ObjectFaceTied_cr(cr);
    st8i(cr, F(W(obj_36), cr->x), cyc_then(6, 0x03));
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
void CameraFollowCar(Carried *cr)
{
    view(cr, 0x05);
    handler(cr, cr->x, 0x95C7);
    (cyc(62), GsuRecordNewCamera_cr(cr));
    follow_step(cr);
    (cyc(148), ObjectSyncToRecord_cr(cr));
    cyc(44); /* RTL */
}

void abi_CameraFollowCar(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraFollowCar(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void CameraFollowCarStep(Carried *cr)
{
    follower_frame(cr);
}

void abi_CameraFollowCarStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraFollowCarStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_CameraReturnStep(void);

/*
 * @routine 08:9833 CameraReturn
 * WHAT IT DOES: The director heads back to its table: frame $08:9847
 * (CameraReturnStep), +$48 = $0F; then that frame.
 * HOW: Word and byte stores; entered by JMLs from the shots below.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void CameraReturn(Carried *cr)
{
    handler(cr, cr->x, 0x9847);
    st8i(cr, F(W(obj_48), cr->x), cyc_then(6, 0x0F));
    CameraReturnStep(cr);
}

void abi_CameraReturn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraReturn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9847 CameraReturnStep
 * WHAT IT DOES: The return's frame: view $05FA = 9; +$48 counts down with
 * the director's table shot (CameraDirectorTable) each frame; at 0 the
 * car's +$48 = 0 and the director goes back to its table (08:8EC1).
 * HOW: DEC, JML CameraDirectorTable; JML to 08:8EC1.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void CameraReturnStep(Carried *cr)
{
    uint8_t n;

    view(cr, 0x09);
    n = (uint8_t)(rd8(cyc_then(30, F(W(obj_48), cr->x))) - 1);
    wr8(F(W(obj_48), cr->x), cyc_then(6, n));
    set_p(cr, nz8(cr->p, n));
    if (n) {
        cyc(48);
        CameraDirectorTable(cr);
        return;
    } else { cyc(22); }
    car_y(cr, 0);
    st8i(cr, F(W(obj_48), cr->y), cyc_then(6, 0x00));
    table_start(cr);
}

void abi_CameraReturnStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraReturnStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/* (a - b)/2 + b with the sign kept (no rounding). */
static uint16_t mid(Carried *cr, uint16_t a, uint32_t at)
{
    set_p(cr, (uint8_t)(cr->p | (P_C)));
    cyc(38);
    cr->a = alu_sbc16(a, rd16(at), &cr->p);
    asr_n(cr, 1);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    cyc(70);
    cr->a = alu_adc16(cr->a, rd16(at), &cr->p);
    return cr->a;
}

/* 08:9825: the track shot's frame: a cue (+$25 bit 0) sends the director
   back (CameraReturn), else the tail. */
static void track_half_frame(Carried *cr)
{
    lda8(cr, (uint8_t)(rd8(cyc_then(30, F(W(obj_25), cr->x))) & 0x01));
    if ((uint8_t)cr->a) {
        cyc(26);
        CameraReturn(cr);
        return;
    }
    (cyc(24), tail(cr));
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
void CameraTrackHalf(Carried *cr)
{
    view(cr, 0x02);
    handler(cr, cr->x, 0x9825);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(52, F(W(obj_29), cr->x)));
    (cyc(62), GsuTableEntryNext_cr(cr));
    (cyc(62), GsuTableEntryTo1F4C_cr(cr));
    copy16x(cr, dpa(cr, 0x04), db(cr) | 0x1F4C, 24, 16);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(24, db(cr) | 0x1F4E));
    wr16(dpw(cr, 0x0A), cyc_then(2, cr->a));
    r_lda16(cr, cyc_then(24, db(cr) | 0x1F50));
    wr16(db(cr) | 0x05F2, cyc_then(24, cr->a));
    r_lda16(cr, cyc_then(30, F(W(obj_29), cr->x)));
    (cyc(62), GsuTableEntryTo1F4C_cr(cr));
    r_lda16(cr, cyc_then(24, db(cr) | 0x1F4E));
    r_add16(cr, 0xFE0C);
    wr16(db(cr) | 0x1F4E, cyc_then(62, cr->a));
    r_lda16(cr, cyc_then(2, dpw(cr, 0x04)));
    wr16(F(W(obj_pos_x), cr->x), cyc_then(30, mid(cr, cr->a, db(cr) | 0x1F4C)));
    r_lda16(cr, cyc_then(2, dpw(cr, 0x0A)));
    r_add16(cr, 0xFE0C);
    wr16(F(W(obj_pos_y), cr->x), cyc_then(30, mid(cr, cr->a, (cyc(38), db(cr)) | 0x1F4E)));
    r_lda16(cr, cyc_then(24, db(cr) | 0x05F2));
    wr16(F(W(obj_pos_z), cr->x), cyc_then(30, mid(cr, cr->a, db(cr) | 0x1F50)));
    (cyc(62), ObjectGsu81E3_cr(cr));
    (cyc(62), ObjectGsu80BCThen81E3_cr(cr));
    cr->a ^= 0xFFFF;
    set_p(cr, nz16(cr->p, cr->a));
    cr->a++;
    set_p(cr, nz16(cr->p, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(38), car_y(cr, 0));
    set_p(cr, nz16(cr->p, cr->x));
    (cyc(80), track_half_frame(cr));
}

void abi_CameraTrackHalf(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraTrackHalf(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9825 CameraTrackHalfStep
 * WHAT IT DOES: The track shot's frame: a cue (+$25 bit 0) sends the
 * director back (CameraReturn), else the director's tail.
 * HOW: AND #1; JML CameraReturn.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void CameraTrackHalfStep(Carried *cr)
{
    track_half_frame(cr);
}

void abi_CameraTrackHalfStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraTrackHalfStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9787 CameraBesideWait
 * WHAT IT DOES: The side shot's frame: while +$48 counts down the
 * director's tail, then the track shot (CameraTrackHalf).
 * HOW: DEC; JMLs.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void CameraBesideWait(Carried *cr)
{
    r_lda8(cr, cyc_then(30, F(W(obj_48), cr->x)));
    if (!(uint8_t)cr->a) {
        cyc(48);
        CameraTrackHalf(cr);
        return;
    }
    {
        uint8_t n = (uint8_t)(rd8(cyc_then(52, F(W(obj_48), cr->x))) - 1);

        wr8(F(W(obj_48), cr->x), cyc_then(6, n));
        set_p(cr, nz8(cr->p, n));
    }
    (cyc(32), tail(cr));
}

void abi_CameraBesideWait(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraBesideWait(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void CameraBesideStep(Carried *cr)
{
    uint16_t sx;
    uint32_t r;
    static const uint8_t f[6] = {0x2E, 0x30, 0x32, 0x36, 0x34, 0x38};
    int i;

    car_y(cr, 0);
    copy16(cr, F(W(obj_pos_x), cr->x), cyc_then(6, F(W(obj_pos_x), cr->y)));
    copy16(cr, F(W(obj_pos_z), cr->x), cyc_then(28, F(W(obj_pos_z), cr->y)));
    st16i(cr, F(W(obj_pos_y), cr->x), cyc_then(22, 0xFF38));
    {
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        r_lda16(cr, cyc_then(74, F(W(obj_pos_x), cr->x)));
        r_add16(cr, 0x0258);
        wr16(F(W(obj_pos_x), cr->x), cyc_then(68, cr->a));
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        r_lda16(cr, cyc_then(74, F(W(obj_pos_z), cr->x)));
        r_add16(cr, 0x0258);
        wr16(F(W(obj_pos_z), cr->x), cyc_then(68, cr->a));
        set_p(cr, (uint8_t)(cr->p | (P_M)));
    }
    cyc(22);
    handler(cr, cr->x, 0x9787);
    sx = cr->x;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(82, F(W(obj_22), cr->y)));
    cr->x = cr->a;
    set_p(cr, nz16(cr->p, cr->x));
    r = cr->x;
    r_lda16_imm(cr, 0x0000);
    for (i = 0; i < 6; i++)
        { cyc(32); wr16((0x700000 + f[i] + r) & 0xFFFFFF, 0x0000); }
    cyc(64);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->x = sx;
    set_p(cr, nz16(cr->p, sx));
    st8i(cr, F(W(obj_48), cr->x), 0x3C);
    CameraBesideWait(cr);
}

void abi_CameraBesideStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraBesideStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9702 CameraBeside
 * WHAT IT DOES: The side shot: frame $08:9711 (CameraBesideStep); then
 * that frame.
 * HOW: Word and byte stores; entered by a JML from CameraLiftOffStep.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void CameraBeside(Carried *cr)
{
    handler(cr, cr->x, 0x9711);
    CameraBesideStep(cr);
}

void abi_CameraBeside(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraBeside(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 08:96D1: the lift-off's frame (see CameraLiftOffStep). */
static void lift_frame(Carried *cr)
{
    r_lda8(cr, cyc_then(30, F(W(obj_48), cr->x)));
    if (!(uint8_t)cr->a) {
        cyc(48);
        CameraReturn(cr);
        return;
    }
    {
        uint8_t n = (uint8_t)(rd8(cyc_then(52, F(W(obj_48), cr->x))) - 1);

        wr8(F(W(obj_48), cr->x), cyc_then(6, n));
        set_p(cr, nz8(cr->p, n));
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(52, F(W(obj_pos_y), cr->x)));
    r_add16(cr, 0xFFCE);
    wr16(F(W(obj_pos_y), cr->x), cyc_then(68, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(22), car_y(cr, 0));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(52, F(W(obj_pos_y), cr->y)));
    r_cmp16(cr, 0xFC18);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (cr->p & P_N) {
        cyc(100);
        tail(cr);
        return;
    } else { cyc(94); }
    CameraBeside(cr);
}

/*
 * @routine 08:96D1 CameraLiftOffStep
 * WHAT IT DOES: The lift-off's frame: while +$48 counts down the director
 * rises $32; once the car (+$06) is no higher than -$3E8 the side shot
 * (CameraBeside), else the tail; at 0 back (CameraReturn).
 * HOW: 16-bit ADC and CMP; JMLs.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void CameraLiftOffStep(Carried *cr)
{
    lift_frame(cr);
}

void abi_CameraLiftOffStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraLiftOffStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:96A3 CameraLiftOff
 * WHAT IT DOES: The lift-off: view 3, the car's track entry +$29 taken,
 * frame $08:96D1 (CameraLiftOffStep), +$48 = $96, +$25 bit 0 cleared; then
 * that frame.
 * HOW: 16-bit copy; word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void CameraLiftOff(Carried *cr)
{
    view(cr, 0x03);
    car_y(cr, 0);
    copy16(cr, F(W(obj_29), cr->x), cyc_then(6, F(W(obj_29), cr->y)));
    cyc(22);
    handler(cr, cr->x, 0x96D1);
    st8i(cr, F(W(obj_48), cr->x), cyc_then(6, 0x96));
    and8(cr, F(W(obj_25), cr->x), 0xFE);
    lift_frame(cr);
}

void abi_CameraLiftOff(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraLiftOff(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9888 CameraPathFollowStep
 * WHAT IT DOES: The followed path's frame: +$36 = 3, CameraApproachTarget,
 * CameraFollowLink.
 * HOW: Two JSLs.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraFollowLink
 * leaves it.
 */
void CameraPathFollowStep(Carried *cr)
{
    st8i(cr, F(W(obj_36), cr->x), cyc_then(6, 0x03));
    CameraApproachTarget(cr);
    (cyc(62), CameraFollowLink(cr));
    cyc(44); /* RTL */
}

void abi_CameraPathFollowStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPathFollowStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9861 CameraPathFollow
 * WHAT IT DOES: The director on path $85C3 (ObjectPathStart), frame
 * $08:9888 (CameraPathFollowStep), mode 1, CameraFollowLink; then that
 * frame.
 * HOW: JSLs and word stores.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void CameraPathFollow(Carried *cr)
{
    cr->y = 0x85C3;
    set_p(cr, nz16(cr->p, cr->y));
    (cyc(86), w_ObjectPathStart(cr));
    handler(cr, cr->x, 0x9888);
    mode(cr, 0x0001);
    (cyc(84), CameraFollowLink(cr));
    CameraPathFollowStep(cr);
}

void abi_CameraPathFollow(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPathFollow(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:998B CameraPathGoStep
 * WHAT IT DOES: A path shot's frame: CameraApproachTarget, then the
 * director's tail.
 * HOW: JSL, JMP to 08:8EF9.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void CameraPathGoStep(Carried *cr)
{
    CameraApproachTarget(cr);
    (cyc(24), tail(cr));
}

void abi_CameraPathGoStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPathGoStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:997C CameraPathGo
 * WHAT IT DOES: The path shots' shared end: frame $08:998B
 * (CameraPathGoStep); then that frame.
 * HOW: Word and byte stores; entered by JMPs.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void CameraPathGo(Carried *cr)
{
    handler(cr, cr->x, 0x998B);
    CameraPathGoStep(cr);
}

void abi_CameraPathGo(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPathGo(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A path shot: the director on path p, mode 1 (unless `free`),
   CameraFollowLink, then CameraPathGo. */
static void path_go(Carried *cr, uint16_t p, int set_mode)
{
    cyc(86);
    cr->y = p;
    set_p(cr, nz16(cr->p, cr->y));
    w_ObjectPathStart(cr);
    if (set_mode)
        { cyc(0); mode(cr, 0x0001); cyc(84); } else { cyc(62); }
    CameraFollowLink(cr);
    (cyc(24), CameraPathGo(cr));
}

/*
 * @routine 08:9896 CameraPath8603
 * WHAT IT DOES: A path shot: the director on path $8603 (ObjectPathStart), mode 1,
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void CameraPath8603(Carried *cr)
{
    path_go(cr, 0x8603, 1);
}

void abi_CameraPath8603(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPath8603(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:98B1 CameraPath8643
 * WHAT IT DOES: A path shot: the director on path $8643 (ObjectPathStart), mode 1,
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void CameraPath8643(Carried *cr)
{
    path_go(cr, 0x8643, 1);
}

void abi_CameraPath8643(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPath8643(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:98CC CameraPath8679
 * WHAT IT DOES: A path shot: the director on path $8679 (ObjectPathStart), mode 1,
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void CameraPath8679(Carried *cr)
{
    path_go(cr, 0x8679, 1);
}

void abi_CameraPath8679(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPath8679(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:98E7 CameraPath869B
 * WHAT IT DOES: A path shot: the director on path $869B (ObjectPathStart), mode 1,
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void CameraPath869B(Carried *cr)
{
    path_go(cr, 0x869B, 1);
}

void abi_CameraPath869B(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPath869B(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9902 CameraPath86B3
 * WHAT IT DOES: A path shot: the director on path $86B3 (ObjectPathStart), mode 1,
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void CameraPath86B3(Carried *cr)
{
    path_go(cr, 0x86B3, 1);
}

void abi_CameraPath86B3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPath86B3(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:991D CameraPath86CB
 * WHAT IT DOES: A path shot: the director on path $86CB (ObjectPathStart), mode 1,
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void CameraPath86CB(Carried *cr)
{
    path_go(cr, 0x86CB, 1);
}

void abi_CameraPath86CB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPath86CB(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9938 CameraPath8709
 * WHAT IT DOES: A path shot: the director on path $8709 (ObjectPathStart),
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void CameraPath8709(Carried *cr)
{
    path_go(cr, 0x8709, 0);
}

void abi_CameraPath8709(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPath8709(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9946 CameraPath86E3
 * WHAT IT DOES: A path shot: the director on path $86E3 (ObjectPathStart), mode 1,
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void CameraPath86E3(Carried *cr)
{
    path_go(cr, 0x86E3, 1);
}

void abi_CameraPath86E3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPath86E3(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9961 CameraPath86FB
 * WHAT IT DOES: A path shot: the director on path $86FB (ObjectPathStart), mode 1,
 * CameraFollowLink, then CameraPathGo.
 * HOW: JSLs; a JMP into CameraPathGo.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as CameraPathGo leaves it.
 */
void CameraPath86FB(Carried *cr)
{
    path_go(cr, 0x86FB, 1);
}

void abi_CameraPath86FB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPath86FB(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 08:9A25: the low shot's frame: the camera's +$1A-$1E = (0, -$64, 0);
   the tail. */
static void low_frame(Carried *cr)
{
    link(cr);
    st16i(cr, cam(cr, 0x001A), 0x0000);
    (cyc(22), link(cr));
    st16i(cr, cam(cr, 0x001C), 0xFF9C);
    (cyc(22), link(cr));
    st16i(cr, cam(cr, 0x001E), 0x0000);
}

/*
 * @routine 08:9A25 CameraLowHoldStep
 * WHAT IT DOES: The low shot's frame: the camera's offset +$1A-$1E = (0,
 * -$64, 0), then the director's tail.
 * HOW: Word stores; JMP to 08:8EF9.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the tail leaves it.
 */
void CameraLowHoldStep(Carried *cr)
{
    low_frame(cr);
    (cyc(46), tail(cr));
}

void abi_CameraLowHoldStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraLowHoldStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9992 CameraLowBehind
 * WHAT IT DOES: The low shot from behind: mode $C080, frame $08:9A25
 * (CameraLowHoldStep), the camera's offset (0, -$64, 0), CameraFollowLink;
 * then that frame.
 * HOW: Word stores; a JMP into the frame.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void CameraLowBehind(Carried *cr)
{
    mode(cr, 0xC080);
    cyc(22);
    handler(cr, cr->x, 0x9A25);
    low_frame(cr);
    (cyc(84), CameraFollowLink(cr));
    (cyc(24), CameraLowHoldStep(cr));
}

void abi_CameraLowBehind(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraLowBehind(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9A16 CameraLowHold
 * WHAT IT DOES: The low shot held: frame $08:9A25 (CameraLowHoldStep);
 * then that frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void CameraLowHold(Carried *cr)
{
    handler(cr, cr->x, 0x9A25);
    CameraLowHoldStep(cr);
}

void abi_CameraLowHold(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraLowHold(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9A0D CameraPathLowStep
 * WHAT IT DOES: The low path shot's frame: CameraApproachTarget; once there
 * the low hold (CameraLowHold), else the tail.
 * HOW: Carry from CameraApproachTarget; BRL to 08:8EF9.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the path leaves it.
 */
void CameraPathLowStep(Carried *cr)
{
    CameraApproachTarget(cr);
    if (cr->p & P_C) {
        cyc(22);
        CameraLowHold(cr);
        return;
    } else { cyc(46); }
    tail(cr);
}

void abi_CameraPathLowStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPathLowStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The low path shots: the director on path p, mode 1, CameraFollowLink,
   frame $08:9A0D (CameraPathLowStep), then that frame. */
static void path_low(Carried *cr, uint16_t p, int jmp)
{
    cr->y = p;
    set_p(cr, nz16(cr->p, cr->y));
    (cyc(86), w_ObjectPathStart(cr));
    if (jmp)
        cyc(24); /* JMP into the shared part */
    mode(cr, 0x0001);
    (cyc(84), CameraFollowLink(cr));
    handler(cr, cr->x, 0x9A0D);
    CameraPathLowStep(cr);
}

/*
 * @routine 08:99DC CameraPathLow87B7
 * WHAT IT DOES: A low path shot on path $87B7: ObjectPathStart, mode 1,
 * CameraFollowLink, frame $08:9A0D (CameraPathLowStep); then that frame.
 * HOW: JSLs; a JMP into the shared part at 08:99ED.
 * Entry: M 8-bit, X 16-bit; X = the director. Exit: as the frame leaves it.
 */
void CameraPathLow87B7(Carried *cr)
{
    path_low(cr, 0x87B7, 1);
}

void abi_CameraPathLow87B7(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPathLow87B7(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:99E6 CameraPathLow875B
 * WHAT IT DOES: As CameraPathLow87B7 on path $875B.
 * HOW: As CameraPathLow87B7.
 * Entry: as CameraPathLow87B7. Exit: as CameraPathLow87B7.
 */
void CameraPathLow875B(Carried *cr)
{
    path_low(cr, 0x875B, 0);
}

void abi_CameraPathLow875B(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPathLow875B(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ObjectPathStart as its adapter leaves the registers. */
static void w_ObjectPathStart(Carried *cr)
{
    uint8_t p = cr->p;

    ObjectPathStart(cr->db, cr->x, cr->y, &cr->a, &cr->y, &p);
    set_p(cr, (uint8_t)(nz16(p, cr->x) | P_M));
}
