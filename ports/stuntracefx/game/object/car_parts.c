/*
 * car_parts.c: the cars' loose parts and the player cars of bank $08: a
 * part's place on its car (+$06, the parent) from the part's offset
 * bytes +$48-$4A turned by the car's angles, the part knocked loose and
 * flying, falling back into place, and the two player cars' set-ups and
 * frames; with the car set-ups that put a car of a kind on the object
 * list.
 *
 * Objects are at $2000 + X through the data bank (X 16-bit, M 8-bit); the
 * handlers live at +$1A/+$1C (bank $08).
 */
#include "machine.h"
#include "math_calls.h"
#include "cpu_ops.h"
#include "carried.h"
#include "wram_names.h"
#include "wram_struct.h"

uint8_t RandomStepLong(void);

void abi_RotateXY(void);
void abi_RotateXZFrom08(void);
void abi_RotateXZ(void);
void abi_ObjectSet5DByKind(void);
void abi_ObjectReleaseLong(void);
void abi_ObjectRelease(void);
void abi_ObjectRebucket(void);
void abi_ObjectCopyPositionFrom(void);
void abi_ObjectCopy14From(void);
void abi_GsuRun(void);
void abi_ObjectVelocityFromRecordOf(void);
void abi_GsuRecordNew426(void);
void abi_GsuRecordVelocity(void);
void abi_ObjectSyncFromRecord(void);
void abi_RecordContactSummary(void);
void abi_ObjectBreakOffContact(void);
void abi_ObjectSpawnChanceD460(void);
void abi_SoundPort0QueuePush(void);
void abi_ObjectApplyVelocity(void);
void abi_ObjectListAppend(void);
void abi_ObjectCarRecordNew(void);
void abi_ObjectAttachLinked(void);
void abi_GsuRecordLand(void);
void abi_PlayerCarEvents(void);
void abi_ObjectPoseFromRecord(void);
void abi_GsuRecordStep6A(void);
void abi_ObjectMoveForward(void);
void abi_ObjectCarSpawn(void);

void CarCrashParts(Carried *cr);
void ObjectCarKind3(Carried *cr);
void ObjectCarKind4(Carried *cr);
void ObjectCarKind5(Carried *cr);
void ObjectCarSpawnListed(Carried *cr);
void ObjectRivalCar(Carried *cr);
void ObjectRivalCarB(Carried *cr);
void ObjectRivalCarStep(Carried *cr);
void PartDetach(Carried *cr);
void PartDetachStep(Carried *cr);
void PartFlyStep(Carried *cr);
void PartHit(Carried *cr);
void PartKnockOff(Carried *cr);
void PartOffset(Carried *cr);
void PartReturn(Carried *cr);
void PartReturnStep(Carried *cr);
void PlayerCar1(Carried *cr);
void PlayerCar1Step(Carried *cr);
void PlayerCar2(Carried *cr);
void PlayerCar2Step(Carried *cr);
static void add16(Carried *cr, uint32_t addr, uint16_t v);
static void add16m(Carried *cr, uint32_t addr, uint32_t by, uint32_t adc);
static void add8(Carried *cr, uint32_t addr, uint8_t v);
static void and8(Carried *cr, uint32_t addr, uint8_t m);
static int axis_from(Carried *cr, uint32_t src, uint32_t lo, uint32_t lo2, uint32_t hi, uint32_t in);
static int bits25(Carried *cr, uint16_t o, uint8_t m);
static int bits25b(Carried *cr, uint16_t o, uint8_t m);
static void car_of_kind(Carried *cr, uint8_t v, uint8_t k);
static void cmp8(Carried *cr, uint8_t m);
static int crash_set_a096(Carried *cr);
static int crash_set_a33d(Carried *cr);
static int crash_set_a5e4(Carried *cr);
static int crash_set_a88b(Carried *cr);
static uint32_t db(Carried *cr);
static uint8_t dec8(Carried *cr, uint32_t addr);
static void detach_frame(Carried *cr);
static uint32_t dp(Carried *cr, uint8_t off);
static void fly_frame(Carried *cr);
static void handler(Carried *cr, uint16_t o, uint16_t h);
static void inc8(Carried *cr, uint32_t addr);
static uint8_t ld8(Carried *cr, uint32_t addr);
static void lda8(Carried *cr, uint8_t v);
static void or8(Carried *cr, uint32_t addr, uint8_t m);
static void parent(Carried *cr);
static void part_on_car(Carried *cr);
static void player1_frame(Carried *cr);
static void player2_frame(Carried *cr);
static void player_attach(Carried *cr, uint16_t h, uint16_t bare);
static void player_drive(Carried *cr);
static void player_record(Carried *cr, uint16_t n, uint32_t match, int read64);
static void rec_word(Carried *cr, uint32_t field, int op, uint16_t m);
static void return_frame(Carried *cr);
static void rival_car(Carried *cr, int b);
static void rival_frame(Carried *cr);
static int shed(Carried *cr, uint16_t kind, uint8_t o48, uint8_t o49, uint8_t o4a, uint8_t c, int mark, int first);
static void sound0(Carried *cr, uint8_t v);
static int spawn(Carried *cr, uint16_t kind);
static void st16i(Carried *cr, uint32_t addr, uint16_t v);
static void st8i(Carried *cr, uint32_t addr, uint8_t v);
static void turned_again(Carried *cr);
static void w_RandomStepLong(Carried *cr);
void GsuRecordLand_cr(Carried *cr);
void GsuRecordNew426_cr(Carried *cr);
void GsuRecordStep6A_cr(Carried *cr);
void GsuRecordVelocity_cr(Carried *cr);
void GsuRun_cr(Carried *cr);
void ObjectApplyVelocity_cr(Carried *cr);
void ObjectAttachLinked_cr(Carried *cr);
void ObjectBreakOffContact(Carried *cr);
void ObjectCallHandler_cr(Carried *cr);
void ObjectCarFrame(Carried *cr);
void ObjectCarRecordNew(Carried *cr);
void ObjectCarSpawn(Carried *cr);
void ObjectCopy14From_cr(Carried *cr);
void ObjectCopyPositionFrom_cr(Carried *cr);
void ObjectListAppend_cr(Carried *cr);
void ObjectMoveForward(Carried *cr);
void ObjectPoseFromRecord_cr(Carried *cr);
void ObjectRebucket_cr(Carried *cr);
void ObjectReleaseLong(Carried *cr);
void ObjectRelease_cr(Carried *cr);
void ObjectSet5DByKind_cr(Carried *cr);
void ObjectSpawnChanceD460(Carried *cr);
void ObjectSpawn_cr(Carried *cr);
void ObjectSyncFromRecord_cr(Carried *cr);
void ObjectVelocityFromRecordOf_cr(Carried *cr);
void PlayerCarEvents(Carried *cr);
void RecordContactSummary_cr(Carried *cr);
void RotateXY_cr(Carried *cr);
void RotateXZFrom08_cr(Carried *cr);
void RotateXZ_cr(Carried *cr);
void SoundPort0QueuePush_cr(Carried *cr);

static uint32_t db(Carried *cr)
{
    return (uint32_t)cr->db << 16;
}

/* Object field `f` ($20xx) of object `o`, through the data bank. */
#define F(f, o) (0x7E0000 | (uint16_t)((f) + (o)))

/* Direct page byte or word at off. */
static uint32_t dp(Carried *cr, uint8_t off)
{
    return (uint16_t)(cr->d + off);
}

/* LDA #v with M 8-bit. */
static void lda8(Carried *cr, uint8_t v)
{
    cyc(16);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
}

/* LDA addr, 8-bit; returns the byte. */
static uint8_t ld8(Carried *cr, uint32_t addr)
{
    cyc(16);
    r_lda8(cr, addr);
    return (uint8_t)cr->a;
}

/* LDA #v, STA addr, 8-bit. */
static void st8i(Carried *cr, uint32_t addr, uint8_t v)
{
    lda8(cr, v);
    cyc(24);
    wr8(addr, v);
}

/* REP #$20, LDA #v, STA addr, SEP #$20. */
static void st16i(Carried *cr, uint32_t addr, uint16_t v)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16_imm(cr, v);
    cyc(28);
    wr16(addr, v);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

/* LDA addr, ORA #m, STA addr (8-bit). */
static void or8(Carried *cr, uint32_t addr, uint8_t m)
{
    cyc(14);
    uint8_t v = (uint8_t)(ld8(cr, addr) | m);

    lda8(cr, v);
    cyc(30);
    wr8(addr, v);
}

/* LDA addr, AND #m, STA addr (8-bit). */
static void and8(Carried *cr, uint32_t addr, uint8_t m)
{
    cyc(10);
    uint8_t v = (uint8_t)(ld8(cr, addr) & m);

    lda8(cr, v);
    cyc(30);
    wr8(addr, v);
}

/* LDA addr, CLC, ADC #v, STA addr (8-bit). */
static void add8(Carried *cr, uint32_t addr, uint8_t v)
{
    cyc(14);
    uint8_t r = ld8(cr, addr);

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    r = alu_adc8(r, v, &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | r);
    cyc(60);
    wr8(addr, r);
}

/* REP #$20, LDA addr, CLC, ADC #v, STA addr, SEP #$20. */
static void add16(Carried *cr, uint32_t addr, uint16_t v)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, addr);
    r_add16(cr, v);
    cyc(68);
    wr16(addr, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

/* REP #$20, LDA addr, CLC, ADC by (a word), STA addr, SEP #$20; adc is
   the time of CLC and the ADC to its read (30 direct page, 38 absolute). */
static void add16m(Carried *cr, uint32_t addr, uint32_t by, uint32_t adc)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, addr);
    r_add16(cr, rd16(cyc_then(adc, by)));
    cyc(30);
    wr16(addr, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(22);
}

/* DEC addr (8-bit); returns the new value. */
static uint8_t dec8(Carried *cr, uint32_t addr)
{
    cyc(52);
    uint8_t v = (uint8_t)(rd8(addr) - 1);

    cyc(6);
    wr8(addr, v);
    set_p(cr, nz8(cr->p, v));
    return v;
}

/* INC addr (8-bit). */
static void inc8(Carried *cr, uint32_t addr)
{
    cyc(24);
    uint8_t v = (uint8_t)(rd8(addr) + 1);

    cyc(6);
    wr8(addr, v);
    set_p(cr, nz8(cr->p, v));
}

/* CMP #m (8-bit). */
static void cmp8(Carried *cr, uint8_t m)
{
    uint8_t a = (uint8_t)cr->a;

    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? (cyc(38), P_C) : (cyc(38), 0))));
}

/* The handler of object o: +$1A = h, +$1C = $08. */
static void handler(Carried *cr, uint16_t o, uint16_t h)
{
    cyc(6);
    st16i(cr, F(W(obj_1a_w), o), h);
    st8i(cr, F(W(obj_1c), o), cyc_then(28, 0x08));
}

/* LDY +$06: Y = the part's car (its parent). */
static void parent(Carried *cr)
{
    cr->y = rd16(cyc_then(30, F(W(obj_06), cr->x)));
    set_p(cr, nz16(cr->p, cr->y));
}

/* LDA +$25 of `o`, AND #m: whether those bits are set. */
static int bits25(Carried *cr, uint16_t o, uint8_t m)
{
    lda8(cr, cyc_then(26, (uint8_t)(ld8(cr, cyc_then(10, F(W(obj_25), o))) & m)));
    return (uint8_t)cr->a != 0;
}

/* As bits25, for a BNE on the result: taken (22) is 4 less than the way
   bits25 stands for. */
static int bits25b(Carried *cr, uint16_t o, uint8_t m)
{
    uint8_t v = (uint8_t)(ld8(cr, cyc_then(10, F(W(obj_25), o))) & m);

    cyc(v ? 22 : 26);
    lda8(cr, v);
    return v != 0;
}

/* LDA #v, JSL SoundPort0QueuePush. */
static void sound0(Carried *cr, uint8_t v)
{
    cyc(44);
    lda8(cr, v);
    SoundPort0QueuePush_cr(cr);
}

/* LDA src, STA lo, STA lo2, BIT #$80: the sign byte `hi` = $FF for a
   negative value, else 0. Time from the assembly: LDA abs (24 with `in`,
   the time before it), STA 16 to direct page or 24 absolute, BIT # 16 and
   BEQ 16 then LDA #$FF and STA, or BEQ taken 22 and STZ. Returns whether
   it was negative: then a BRA (22) comes before the next load. */
static int axis_from(Carried *cr, uint32_t src, uint32_t lo, uint32_t lo2, uint32_t hi, uint32_t in)
{
    uint8_t r;
    uint32_t w2 = (lo2 & 0xFFFF) > 0xFF ? 8 : 0, wh = (hi & 0xFFFF) > 0xFF ? 8 : 0;

    cyc(in);
    r = ld8(cr, src);
    cyc(16);
    wr8(lo, r);
    cyc(16 + w2);
    wr8(lo2, r);
    if (r & 0x80) {
        cyc(32);
        lda8(cr, 0xFF);
        cyc(16 + wh);
        wr8(hi, 0xFF);
        return 1;
    }
    cyc(54 + wh);
    wr8(hi, 0x00);
    return 0;
}

/* LDA lo, STA hi (8-bit): the turned vector back as the next input. */
static void turned_again(Carried *cr)
{
    wr8(dp(cr, 0x02), cyc_then(16, ld8(cr, dp(cr, 0x04))));
    wr8(dp(cr, 0x08), cyc_then(16, ld8(cr, dp(cr, 0x0A))));
    cyc(8);
    wr8(dp(cr, 0x94), cyc_then(16, ld8(cr, WA(wram_05F2))));
}

/*
 * @routine 08:AB71 PartOffset
 * WHAT IT DOES: A part's offset on its car: the vector of its bytes +$48,
 * +$49, +$4A (through $1F58-$1F5A) turned by the car's (Y's) angles +$16
 * (RotateXY), +$14 (RotateXZFrom08) and +$15 (RotateXZ), doubled; left in
 * direct page $04/$0A and $05F2 as words.
 * HOW: Direct page $02/$08/$94 in (sign bytes $05, $0B, $05F3), one ASL
 * per word.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = the part, Y = its car. Exit: M
 * 8-bit; A = the doubled Z word; N, Z, C from its ASL.
 */
void PartOffset(Carried *cr)
{
    uint8_t v;
    int neg;

    v = ld8(cr, cyc_then(14, F(W(obj_48), cr->x)));
    WR8(wram_1F58, cyc_then(24, v));
    v = ld8(cr, cyc_then(14, F(W(obj_49), cr->x)));
    WR8(wram_1F59, cyc_then(24, v));
    v = ld8(cr, cyc_then(14, F(W(obj_4a), cr->x)));
    WR8(wram_1F5A, cyc_then(24, v));
    neg = axis_from(cr, WA(wram_1F58), dp(cr, 0x02), dp(cr, 0x04), dp(cr, 0x05), 8);
    neg = axis_from(cr, WA(wram_1F59), dp(cr, 0x08), dp(cr, 0x0A), dp(cr, 0x0B), neg ? 30 : 8);
    neg = axis_from(cr, WA(wram_1F5A), dp(cr, 0x94), WA(wram_05F2), WA(rot_z_high), neg ? 30 : 8);
    ld8(cr, cyc_then(neg ? 36 : 14, F(W(obj_16), cr->y)));   /* BRA, LDA abs,Y */
    (cyc(62), RotateXY_cr(cr));
    turned_again(cr);
    ld8(cr, cyc_then(14, F(W(obj_14), cr->y)));
    (cyc(62), RotateXZFrom08_cr(cr));
    turned_again(cr);
    ld8(cr, cyc_then(14, F(W(obj_15), cr->y)));
    (cyc(62), RotateXZ_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(38, dp(cr, 0x04)));
    r_asl16(cr);
    wr16(dp(cr, 0x04), cyc_then(30, cr->a));
    r_lda16(cr, cyc_then(16, dp(cr, 0x0A)));
    r_asl16(cr);
    wr16(dp(cr, 0x0A), cyc_then(30, cr->a));
    r_lda16(cr, cyc_then(24, WA(wram_05F2)));
    r_asl16(cr);
    WR16(wram_05F2, cyc_then(38, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(64);
}

void abi_PartOffset(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PartOffset(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The part's position = its car's + the offset (PartOffset,
   ObjectCopyPositionFrom, ObjectCopy14From, then the three words). */
static void part_on_car(Carried *cr)
{
    (cyc(46), PartOffset(cr));
    (cyc(62), ObjectCopyPositionFrom_cr(cr));
    (cyc(62), ObjectCopy14From_cr(cr));
}

/* 08:AC27: the detaching part's frame: released at +$4B = 8; at 7 the car's
   +$25 bit 7 is cleared; +$4B counts up and the part sits at its car plus
   its offset times 1-4 (by +$4B: up to 3, it, from 4, 9 - it; none at
   9). */
static void detach_frame(Carried *cr)
{
    uint8_t v;

    parent(cr);
    v = ld8(cr, cyc_then(14, F(W(obj_4b), cr->x)));
    cyc(14);
    cmp8(cr, 0x08);
    if (v == 0x08) {
        cyc(12);
        ObjectReleaseLong(cr);
        return;
    }
    v = ld8(cr, F(W(obj_4b), cr->x));
    cmp8(cr, 0x07);
    if (v == 0x07)
        { cyc(4); and8(cr, F(W(obj_25), cr->y), 0x7F); } else { cyc(26); }
    cyc(6);
    inc8(cr, F(W(obj_4b), cr->x));
    part_on_car(cr);
    v = ld8(cr, cyc_then(14, F(W(obj_4b), cr->x)));
    cmp8(cr, 0x04);
    if (!((uint8_t)(v - 0x04) & 0x80)) {
        cyc(22);
        set_p(cr, (uint8_t)(cr->p | (P_C)));
        v = alu_sbc8(v, 0x09, &cr->p);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        if (!v) {
            ObjectRebucket_cr(cr);
            return;
        }
        v ^= 0xFF;
        lda8(cr, v);
        v++;
        lda8(cr, v);
    }
    do {
        cyc(22);
        add16m(cr, F(W(obj_pos_x), cr->x), dp(cr, 0x04), 30);
        add16m(cr, F(W(obj_pos_y), cr->x), dp(cr, 0x0A), 30);
        add16m(cr, F(W(obj_pos_z), cr->x), WA(wram_05F2), 38);
        cyc(32);
        lda8(cr, v);
        v--;
        lda8(cr, v);
    } while (v);
    cyc(56);
    ObjectRebucket_cr(cr);
    cyc(44);
}

/*
 * @routine 08:AC04 PartDetach
 * WHAT IT DOES: A part comes off its car: the car's +$25 bit 7 set, the
 * part's model by the car's kind (+$49, ObjectSet5DByKind), +$4B = 0, frame
 * $08:AC27 (PartDetachStep); then that frame.
 * HOW: Byte and word stores; JSR ObjectSet5DByKind.
 * Entry: M 8-bit, X 16-bit; X = the part. Exit: as the frame leaves it.
 */
void PartDetach(Carried *cr)
{
    parent(cr);
    or8(cr, F(W(obj_25), cr->y), 0x80);
    ld8(cr, cyc_then(14, F(W(obj_49), cr->y)));
    ObjectSet5DByKind_cr(cr);
    wr8(F(W(obj_4b), cr->x), cyc_then(30, 0x00));
    cyc(4);
    handler(cr, cr->x, 0xAC27);
    detach_frame(cr);
}

void abi_PartDetach(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PartDetach(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:AC27 PartDetachStep
 * WHAT IT DOES: The detaching part's frame: its count +$4B goes up each
 * frame; it sits at its car (+$06) plus its offset (PartOffset) taken 1 to
 * 4 times (rising to 4, then falling); at 7 the car's +$25 bit 7 is
 * cleared and at 8 the part is released.
 * HOW: CMP, INC; SBC #9 and EOR/INC for the falling half; a PHA/PLA count
 * loop of 16-bit ADCs; ObjectReleaseLong by JML.
 * Entry: M 8-bit, X 16-bit; X = the part; D = $0000. Exit: as
 * ObjectRebucket or ObjectReleaseLong leaves it.
 */
void PartDetachStep(Carried *cr)
{
    detach_frame(cr);
}

void abi_PartDetachStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PartDetachStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 08:AE0F: the returning part's frame (see PartReturnStep). */
static void return_frame(Carried *cr)
{
    uint8_t v;

    st8i(cr, F(W(obj_36), cr->x), cyc_then(6, 0x03));
    parent(cr);
    (cyc(62), ObjectVelocityFromRecordOf_cr(cr));
    (cyc(46), PartOffset(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(52, F(W(obj_pos_x), cr->y)));
    r_add16(cr, rd16(cyc_then(30, dp(cr, 0x04))));
    wr16(dp(cr, 0x04), cyc_then(16, cr->a));
    r_lda16(cr, cyc_then(30, F(W(obj_pos_y), cr->y)));
    r_add16(cr, rd16(cyc_then(30, dp(cr, 0x0A))));
    wr16(dp(cr, 0x0A), cyc_then(16, cr->a));
    r_lda16(cr, cyc_then(30, F(W(obj_pos_z), cr->y)));
    r_add16(cr, rd16(cyc_then(38, WA(wram_05F2))));
    WR16(wram_05F2, cyc_then(24, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    {
        static const uint16_t f[3] = {0x200C, 0x200E, 0x2010};
        uint32_t t[3];
        int i;

        t[0] = dp(cr, 0x04);
        t[1] = dp(cr, 0x0A);
        t[2] = WA(wram_05F2);
        for (i = 0; i < 3; i++) {
            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
            r_lda16(cr, cyc_then(74, F(f[i], cr->x)));
            cyc(16);
            wr16(W(dp_3e), cr->a);
            cyc((t[i] & 0xFFFF) < 0x100 ? 16 : 24);   /* LDA direct page or absolute */
            r_lda16(cr, t[i]);
            r_approach(cr, ApproachQuarterMin4);
            wr16(F(f[i], cr->x), cyc_then(30, cr->a));
            set_p(cr, (uint8_t)(cr->p | (P_M)));
        }
        cyc(22);
        for (i = 0; i < 3; i++) {
            cyc(14);
            wr8(W(dp_3e), cyc_then(16, ld8(cr, F(W(obj_14) + i, cr->x))));
            ld8(cr, cyc_then(14, F(W(obj_14) + i, cr->y)));
            (cyc(62), r_approach_byte(cr, ApproachQuarterMin4Byte));
            wr8(F(W(obj_14) + i, cr->x), cyc_then(30, (uint8_t)cr->a));
        }
        cyc(62);
    }
    ObjectApplyVelocity_cr(cr);
    v = ld8(cr, cyc_then(14, F(W(obj_4b), cr->x)));
    cmp8(cr, 0x01);
    if (v == 0x01) {
        int hit;

        cyc(4);
        hit = bits25(cr, cr->x, 0x02);
        cyc(8);
        sound0(cr, hit ? (cyc(32), 0x4D) : (cyc(6), 0x4E));
        cyc(hit ? 62 : 84);             /* JSL, after the $4E arm's BRA */
    } else { cyc(88); }
    ObjectRebucket_cr(cr);
    st8i(cr, F(W(obj_36), cr->x), cyc_then(6, 0x03));
    if (ld8(cr, cyc_then(14, F(W(obj_4b), cr->x)))) {
        dec8(cr, F(W(obj_4b), cr->x));
        return;
    } else { cyc(48); }
    parent(cr);
    cyc(4);
    if (!bits25(cr, cr->y, 0x04))
        { cyc(0); handler(cr, cr->x, 0xF927); } else { cyc(22); }
    cyc(4);
    if (bits25(cr, cr->x, 0x01))
        { cyc(0); and8(cr, F(W(obj_25), cr->y), 0xFB); } else { cyc(22); }
}

/*
 * @routine 08:AE0F PartReturnStep
 * WHAT IT DOES: The part falling back onto its car: it moves a quarter of
 * the way to its place on the car (PartOffset from the car's position) and
 * turns a quarter of the way to the car's angles, with the car's record
 * velocity (ObjectVelocityFromRecordOf, ObjectApplyVelocity); on the last
 * count (+$4B = 1) sound $4E ($4D with +$25 bit 1); while +$4B lasts it
 * counts down; then, unless the car's +$25 bit 2 is set, its frame becomes
 * ObjectReleaseLong ($08:F927), and with its own +$25 bit 0 the car's bit
 * 2 is cleared.
 * HOW: ApproachQuarterMin4 on the position words, ApproachQuarterMin4Byte
 * on +$14-$16, all through $3E; +$36 = 3.
 * Entry: M 8-bit, X 16-bit; X = the part; D = $0000. Exit: as the path
 * leaves it.
 */
void PartReturnStep(Carried *cr)
{
    return_frame(cr);
    cyc(44);
}

void abi_PartReturnStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PartReturnStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:ADE2 PartReturn
 * WHAT IT DOES: The flying part turns back to its car: frame $08:AE0F
 * (PartReturnStep), +$4B = $0A, sound $4B ($4C with +$25 bit 1); then that
 * frame.
 * HOW: Word and byte stores; entered by a JML from PartFlyStep.
 * Entry: M 8-bit, X 16-bit; X = the part; D = $0000. Exit: as the frame
 * leaves it.
 */
void PartReturn(Carried *cr)
{
    cyc(4);
    handler(cr, cr->x, 0xAE0F);
    st8i(cr, F(W(obj_4b), cr->x), cyc_then(6, 0x0A));
    {
        int hit;

        cyc(4);
        hit = bits25(cr, cr->x, 0x02);
        cyc(14);
        sound0(cr, hit ? (cyc(26), 0x4C) : 0x4B);
        if (!hit)
            cyc(22);                    /* BRA */
    }
    return_frame(cr);
    cyc(44);
}

void abi_PartReturn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PartReturn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 08:AD70: the flying part's frame (see PartFlyStep). */
static void fly_frame(Carried *cr)
{
    uint8_t v;

    st8i(cr, F(W(obj_36), cr->x), cyc_then(6, 0x03));
    (cyc(62), ObjectSyncFromRecord_cr(cr));
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if (!(uint8_t)cr->a) {
        cyc(16);
        parent(cr);
        cyc(4);
        if (bits25(cr, cr->y, 0x10)) {
            cyc(58);
            RecordContactSummary_cr(cr);
            cyc(0);
            lda8(cr, (uint8_t)(cr->a & 0x80));
            if ((uint8_t)cr->a) {
                cyc(40);
                ObjectBreakOffContact(cr);
                return;
            }
        }
    }
    cyc(28);
    st8i(cr, F(W(obj_36), cr->x), 0x03);
    parent(cr);
    cyc(4);
    if (!bits25(cr, cr->y, 0x10) || (cyc(10), ld8(cr, F(W(obj_4b), cr->x)))) {
        cyc(36);
        if (!ld8(cr, F(W(obj_4b), cr->x))) {
            cyc(48);
            PartReturn(cr);
            return;
        }
        dec8(cr, F(W(obj_4b), cr->x));
        add8(cr, F(W(obj_14), cr->x), 0x08);
        add8(cr, F(W(obj_15), cr->x), 0x04);
    } else { cyc(48); }
    cyc(14);
    v = ld8(cr, F(W(obj_27), cr->x));
    if (!(v & 0x80)) {
        v--;
        lda8(cr, v);
        if (!v) {
            cyc(60);
            ObjectReleaseLong(cr);
            return;
        }
        wr8(F(W(obj_27), cr->x), cyc_then(66, v));
        cyc(46);
    } else { cyc(68); }
    ObjectSpawnChanceD460(cr);
    cyc(134);
}

/*
 * @routine 08:AD70 PartFlyStep
 * WHAT IT DOES: The flying part's frame: +$36 = 3; synced from its record;
 * with no contacts, while its car's +$25 bit 4 is set, a record summary
 * with bit 7 breaks it off (ObjectBreakOffContact). Then, unless the car's
 * bit 4 is set and +$4B is 0, it tumbles (+$4B down, +$14 by 8, +$15 by
 * 4) or, at +$4B = 0, turns back (PartReturn); a life +$27 (bit 7 clear)
 * counts down and releases it at 0; ObjectSpawnChanceD460.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; JMP, JML and BRL to
 * the routines named.
 * Entry: M 8-bit, X 16-bit; X = the part; D = $0000. Exit: as the path
 * leaves it.
 */
void PartFlyStep(Carried *cr)
{
    fly_frame(cr);
}

void abi_PartFlyStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PartFlyStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:ACB9 PartKnockOff
 * WHAT IT DOES: A part knocked off its car: its model by the car's kind,
 * at the car's position plus its offset, the car's record velocity, a
 * record of its own (GsuRecordNew426). With none free it is released
 * (and with +$25 bit 0 the car's +$25 bits 2 and 4 are cleared first).
 * With one: a life +$27 with bit 7 clear sets bit 7 of the record's flags
 * (+$16); its velocity grows by the offset (Y doubled), given to the
 * record; frame $08:AD70 (PartFlyStep); then that frame.
 * HOW: JSR ObjectSet5DByKind, PartOffset; ObjectCopyPositionFrom,
 * ObjectCopy14From; 16-bit ADCs; GsuRecordVelocity.
 * Entry: M 8-bit, X 16-bit; X = the part; D = $0000. Exit: as the path
 * leaves it.
 */
void PartKnockOff(Carried *cr)
{
    parent(cr);
    ld8(cr, cyc_then(14, F(W(obj_49), cr->y)));
    ObjectSet5DByKind_cr(cr);
    (cyc(0), part_on_car(cr));
    add16m(cr, F(W(obj_pos_x), cr->x), dp(cr, 0x04), 30);
    add16m(cr, F(W(obj_pos_y), cr->x), dp(cr, 0x0A), 30);
    add16m(cr, F(W(obj_pos_z), cr->x), WA(wram_05F2), 38);
    (cyc(62), ObjectVelocityFromRecordOf_cr(cr));
    (cyc(62), GsuRecordNew426_cr(cr));
    if (cr->p & P_N) {
        cyc(20);
        if (bits25b(cr, cr->x, 0x01)) {
            parent(cr);
            cyc(4);
            and8(cr, F(W(obj_25), cr->y), 0xFB);
            cyc(4);
            and8(cr, F(W(obj_25), cr->y), 0xEF);
        } else { cyc(22); }
        (cyc(62), ObjectRelease_cr(cr));
        cyc(44);
        return;
    } else { cyc(36); }
    if (!(ld8(cr, F(W(obj_27), cr->x)) & 0x80)) {
        uint16_t sx = cr->x;
        uint32_t at;

        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        r_lda16(cr, cyc_then(98, F(W(obj_22), cr->x)));
        cr->x = cr->a;
        set_p(cr, nz16(cr->p, cr->x));
        at = (0x700016 + cr->x) & 0xFFFFFF;
        cyc(46);
        r_lda16(cr, at);
        cr->a |= 0x0080;
        set_p(cr, nz16(cr->p, cr->a));
        cyc(56);
        wr16(at, cr->a);
        cr->x = sx;
        set_p(cr, nz16(cr->p, sx));
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(36);                        /* PLX of 16 bits, SEP, less the other way's BMI taken */
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(60, dp(cr, 0x04)));
    r_add16(cr, rd16(cyc_then(44, F(W(obj_vel_x), cr->x))));
    wr16(F(W(obj_vel_x), cr->x), cyc_then(30, cr->a));
    r_lda16(cr, cyc_then(16, dp(cr, 0x0A)));
    r_asl16(cr);
    r_add16(cr, rd16(cyc_then(58, F(W(obj_vel_y), cr->x))));
    wr16(F(W(obj_vel_y), cr->x), cyc_then(30, cr->a));
    r_lda16(cr, cyc_then(24, WA(wram_05F2)));
    r_add16(cr, rd16(cyc_then(44, F(W(obj_vel_z), cr->x))));
    wr16(F(W(obj_vel_z), cr->x), cyc_then(30, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(84), GsuRecordVelocity_cr(cr));
    cyc(4);
    handler(cr, cr->x, 0xAD70);
    fly_frame(cr);
}

void abi_PartKnockOff(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PartKnockOff(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:AC97 PartHit
 * WHAT IT DOES: A part hit off its car: runs Super FX program $04:888E,
 * sets its +$25 bit 0 and its car's +$25 bit 2, and knocks it off
 * (PartKnockOff).
 * HOW: GsuRun with X and Y kept (PHX, PHY, PLY, PLX); a BRA into
 * PartKnockOff.
 * Entry: M 8-bit, X 16-bit; X = the part; D = $0000. Exit: as PartKnockOff
 * leaves it.
 */
void PartHit(Carried *cr)
{
    uint16_t sx = cr->x, sy = cr->y;

    cyc(146);
    lda8(cr, 0x04);
    cr->x = 0x888E;
    set_p(cr, nz16(cr->p, cr->x));
    GsuRun_cr(cr);
    cr->y = sy;
    set_p(cr, nz16(cr->p, sy));
    cr->x = sx;
    set_p(cr, nz16(cr->p, sx));
    or8(cr, F(W(obj_25), cr->x), cyc_then(72, 0x01));
    parent(cr);
    or8(cr, F(W(obj_25), cr->y), 0x04);
    (cyc(22), PartKnockOff(cr));
}

void abi_PartHit(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PartHit(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The player car's frame after its own input: GsuRecordLand,
   PlayerCarEvents, ObjectPoseFromRecord, GsuRecordStep6A,
   ObjectMoveForward, ObjectRebucket, +$36 = 3. */
static void player_drive(Carried *cr)
{
    GsuRecordLand_cr(cr);
    (cyc(62), PlayerCarEvents(cr));
    (cyc(62), ObjectPoseFromRecord_cr(cr));
    (cyc(46), GsuRecordStep6A_cr(cr));
    (cyc(46), ObjectMoveForward(cr));
    (cyc(62), ObjectRebucket_cr(cr));
    st8i(cr, F(W(obj_36), cr->x), cyc_then(6, 0x03));
}

/* 08:AF93: player 1's frame (see PlayerCar1Step). */
static void player1_frame(Carried *cr)
{
    uint16_t sx;

    st8i(cr, WA(current_player), 0x00);
    player_drive(cr);
    and8(cr, F(W(obj_24), cr->x), cyc_then(4, 0xFD));
    cr->y = cr->x;
    set_p(cr, nz16(cr->p, cr->y));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    sx = cr->x;
    r_lda16(cr, cyc_then(96, F(W(obj_22), cr->y)));
    cr->x = cr->a;
    set_p(cr, nz16(cr->p, cr->x));
    cyc(46);
    r_lda16(cr, (0x70001C + cr->x) & 0xFFFFFF);
    r_and16(cr, 0x0002);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (cr->a)
        { cyc(60); or8(cr, F(W(obj_24), cr->y), 0x02); } else { cyc(66); }
    cyc(36);
    cr->x = sx;
    set_p(cr, nz16(cr->p, sx));
    cyc(44);
}

/* A player car's record (ObjectCarRecordNew with A = n, M 16-bit); with
   one, it is the record matched at `match` and gets bit 3 of +$1C (for
   player 2 the word at +$64 is read with bit 3 first, unused). */
static void player_record(Carried *cr, uint16_t n, uint32_t match, int read64)
{
    cyc(48);
    uint16_t sx;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16_imm(cr, n);
    ObjectCarRecordNew(cr);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(52, F(W(obj_22), cr->x)));
    if (cr->a) {
        uint32_t at;

        sx = cr->x;
        cr->x = cr->a;
        set_p(cr, nz16(cr->p, cr->x));
        cyc(92);
        wr16(match, cr->a);
        if (read64) {
            cyc(32);
            r_lda16(cr, (0x700064 + cr->x) & 0xFFFFFF);
            cr->a |= 0x0008;
            set_p(cr, nz16(cr->p, cr->a));
        }
        at = (0x70001C + cr->x) & 0xFFFFFF;
        cyc(read64 ? 56 : 32);          /* ORA # before the LDA after the +$64 read */
        r_lda16(cr, at);
        cr->a |= 0x0008;
        set_p(cr, nz16(cr->p, cr->a));
        cyc(56);
        wr16(at, cr->a);
        cr->x = sx;
        set_p(cr, nz16(cr->p, sx));
    }
    cyc(62);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

/* The player car's set-up after its record: frame h, attached to the bare
   object at `bare` (ObjectAttachLinked), its model by its kind (+$49),
   $1A5E-$1A5F = 0, +$29 = 0. */
static void player_attach(Carried *cr, uint16_t h, uint16_t bare)
{
    handler(cr, cr->x, h);
    cr->y = rd16(cyc_then(24, db(cr) | bare));
    set_p(cr, nz16(cr->p, cr->y));
    (cyc(62), ObjectAttachLinked_cr(cr));
    ld8(cr, cyc_then(14, F(W(obj_49), cr->x)));
    ObjectSet5DByKind_cr(cr);
    st16i(cr, (cyc(4), 0x7E0000) | W(wram_1A5E), 0x0000);
    st16i(cr, F(W(obj_29), cr->x), cyc_then(32, 0x0000));
}

/*
 * @routine 08:AF0F PlayerCar1
 * WHAT IT DOES: Sets up player 1's car: list kind +$26 = 1 on the object
 * list (ObjectListAppend), $A0 higher; in a one-player race with the bare
 * object's frame at $8E2A, +$27 = $8669; its record (ObjectCarRecordNew 1)
 * matched at $70:2476 with bit 3 of +$1C; frame $08:AF93 (PlayerCar1Step),
 * attached to the bare object $1ADA, its model by kind, $1A5E = 0, +$29 =
 * 0; then that frame.
 * HOW: 16-bit compares and long record accesses; ObjectCarRecordNew is
 * called with M 16-bit.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void PlayerCar1(Carried *cr)
{
    st8i(cr, F(W(obj_26), cr->x), cyc_then(6, 0x01));
    (cyc(62), ObjectListAppend_cr(cr));
    add16(cr, F(W(obj_pos_y), cr->x), 0xFF60);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(68, WA(hud_mode)));
    r_and16(cr, 0x00FF);
    if (!cr->a) {
        cr->y = rd16(cyc_then(62, WA(bare_object_a)));
        set_p(cr, nz16(cr->p, cr->y));
        r_lda16(cr, cyc_then(30, F(W(obj_1a_w), cr->y)));
        r_cmp16(cr, 0x8E2A);
        if (cr->a == 0x8E2A) {
            r_lda16_imm(cr, 0x8669);
            wr16(F(W(obj_27), cr->x), cyc_then(56, 0x8669));
        } else { cyc(46); }
    } else { cyc(44); }
    player_record(cr, 0x0001, 0x702476, 0);
    player_attach(cr, 0xAF93, W(bare_object_a));
    (cyc(22), player1_frame(cr));
}

void abi_PlayerCar1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerCar1(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:AF93 PlayerCar1Step
 * WHAT IT DOES: Player 1's car frame: current player $1A5D = 0, then the
 * car's drive (GsuRecordLand, PlayerCarEvents, ObjectPoseFromRecord,
 * GsuRecordStep6A, ObjectMoveForward, ObjectRebucket), +$36 = 3; +$24 bit
 * 1 follows bit 1 of its record's +$1C.
 * HOW: JSRs and JSLs; the record word through X (PHX/PLX).
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; X
 * unchanged, Y = X; N, Z from X.
 */
void PlayerCar1Step(Carried *cr)
{
    player1_frame(cr);
}

void abi_PlayerCar1Step(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerCar1Step(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 08:B049: player 2's frame (see PlayerCar2Step). */
static void player2_frame(Carried *cr)
{
    st8i(cr, WA(current_player), 0x01);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(46, WA(joy2_held)));
    WR16(seq_input, cyc_then(24, cr->a));
    r_lda16(cr, cyc_then(24, WA(wram_1F25)));
    WR16(wram_1AF2, cyc_then(24, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(22), player_drive(cr));
    cyc(44);
}

/*
 * @routine 08:AFD7 PlayerCar2
 * WHAT IT DOES: Sets up player 2's car: list kind +$26 = 2 on the object
 * list, $A0 higher; its record (ObjectCarRecordNew 2) matched at
 * $70:2478 with bit 3 of +$1C; frame $08:B049 (PlayerCar2Step), attached
 * to the bare object $1ADC, its model by kind, $1A5E = 0, +$29 = 0; then
 * that frame.
 * HOW: As PlayerCar1 (the record's +$64 is read and ORed, the result
 * unused).
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void PlayerCar2(Carried *cr)
{
    st8i(cr, F(W(obj_26), cr->x), cyc_then(6, 0x02));
    (cyc(62), ObjectListAppend_cr(cr));
    add16(cr, F(W(obj_pos_y), cr->x), 0xFF60);
    cyc(44);
    player_record(cr, 0x0002, 0x702478, 1);
    player_attach(cr, 0xB049, W(bare_object_b));
    (cyc(22), player2_frame(cr));
}

void abi_PlayerCar2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerCar2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B049 PlayerCar2Step
 * WHAT IT DOES: Player 2's car frame: current player $1A5D = 1, player 2's
 * held buttons ($030D) and $1F25 copied to $1AF0/$1AF2, then the car's
 * drive (GsuRecordLand, PlayerCarEvents, ObjectPoseFromRecord,
 * GsuRecordStep6A, ObjectMoveForward, ObjectRebucket), +$36 = 3.
 * HOW: 16-bit copies; JSRs and JSLs.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: A low byte = 3.
 */
void PlayerCar2Step(Carried *cr)
{
    player2_frame(cr);
}

void abi_PlayerCar2Step(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerCar2Step(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B1E1 ObjectCarSpawnListed
 * WHAT IT DOES: Puts object X on the object list (ObjectListAppend), then
 * runs into ObjectCarSpawn.
 * HOW: JSL ObjectListAppend, falling into 08:B1E5.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectCarSpawn
 * leaves it.
 */
void ObjectCarSpawnListed(Carried *cr)
{
    (cyc(62), ObjectListAppend_cr(cr));
    ObjectCarSpawn(cr);
}

void abi_ObjectCarSpawnListed(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarSpawnListed(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A car of list kind k with +$48 = v (08:B1BA-B1DE). */
static void car_of_kind(Carried *cr, uint8_t v, uint8_t k)
{
    st8i(cr, F(W(obj_48), cr->x), cyc_then(6, v));
    st8i(cr, F(W(obj_26), cr->x), cyc_then(6, k));
    (cyc(24), ObjectCarSpawnListed(cr));
}

/*
 * @routine 08:B1BA ObjectCarKind3
 * WHAT IT DOES: Sets up a car of list kind 3 (+$48 = 0, +$26 = 3) and
 * spawns it (ObjectCarSpawnListed).
 * HOW: Two stores and a JMP; course data points here.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectCarSpawn
 * leaves it.
 */
void ObjectCarKind3(Carried *cr)
{
    car_of_kind(cr, 0x00, 0x03);
}

void abi_ObjectCarKind3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarKind3(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B1C7 ObjectCarKind4
 * WHAT IT DOES: Sets up a car of list kind 4 (+$48 = 5, +$26 = 4) and
 * spawns it (ObjectCarSpawnListed).
 * HOW: As ObjectCarKind3.
 * Entry: as ObjectCarKind3. Exit: as ObjectCarKind3.
 */
void ObjectCarKind4(Carried *cr)
{
    car_of_kind(cr, 0x05, 0x04);
}

void abi_ObjectCarKind4(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarKind4(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B1D4 ObjectCarKind5
 * WHAT IT DOES: Sets up a car of list kind 5 (+$48 = 0, +$26 = 5) and
 * spawns it (ObjectCarSpawnListed).
 * HOW: As ObjectCarKind3.
 * Entry: as ObjectCarKind3. Exit: as ObjectCarKind3.
 */
void ObjectCarKind5(Carried *cr)
{
    car_of_kind(cr, 0x00, 0x05);
}

void abi_ObjectCarKind5(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarKind5(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ObjectSpawn(void);
void abi_ObjectCallHandler(void);

/* A new object of `kind` ($1F2B, ObjectSpawn): 1 when one was made (C
   set), Y = it. */
static int spawn(Carried *cr, uint16_t kind)
{
    st16i(cr, (cyc(0), 0x7E0000) | W(wram_1F2B), kind);
    (cyc(84), ObjectSpawn_cr(cr));
    return (cr->p & P_C) != 0;
}

/* One shed part: kind `kind` with offset bytes +$48-$4A (o48, o49, o4a),
   its car +$06 = X, +$4B = c, a life +$27 of $FF ($14 with flag bit 1 of
   $1F58), +$25 bit 1 with `mark`; with flag bit 0 (hit) frame $08:ACB9
   (PartKnockOff) or, for the set's first part, $08:AC97 (PartHit) run at
   once (ObjectCallHandler); else $08:AC04 (PartDetach). Returns 1 when the
   crash must end with carry set: no first part, or the first part's hit
   left the car's +$25 bit 2 clear. */
static int shed(Carried *cr, uint16_t kind, uint8_t o48, uint8_t o49, uint8_t o4a, uint8_t c, int mark, int first)
{
    uint8_t a;

    if (!spawn(cr, kind)) {
        /* BCS not taken, JML; for another part the next part's REP, LDA #, STA, less its st16i */
        cyc(first ? 48 : 52);
        return first;
    } else { cyc(28); }
    st8i(cr, F(W(obj_48), cr->y), o48);
    st8i(cr, F(W(obj_49), cr->y), cyc_then(6, o49));
    st8i(cr, F(W(obj_4a), cr->y), cyc_then(6, o4a));
    a = (uint8_t)cr->a;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->a = cr->x;
    set_p(cr, nz16(cr->p, cr->a));
    wr16(F(W(obj_06), cr->y), cyc_then(88, cr->x));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(40);
    lda8(cr, a);
    st8i(cr, F(W(obj_4b), cr->y), c);
    st8i(cr, F(W(obj_27), cr->y), cyc_then(6, 0xFF));
    lda8(cr, cyc_then(22, (uint8_t)(ld8(cr, (cyc(8), 0x7E0000) | W(wram_1F58)) & 0x02)));
    if ((uint8_t)cr->a)
        st8i(cr, F(W(obj_27), cr->y), 0x14);
    if (mark)
        or8(cr, F(W(obj_25), cr->y), 0x02);
    cyc(8);
    lda8(cr, cyc_then(20, (uint8_t)(ld8(cr, WA(wram_1F58)) & 0x01)));
    if (!(uint8_t)cr->a) {
        handler(cr, cr->y, 0xAC04);
        cyc(26);
        return 0;
    } else { cyc(6); }
    if (!first) {
        handler(cr, cr->y, 0xACB9);
        cyc(4);
        return 0;
    }
    handler(cr, cr->y, 0xAC97);
    (cyc(46), ObjectCallHandler_cr(cr));
    cyc(118);
    if (!bits25(cr, cr->x, 0x04))
        { cyc(22); return 1; }          /* BNE not taken, JML: 22 more than the way on */
    return 0;
}

/* 08:A096: the six parts of one car set. */
static int crash_set_a096(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (shed(cr, 0xA8C8, 0xD8, 0xEC, 0xB6, 0x23, 1, 1))
        return 1;
    shed(cr, 0xA83C, 0x00, 0xDC, 0x14, 0x00, 0, 0);
    cyc(0);
    shed(cr, 0xA858, 0x00, 0xEC, 0x1C, 0x0A, 1, 0);
    cyc(0);
    shed(cr, 0xA874, 0x00, 0xE0, 0xE2, 0x14, 0, 0);
    cyc(0);
    shed(cr, 0xA890, 0x00, 0xE8, 0xC6, 0x19, 1, 0);
    cyc(0);
    shed(cr, 0xA8AC, 0x28, 0xEC, 0xB6, 0x1E, 0, 0);
    cyc(24);
    return 0;
}

/* 08:A33D: the six parts of one car set. */
static int crash_set_a33d(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (shed(cr, 0xA44C, 0x00, 0xEE, 0x42, 0x23, 1, 1))
        return 1;
    shed(cr, 0xA430, 0x00, 0xF1, 0x3C, 0x00, 0, 0);
    cyc(0);
    shed(cr, 0xA3C0, 0x00, 0xCE, 0x0F, 0x0A, 1, 0);
    cyc(0);
    shed(cr, 0xA3DC, 0x00, 0xD0, 0xE9, 0x14, 0, 0);
    cyc(0);
    shed(cr, 0xA3F8, 0x00, 0xEF, 0xF6, 0x19, 1, 0);
    cyc(0);
    shed(cr, 0xA414, 0x00, 0x0F, 0x02, 0x1E, 0, 0);
    cyc(24);
    return 0;
}

/* 08:A5E4: the six parts of one car set. */
static int crash_set_a5e4(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (shed(cr, 0xA3A4, 0x00, 0xE3, 0xC5, 0x23, 1, 1))
        return 1;
    shed(cr, 0xA318, 0x00, 0xF9, 0x2D, 0x00, 0, 0);
    cyc(0);
    shed(cr, 0xA334, 0xE9, 0xFA, 0xF8, 0x0A, 1, 0);
    cyc(0);
    shed(cr, 0xA350, 0x17, 0xFA, 0xF8, 0x14, 0, 0);
    cyc(0);
    shed(cr, 0xA36C, 0x00, 0xF6, 0xF6, 0x19, 1, 0);
    cyc(0);
    shed(cr, 0xA388, 0x00, 0xF7, 0xDA, 0x1E, 0, 0);
    cyc(24);
    return 0;
}

/* 08:A88B: the six parts of one car set. */
static int crash_set_a88b(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (shed(cr, 0xA4F4, 0x00, 0xEC, 0xC5, 0x23, 1, 1))
        return 1;
    shed(cr, 0xA468, 0x00, 0xFA, 0x21, 0x00, 0, 0);
    cyc(0);
    shed(cr, 0xA484, 0x00, 0xD5, 0xEE, 0x0A, 1, 0);
    cyc(0);
    shed(cr, 0xA4A0, 0xEF, 0xF6, 0xEC, 0x14, 0, 0);
    cyc(0);
    shed(cr, 0xA4BC, 0x11, 0xF6, 0xEC, 0x19, 1, 0);
    cyc(0);
    shed(cr, 0xA4D8, 0x00, 0xF8, 0xC9, 0x1E, 0, 0);
    return 0;
}

/*
 * @routine 08:A034 CarCrashParts
 * WHAT IT DOES: A car crashes and sheds its six parts, by its model +$27
 * ($81B9/$876E, $853D/$8AA8, $8411/$8990, $82E5/$8886 each pick a set of
 * part kinds and offsets): flags in A (kept at $1F58), bit 0 a hit (the
 * parts knocked off flying, the first at once), bit 1 a 20-frame life;
 * then sound $5B (hit) or $43, carry clear. Carry set with nothing done
 * when the car has already crashed (+$25 bit 2 or 7), its model is none
 * of these or no first part could be made, or carry set after the first
 * part's hit when that left the car's +$25 bit 2 clear.
 * HOW: 16-bit compares, BRL to each set; ObjectSpawn blocks, each storing
 * the part's bytes and frame; ObjectCallHandler for the first hit part.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = the car; A low byte = flags.
 * Exit: M 8-bit; carry as above.
 */
void CarCrashParts(Carried *cr)
{
    static const uint16_t model[8] = {0x81B9, 0x853D, 0x8411, 0x82E5, 0x876E, 0x8AA8, 0x8990, 0x8886};
    static int (*const set[4])(Carried *) = {crash_set_a5e4, crash_set_a88b, crash_set_a33d, crash_set_a096};
    int i, hit;

    WR8(wram_1F58, cyc_then(24, (uint8_t)cr->a));
    cyc(4);
    if (bits25(cr, cr->x, 0x04) || (cyc(0), bits25(cr, cr->x, 0x80)))
        { cyc(22); goto fail; }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(48, F(W(obj_27), cr->x)));
    for (i = 0; i < 8; i++) {
        cyc(46);
        r_cmp16(cr, model[i]);
        if (cr->a == model[i])
            { cyc(50); break; }
    }
    if (i == 8 || set[i & 3](cr))
        goto fail; else { cyc(4); }
    lda8(cr, cyc_then(34, (uint8_t)(ld8(cr, WA(wram_1F58)) & 0x01)));
    hit = (uint8_t)cr->a != 0;
    sound0(cr, hit ? 0x5B : (cyc(6), 0x43));
    cyc(hit ? 80 : 58);                 /* BRA after the hit's sound, CLC, RTL */
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    return;
fail:
    cyc(80);                            /* SEP, SEC, RTL */
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    set_p(cr, (uint8_t)(cr->p | (P_C)));
}

void abi_CarCrashParts(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CarCrashParts(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_RandomStepLong(void);
void abi_ObjectCarFrame(void);

/* PHX, REP #$20, the object's record word at `field` (long, through X =
   +$22) changed by `op` with m, PLX, SEP #$20 (the SEP after the PLX when
   `sep_last`). 0: OR, 1: AND, 2: EOR. */
static void rec_word(Carried *cr, uint32_t field, int op, uint16_t m)
{
    uint16_t sx = cr->x;
    uint32_t at;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(76, F(W(obj_22), cr->x)));
    cr->x = cr->a;
    set_p(cr, nz16(cr->p, cr->x));
    at = (field + cr->x) & 0xFFFFFF;
    cyc(46);
    r_lda16(cr, at);
    cr->a = op == 0 ? (uint16_t)(cr->a | m) : op == 1 ? (uint16_t)(cr->a & m) : (uint16_t)(cr->a ^ m);
    set_p(cr, nz16(cr->p, cr->a));
    cyc(56);
    wr16(at, cr->a);
    cr->x = sx;
    set_p(cr, nz16(cr->p, sx));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

/* 08:B0F4: the rival car's frame (see ObjectRivalCarStep). */
static void rival_frame(Carried *cr)
{
    uint16_t car;
    uint8_t v;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(46, WA(wram_1AEC)));
    r_and16(cr, 0x0400);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (cr->a) {
        cyc(74);
        ld8(cr, F(W(obj_36), cr->x));
        cmp8(cr, 0x01);
        if ((uint8_t)cr->a == 0x01) {
            cyc(6);
            st8i(cr, F(W(obj_36), cr->x), 0x02);
            cyc(6);
            rec_word(cr, 0x70001C, 0, 0x0020);
            cyc(58);
        } else {
            rec_word(cr, 0x70001C, 1, 0xFFDF);
            cyc(80);                    /* SEP, PLX, BRA */
        }
    } else { cyc(66); }
    (cyc(62), w_RandomStepLong(cr));
    cmp8(cr, 0xF2);
    if ((uint8_t)cr->a >= 0xF2)
        { cyc(6); rec_word(cr, 0x700070, 2, 0x0002); cyc(58); } else { cyc(26); }
    car = cr->x;
    WR16(wram_1F18, cyc_then(24, car));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(52, F(W(obj_22), cr->x)));
    cr->x = cr->a;
    set_p(cr, nz16(cr->p, cr->x));
    cyc(46);
    r_lda16(cr, (0x70002C + cr->x) & 0xFFFFFF);
    r_and16(cr, 0x0004);
    if (cr->a) {
        cr->x = rd16(cyc_then(62, WA(wram_1F18)));
        set_p(cr, nz16(cr->p, cr->x));
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        or8(cr, F(W(obj_25), cr->x), cyc_then(22, 0x10));
        cyc(62);
        lda8(cr, 0x03);
        CarCrashParts(cr);
        if (!(cr->p & P_C)) {
            cyc(40);                    /* BCS not taken, DEC abs to its read */
            v = (uint8_t)(RD8(script_byte_1f71) - 1);
            cyc(6);
            WR8(script_byte_1f71, v);
            set_p(cr, nz8(cr->p, v));
            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
            r_lda16(cr, cyc_then(52, F(W(obj_22), cr->x)));
            cr->x = cr->a;
            set_p(cr, nz16(cr->p, cr->x));
            cyc(46);
            r_lda16(cr, (0x700016 + cr->x) & 0xFFFFFF);
            cr->a |= 0x0020;
            set_p(cr, nz16(cr->p, cr->a));
            cyc(56);
            wr16((0x700016 + cr->x) & 0xFFFFFF, cr->a);
            cyc(32);
            r_lda16(cr, (0x70001C + cr->x) & 0xFFFFFF);
            cr->a |= 0x0010;
            set_p(cr, nz16(cr->p, cr->a));
            cyc(56);
            wr16((0x70001C + cr->x) & 0xFFFFFF, cr->a);
        } else { cyc(22); }
    } else { cyc(44); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->x = rd16(cyc_then(46, WA(wram_1F18)));
    set_p(cr, nz16(cr->p, cr->x));
    cyc(4);
    if (bits25b(cr, cr->x, 0x04))
        or8(cr, F(W(obj_24), cr->x), 0x20);
    else
        cyc(22);
    cyc(24);
    ObjectCarFrame(cr);
}

/* A rival car's set-up: counted in $1F71, +$48 = 0, list kind +$26 = 0,
   spawned (ObjectCarSpawnListed), its record's +$64 bit 4 set (and +$70 bit
   1 with `b`), frame $08:B0F4; then that frame. */
static void rival_car(Carried *cr, int b)
{
    uint16_t sx;
    uint32_t at;

    inc8(cr, WA(script_byte_1f71));
    st8i(cr, F(W(obj_48), cr->x), cyc_then(6, 0x00));
    st8i(cr, F(W(obj_26), cr->x), cyc_then(6, 0x00));
    (cyc(62), ObjectCarSpawnListed(cr));
    sx = cr->x;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(82, F(W(obj_22), cr->x)));
    cr->x = cr->a;
    set_p(cr, nz16(cr->p, cr->x));
    at = (0x700064 + cr->x) & 0xFFFFFF;
    cyc(46);
    r_lda16(cr, at);
    cr->a |= 0x0010;
    set_p(cr, nz16(cr->p, cr->a));
    cyc(56);
    wr16(at, cr->a);
    if (b) {
        at = (0x700070 + cr->x) & 0xFFFFFF;
        cyc(32);
        r_lda16(cr, at);
        cr->a |= 0x0002;
        set_p(cr, nz16(cr->p, cr->a));
        cyc(56);
        wr16(at, cr->a);
    }
    cyc(62);
    cr->x = sx;
    set_p(cr, nz16(cr->p, sx));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    handler(cr, cr->x, 0xB0F4);
    (cyc(24), rival_frame(cr));
}

/*
 * @routine 08:B079 ObjectRivalCar
 * WHAT IT DOES: Sets up a rival car: counted in $1F71, +$48 = 0, list kind
 * 0, spawned as a car (ObjectCarSpawnListed), its record's +$64 bit 4 set,
 * frame $08:B0F4 (ObjectRivalCarStep); then that frame.
 * HOW: Byte stores, the record word through X (PHX/PLX); a JMP into the
 * frame. Reached only by unit_rival_cars_08.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectRivalCar(Carried *cr)
{
    rival_car(cr, 0);
}

void abi_ObjectRivalCar(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRivalCar(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B0B1 ObjectRivalCarB
 * WHAT IT DOES: As ObjectRivalCar, the record's +$70 bit 1 set as well.
 * HOW: As ObjectRivalCar. Nothing in the game reaches it; reached only by
 * unit_rival_cars_08.
 * Entry: as ObjectRivalCar. Exit: as ObjectRivalCar.
 */
void ObjectRivalCarB(Carried *cr)
{
    rival_car(cr, 1);
}

void abi_ObjectRivalCarB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRivalCarB(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B0F4 ObjectRivalCarStep
 * WHAT IT DOES: The rival car's frame: with $1AEC bit 10, its record's +$1C
 * bit 5 follows +$36 = 1 (then +$36 = 2); one frame in 14 (the random step
 * at $F2 or above) its record's +$70 bit 1 flips; when its record's +$2C
 * bit 2 is set it is marked (+$25 bit 4) and crashes (CarCrashParts with
 * flags 3), and if it did, $1F71 counts down and the record's flags +$16
 * bit 5 and +$1C bit 4 are set; with +$25 bit 2, +$24 bit 5; then the car's
 * frame (ObjectCarFrame).
 * HOW: The record words through X = +$22 (long, 16-bit); the car kept at
 * $1F18 across; JMP ObjectCarFrame. Reached only by unit_rival_cars_08.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectCarFrame
 * leaves it.
 */
void ObjectRivalCarStep(Carried *cr)
{
    rival_frame(cr);
}

void abi_ObjectRivalCarStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRivalCarStep(&r);
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
