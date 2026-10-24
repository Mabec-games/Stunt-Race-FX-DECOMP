/*
 * object_handlers.c: object handlers of bank $08 (the long pointer at
 * +$1A/+$1C that ObjectRunHandlerA runs each frame): bursts of pieces, a
 * blinking child, and the Speed Trax car and signal pair. A handler that
 * becomes another stores the next one's address and runs it at once
 * (falls into it).
 *
 * Objects are at $2000 + X through the data bank; the Super FX record of
 * the object in hand is $70:0000 + ($70:2476).
 */
#include "machine.h"
#include "math_calls.h"
#include "cpu_ops.h"
#include "carried.h"

int ObjectPathCommand(uint8_t bank, uint16_t obj, uint16_t *a, uint16_t *y, uint8_t *p);
int ObjectPathStart(uint8_t bank, uint16_t obj, uint16_t path, uint16_t *a, uint16_t *y, uint8_t *p);
uint8_t RandomStepLong(void);

void abi_ObjectSpawn(void);
void abi_ObjectCopyPositionTo(void);
void abi_RandomStepLong(void);
void abi_GsuRecordNew426(void);
void abi_GsuRecordNew422(void);
void abi_GsuRecordVelocity(void);
void abi_GsuRecordPosition(void);
void abi_GsuRecordSetFlags(void);
void abi_ObjectSyncFromRecord(void);
void abi_RecordContactSummary(void);
void abi_ObjectReleaseLong(void);
void abi_ObjectReleaseY(void);
void abi_ObjectRelease(void);
void abi_ObjectBreakOffContact(void);
void abi_ObjectBreakOffChance(void);
void abi_ObjectRebucket(void);
void abi_ObjectCarSpawn(void);
void abi_CarCrashParts(void);
void abi_CameraSnapToCar(void);
void abi_ObjectNearFocus(void);
void abi_ObjectPoseFromRecord(void);
void abi_GsuRecordFlag4(void);

typedef void (*AbiFn)(void);

void CameraApproachTarget(Carried *cr);
void CameraDirectorBehind(Carried *cr);
void CameraDirectorFixed(Carried *cr);
void CameraDirectorFixedStart(Carried *cr);
void CameraDirectorHold(Carried *cr);
void CameraDirectorNext(Carried *cr);
void CameraDirectorPath(Carried *cr);
void CameraDirectorPathStart(Carried *cr);
void CameraDirectorReset(Carried *cr);
void CameraDirectorSkip2(Carried *cr);
void CameraDirectorSkip3(Carried *cr);
void CameraDirectorSkip5(Carried *cr);
void CameraDirectorTable(Carried *cr);
void CameraDirectorTrack(Carried *cr);
void CameraDirectorWait(Carried *cr);
void CameraFollowLink(Carried *cr);
void CameraNearestCar(Carried *cr);
void CameraOnCarNear(Carried *cr);
void CameraOnMarkedCar(Carried *cr);
void CameraPointFromTable(Carried *cr);
void DataBank7E(Carried *cr);
void ObjectAttachFollow(Carried *cr);
void ObjectAttachSpawn(Carried *cr);
void ObjectAttachStart(Carried *cr);
void ObjectAttachTurned(Carried *cr);
void ObjectBlinkerFade(Carried *cr);
void ObjectBlinkerRise(Carried *cr);
void ObjectBlinkerStart(Carried *cr);
void ObjectBreakOffA(Carried *cr);
void ObjectBreakOffB(Carried *cr);
void ObjectBreakOffC(Carried *cr);
void ObjectBreakOffChance(Carried *cr);
void ObjectBreakOffContact(Carried *cr);
void ObjectBreakOffD(Carried *cr);
void ObjectBreakOffE(Carried *cr);
void ObjectBreakOffF(Carried *cr);
void ObjectBreakOffG(Carried *cr);
void ObjectBreakOffH(Carried *cr);
void ObjectBurstPieceStart(Carried *cr);
void ObjectBurstPieceStep(Carried *cr);
void ObjectBurstSpawn(Carried *cr);
void ObjectCarAttachedStart(Carried *cr);
void ObjectCarAttachedStep(Carried *cr);
void ObjectChildFlying(Carried *cr);
void ObjectChildFollow(Carried *cr);
void ObjectChildLoose(Carried *cr);
void ObjectChildLooseBack(Carried *cr);
void ObjectChildReturn(Carried *cr);
void ObjectChildStart(Carried *cr);
void ObjectChildStep(Carried *cr);
void ObjectChildrenSpawn(Carried *cr);
void ObjectDemoCarStart(Carried *cr);
void ObjectDemoCarStep(Carried *cr);
void ObjectExhaustStart(Carried *cr);
void ObjectExhaustStep(Carried *cr);
void ObjectLampAnimate(Carried *cr);
void ObjectLampDrop(Carried *cr);
void ObjectLampDropB(Carried *cr);
void ObjectLampDropBStart(Carried *cr);
void ObjectLampOnParent(Carried *cr);
void ObjectLampStart(Carried *cr);
void ObjectLampWait(Carried *cr);
void ObjectMarkerFollow(Carried *cr);
void ObjectMarkerStart(Carried *cr);
void ObjectOffsetOnParent2(Carried *cr);
void ObjectPaceCarSetup(Carried *cr);
void ObjectPaceCarStart(Carried *cr);
void ObjectPaceCarStep(Carried *cr);
void ObjectParentRecordBit8(Carried *cr);
void ObjectPieceFall(Carried *cr);
void ObjectPieceFallStart(Carried *cr);
void ObjectPieceSpray(Carried *cr);
void ObjectPieceSprayXSign(Carried *cr);
void ObjectPieceSprayYSign(Carried *cr);
void ObjectPieceSprayZSign(Carried *cr);
void ObjectRingIdle(Carried *cr);
void ObjectRingPartStart(Carried *cr);
void ObjectRingPartStep(Carried *cr);
void ObjectRingSpin(Carried *cr);
void ObjectRingStartBackX(Carried *cr);
void ObjectRingStartBackZ(Carried *cr);
void ObjectRingStartX(Carried *cr);
void ObjectRingStartZ(Carried *cr);
void ObjectSignalStep(Carried *cr);
void ObjectSparkStart(Carried *cr);
void ObjectSparkStep(Carried *cr);
void ObjectSparksStart(Carried *cr);
void PlayerCarEvents(Carried *cr);
static void attach_bit6(Carried *cr);
static void attach_hide(Carried *cr);
static int attach_parent_gone(Carried *cr);
static int below16(Carried *cr, uint16_t a, uint16_t k);
static void blinker_show(Carried *cr);
static void break_off_f(Carried *cr);
static void burst_piece(Carried *cr, uint16_t kind, int last);
static void camera_on(Carried *cr, uint16_t car, uint16_t y, uint16_t z, int by_x);
static void child_back(Carried *cr);
static int child_d1bf(Carried *cr);
static int child_fast(Carried *cr, uint32_t b);
static void child_flags(Carried *cr);
static void child_handler(Carried *cr, uint16_t h);
static uint16_t child_y(Carried *cr, uint16_t f5, uint16_t a);
static int cmp8n(Carried *cr, uint8_t v, uint8_t k);
static int count_down(Carried *cr);
static uint32_t db(Carried *cr);
static void director_entry(Carried *cr, unsigned n);
static void director_mode(Carried *cr, uint16_t v);
static void director_place(Carried *cr, uint16_t dx, uint16_t dy, uint16_t dz);
static uint16_t director_slope(Carried *cr, uint16_t a);
static void director_still(Carried *cr);
static void director_table_start(Carried *cr, uint32_t lead);
static void director_tail(Carried *cr);
static void director_track_start(Carried *cr);
static uint32_t dpx(Carried *cr, uint16_t off, uint16_t x);
static void lamp_drop_start(Carried *cr);
static void lamp_look_step(Carried *cr, uint16_t table);
static void lamp_place(Carried *cr);
static uint32_t latch(Carried *cr, uint16_t bit, int on, CarriedFn snd);
static uint16_t latch_addr(Carried *cr);
static void lda8(Carried *cr, uint8_t v);
static void offset_on_parent(Carried *cr, int sh);
static void offset_on_parent_ex(Carried *cr, int sh, int with14);
static void piece_drop(Carried *cr);
static void piece_fall4(Carried *cr);
static void piece_handler(Carried *cr, uint8_t end);
static void piece_look(Carried *cr, uint16_t look, int adv, uint8_t f5);
static uint16_t random_word(Carried *cr, uint16_t off, uint16_t add);
static void record_42_set(Carried *cr, uint16_t v);
static void record_place(Carried *cr, uint16_t x, uint16_t y, uint16_t z, uint16_t flags);
static void ring_spawn(Carried *cr, unsigned lead);
static void ring_start(Carried *cr, uint8_t h, uint16_t vx, uint16_t vy, uint16_t vz,
                       unsigned lead);
static void set_child(Carried *cr);
static void set_child_of_x(Carried *cr);
static void set_handler(Carried *cr, uint16_t obj, uint16_t h);
static uint16_t sext_word(Carried *cr, uint32_t at, uint32_t lead);
static void snd16(Carried *cr, uint16_t s);
static void spark(Carried *cr, uint16_t vx, uint16_t vz);
static int spawn(Carried *cr, uint16_t kind);
static void spray_from_y(Carried *cr);
static void spray_from_z(Carried *cr);
static void spray_turn(Carried *cr);
static void sta16_imm(Carried *cr, uint32_t addr, uint16_t v);
static void swap_xy(Carried *cr);
static void swap_xy_less(Carried *cr, unsigned less);
static void sync_36(Carried *cr);
static void w_ObjectPathCommand(Carried *cr);
static void w_ObjectPathStart(Carried *cr);
static void w_RandomStepLong(Carried *cr);
void CameraFollowStep_cr(Carried *cr);
void CameraSnapToCar(Carried *cr);
void CarCrashParts(Carried *cr);
void GsuRecordFlag4_cr(Carried *cr);
void GsuRecordNew422_cr(Carried *cr);
void GsuRecordNew426_cr(Carried *cr);
void GsuRecordPosition_cr(Carried *cr);
void GsuRecordSetFlags_cr(Carried *cr);
void GsuRecordVelocity_cr(Carried *cr);
void GsuRecordWord58_cr(Carried *cr);
void GsuRecordWord88_cr(Carried *cr);
void GsuTableEntryNext_cr(Carried *cr);
void GsuTableEntryTo1F4C_cr(Carried *cr);
void ObjectAdvance5DIfFlag(Carried *cr);
void ObjectApplyVelocity_cr(Carried *cr);
void ObjectBillboardTilt(Carried *cr);
void ObjectCarRecordNew(Carried *cr);
void ObjectCarSpawn(Carried *cr);
void ObjectCopy14From_cr(Carried *cr);
void ObjectCopyPositionFrom_cr(Carried *cr);
void ObjectCopyPositionTo_cr(Carried *cr);
void ObjectCountStep5F_cr(Carried *cr);
void ObjectFaceTied_cr(Carried *cr);
void ObjectNearFocus_cr(Carried *cr);
void ObjectPlaceOnParent(Carried *cr);
void ObjectPoseFromRecord_cr(Carried *cr);
void ObjectRebucket_cr(Carried *cr);
void ObjectReleaseLong(Carried *cr);
void ObjectReleaseY_cr(Carried *cr);
void ObjectRelease_cr(Carried *cr);
void ObjectSet5DByKind_cr(Carried *cr);
void ObjectSpawnAttached(Carried *cr);
void ObjectSpawnChanceD460(Carried *cr);
void ObjectSpawnD460UnlessFlag(Carried *cr);
void ObjectSpawnSparklePair(Carried *cr);
void ObjectSpawnTrail(Carried *cr);
void ObjectSpawn_cr(Carried *cr);
void ObjectSyncFromRecord_cr(Carried *cr);
void ObjectTurnFromRecord_cr(Carried *cr);
void ObjectVelocityFromRecordOf_cr(Carried *cr);
void ObjectVelocityFromRecordScaled_cr(Carried *cr);
void ObjectVelocityFromRecord_cr(Carried *cr);
void PlayerHoldSound32_cr(Carried *cr);
void PlayerHoldSound33_cr(Carried *cr);
void PlayerHoldSound34_cr(Carried *cr);
void PlayerHoldSound35_cr(Carried *cr);
void PlayerSound0B_cr(Carried *cr);
void PlayerSound0C_cr(Carried *cr);
void PlayerSound0D_cr(Carried *cr);
void PlayerSound12_cr(Carried *cr);
void PlayerSound23_cr(Carried *cr);
void PlayerSound24_cr(Carried *cr);
void PlayerStopHeldSound_cr(Carried *cr);
void RecordContactSummary_cr(Carried *cr);
void RotateXY_cr(Carried *cr);
void RotateXZFrom08_cr(Carried *cr);
void RotateXZ_cr(Carried *cr);
void SoundAtRateForLevel_cr(Carried *cr);
void SoundPort0QueuePush_cr(Carried *cr);

static uint32_t db(Carried *cr)
{
    return (uint32_t)cr->db << 16;
}

/* LDA #v with M 8-bit. */
static void lda8(Carried *cr, uint8_t v)
{
    cyc(30);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
}

/* REP #$20, LDA #v, STA addr, SEP #$20. */
static void sta16_imm(Carried *cr, uint32_t addr, uint16_t v)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16_imm(cr, v);
    cyc(10);
    wr16(addr, v);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

/* The handler pointer of object `obj`: +$1A = `h`, +$1C = $08. */
static void set_handler(Carried *cr, uint16_t obj, uint16_t h)
{
    uint32_t b = (cyc(28), db(cr));

    sta16_imm(cr, b | (uint16_t)(0x201A + obj), h);
    lda8(cr, 0x08);
    cyc(38);
    wr8(b | (uint16_t)(0x201C + obj), 0x08);
}

/* A new object of `kind` ($1F2B, ObjectSpawn): returns whether one was
   made (C), Y = it. */
static int spawn(Carried *cr, uint16_t kind)
{
    sta16_imm(cr, db(cr) | 0x1F2B, kind);
    (cyc(84), ObjectSpawn_cr(cr));
    cyc(22);
    return (cr->p & P_C) != 0;
}

/* PHX, TYX, PLY: X and Y swapped, N and Z from the pulled Y. */
static void swap_xy_less(Carried *cr, unsigned less)
{
    cyc(126 - less);
    uint16_t t = cr->x;

    cr->x = cr->y;
    cr->y = t;
    set_p(cr, nz16(cr->p, cr->y));
}

static void swap_xy(Carried *cr)
{
    swap_xy_less(cr, 0);
}

/* PHA, REP #$20, TYA, STA +$06,X, SEP #$20, PLA: the new object Y as
   object X's child. */
static void set_child(Carried *cr)
{
    uint8_t lo = (uint8_t)cr->a;

    wr16(db(cr) | (uint16_t)(0x2006 + cr->x), cyc_then(66, cr->y));
    cr->a = (uint16_t)((cr->y & 0xFF00) | lo);
    set_p(cr, nz8(cr->p, lo));
}

/* The record's position (direct page $02, $08, $94: X, Y, Z) set
   (GsuRecordPosition), then its flags from $02 (GsuRecordSetFlags). */
static void record_place(Carried *cr, uint16_t x, uint16_t y, uint16_t z, uint16_t flags)
{
    sta16_imm(cr, 0x000002, x);
    cyc(36);
    sta16_imm(cr, 0x000008, y);
    cyc(36);
    sta16_imm(cr, 0x000094, z);
    (cyc(84), GsuRecordPosition_cr(cr));
    cyc(14);
    sta16_imm(cr, 0x000002, flags);
    (cyc(84), GsuRecordSetFlags_cr(cr));
}

/* PHX, REP #$20, X = $70:2476 (the record): its +$42 = v; SEP #$20, PLX. */
static void record_42_set(Carried *cr, uint16_t v)
{
    uint16_t x = cr->x;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(84);
    r_lda16(cr, 0x702476);
    r_tax16(cr);
    r_lda16_imm(cr, v);
    cyc(2);
    wr16(0x700042 + cr->x, v);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->x = x;
    set_p(cr, nz16(cr->p, x));
}

/* ObjectSyncFromRecord, then +$36 = 3. */
static void sync_36(Carried *cr)
{
    (cyc(62), ObjectSyncFromRecord_cr(cr));
    lda8(cr, 0x03);
    wr8(db(cr) | (uint16_t)(0x2036 + cr->x), cyc_then(16, 0x03));
    cyc(44);
}

/* LDA +$48,X: whether the count has run out (0); otherwise it is counted
   down (DEC +$48,X). */
static int count_down(Carried *cr)
{
    uint32_t b = db(cr);
    cyc(30);
    uint8_t v = rd8(b | (uint16_t)(0x2048 + cr->x));

    lda8(cr, v);
    if (!v)
        return 1;
    cyc(22);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2048 + cr->x)) - 1);
    cyc(6);
    wr8(b | (uint16_t)(0x2048 + cr->x), v);
    set_p(cr, nz8(cr->p, v));
    return 0;
}

/* +off,X 8-bit = random & $3F, then the word there + add (16-bit, the
   high byte as it was). Returns the word. */
static uint16_t random_word(Carried *cr, uint16_t off, uint16_t add)
{
    uint32_t b = db(cr);
    uint16_t w;
    uint8_t v;

    (cyc(62), w_RandomStepLong(cr));
    v = (uint8_t)(cr->a & 0x3F);
    lda8(cr, v);
    cyc(16);
    wr8(b | (uint16_t)(off + cr->x), v);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    w = rd16(b | (uint16_t)(off + cr->x));
    cr->a = w;
    if (add & 0x8000)
        r_sub16(cr, (uint16_t)-add);
    else
        { cyc(6); r_add16(cr, add); }
    cyc(62);
    wr16(b | (uint16_t)(off + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    return cr->a;
}

/* One burst piece: a new object of `kind` with handler $08:ED39
   (ObjectBurstPieceStart) at object X's position; none when the spawn
   fails. */
static void burst_piece(Carried *cr, uint16_t kind, int last)
{
    if (!spawn(cr, kind))
        { cyc(last ? 70 : 48); return; }   /* BCS not taken, JML; the RTL after the last */
    set_handler(cr, cr->y, 0xED39);
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    cyc(22);
}

/*
 * @routine 08:EC9C ObjectBurstSpawn
 * WHAT IT DOES: Object X bursts into four pieces: new objects of kinds
 * $A5F0, $A60C, $A628 and $A644 at its position, each with handler $08:ED39
 * (ObjectBurstPieceStart).
 * HOW: ObjectSpawn and ObjectCopyPositionTo for each; a failed spawn skips
 * its piece.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; Y = the last piece
 * (or as ObjectSpawn left it); A and flags as the last call left them.
 */
void ObjectBurstSpawn(Carried *cr)
{
    cyc(22);
    burst_piece(cr, 0xA5F0, 0);
    burst_piece(cr, 0xA60C, 0);
    burst_piece(cr, 0xA628, 0);
    burst_piece(cr, 0xA644, 1);
}

void abi_ObjectBurstSpawn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBurstSpawn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:EDB6 ObjectBurstPieceStep
 * WHAT IT DOES: A burst piece's frame: synced from its Super FX record
 * (ObjectSyncFromRecord); released when that says so (C set), or after
 * the code at $08:D337 when the record's contact summary has bit 7 and
 * the sync's bit 0 is clear; otherwise released once its count +$48 runs
 * out, else counted down.
 * HOW: The releases are jumps (ObjectReleaseLong; $08:D337 as original
 * code).
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte the
 * count; as the release leaves it otherwise.
 */
void ObjectBurstPieceStep(Carried *cr)
{
    (cyc(62), ObjectSyncFromRecord_cr(cr));
    if (cr->p & P_C) {
        cyc(46);
        ObjectReleaseLong(cr);
        return;
    }
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if (!(cr->a & 0x01)) {
        cyc(86);
        RecordContactSummary_cr(cr);
        lda8(cr, (uint8_t)(cr->a & 0x80));
        if (cr->a & 0x80) {
            cyc(26);
            ObjectBreakOffContact(cr);
            return;
        } else { cyc(0); }
    } else { cyc(22); }
    cyc(8);
    if (count_down(cr))
        { cyc(18); ObjectReleaseLong(cr); } else { cyc(44); }
}

void abi_ObjectBurstPieceStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBurstPieceStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:ED39 ObjectBurstPieceStart
 * WHAT IT DOES: A burst piece's first frame: handler $08:EDB6
 * (ObjectBurstPieceStep), a random velocity (X and Z -$1F to +$20, Y up
 * -$1E to -$5D), count +$48 = $28, random +$15 and +$14, a Super FX record
 * (GsuRecordNew426) given the velocity (GsuRecordVelocity); then its first
 * step.
 * HOW: Each random part is RandomStepLong & $3F stored to the low byte and
 * then adjusted as a word (the high byte as it was). Falls into
 * ObjectBurstPieceStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectBurstPieceStep.
 */
void ObjectBurstPieceStart(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t w;

    set_handler(cr, cr->x, 0xEDB6);
    random_word(cr, 0x202E, (uint16_t)-0x001F);
    cyc(22);
    random_word(cr, 0x2030, 0x001E);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(74);
    w = (uint16_t)((rd16(b | (uint16_t)(0x2030 + cr->x)) ^ 0xFFFF) + 1);
    cr->a = w;
    set_p(cr, nz16(cr->p, w));
    cyc(68);
    wr16(b | (uint16_t)(0x2030 + cr->x), w);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(22);
    random_word(cr, 0x2032, (uint16_t)-0x001F);
    lda8(cr, 0x28);
    cyc(38);
    wr8(b | (uint16_t)(0x2048 + cr->x), 0x28);
    (cyc(62), w_RandomStepLong(cr));
    cyc(30);
    wr8(b | (uint16_t)(0x2015 + cr->x), (uint8_t)cr->a);
    (cyc(62), w_RandomStepLong(cr));
    cyc(30);
    wr8(b | (uint16_t)(0x2014 + cr->x), (uint8_t)cr->a);
    (cyc(62), GsuRecordNew426_cr(cr));
    (cyc(62), GsuRecordVelocity_cr(cr));
    ObjectBurstPieceStep(cr);
}

void abi_ObjectBurstPieceStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBurstPieceStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:EDDB ObjectHandlerEmptyA
 * WHAT IT DOES: Nothing: an empty object handler.
 * HOW: RTL. Not reached in the traces so far.
 * Entry: any. Exit: every register unchanged.
 */
/* ObjectHandlerEmptyA on carried registers: no register work. */
void ObjectHandlerEmptyA_cr(Carried *cr)
{
    (void)cr;
    cyc(44);
}

void abi_ObjectHandlerEmptyA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectHandlerEmptyA_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:EDDC ObjectHandlerEmptyB
 * WHAT IT DOES: Nothing: an empty object handler.
 * HOW: RTL. Not reached in the traces so far.
 * Entry: any. Exit: every register unchanged.
 */
/* ObjectHandlerEmptyB on carried registers: no register work. */
void ObjectHandlerEmptyB_cr(Carried *cr)
{
    (void)cr;
    cyc(44);
}

void abi_ObjectHandlerEmptyB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectHandlerEmptyB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The blinker's record: +$42 = $1000 (shown), synced, +$36 = 3. */
static void blinker_show(Carried *cr)
{
    record_42_set(cr, 0x1000);
    (cyc(58), sync_36(cr));
}

void abi_ObjectBlinkerFade(void);

/*
 * @routine 08:EE63 ObjectBlinkerRise
 * WHAT IT DOES: The blinker's first phase: while its count +$48 runs, its
 * record shown (+$42 = $1000) and synced (+$36 = 3); then count $28 and
 * handler $08:EE9F (ObjectBlinkerFade), run at once.
 * HOW: Falls into ObjectBlinkerFade.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as
 * ObjectSyncFromRecord leaves it, A low byte 3.
 */
void ObjectBlinkerRise(Carried *cr)
{
    uint32_t b = db(cr);

    if (!count_down(cr)) {
        blinker_show(cr);
        return;
    }
    lda8(cr, 0x28);
    cyc(34);
    wr8(b | (uint16_t)(0x2048 + cr->x), 0x28);
    set_handler(cr, cr->x, 0xEE9F);
    ObjectBlinkerFade(cr);
}

void abi_ObjectBlinkerRise(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBlinkerRise(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:EE9F ObjectBlinkerFade
 * WHAT IT DOES: The blinker's second phase: while its count +$48 runs, its
 * colour +$5A (low seven bits, 0-$0F) steps down one a frame with the
 * record shown, and at 0 the record gets +$42 = $1010; when the count runs
 * out, the record's bit 12 of +$42 is cleared and both its child (+$06,
 * ObjectReleaseY) and it are released.
 * HOW: The colour steps by ADC #$FF with a wrap to $10 kept in its low
 * seven bits, bit 7 set; the release is a jump (ObjectReleaseLong).
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the last call
 * leaves it.
 */
void ObjectBlinkerFade(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t x;
    uint8_t v, p;

    if (count_down(cr)) {
        x = cr->x;
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cyc(102);
        r_lda16(cr, 0x702476);
        r_tax16(cr);
        cyc(16);
        r_lda16(cr, 0x700042 + cr->x);
        r_and16(cr, 0xEFFF);
        cyc(54);
        wr16(0x700042 + cr->x, cr->a);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cr->x = x;
        set_p(cr, nz16(cr->p, x));
        cyc(88);
        cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
        set_p(cr, nz16(cr->p, cr->y));
        (cyc(62), ObjectReleaseY_cr(cr));
        (cyc(32), ObjectReleaseLong(cr));
        return;
    }
    cyc(30);
    v = rd8(b | (uint16_t)(0x205A + cr->x));
    lda8(cr, (uint8_t)(v & 0x7F));
    set_p(cr, (uint8_t)(cr->p | (P_C)));
    if (!(v & 0x7F)) {
        cyc(24);
        record_42_set(cr, 0x1010);
        (cyc(82), sync_36(cr));
        return;
    }
    cyc(48);
    v = rd8(b | (uint16_t)(0x205A + cr->x));
    p = (uint8_t)(cr->p & ~P_C);
    v = alu_adc8(v, 0xFF, &p);
    if (!(v & 0x80)) {
        cyc(48);
        p &= (uint8_t)~P_C;
        v = alu_adc8(v, 0x10, &p);
    }
    v &= 0x7F;
    if (v >= 0x10)
        v = (uint8_t)(v - 0x10);
    v |= 0x80;
    set_p(cr, p);
    lda8(cr, v);
    cyc(122);
    wr8(b | (uint16_t)(0x205A + cr->x), v);
    (cyc(24), blinker_show(cr));
}

void abi_ObjectBlinkerFade(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBlinkerFade(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:EDDD ObjectBlinkerStart
 * WHAT IT DOES: Makes object X a blinker: a child object of kind $A91C at
 * its position (its +$06, put in its bucket), handler $08:EE63
 * (ObjectBlinkerRise), count +$48 = $32, colour +$5A = $8F, +$20 =
 * $D354, a Super FX record (GsuRecordNew422) at $02D0, $0190, $02D0 with
 * flags $4000; then the first phase at once. With no record the handler
 * just returns ($08:A033).
 * HOW: Falls into ObjectBlinkerRise; a failed spawn skips the child.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectBlinkerRise.
 */
void ObjectBlinkerStart(Carried *cr)
{
    uint32_t b = (cyc(22), db(cr));

    if (spawn(cr, 0xA91C)) {
        cyc(62);
        ObjectCopyPositionTo_cr(cr);
        (cyc(22), set_child(cr));
        (cyc(66), swap_xy(cr));
        ObjectRebucket_cr(cr);
        swap_xy_less(cr, 94);
        cyc(48);
    } else { cyc(26); }
    set_handler(cr, cr->x, 0xEE63);
    lda8(cr, 0x32);
    cyc(16);
    wr8(b | (uint16_t)(0x2048 + cr->x), 0x32);
    lda8(cr, 0x8F);
    cyc(32);
    wr8(b | (uint16_t)(0x205A + cr->x), 0x8F);
    cyc(28);
    sta16_imm(cr, b | (uint16_t)(0x2020 + cr->x), 0xD354);
    (cyc(84), GsuRecordNew422_cr(cr));
    if (cr->p & P_N) {
        cyc(90);
        return;                     /* BRL to the RTL at $08:A033 */
        return;
    } else { cyc(36); }
    record_place(cr, 0x02D0, 0x0190, 0x02D0, 0x4000);
    ObjectBlinkerRise(cr);
}

void abi_ObjectBlinkerStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBlinkerStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:EF19 ObjectPaceCarStep
 * WHAT IT DOES: The Speed Trax pace car's frame: near the focus
 * (ObjectNearFocus), rebucketed, posed from its record
 * (ObjectPoseFromRecord). Until $1F71 is 2: once its record's Z ($70:0022)
 * + $258 is past $1F73, $1F71 = 1; its record's bit 11 of +$42 is set
 * while its speed ($70:004A) is below $1E, cleared otherwise; then the
 * code at $08:CD7E. At $1F71 = 2: the record's +$1C gets bit 4, its +$24
 * bits 5 and 3, its Y + $B1E0, and its handler cleared.
 * HOW: 16-bit record words through $70:0000 + its +$22; the code at
 * $08:CD7E is a JSL to GsuRecordFlag4 and an RTL, called directly here.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; Y = the object; as
 * the code at $08:CD7E leaves it, or A low byte 0 after the clear.
 */
void ObjectPaceCarStep(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t x, w;
    uint8_t v;

    (cyc(62), ObjectNearFocus_cr(cr));
    ObjectRebucket_cr(cr);
    (cyc(62), ObjectPoseFromRecord_cr(cr));
    x = cr->x;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->y = cr->x;
    cyc(96);
    r_lda16(cr, b | (uint16_t)(0x2022 + cr->x));
    r_tax16(cr);
    cyc(8);
    r_lda16(cr, b | 0x1F71);
    r_and16(cr, 0x00FF);
    r_cmp16(cr, 0x0002);
    if (cr->a != 0x0002) {
        cyc(94);
        r_lda16(cr, 0x700022 + cr->x);
        r_add16(cr, 0x0258);
        cyc(62);
        w = rd16(b | 0x1F73);
        r_cmp16(cr, w);
        if (!(cr->p & P_N)) {
            set_p(cr, (uint8_t)(cr->p | (P_M)));
            lda8(cr, 0x01);
            cyc(48);
            wr8(b | 0x1F71, 0x01);
            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        }
        cyc(54);
        r_lda16(cr, 0x700042 + cr->x);
        r_and16(cr, 0xF7FF);
        cyc(54);
        wr16(0x700042 + cr->x, cr->a);
        cyc(32);
        r_lda16(cr, 0x70004A + cr->x);
        r_cmp16(cr, 0x001E);
        if (cr->p & P_N) {
            cyc(72);
            r_lda16(cr, 0x700042 + cr->x);
            cr->a |= 0x0800;
            set_p(cr, nz16(cr->p, cr->a));
            cyc(56);
            wr16(0x700042 + cr->x, cr->a);
        } else { cyc(46); }
        cyc(144);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cr->x = x;
        set_p(cr, nz16(cr->p, x));
        GsuRecordFlag4_cr(cr);
        cyc(44);
        return;
    }
    cyc(100);
    r_lda16(cr, 0x70001C + cr->x);
    cr->a |= 0x0010;
    set_p(cr, nz16(cr->p, cr->a));
    cyc(56);
    wr16(0x70001C + cr->x, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->x = x;
    set_p(cr, nz16(cr->p, x));
    cyc(88);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) | 0x20);
    lda8(cr, v);
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), v);
    cyc(30);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) | 0x08);
    lda8(cr, v);
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), v);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    w = rd16(b | (uint16_t)(0x200E + cr->x));
    cr->a = w;
    r_add16(cr, 0xB1E0);
    cyc(68);
    wr16(b | (uint16_t)(0x200E + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(50);
    sta16_imm(cr, b | (uint16_t)(0x201A + cr->x), 0x0000);
    lda8(cr, 0x00);
    cyc(38);
    wr8(b | (uint16_t)(0x201C + cr->x), 0x00);
    cyc(44);
}

void abi_ObjectPaceCarStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPaceCarStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:EF06 ObjectPaceCarStart
 * WHAT IT DOES: The Speed Trax pace car's first frame: its car set up
 * (ObjectCarSpawn), handler $08:EF19 (ObjectPaceCarStep), run at once.
 * HOW: Falls into ObjectPaceCarStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectPaceCarStep.
 */
void ObjectPaceCarStart(Carried *cr)
{
    cyc(62);
    ObjectCarSpawn(cr);
    set_handler(cr, cr->x, 0xEF19);
    ObjectPaceCarStep(cr);
}

void abi_ObjectPaceCarStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPaceCarStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:F0B1 ObjectSignalStep
 * WHAT IT DOES: The pace car's signal: synced (+$36 = 3) each frame; once
 * $1F71 is set, its child (+$06) gets +$04 = $A794 and its light +$5A
 * (low seven bits, 0-7) steps up one a frame, and at 7 the object $0346
 * gets +$04 = $A75C, the child and the signal are released and $1F71 =
 * 2.
 * HOW: The light steps by ADC #1 with a wrap to 8 in its low seven bits,
 * bit 7 set.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the last call
 * leaves it.
 */
void ObjectSignalStep(Carried *cr)
{
    uint32_t b = db(cr);
    uint8_t v, p;

    cyc(24);
    r_lda8(cr, b | 0x1F71);
    if ((uint8_t)cr->a) {
        cyc(46);
        cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
        set_p(cr, nz16(cr->p, cr->y));
        cyc(28);
        sta16_imm(cr, b | (uint16_t)(0x2004 + cr->y), 0xA794);
        cyc(52);
        v = rd8(b | (uint16_t)(0x205A + cr->y));
        lda8(cr, (uint8_t)(v & 0x7F));
        if ((v & 0x7F) == 0x07) {
            cyc(48);
            cr->y = rd16(b | 0x0346);
            set_p(cr, nz16(cr->p, cr->y));
            cyc(28);
            sta16_imm(cr, b | (uint16_t)(0x2004 + cr->y), 0xA75C);
            cyc(52);
            cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
            set_p(cr, nz16(cr->p, cr->y));
            (cyc(62), ObjectReleaseY_cr(cr));
            (cyc(62), ObjectRelease_cr(cr));
            lda8(cr, 0x02);
            cyc(10);
            wr8(b | 0x1F71, 0x02);
            cyc(44);
            return;
        }
        cyc(48);
        v = rd8(b | (uint16_t)(0x205A + cr->y));
        p = (uint8_t)(cr->p & ~P_C);
        v = alu_adc8(v, 0x01, &p);
        if (!(v & 0x80)) {
            cyc(48);
            p &= (uint8_t)~P_C;
            v = alu_adc8(v, 0x08, &p);
        }
        v &= 0x7F;
        if (v >= 0x08)
            v = (uint8_t)(v - 0x08);
        v |= 0x80;
        set_p(cr, p);
        lda8(cr, v);
        cyc(122);
        wr8(b | (uint16_t)(0x205A + cr->y), v);
    } else { cyc(22); }
    sync_36(cr);
}

void abi_ObjectSignalStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSignalStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:EFAF ObjectPaceCarSetup
 * WHAT IT DOES: In Speed Trax (HUD mode 0; otherwise released): $1F73 =
 * its Z, $1F71 = 0, its Y - $64; a pace car (kind $A040, handler $08:EF06
 * ObjectPaceCarStart) at its position, Z - $DAC, Y 0, +$27 = the selection
 * handler $18DB; a signal (kind $A7B0) at its position, Y + $32, light
 * +$5A = $80, put in its bucket and made its child; then it becomes the
 * signal's driver: handler $08:F0B1 (ObjectSignalStep), +$20 = $C6DC, a
 * Super FX record (GsuRecordNew422) at $0640, $0640, $0640 with flags
 * $C100, count +$48 = $14, and its first step. A failed spawn or record
 * returns ($08:A033).
 * HOW: Falls into ObjectSignalStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSignalStep.
 */
void ObjectPaceCarSetup(Carried *cr)
{
    uint32_t b = db(cr);

    cyc(24);
    r_lda8(cr, b | 0x0D2B);
    if ((uint8_t)cr->a) {
        cyc(46);
        ObjectReleaseLong(cr);
        return;
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(74);
    r_lda16(cr, b | (uint16_t)(0x2010 + cr->x));
    cyc(24);
    wr16(b | 0x1F73, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    lda8(cr, 0x00);
    cyc(32);
    wr8(b | 0x1F71, 0x00);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, b | (uint16_t)(0x200E + cr->x));
    r_add16(cr, 0xFF9C);
    cyc(68);
    wr16(b | (uint16_t)(0x200E + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(44);
    if (!spawn(cr, 0xA040)) {
        cyc(70);                    /* BCS not taken, JML to the RTL at $08:A033, RTL */
        return;
    }
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, b | (uint16_t)(0x2010 + cr->y));
    r_add16(cr, 0xF254);
    cyc(68);
    wr16(b | (uint16_t)(0x2010 + cr->y), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(50);
    sta16_imm(cr, b | (uint16_t)(0x200E + cr->y), 0x0000);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(68);
    r_lda16(cr, b | 0x18DB);
    cyc(30);
    wr16(b | (uint16_t)(0x2027 + cr->y), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(22);
    set_handler(cr, cr->y, 0xEF06);
    cyc(22);
    if (!spawn(cr, 0xA7B0)) {
        cyc(70);
        return;
    }
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, b | (uint16_t)(0x200E + cr->y));
    r_add16(cr, 0x0032);
    cyc(68);
    wr16(b | (uint16_t)(0x200E + cr->y), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    lda8(cr, 0x80);
    cyc(54);
    wr8(b | (uint16_t)(0x205A + cr->y), 0x80);
    (cyc(16), swap_xy(cr));
    ObjectRebucket_cr(cr);
    (cyc(70), swap_xy_less(cr, 94));
    set_child(cr);
    cyc(50);
    set_handler(cr, cr->x, 0xF0B1);
    cyc(28);
    sta16_imm(cr, b | (uint16_t)(0x2020 + cr->x), 0xC6DC);
    (cyc(84), GsuRecordNew422_cr(cr));
    if (cr->p & P_N) {
        cyc(90);                    /* BPL not taken, JML to the RTL at $08:A033, RTL */
        return;
    } else { cyc(36); }
    record_place(cr, 0x0640, 0x0640, 0x0640, 0xC100);
    lda8(cr, 0x14);
    cyc(16);
    wr8(b | (uint16_t)(0x2048 + cr->x), 0x14);
    ObjectSignalStep(cr);
}

void abi_ObjectPaceCarSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPaceCarSetup(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ObjectTurnFromRecord(void);

/* |w| as the original takes it (EOR #$FFFF, INC A when negative). */
static uint16_t abs16(uint16_t w)
{
    return (w & 0x8000) ? (cyc(32), (uint16_t)((w ^ 0xFFFF) + 1)) : w;
}

/* With record X: whether |+$2E| + |+$32| (kept in $1F2B) is at least $64,
   signed (BPL after CMP #$0064). */
static int child_fast(Carried *cr, uint32_t b)
{
    uint16_t w;

    cyc(70);
    w = abs16(rd16(0x70002E + cr->x));
    cyc(46);
    wr16(b | 0x1F2B, w);
    cyc(32);
    w = abs16(rd16(0x700032 + cr->x));
    cyc(60);
    cr->a = (uint16_t)(w + rd16(b | 0x1F2B));
    r_cmp16(cr, 0x0064);
    cyc(24);
    return !(cr->p & P_N);
}

/* ObjectBreakOffChance with X = the record, Y = the child: whether a
   piece was made (C). */
static int child_d1bf(Carried *cr)
{
    (cyc(68), ObjectBreakOffChance(cr));
    return (cr->p & P_C) != 0;
}

/* The child Y's next handler `h` (bank $08). */
static void child_handler(Carried *cr, uint16_t h)
{
    cyc(16);
    set_handler(cr, cr->y, h);
}

/*
 * @routine 08:BD22 ObjectChildFollow
 * WHAT IT DOES: Keeps object X (a child of +$06) on its parent: rebucketed;
 * when the parent's record has flag $0400, its own record gets $0480 and
 * its +$24 bit 5, and nothing more. Otherwise its +$24 bit 5 and its
 * record's $0480 are cleared; unless the parent's +$24 has bit 6 or its
 * record's +$64 has neither bit 0 nor 1, and when its own record's +$46
 * has bits 0 and 4: by its record's +$2C (bit 7, bit 13, bit 11, bit 12,
 * bits 5-6) or, failing those, the parent's +$40 bit 11 or its +$46 bit 5,
 * a new handler for it ($D1F7, $D249, $D39F, $D2CF; $D33F or $D303 by
 * speed; $D3D3) when it moves fast enough (|+$2E| + |+$32| >= $64, where
 * asked) and the code at $08:D1BF allows it (C). Then its position is its
 * record's (+$1E + $4A, +$20 + $4C + $FFEC - its +$5F, +$22 + $4E) and
 * its turn (ObjectTurnFromRecord).
 * HOW: 16-bit record reads at $70:0000 + +$22; the object kept on the
 * stack (PHX) and in Y.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X = the object; Y
 * as the path leaves it.
 */
void ObjectChildFollow(Carried *cr)
{
    uint32_t b = (cyc(92), db(cr));
    uint16_t obj = cr->x, me, w, d;
    unsigned lead = 110;

    ObjectRebucket_cr(cr);
    me = cr->x;
    cr->y = me;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(66);
    cr->x = rd16(b | (uint16_t)(0x2006 + cr->x));
    cyc(44);
    cr->x = rd16(b | (uint16_t)(0x2022 + cr->x));
    cyc(46);
    r_lda16(cr, 0x700016 + cr->x);
    if (cr->a & 0x0400) {
        cyc(70);
        cr->x = rd16(b | (uint16_t)(0x2022 + cr->y));
        cyc(46);
        r_lda16(cr, 0x700016 + cr->x);
        cr->a |= 0x0480;
        set_p(cr, nz16(cr->p, cr->a));
        cyc(56);
        wr16(0x700016 + cr->x, cr->a);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(52);
        w = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->y)) | 0x20);
        lda8(cr, (uint8_t)w);
        cyc(16);
        wr8(b | (uint16_t)(0x2024 + cr->y), (uint8_t)w);
        cr->x = obj;
        set_p(cr, nz16(cr->p, obj));
        cyc(124);
        return;
    }
    cr->x = cr->y;
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(112);
    w = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) & 0xDF);
    lda8(cr, (uint8_t)w);
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)w);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(82);
    cr->x = rd16(b | (uint16_t)(0x2022 + cr->x));
    cyc(46);
    r_lda16(cr, 0x700016 + cr->x);
    r_and16(cr, 0xFB7F);
    cyc(54);
    wr16(0x700016 + cr->x, cr->a);
    cyc(44);
    cr->x = rd16(b | (uint16_t)(0x2006 + cr->y));
    cyc(44);
    r_lda16(cr, b | (uint16_t)(0x2024 + cr->x));
    if (cr->a & 0x0040)
        { cyc(48); goto place; }
    cyc(76);
    cr->x = rd16(b | (uint16_t)(0x2022 + cr->x));
    cyc(46);
    r_lda16(cr, 0x700040 + cr->x);
    cyc(24);
    wr16(b | 0x1F4C, cr->a);
    cyc(32);
    r_lda16(cr, 0x700064 + cr->x);
    if (!(cr->a & 0x0003))
        { cyc(48); goto place; }
    cyc(142);
    cr->x = rd16(b | (uint16_t)(0x2022 + me));
    cyc(46);
    r_lda16(cr, 0x700046 + cr->x);
    if (!(cr->a & 0x0001) || (cyc(46), !(cr->a & 0x0010)))
        { cyc(48); goto place; }
    cyc(78);
    d = rd16(0x70002C + cr->x);
    cr->a = d;
    if (d & 0x0080) {
        cyc(48);
        if (!child_d1bf(cr))
            { cyc(0); goto place; } else { cyc(0); }
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        child_handler(cr, cyc_then(28, child_fast(cr, b) ? 0xD33F : (cyc(24), 0xD303)));
        goto place;
    } else { cyc(78); }
    if (d & 0x2000) {
        if (child_fast(cr, b) && child_d1bf(cr))
            { cyc(6); child_handler(cr, 0xD2CF); } else { cyc(22); }
        cyc(2);
        goto place;
    } else { cyc(46); }
    if (d & 0x0800) {
        if (child_fast(cr, b) && child_d1bf(cr))
            { cyc(6); child_handler(cr, 0xD249); } else { cyc(22); }
        cyc(2);
        goto place;
    } else { cyc(46); }
    if (d & 0x1000) {
        if (child_fast(cr, b) && child_d1bf(cr))
            { cyc(6); child_handler(cr, 0xD1F7); } else { cyc(22); }
        cyc(2);
        goto place;
    }
    if (d & 0x0060) {
        cyc(46);
        if (child_fast(cr, b) && child_d1bf(cr))
            { cyc(6); child_handler(cr, 0xD39F); } else { cyc(22); }
        cyc(2);
        goto place;
    }
    cyc(84);
    r_lda16(cr, b | 0x1F4C);
    if (!(cr->a & 0x0800)) {
        cyc(78);
        r_lda16(cr, 0x700046 + cr->x);
        if (!(cr->a & 0x0020))
            { cyc(48); goto place; }
    }
    cyc(28);
    if ((cyc(20), child_d1bf(cr)))
        { cyc(0); child_handler(cr, 0xD3D3); lead = 88; /* falls into $BF53, no BRA */ } else { cyc(0); }
place:
    /* $08:BF53 */
    cr->y = me;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(lead);
    cr->x = rd16(b | (uint16_t)(0x2022 + cr->y));
    cyc(46);
    r_lda16(cr, 0x70001E + cr->x);
    cyc(46);
    r_add16(cr, rd16(0x70004A + cr->x));
    cyc(30);
    wr16(b | (uint16_t)(0x200C + cr->y), cr->a);
    cyc(30);
    r_lda16(cr, b | (uint16_t)(0x205F + cr->y));
    cr->a = (uint16_t)(((cr->a & 0x00FF) ^ 0xFFFF) + 1);
    r_add16(cr, 0xFFEC);
    cyc(146);
    r_add16(cr, rd16(0x700020 + cr->x));
    cyc(46);
    r_add16(cr, rd16(0x70004C + cr->x));
    cyc(30);
    wr16(b | (uint16_t)(0x200E + cr->y), cr->a);
    cyc(32);
    r_lda16(cr, 0x700022 + cr->x);
    cyc(46);
    r_add16(cr, rd16(0x70004E + cr->x));
    cyc(30);
    wr16(b | (uint16_t)(0x2010 + cr->y), cr->a);
    (cyc(46), ObjectTurnFromRecord_cr(cr));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->x = obj;
    set_p(cr, nz16(cr->p, obj));
    cyc(100);
}

void abi_ObjectChildFollow(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectChildFollow(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ObjectVelocityFromRecordOf(void);
void abi_ObjectSpawnChanceD460(void);
void abi_ObjectApplyVelocity(void);

/*
 * @routine 08:BA57 ObjectChildrenSpawn
 * WHAT IT DOES: Gives object X a child object for each record in its
 * record's chain (its record's +$3E, then each one's +$34, to 0), except
 * records with +$46 bit 9: the child's kind, +$29 and +$5F from the three
 * words at $09:8000 + the record's +$12; with a nonzero +$29 the child
 * gets +$24 bit 3, +$3A = 0, +$5F and +$29 (+ 2 for two players);
 * handler $08:BB16 (ObjectChildStart), +$22 = the record, +$06 = object
 * X. With no record of its own it just returns ($08:A033).
 * HOW: $1F50 keeps the parent; $1F4C, $1F52, $1F4E the table's words; a
 * failed spawn skips the record.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X = the object, N
 * and Z from it (PLX); Y the last child (or as ObjectSpawn left it).
 */
void ObjectChildrenSpawn(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t obj = cr->x, rec, t;

    cyc(24);
    wr16(b | 0x1F50, obj);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, b | (uint16_t)(0x2022 + obj));
    if (!cr->a) {
        cyc(90);
        return;                     /* JMP to the RTL at $08:A033 */
        return;
    }
    cr->x = cr->a;
    cyc(98);
    r_lda16(cr, 0x70003E + cr->x);
    for (;;) {
        cyc(14);
        r_tax16(cr);
        if (!cr->x)
            { cyc(118); break; } else { cyc(24); }
        rec = cr->x;
        r_lda16(cr, 0x700046 + rec);
        if (!(cr->a & 0x0200)) {
            cyc(108);
            t = rd16(0x700012 + rec);
            cyc(86);
            wr16(b | 0x1F4C, rd16(0x098000 + t));
            cyc(72);
            wr16(b | 0x1F52, rd16(0x098002 + t));
            cyc(72);
            r_lda16(cr, 0x098004 + t);
            wr16(b | 0x1F4E, cr->a);
            cr->x = rec;
            cyc(104);
            r_lda16(cr, b | 0x1F4C);
            cyc(24);
            wr16(b | 0x1F2B, cr->a);
            set_p(cr, (uint8_t)(cr->p | (P_M)));
            (cyc(84), ObjectSpawn_cr(cr));
            if (cr->p & P_C) {
                cyc(46);
                r_lda8(cr, b | 0x1F52);
                cyc(24);
                cr->a |= rd8(b | 0x1F53);
                set_p(cr, nz8(cr->p, (uint8_t)cr->a));
                if ((uint8_t)cr->a) {
                    cyc(46);
                    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->y)) | 0x08));
                    cyc(16);
                    wr8(b | (uint16_t)(0x2024 + cr->y), (uint8_t)cr->a);
                    lda8(cr, 0x00);
                    cyc(16);
                    wr8(b | (uint16_t)(0x203A + cr->y), 0x00);
                    cyc(24);
                    r_lda8(cr, b | 0x1F4E);
                    cyc(30);
                    wr8(b | (uint16_t)(0x205F + cr->y), (uint8_t)cr->a);
                    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
                    cyc(46);
                    r_lda16(cr, b | 0x1F52);
                    cyc(30);
                    wr16(b | (uint16_t)(0x2029 + cr->y), cr->a);
                    set_p(cr, (uint8_t)(cr->p | (P_M)));
                    cyc(46);
                    r_lda8(cr, b | 0x0D62);
                    if ((uint8_t)cr->a) {
                        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
                        cyc(68);
                        r_lda16(cr, b | (uint16_t)(0x2029 + cr->y));
                        r_add16(cr, 0x0002);
                        cyc(68);
                        wr16(b | (uint16_t)(0x2029 + cr->y), cr->a);
                        set_p(cr, (uint8_t)(cr->p | (P_M)));
                    }
                }
                cyc(22);
                set_handler(cr, cr->y, 0xBB16);
                cyc(88);
                wr16(b | (uint16_t)(0x2022 + cr->y), cr->x);
                set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
                cyc(96);
                r_lda16(cr, b | 0x1F50);
                cyc(30);
                wr16(b | (uint16_t)(0x2006 + cr->y), cr->a);
                set_p(cr, (uint8_t)(cr->p | (P_M)));
            } else { cyc(26); }
        } else { cyc(48); }
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cyc(76);
        r_lda16(cr, 0x700034 + cr->x);
        cyc(24);
    }
    cr->x = obj;
    set_p(cr, (uint8_t)(nz16(cr->p, obj) | P_M));
}

void abi_ObjectChildrenSpawn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectChildrenSpawn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* $08:BB4E: object X's +$24 bit 6 cleared, then its bit 5 set when its
   parent's +$24 has bit 3, or its bit 6 when the parent's has bit 6;
   +$36 = 3. */
static void child_flags(Carried *cr)
{
    uint32_t b = db(cr);
    uint8_t v, pv;

    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    cyc(30);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) & 0xBF);
    lda8(cr, v);
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), v);
    cyc(30);
    pv = rd8(b | (uint16_t)(0x2024 + cr->y));
    lda8(cr, pv);
    if (pv & 0x08) {
        cyc(38);
        v = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) | 0x20);
        lda8(cr, v);
        cyc(16);
        wr8(b | (uint16_t)(0x2024 + cr->x), v);
    } else if (pv & 0x40) {
        cyc(64);
        v = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) | 0x40);
        lda8(cr, v);
        cyc(16);
        wr8(b | (uint16_t)(0x2024 + cr->x), v);
        cyc(22);                        /* BRA */
    } else {
        cyc(40);                        /* neither bit: the branches over both */
    }
    lda8(cr, 0x03);
    cyc(16);
    wr8(b | (uint16_t)(0x2036 + cr->x), 0x03);
    cyc(44);
}

/*
 * @routine 08:BB2E ObjectChildStep
 * WHAT IT DOES: A child object's frame on its parent: follows it
 * (ObjectChildFollow); when the parent's +$25 has bit 2 its handler
 * becomes $08:BB7C (ObjectChildLoose, next frame); then its +$24 flags
 * from the parent's and +$36 = 3.
 * HOW: Byte tests through the data bank.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3; Y =
 * the parent.
 */
void ObjectChildStep(Carried *cr)
{
    uint32_t b = (cyc(46), db(cr));

    ObjectChildFollow(cr);
    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->y)) & 0x04));
    if ((uint8_t)cr->a)
        { cyc(8); set_handler(cr, cr->x, 0xBB7C); } else { cyc(34); }
    child_flags(cr);
}

void abi_ObjectChildStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectChildStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:BB16 ObjectChildStart
 * WHAT IT DOES: A child object's first frame: handler $08:BB2E
 * (ObjectChildStep), its +$26 from its parent's; then its first step.
 * HOW: Falls into ObjectChildStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectChildStep.
 */
void ObjectChildStart(Carried *cr)
{
    uint32_t b = db(cr);

    set_handler(cr, cr->x, 0xBB2E);
    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2026 + cr->y));
    cyc(30);
    wr8(b | (uint16_t)(0x2026 + cr->x), (uint8_t)cr->a);
    ObjectChildStep(cr);
}

void abi_ObjectChildStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectChildStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* $08:BD06: back on the parent: +$22 = +$27 (its record again), handler
   $08:BB2E (ObjectChildStep), then the flags. */
static void child_back(Carried *cr)
{
    uint32_t b = db(cr);

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(60);
    r_lda16(cr, b | (uint16_t)(0x2027 + cr->x));
    cyc(30);
    wr16(b | (uint16_t)(0x2022 + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(22);
    set_handler(cr, cr->x, 0xBB2E);
    (cyc(24), child_flags(cr));
}

/* LDA +$5F (low byte), negated, + $FFEC, + a: a Y offset below the
   parent by the child's +$5F and $14. */
static uint16_t child_y(Carried *cr, uint16_t f5, uint16_t a)
{
    cr->a = (uint16_t)(((f5 & 0x00FF) ^ 0xFFFF) + 1);
    r_add16(cr, 0xFFEC);
    r_add16(cr, a);
    return cr->a;
}

/*
 * @routine 08:BC73 ObjectChildReturn
 * WHAT IT DOES: A loose child going back: its velocity from its parent's
 * record (ObjectVelocityFromRecordOf); its position a step (a quarter, at
 * least 4: ApproachQuarterMin4) toward its place on the parent's record
 * (+$1E + $4A, +$20 + $4C + $FFEC - its +$5F, +$22 + $4E, its turn by
 * ObjectTurnFromRecord), moved (ObjectApplyVelocity) and rebucketed; once
 * the parent's +$25 bit 2 is clear it is back (+$22 = +$27, handler
 * $08:BB2E). Then its +$24 flags and +$36 = 3.
 * HOW: The target kept in direct page $04, $0A and $05F2.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
 */
void ObjectChildReturn(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t obj, w;

    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    (cyc(62), ObjectVelocityFromRecordOf_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    obj = cr->x;
    cyc(82);
    r_lda16(cr, b | (uint16_t)(0x2027 + cr->x));
    cr->y = cr->x;
    r_tax16(cr);
    cyc(30);
    r_lda16(cr, 0x70001E + cr->x);
    cyc(46);
    r_add16(cr, rd16(0x70004A + cr->x));
    cyc(16);
    wr16(0x000004, cr->a);
    cyc(30);
    w = rd16(b | (uint16_t)(0x205F + cr->y));
    cyc(146);
    child_y(cr, w, rd16(0x700020 + cr->x));
    cyc(46);
    r_add16(cr, rd16(0x70004C + cr->x));
    cyc(16);
    wr16(0x00000A, cr->a);
    cyc(32);
    r_lda16(cr, 0x700022 + cr->x);
    cyc(46);
    r_add16(cr, rd16(0x70004E + cr->x));
    cyc(24);
    wr16(b | 0x05F2, cr->a);
    (cyc(46), ObjectTurnFromRecord_cr(cr));
    cr->x = obj;
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(88);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    wr16(0x00003E, cyc_then(16, rd16(b | (uint16_t)(0x200C + cr->x))));
    cyc(16);
    r_lda16(cr, 0x000004);
    r_approach(cr, ApproachQuarterMin4);
    cyc(30);
    wr16(b | (uint16_t)(0x200C + cr->x), cr->a);
    cyc(74);
    wr16(0x00003E, cyc_then(16, rd16(b | (uint16_t)(0x200E + cr->x))));
    cyc(16);
    r_lda16(cr, 0x00000A);
    r_approach(cr, ApproachQuarterMin4);
    cyc(30);
    wr16(b | (uint16_t)(0x200E + cr->x), cr->a);
    cyc(74);
    wr16(0x00003E, cyc_then(16, rd16(b | (uint16_t)(0x2010 + cr->x))));
    cyc(24);
    r_lda16(cr, b | 0x05F2);
    r_approach(cr, ApproachQuarterMin4);
    cyc(30);
    wr16(b | (uint16_t)(0x2010 + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(84), ObjectApplyVelocity_cr(cr));
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->y)) & 0x04));
    if (!(uint8_t)cr->a) {
        child_back(cr);
        return;
    } else { cyc(34); }
    child_flags(cr);
}

void abi_ObjectChildReturn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectChildReturn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:BBE2 ObjectChildFlying
 * WHAT IT DOES: A loose child's flight: synced from its record
 * (ObjectSyncFromRecord); with the parent's +$25 bit 4 and a contact
 * (RecordContactSummary bit 7, the sync's bit 0 clear) the code at
 * $08:D337. While its count +$4B runs it drops ($14 + its +$5F lower)
 * and may spawn (ObjectSpawnChanceD460); at 0 with the parent's +$25 bit
 * 4 it is released if the parent's record +$64 has bit 4, else may spawn;
 * at 0 otherwise it goes back (handler $08:BC73, ObjectChildReturn, at
 * once). Then its +$24 flags and +$36 = 3.
 * HOW: Byte tests through the data bank; the release is a jump.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the path
 * leaves it.
 */
void ObjectChildFlying(Carried *cr)
{
    uint32_t b = (cyc(62), db(cr));
    uint16_t x;
    uint8_t v;

    ObjectSyncFromRecord_cr(cr);
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if (!(uint8_t)cr->a) {
        cyc(32);
        cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
        set_p(cr, nz16(cr->p, cr->y));
        cyc(30);
        lda8(cr, cyc_then(26, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->y)) & 0x10)));
        if ((uint8_t)cr->a) {
            cyc(44);
            RecordContactSummary_cr(cr);
            lda8(cr, (uint8_t)(cr->a & 0x80));
            if ((uint8_t)cr->a) {
                cyc(26);
                ObjectBreakOffContact(cr);
                return;
            } else { cyc(0); }
        }
    }
    cyc(38);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->y)) & 0x10));
    if ((uint8_t)cr->a) {
        cyc(38);
        r_lda8(cr, b | (uint16_t)(0x204B + cr->x));
        if (!(uint8_t)cr->a) {
            x = cr->x;
            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
            cyc(130);
            cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
            cyc(30);
            cr->x = rd16(b | (uint16_t)(0x2022 + cr->y));
            cyc(46);
            r_lda16(cr, 0x700064 + cr->x);
            cr->x = x;
            r_and16(cr, 0x0010);
            set_p(cr, (uint8_t)(cr->p | (P_M)));
            if (cr->a) {
                cyc(126);
                ObjectReleaseLong(cr);
                return;
            } else { cyc(148); }
            ObjectSpawnChanceD460(cr);
            (cyc(114), child_flags(cr));
            return;
        }
    } else { cyc(12); }
    cyc(52);
    r_lda8(cr, b | (uint16_t)(0x204B + cr->x));
    if (!(uint8_t)cr->a) {
        cyc(48);
        set_handler(cr, cr->x, 0xBC73);
        ObjectChildReturn(cr);
        return;
    }
    cyc(52);
    v = (uint8_t)(rd8(b | (uint16_t)(0x204B + cr->x)) - 1);
    cyc(6);
    wr8(b | (uint16_t)(0x204B + cr->x), v);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, b | (uint16_t)(0x205F + cr->x));
    cyc(144);
    child_y(cr, cr->a, rd16(b | (uint16_t)(0x200E + cr->x)));
    cyc(30);
    wr16(b | (uint16_t)(0x200E + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(68), ObjectSpawnChanceD460(cr));
    (cyc(114), child_flags(cr));
}

void abi_ObjectChildFlying(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectChildFlying(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:BB7C ObjectChildLoose
 * WHAT IT DOES: A child comes loose from its parent: handler $08:BBE2
 * (ObjectChildFlying), +$27 = its record (+$22), a record of its own
 * (GsuRecordNew426) with the parent's record's velocity
 * (ObjectVelocityFromRecordOf), then X and Z halved (+$36, +$3A of the
 * old record, arithmetic shift) and Y $FFCE (GsuRecordVelocity); count
 * +$4B = random 0-$0F + $0A; then its first flight.
 * HOW: Falls into ObjectChildFlying; the halving is CMP #$8000, ROR A.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectChildFlying.
 */
void ObjectChildLoose(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t x, w;
    uint8_t v;

    set_handler(cr, cr->x, 0xBBE2);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, b | (uint16_t)(0x2022 + cr->x));
    cyc(30);
    wr16(b | (uint16_t)(0x2027 + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(84), GsuRecordNew426_cr(cr));
    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    (cyc(62), ObjectVelocityFromRecordOf_cr(cr));
    if (cr->p & P_N) {
        child_back(cr);
        return;
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    x = cr->x;
    cr->y = cr->x;
    cyc(118);
    cr->x = rd16(b | (uint16_t)(0x2027 + cr->x));
    cyc(46);
    w = rd16(0x700036 + cr->x);
    cyc(68);
    wr16(b | (uint16_t)(0x202E + cr->y), (uint16_t)((w >> 1) | (w & 0x8000)));
    cyc(54);
    wr16(b | (uint16_t)(0x2030 + cr->y), 0xFFCE);
    cyc(32);
    w = rd16(0x70003A + cr->x);
    cr->a = (uint16_t)((w >> 1) | (w & 0x8000));
    set_p(cr, (uint8_t)(nz16(cr->p, cr->a) & ~P_C) | (w & 1));
    cyc(68);
    wr16(b | (uint16_t)(0x2032 + cr->y), cr->a);
    cr->x = x;
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(120), GsuRecordVelocity_cr(cr));
    (cyc(62), w_RandomStepLong(cr));
    v = (uint8_t)(cr->a & 0x0F);
    lda8(cr, v);
    cyc(16);
    wr8(b | (uint16_t)(0x204B + cr->x), v);
    cyc(30);
    v = (uint8_t)(rd8(b | (uint16_t)(0x204B + cr->x)) + 0x0A);
    lda8(cr, v);
    cyc(30);
    wr8(b | (uint16_t)(0x204B + cr->x), v);
    ObjectChildFlying(cr);
}

void abi_ObjectChildLoose(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectChildLoose(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:BBA2 ObjectChildLooseBack
 * WHAT IT DOES: ObjectChildLoose's way out when the parent's record
 * velocity leaves N set: the child back on its parent as ObjectChildBack
 * ($08:BD06). ObjectVelocityFromRecordOf leaves N from the object index,
 * which is never negative, so only a direct call reaches it.
 * HOW: BRL to ObjectChildBack's code.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectChildBack.
 */
void ObjectChildLooseBack(Carried *cr)
{
    (cyc(22), child_back(cr));
}

void abi_ObjectChildLooseBack(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectChildLooseBack(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ObjectVelocityFromRecord(void);
void abi_ObjectVelocityFromRecordScaled(void);
void abi_ObjectAdvance5DIfFlag(void);
void abi_RotateXZFrom08(void);
void abi_RotateXZ(void);

/*
 * @routine 08:D1BF ObjectBreakOffChance
 * WHAT IT DOES: For child Y breaking off: on a random byte of $7F or more,
 * a new object (kind $A1AC) at Y's position with Y's parent (+$06) as its
 * +$27; C set when one was made, clear otherwise.
 * HOW: X kept (PHX, PLX); ObjectSpawn and ObjectCopyPositionTo from Y
 * (TYX).
 * Entry: X 16-bit; Y = the child. Exit: M 8-bit; X kept; C as above; Y =
 * the new object when there is one.
 */
void ObjectBreakOffChance(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t x = cr->x;

    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->x = cr->y;
    set_p(cr, nz16(cr->p, cr->x));
    (cyc(128), w_RandomStepLong(cr));
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)((uint8_t)cr->a - 0x7F)) & ~P_C) | ((uint8_t)cr->a >= 0x7F ? (cyc(60), P_C) : (cyc(38), 0))));
    if ((uint8_t)cr->a >= 0x7F && spawn(cr, 0xA1AC)) {
        cyc(62);
        ObjectCopyPositionTo_cr(cr);
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cyc(52);
        r_lda16(cr, b | (uint16_t)(0x2006 + cr->x));
        cyc(30);
        wr16(b | (uint16_t)(0x2027 + cr->y), cr->a);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cr->x = x;
        set_p(cr, nz16(cr->p, x) | P_C);
        cyc(114);
        return;
    } else { cyc(118); }
    cr->x = x;
    set_p(cr, (uint8_t)(nz16(cr->p, x) & ~P_C));
}

void abi_ObjectBreakOffChance(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakOffChance(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D549 ObjectPieceFall
 * WHAT IT DOES: A broken-off piece's frame: rebucketed; its +$18 and +$19
 * = its frame count +$48 with bit 7; released once the count reaches its
 * end +$49, otherwise counted up and moved (ObjectApplyVelocity); +$36 =
 * 3.
 * HOW: $1F58 and $1F59 hold the count and the end.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3, or as
 * ObjectReleaseLong leaves it.
 */
void ObjectPieceFall(Carried *cr)
{
    uint32_t b = (cyc(62), db(cr));
    uint8_t v;

    ObjectRebucket_cr(cr);
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2048 + cr->x));
    cyc(24);
    wr8(b | 0x1F58, (uint8_t)cr->a);
    cyc(24);
    v = (uint8_t)(rd8(b | 0x1F58) | 0x80);
    lda8(cr, v);
    cyc(16);
    wr8(b | (uint16_t)(0x2018 + cr->x), v);
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2018 + cr->x));
    cyc(30);
    wr8(b | (uint16_t)(0x2019 + cr->x), (uint8_t)cr->a);
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2049 + cr->x));
    cyc(24);
    wr8(b | 0x1F59, (uint8_t)cr->a);
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2048 + cr->x));
    cyc(24);
    v = rd8(b | 0x1F59);
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)((uint8_t)cr->a - v)) & ~P_C) | ((uint8_t)cr->a >= v ? (cyc(48), P_C) : 0)));
    if ((uint8_t)cr->a == v) {
        ObjectReleaseLong(cr);
        return;
    }
    cyc(52);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2048 + cr->x)) + 1);
    cyc(6);
    wr8(b | (uint16_t)(0x2048 + cr->x), v);
    set_p(cr, nz8(cr->p, v));
    (cyc(62), ObjectApplyVelocity_cr(cr));
    lda8(cr, 0x03);
    cyc(16);
    wr8(b | (uint16_t)(0x2036 + cr->x), 0x03);
    cyc(44);
}

void abi_ObjectPieceFall(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPieceFall(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Handler $08:D549 (ObjectPieceFall), count +$48 = 0, end +$49. */
static void piece_handler(Carried *cr, uint8_t end)
{
    uint32_t b = db(cr);

    set_handler(cr, cr->x, 0xD549);
    lda8(cr, 0x00);
    cyc(16);
    wr8(b | (uint16_t)(0x2048 + cr->x), 0x00);
    if (end == 0x06)
        return;
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->x)) | 0x01));
    cyc(16);
    wr8(b | (uint16_t)(0x2025 + cr->x), (uint8_t)cr->a);
    lda8(cr, end);
    cyc(16);
    wr8(b | (uint16_t)(0x2049 + cr->x), end);
}

/* The break-off look: +$5D = `look`, ObjectAdvance5DIfFlag when `adv`,
   +$24 bit 3, +$3A = 0, +$5F = `f5`. */
static void piece_look(Carried *cr, uint16_t look, int adv, uint8_t f5)
{
    uint32_t b = (cyc(28), db(cr));

    sta16_imm(cr, b | (uint16_t)(0x205D + cr->x), look);
    if (adv)
        { cyc(68); ObjectAdvance5DIfFlag(cr); } else { cyc(22); }
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) | 0x08));
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
    lda8(cr, 0x00);
    cyc(16);
    wr8(b | (uint16_t)(0x203A + cr->x), 0x00);
    lda8(cr, f5);
    cyc(16);
    wr8(b | (uint16_t)(0x205F + cr->x), f5);
}

/* +$0E (Y) + $28. */
static void piece_drop(Carried *cr)
{
    uint32_t b = db(cr);

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, b | (uint16_t)(0x200E + cr->x));
    r_add16(cr, 0x0028);
    cyc(68);
    wr16(b | (uint16_t)(0x200E + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(22);
}

/* $08:D403: handler ObjectPieceFall, count 0, end 4, run at once. */
static void piece_fall4(Carried *cr)
{
    cyc(0);
    piece_handler(cr, 0x04);
    (cyc(32), ObjectPieceFall(cr));
}

/*
 * @routine 08:D1F7 ObjectBreakOffA
 * WHAT IT DOES: A child broken off (look $998D): +$24 bit 3, +$3A = 0,
 * +$5F = $14, its parent's record velocity scaled
 * (ObjectVelocityFromRecordScaled), $28 lower; then falls
 * (ObjectPieceFall, 4 frames).
 * HOW: Ends in ObjectPieceFall with +$25 bit 0 set.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectPieceFall.
 */
void ObjectBreakOffA(Carried *cr)
{
    piece_look(cr, 0x998D, 0, 0x14);
    (cyc(46), ObjectVelocityFromRecordScaled_cr(cr));
    (cyc(0), piece_drop(cr));
    piece_fall4(cr);
}

void abi_ObjectBreakOffA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakOffA(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D249 ObjectBreakOffB
 * WHAT IT DOES: As ObjectBreakOffA with look $9B96, the record's velocity
 * unscaled (ObjectVelocityFromRecord) and 3 frames.
 * HOW: As ObjectBreakOffA. Not reached in the traces so far.
 * Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
 */
void ObjectBreakOffB(Carried *cr)
{
    piece_look(cr, 0x9B96, 0, 0x14);
    (cyc(46), ObjectVelocityFromRecord_cr(cr));
    piece_drop(cr);
    piece_handler(cr, 0x03);
    (cyc(32), ObjectPieceFall(cr));
}

void abi_ObjectBreakOffB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakOffB(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D29B ObjectBreakOffC
 * WHAT IT DOES: As ObjectBreakOffB with look $9B9F (ObjectAdvance5DIfFlag)
 * and 4 frames.
 * HOW: As ObjectBreakOffA. Not reached in the traces so far.
 * Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
 */
void ObjectBreakOffC(Carried *cr)
{
    piece_look(cr, 0x9B9F, 1, 0x14);
    (cyc(46), ObjectVelocityFromRecord_cr(cr));
    piece_drop(cr);
    (cyc(32), piece_fall4(cr));
}

void abi_ObjectBreakOffC(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakOffC(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D2CF ObjectBreakOffD
 * WHAT IT DOES: As ObjectBreakOffC with look $9B8F.
 * HOW: As ObjectBreakOffA.
 * Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
 */
void ObjectBreakOffD(Carried *cr)
{
    piece_look(cr, 0x9B8F, 1, 0x14);
    (cyc(46), ObjectVelocityFromRecord_cr(cr));
    (cyc(0), piece_drop(cr));
    (cyc(32), piece_fall4(cr));
}

void abi_ObjectBreakOffD(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakOffD(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D303 ObjectBreakOffE
 * WHAT IT DOES: As ObjectBreakOffC with look $9B79.
 * HOW: As ObjectBreakOffA.
 * Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
 */
void ObjectBreakOffE(Carried *cr)
{
    piece_look(cr, 0x9B79, 1, 0x14);
    (cyc(46), ObjectVelocityFromRecord_cr(cr));
    (cyc(0), piece_drop(cr));
    (cyc(32), piece_fall4(cr));
}

void abi_ObjectBreakOffE(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakOffE(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* $08:D36D: look $9B63 (advanced), +$5F = $1F58, $28 lower, falls. */
static void break_off_f(Carried *cr)
{
    uint32_t b = (cyc(28), db(cr));

    sta16_imm(cr, b | (uint16_t)(0x205D + cr->x), 0x9B63);
    (cyc(68), ObjectAdvance5DIfFlag(cr));
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) | 0x08));
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
    lda8(cr, 0x00);
    cyc(16);
    wr8(b | (uint16_t)(0x203A + cr->x), 0x00);
    cyc(24);
    r_lda8(cr, b | 0x1F58);
    cyc(30);
    wr8(b | (uint16_t)(0x205F + cr->x), (uint8_t)cr->a);
    piece_drop(cr);
    (cyc(32), piece_fall4(cr));
}

/*
 * @routine 08:D337 ObjectBreakOffContact
 * WHAT IT DOES: A piece hit on contact: look $9B63 with +$5F = $6E, $28
 * lower, falls (4 frames).
 * HOW: $1F58 = $6E, then the tail of ObjectBreakOffF.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectPieceFall.
 */
void ObjectBreakOffContact(Carried *cr)
{
    lda8(cr, 0x6E);
    wr8(db(cr) | 0x1F58, cyc_then(10, 0x6E));
    (cyc(24), break_off_f(cr));
}

void abi_ObjectBreakOffContact(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakOffContact(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D33F ObjectBreakOffF
 * WHAT IT DOES: As ObjectBreakOffC with look $9B63 and +$5F = half the
 * speed of its parent's (+$27) record ($4A, its size) + $14, at most $78.
 * HOW: The halving is CMP #$8000, ROR A; $1F58 holds +$5F.
 * Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
 */
void ObjectBreakOffF(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t x = cr->x, w;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(82);
    cr->x = rd16(b | (uint16_t)(0x2027 + cr->x));
    cyc(44);
    cr->x = rd16(b | (uint16_t)(0x2022 + cr->x));
    cyc(46);
    w = abs16(cyc_then(4, rd16(0x70004A + cr->x)));
    cr->a = (uint16_t)((w >> 1) | (w & 0x8000));
    r_add16(cr, 0x0014);
    r_cmp16(cr, 0x0078);
    if (!(cr->p & P_N))
        r_lda16_imm(cr, 0x0078); else { cyc(20); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(166);
    wr8(b | 0x1F58, (uint8_t)cr->a);
    cr->x = x;
    set_p(cr, nz16(cr->p, x));
    (cyc(82), ObjectVelocityFromRecord_cr(cr));
    (cyc(0), break_off_f(cr));
}

void abi_ObjectBreakOffF(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakOffF(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D39F ObjectBreakOffG
 * WHAT IT DOES: As ObjectBreakOffD with look $9B56, $28 lower before the
 * scaled velocity (ObjectVelocityFromRecordScaled).
 * HOW: As ObjectBreakOffA.
 * Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
 */
void ObjectBreakOffG(Carried *cr)
{
    piece_look(cr, 0x9B56, 1, 0x14);
    piece_drop(cr);
    (cyc(46), ObjectVelocityFromRecordScaled_cr(cr));
    (cyc(32), piece_fall4(cr));
}

void abi_ObjectBreakOffG(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakOffG(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D3D3 ObjectBreakOffH
 * WHAT IT DOES: As ObjectBreakOffG with look $9B40 and +$5F = $0A.
 * HOW: As ObjectBreakOffA.
 * Entry: as ObjectBreakOffA. Exit: as ObjectBreakOffA.
 */
void ObjectBreakOffH(Carried *cr)
{
    piece_look(cr, 0x9B40, 1, 0x0A);
    piece_drop(cr);
    (cyc(46), ObjectVelocityFromRecordScaled_cr(cr));
    (cyc(0), piece_fall4(cr));
}

void abi_ObjectBreakOffH(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakOffH(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D428 ObjectPieceFallStart
 * WHAT IT DOES: A piece's first frame: falls for 6 frames
 * (ObjectPieceFall), look $997C, +$24 bit 3, +$3A = 0, +$5F = $0A.
 * HOW: Falls into ObjectPieceFall. No caller found: not reached in the
 * traces so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectPieceFall.
 */
void ObjectPieceFallStart(Carried *cr)
{
    uint32_t b = db(cr);

    piece_handler(cr, 0x06);
    lda8(cr, 0x06);
    cyc(16);
    wr8(b | (uint16_t)(0x2049 + cr->x), 0x06);
    piece_look(cr, 0x997C, 0, 0x0A);
    (cyc(24), ObjectPieceFall(cr));
}

void abi_ObjectPieceFallStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPieceFallStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D460 ObjectPieceSpray
 * WHAT IT DOES: A spray piece's first frame (from ObjectSpawnChanceD460):
 * falls for 6 frames (ObjectPieceFall), look $997C, +$24 bit 3, +$3A = 0,
 * +$5F = $0A; its offset ($00, $00, random -$28 to -$19) turned by its
 * parent's (+$27) angles +$14 (RotateXZFrom08) and +$15 (RotateXZ),
 * doubled and added to its position; its parent's record velocity scaled
 * (ObjectVelocityFromRecordScaled), Y - $14, ObjectAdvance5DIfFlag; then
 * its first fall.
 * HOW: The offset in direct page $02/$04, $08/$0A, $94/$05F2 with their
 * sign bytes; the copies between the turns are 8-bit.
 * The sign stores the offsets never take run only as the XSign, YSign and
 * ZSign entries.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectPieceFall.
 */

void ObjectPieceSpray(Carried *cr)
{
    uint32_t b = db(cr);
    uint8_t v;

    piece_handler(cr, 0x06);
    lda8(cr, 0x06);
    cyc(16);
    wr8(b | (uint16_t)(0x2049 + cr->x), 0x06);
    (cyc(62), w_RandomStepLong(cr));
    v = (uint8_t)(cr->a & 0x0F);
    lda8(cr, v);
    cyc(10);
    wr8(b | 0x1F4C, v);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(46);
    r_lda16(cr, b | 0x1F4C);
    r_add16(cr, 0xFFD8);
    cyc(62);
    wr16(b | 0x1F4C, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(22);
    piece_look(cr, 0x997C, 0, 0x0A);
    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2027 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    lda8(cr, 0x00);
    cyc(2);
    wr8(0x000002, 0x00);
    cyc(16);
    wr8(0x000004, 0x00);
    cyc(54);
    wr8(0x000005, 0x00);
    spray_from_y(cr);
}

void abi_ObjectPieceSpray(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPieceSpray(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ObjectPieceSpray from its second offset component on (0: direct page
   $08, $0A, sign $0B). */
static void spray_from_y(Carried *cr)
{
    lda8(cr, 0x00);
    cyc(2);
    wr8(0x000008, 0x00);
    cyc(16);
    wr8(0x00000A, 0x00);
    cyc(54);
    wr8(0x00000B, 0x00);
    spray_from_z(cr);
}

/* ObjectPieceSpray from its third offset component ($1F4C into direct
   page $94 and $05F2 with its sign byte $05F3) on. */
static void spray_from_z(Carried *cr)
{
    uint32_t b = db(cr);
    uint8_t v;

    cyc(24);
    r_lda8(cr, b | 0x1F4C);
    v = (uint8_t)cr->a;
    cyc(16);
    wr8(0x000094, v);
    cyc(24);
    wr8(b | 0x05F2, v);
    if (v & 0x80) {
        lda8(cr, 0xFF);
        cyc(42);
        wr8(b | 0x05F3, 0xFF);
    } else {
        wr8(b | 0x05F3, 0x00);
    }
    cyc(22);
    spray_turn(cr);
}

/* ObjectPieceSpray from its turns on: RotateXZFrom08 and RotateXZ by the
   parent Y's angles, doubled onto X's position, the scaled record
   velocity, Y - $14, ObjectAdvance5DIfFlag, the first fall. */
static void spray_turn(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t w;

    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2014 + cr->y));
    (cyc(62), RotateXZFrom08_cr(cr));
    cyc(16);
    r_lda8(cr, 0x000004);
    cyc(16);
    wr8(0x000002, (uint8_t)cr->a);
    cyc(16);
    r_lda8(cr, 0x00000A);
    cyc(16);
    wr8(0x000008, (uint8_t)cr->a);
    cyc(24);
    r_lda8(cr, b | 0x05F2);
    cyc(16);
    wr8(0x000094, (uint8_t)cr->a);
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2015 + cr->y));
    (cyc(62), RotateXZ_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(38);
    r_lda16(cr, 0x000004);
    r_asl16(cr);
    cyc(30);
    wr16(0x000004, cr->a);
    cyc(16);
    r_lda16(cr, 0x00000A);
    r_asl16(cr);
    cyc(30);
    wr16(0x00000A, cr->a);
    cyc(24);
    r_lda16(cr, b | 0x05F2);
    r_asl16(cr);
    cyc(38);
    wr16(b | 0x05F2, cr->a);
    cyc(30);
    r_lda16(cr, b | (uint16_t)(0x200C + cr->x));
    cyc(30);
    r_add16(cr, rd16(0x000004));
    cyc(30);
    wr16(b | (uint16_t)(0x200C + cr->x), cr->a);
    cyc(30);
    r_lda16(cr, b | (uint16_t)(0x2010 + cr->x));
    cyc(38);
    r_add16(cr, rd16(b | 0x05F2));
    cyc(30);
    wr16(b | (uint16_t)(0x2010 + cr->x), cr->a);
    cyc(30);
    r_lda16(cr, b | (uint16_t)(0x200E + cr->x));
    cyc(30);
    r_add16(cr, rd16(0x00000A));
    cyc(30);
    wr16(b | (uint16_t)(0x200E + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(68), ObjectVelocityFromRecordScaled_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    w = rd16(b | (uint16_t)(0x2030 + cr->x));
    cr->a = w;
    r_add16(cr, 0xFFEC);
    cyc(68);
    wr16(b | (uint16_t)(0x2030 + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(68), ObjectAdvance5DIfFlag(cr));
    (cyc(0), ObjectPieceFall(cr));
}

/*
 * @routine 08:D4B9 ObjectPieceSprayXSign
 * WHAT IT DOES: ObjectPieceSpray entered at the sign store its first
 * offset component (the constant 0) never takes: direct page $05 = $FF,
 * then the rest of the first frame as ObjectPieceSpray.
 * HOW: LDA #$FF, STA $05, then the join; no code reaches these bytes, a
 * direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its parent; D = $0000; DB =
 * $7E. Exit: as ObjectPieceSpray.
 */
void ObjectPieceSprayXSign(Carried *cr)
{
    lda8(cr, 0xFF);
    cyc(2);
    wr8(0x000005, 0xFF);
    (cyc(22), spray_from_y(cr));
}

void abi_ObjectPieceSprayXSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPieceSprayXSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D4CB ObjectPieceSprayYSign
 * WHAT IT DOES: ObjectPieceSpray entered at the sign store its second
 * offset component (the constant 0) never takes: direct page $0B = $FF,
 * then the rest of the first frame as ObjectPieceSpray.
 * HOW: LDA #$FF, STA $0B, then the join; no code reaches these bytes, a
 * direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its parent; D = $0000; DB =
 * $7E. Exit: as ObjectPieceSpray.
 */
void ObjectPieceSprayYSign(Carried *cr)
{
    lda8(cr, 0xFF);
    cyc(2);
    wr8(0x00000B, 0xFF);
    (cyc(22), spray_from_z(cr));
}

void abi_ObjectPieceSprayYSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPieceSprayYSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D4E6 ObjectPieceSprayZSign
 * WHAT IT DOES: ObjectPieceSpray entered at the sign store its third
 * offset component (random -$28 to -$19, always negative) never takes:
 * $05F3 = 0, then the turns and the rest as ObjectPieceSpray.
 * HOW: STZ $05F3, then the join; no code reaches these bytes, a direct
 * call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its parent; D = $0000; DB =
 * $7E. Exit: as ObjectPieceSpray.
 */
void ObjectPieceSprayZSign(Carried *cr)
{
    wr8(db(cr) | 0x05F3, cyc_then(24, 0x00));
    spray_turn(cr);
}

void abi_ObjectPieceSprayZSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPieceSprayZSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_RotateXY(void);
void abi_ObjectCopy14From(void);
void abi_ObjectBillboardTilt(void);
void abi_ObjectCopyPositionFrom(void);
void abi_ObjectCarRecordNew(void);
void abi_GsuRecordWord58(void);
void abi_GsuRecordWord88(void);
void abi_SoundPort0QueuePush(void);

/* LDA.L word,X (16-bit) with its low byte sign-extended (BIT #$80, ORA
   #$FF00 or AND #$00FF). */
static uint16_t sext_word(Carried *cr, uint32_t at, uint32_t lead)
{
    cyc(lead);
    uint16_t w = rd16(at);

    w = (w & 0x0080) ? (cyc(16), (uint16_t)(w | 0xFF00)) : (uint16_t)(w & 0x00FF);
    cr->a = w;
    set_p(cr, nz16(cr->p, w));
    return w;
}

/* `n` when a read at a is ROM (banks $00-$3F and $80-$BF from $8000), whose
   clocks the C states; a work RAM read is placed on the original's. */
static uint32_t rom_n(uint32_t a, uint32_t n)
{
    uint8_t bank = (uint8_t)(a >> 16);

    return (bank < 0x40 || (bank >= 0x80 && bank < 0xC0)) && (a & 0xFFFF) >= 0x8000 ? n : 0;
}

/* $08:E13C: +$24 bit 5 (hidden), +$36 = 3. */
static void attach_hide(Carried *cr)
{
    uint32_t b = db(cr);

    cyc(54);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) | 0x20));
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
    lda8(cr, 0x03);
    cyc(16);
    wr8(b | (uint16_t)(0x2036 + cr->x), 0x03);
    cyc(44);
}

/* $08:E14A: +$24 bit 6, +$36 = 3. */
static void attach_bit6(Carried *cr)
{
    uint32_t b = db(cr);

    cyc(64);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) | 0x40));
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
    lda8(cr, 0x03);
    cyc(16);
    wr8(b | (uint16_t)(0x2036 + cr->x), 0x03);
    cyc(44);
}

/* LDY +$06,X; whether the parent is gone or loose (its +$24 bit 3 or +$25
   bit 2): then the object hides ($08:E13C). */
static int attach_parent_gone(Carried *cr)
{
    uint32_t b = db(cr);

    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->y)) & 0x08));
    if ((uint8_t)cr->a) {
        cyc(10);
        attach_hide(cr);
        return 1;
    }
    cyc(38);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->y)) & 0x04));
    if ((uint8_t)cr->a) {
        cyc(10);
        attach_hide(cr);
        return 1;
    }
    return 0;
}

/* The offset in object X's velocity bytes (+$2E, +$30, +$32, via $1F58-
   $1F5A) as signed direct page words $04, $0A and $05F2, turned by parent
   Y's +$16 (RotateXY), +$14 (RotateXZFrom08) and +$15 (RotateXZ), shifted
   left `sh` times and added to the parent's position into object X's. */

static void offset_on_parent(Carried *cr, int sh)
{
    offset_on_parent_ex(cr, sh, 1);
}

/* As offset_on_parent; without the +$14 turn (RotateXZFrom08) when
   `with14` is 0. */
static void offset_on_parent_ex(Carried *cr, int sh, int with14)
{
    uint32_t b = db(cr);
    uint8_t v;
    int i;

    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x202E + cr->x));
    cyc(24);
    wr8(b | 0x1F58, (uint8_t)cr->a);
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2030 + cr->x));
    cyc(24);
    wr8(b | 0x1F59, (uint8_t)cr->a);
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2032 + cr->x));
    cyc(24);
    wr8(b | 0x1F5A, (uint8_t)cr->a);
    cyc(24);
    r_lda8(cr, b | 0x1F58);
    v = (uint8_t)cr->a;
    cyc(16);
    wr8(0x000002, v);
    cyc(16);
    wr8(0x000004, v);
    if (v & 0x80) {
        lda8(cr, 0xFF);
        cyc(34);
        wr8(0x000005, 0xFF);
    } else {
        cyc(54);
        wr8(0x000005, 0x00);
    }
    cyc((v & 0x80) ? 46 : 24);          /* BRA (negative), LDA abs */
    r_lda8(cr, b | 0x1F59);
    v = (uint8_t)cr->a;
    cyc(16);
    wr8(0x000008, v);
    cyc(16);
    wr8(0x00000A, v);
    if (v & 0x80) {
        lda8(cr, 0xFF);
        cyc(34);
        wr8(0x00000B, 0xFF);
    } else {
        cyc(54);
        wr8(0x00000B, 0x00);
    }
    cyc((v & 0x80) ? 46 : 24);          /* BRA (negative), LDA abs */
    r_lda8(cr, b | 0x1F5A);
    v = (uint8_t)cr->a;
    cyc(16);
    wr8(0x000094, v);
    cyc(24);
    wr8(b | 0x05F2, v);
    if (v & 0x80) {
        lda8(cr, 0xFF);
        cyc(42);
        wr8(b | 0x05F3, 0xFF);
    } else {
        cyc(62);
        wr8(b | 0x05F3, 0x00);
    }
    cyc((v & 0x80) ? 52 : 30);          /* BRA (negative), LDA abs,Y */
    r_lda8(cr, b | (uint16_t)(0x2016 + cr->y));
    (cyc(62), RotateXY_cr(cr));
    cyc(16);
    r_lda8(cr, 0x000004);
    cyc(16);
    wr8(0x000002, (uint8_t)cr->a);
    cyc(16);
    r_lda8(cr, 0x00000A);
    cyc(16);
    wr8(0x000008, (uint8_t)cr->a);
    cyc(24);
    r_lda8(cr, b | 0x05F2);
    cyc(16);
    wr8(0x000094, (uint8_t)cr->a);
    if (with14) {
        cyc(30);
        r_lda8(cr, b | (uint16_t)(0x2014 + cr->y));
        (cyc(62), RotateXZFrom08_cr(cr));
        cyc(16);
        r_lda8(cr, 0x000004);
        cyc(16);
        wr8(0x000002, (uint8_t)cr->a);
        cyc(16);
        r_lda8(cr, 0x00000A);
        cyc(16);
        wr8(0x000008, (uint8_t)cr->a);
        cyc(24);
        r_lda8(cr, b | 0x05F2);
        cyc(16);
        wr8(0x000094, (uint8_t)cr->a);
    }
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2015 + cr->y));
    (cyc(62), RotateXZ_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(38);
    r_lda16(cr, 0x000004);
    for (i = 0; i < sh; i++)
        { cyc(14); r_asl16(cr); }
    cyc(16);
    wr16(0x000004, cr->a);
    cyc(16);
    r_lda16(cr, 0x00000A);
    for (i = 0; i < sh; i++)
        { cyc(14); r_asl16(cr); }
    cyc(16);
    wr16(0x00000A, cr->a);
    cyc(24);
    r_lda16(cr, b | 0x05F2);
    for (i = 0; i < sh; i++)
        { cyc(14); r_asl16(cr); }
    cyc(24);
    wr16(b | 0x05F2, cr->a);
    cyc(30);
    r_lda16(cr, b | (uint16_t)(0x200C + cr->y));
    cyc(30);
    r_add16(cr, rd16(0x000004));
    cyc(30);
    wr16(b | (uint16_t)(0x200C + cr->x), cr->a);
    cyc(30);
    r_lda16(cr, b | (uint16_t)(0x2010 + cr->y));
    cyc(38);
    r_add16(cr, rd16(b | 0x05F2));
    cyc(30);
    wr16(b | (uint16_t)(0x2010 + cr->x), cr->a);
    cyc(30);
    r_lda16(cr, b | (uint16_t)(0x200E + cr->y));
    cyc(30);
    r_add16(cr, rd16(0x00000A));
    cyc(30);
    wr16(b | (uint16_t)(0x200E + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

/*
 * @routine 08:DCB4 ObjectAttachSpawn
 * WHAT IT DOES: Gives object X the attachments listed for the course
 * script's entry ($70:2392 indexes the list at $09:8090: a count, then five
 * bytes each): for each, a new object (kind $A040) with object X as its
 * +$06, its +$04, look, handler and bank from the five words at $09:8000 +
 * the entry's first word (+$04, $1F50, $1F52, +$1A, +$1C); with a look
 * ($1F50 nonzero) +$24 bit 3, +$3A = 0, +$5F = $1F52, +$5D = $1F50; and
 * the entry's three signed bytes as its velocity (+$2E, +$30, +$32).
 * HOW: The list index in X, the count on the stack; $1F4C keeps object X;
 * a failed spawn skips its entry.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X = the object; Y
 * the last attachment (or as ObjectSpawn left it).
 */
void ObjectAttachSpawn(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t t, i;
    uint8_t n;
    int made = 0;

    cyc(24);
    wr16(b | 0x1F4C, cr->x);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(54);
    r_lda16(cr, 0x702392);
    r_tax16(cr);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(38);
    r_lda8(cr, 0x098090 + cr->x);
    n = (uint8_t)cr->a;
    if (n) {
        cyc(80 + rom_n(0x098090 + cr->x, 8));
        cr->x++;
        for (;;) {
            if ((made = spawn(cr, 0xA040))) {
                i = cr->x;
                set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
                cyc(76);
                r_lda16(cr, b | 0x1F4C);
                cyc(30);
                wr16(b | (uint16_t)(0x2006 + cr->y), cr->a);
                cyc(32 + rom_n(0x098090 + i, 16));
                t = rd16(0x098090 + i);
                cyc(92);
                wr16(b | (uint16_t)(0x2004 + cr->y), rd16(0x098000 + t));
                cyc(72);
                wr16(b | 0x1F50, rd16(0x098002 + t));
                cyc(72);
                wr16(b | 0x1F52, rd16(0x098004 + t));
                cyc(78);
                wr16(b | (uint16_t)(0x201A + cr->y), rd16(0x098006 + t));
                set_p(cr, (uint8_t)(cr->p | (P_M)));
                cyc(92);
                r_lda8(cr, 0x098008 + t);
                wr8(b | (uint16_t)(0x201C + cr->y), (uint8_t)cr->a);
                cr->x = i;
                cyc(60);
                r_lda8(cr, b | 0x1F50);
                cyc(24);
                cr->a |= rd8(b | 0x1F51);
                set_p(cr, nz8(cr->p, (uint8_t)cr->a));
                if ((uint8_t)cr->a) {
                    cyc(46);
                    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->y)) | 0x08));
                    cyc(16);
                    wr8(b | (uint16_t)(0x2024 + cr->y), (uint8_t)cr->a);
                    lda8(cr, 0x00);
                    cyc(16);
                    wr8(b | (uint16_t)(0x203A + cr->y), 0x00);
                    cyc(24);
                    r_lda8(cr, b | 0x1F52);
                    cyc(30);
                    wr8(b | (uint16_t)(0x205F + cr->y), (uint8_t)cr->a);
                    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
                    cyc(46);
                    r_lda16(cr, b | 0x1F50);
                    cyc(30);
                    wr16(b | (uint16_t)(0x205D + cr->y), cr->a);
                    set_p(cr, (uint8_t)(cr->p | (P_M)));
                }
                cyc(28);
                set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
                wr16(b | (uint16_t)(0x202E + cr->y), cyc_then(100 + rom_n(0x098092 + cr->x, 16), sext_word(cr, 0x098092 + cr->x, 48)));
                wr16(b | (uint16_t)(0x2030 + cr->y), cyc_then(100, sext_word(cr, 0x098093 + cr->x, 32 + rom_n(0x098093 + cr->x, 16))));
                wr16(b | (uint16_t)(0x2032 + cr->y), cyc_then(100, sext_word(cr, 0x098094 + cr->x, 32 + rom_n(0x098094 + cr->x, 16))));
                set_p(cr, (uint8_t)(cr->p | (P_M)));
            } else { cyc(26); }
            cyc(86);
            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
            r_txa16(cr);
            r_add16(cr, 0x0005);
            r_tax16(cr);
            set_p(cr, (uint8_t)(cr->p | (P_M)));
            n--;
            lda8(cr, n);
            if (!n)
                break;
            cyc(88);
        }
        cyc(made ? 44 : 22);            /* BEQ taken; the SEP of a made one */
    } else { cyc(78); }
    cr->x = rd16(b | 0x1F4C);
    set_p(cr, nz16(cr->p, cr->x));
    cyc(44);
}

void abi_ObjectAttachSpawn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectAttachSpawn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:DDB4 ObjectAttachFollow
 * WHAT IT DOES: An attachment's frame: hidden ($08:E13C) when its parent
 * is gone or loose, or when the parent's record has +$1C bit 4; otherwise
 * placed at the parent's record position (+$1E, +$60, +$22) with its
 * heading (+$27 to +$14, +$25 to +$15), moved by its velocity
 * (ObjectApplyVelocity), rebucketed, +$24 bits 5 and 6 cleared, and bit 6
 * set again ($08:E14A) when the parent's +$24 has it.
 * HOW: Y = the attachment and X = the parent's record while copying.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the path leaves
 * it.
 */
void ObjectAttachFollow(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t obj = cr->x;

    if (attach_parent_gone(cr))
        return;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(90);
    r_lda16(cr, b | (uint16_t)(0x2022 + cr->y));
    cr->y = obj;
    r_tax16(cr);
    cyc(30);
    r_lda16(cr, 0x70001C + cr->x);
    r_and16(cr, 0x0010);
    if (cr->a) {
        cyc(96);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cr->x = obj;
        set_p(cr, nz16(cr->p, obj));
        attach_hide(cr);
        return;
    }
    cyc(76);
    r_lda16(cr, 0x70001E + cr->x);
    cyc(30);
    wr16(b | (uint16_t)(0x200C + cr->y), cr->a);
    cyc(32);
    r_lda16(cr, 0x700022 + cr->x);
    cyc(30);
    wr16(b | (uint16_t)(0x2010 + cr->y), cr->a);
    cyc(32);
    r_lda16(cr, 0x700060 + cr->x);
    cyc(30);
    wr16(b | (uint16_t)(0x200E + cr->y), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(54);
    r_lda8(cr, 0x700027 + cr->x);
    cyc(30);
    wr8(b | (uint16_t)(0x2014 + cr->y), (uint8_t)cr->a);
    cyc(32);
    r_lda8(cr, 0x700025 + cr->x);
    cyc(30);
    wr8(b | (uint16_t)(0x2015 + cr->y), (uint8_t)cr->a);
    cr->x = obj;
    set_p(cr, nz16(cr->p, obj));
    (cyc(98), ObjectApplyVelocity_cr(cr));
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) & 0xDF));
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) & 0xBF));
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->y)) & 0x40));
    if ((uint8_t)cr->a)
        attach_bit6(cr); else { cyc(52); }
}

void abi_ObjectAttachFollow(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectAttachFollow(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:DD8D ObjectAttachStart
 * WHAT IT DOES: An attachment's first frame: handler $08:DDB4
 * (ObjectAttachFollow), its Y velocity's low byte sign-extended and
 * doubled; then its first frame.
 * HOW: Falls into ObjectAttachFollow.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectAttachFollow.
 */
void ObjectAttachStart(Carried *cr)
{
    uint32_t b = db(cr);

    set_handler(cr, cr->x, 0xDDB4);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(4);
    sext_word(cr, b | (uint16_t)(0x2030 + cr->x), 48);
    r_asl16(cr);
    cyc(114);
    wr16(b | (uint16_t)(0x2030 + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(22), ObjectAttachFollow(cr));
}

void abi_ObjectAttachStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectAttachStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:DF4C ObjectAttachTurned
 * WHAT IT DOES: An attachment turned with its parent: hidden ($08:E13C)
 * when the parent is gone or loose; its heading +$15 = the parent's +$15 +
 * the high byte of the parent's record +$3A, +$14 and +$16 the parent's;
 * its position the parent's plus its velocity bytes turned by the
 * parent's angles, times 8; rebucketed; +$36 = 3.
 * HOW: RotateXY, RotateXZFrom08 and RotateXZ on the offset in direct page.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
 */
void ObjectAttachTurned(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t x, y;
    uint8_t p;

    if (attach_parent_gone(cr))
        return;
    x = cr->x;
    y = cr->y;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(134);
    cr->x = rd16(b | (uint16_t)(0x2022 + y));
    cyc(46);
    r_lda16(cr, 0x70003A + cr->x);
    r_xba(cr);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->y = y;
    cr->x = x;
    set_p(cr, nz16(cr->p, x));
    p = (uint8_t)(cr->p & ~P_C);
    cyc(158);
    lda8(cr, alu_adc8((uint8_t)cr->a, rd8(b | (uint16_t)(0x2015 + cr->y)), &p));
    set_p(cr, (uint8_t)((cr->p & ~(P_C | P_V)) | (p & (P_C | P_V))));
    wr8(b | (uint16_t)(0x2015 + cr->x), (uint8_t)cr->a);
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2014 + cr->y));
    cyc(30);
    wr8(b | (uint16_t)(0x2014 + cr->x), (uint8_t)cr->a);
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2016 + cr->y));
    cyc(30);
    wr8(b | (uint16_t)(0x2016 + cr->x), (uint8_t)cr->a);
    offset_on_parent(cr, 3);
    (cyc(84), ObjectRebucket_cr(cr));
    lda8(cr, 0x03);
    cyc(16);
    wr8(b | (uint16_t)(0x2036 + cr->x), 0x03);
    cyc(44);
}

void abi_ObjectAttachTurned(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectAttachTurned(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* PHA, REP #$20, TXA, STA +$06,Y, SEP #$20, PLA: object X as the new
   object's parent. */
static void set_child_of_x(Carried *cr)
{
    wr16(db(cr) | (uint16_t)(0x2006 + cr->y), cyc_then(88, cr->x));
}

/* One spark (kind $A1E4, handler $08:E048, parent X) with velocity
   (vx, $0A, vz); none when the spawn fails. */
static void spark(Carried *cr, uint16_t vx, uint16_t vz)
{
    uint32_t b = (cyc(22), db(cr));

    if (!spawn(cr, 0xA1E4))
        { cyc(26); return; }
    set_child_of_x(cr);
    cyc(50);
    set_handler(cr, cr->y, 0xE048);
    cyc(28);
    sta16_imm(cr, b | (uint16_t)(0x202E + cr->y), vx);
    cyc(6);
    sta16_imm(cr, b | (uint16_t)(0x2030 + cr->y), 0x000A);
    cyc(6);
    sta16_imm(cr, b | (uint16_t)(0x2032 + cr->y), vz);
    cyc(22);                            /* SEP */
}

/*
 * @routine 08:DE31 ObjectSparksStart
 * WHAT IT DOES: Gives object X four sparks (kind $A1E4, handler $08:E048
 * ObjectSparkStart) at offsets (-$1E or $1E, $0A, $3C or -$14), its own
 * handler $08:DF4C (ObjectAttachTurned); then its first frame.
 * HOW: Falls into ObjectAttachTurned; a failed spawn skips its spark.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectAttachTurned.
 */
void ObjectSparksStart(Carried *cr)
{
    set_handler(cr, cr->x, 0xDF4C);
    spark(cr, 0xFFE2, 0x003C);
    spark(cr, 0x001E, 0x003C);
    spark(cr, 0xFFE2, 0xFFEC);
    spark(cr, 0x001E, 0xFFEC);
    ObjectAttachTurned(cr);
}

void abi_ObjectSparksStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSparksStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E073 ObjectSparkStep
 * WHAT IT DOES: A spark's frame: its position its parent's plus its
 * velocity bytes turned by the parent's angles, times 8; the parent's
 * +$14-$16 copied (ObjectCopy14From); tilted toward the camera
 * (ObjectBillboardTilt); rebucketed; +$36 = 3.
 * HOW: As ObjectAttachTurned without the heading's add.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
 */
void ObjectSparkStep(Carried *cr)
{
    uint32_t b = db(cr);

    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    offset_on_parent(cr, 3);
    (cyc(84), ObjectCopy14From_cr(cr));
    cr->y = cr->x;
    set_p(cr, nz16(cr->p, cr->y));
    (cyc(60), ObjectBillboardTilt(cr));
    (cyc(62), ObjectRebucket_cr(cr));
    lda8(cr, 0x03);
    cyc(16);
    wr8(b | (uint16_t)(0x2036 + cr->x), 0x03);
    cyc(44);
}

void abi_ObjectSparkStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSparkStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E048 ObjectSparkStart
 * WHAT IT DOES: A spark's first frame: +$24 bit 3, +$3A = 0, +$5F = $6E,
 * handler $08:E073 (ObjectSparkStep), +$29 = $9BBF; then its first step.
 * HOW: Falls into ObjectSparkStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSparkStep.
 */
void ObjectSparkStart(Carried *cr)
{
    uint32_t b = db(cr);

    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) | 0x08));
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
    lda8(cr, 0x00);
    cyc(16);
    wr8(b | (uint16_t)(0x203A + cr->x), 0x00);
    lda8(cr, 0x6E);
    cyc(16);
    wr8(b | (uint16_t)(0x205F + cr->x), 0x6E);
    set_handler(cr, cr->x, 0xE073);
    cyc(28);
    sta16_imm(cr, b | (uint16_t)(0x2029 + cr->x), 0x9BBF);
    (cyc(22), ObjectSparkStep(cr));
}

void abi_ObjectSparkStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSparkStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E1C7 ObjectCarAttachedStep
 * WHAT IT DOES: A car carried on another: rebucketed, posed from its
 * record (ObjectPoseFromRecord) and its record marked (GsuRecordFlag4, the
 * code at $08:CD7E).
 * HOW: Three calls. Reached only by unit_entries_08.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as GsuRecordFlag4.
 */
void ObjectCarAttachedStep(Carried *cr)
{
    (cyc(62), ObjectRebucket_cr(cr));
    (cyc(62), ObjectPoseFromRecord_cr(cr));
    (cyc(86), GsuRecordFlag4_cr(cr));
    cyc(44);
}

void abi_ObjectCarAttachedStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarAttachedStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E158 ObjectCarAttachedStart
 * WHAT IT DOES: A car carried on its parent (+$06): at the parent's
 * position, $640 back and $1E0 up, a car record of its own
 * (ObjectCarRecordNew with A = 0) marked carried (+$1C bit 3), each
 * record's +$8E naming the other's record; handler $08:E1C7
 * (ObjectCarAttachedStep), run at once.
 * HOW: The records through X with the objects swapped in X and Y. Not
 * reached in the traces so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectCarAttachedStep.
 */
void ObjectCarAttachedStart(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t obj = cr->x, par;

    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    (cyc(62), ObjectCopyPositionFrom_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, b | (uint16_t)(0x2010 + cr->x));
    r_sub16(cr, 0x0640);
    cyc(62);
    wr16(b | (uint16_t)(0x2010 + cr->x), cr->a);
    cyc(74);
    r_lda16(cr, b | (uint16_t)(0x200E + cr->x));
    r_add16(cr, 0xFE20);
    cyc(68);
    wr16(b | (uint16_t)(0x200E + cr->x), cr->a);
    r_lda16_imm(cr, 0x0000);
    (cyc(92), ObjectCarRecordNew(cr));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(52);
    par = rd16(b | (uint16_t)(0x2006 + cr->x));
    cr->y = par;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(82);
    cr->x = rd16(b | (uint16_t)(0x2022 + obj));
    cyc(44);
    r_lda16(cr, b | (uint16_t)(0x2022 + par));
    cyc(32);
    wr16(0x70008E + cr->x, cr->a);
    cyc(32);
    r_lda16(cr, 0x70001C + cr->x);
    cr->a |= 0x0008;
    set_p(cr, nz16(cr->p, cr->a));
    cyc(56);
    wr16(0x70001C + cr->x, cr->a);
    cyc(176);
    cr->x = rd16(b | (uint16_t)(0x2022 + par));
    cyc(44);
    r_lda16(cr, b | (uint16_t)(0x2022 + obj));
    cyc(32);
    wr16(0x70008E + cr->x, cr->a);
    cr->x = obj;
    cr->y = par;
    set_p(cr, (uint8_t)(nz16(cr->p, par) | P_M));
    cyc(138);
    set_handler(cr, cr->x, 0xE1C7);
    ObjectCarAttachedStep(cr);
}

void abi_ObjectCarAttachedStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarAttachedStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E426 ObjectParentRecordBit8
 * WHAT IT DOES: Whether object X's parent's (+$06) record has +$40 bit 8,
 * in C.
 * HOW: 16-bit through the parent's +$22; X kept.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; Y = the parent; C
 * as above; X kept, N and Z from it.
 */
void ObjectParentRecordBit8(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t x = cr->x;

    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(82);
    cr->x = rd16(b | (uint16_t)(0x2022 + cr->y));
    cyc(46);
    r_lda16(cr, 0x700040 + cr->x);
    r_and16(cr, 0x0100);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->x = x;
    set_p(cr, (uint8_t)((nz16(cr->p, x) & ~P_C) | (cr->a ? (cyc(158), P_C) : (cyc(152), 0))));
}

void abi_ObjectParentRecordBit8(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectParentRecordBit8(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E244 ObjectExhaustStep
 * WHAT IT DOES: An exhaust flame's frame by its state +$39: after its
 * start count +$4B, +$48 = the parent's record +$88 - 1; hidden (+$24 bit
 * 5) while the parent is gone or loose (+$25 bits 3, 4) or its record has
 * +$40 bit 8 (ObjectParentRecordBit8). State 1: a changed +$88 starts
 * state 2 for 8 frames; a drop in the parent's record +$58 of 2 to 9 since
 * the last ($1F2B, +$4A) starts state 3 for 8 frames with sound $3B.
 * State 2 shows frame +$88 - 1, state 3 frame 4 (+$18 with bit 7). While
 * +$49 counts down, every fourth frame it is at the parent's position,
 * moved by its velocity, +$24 bit 7 set when the parent's link chain's
 * end has bit 14 (there it is rebucketed at the link's +$08, +$0A, +$0C
 * offset from (0, -$64, $1F4)), else rebucketed; back to state 1 at 0.
 * +$36 = 3.
 * HOW: GsuRecordWord88 and GsuRecordWord58 read the parent's record; 8-bit
 * with 16-bit position adds.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
 */

void ObjectExhaustStep(Carried *cr)
{
    uint32_t b = db(cr);
    uint8_t v, w, p;
    uint16_t t;
    unsigned end = 38;

    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x204B + cr->x));
    if ((uint8_t)cr->a) {
        cyc(52);
        v = (uint8_t)(rd8(b | (uint16_t)(0x204B + cr->x)) - 1);
        cyc(6);
        wr8(b | (uint16_t)(0x204B + cr->x), v);
        (cyc(46), GsuRecordWord88_cr(cr));
        cyc(30);
        wr8(b | (uint16_t)(0x2048 + cr->x), (uint8_t)cr->a);
    } else { cyc(48); }
    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->y)) & 0x08));
    if ((uint8_t)cr->a)
        { cyc(10); goto hide; }
    cyc(38);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->y)) & 0x10));
    if ((uint8_t)cr->a)
        { cyc(10); goto hide; }
    cyc(54);
    ObjectParentRecordBit8(cr);
    if (cr->p & P_C)
        { cyc(22); goto hide; }
    cyc(52);
    r_lda8(cr, b | (uint16_t)(0x2039 + cr->x));
    cyc(20);
    cmp8n(cr, (uint8_t)cr->a, 0x01);
    if ((uint8_t)cr->a == 0x01) {
        cyc(59);
        GsuRecordWord88_cr(cr);
        v = (uint8_t)cr->a;
        cyc(30);
        w = rd8(b | (uint16_t)(0x2048 + cr->x));
        set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - w)) & ~P_C) | (v >= w ? (cyc(6), P_C) : 0)));
        if (v != w) {
            cyc(62);
            wr8(b | (uint16_t)(0x2048 + cr->x), v);
            cyc((uint8_t)(v - w) & 0x80 ? 38 : 16);   /* BRA after the first STA only */
            lda8(cr, 0x02);
            wr8(b | (uint16_t)(0x2039 + cr->x), 0x02);
            lda8(cr, 0x08);
            cyc(16);
            wr8(b | (uint16_t)(0x2049 + cr->x), 0x08);
        } else { cyc(16); }
        cyc(46);
        GsuRecordWord58_cr(cr);
        v = (uint8_t)cr->a;
        cyc(24);
        wr8(b | 0x1F2B, v);
        if (!(cr->p & P_C)) {
            p = (uint8_t)(cr->p | P_C);
            cyc(60);
            w = alu_sbc8(v, rd8(b | (uint16_t)(0x204A + cr->x)), &p);
            set_p(cr, p);
            if (w & 0x80) {
                cyc(68);
                w = (uint8_t)((w ^ 0xFF) + 1);
                set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(w - 0x02)) & ~P_C) | (w >= 0x02 ? P_C : 0)));
                if ((uint8_t)(w - 0x02) & 0x80)
                    goto state2;
                set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(w - 0x0A)) & ~P_C) | (w >= 0x0A ? (cyc(32), P_C) : 0)));
                if ((uint8_t)(w - 0x0A) & 0x80) {
                    lda8(cr, 0x03);
                    cyc(42);
                    wr8(b | (uint16_t)(0x2039 + cr->x), 0x03);
                    lda8(cr, 0x08);
                    cyc(16);
                    wr8(b | (uint16_t)(0x2049 + cr->x), 0x08);
                    cyc(48);
                    lda8(cr, 0x3B);
                    SoundPort0QueuePush_cr(cr);
                }
            } else { cyc(6); }
        } else { cyc(22); }
        cyc(24);
        r_lda8(cr, b | 0x1F2B);
        cyc(30);
        wr8(b | (uint16_t)(0x204A + cr->x), (uint8_t)cr->a);
    } else { cyc(39); }
state2:
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2039 + cr->x));
    cyc(33);
    cmp8n(cr, (uint8_t)cr->a, 0x02);
    if ((uint8_t)cr->a == 0x02) {
        cyc(46);
        GsuRecordWord88_cr(cr);
        cyc(24);
        wr8(b | 0x1F58, (uint8_t)cr->a);
        cyc(24);
        v = (uint8_t)(rd8(b | 0x1F58) | 0x80);
        lda8(cr, v);
        cyc(16);
        wr8(b | (uint16_t)(0x2018 + cr->x), v);
    } else { cyc(26); }
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2039 + cr->x));
    cyc(20);
    cmp8n(cr, (uint8_t)cr->a, 0x03);
    if ((uint8_t)cr->a == 0x03) {
        lda8(cr, 0x84);
        cyc(45);
        wr8(b | (uint16_t)(0x2018 + cr->x), 0x84);
        (cyc(46), GsuRecordWord58_cr(cr));
        cyc(30);
        wr8(b | (uint16_t)(0x204A + cr->x), (uint8_t)cr->a);
    } else { cyc(39); }
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2049 + cr->x));
    v = (uint8_t)cr->a;
    if (!v)
        { cyc(22); goto hide; }
    v--;
    lda8(cr, v);
    if (!v) {
        cyc(58);
        wr8(b | (uint16_t)(0x2049 + cr->x), 0x00);
        lda8(cr, 0x01);
        cyc(16);
        wr8(b | (uint16_t)(0x2039 + cr->x), 0x01);
        goto hide;
    }
    cyc(52);
    wr8(b | (uint16_t)(0x2049 + cr->x), v);
    lda8(cr, (uint8_t)(v & 0x03));
    if (!(uint8_t)cr->a)
        { cyc(8); goto hide; }
    cyc(60);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) & 0xDF));
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    (cyc(62), ObjectCopyPositionFrom_cr(cr));
    (cyc(62), ObjectApplyVelocity_cr(cr));
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) & 0x7F));
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    cr->y = rd16(b | (uint16_t)(0x204F + cr->y));
    cyc(44);
    cr->y = rd16(b | (uint16_t)(0x204F + cr->y));
    cyc(44);
    wr16(b | 0x1F4C, cyc_then(24, rd16(b | (uint16_t)(0x0008 + cr->y))));
    cyc(30);
    wr16(b | 0x1F4E, cyc_then(24, rd16(b | (uint16_t)(0x000A + cr->y))));
    cyc(30);
    wr16(b | 0x1F50, cyc_then(24, rd16(b | (uint16_t)(0x000C + cr->y))));
    cyc(30);
    r_lda16(cr, b | cr->y);
    r_and16(cr, 0x4000);
    if (!cr->a) {
        cyc(152);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        ObjectRebucket_cr(cr);
    } else {
        cyc(50);
        sta16_imm(cr, b | (uint16_t)(0x200C + cr->x), 0x0000);
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        r_lda16_imm(cr, 0xFF9C);
        cyc(16);
        wr16(b | (uint16_t)(0x200E + cr->x), 0xFF9C);
        r_lda16_imm(cr, 0x01F4);
        cyc(16);
        wr16(b | (uint16_t)(0x2010 + cr->x), 0x01F4);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(52);
        lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) | 0x80));
        cyc(16);
        wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
        for (t = 0; t < 3; t++) {
            static const uint16_t f[3] = {0x200C, 0x200E, 0x2010}, s[3] = {0x1F4C, 0x1F4E, 0x1F50};

            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
            cyc(t ? 74 : 52)       /* first: no SEP before the REP */;
            r_lda16(cr, b | (uint16_t)(f[t] + cr->x));
            cyc(38);
            r_add16(cr, rd16(b | s[t]));
            cyc(30);
            wr16(b | (uint16_t)(f[t] + cr->x), cr->a);
            set_p(cr, (uint8_t)(cr->p | (P_M)));
        }
        cyc(84);
        ObjectRebucket_cr(cr);
        for (t = 0; t < 3; t++) {
            static const uint16_t f[3] = {0x200C, 0x200E, 0x2010}, s[3] = {0x1F4C, 0x1F4E, 0x1F50};

            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
            cyc(t ? 74 : 52);
            r_lda16(cr, b | (uint16_t)(f[t] + cr->x));
            cyc(38);
            r_sub16(cr, rd16(b | s[t]));
            cyc(24);
            wr16(b | (uint16_t)(f[t] + cr->x), cr->a);
            set_p(cr, (uint8_t)(cr->p | (P_M)));
        }
        end = 60;                       /* SEP, BRA, LDA, STA */
    }
    lda8(cr, 0x03);
    cyc(end);
    wr8(b | (uint16_t)(0x2036 + cr->x), 0x03);
    cyc(44);
    return;
hide:
    attach_hide(cr);
}

void abi_ObjectExhaustStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectExhaustStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E1D2 ObjectExhaustStart
 * WHAT IT DOES: An exhaust flame's first frame: released when its
 * parent's record has +$70 bit 3, or $70:26F2 lacks bit 8, or in HUD
 * modes 1-3; otherwise handler $08:E244 (ObjectExhaustStep), +$4A = the
 * parent's record +$58, +$49 = 0, its Y velocity's low byte
 * sign-extended and doubled, start count +$4B = $0A, state +$39 = 1; then
 * its first step.
 * HOW: Falls into ObjectExhaustStep; the release is ObjectRelease.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectExhaustStep or
 * ObjectRelease.
 */
void ObjectExhaustStart(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t x = cr->x;
    uint8_t v;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(82);
    cr->x = rd16(b | (uint16_t)(0x2006 + cr->x));
    cyc(44);
    cr->x = rd16(b | (uint16_t)(0x2022 + cr->x));
    cyc(46);
    r_lda16(cr, 0x700070 + cr->x);
    cr->x = x;
    r_and16(cr, 0x0008);
    if (!cr->a) {
        cyc(106);
        r_lda16(cr, 0x7026F2);
        r_and16(cr, 0x0100);
        if (cr->a) {
            set_p(cr, (uint8_t)(cr->p | (P_M)));
            cyc(84);
            r_lda8(cr, b | 0x0D2B);
            v = (uint8_t)cr->a;
            if (v != 0x01 && (cyc(26), v != 0x02) && (cyc(26), v != 0x03)) {
                cyc(50);
                set_handler(cr, cr->x, 0xE244);
                (cyc(46), GsuRecordWord58_cr(cr));
                cyc(30);
                wr8(b | (uint16_t)(0x204A + cr->x), (uint8_t)cr->a);
                lda8(cr, 0x00);
                cyc(16);
                wr8(b | (uint16_t)(0x2049 + cr->x), 0x00);
                set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
                cyc(4);
                sext_word(cr, b | (uint16_t)(0x2030 + cr->x), 48);
                r_asl16(cr);
                cyc(114);
                wr16(b | (uint16_t)(0x2030 + cr->x), cr->a);
                set_p(cr, (uint8_t)(cr->p | (P_M)));
                lda8(cr, 0x0A);
                cyc(38);
                wr8(b | (uint16_t)(0x204B + cr->x), 0x0A);
                lda8(cr, 0x01);
                cyc(16);
                wr8(b | (uint16_t)(0x2039 + cr->x), 0x01);
                ObjectExhaustStep(cr);
                return;
            }
        }
    } else { cyc(36); }
    cyc(128);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    ObjectRelease_cr(cr);
    cyc(44);
}

void abi_ObjectExhaustStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectExhaustStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_SoundAtRateForLevel(void);
void abi_PlayerSound12(void);
void abi_PlayerSound0C(void);
void abi_PlayerSound23(void);
void abi_PlayerSound24(void);
void abi_PlayerSound0D(void);
void abi_PlayerSound0B(void);
void abi_PlayerStopHeldSound(void);
void abi_PlayerHoldSound32(void);
void abi_PlayerHoldSound33(void);
void abi_PlayerHoldSound34(void);
void abi_PlayerHoldSound35(void);
void abi_ObjectSpawnSparklePair(void);
void abi_ObjectSpawnAttached(void);
void abi_ObjectSpawnTrail(void);
void abi_ObjectSpawnD460UnlessFlag(void);

/* LDA $1A5D, AND #$FF (16-bit): the player in hand's sound latches are at
   $1A60 for player 2, $1A5E for player 1. */
static uint16_t latch_addr(Carried *cr)
{
    uint32_t b = db(cr);

    cyc(64);
    r_lda16(cr, b | 0x1A5D);
    r_and16(cr, 0x00FF);
    return cr->a ? 0x1A60 : (cyc(6), 0x1A5E);
}

/* A latch bit and its sound: when `on` and the bit is clear, the sound
   (`snd`) and the bit set; when off and the bit is set, the sound and the
   bit cleared. 16-bit, the latch word re-chosen before the store. Returns
   the time from its last access to the code after it (each copy's tail in
   the assembly): unchanged, BIT # and the branch taken, then on the BRA
   (68 on, 46 off; 22 more for player 2's word, its BRA); changed, the
   BRAs after the store (on: 44 for player 2's word, 22 for player 1's;
   off: 22 or 0). */
static uint32_t latch(Carried *cr, uint16_t bit, int on, CarriedFn snd)
{
    uint32_t b = db(cr);
    uint16_t w, at, word = latch_addr(cr);

    r_lda16(cr, cyc_then(62, b | word));
    set_p(cr, (uint8_t)((cr->p & ~P_Z) | ((cr->a & bit) ? 0 : P_Z)));
    if (((cr->a & bit) ? 1 : 0) == on)
        return (on ? 68 : 46) + (word == 0x1A60 ? 22 : 0);   /* player 2's word: BRA */
    w = cr->a;
    if (word == 0x1A60)
        cyc(22);                            /* player 2's BRA past player 1's load */
    snd(cr);
    w = on ? (uint16_t)(w | bit) : (uint16_t)(w & ~bit);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    at = (cyc(50), latch_addr(cr));
    wr16(b | at, cyc_then(98, w));
    cr->a = w;
    set_p(cr, nz16(cr->p, w));
    return (at == 0x1A60 ? 22 : 0) + (on ? 22 : 0);
}

/* CMP #k (8-bit) on v: N, Z, C set; whether v - k is negative. */
static int cmp8n(Carried *cr, uint8_t v, uint8_t k)
{
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(v - k)) & ~P_C) | (v >= k ? (cyc(5), P_C) : (cyc(5), 0))));
    return ((uint8_t)(v - k) & 0x80) != 0;
}

/* CMP #k / BMI on A (16-bit): whether A - k is negative. */
static int below16(Carried *cr, uint16_t a, uint16_t k)
{
    set_p(cr, (uint8_t)((nz16(cr->p, (uint16_t)(a - k)) & ~P_C) | (a >= k ? (cyc(46), P_C) : 0)));
    return ((uint16_t)(a - k) & 0x8000) != 0;
}

/* LDA #s, JSL SoundPort0QueuePush (16-bit). */
static void snd16(Carried *cr, uint16_t s)
{
    r_lda16_imm(cr, s);
    (cyc(58), SoundPort0QueuePush_cr(cr));
}

/*
 * @routine 08:C124 PlayerCarEvents
 * WHAT IT DOES: The player car X's per-frame events from its Super FX
 * record: unless its +$25 has bit 4 or the record +$40 bit 8: with $1AEC
 * bit 5, the engine sound at its speed (SoundAtRateForLevel) when one of
 * its parts' records (the +$3E/+$34 chain) is a loose wheel (+$2C bit 7,
 * +$46 bits 0 and 4), else the latch for record +$92 bit 5 (sounds $12 /
 * $0C). Then the latches for +$42 bit 10 (sounds $23 / $24) and for a
 * landing (+$42 bit 13 with +$40 bit 11, not bit 8: sounds $0D / $0B); the
 * held sound by +$57 (stop, $32-$35); a hit by +$94 (sounds $54-$57 by
 * size, sparks from $50 on); sounds $0A / $0D for +$2C bits 8 / 9; a
 * scrape by -$62 (sounds $16-$19, from $46 on +$25 bit 5 and an attached
 * object); a crash by +$40 bit 0 (the code at $08:A034 with A = 1, then
 * unless it says so (C) +$25 bit 4 and a burst for one player), bit 6
 * (+$25 bit 5, sparks, $08:A034 with A = 0) or bit 5 ($08:A034 with A =
 * 1, a burst); unless the car's +$24 has bit 6, trails for +$40 bit 1
 * (and a spray chance) or, 14 in 256, for +$42 bit 10; a record +$82 of
 * $199 or more latches bit 5 once ($00:1FFE = $11, sound $3D) and may
 * spray. Last its +$24 bit 5 follows +$25 bits 2 and 7 and bit 6 the
 * link chain's end (bit 14).
 * HOW: The car in $1F18; the record in X; the latches through $1A5D; the
 * code at $08:A034 is CarCrashParts.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X = the object.
 */
void PlayerCarEvents(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t obj = cr->x, rec, w, x;
    uint8_t v;
    int found;
    uint32_t to_c4a2 = 104, tail = 78, sparkle = 186;
    int16_t lvl;

    cyc(24);
    wr16(b | 0x1F18, obj);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->y = obj;
    cyc(96);
    r_lda16(cr, b | (uint16_t)(0x2022 + obj));
    r_tax16(cr);
    rec = cr->x;
    cyc(14);
    r_lda16(cr, b | (uint16_t)(0x2025 + cr->y));
    if (cr->a & 0x0010)
        { cyc(26); goto tail; }
    cyc(78);
    r_lda16(cr, 0x700040 + cr->x);
    if (!(cr->a & 0x0100)) {
        cyc(70);
        r_lda16(cr, b | 0x1AEC);
        if (cr->a & 0x0020) {
            cyc(70);
            wr16(b | 0x1F4C, 0x0000);
            cyc(62);
            r_lda16(cr, 0x70003E + cr->x);
            found = 0;
            do {
                r_tax16(cr);
                cyc(16);
                r_lda16(cr, 0x70002C + cr->x);
                if (cr->a & 0x0080) {
                    cyc(72);
                    r_lda16(cr, 0x700046 + cr->x);
                    if ((cr->a & 0x0001) && (cyc(40), (cr->a & 0x0010))) {
                        r_lda16_imm(cr, 0x0001);
                        cyc(50);
                        wr16(b | 0x1F4C, 0x0001);
                    } else { cyc(46); }
                } else { cyc(46); }
                cyc(32);
                r_lda16(cr, 0x700034 + cr->x);
            } while (cyc_loop(22, cr->a));
            (void)found;
            cr->x = rec;
            cyc(76);
            r_lda16(cr, b | 0x1F4C);
            if (cr->a) {
                cyc(46);
                r_lda16(cr, b | 0x0D62);
                r_and16(cr, 0x00FF);
                if (!cr->a) {
                    r_lda16_imm(cr, 0x0010);
                    cyc(56);
                    wr16(0x7026EE, 0x0010);
                } else { cyc(44); }
                cyc(32);
                r_lda16(cr, 0x70004A + cr->x);
                cr->a = abs16(cr->a);
                lvl = (int16_t)cr->a;
                SoundAtRateForLevel_cr(cr);
                if (cr->a == 0x0023 && lvl >= 0x28)     /* queued: the RTS, and the BRA below $A0 */
                    tail += lvl < 0xA0 ? 50 : 28;
            }
        } else {
            cyc(102);
            r_lda16(cr, 0x700092 + cr->x);
            if (cr->a & 0x0020)
                tail = latch(cr, 0x0004, 1, PlayerSound12_cr);
            else
                { cyc(6); tail = latch(cr, 0x0004, 0, PlayerSound0C_cr); }
            tail += 32;                         /* LDA long at $C213 */
        }
    } else { cyc(24); }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(tail);
    r_lda16(cr, 0x700042 + cr->x);
    if (cr->a & 0x0400)
        tail = latch(cr, 0x0001, 1, PlayerSound23_cr);
    else
        { cyc(6); tail = latch(cr, 0x0001, 0, PlayerSound24_cr); }
    cyc(tail + 32);                             /* LDA long at $C280 */
    r_lda16(cr, 0x700042 + cr->x);
    cyc(cr->a & 0x0080 ? 72 : 78);              /* BEQ to the next line: 6 more taken */
    r_lda16(cr, 0x700042 + cr->x);
    found = 0;
    if (cr->a & 0x2000) {
        cyc(72);
        r_lda16(cr, 0x700040 + cr->x);
        if ((cr->a & 0x0800) && (cyc(40), !(cr->a & 0x0100)))
            found = 1;
    }
    tail = latch(cr, 0x0002, found, found ? PlayerSound0D_cr : (cyc(6), PlayerSound0B_cr));
    /* $08:C304: the held sound by the record's +$57. */
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(tail + 54);                             /* SEP, LDA long */
    r_lda8(cr, 0x700057 + cr->x);
    v = (uint8_t)cr->a;
    cyc(24);
    wr8(b | 0x1F5E, v);
    cyc(34);
    if (!cmp8n(cr, v, 0x00)) {
        cyc(98);
        if (cmp8n(cr, v, 0x01))
            { cyc(6); PlayerStopHeldSound_cr(cr); }
        else if (cmp8n(cr, v, 0x02))
            PlayerHoldSound32_cr(cr);
        else { cyc(20); if (cmp8n(cr, v, 0x04))
            { cyc(13); PlayerHoldSound33_cr(cr); }
        else { cyc(20); if (cmp8n(cr, v, 0x05))
            { cyc(26); PlayerHoldSound34_cr(cr); }
        else { cyc(20); if (cmp8n(cr, v, 0x64))
            { cyc(39); PlayerHoldSound35_cr(cr); } } } }
        cyc(1);
    }
    lda8(cr, v);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(73);
    r_lda16(cr, 0x700094 + cr->x);
    w = cr->a;
    if (w && (cyc(56), !(w & 0x8000))) {
        sparkle = 186;
        if (!below16(cr, w, 0x0032)) {
            if (below16(cr, w, 0x01F4))
                { cyc(60); snd16(cr, 0x0054); sparkle = 178; }
            else if (below16(cr, w, 0x0320))
                { cyc(60); snd16(cr, 0x0055); sparkle = 178; }
            else if (below16(cr, w, 0x044C))
                { cyc(60); snd16(cr, 0x0056); sparkle = 178; }
            else if (below16(cr, w, 0x07D0))
                { cyc(60); snd16(cr, 0x0057); sparkle = 178; }
        }
        cr->a = w;
        if (!below16(cr, w, 0x0032)) {
            x = cr->x;
            set_p(cr, (uint8_t)(cr->p | (P_M)));
            cyc(sparkle);
            cr->x = rd16(b | 0x1F18);
            (cyc(62), ObjectSpawnSparklePair(cr));
            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
            cr->x = x;
        }
        cyc(36);
    }
    cyc(54);
    r_lda16(cr, 0x70002C + cr->x);
    if (cr->a & 0x0100)
        { cyc(30); snd16(cr, 0x000A); cyc(54); }             /* BRA, LDA long */
    else if (cr->a & 0x0200)
        { cyc(76); snd16(cr, 0x000D); cyc(32); }
    else
        cyc(124);
    r_lda16(cr, 0x700062 + cr->x);
    if (cr->a & 0x8000) {
        int a1 = 0, pushed = 0;

        cyc(74);
        w = (uint16_t)((cr->a ^ 0xFFFF) + 1);
        cyc(34);
        if (!below16(cr, w, 0x0041)) {
            cyc(0);
            if (below16(cr, w, 0x0050))
                { snd16(cr, 0x0016); a1 = 1; pushed = 1; }
            else if (below16(cr, w, 0x005A))
                { snd16(cr, 0x0017); pushed = 1; }
            else if (below16(cr, w, 0x006E))
                { snd16(cr, 0x0018); pushed = 1; }
            else if (below16(cr, w, 0x03E8))
                { snd16(cr, 0x0019); pushed = 1; }
        }
        if (!below16(cr, w, 0x0046)) {
            uint16_t ysave = cr->y;

            x = cr->x;
            cr->x = cr->y;
            set_p(cr, (uint8_t)(cr->p | (P_M)));
            cyc(pushed ? 178 : 186);
            lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->x)) | 0x20));
            cyc(16);
            wr8(b | (uint16_t)(0x2025 + cr->x), (uint8_t)cr->a);
            lda8(cr, 0x0A);
            cyc(10);
            wr8(b | 0x1F59, 0x0A);
            lda8(cr, 0x00);
            cyc(10);
            wr8(b | 0x1F5A, 0x00);
            (cyc(62), ObjectSpawnAttached(cr));
            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
            cr->y = ysave;
            cr->x = x;
        } else { cyc(a1 ? 10 : pushed ? 2 : 10); }
        cyc(72);
    }
    cyc(54);
    r_lda16(cr, 0x700040 + cr->x);
    if (cr->a & 0x0001) {
        x = cr->x;
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(116);
        cr->x = rd16(b | 0x1F18);
        cyc(48);
        lda8(cr, 0x01);
        CarCrashParts(cr);
        if (!(cr->p & P_C)) {
            cyc(46);
            lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->x)) | 0x10));
            cyc(16);
            wr8(b | (uint16_t)(0x2025 + cr->x), (uint8_t)cr->a);
            cyc(24);
            r_lda8(cr, b | 0x0D62);
            if (!(uint8_t)cr->a)
                { cyc(78); ObjectBurstSpawn(cr); }
        }
        cyc(22);
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cr->x = x;
    } else {
        cyc(78);
        r_lda16(cr, 0x700040 + cr->x);
        if (cr->a & 0x0040) {
            x = cr->x;
            set_p(cr, (uint8_t)(cr->p | (P_M)));
            cyc(116);
            cr->x = rd16(b | 0x1F18);
            cyc(30);
            lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->x)) | 0x20));
            cyc(16);
            wr8(b | (uint16_t)(0x2025 + cr->x), (uint8_t)cr->a);
            (cyc(62), ObjectSpawnSparklePair(cr));
            cyc(48);
            lda8(cr, 0x00);
            CarCrashParts(cr);
            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
            cr->x = x;
        } else {
            cyc(78);
            r_lda16(cr, 0x700040 + cr->x);
            if (cr->a & 0x0020) {
                x = cr->x;
                set_p(cr, (uint8_t)(cr->p | (P_M)));
                cyc(116);
                cr->x = rd16(b | 0x1F18);
                cyc(48);
                lda8(cr, 0x01);
                CarCrashParts(cr);
                cyc(24);
                r_lda8(cr, b | 0x0D62);
                if (!(uint8_t)cr->a)
                    { cyc(78); ObjectBurstSpawn(cr); }
                set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
                cr->x = x;
            } else {
                to_c4a2 = 70;           /* bit 5 clear: straight on */
            }
        }
    }
    /* $08:C4A2 */
    cyc(to_c4a2);
    cr->y = rd16(b | 0x1F18);
    cyc(30);
    r_lda16(cr, b | (uint16_t)(0x2024 + cr->y));
    if (cr->a & 0x0040)
        { cyc(24); goto tail; }
    cyc(78);
    r_lda16(cr, 0x700040 + cr->x);
    if (cr->a & 0x0002) {
        x = cr->x;
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(116);
        cr->x = rd16(b | 0x1F18);
        (cyc(62), ObjectSpawnTrail(cr));
        (cyc(46), ObjectSpawnD460UnlessFlag(cr));
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cr->x = x;
        cyc(192);
    } else {
        cyc(78);
        r_lda16(cr, 0x700042 + cr->x);
        if (!(cr->a & 0x0400)) {
            cyc(100);                   /* bit $0400 clear: on to $70:0082 */
        } else {
            cyc(124);
            set_p(cr, (uint8_t)(cr->p | (P_M)));
            w_RandomStepLong(cr);
            cmp8n(cr, (uint8_t)cr->a, 0xF2);
            if (cr->p & P_C) {
                x = cr->x;
                cyc(109);
                cr->x = rd16(b | 0x1F18);
                (cyc(62), ObjectSpawnTrail(cr));
                cr->x = x;
            } else { cyc(1); }
            cyc(8);
            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
            cyc(104);
        }
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, 0x700082 + cr->x);
    if (!below16(cr, cr->a, 0x0199)) {
        uint16_t at;
        unsigned lead = 92;

        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(40);
        r_lda8(cr, b | 0x1A5D);
        at = (uint8_t)cr->a ? 0x1A60 : (cyc(6), 0x1A5E);
        cyc(56);
        r_lda8(cr, b | at);
        v = (uint8_t)cr->a;
        if (at == 0x1A60)
            lead = 114;                     /* the BRA past player 1's load */
        if (!(v & 0x20)) {
            lda8(cr, 0x11);
            cyc(at == 0x1A60 ? 116 : 94);  /* player 2's: BRA; BIT, BNE, PHA, PHA, LDA #, STA long */
            wr8(0x001FFE, 0x11);
            cyc(76);
            lda8(cr, 0x3D);
            SoundPort0QueuePush_cr(cr);
            v |= 0x20;
            cyc(90);
            r_lda8(cr, b | 0x1A5D);
            at = (uint8_t)cr->a ? 0x1A60 : (cyc(6), 0x1A5E);
            lda8(cr, v);
            cyc(54);
            wr8(b | at, v);
            lead = at == 0x1A60 ? 76 : 54;      /* player 2's: BRA */
        }
        x = cr->x;
        cyc(lead);
        cr->x = rd16(b | 0x1F18);
        cyc(30);
        lda8(cr, cyc_then(34, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->x)) & 0x04)));
        if (!(uint8_t)cr->a) {
            cyc(36);
            w_RandomStepLong(cr);
            cyc(1);
            cmp8n(cr, (uint8_t)cr->a, 0x4C);
            if (cr->p & P_C)
                { cyc(78); ObjectSpawnD460UnlessFlag(cr); }
            cyc(58);
        }
        cyc(12);
        cr->x = x;
    }
tail:
    /* $08:C554 */
    cr->x = obj;
    set_p(cr, (uint8_t)(nz16(cr->p, obj) | P_M));
    cyc(134);
    v = rd8(b | (uint16_t)(0x2025 + cr->x));
    lda8(cr, (uint8_t)(v & 0x04));
    if (!(v & 0x04)) {
        cyc(38);
        lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->x)) & 0x80));
        if (!(uint8_t)cr->a) {
            cyc(38);
            lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) & 0xDF));
            cyc(16);
            wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
            cyc(22);                    /* BRA */
            goto link;
        }
    }
    cyc(64);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) | 0x20));
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
link:
    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x204F + cr->x));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    cr->y = rd16(b | (uint16_t)(0x204F + cr->y));
    cyc(44);
    r_lda16(cr, b | cr->y);
    r_and16(cr, 0x4000);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (cr->a) {
        cyc(90);
        lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) | 0x40));
        cyc(16);
        wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
        cyc(22);                        /* BRA */
    } else {
        cyc(96);
        lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) & 0xBF));
        cyc(16);
        wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
    }
    cyc(44);
}

void abi_PlayerCarEvents(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerCarEvents(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ObjectPlaceOnParent(void);
void abi_ObjectCountStep5F(void);

/*
 * @routine 08:E443 ObjectOffsetOnParent2
 * WHAT IT DOES: An attachment placed at its velocity bytes turned by its
 * parent's angles, times 2: hidden ($08:E13C) when the parent's +$24 has
 * bit 3 or 5; +$18 = $80; then +$24 bit 5 cleared and rebucketed.
 * HOW: As ObjectAttachTurned's placement with one shift. No caller found:
 * not reached in the traces so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as ObjectRebucket.
 */
void ObjectOffsetOnParent2(Carried *cr)
{
    uint32_t b = db(cr);

    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->y)) & 0x08));
    if ((uint8_t)cr->a) {
        cyc(10);
        attach_hide(cr);
        return;
    }
    cyc(38);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->y)) & 0x20));
    if ((uint8_t)cr->a) {
        cyc(10);
        attach_hide(cr);
        return;
    }
    lda8(cr, 0x80);
    cyc(40);
    wr8(b | (uint16_t)(0x2018 + cr->x), 0x80);
    offset_on_parent(cr, 1);
    cyc(52);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) & 0xDF));
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

void abi_ObjectOffsetOnParent2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectOffsetOnParent2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* $08:E682: on its parent (ObjectPlaceOnParent with X = the parent, Y =
   the lamp), rebucketed, +$24 bits 5 and 6 cleared, bit 6 again when the
   parent has it ($08:E14A); +$36 = 3. */
static void lamp_place(Carried *cr)
{
    uint32_t b = db(cr);

    cyc(38);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    swap_xy(cr);
    ObjectPlaceOnParent(cr);
    cr->x = cr->y;
    set_p(cr, nz16(cr->p, cr->x));
    (cyc(76), ObjectRebucket_cr(cr));
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) & 0xDF));
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) & 0xBF));
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->y)) & 0x40));
    if ((uint8_t)cr->a) {
        attach_bit6(cr);
        return;
    }
    lda8(cr, 0x03);
    cyc(24);
    wr8(b | (uint16_t)(0x2036 + cr->x), 0x03);
    cyc(44);
}

void abi_ObjectLampDrop(void);
void abi_ObjectLampDropB(void);
void abi_ObjectLampDropBStart(void);

/* $08:E6BE: with the parent's +$25 bit 4 released; otherwise handler
   $08:E6DE (ObjectLampDrop), +$49 = 0, and its first frame. */
static void lamp_drop_start(Carried *cr)
{
    uint32_t b = db(cr);

    cyc(52);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->y)) & 0x10));
    if ((uint8_t)cr->a) {
        cyc(34);
        ObjectReleaseLong(cr);
        return;
    } else { cyc(8); }
    set_handler(cr, cr->x, 0xE6DE);
    cyc(30);
    wr8(b | (uint16_t)(0x2049 + cr->x), 0x00);
    ObjectLampDrop(cr);
}

/*
 * @routine 08:E653 ObjectLampOnParent
 * WHAT IT DOES: A lamp's frame on its parent (+$06): hidden ($08:E13C)
 * when the parent is gone (+$24 bit 3); when the parent is loose (+$25 bit
 * 2) it drops ($08:E6BE); with +$25 bit 5 it bounces ($08:E712,
 * ObjectLampDropBStart); with bit 6 it takes look $9AB1 and drops;
 * otherwise placed on the parent ($08:E682).
 * HOW: Byte tests through the data bank.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the path leaves
 * it.
 */
void ObjectLampOnParent(Carried *cr)
{
    uint32_t b = db(cr);

    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->y)) & 0x08));
    if ((uint8_t)cr->a) {
        cyc(10);
        attach_hide(cr);
        return;
    }
    cyc(38);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->y)) & 0x04));
    if ((uint8_t)cr->a) {
        cyc(12);
        lamp_drop_start(cr);
        return;
    }
    cyc(38);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->y)) & 0x20));
    if ((uint8_t)cr->a) {
        cyc(34);
        ObjectLampDropBStart(cr);
        return;
    }
    cyc(38);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->y)) & 0x40));
    if ((uint8_t)cr->a) {
        cyc(62);
        sta16_imm(cr, b | (uint16_t)(0x205D + cr->x), 0x9AB1);
        lamp_drop_start(cr);
        return;
    }
    lamp_place(cr);
}

void abi_ObjectLampOnParent(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLampOnParent(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* One step of a lamp's look animation: unless its count +$4B is 4, the
   count up one and its look +$5D the word for it in the table at
   $08:table (X 8-bit for the index). */
static void lamp_look_step(Carried *cr, uint16_t table)
{
    uint32_t b = db(cr);
    uint8_t n;
    uint16_t x = cr->x;

    cyc(78);
    r_lda8(cr, b | (uint16_t)(0x204B + cr->x));
    n = (uint8_t)cr->a;
    if (n == 0x04)
        { cyc(38); return; }               /* CMP, BNE, BRL; the caller's 24 is a JMP */
    n++;
    lda8(cr, n);
    cyc(52);
    wr8(b | (uint16_t)(0x204B + cr->x), n);
    cr->x = (uint8_t)(n << 1);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(238);
    r_lda16(cr, 0x080000 | (uint16_t)(table + cr->x));
    cr->x = x;
    wr16(b | (uint16_t)(0x205D + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(22);
}

/*
 * @routine 08:E51E ObjectLampAnimate
 * WHAT IT DOES: A lamp's look by $1A5B bits 2-4: 4, 8, $0C, $1C step its
 * count +$4B up to 4 through the looks at $08:E57F, $08:E5AD, $08:E5DB,
 * $08:E609; $10 sets look $9AAF; anything else clears the count; then its
 * frame on the parent (ObjectLampOnParent).
 * HOW: Compare chain. The game keeps the count at 0-4, so each table's
 * word 0 is read only for a count of $FF (a unit call stages it).
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectLampOnParent.
 */
void ObjectLampAnimate(Carried *cr)
{
    uint32_t b = db(cr);
    uint8_t v;

    cyc(24);
    r_lda8(cr, b | 0x1A5B);
    v = (uint8_t)(cr->a & 0x1C);
    lda8(cr, v);
    if (v == 0x08)
        lamp_look_step(cr, 0xE5AD);
    else if (v == 0x04)
        { cyc(38); lamp_look_step(cr, 0xE57F); }
    else if (v == 0x0C)
        { cyc(76); lamp_look_step(cr, 0xE5DB); }
    else if (v == 0x1C)
        { cyc(114); lamp_look_step(cr, 0xE609); }
    else if (v == 0x10)
        { cyc(228); sta16_imm(cr, b | (uint16_t)(0x205D + cr->x), 0x9AAF); cyc(22); /* SEP */ }
    else {
        lda8(cr, 0x00);
        cyc(192);
        wr8(b | (uint16_t)(0x204B + cr->x), 0x00);
    }
    cyc(24);
    ObjectLampOnParent(cr);
}

void abi_ObjectLampAnimate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLampAnimate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E61C ObjectLampWait
 * WHAT IT DOES: A lamp waiting: with $1A5B bit 5 its count +$4B = 0 and
 * handler $08:E51E (ObjectLampAnimate), run at once; otherwise look $9AAF,
 * handler $08:E653 (ObjectLampOnParent), run at once.
 * HOW: Falls into the chosen handler.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as the handler leaves it.
 */
void ObjectLampWait(Carried *cr)
{
    uint32_t b = db(cr);

    cyc(24);
    r_lda8(cr, b | 0x1A5B);
    lda8(cr, (uint8_t)(cr->a & 0x20));
    if ((uint8_t)cr->a) {
        lda8(cr, 0x00);
        cyc(18);
        wr8(b | (uint16_t)(0x204B + cr->x), 0x00);
        set_handler(cr, cr->x, 0xE51E);
        (cyc(24), ObjectLampAnimate(cr));
        return;
    } else { cyc(36); }
    sta16_imm(cr, b | (uint16_t)(0x205D + cr->x), 0x9AAF);
    cyc(22);
    set_handler(cr, cr->x, 0xE653);
    ObjectLampOnParent(cr);
}

void abi_ObjectLampWait(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLampWait(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E613 ObjectLampStart
 * WHAT IT DOES: A lamp's first frame: its +$26 from its parent's; then as
 * ObjectLampWait.
 * HOW: Falls into ObjectLampWait.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectLampWait.
 */
void ObjectLampStart(Carried *cr)
{
    uint32_t b = db(cr);

    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2026 + cr->y));
    cyc(30);
    wr8(b | (uint16_t)(0x2026 + cr->x), (uint8_t)cr->a);
    ObjectLampWait(cr);
}

void abi_ObjectLampStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLampStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E6DE ObjectLampDrop
 * WHAT IT DOES: A dropping lamp: +$48 up $10; the count +$5F stepped
 * (ObjectCountStep5F with $1F58 = 8, $1F59 = $10, $1F5A = 5); when done,
 * +$48 = 0 and handler $08:E61C (ObjectLampWait); placed on its parent
 * either way.
 * HOW: Ends in the placement at $08:E682.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as the placement leaves it.
 */
void ObjectLampDrop(Carried *cr)
{
    uint32_t b = db(cr);
    uint8_t p, v;

    cyc(30);
    v = rd8(b | (uint16_t)(0x2048 + cr->x));
    p = (uint8_t)(cr->p & ~P_C);
    v = alu_adc8(v, 0x10, &p);
    set_p(cr, p);
    lda8(cr, v);
    cyc(30);
    wr8(b | (uint16_t)(0x2048 + cr->x), v);
    lda8(cr, 0x08);
    cyc(10);
    wr8(b | 0x1F58, 0x08);
    lda8(cr, 0x10);
    cyc(10);
    wr8(b | 0x1F59, 0x10);
    lda8(cr, 0x05);
    cyc(10);
    wr8(b | 0x1F5A, 0x05);
    (cyc(46), ObjectCountStep5F_cr(cr));
    if (cr->p & P_C) {
        lda8(cr, 0x00);
        cyc(32);
        wr8(b | (uint16_t)(0x2048 + cr->x), 0x00);
        set_handler(cr, cr->x, 0xE61C);
    }
    cyc(16);
    lamp_place(cr);
}

void abi_ObjectLampDrop(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLampDrop(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E72C ObjectLampDropB
 * WHAT IT DOES: A bouncing lamp: the count +$5F stepped
 * (ObjectCountStep5F with $1F58 = 3, $1F59 = 6, $1F5A = $0A); when done,
 * +$49 = 0, handler $08:E61C (ObjectLampWait) and +$25 bit 0 cleared;
 * placed on its parent either way.
 * HOW: Ends in the placement at $08:E682. Reached only by
 * unit_entries_08.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as the placement leaves it.
 */
void ObjectLampDropB(Carried *cr)
{
    uint32_t b = db(cr);

    lda8(cr, 0x03);
    cyc(10);
    wr8(b | 0x1F58, 0x03);
    lda8(cr, 0x06);
    cyc(10);
    wr8(b | 0x1F59, 0x06);
    lda8(cr, 0x0A);
    cyc(10);
    wr8(b | 0x1F5A, 0x0A);
    (cyc(46), ObjectCountStep5F_cr(cr));
    if (cr->p & P_C) {
        cyc(52);
        wr8(b | (uint16_t)(0x2049 + cr->x), 0x00);
        set_handler(cr, cr->x, 0xE61C);
        cyc(30);
        lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->x)) & 0xFE));
        cyc(16);
        wr8(b | (uint16_t)(0x2025 + cr->x), (uint8_t)cr->a);
    } else { cyc(24); }
    cyc(16);
    lamp_place(cr);
}

void abi_ObjectLampDropB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLampDropB(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E712 ObjectLampDropBStart
 * WHAT IT DOES: A lamp starts bouncing: handler $08:E72C
 * (ObjectLampDropB), +$49 = 0, +$25 bit 0; then its first frame.
 * HOW: Falls into ObjectLampDropB. Reached only by unit_entries_08.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectLampDropB.
 */
void ObjectLampDropBStart(Carried *cr)
{
    uint32_t b = db(cr);

    set_handler(cr, cr->x, 0xE72C);
    cyc(30);
    wr8(b | (uint16_t)(0x2049 + cr->x), 0x00);
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->x)) | 0x01));
    cyc(16);
    wr8(b | (uint16_t)(0x2025 + cr->x), (uint8_t)cr->a);
    ObjectLampDropB(cr);
}

void abi_ObjectLampDropBStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLampDropBStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/* A bank $00 direct page word at the pointer X (D = $0000): dp,X. */
static uint32_t dpx(Carried *cr, uint16_t off, uint16_t x)
{
    return (uint16_t)(cr->d + off + x);
}

/*
 * @routine 08:9A4F CameraPointFromTable
 * WHAT IT DOES: Steps the camera point of object X (its link +$4F, a
 * pointer Y): unless the point's word has bit 5, when the object's +$25
 * bit 0 is set (sound $24) the point's index ($34,Y) moves one through the
 * course's list (from $09:8090 + $32,Y: count, start), back when +$25
 * bit 1 asks; then the point's flags ($00,Y) and target offset ($02, $08,
 * $94) from the list's entry (index * 8 + 2; entry 0 while the parent is
 * loose, with +$27 = 7). A point with bit 7 counts the object's +$27
 * down, with flags $8201 and offset (0, 0, -$32), or at 0 gets bit 14.
 * The target is the offset plus the point's +$26/+$28/+$2A and
 * +$2C/+$2E/+$30; the point's position (+$1A, +$1C, +$1E) moves a quarter
 * of the way (ApproachQuarterMin4) and the target is kept at +$20, +$22,
 * +$24.
 * HOW: The point through direct page + X with X and Y swapped (PHX, TYX,
 * PLY) and swapped back at the end.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X = the object, Y
 * = the point.
 */
void CameraPointFromTable(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t obj = cr->x, pt, x, w, i;
    uint8_t v, n;
    static const uint16_t pos[3] = {0x1A, 0x1C, 0x1E}, tgt[3] = {0x20, 0x22, 0x24};
    static const uint16_t off[3] = {0x02, 0x08, 0x94}, a1[3] = {0x26, 0x28, 0x2A}, a2[3] = {0x2C, 0x2E, 0x30};

    cyc(30);
    pt = rd16(b | (uint16_t)(0x204F + obj));
    cr->y = pt;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, b | pt);
    r_and16(cr, 0x0020);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (!cr->a) {
        cyc(90);
        lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + obj)) & 0x01));
        if ((uint8_t)cr->a) {
            cyc(38);
            lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + obj)) & 0xFE));
            cyc(16);
            wr8(b | (uint16_t)(0x2025 + obj), (uint8_t)cr->a);
            cyc(48);
            lda8(cr, 0x24);
            SoundPort0QueuePush_cr(cr);
            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
            cyc(82);
            r_lda16(cr, b | (uint16_t)(0x0032 + cr->y));
            r_tax16(cr);
            set_p(cr, (uint8_t)(cr->p | (P_M)));
            cyc(70);
            r_lda8(cr, 0x098090 + cr->x);
            wr8(b | 0x1F2B, (uint8_t)cr->a);
            cyc(64);
            r_lda8(cr, 0x098091 + cr->x);
            wr8(b | 0x1F2D, (uint8_t)cr->a);
            cr->x = obj;
            cyc(66);
            lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + obj)) & 0x02));
            if ((uint8_t)cr->a) {
                cyc(38);
                lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + obj)) & 0xFD));
                cyc(16);
                wr8(b | (uint16_t)(0x2025 + obj), (uint8_t)cr->a);
                cyc(24);
                r_lda8(cr, b | 0x1F2D);
                n = (uint8_t)cr->a;
                if (n) {
                    uint8_t p = (uint8_t)(cr->p & ~P_C);

                    cyc(54);
                    v = alu_adc8(n, rd8(b | 0x1F2B), &p);
                    v--;
                    set_p(cr, p);
                    cyc(44);
                    w = rd8(b | (uint16_t)(0x0034 + cr->y));
                    if (v == (uint8_t)w)
                        { cyc(32); v = 0x00; }
                    lda8(cr, v);
                    goto store;
                }
            }
            cyc(64);
            v = (uint8_t)(rd8(b | (uint16_t)(0x0034 + cr->y)) + 1);
            cyc(38);
            n = rd8(b | 0x1F2B);
            if (!((uint8_t)(v - n) & 0x80))
                { cyc(10); v = 0x00; }
            lda8(cr, v);
store:
            cyc(22);
            wr8(b | (uint16_t)(0x0034 + cr->y), v);
            cyc(28);
            sta16_imm(cr, b | (uint16_t)(0x2027 + obj), 0x0007);
        } else { cyc(12); }
    } else { cyc(44); }
    /* $08:9ACD: the entry by the parent's +$25 bit 2. */
    cyc(82);
    cr->y = rd16(b | (uint16_t)(0x2006 + obj));
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->y)) & 0x04));
    cr->y = pt;
    set_p(cr, nz16(cr->p, pt));
    if ((uint8_t)cr->a) {
        cyc(72);
        sta16_imm(cr, b | (uint16_t)(0x2027 + obj), 0x0007);
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        x = 0x0000;
        cyc(106);                       /* LDX #0, BRA to the join */
    } else {
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cyc(152);
        r_lda16(cr, b | (uint16_t)(0x0034 + cr->y));
        r_and16(cr, 0x00FF);
        r_asl16(cr);
        r_asl16(cr);
        r_asl16(cr);
        cyc(108);
        r_add16(cr, rd16(b | (uint16_t)(0x0032 + cr->y)));
        x = cr->a;
    }
    x = (uint16_t)(x + 2);
    cyc(120);
    r_lda16(cr, 0x098090 + x);
    wr16(b | cr->y, cr->a);
    cyc(64);
    wr16(0x000002, rd16(0x098092 + x));
    cyc(64);
    wr16(0x000008, rd16(0x098094 + x));
    cyc(64);
    r_lda16(cr, 0x098096 + x);
    wr16(0x000094, cr->a);
    /* PHX, TYX, PLY: X = the point, Y = the object. */
    cr->x = pt;
    cr->y = obj;
    r_lda16(cr, cyc_then(138, dpx(cr, 0x00, cr->x)));
    if (cr->a & 0x0080) {
        cyc(70);
        r_lda16(cr, b | (uint16_t)(0x2027 + cr->y));
        if (cr->a) {
            w = (uint16_t)(cr->a - 1);
            cyc(60);
            wr16(b | (uint16_t)(0x2027 + cr->y), w);
            r_lda16(cr, cyc_then(22, dpx(cr, 0x00, cr->x)));
            cr->a |= 0x8201;
            wr16(dpx(cr, 0x00, cr->x), cyc_then(46, cr->a));
            cyc(16);
            wr16(0x000002, 0x0000);
            cyc(16);
            wr16(0x000008, 0x0000);
            r_lda16_imm(cr, 0xFFCE);
            cyc(2);
            wr16(0x000094, 0xFFCE);
            cyc(22);                    /* BRA */
        } else {
            r_lda16(cr, cyc_then(44, dpx(cr, 0x00, cr->x)));
            cr->a |= 0x4000;
            wr16(dpx(cr, 0x00, cr->x), cyc_then(46, cr->a));
        }
    } else { cyc(46); }
    for (i = 0; i < 3; i++) {
        cyc(60);
        r_lda16(cr, off[i]);
        r_add16(cr, rd16(cyc_then(36, dpx(cr, a1[i], cr->x))));
        cyc(16);
        wr16(off[i], cr->a);
    }
    for (i = 0; i < 3; i++) {
        cyc(60);
        r_lda16(cr, off[i]);
        r_add16(cr, rd16(cyc_then(36, dpx(cr, a2[i], cr->x))));
        cyc(16);
        wr16(off[i], cr->a);
    }
    for (i = 0; i < 3; i++) {
        wr16(0x00003E, cyc_then(16, rd16(cyc_then(66, dpx(cr, pos[i], cr->x)))));
        cyc(16);
        r_lda16(cr, off[i]);
        r_approach(cr, ApproachQuarterMin4);
        wr16(dpx(cr, pos[i], cr->x), cyc_then(22, cr->a));
    }
    for (i = 0; i < 3; i++) {
        cyc(60);
        r_lda16(cr, off[i]);
        wr16(dpx(cr, tgt[i], cr->x), cyc_then(22, cr->a));
    }
    cyc(146);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->x = obj;
    cr->y = pt;
    set_p(cr, nz16(cr->p, pt));
}

void abi_CameraPointFromTable(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraPointFromTable(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ObjectSet5DByKind(void);

/* The ring's four parts: kinds, X velocities. */
static const uint16_t ring_kind[4] = {0xA660, 0xA6D0, 0xA67C, 0xA5D4};
static const uint16_t ring_vx[4] = {0xFF90, 0xFFDB, 0x0025, 0x0070};

/* $08:9C4F: in engine phase 8 ($0DD6) with a record in hand ($70:2476):
   its Y = the record's +$20 - $1B58 and +$27 = the record's +$20 - $1F4
   ($1F4C, $1F4E); four parts (handler $08:9DF1, ObjectRingPartStart)
   counted in $1F58; with all four +$16 = $40, +$15 + $20 and handler
   $08:9DBE (ObjectRingSpin), else handler $08:9DAE (ObjectRingIdle).
   +$36 = 3 unless it went idle. */
static void ring_spawn(Carried *cr, unsigned lead)
{
    uint32_t b = db(cr);
    uint16_t x = cr->x;
    unsigned i;
    uint8_t v, p;

    cyc(lead);
    r_lda8(cr, b | 0x0DD6);
    cyc(20);
    cmp8n(cr, (uint8_t)cr->a, 0x08);
    if ((uint8_t)cr->a != 0x08)
        { cyc(37); goto done; }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(67);
    r_lda16(cr, 0x702476);
    if (!cr->a) {
        /* With no matched record the original branches to 08:9DA8 with M
           still 16-bit, where LDA #$03, STA $2036,X decode as LDA #$9D03,
           ROL $20,X (a word, direct page), then the RTL. */
        uint16_t at, w;

        cr->a = 0x9D03;
        set_p(cr, nz16(cr->p, 0x9D03));
        at = (uint16_t)(cr->d + 0x20 + cr->x);
        cyc(92);
        w = rd16(at);
        v = (uint8_t)(cr->p & P_C);
        set_p(cr, (uint8_t)((cr->p & ~P_C) | (w >> 15)));
        w = (uint16_t)(w << 1 | v);
        cyc(14);
        wr16(at, w);
        set_p(cr, nz16(cr->p, w));
        cyc(44);                        /* RTL */
        return;
    }
    cr->x = cr->a;
    cyc(98);
    r_lda16(cr, 0x700020 + cr->x);
    cyc(24);
    wr16(b | 0x1F4E, cr->a);
    r_add16(cr, 0xE4A8);
    cyc(62);
    wr16(b | 0x1F4C, cr->a);
    cr->x = x;
    cyc(104);
    r_lda16(cr, b | 0x1F4C);
    cyc(30);
    wr16(b | (uint16_t)(0x200E + cr->x), cr->a);
    cyc(68);
    r_lda16(cr, b | 0x1F4E);
    r_add16(cr, 0xFE0C);
    cyc(62);
    wr16(b | 0x1F4E, cr->a);
    cyc(68);
    r_lda16(cr, b | 0x1F4E);
    cyc(30);
    wr16(b | (uint16_t)(0x2027 + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(46);
    wr8(b | 0x1F58, 0x00);
    for (i = 0; i < 4; i++) {
        cyc(22);
        if (spawn(cr, ring_kind[i])) {
            set_handler(cr, cr->y, 0x9DF1);
            cyc(28);
            sta16_imm(cr, b | (uint16_t)(0x202E + cr->y), ring_vx[i]);
            (cyc(22), set_child_of_x(cr));
            cyc(74);
            v = (uint8_t)(rd8(b | 0x1F58) + 1);
            cyc(6);
            wr8(b | 0x1F58, v);
        } else { cyc(26); }
    }
    cyc(24);
    r_lda8(cr, b | 0x1F58);
    if ((uint8_t)cr->a != 0x04) {
        cyc(38);
        set_handler(cr, cr->x, 0x9DAE);
        cyc(44);                        /* RTL */
        return;
    }
    lda8(cr, 0x40);
    cyc(48);
    wr8(b | (uint16_t)(0x2016 + cr->x), 0x40);
    cyc(30);
    v = rd8(b | (uint16_t)(0x2015 + cr->x));
    p = (uint8_t)(cr->p & ~P_C);
    v = alu_adc8(v, 0x20, &p);
    set_p(cr, p);
    cyc(60);
    wr8(b | (uint16_t)(0x2015 + cr->x), v);
    set_handler(cr, cr->x, 0x9DBE);
done:
    lda8(cr, 0x03);
    cyc(16);
    wr8(b | (uint16_t)(0x2036 + cr->x), 0x03);
    cyc(44);
}

/* A ring's start: heading +$15 and velocity, then $08:9C4F. */
static void ring_start(Carried *cr, uint8_t h, uint16_t vx, uint16_t vy, uint16_t vz,
                       unsigned lead)
{
    uint32_t b = db(cr);

    lda8(cr, h);
    cyc(30);
    wr8(b | (uint16_t)(0x2015 + cr->x), h);
    cyc(28);
    sta16_imm(cr, b | (uint16_t)(0x202E + cr->x), vx);
    cyc(6);
    sta16_imm(cr, b | (uint16_t)(0x2030 + cr->x), vy);
    cyc(6);
    sta16_imm(cr, b | (uint16_t)(0x2032 + cr->x), vz);
    ring_spawn(cr, lead);
}

/*
 * @routine 08:9BD9 ObjectRingStartZ
 * WHAT IT DOES: A spinning ring's first frame heading $00 with velocity
 * (0, 0, -$258): in engine phase 8 its Y from the record in hand and its
 * four parts made ($08:9C4F); +$36 = 3.
 * HOW: NOP, then the shared start at $08:9C4F. With no record in hand
 * ($70:2476 = 0) the start branches to 08:9DA8 with M still 16-bit, so
 * its LDA #$03, STA $2036,X run as LDA #$9D03, ROL $20,X (direct page,
 * a word) before the RTL.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3; on
 * the no-record path M 16-bit, A = $9D03, the flags from the ROL.
 */
void ObjectRingStartZ(Carried *cr)
{
    ring_start(cr, 0x00, 0x0000, 0x0000, 0xFDA8, 68); /* SEP, BRA, LDA */
}

void abi_ObjectRingStartZ(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRingStartZ(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9BF7 ObjectRingStartX
 * WHAT IT DOES: As ObjectRingStartZ heading $C0 with velocity (-$258, 0,
 * 0).
 * HOW: As ObjectRingStartZ.
 * Entry: as ObjectRingStartZ. Exit: as ObjectRingStartZ.
 */
void ObjectRingStartX(Carried *cr)
{
    ring_start(cr, 0xC0, 0xFDA8, 0x0000, 0x0000, 68);
}

void abi_ObjectRingStartX(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRingStartX(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9C15 ObjectRingStartBackZ
 * WHAT IT DOES: As ObjectRingStartZ heading $80 with velocity (0, 0,
 * $258).
 * HOW: As ObjectRingStartZ.
 * Entry: as ObjectRingStartZ. Exit: as ObjectRingStartZ.
 */
void ObjectRingStartBackZ(Carried *cr)
{
    ring_start(cr, 0x80, 0x0000, 0x0000, 0x0258, 68);
}

void abi_ObjectRingStartBackZ(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRingStartBackZ(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9C33 ObjectRingStartBackX
 * WHAT IT DOES: As ObjectRingStartZ heading $40 with velocity ($258, 0,
 * 0).
 * HOW: As ObjectRingStartZ.
 * Entry: as ObjectRingStartZ. Exit: as ObjectRingStartZ.
 */
void ObjectRingStartBackX(Carried *cr)
{
    ring_start(cr, 0x40, 0x0258, 0x0000, 0x0000, 46); /* falls through, no BRA */
}

void abi_ObjectRingStartBackX(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRingStartBackX(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9DAE ObjectRingIdle
 * WHAT IT DOES: A ring whose parts were not all made: keeps this handler
 * and does nothing more.
 * HOW: Stores its own address. Not reached in the traces so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 8.
 */
void ObjectRingIdle(Carried *cr)
{
    set_handler(cr, cr->x, 0x9DAE);
    cyc(44);                            /* RTL */
}

void abi_ObjectRingIdle(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRingIdle(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9DBE ObjectRingSpin
 * WHAT IT DOES: A ring's frame: moved by its velocity, its Y a quarter of
 * the way to +$27 (ApproachQuarterMin4), its +$16 down 6 and +$15 down 1
 * (it spins), rebucketed; +$36 = 3.
 * HOW: 8-bit adds of $FA and $FF.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
 */
void ObjectRingSpin(Carried *cr)
{
    uint32_t b = (cyc(62), db(cr));
    uint8_t v, p;

    ObjectApplyVelocity_cr(cr);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    wr16(0x00003E, cyc_then(16, rd16(b | (uint16_t)(0x200E + cr->x))));
    cyc(30);
    r_lda16(cr, b | (uint16_t)(0x2027 + cr->x));
    r_approach(cr, ApproachQuarterMin4);
    cyc(30);
    wr16(b | (uint16_t)(0x200E + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(52);
    v = rd8(b | (uint16_t)(0x2016 + cr->x));
    p = (uint8_t)(cr->p & ~P_C);
    v = alu_adc8(v, 0xFA, &p);
    cyc(60);
    wr8(b | (uint16_t)(0x2016 + cr->x), v);
    cyc(30);
    v = rd8(b | (uint16_t)(0x2015 + cr->x));
    p &= (uint8_t)~P_C;
    v = alu_adc8(v, 0xFF, &p);
    set_p(cr, p);
    cyc(60);
    wr8(b | (uint16_t)(0x2015 + cr->x), v);
    (cyc(62), ObjectRebucket_cr(cr));
    lda8(cr, 0x03);
    cyc(16);
    wr8(b | (uint16_t)(0x2036 + cr->x), 0x03);
    cyc(44);
}

void abi_ObjectRingSpin(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRingSpin(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9E05 ObjectRingPartStep
 * WHAT IT DOES: A ring part's frame: on its parent at its velocity bytes
 * turned by the parent's +$16 (RotateXY) and +$15 (RotateXZ), times 16;
 * the parent's +$16 and +$15 copied; rebucketed; +$36 = 3; released once
 * its count +$48 runs out, else counted down.
 * HOW: As ObjectAttachTurned's placement without the +$14 turn.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the path leaves
 * it.
 */
void ObjectRingPartStep(Carried *cr)
{
    uint32_t b = db(cr);
    uint8_t v;

    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    offset_on_parent_ex(cr, 4, 0);
    cyc(52);
    r_lda8(cr, b | (uint16_t)(0x2016 + cr->y));
    cyc(30);
    wr8(b | (uint16_t)(0x2016 + cr->x), (uint8_t)cr->a);
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2015 + cr->y));
    cyc(30);
    wr8(b | (uint16_t)(0x2015 + cr->x), (uint8_t)cr->a);
    (cyc(62), ObjectRebucket_cr(cr));
    lda8(cr, 0x03);
    cyc(16);
    wr8(b | (uint16_t)(0x2036 + cr->x), 0x03);
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2048 + cr->x));
    if (!(uint8_t)cr->a) {
        cyc(48);
        ObjectReleaseLong(cr);
        return;
    }
    cyc(52);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2048 + cr->x)) - 1);
    cyc(6);
    wr8(b | (uint16_t)(0x2048 + cr->x), v);
    set_p(cr, nz8(cr->p, v));
    cyc(44);
}

void abi_ObjectRingPartStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRingPartStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9DF1 ObjectRingPartStart
 * WHAT IT DOES: A ring part's first frame: count +$48 = $28, handler
 * $08:9E05 (ObjectRingPartStep); then its first step.
 * HOW: Falls into ObjectRingPartStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRingPartStep.
 */
void ObjectRingPartStart(Carried *cr)
{
    uint32_t b = db(cr);

    lda8(cr, 0x28);
    cyc(16);
    wr8(b | (uint16_t)(0x2048 + cr->x), 0x28);
    set_handler(cr, cr->x, 0x9E05);
    ObjectRingPartStep(cr);
}

void abi_ObjectRingPartStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRingPartStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9ECD ObjectHandlerEmptyC
 * WHAT IT DOES: Nothing: an empty object handler.
 * HOW: RTL. Not reached in the traces so far.
 * Entry: any. Exit: every register unchanged.
 */
/* ObjectHandlerEmptyC on carried registers: no register work. */
void ObjectHandlerEmptyC_cr(Carried *cr)
{
    (void)cr;
    cyc(44);
}

void abi_ObjectHandlerEmptyC(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectHandlerEmptyC_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9EFD ObjectMarkerFollow
 * WHAT IT DOES: A marker on the car named by the ($1168 & 3)th word of
 * the table at $116C: that car's +$27 kept at $1174, the car at $1176;
 * at the car's position (ObjectCopyPositionFrom) $12C higher, rebucketed,
 * +$36 = 3; released when $116A is nonzero.
 * HOW: 16-bit table reads through the data bank. Not reached in the traces
 * so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as the path leaves
 * it.
 */
void ObjectMarkerFollow(Carried *cr)
{
    uint32_t b = db(cr);

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(46);
    r_lda16(cr, b | 0x1168);
    r_and16(cr, 0x0003);
    r_asl16(cr);
    r_tay16(cr);
    cyc(80);
    r_lda16(cr, b | (uint16_t)(0x116C + cr->y));
    r_tay16(cr);
    cyc(44);
    r_lda16(cr, b | (uint16_t)(0x2027 + cr->y));
    cyc(24);
    wr16(b | 0x1176, cr->y);
    cyc(24);
    wr16(b | 0x1174, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(84), ObjectCopyPositionFrom_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, b | (uint16_t)(0x200E + cr->x));
    r_add16(cr, 0xFED4);
    cyc(68);
    wr16(b | (uint16_t)(0x200E + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(84), ObjectRebucket_cr(cr));
    lda8(cr, 0x03);
    cyc(16);
    wr8(b | (uint16_t)(0x2036 + cr->x), 0x03);
    cyc(24);
    r_lda8(cr, b | 0x116A);
    if ((uint8_t)cr->a)
        { cyc(78); ObjectRelease_cr(cr); cyc(44); } else { cyc(66); }   /* RTL */
}

void abi_ObjectMarkerFollow(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectMarkerFollow(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9ECE ObjectMarkerStart
 * WHAT IT DOES: A marker's first frame: data bank $7E, handler $08:9EFD
 * (ObjectMarkerFollow), +$24 bit 3, +$3A = 0, +$5F = $64, +$04 = $A158;
 * then its first frame.
 * HOW: PHA, PLB for the bank; falls into ObjectMarkerFollow. Not reached
 * in the traces so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectMarkerFollow.
 */
void ObjectMarkerStart(Carried *cr)
{
    uint32_t b;

    cyc(36);
    lda8(cr, 0x7E);
    cr->db = 0x7E;
    set_p(cr, nz8(cr->p, 0x7E));
    b = db(cr);
    set_handler(cr, cr->x, 0x9EFD);
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) | 0x08));
    cyc(16);
    wr8(b | (uint16_t)(0x2024 + cr->x), (uint8_t)cr->a);
    lda8(cr, 0x00);
    cyc(16);
    wr8(b | (uint16_t)(0x203A + cr->x), 0x00);
    lda8(cr, 0x64);
    cyc(16);
    wr8(b | (uint16_t)(0x205F + cr->x), 0x64);
    cyc(28);
    sta16_imm(cr, b | (uint16_t)(0x2004 + cr->x), 0xA158);
    (cyc(22), ObjectMarkerFollow(cr));
}

void abi_ObjectMarkerStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectMarkerStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9FB2 ObjectDemoCarStep
 * WHAT IT DOES: A demo car's frame: its look by kind (ObjectSet5DByKind
 * with $00:18D3); while its count +$48 runs it counts down, at 0 its
 * record gets +$1C bit 4; rebucketed, posed from its record
 * (ObjectPoseFromRecord), +$36 = 3 and the record marked (GsuRecordFlag4,
 * the code at $08:CD7E).
 * HOW: The record through +$22.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as GsuRecordFlag4.
 */
void ObjectDemoCarStep(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t x;
    uint8_t v;

    cyc(32);
    r_lda8(cr, 0x0018D3);
    ObjectSet5DByKind_cr(cr);
    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2048 + cr->x));
    if ((uint8_t)cr->a) {
        cyc(52);
        v = (uint8_t)(rd8(b | (uint16_t)(0x2048 + cr->x)) - 1);
        cyc(6);
        wr8(b | (uint16_t)(0x2048 + cr->x), v);
        set_p(cr, nz8(cr->p, v));
        cyc(84);                        /* BRA, JSL */
    } else {
        x = cr->x;
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cyc(130);
        cr->x = rd16(b | (uint16_t)(0x2022 + cr->x));
        cyc(46);
        r_lda16(cr, 0x70001C + cr->x);
        cr->a |= 0x0010;
        set_p(cr, nz16(cr->p, cr->a));
        cyc(56);
        wr16(0x70001C + cr->x, cr->a);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cr->x = x;
        set_p(cr, nz16(cr->p, x));
        cyc(120);
    }
    ObjectRebucket_cr(cr);
    (cyc(62), ObjectPoseFromRecord_cr(cr));
    lda8(cr, 0x03);
    cyc(16);
    wr8(b | (uint16_t)(0x2036 + cr->x), 0x03);
    (cyc(86), GsuRecordFlag4_cr(cr));
    cyc(44);
}

void abi_ObjectDemoCarStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectDemoCarStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The camera $1A64 on car `car` at offset (0, y, z), mode 1. */
static void camera_on(Carried *cr, uint16_t car, uint16_t y, uint16_t z, int by_x)
{
    uint32_t b = db(cr);
    uint16_t p = 0x1A64;

    if (by_x) {
        wr16(dpx(cr, 0x02, p), cyc_then(22, car));
        wr16(dpx(cr, 0x1A, p), cyc_then(46, 0x0000));
        wr16(dpx(cr, 0x1C, p), cyc_then(46, y));
        wr16(dpx(cr, 0x1E, p), cyc_then(46, z));
        wr8(dpx(cr, 0x00, p), cyc_then(60, 0x01));
    } else {
        cyc(90);
        wr16(b | (uint16_t)(0x0002 + p), car);
        cyc(54);
        wr16(b | (uint16_t)(0x001A + p), 0x0000);
        cyc(54);
        wr16(b | (uint16_t)(0x001C + p), y);
        cyc(54);
        wr16(b | (uint16_t)(0x001E + p), z);
        cyc(68);
        wr8(b | p, 0x01);
    }
    cyc(14);
    cr->a = z;                                  /* the last LDA # (16-bit) */
}

/*
 * @routine 08:9F3B ObjectDemoCarStart
 * WHAT IT DOES: A demo car's first frame: a car record (ObjectCarRecordNew
 * with A = 4) with +$70 bit 3 and +$16 bit 7; the camera ($1A64) on it at
 * (0, -$C8, $258), mode 1; handler $08:9FB2 (ObjectDemoCarStep); its
 * number in the table at $116C ($116A counted up); count +$48 = 4; then
 * its first step.
 * HOW: Falls into ObjectDemoCarStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectDemoCarStep.
 */
void ObjectDemoCarStart(Carried *cr)
{
    uint32_t b = (cyc(70), db(cr));
    uint16_t x = cr->x, n;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16_imm(cr, 0x0004);
    ObjectCarRecordNew(cr);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, b | (uint16_t)(0x2022 + cr->x));
    if (cr->a) {
        x = cr->x;
        cr->x = cr->a;
        cyc(92);
        r_lda16(cr, 0x700070 + cr->x);
        cr->a |= 0x0008;
        cyc(56);
        wr16(0x700070 + cr->x, cr->a);
        cyc(32);
        r_lda16(cr, 0x700016 + cr->x);
        cr->a |= 0x0080;
        set_p(cr, nz16(cr->p, cr->a));
        cyc(56);
        wr16(0x700016 + cr->x, cr->a);
        cr->x = x;
    }
    cyc(36);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cr->y = 0x1A64;
    camera_on(cr, cr->x, 0xFF38, 0x0258, 0);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(8);
    set_handler(cr, cr->x, 0x9FB2);
    cyc(24);
    r_lda8(cr, b | 0x116A);
    n = (uint16_t)(((uint8_t)cr->a << 1) & 0x00FF);
    x = cr->x;
    cyc(172);
    wr16(0x00116C + n, x);
    cr->x = x;
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(60);
    wr8(b | 0x116A, cyc_then(6, (uint8_t)(rd8(b | 0x116A) + 1)));
    lda8(cr, 0x04);
    cyc(16);
    wr8(b | (uint16_t)(0x2048 + cr->x), 0x04);
    ObjectDemoCarStep(cr);
}

void abi_ObjectDemoCarStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectDemoCarStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:9FEC CameraOnCarNear
 * WHAT IT DOES: The camera ($1A64) on object X at (0, -$1F4, $258), mode
 * 1.
 * HOW: Stores through Y = $1A64. Not reached in the traces so far.
 * Entry: X 16-bit; X = object. Exit: M 8-bit, X 16-bit; Y = $1A64; A low
 * byte 1.
 */
void CameraOnCarNear(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cr->y = 0x1A64;
    camera_on(cr, cr->x, 0xFE0C, 0x0258, 0);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    lda8(cr, 0x01);
}

void abi_CameraOnCarNear(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraOnCarNear(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:A00F CameraOnMarkedCar
 * WHAT IT DOES: The camera ($1A64) on the car at $1176 at (0, -$C8, $2BC),
 * mode 1.
 * HOW: Stores through direct page + X = $1A64. Not reached in the traces
 * so far.
 * Entry: any. Exit: M 8-bit, X 16-bit; X = $1A64, Y = the car; A low byte
 * 1.
 */
void CameraOnMarkedCar(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
    cr->x = 0x1A64;
    cr->y = rd16(cyc_then(70, db(cr) | 0x1176));
    camera_on(cr, cr->y, 0xFF38, 0x02BC, 1);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    lda8(cr, 0x01);
}

void abi_CameraOnMarkedCar(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraOnMarkedCar(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:A02F DataBank7E
 * WHAT IT DOES: Sets the data bank to $7E.
 * HOW: LDA #$7E, PHA, PLB; ends at the RTL at $08:A033 that other code
 * jumps to. Not reached in the traces so far.
 * Entry: M 8-bit. Exit: data bank $7E; A low byte $7E; N clear, Z clear.
 */
void DataBank7E(Carried *cr)
{
    cyc(80);
    lda8(cr, 0x7E);
    cr->db = 0x7E;
    set_p(cr, nz8(cr->p, 0x7E));
}

void abi_DataBank7E(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DataBank7E(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:A033 DataBank7EEnd
 * WHAT IT DOES: Nothing: DataBank7E's RTL alone, where code that has
 * nothing to do jumps to return.
 * HOW: RTL.
 * Entry: any. Exit: every register unchanged.
 */
/* DataBank7EEnd on carried registers: no register work. */
void DataBank7EEnd_cr(Carried *cr)
{
    (void)cr;
    cyc(44);                            /* RTL */
}

void abi_DataBank7EEnd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DataBank7EEnd_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ObjectPathCommand(void);
void abi_CameraFollowStep(void);
void abi_ObjectFaceTied(void);

/*
 * @routine 08:89E6 CameraApproachTarget
 * WHAT IT DOES: Moves object X's camera point (its link +$4F) an eighth of
 * the way (at least 8: ApproachEighthMin8) from its position (+$1A, +$1C,
 * +$1E) to its target (+$20, +$22, +$24); while the object's count +$27
 * runs, the target moves by the object's velocity and C is clear;
 * otherwise the object's path's next command (ObjectPathCommand) decides
 * C.
 * HOW: The point through the data bank with Y; 16-bit.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; C as above; Y = the
 * point.
 */
void CameraApproachTarget(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t w;
    unsigned i;
    static const uint16_t pos[3] = {0x1A, 0x1C, 0x1E}, tgt[3] = {0x20, 0x22, 0x24};

    cyc(92);
    cr->y = rd16(b | (uint16_t)(0x204F + cr->x));
    for (i = 0; i < 3; i++) {
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cyc(52);
        wr16(0x00003E, cyc_then(16, rd16(b | (uint16_t)(pos[i] + cr->y))));
        cyc(30);
        r_lda16(cr, b | (uint16_t)(tgt[i] + cr->y));
        (cyc(14), r_approach(cr, ApproachEighthMin8));
        cyc(30);
        wr16(b | (uint16_t)(pos[i] + cr->y), cr->a);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(22);
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, b | (uint16_t)(0x2027 + cr->x));
    if (cr->a) {
        w = (uint16_t)(cr->a - 1);
        cr->a = w;
        cyc(60);
        wr16(b | (uint16_t)(0x2027 + cr->x), w);
        for (i = 0; i < 3; i++) {
            cyc(30);
            r_lda16(cr, b | (uint16_t)(0x202E + i * 2 + cr->x));
            cyc(44);
            r_add16(cr, rd16(b | (uint16_t)(tgt[i] + cr->y)));
            cyc(30);
            wr16(b | (uint16_t)(tgt[i] + cr->y), cr->a);
        }
        cyc(102);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
        return;
    } else { cyc(22); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    w_ObjectPathCommand(cr);
}

void abi_CameraApproachTarget(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraApproachTarget(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:958F CameraFollowLink
 * WHAT IT DOES: Steps object X's camera point (its link +$4F): it follows
 * its target (CameraFollowStep) and faces it (ObjectFaceTied).
 * HOW: The point in X between them; X kept (PHX, PLX).
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X = the object.
 */
void CameraFollowLink(Carried *cr)
{
    uint16_t x = cr->x;

    cr->y = rd16(cyc_then(60, db(cr) | (uint16_t)(0x204F + cr->x)));
    cr->x = cr->y;
    set_p(cr, nz16(cr->p, cr->x));
    (cyc(76), CameraFollowStep_cr(cr));
    (cyc(62), ObjectFaceTied_cr(cr));
    cyc(102);                           /* PLX, SEP, RTL */
    cr->x = x;
    set_p(cr, (uint8_t)(nz16(cr->p, x) | P_M));
}

void abi_CameraFollowLink(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraFollowLink(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_GsuTableEntryNext(void);
void abi_GsuTableEntryTo1F4C(void);
void abi_ObjectPathStart(void);

/* $08:8EF9: the camera point steps (CameraFollowLink), +$36 = 3. */
static void director_tail(Carried *cr)
{
    cyc(62);
    CameraFollowLink(cr);
    lda8(cr, 0x03);
    wr8(db(cr) | (uint16_t)(0x2036 + cr->x), cyc_then(16, 0x03));
    cyc(44);
}

/* LDY +$4F,X; REP #$20; LDA #v; STA $0000,Y; SEP #$20: the camera
   point's mode. */
static void director_mode(Carried *cr, uint16_t v)
{
    uint32_t b = db(cr);

    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x204F + cr->x));
    cyc(28);
    sta16_imm(cr, b | cr->y, v);
}

/* PHX, the parent's (+$06) record +$58, PLX, then GsuTableEntryNext n
   times: an entry of the course's camera table. */
static void director_entry(Carried *cr, unsigned n)
{
    uint32_t b = db(cr);
    uint16_t x = cr->x;

    cyc(60);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    cr->x = rd16(b | (uint16_t)(0x2022 + cr->y));
    cyc(46);
    r_lda16(cr, 0x700058 + cr->x);
    cr->x = x;
    cyc(36);
    while (n--)
        { cyc(62); GsuTableEntryNext_cr(cr); }
}

/* STA $1F2B, GsuTableEntryTo1F4C, then the point's $08/$0A/$0C =
   $1F4C/$1F4E/$1F50 plus (dx, dy, dz). */
static void director_place(Carried *cr, uint16_t dx, uint16_t dy, uint16_t dz)
{
    uint32_t b = db(cr);

    cyc(24);
    wr16(b | 0x1F2B, cr->a);
    (cyc(62), GsuTableEntryTo1F4C_cr(cr));
    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x204F + cr->x));
    cyc(24);
    r_lda16(cr, b | 0x1F4C);
    if (dx)
        { cyc(38); r_add16(cr, dx); }
    cyc(30);
    wr16(b | (uint16_t)(0x0008 + cr->y), cr->a);
    cyc(24);
    r_lda16(cr, b | 0x1F4E);
    r_add16(cr, dy);
    cyc(68);
    wr16(b | (uint16_t)(0x000A + cr->y), cr->a);
    cyc(24);
    r_lda16(cr, b | 0x1F50);
    if (dz)
        { cyc(38); r_add16(cr, dz); }
    cyc(30);
    wr16(b | (uint16_t)(0x000C + cr->y), cr->a);
}

/* +$27 and the velocity cleared (four 16-bit stores). */
static void director_still(Carried *cr)
{
    uint32_t b = (cyc(50), db(cr));

    sta16_imm(cr, b | (uint16_t)(0x2027 + cr->x), 0x0000);
    cyc(50);
    sta16_imm(cr, b | (uint16_t)(0x202E + cr->x), 0x0000);
    cyc(50);
    sta16_imm(cr, b | (uint16_t)(0x2030 + cr->x), 0x0000);
    cyc(50);
    sta16_imm(cr, b | (uint16_t)(0x2032 + cr->x), 0x0000);
}

/* (A - the point's word) >> 5, arithmetic (CMP #$8000, ROR A five
   times). */
static uint16_t director_slope(Carried *cr, uint16_t a)
{
    int i;

    for (i = 0; i < 5; i++)
        a = (uint16_t)((a >> 1) | (a & 0x8000));
    cr->a = a;
    set_p(cr, nz16(cr->p, a));
    return a;
}

/*
 * @routine 08:8D2D CameraDirectorTrack
 * WHAT IT DOES: The replay camera on the course's camera table: when the
 * parent's record +$58 moves to a new entry (not 0, not the point's $04,
 * low two bits clear) the point's $04 = it, the camera ($08, $0A, $0C) at
 * the next entry's place + ($3E8, -$5DC, $3E8), +$27 = $18 and a velocity
 * a thirty-second of the way to the entry after; then while +$27 runs the
 * camera rises $19 a frame, and it moves by the velocity (X and Z; the Y
 * add is not stored); then the point's step ($08:8EF9).
 * HOW: GsuTableEntryNext and GsuTableEntryTo1F4C through the record's
 * table; 16-bit.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
 */
void CameraDirectorTrack(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t e;

    director_entry(cr, 0);
    e = cr->a;
    r_cmp16(cr, 0x0000);
    if (e) {
        cyc(76);
        cr->y = rd16(b | (uint16_t)(0x204F + cr->x));
        cyc(30);
        r_cmp16(cr, rd16(b | (uint16_t)(0x0004 + cr->y)));
        if (!(cr->p & P_Z) && (cyc(46), !(e & 0x0003))) {
            cyc(52);
            wr16(b | (uint16_t)(0x0004 + cr->y), e);
            (cyc(62), GsuTableEntryNext_cr(cr));
            director_place(cr, 0x03E8, 0xFA24, 0x03E8);
            r_lda16_imm(cr, 0x0018);
            cyc(16);
            wr16(b | (uint16_t)(0x2027 + cr->x), 0x0018);
            cyc(24);
            r_lda16(cr, b | 0x1F2B);
            (cyc(62), GsuTableEntryNext_cr(cr));
            (cyc(62), GsuTableEntryTo1F4C_cr(cr));
            cyc(30);
            cr->y = rd16(b | (uint16_t)(0x204F + cr->x));
            cyc(24);
            r_lda16(cr, b | 0x1F4C);
            cyc(44);
            r_sub16(cr, rd16(b | (uint16_t)(0x0008 + cr->y)));
            wr16(b | (uint16_t)(0x202E + cr->x), cyc_then(214, director_slope(cr, cr->a)));
            cyc(24);
            r_lda16(cr, b | 0x1F4E);
            cyc(44);
            r_sub16(cr, rd16(b | (uint16_t)(0x000A + cr->y)));
            wr16(b | (uint16_t)(0x2030 + cr->x), cyc_then(214, director_slope(cr, cr->a)));
            cyc(24);
            r_lda16(cr, b | 0x1F50);
            cyc(44);
            r_sub16(cr, rd16(b | (uint16_t)(0x000C + cr->y)));
            wr16(b | (uint16_t)(0x2032 + cr->x), cyc_then(214, director_slope(cr, cr->a)));
        } else { cyc(46); }
    } else { cyc(70); }
    /* $08:8DF4 */
    cyc(30);
    r_lda16(cr, b | (uint16_t)(0x2027 + cr->x));
    if (cr->a) {
        cyc(60);
        wr16(b | (uint16_t)(0x2027 + cr->x), (uint16_t)(cr->a - 1));
        cyc(30);
        cr->y = rd16(b | (uint16_t)(0x204F + cr->x));
        cyc(30);
        r_lda16(cr, b | (uint16_t)(0x000A + cr->y));
        r_add16(cr, 0x0019);
        cyc(68);
        wr16(b | (uint16_t)(0x000A + cr->y), cr->a);
    } else { cyc(22); }
    cyc(30);
    r_lda16(cr, b | (uint16_t)(0x0008 + cr->y));
    cyc(44);
    r_add16(cr, rd16(b | (uint16_t)(0x202E + cr->x)));
    cyc(30);
    wr16(b | (uint16_t)(0x0008 + cr->y), cr->a);
    cyc(30);
    r_lda16(cr, b | (uint16_t)(0x000A + cr->y));
    cyc(44);
    r_add16(cr, rd16(b | (uint16_t)(0x2030 + cr->x)));
    cyc(30);
    r_lda16(cr, b | (uint16_t)(0x000C + cr->y));
    cyc(44);
    r_add16(cr, rd16(b | (uint16_t)(0x2032 + cr->x)));
    cyc(30);
    wr16(b | (uint16_t)(0x000C + cr->y), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(46), director_tail(cr));
}

void abi_CameraDirectorTrack(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDirectorTrack(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* $08:8CDA: handler $08:8D2D (CameraDirectorTrack), mode 3, $70:273C bit 0
   cleared, still; then its first frame. */
static void director_track_start(Carried *cr)
{
    cyc(22);
    set_handler(cr, cr->x, 0x8D2D);
    director_mode(cr, 0x0003);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(76);
    r_lda16(cr, 0x70273C);
    r_and16(cr, 0xFFFE);
    cyc(54);
    wr16(0x70273C, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    director_still(cr);
    (cyc(22), CameraDirectorTrack(cr));
}

/*
 * @routine 08:8BD7 CameraDirectorHold
 * WHAT IT DOES: The replay camera held: this handler again, mode 3, the
 * point stepped twice (CameraFollowLink, then the tail at $08:8EF9).
 * HOW: Two CameraFollowLink calls.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
 */
void CameraDirectorHold(Carried *cr)
{
    set_handler(cr, cr->x, 0x8BD7);
    director_mode(cr, 0x0003);
    (cyc(84), CameraFollowLink(cr));
    (cyc(32), director_tail(cr));
}

void abi_CameraDirectorHold(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDirectorHold(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:8BFB CameraDirectorSkip3
 * WHAT IT DOES: The replay camera three entries on in the course's camera
 * table at that place + ($3E8, -$1F4, $3E8), then tracking
 * ($08:8CDA, CameraDirectorTrack).
 * HOW: GsuTableEntryNext three times.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as CameraDirectorTrack.
 */
void CameraDirectorSkip3(Carried *cr)
{
    director_entry(cr, 3);
    director_place(cr, 0x03E8, 0xFE0C, 0x03E8);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(24), director_track_start(cr));
}

void abi_CameraDirectorSkip3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDirectorSkip3(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:8C43 CameraDirectorSkip5
 * WHAT IT DOES: The replay camera five entries on, the point's $04 = that
 * entry, at its place + ($3E8, -$1F4, $3E8); then held
 * (CameraDirectorHold).
 * HOW: GsuTableEntryNext five times.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as CameraDirectorHold.
 */
void CameraDirectorSkip5(Carried *cr)
{
    uint32_t b = db(cr);

    director_entry(cr, 5);
    cyc(24);
    wr16(b | 0x1F2B, cr->a);
    (cyc(62), GsuTableEntryTo1F4C_cr(cr));
    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x204F + cr->x));
    cyc(24);
    r_lda16(cr, b | 0x1F2B);
    cyc(30);
    wr16(b | (uint16_t)(0x0004 + cr->y), cr->a);
    cyc(24);
    r_lda16(cr, b | 0x1F4C);
    r_add16(cr, 0x03E8);
    cyc(68);
    wr16(b | (uint16_t)(0x0008 + cr->y), cr->a);
    cyc(24);
    r_lda16(cr, b | 0x1F4E);
    r_add16(cr, 0xFE0C);
    cyc(68);
    wr16(b | (uint16_t)(0x000A + cr->y), cr->a);
    cyc(24);
    r_lda16(cr, b | 0x1F50);
    r_add16(cr, 0x03E8);
    cyc(68);
    wr16(b | (uint16_t)(0x000C + cr->y), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(46), CameraDirectorHold(cr));
}

void abi_CameraDirectorSkip5(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDirectorSkip5(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:8C99 CameraDirectorSkip2
 * WHAT IT DOES: As CameraDirectorSkip3 two entries on.
 * HOW: GsuTableEntryNext twice.
 * Entry: as CameraDirectorSkip3. Exit: as CameraDirectorSkip3.
 */
void CameraDirectorSkip2(Carried *cr)
{
    director_entry(cr, 2);
    director_place(cr, 0x03E8, 0xFE0C, 0x03E8);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    director_track_start(cr);
}

void abi_CameraDirectorSkip2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDirectorSkip2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:8B5B CameraDirectorNext
 * WHAT IT DOES: The replay camera on the next entry of the course's camera
 * table at its place, $C8 up; handler $08:8D2D (CameraDirectorTrack), mode
 * 3, still; then tracking.
 * HOW: GsuTableEntryNext once. Reached only by unit_entries_08.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as CameraDirectorTrack.
 */
void CameraDirectorNext(Carried *cr)
{
    set_handler(cr, cr->x, 0x8D2D);
    director_mode(cr, 0x0003);
    cyc(22);
    director_entry(cr, 1);
    director_place(cr, 0x0000, 0xFF38, 0x0000);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    director_still(cr);
    (cyc(46), CameraDirectorTrack(cr));
}

void abi_CameraDirectorNext(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDirectorNext(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_CameraDirectorFixed(void);

/* $08:8EC1: the point's $04 = 0 and $34 = 0, +$25 bits 0 and 1 cleared,
   handler $08:8EF5 (CameraDirectorTable); then its first frame. */
void abi_CameraDirectorTable(void);

static void director_table_start(Carried *cr, uint32_t lead)
{
    uint32_t b = db(cr);

    cyc(lead);
    cr->y = rd16(b | (uint16_t)(0x204F + cr->x));
    cyc(28);
    sta16_imm(cr, b | (uint16_t)(0x0004 + cr->y), 0x0000);
    cyc(52);
    cr->y = rd16(b | (uint16_t)(0x204F + cr->x));
    lda8(cr, 0x00);
    cyc(16);
    wr8(b | (uint16_t)(0x0034 + cr->y), 0x00);
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->x)) & 0xFE));
    cyc(16);
    wr8(b | (uint16_t)(0x2025 + cr->x), (uint8_t)cr->a);
    cyc(30);
    lda8(cr, (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->x)) & 0xFD));
    cyc(16);
    wr8(b | (uint16_t)(0x2025 + cr->x), (uint8_t)cr->a);
    set_handler(cr, cr->x, 0x8EF5);
    CameraDirectorTable(cr);
}

/*
 * @routine 08:8EF5 CameraDirectorTable
 * WHAT IT DOES: The replay camera from the camera point table
 * (CameraPointFromTable), then the point's step ($08:8EF9).
 * HOW: Two calls.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
 */
void CameraDirectorTable(Carried *cr)
{
    (cyc(62), CameraPointFromTable(cr));
    director_tail(cr);
}

void abi_CameraDirectorTable(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDirectorTable(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:8E51 CameraDirectorPath
 * WHAT IT DOES: The replay camera along its path (CameraApproachTarget);
 * at the path's end it goes to the table ($08:8EC1), else the point's
 * step.
 * HOW: By C from CameraApproachTarget.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as the path leaves it.
 */
void CameraDirectorPath(Carried *cr)
{
    CameraApproachTarget(cr);
    if (cr->p & P_C) {
        cyc(0);
        director_table_start(cr, 76);
        return;
    } else { cyc(46); }
    director_tail(cr);
}

void abi_CameraDirectorPath(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDirectorPath(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:8E2A CameraDirectorPathStart
 * WHAT IT DOES: The replay camera starts on path $87F1 (ObjectPathStart):
 * handler $08:8E51 (CameraDirectorPath), mode 1, the point stepped once;
 * then its first frame.
 * HOW: Falls into CameraDirectorPath.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as CameraDirectorPath.
 */
void CameraDirectorPathStart(Carried *cr)
{
    set_handler(cr, cr->x, 0x8E51);
    director_mode(cr, 0x0001);
    cr->y = 0x87F1;
    set_p(cr, nz16(cr->p, 0x87F1));
    (cyc(108), w_ObjectPathStart(cr));
    (cyc(62), CameraFollowLink(cr));
    CameraDirectorPath(cr);
}

void abi_CameraDirectorPathStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDirectorPathStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:8EA2 CameraDirectorWait
 * WHAT IT DOES: The replay camera waits out its count +$48 (stepping the
 * point), then goes to the table ($08:8EC1).
 * HOW: DEC of the count.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as the path leaves it.
 */
void CameraDirectorWait(Carried *cr)
{
    uint32_t b = db(cr);
    uint8_t v;

    cyc(30);
    r_lda8(cr, b | (uint16_t)(0x2048 + cr->x));
    if (!(uint8_t)cr->a) {
        cyc(2);
        director_table_start(cr, 76);
        return;
    }
    cyc(52);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2048 + cr->x)) - 1);
    cyc(6);
    wr8(b | (uint16_t)(0x2048 + cr->x), v);
    set_p(cr, nz8(cr->p, v));
    (cyc(24), director_tail(cr));
}

void abi_CameraDirectorWait(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDirectorWait(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:8E5D CameraDirectorBehind
 * WHAT IT DOES: The replay camera behind the car: mode $21, target offset
 * (0, -$8C, -$1F4), the code at $08:8F03, handler $08:8EA2
 * (CameraDirectorWait) with count $50; then its first frame.
 * HOW: Falls into CameraDirectorWait; $08:8F03 is CameraSnapToCar.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as CameraDirectorWait.
 */
void CameraDirectorBehind(Carried *cr)
{
    uint32_t b = db(cr);

    director_mode(cr, 0x0021);
    cyc(52);
    cr->y = rd16(b | (uint16_t)(0x204F + cr->x));
    cyc(28);
    sta16_imm(cr, b | (uint16_t)(0x0020 + cr->y), 0x0000);
    cyc(50);
    sta16_imm(cr, b | (uint16_t)(0x0022 + cr->y), 0xFF74);
    cyc(50);
    sta16_imm(cr, b | (uint16_t)(0x0024 + cr->y), 0xFE0C);
    (cyc(68), CameraSnapToCar(cr));
    cyc(0);
    set_handler(cr, cr->x, 0x8EA2);
    lda8(cr, 0x50);
    cyc(16);
    wr8(b | (uint16_t)(0x2048 + cr->x), 0x50);
    CameraDirectorWait(cr);
}

void abi_CameraDirectorBehind(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDirectorBehind(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:8EB1 CameraDirectorReset
 * WHAT IT DOES: The replay camera in mode $655, the code at $08:8F03,
 * then to the table ($08:8EC1).
 * HOW: As CameraDirectorBehind. Not reached in the traces so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as CameraDirectorTable.
 */
void CameraDirectorReset(Carried *cr)
{
    director_mode(cr, 0x0655);
    (cyc(68), CameraSnapToCar(cr));
    director_table_start(cr, 30);
}

void abi_CameraDirectorReset(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDirectorReset(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:8A59 CameraDirectorFixedStart
 * WHAT IT DOES: The replay camera fixed: handler $08:8A75
 * (CameraDirectorFixed), mode 2; then its first frame.
 * HOW: Falls into CameraDirectorFixed. Not reached in the traces so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as CameraDirectorFixed.
 */
void CameraDirectorFixedStart(Carried *cr)
{
    set_handler(cr, cr->x, 0x8A75);
    director_mode(cr, 0x0002);
    (cyc(22), CameraDirectorFixed(cr));
}

void abi_CameraDirectorFixedStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDirectorFixedStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:8A75 CameraDirectorFixed
 * WHAT IT DOES: The fixed replay camera: the point's $04 = 0 and $06 =
 * $7D0; the point's step ($08:8EF9).
 * HOW: Two stores. Not reached in the traces so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte 3.
 */
void CameraDirectorFixed(Carried *cr)
{
    uint32_t b = db(cr);

    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x204F + cr->x));
    lda8(cr, 0x00);
    cyc(16);
    wr8(b | (uint16_t)(0x0004 + cr->y), 0x00);
    cyc(28);
    sta16_imm(cr, b | (uint16_t)(0x0006 + cr->y), 0x07D0);
    (cyc(46), director_tail(cr));
}

void abi_CameraDirectorFixed(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraDirectorFixed(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:8A8A CameraNearestCar
 * WHAT IT DOES: Points the camera ($1A64) at the car nearest object X: on
 * its first frame ($04 = 0) $04 = 1 and mode bit 0 cleared; the distance
 * (half-sums of |dx|, |dy|, |dz| / 4) to the first car of the list at
 * $7E:FFAE, when below the camera's $06, becomes $06 and that car the
 * camera's target ($02) with object X's position at $08/$0A/$0C and mode
 * bit 0 set. +$36 = 3. With no car taken, A = $9D03 and the word at
 * direct page $20 + X is rotated left through C instead.
 * HOW: Only the list's first entry is looked at. With no car taken the
 * code reaches $08:8B55 with A 16-bit, where its bytes read LDA #$9D03,
 * ROL $20,X, RTL.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit, A low byte 3; with
 * no car taken M 16-bit, A = $9D03, N, Z, C from the rotate.
 */
void CameraNearestCar(Carried *cr)
{
    uint32_t b = db(cr);
    uint16_t x = cr->x, p = 0x1A64, car, d, t;

    cr->y = p;
    cyc(54);
    r_lda8(cr, b | (uint16_t)(0x0004 + p));
    if (!(uint8_t)cr->a) {
        lda8(cr, 0x01);
        cyc(32);
        wr8(b | (uint16_t)(0x0004 + p), 0x01);
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cyc(52);
        r_lda16(cr, b | p);
        r_and16(cr, 0xFFFE);
        cyc(52);
        wr16(b | p, cr->a);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(104);
    wr16(b | 0x1F4C, cyc_then(24, rd16(b | (uint16_t)(0x200C + x))));
    cyc(30);
    wr16(b | 0x1F4E, cyc_then(24, rd16(b | (uint16_t)(0x200E + x))));
    cyc(30);
    wr16(b | 0x1F50, cyc_then(24, rd16(b | (uint16_t)(0x2010 + x))));
    cyc(24);
    wr16(b | 0x1F52, 0x0000);
    cyc(30);
    r_lda16(cr, b | (uint16_t)(0x0006 + p));
    cyc(24);
    wr16(b | 0x1F2D, cr->a);
    cyc(56);
    r_lda16(cr, 0x7EFFAE);
    if (cr->a) {
        car = cr->a;
        cr->y = car;
        cyc(60);
        d = rd16(b | (uint16_t)(0x200C + car));
        cyc(38);
        d = abs16((uint16_t)(d - rd16(b | 0x1F4C)));
        d = (uint16_t)((d >> 2) | ((d & 0x8000) ? 0xC000 : 0));
        cyc(122);
        wr16(b | 0x1F2B, d);
        cyc(30);
        t = rd16(b | (uint16_t)(0x200E + car));
        cyc(38);
        t = abs16((uint16_t)(t - rd16(b | 0x1F4E)));
        t = (uint16_t)((t >> 2) | ((t & 0x8000) ? 0xC000 : 0));
        cyc(136);
        t = (uint16_t)(t + rd16(b | 0x1F2B));
        cyc(24);
        wr16(b | 0x1F2B, t);
        cyc(30);
        d = rd16(b | (uint16_t)(0x2010 + car));
        cyc(38);
        d = abs16((uint16_t)(d - rd16(b | 0x1F50)));
        d = (uint16_t)((d >> 2) | ((d & 0x8000) ? 0xC000 : 0));
        cyc(136);
        d = (uint16_t)(d + rd16(b | 0x1F2B));
        cr->a = d;
        cyc(24);
        r_cmp16(cr, rd16(b | 0x1F2D));
        if (cr->p & P_N) {
            cyc(40);
            wr16(b | 0x1F2D, d);
            cyc(24);
            wr16(b | 0x1F52, car);
        } else { cyc(22); }
    }
    cr->x = x;
    cr->y = p;
    cyc(112);
    r_lda16(cr, b | 0x1F2D);
    cyc(30);
    wr16(b | (uint16_t)(0x0006 + p), cr->a);
    cyc(24);
    r_lda16(cr, b | 0x1F52);
    if (!cr->a) {
        t = (uint16_t)(cr->d + 0x20 + x);
        cyc(68);
        d = rd16(t);
        car = (uint16_t)(d << 1 | (cr->p & P_C));
        cr->a = 0x9D03;
        cyc(14);
        wr16(t, car);
        set_p(cr, (uint8_t)(nz16((uint8_t)(cr->p & ~P_C), car) | d >> 15));
        cyc(44);                        /* RTL */
        return;
    }
    cyc(46);
    wr16(b | (uint16_t)(0x0002 + p), cr->a);
    cyc(30);
    wr16(b | (uint16_t)(0x0008 + p), cyc_then(30, rd16(b | (uint16_t)(0x200C + x))));
    cyc(30);
    wr16(b | (uint16_t)(0x000A + p), cyc_then(30, rd16(b | (uint16_t)(0x200E + x))));
    cyc(30);
    wr16(b | (uint16_t)(0x000C + p), cyc_then(30, rd16(b | (uint16_t)(0x2010 + x))));
    cyc(30);
    r_lda16(cr, b | p);
    cr->a |= 0x0001;
    cyc(54);
    wr16(b | p, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    lda8(cr, 0x03);
    cyc(38);
    wr8(b | (uint16_t)(0x2036 + cr->x), 0x03);
    cyc(44);                            /* RTL */
}

void abi_CameraNearestCar(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CameraNearestCar(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* RandomStepLong as its adapter leaves the registers. */
static void w_RandomStepLong(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | (cyc(46), RandomStepLong()));
}

/* ObjectPathCommand as its adapter leaves the registers. */
static void w_ObjectPathCommand(Carried *cr)
{
    uint8_t p = cr->p;

    ObjectPathCommand(cr->db, cr->x, &cr->a, &cr->y, &p);
    set_p(cr, (uint8_t)(nz16(p, cr->x) | P_M));
    cyc(28);
}

/* ObjectPathStart as its adapter leaves the registers. */
static void w_ObjectPathStart(Carried *cr)
{
    uint8_t p = cr->p;

    ObjectPathStart(cr->db, cr->x, cr->y, &cr->a, &cr->y, &p);
    set_p(cr, (uint8_t)(nz16(p, cr->x) | P_M));
}
