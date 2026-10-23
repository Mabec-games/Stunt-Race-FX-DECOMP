/*
 * object.c: the object table in work RAM bank $7E. Objects are $62-byte
 * records from $7E:2000; free records form a list linked through their first
 * word. Fields used here: +$0C/+$0E/+$10 position (16-bit each), +$14-$16
 * three bytes copied as a group, +$1A a word, +$2E/+$30/+$32 velocity,
 * +$4F the link to a related object.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "carried.h"

uint16_t GsuRecordFlag4(uint8_t bank, uint16_t obj, int *has);
uint16_t GsuRecordFromObject(uint8_t bank, uint16_t obj);
uint8_t GsuRun(uint8_t bank, uint16_t start);
uint16_t ObjectPoseFromRecord(uint8_t bank, uint16_t obj, uint8_t *p);
uint8_t RandomStepLong(void);
void abi_ObjectAttachSpawn(void);

/* A field of the object at offset `obj` in the data bank. */
static uint32_t field(uint8_t bank, uint16_t obj, uint16_t off)
{
    return (uint32_t)bank << 16 | (uint16_t)(0x2000 + off + obj);
}

/*
 * @routine 03:89A8 ObjectLinkedSet1A
 * WHAT IT DOES: Stores A in field +$1A of the object that X's object links to.
 * HOW: Reads the link at +$4F, then stores there (bank $7E, long addressing).
 * Entry: M 16-bit, X 16-bit; A = value, X = object. Exit: A unchanged; X =
 * the linked object; N, Z from A.
 */
uint16_t ObjectLinkedSet1A(uint16_t obj, uint16_t value)
{
    cyc(62);
    uint16_t linked = rd16(0x7E204F + obj);

    cyc(82);
    wr16(0x7E201A + linked, value);
    cyc(44);
    return linked;
}

void BucketTriggersFixedArea_cr(Carried *cr);
void BucketTriggersFromArea_cr(Carried *cr);
void BucketTriggersFromView_cr(Carried *cr);
void CarSpawnCrash(Carried *cr);
void ObjectAdvance5DIfFlag(Carried *cr);
void ObjectAlloc_cr(Carried *cr);
void ObjectApplyVelocity_cr(Carried *cr);
void ObjectAttachLinked_cr(Carried *cr);
void ObjectAttachedSignX(Carried *cr);
void ObjectAttachedStart(Carried *cr);
void ObjectAttachedStep(Carried *cr);
void ObjectBillboardTilt(Carried *cr);
void ObjectBucketInsert_cr(Carried *cr);
void ObjectBucketRemove_cr(Carried *cr);
void ObjectCallHandler_cr(Carried *cr);
void ObjectCarFrame(Carried *cr);
void ObjectCarPoseStep(Carried *cr);
void ObjectCarRecordNew(Carried *cr);
void ObjectCarRecordRestart(Carried *cr);
void ObjectCarRecordSetup(Carried *cr);
void ObjectCarRecordTake(Carried *cr);
void ObjectCarSpawn(Carried *cr);
void ObjectCarSpawnPlace(Carried *cr);
void ObjectCopy14From_cr(Carried *cr);
void ObjectCopy14To_cr(Carried *cr);
void ObjectCopyPositionFrom_cr(Carried *cr);
void ObjectCopyPositionTo_cr(Carried *cr);
void ObjectCountStep5F_cr(Carried *cr);
void ObjectFlag40IfMatch_cr(Carried *cr);
void ObjectFree_cr(Carried *cr);
void ObjectFreeLong_cr(Carried *cr);
void ObjectGroupsActivate_cr(Carried *cr);
void ObjectGroupsActivateClear08(Carried *cr);
void ObjectInit_cr(Carried *cr);
void ObjectLinkBobStart(Carried *cr);
void ObjectLinkEventTrigger_cr(Carried *cr);
void ObjectLinkEventTrigger2(Carried *cr);
void ObjectLinkFlag4000Step_cr(Carried *cr);
void ObjectLinkSpeedSet(Carried *cr);
void ObjectLinkedHandlerByState_cr(Carried *cr);
void ObjectLinkedSet1A_cr(Carried *cr);
void ObjectListAppend_cr(Carried *cr);
void ObjectListClear(Carried *cr);
void ObjectListRemove(Carried *cr);
void ObjectMoveForward(Carried *cr);
void ObjectMoveForwardXSign(Carried *cr);
void ObjectMoveForwardZSign(Carried *cr);
void ObjectNearFocus_cr(Carried *cr);
void ObjectPlaceAll_cr(Carried *cr);
void ObjectPlaceFromTable_cr(Carried *cr);
void ObjectPlaceOnParent(Carried *cr);
void ObjectPoolInit_cr(Carried *cr);
void ObjectRebucket_cr(Carried *cr);
void ObjectRecordFlag70Step_cr(Carried *cr);
void ObjectRelease_cr(Carried *cr);
void ObjectReleaseLong(Carried *cr);
void ObjectReleaseWhenPassed(Carried *cr);
void ObjectReleaseWhenPassedBehind(Carried *cr);
void ObjectReleaseY_cr(Carried *cr);
void ObjectRunHandlerA_cr(Carried *cr);
void ObjectRunHandlerB_cr(Carried *cr);
void ObjectSet5DByKind_cr(Carried *cr);
void ObjectSet5DByKindLong(Carried *cr);
void ObjectSparkleStart(Carried *cr);
void ObjectSparkleStep(Carried *cr);
void ObjectSpawn_cr(Carried *cr);
void ObjectSpawnAttached(Carried *cr);
void ObjectSpawnChanceD460(Carried *cr);
void ObjectSpawnD460UnlessFlag(Carried *cr);
void ObjectSpawnFromScript(Carried *cr);
void ObjectSpawnPuff(Carried *cr);
void ObjectSpawnSparklePair(Carried *cr);
void ObjectSpawnTrail(Carried *cr);
void ObjectStep48To8_cr(Carried *cr);
void ObjectTableC056_cr(Carried *cr);
void ObjectTableE8AF_cr(Carried *cr);
void ObjectTimedSetup(Carried *cr);
void ObjectTimedStep(Carried *cr);
void ObjectTrailSignX(Carried *cr);
void ObjectTrailSignY(Carried *cr);
void ObjectTrailStart(Carried *cr);
void ObjectTrailStep(Carried *cr);
void ObjectVelocityFromRecord_cr(Carried *cr);
void PlayerLinkSet27(Carried *cr);
void ObjectCallHandler(Carried *cr, uint16_t obj);
void ObjectReleaseY(Carried *cr);
void ObjectRunHandlerA(Carried *cr, uint8_t bank, uint16_t obj);
void ObjectRunHandlerB(Carried *cr, uint8_t bank, uint16_t obj);
void ObjectsRunHandlers(Carried *cr);
static void abi_triggers_from(Carried *cr, uint16_t rec, uint16_t x);
static void add_random(Carried *cr, uint32_t b, uint16_t off, uint16_t mask);
static void camera_turn_negated(Carried *cr, uint32_t b, uint16_t camera);
static uint32_t car_field(Carried *cr, uint16_t off);
static void child(Carried *cr, uint16_t off);
static uint8_t cmp8_flags(Carried *cr, uint8_t a, uint8_t m);
static uint32_t dp8(Carried *cr, uint8_t off);
static void move_forward_from(Carried *cr, int arm);
static uint8_t neg8(Carried *cr, uint8_t a);
static void parent_place(Carried *cr, uint32_t b, int attached);
static void parent_place_from(Carried *cr, uint32_t b, int attached, int stage);
static void passed_tail(Carried *cr, unsigned extra);
static void release(Carried *cr);
static int run_handler(Carried *cr, uint8_t bank, uint16_t obj, uint16_t word_at, uint16_t bank_at, int need_no_record);
static void set_handler(Carried *cr, uint16_t o, uint16_t h);
static void spawn_sparkle(Carried *cr, uint32_t b);
static void swap_xy(Carried *cr);
static void w_GsuRecordFlag4(Carried *cr);
static void w_GsuRecordFromObject(Carried *cr);
static void w_GsuRun(Carried *cr);
static void w_ObjectPoseFromRecord(Carried *cr);
static void w_RandomStepLong(Carried *cr);
void CrashGreen_cr(Carried *cr);
void CrashWhite_cr(Carried *cr);
void ObjectAttachSpawn(Carried *cr);
void ObjectChildrenSpawn(Carried *cr);
void RotateXY_cr(Carried *cr);
void RotateXZFrom08_cr(Carried *cr);
void RotateXZ_cr(Carried *cr);

void ObjectLinkedSet1A_cr(Carried *cr)
{
    cr->x = ObjectLinkedSet1A(cr->x, cr->a);
    set_p(cr, nz16(cr->p, cr->a));
}

void abi_ObjectLinkedSet1A(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLinkedSet1A_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Copy n bytes of fields from one object to another, in the data bank. */
static uint8_t copy_bytes(uint8_t bank, uint16_t from, uint16_t to, uint16_t off, unsigned n)
{
    uint8_t v = 0;
    unsigned i;

    cyc(0);
    for (i = 0; i < n; i++) {
        v = rd8(cyc_then(30, field(bank, from, (uint16_t)(off + i))));
        wr8(field(bank, to, (uint16_t)(off + i)), cyc_then(30, v));
    }
    cyc(44);
    return v;
}

static uint16_t copy_words(uint8_t bank, uint16_t from, uint16_t to)
{
    uint16_t v = 0;
    unsigned i;

    cyc(22);
    for (i = 0; i < 3; i++) {
        v = rd16(cyc_then(30, field(bank, from, (uint16_t)(0x0C + 2 * i))));
        wr16(field(bank, to, (uint16_t)(0x0C + 2 * i)), cyc_then(30, v));
    }
    cyc(66);
    return v;
}

/*
 * @routine 03:D214 ObjectCopy14To
 * WHAT IT DOES: Copies object X's three bytes at +$14-$16 to object Y.
 * HOW: Three byte copies, in the data bank.
 * Entry: M 8-bit, X 16-bit; DB = the object table's bank. Exit: A low byte =
 * the last byte; N, Z from it.
 */
void ObjectCopy14To(uint8_t bank, uint16_t from, uint16_t to)
{
    copy_bytes(bank, from, to, 0x14, 3);
}

void ObjectCopy14To_cr(Carried *cr)
{
    uint8_t v = copy_bytes(cr->db, cr->x, cr->y, 0x14, 3);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
}

void abi_ObjectCopy14To(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCopy14To_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D227 ObjectCopy14From
 * WHAT IT DOES: Copies object Y's three bytes at +$14-$16 to object X.
 * HOW: As ObjectCopy14To, the other way.
 * Entry: as ObjectCopy14To. Exit: as ObjectCopy14To.
 */
void ObjectCopy14From(uint8_t bank, uint16_t to, uint16_t from)
{
    copy_bytes(bank, from, to, 0x14, 3);
}

void ObjectCopy14From_cr(Carried *cr)
{
    uint8_t v = copy_bytes(cr->db, cr->y, cr->x, 0x14, 3);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
}

void abi_ObjectCopy14From(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCopy14From_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D1E6 ObjectCopyPositionTo
 * WHAT IT DOES: Copies object X's position to object Y.
 * HOW: Three 16-bit copies (+$0C, +$0E, +$10), in the data bank.
 * Entry: M 8-bit, X 16-bit; DB = the object table's bank. Exit: A = the
 * last word; N, Z from it (16-bit); M 8-bit.
 */
uint16_t ObjectCopyPositionTo(uint8_t bank, uint16_t from, uint16_t to)
{
    return copy_words(bank, from, to);
}

void ObjectCopyPositionTo_cr(Carried *cr)
{
    cr->a = copy_words(cr->db, cr->x, cr->y);
    set_p(cr, (uint8_t)(nz16(cr->p, cr->a) | P_M));
}

void abi_ObjectCopyPositionTo(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCopyPositionTo_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D1FD ObjectCopyPositionFrom
 * WHAT IT DOES: Copies object Y's position to object X.
 * HOW: As ObjectCopyPositionTo, the other way.
 * Entry: as ObjectCopyPositionTo. Exit: as ObjectCopyPositionTo.
 */
uint16_t ObjectCopyPositionFrom(uint8_t bank, uint16_t to, uint16_t from)
{
    return copy_words(bank, from, to);
}

void ObjectCopyPositionFrom_cr(Carried *cr)
{
    cr->a = copy_words(cr->db, cr->y, cr->x);
    set_p(cr, (uint8_t)(nz16(cr->p, cr->a) | P_M));
}

void abi_ObjectCopyPositionFrom(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCopyPositionFrom_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:C06C ObjectApplyVelocity
 * WHAT IT DOES: Moves object X by its velocity.
 * HOW: Adds +$2E, +$30, +$32 to +$0C, +$0E, +$10 (16-bit), in the data bank.
 * Entry: M 8-bit, X 16-bit; DB = the object table's bank. Exit: A = the new
 * +$10; flags from the last add; M 8-bit.
 */
static uint16_t apply_velocity(uint8_t bank, uint16_t obj, uint8_t *p)
{
    uint16_t r = 0;
    unsigned i;

    cyc(22);
    for (i = 0; i < 3; i++) {
        uint16_t a = rd16(cyc_then(30, field(bank, obj, (uint16_t)(0x0C + 2 * i))));
        uint16_t b = rd16(cyc_then(44, field(bank, obj, (uint16_t)(0x2E + 2 * i))));
        uint32_t s = (uint32_t)a + b;
        r = (uint16_t)s;
        wr16(field(bank, obj, (uint16_t)(0x0C + 2 * i)), cyc_then(30, r));
        *p = (uint8_t)((nz16(*p, r) & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0)
                       | ((~(a ^ b) & (a ^ r) & 0x8000) ? P_V : 0));
    }
    cyc(66);
    return r;
}

uint16_t ObjectApplyVelocity(uint8_t bank, uint16_t obj, uint8_t *p)
{
    return apply_velocity(bank, obj, p);
}

void ObjectApplyVelocity_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = apply_velocity(cr->db, cr->x, &p);
    set_p(cr, (uint8_t)(p | P_M));
}

void abi_ObjectApplyVelocity(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectApplyVelocity_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D3DE ObjectPoolInit
 * WHAT IT DOES: Empties the object table: links all $136 records into the
 * free list and clears the counters at $0329-$032E.
 * HOW: Starting at offset 1, each record's first word gets the next record's
 * offset ($62 on); the last gets 0. Data bank $7E inside.
 * Entry: any widths. Exit: A = 1 + $136 * $62; X = the last record; Y = 0;
 * P unchanged.
 */
void ObjectPoolInit(void)
{
    uint16_t x = 1;
    unsigned i;

    cyc(178);
    wr16(0x7E0329, 0);
    cyc(24);
    wr16(0x7E032B, 0);
    cyc(48);
    wr16(0x7E032D, 1);
    for (i = 0; i < 0x136; i++) {
        uint16_t next = (uint16_t)(x + 0x62);
        cyc(106);
        wr16(0x7E2000 + x, next);
        if (i + 1 < 0x136)
            { cyc(12); x = next; }
    }
    cyc(60);
    wr16(0x7E2000 + x, 0);
}

void ObjectPoolInit_cr(Carried *cr)
{
    ObjectPoolInit();
    cr->a = (uint16_t)(1 + 0x136 * 0x62);
    cr->x = (uint16_t)(1 + 0x135 * 0x62);
    cr->y = 0;
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
    } else { cyc(106); }
}

void abi_ObjectPoolInit(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPoolInit_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 7F:12AC ObjectLinkFlag4000Step
 * WHAT IT DOES: When the object two links on from the one at $0346 has bit
 * 14 set in its first word, adds 8 to $06CD.
 * HOW: Follows the link at +$4F twice (bank $7E), reads the record's first
 * word in bank 0.
 * Entry: any widths. Exit: A = the masked word, or the new $06CD; X, P
 * unchanged.
 */
uint16_t ObjectLinkFlag4000Step(void)
{
    cyc(98);
    uint16_t x = rd16(0x000346), a;

    cyc(32);
    x = rd16(0x7E204F + x);
    cyc(46);
    x = rd16(0x7E204F + x);
    cyc(46);
    a = rd16(0x000000 + x) & 0x4000;
    if (!a)
        { cyc(46); return a; }
    cyc(72);
    a = (uint16_t)(rd16(0x0006CD) + 8);
    cyc(70);
    wr16(0x0006CD, a);
    return a;
}

void ObjectLinkFlag4000Step_cr(Carried *cr)
{
    cr->a = ObjectLinkFlag4000Step();
    cyc(108);
}

void abi_ObjectLinkFlag4000Step(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLinkFlag4000Step_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B830 ObjectFlag40IfMatch
 * WHAT IT DOES: When A equals the word at $70:2476, sets bit 6 of field
 * +$25 of the object at $0346.
 * HOW: 16-bit compare, then an 8-bit OR through the data bank.
 * Entry: M 16-bit, X 16-bit; A = value. Exit: no match: A unchanged; M
 * 16-bit; N, Z, C from the compare. Match: A low byte = the new field; X =
 * the object; M 8-bit; N, Z from it; C set.
 */
int ObjectFlag40IfMatch(uint8_t bank, uint16_t a, uint16_t *x, uint8_t *v, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    cyc(54);
    uint16_t m = rd16(0x702476);

    *p = (uint8_t)((nz16(*p, (uint16_t)(a - m)) & ~P_C) | (a >= m ? P_C : 0));
    if (a != m)
        { cyc(22); return 0; }
    cyc(40);
    *x = rd16(b | 0x0346);
    cyc(52);
    *v = (uint8_t)(rd8(b | (uint16_t)(0x2025 + *x)) | 0x40);
    cyc(46);
    wr8(b | (uint16_t)(0x2025 + *x), *v);
    *p = nz8(*p, *v);
    return 1;
}

void ObjectFlag40IfMatch_cr(Carried *cr)
{
    uint8_t v = 0, p = cr->p;

    if (ObjectFlag40IfMatch(cr->db, cr->a, &cr->x, &v, &p)) {
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        p |= P_M;
    }
    set_p(cr, p);
}

void abi_ObjectFlag40IfMatch(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFlag40IfMatch_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* An 8-bit add as ADC with carry clear: the sum, with C and V. */
static uint8_t add8(uint8_t a, uint8_t b, uint8_t *p)
{
    unsigned s = (unsigned)a + b;

    *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFF ? P_C : 0) | ((~(a ^ b) & (a ^ (uint8_t)s) & 0x80) ? P_V : 0));
    return (uint8_t)s;
}

/*
 * @routine 08:E896 ObjectTableE8AF
 * WHAT IT DOES: Returns the byte of the table at $08:E8AF for (A + object
 * X's +$15 + $1F4C) / 8.
 * HOW: 8-bit adds, three right shifts.
 * Entry: M 8-bit, X 16-bit; X = object, A = value. Exit: A low byte = the
 * table byte; X, Y unchanged; N, Z from X; C from the last shift; V from
 * the second add.
 */
uint8_t ObjectTableE8AF(uint8_t bank, uint8_t a, uint16_t obj, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;

    cyc(44);
    a = add8(a, rd8(b | (uint16_t)(0x2015 + obj)), p);
    cyc(38);
    a = add8(a, rd8(b | 0x1F4C), p);
    *p = (uint8_t)((*p & ~P_C) | (a & 0x04 ? P_C : 0));
    cyc(314);
    return rd8(0x08E8AF + (a >> 3));
}

void ObjectTableE8AF_cr(Carried *cr)
{
    uint8_t p = cr->p, v = ObjectTableE8AF(cr->db, (uint8_t)cr->a, cr->x, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz16(p, cr->x));
}

void abi_ObjectTableE8AF(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTableE8AF_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:C03C ObjectTableC056
 * WHAT IT DOES: Sets $1F5A to the byte of the table at $08:C056 for (object
 * Y's +$15 - $1F5A) / 4.
 * HOW: An 8-bit subtract, two right shifts.
 * Entry: M 8-bit, X 16-bit; Y = object. Exit: A low byte = the new $1F5A;
 * X, Y unchanged; N, Z from Y; C from the last shift; V from the subtract.
 */
uint8_t ObjectTableC056(uint8_t bank, uint16_t obj, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    cyc(98);
    uint8_t f = rd8(b | (uint16_t)(0x2015 + obj)), s = rd8(cyc_then(38, b | 0x1F5A)), d = (uint8_t)(f - s), v;

    *p = (uint8_t)((*p & ~(P_C | P_V)) | (d & 0x02 ? P_C : 0) | (((f ^ s) & (f ^ d) & 0x80) ? P_V : 0));
    cyc(188);
    v = rd8(0x08C056 + (d >> 2));
    wr8(b | 0x1F5A, v);
    cyc(136);
    return v;
}

void ObjectTableC056_cr(Carried *cr)
{
    uint8_t p = cr->p, v = ObjectTableC056(cr->db, cr->y, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz16(p, cr->y));
}

void abi_ObjectTableC056(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTableC056_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B75B ObjectListAppend
 * WHAT IT DOES: Appends object X to the zero-ended list of up to 32 words at
 * $7E:FFBD, whose length in bytes is at $7E:FFFD; a full list is left alone.
 * HOW: Stores X at the end, steps the length by 2 and stores the 0 that
 * ends the list.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A = 0, or $40 when full (M
 * 8-bit); X, Y unchanged; N, Z from X; C from the full-length compare.
 */
static int object_list_append(uint16_t obj, uint16_t *len)
{
    cyc(128);
    uint16_t n = rd16(0x7EFFFD);

    *len = n;
    if (n == 0x0040)
        return 0;
    cyc(100);
    wr16(0x7EFFBD + n, obj);
    n = (uint16_t)(n + 2);
    cyc(74);
    wr16(0x7EFFFD, n);
    cyc(56);
    wr16(0x7EFFBD + n, 0x0000);
    return 1;
}

int ObjectListAppend(uint16_t obj)
{
    uint16_t n;

    return object_list_append(obj, &n);
}

void ObjectListAppend_cr(Carried *cr)
{
    uint16_t n;

    cr->a = object_list_append(cr->x, &n) ? (cyc(138), 0x0000) : 0x0040;
    set_p(cr, (uint8_t)((nz16(cr->p, cr->x) & ~P_C) | (n >= 0x0040 ? P_C : 0) | P_M));
}

void abi_ObjectListAppend(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectListAppend_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:8364 ObjectBucketInsert
 * WHAT IT DOES: Puts object X at the head of the list for its bucket: the
 * bucket number is the object's +$40, the list heads are words at $7F:0000
 * + the bucket, and +$3C/+$3E link to the next and previous objects.
 * HOW: The old head becomes the object's next (+$3C, its previous cleared)
 * and gets the object as its previous; the object becomes the head. Direct
 * page $40 holds the object while the old head is updated. Data bank $7E
 * inside.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = object. Exit: A = the object;
 * X, Y unchanged; N, Z from the caller's data bank.
 */
void ObjectBucketInsert(uint16_t obj)
{
    cyc(222);
    uint16_t bucket = rd16(0x7E2040 + obj), head;

    cyc(30);
    wr16(0x7E203E + obj, 0x0000);
    cyc(60);
    head = rd16(0x7F0000 + bucket);
    cyc(30);
    wr16(0x7E203C + obj, head);
    if (head) {
        cyc(32);
        wr16(0x000040, obj);
        cyc(30);
        wr16(0x7E203E + head, cyc_then(30, rd16(0x000040)));
    } else { cyc(8); }
    cyc(60);
    wr16(0x7F0000 + bucket, obj);
}

void ObjectBucketInsert_cr(Carried *cr)
{
    ObjectBucketInsert(cr->x);
    cr->a = cr->x;
    set_p(cr, nz8(cr->p, cr->db));
}

void abi_ObjectBucketInsert(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBucketInsert_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* An arithmetic shift right by one, as CMP #$8000 / ROR A does it. */
static uint16_t asr1c(uint16_t v, uint8_t *p)
{
    *p = (uint8_t)((*p & ~P_C) | (v & 1 ? P_C : 0));
    return (uint16_t)((v >> 1) | (v & 0x8000));
}

/*
 * @routine 08:D5AB ObjectVelocityFromRecord
 * WHAT IT DOES: Sets object X's X and Z velocity (+$2E, +$32) to an eighth
 * of the values at +$2E and +$32 of the Super FX record of the object named
 * by its +$27.
 * HOW: Each value is shifted right three times keeping its sign.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A = the Z velocity (M 8-bit);
 * X, Y = the object; N, Z from X; C from the last shift.
 */
uint16_t ObjectVelocityFromRecord(uint8_t bank, uint16_t obj, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    cyc(30);
    uint16_t rec = rd16(cyc_then(52, b | (uint16_t)(0x2022 + rd16(b | (uint16_t)(0x2027 + obj))))), v;

    cyc(60);
    v = asr1c(asr1c(asr1c(rd16(0x70002E + rec), p), p), p);
    cyc(144);
    wr16(b | (uint16_t)(0x202E + obj), v);
    cyc(32);
    v = asr1c(asr1c(asr1c(rd16(0x700032 + rec), p), p), p);
    cyc(144);
    wr16(b | (uint16_t)(0x2032 + obj), v);
    *p = nz16(*p, obj);
    cyc(78);
    return v;
}

void ObjectVelocityFromRecord_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ObjectVelocityFromRecord(cr->db, cr->x, &p);
    cr->y = cr->x;
    set_p(cr, (uint8_t)(p | P_M));
}

void abi_ObjectVelocityFromRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectVelocityFromRecord_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 08:8392 ObjectBucketRemove
 * WHAT IT DOES: Takes object X out of its bucket's list (see
 * ObjectBucketInsert): its previous object, or the bucket's head when it has
 * none, is linked to its next, and its next gets its previous.
 * HOW: Data bank $7E inside; the head is $7F:0000 + the object's +$40.
 * Entry: X 16-bit; X = object. Exit: A = the previous object (or the next,
 * when there is no next or no previous); X, Y unchanged; M 16-bit; N, Z
 * from the caller's data bank.
 */
uint16_t ObjectBucketRemove(uint16_t obj)
{
    cyc(236);
    uint16_t prev = rd16(0x7E203E + obj), next, bucket;

    if (prev) {
        cyc(60);
        next = rd16(0x7E203C + obj);
        cyc(30);
        wr16(0x7E203C + prev, next);
        if (!next)
            { cyc(166); return next; }
        cyc(74);
        wr16(0x7E203E + next, prev);
        cyc(166);
        return prev;
    }
    cyc(52);
    bucket = rd16(0x7E2040 + obj);  /* the bucket before the next object */
    cyc(44);
    next = rd16(0x7E203C + obj);
    cyc(32);
    wr16(0x7F0000 + bucket, next);
    if (next)
        { cyc(60); wr16(0x7E203E + next, 0x0000); } else { cyc(22); }
    cyc(144);
    return next;
}

void ObjectBucketRemove_cr(Carried *cr)
{
    cr->a = ObjectBucketRemove(cr->x);
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~P_M));
}

void abi_ObjectBucketRemove(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBucketRemove_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B48E ObjectSet5DByKind
 * WHAT IT DOES: Sets object X's +$5D to $9163, $9335 or $9507 for A = 1, 2 or
 * 3, and to $8F91 otherwise.
 * HOW: Compares in turn; one 16-bit store through the data bank.
 * Entry: M 8-bit, X 16-bit; X = object, A = kind. Exit: A = the value (M
 * 8-bit); N set, Z clear; C from the last compare.
 */
uint16_t ObjectSet5DByKind(uint8_t bank, uint16_t obj, uint8_t kind, uint8_t *p)
{
    uint16_t v = kind == 1 ? 0x9163 : (cyc(32), kind == 2 ? 0x9335 : (cyc(26), kind == 3 ? (cyc(6), 0x9507) : 0x8F91));
    uint8_t last = kind == 1 ? 1 : kind == 2 ? 2 : 3;

    *p = (uint8_t)((*p & ~(P_C | P_Z)) | P_N | (kind >= last ? P_C : 0));
    cyc(160);
    wr16((uint32_t)bank << 16 | (uint16_t)(0x205D + obj), v);
    return v;
}

void ObjectSet5DByKind_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ObjectSet5DByKind(cr->db, cr->x, (uint8_t)cr->a, &p);
    set_p(cr, (uint8_t)(p | P_M));
    cyc(64);
}

void abi_ObjectSet5DByKind(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSet5DByKind_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B48A ObjectSet5DByKindLong
 * WHAT IT DOES: ObjectSet5DByKind for a JSL caller; nothing in the game
 * calls it, so a direct call is its only way in.
 * HOW: JSR, RTL.
 * Entry: as ObjectSet5DByKind. Exit: as ObjectSet5DByKind.
 */
void ObjectSet5DByKindLong(Carried *cr)
{
    ObjectSet5DByKind_cr(cr);
    cyc(44);                            /* RTL */
}

void abi_ObjectSet5DByKindLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSet5DByKindLong(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B8F9 ObjectPlaceFromTable
 * WHAT IT DOES: Sets up the object in entry A (from 1) of the list at
 * $7E:FFAE: its +$27 from $1F2D, its +$49 and +$4A from the bytes at $1F2B
 * and $1F4C, and, unless bit 0 of $1AEE is set, its position (+$0C, +$0E,
 * +$10) and +$15 byte from entry X (from 1) of the 8-byte table at
 * $7E:FF66.
 * HOW: Data bank $7E inside; word copies, then byte copies.
 * Entry: M 16-bit, X 16-bit; A low byte = list entry, X low byte = table
 * entry. Exit: A low byte = the byte from $1F4C; X = the table offset; Y =
 * the object; P unchanged.
 */
uint16_t ObjectPlaceFromTable(uint8_t list, uint8_t entry, uint16_t *x, uint16_t *y)
{
    cyc(416);
    uint16_t obj = rd16(0x7EFFAE + (uint16_t)((list - 1) << 1)), t = (uint16_t)((uint16_t)(entry - 1) << 3), a, w = 0;
    unsigned i;

    cyc(182);
    wr16(0x7E2027 + obj, cyc_then(30, rd16(0x7E1F2D)));
    cyc(24);
    a = rd16(0x7E1AEE) & 0x0001;
    if (!a) {
        cyc(40);
        for (i = 0; i < 3; i++) {
            cyc(30);
            w = rd16(0x7EFF66 + 2 * i + t);
            cyc(30);
            wr16(0x7E200C + 2 * i + obj, w);
        }
        cyc(52);
        a = (uint16_t)((w & 0xFF00) | rd8(0x7EFF6C + t));
        cyc(30);
        wr8(0x7E2015 + obj, (uint8_t)a);
    } else { cyc(46); }
    cyc(46);
    a = (uint16_t)((a & 0xFF00) | rd8(0x7E1F2B));
    cyc(30);
    wr8(0x7E2049 + obj, (uint8_t)a);
    cyc(24);
    a = (uint16_t)((a & 0xFF00) | rd8(0x7E1F4C));
    cyc(30);
    wr8(0x7E204A + obj, (uint8_t)a);
    *x = t;
    *y = obj;
    cyc(100);
    return a;
}

void ObjectPlaceFromTable_cr(Carried *cr)
{
    cr->a = ObjectPlaceFromTable((uint8_t)cr->a, (uint8_t)cr->x, &cr->x, &cr->y);
}

void abi_ObjectPlaceFromTable(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPlaceFromTable_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:D397 ObjectLinkEventTrigger
 * WHAT IT DOES: Unless bit 7 of $0DA4 is already set: when the object two
 * links on from the one at $0346 has bit 14 set in its first word and
 * $1A24 is non-zero, clears $1A24, sets $0DA4 to $80 and starts the values
 * at $1A1C-$1A22 ($1A, 0, $FE, 0).
 * HOW: Links through +$4F; everything through the data bank.
 * Entry: any widths. Exit: M 8-bit, X 8-bit (X, Y high bytes clear); A and
 * N, Z as left by the test that ended it (see the code).
 */
uint16_t ObjectLinkEventTrigger(uint8_t bank, uint16_t a, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    cyc(46);
    uint8_t f = rd8(b | 0x0DA4) & 0x80;

    if (f) {
        *p = (uint8_t)((*p & ~P_Z) | P_N);
        return (uint16_t)((a & 0xFF00) | f);
    }
    cyc(78);
    *x = rd16(b | 0x0346);
    cyc(30);
    *y = rd16(b | (uint16_t)(0x204F + *x));
    cyc(30);
    *y = rd16(b + 0x204F + *y);
    cyc(44);
    a = rd16(b | *y) & 0x4000;
    if (!a) {
        cyc(24);                        /* AND #, which the other exit has not run */
        *p = (uint8_t)((*p & ~P_N) | P_Z);
        return 0;
    }
    if (!(a = rd16(cyc_then(86, b | 0x1A24)))) {
        *p = (uint8_t)((*p & ~P_N) | P_Z);
        return 0;
    }
    wr16(b | 0x1A24, 0x0000);
    wr8(b | 0x0DA4, 0x80);
    wr8(b | 0x1A1C, 0x1A);
    wr8(b | 0x1A1E, 0x00);
    wr8(b | 0x1A22, 0x00);
    wr8(b | 0x1A20, 0xFE);
    *p = (uint8_t)((*p & ~P_Z) | P_N);
    return 0x00FE;
}

void ObjectLinkEventTrigger_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ObjectLinkEventTrigger(cr->db, cr->a, &cr->x, &cr->y, &p);
    cr->x &= 0x00FF;
    cr->y &= 0x00FF;
    set_p(cr, (uint8_t)(p | P_M | P_X));
}

void abi_ObjectLinkEventTrigger(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLinkEventTrigger_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


void abi_ObjectBucketInsert(void);

/*
 * @routine 03:CB25 ObjectInit
 * WHAT IT DOES: Clears object Y's record from +$04 on (47 words), puts it in
 * bucket 0's list, and sets its +$36 to 0 and its +$59 to $69.
 * HOW: Data bank $7E inside; ObjectBucketInsert; X and Y are swapped while
 * it works and swapped back.
 * Entry: M 8-bit, X 16-bit; Y = object. Exit: A = $69 with the object's
 * high byte; X, Y unchanged; M 8-bit; N, Z from the caller's data bank.
 */
void ObjectInit(uint16_t obj)
{
    unsigned i;

    cyc(234);
    for (i = 0; i < 0x2F; i++)
        { cyc(94); wr16(0x7E2004 + 2 * i + obj, 0x0000); }
    cyc(184);
    wr16(0x7E2040 + obj, 0x0000);
    cyc(62);
    ObjectBucketInsert(obj);
    cyc(212);
    wr8(0x7E2036 + obj, 0x00);
    cyc(46);
    wr8(0x7E2059 + obj, 0x69);
    cyc(152);
}

void ObjectInit_cr(Carried *cr)
{
    ObjectInit(cr->y);
    cr->a = (uint16_t)((cr->y & 0xFF00) | 0x69);
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) | P_M));
}

void abi_ObjectInit(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectInit_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 08:B8B4 ObjectPlaceAll
 * WHAT IT DOES: Sets up every object in the list at $7E:FFA6 (half the
 * count at $7E:FFB8 of them) with ObjectPlaceFromTable: object n (from 1)
 * takes table entry $7E:FFA6[n], and its $1F2B, $1F2D and $1F4C words from
 * the lists at $18D3, $18DB and $18E3.
 * HOW: Direct page $02 counts down from the count halved (arithmetic shift),
 * $08 counts the entry up from 1; the three words are stored through the
 * data bank before each call.
 * Entry: any widths, D = $0000. Exit: A = the last ObjectPlaceFromTable A;
 * X, Y, P restored, then N, Z from X (pulled at its entry width).
 */
uint16_t ObjectPlaceAll(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    cyc(136);
    uint16_t n = rd16(0x7EFFB8), i = 0, a = 0, x, y;

    n = (uint16_t)((n >> 1) | (n & 0x8000));
    cyc(54);
    wr16(0x000002, n);
    cyc(40);
    wr16(0x000008, 0x0001);
    cyc(2);
    do {
        cyc(54);
        wr16(b | 0x1F2B, cyc_then(24, rd16(0x0018D3 + i)));
        cyc(32);
        wr16(b | 0x1F2D, cyc_then(24, rd16(0x0018DB + i)));
        cyc(32);
        wr16(b | 0x1F4C, cyc_then(24, rd16(0x0018E3 + i)));
        cyc(32);
        x = rd16(0x7EFFA6 + i);
        cyc(60);
        a = ObjectPlaceFromTable((uint8_t)rd16(0x000008), (uint8_t)x, &x, &y);
        i = (uint16_t)(i + 2);
        cyc(80);
        wr16(0x000008, cyc_then(14, (uint16_t)(rd16(0x000008) + 1)));
        cyc(16);
        n = (uint16_t)(rd16(0x000002) - 1);
        cyc(14);
        wr16(0x000002, n);
    } while (n);
    cyc(160);
    return a;
}

void ObjectPlaceAll_cr(Carried *cr)
{
    cr->a = ObjectPlaceAll(cr->db);
    set_p(cr, (cr->p & P_X) ? nz8(cr->p, (uint8_t)cr->x) : nz16(cr->p, cr->x));
}

void abi_ObjectPlaceAll(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPlaceAll_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E760 ObjectCountStep5F
 * WHAT IT DOES: Steps object X's counter +$49 up by one; while it has not
 * passed the limit at $1F58 its +$5F rises by $1F5A, after that +$5F falls
 * by $1F5A and the routine reports when the counter has reached $1F59.
 * HOW: 8-bit compares (BEQ/BMI on the difference); returns with carry set
 * only when the counter equals $1F59 on the falling side.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: C set when done; rising: A
 * low byte = the new +$5F, N, Z, V from the add; falling: A low byte = the
 * counter, N, Z from the compare with $1F59, V from the subtract.
 */
int ObjectCountStep5F(uint8_t bank, uint16_t obj, uint8_t *a, uint8_t *p)
{
    uint32_t c = field(bank, obj, 0x49), f = field(bank, obj, 0x5F), b = (uint32_t)bank << 16;
    uint8_t n, lim, v, d, r;

    cyc(30);
    n = rd8(c);
    cyc(6);
    wr8(c, (uint8_t)(n + 1));
    cyc(30);
    n = rd8(c);
    cyc(24);
    lim = rd8(b | 0x1F58);
    if (n == lim || (cyc(16), ((uint8_t)(n - lim) & 0x80))) {
        cyc(52);
        v = rd8(f);
        cyc(38);
        d = rd8(b | 0x1F5A);
        r = (uint8_t)(v + d);
        cyc(30);
        wr8(f, r);
        *p = (uint8_t)((nz8(*p, r) & ~(P_C | P_V)) | ((~(v ^ d) & (v ^ r) & 0x80) ? P_V : (cyc(78), 0)));
        *a = r;
        return 0;
    }
    cyc(78);
    v = rd8(f);
    cyc(38);
    d = rd8(b | 0x1F5A);
    r = (uint8_t)(v - d);
    cyc(30);
    wr8(f, r);
    *p = (uint8_t)((*p & ~P_V) | (((v ^ d) & (v ^ r) & 0x80) ? P_V : 0));
    cyc(30);
    n = rd8(c);
    *a = n;
    cyc(24);
    lim = rd8(b | 0x1F59);
    r = (uint8_t)(n - lim);
    *p = (uint8_t)(nz8(*p, r) & ~P_C);
    if (r)
        { cyc(76); return 0; }
    *p |= P_C;
    cyc(104);
    return 1;
}

void ObjectCountStep5F_cr(Carried *cr)
{
    uint8_t a = 0, p = cr->p;

    ObjectCountStep5F(cr->db, cr->x, &a, &p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    set_p(cr, p);
}

void abi_ObjectCountStep5F(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCountStep5F_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A 16-bit subtract as SBC with carry set; V from it. */
static uint16_t sub16v(uint16_t a, uint16_t b, uint8_t *p)
{
    uint16_t r = (uint16_t)(a - b);

    *p = (uint8_t)((*p & ~P_V) | (((a ^ b) & (a ^ r) & 0x8000) ? P_V : 0));
    return r;
}

/* The magnitude as EOR #$FFFF / INC does it ($8000 stays $8000). */
static uint16_t magnitude(uint16_t v)
{
    return (v & 0x8000) ? (cyc(32), (uint16_t)(-v)) : v;
}

/*
 * @routine 08:B5FA ObjectNearFocus
 * WHAT IT DOES: Switches object X between near and far by its distance
 * from the object at $1A64 (|dX| + |dZ| against $1194). Near: clears bit 3
 * of +$24, zeroes +$5F and sets +$5D by its kind (+$49) with
 * ObjectSet5DByKind. Far: sets bit 3 of +$24, zeroes +$3A, sets +$5F to
 * $32, +$5D to $99A3 + 2 * kind, and puts a direction ((the $1A64 object's
 * +$11 + its own +$15) / 8, low two bits clear, bit 7 set) above the low
 * two bits of +$18.
 * HOW: 16-bit differences made positive by EOR/INC, summed, compared; the
 * rest in 8-bit through the data bank. $1F2B keeps |dX|.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: Y = $1A64; M
 * 8-bit; near: as ObjectSet5DByKind (V from the distance add); far: A = the
 * +$5D value, N, Z, C, V from its add.
 */
uint16_t ObjectNearFocus(uint8_t bank, uint16_t obj, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t dx, dz, sum;
    unsigned s;
    uint8_t k;

    cyc(76);
    dx = magnitude(sub16v(rd16(b | 0x1A6C), rd16(cyc_then(44, field(bank, obj, 0x0C))), p));
    cyc(46);
    wr16(b | 0x1F2B, dx);
    cyc(30);
    dz = magnitude(sub16v(rd16(b | 0x1A70), rd16(cyc_then(44, field(bank, obj, 0x10))), p));
    s = (unsigned)dz + dx;
    sum = (uint16_t)s;
    *p = (uint8_t)((*p & ~P_V) | ((~(dz ^ dx) & (dz ^ sum) & 0x8000) ? P_V : 0));
    if ((uint16_t)(sum - 0x1194) & 0x8000) {
        wr8(field(bank, obj, 0x24), cyc_then(46, rd8(cyc_then(168, field(bank, obj, 0x24))) & 0xF7));
        wr8(field(bank, obj, 0x5F), cyc_then(46, 0x00));
        return cyc_then(64, ObjectSet5DByKind(bank, obj, rd8(cyc_then(30, field(bank, obj, 0x49))), p));
    }
    k = (uint8_t)(rd8(cyc_then(174, field(bank, obj, 0x18))) & 0x03);
    wr8(field(bank, obj, 0x18), cyc_then(46, k));
    cyc(54);
    k = (uint8_t)(((uint8_t)(rd8(b | 0x1A75) + rd8(cyc_then(44, field(bank, obj, 0x15)))) >> 3 & 0xFC) | 0x80 | k);
    wr8(field(bank, obj, 0x18), cyc_then(142, k));
    wr8(field(bank, obj, 0x24), cyc_then(46, rd8(cyc_then(30, field(bank, obj, 0x24))) | 0x08));
    wr8(field(bank, obj, 0x3A), cyc_then(46, 0x00));
    wr8(field(bank, obj, 0x5F), cyc_then(46, 0x32));
    dx = (uint16_t)(rd8(cyc_then(30, field(bank, obj, 0x49))) << 1);
    s = (unsigned)dx + 0x99A3;
    sum = (uint16_t)s;
    *p = (uint8_t)((nz16(*p, sum) & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0)
                   | ((~(dx ^ 0x99A3) & (dx ^ sum) & 0x8000) ? P_V : 0));
    wr16(field(bank, obj, 0x5D), cyc_then(128, sum));
    return sum;
}

void ObjectNearFocus_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ObjectNearFocus(cr->db, cr->x, &p);
    cr->y = 0x1A64;
    set_p(cr, (uint8_t)(p | P_M));
    cyc(128);
}

void abi_ObjectNearFocus(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectNearFocus_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


uint16_t ScaleObjectXZ(uint16_t obj, uint8_t *p);

/*
 * @routine 08:83CC ObjectRebucket
 * WHAT IT DOES: Recomputes object X's bucket from its X and Z position
 * (ScaleObjectXZ) and, when it changed, moves the object from its old
 * bucket's list to the new one's.
 * HOW: Compared with +$40 (bank $7E); ObjectBucketRemove, the new +$40,
 * ObjectBucketInsert. P is saved and restored around it.
 * Entry: any M, X 16-bit, D = $0000; X = object. Exit: A = the bucket when
 * unchanged, else the object; X, Y, P unchanged; $02 and $94 as
 * ScaleDp02Dp94 leaves them.
 */
uint16_t ObjectRebucket(uint16_t obj)
{
    cyc(90);
    uint8_t p = 0;
    uint16_t key = ScaleObjectXZ(obj, &p);

    cyc(120);
    if (key == rd16(0x7E2040 + obj)) {
        cyc(94);
        return key;
    }
    cyc(108);
    ObjectBucketRemove(obj);
    cyc(68);
    wr16(0x7E2040 + obj, key);
    cyc(62);
    ObjectBucketInsert(obj);
    cyc(216);
    return obj;
}

void ObjectRebucket_cr(Carried *cr)
{
    cr->a = ObjectRebucket(cr->x);
}

void abi_ObjectRebucket(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRebucket_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void CrashWhite(uint8_t bank);

/*
 * @routine 08:B5B3 ObjectRecordFlag70Step
 * WHAT IT DOES: While $0D53 is clear and object X's countdown +$48 is
 * running, counts it down; otherwise sets bit 2 of the word at +$70 of the
 * object's Super FX record when $0D53 is set, clears it when not.
 * HOW: 8-bit tests through the data bank, 16-bit read-modify-write of the
 * record word. An object with no record branches to $08:B5F6, a JSL to the
 * stop screen CrashWhite ($00:8014) that would fall into ObjectNearFocus;
 * no script reaches that path.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X, Y unchanged.
 * Counting: A low byte = the count before the step, N, Z from the new
 * count. Otherwise: A = the new record word, N, Z from X.
 */
uint16_t ObjectRecordFlag70Step(uint8_t bank, uint16_t obj, uint16_t a, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    cyc(46);
    uint8_t flag = rd8(b | 0x0D53), n;
    uint16_t rec, w;

    if (!flag) {
        n = rd8(cyc_then(46, field(bank, obj, 0x48)));
        if (n) {
            wr8(field(bank, obj, 0x48), cyc_then(66, (uint8_t)(n - 1)));
            *p = nz8(*p, (uint8_t)(n - 1));
            cyc(68);                    /* JMP to the RTL, RTL */
            return (uint16_t)((a & 0xFF00) | n);
        } else { cyc(26); }
    }
    rec = rd16(cyc_then(104, field(bank, obj, 0x22)));
    if (!rec)
        { cyc(84); CrashWhite(bank); }
    cyc(flag ? 142 : 148);              /* BEQ taken without the flag */
    w = rd16(0x700070 + rec);
    w = flag ? (uint16_t)(w | 0x0004) : (uint16_t)(w & 0xFFFB);
    cyc(56);
    wr16(0x700070 + rec, w);
    if (flag)
        cyc(22);                        /* BRA */
    *p = nz16(*p, obj);
    cyc(102);                           /* PLX, SEP, RTL */
    return w;
}

void ObjectRecordFlag70Step_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ObjectRecordFlag70Step(cr->db, cr->x, cr->a, &p);
    set_p(cr, (uint8_t)(p | P_M));
}

void abi_ObjectRecordFlag70Step(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRecordFlag70Step_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

uint16_t GsuRecordWords88(void);

/* CMP of two 8-bit values: N, Z, C. */
static uint8_t cmp8(uint8_t p, uint8_t a, uint8_t m)
{
    return (uint8_t)((nz8(p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? (cyc(7), P_C) : (cyc(7), 0)));
}

/* Points the linked object (+$4F) at the handler $08:addr. k is the record
   kind ($1F58) the handler was picked by (read again, then compared with 1
   and 4), or -1 when picked by no kind. */
static uint16_t set_linked_handler(uint8_t bank, uint16_t obj, uint16_t addr, int k)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t y = rd16(cyc_then(52, field(bank, obj, 0x4F)));

    if (k >= 0) {
        cyc(24);
        rd8(b | 0x1F58);
        cyc(k == 1 ? 38 : k == 4 ? 70 : 64);           /* CMP, BEQ (taken) */
    }
    cyc(76);
    wr16(b | (uint16_t)(0x201A + y), addr);
    cyc(68);
    wr8(b | (uint16_t)(0x201C + y), 0x08);
    return y;
}

/*
 * @routine 08:B4C6 ObjectLinkedHandlerByState
 * WHAT IT DOES: Once per object (bit 3 of +$25 marks it done), gives
 * object X's linked object (+$4F) a new handler in bank 08 by the state
 * byte at $0DD6 + Y: 8 sets bits 3 of the record's +$64 and 5 of its +$70
 * and picks $91EB, $93D7 or $92DF by the record's +$88 (1, 4, other);
 * $0A or $0C clears bits 4, 5 and 10 of the record's +$42 and, when $0E59
 * is set, picks $94A1; other states pick $951B when $19ED is 8. Each pick
 * also runs GsuRecordWords88 unless bit 0 of $1AEE is set, and marks the
 * object done.
 * HOW: 8-bit compares; the handler is the linked object's +$1A (address)
 * and +$1C (bank). Only the $0A/$0C path is reached by a script.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the state index. Exit: M
 * 8-bit; X unchanged; Y = the linked object when a handler was picked; A,
 * N, Z, C as the path leaves them; V unchanged.
 */
uint16_t ObjectLinkedHandlerByState(uint8_t bank, uint16_t obj, uint16_t a, uint16_t *y, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t f = rd8(cyc_then(52, field(bank, obj, 0x25))), v, k;
    uint16_t rec, w;

    if (f & 0x08) {
        cyc(64);
        *p = nz8(*p, 0x08);
        return (uint16_t)((a & 0xFF00) | 0x08);
    }
    cyc(68);
    v = rd8(b | (uint16_t)(0x0DD6 + *y));
    *p = cmp8(*p, v, 0x08);
    if (v == 0x08) {
        rec = rd16(cyc_then(113, field(bank, obj, 0x22)));
        cyc(46);
        wr16(0x700064 + rec, cyc_then(56, (uint16_t)(rd16(0x700064 + rec) | 0x0008)));
        cyc(32);
        wr16(0x700070 + rec, cyc_then(56, (uint16_t)(rd16(0x700070 + rec) | 0x0020)));
        cyc(32);
        k = (uint8_t)rd16(0x700088 + rec);
        cyc(46);
        wr8(b | 0x1F58, k);
        *p = cmp8(*p, k, 0x01);
        if (k != 0x01)
            *p = cmp8(*p, k, 0x04); else { cyc(7); }
        *y = set_linked_handler(bank, obj, k == 0x01 ? 0x91EB : k == 0x04 ? 0x93D7 : 0x92DF, k);
    } else {
        *p = cmp8(*p, v, 0x0C);
        if (v != 0x0C)
            { cyc(25); *p = cmp8(*p, v, 0x0A); }
        if (v == 0x0C || v == 0x0A) {
            rec = rd16(cyc_then(138, field(bank, obj, 0x22)));
            cyc(46);
            w = (uint16_t)(rd16(0x700042 + rec) & 0xFBCF);
            cyc(56);
            wr16(0x700042 + rec, w);
            cyc(82);
            v = rd8(b | 0x0E59);
            if (!v) {
                cyc(46);
                *p = nz8(*p, 0);
                return (uint16_t)(w & 0xFF00);
            }
            *y = set_linked_handler(bank, obj, 0x94A1, -1);
        } else {
            cyc(74);
            v = rd8(b | 0x19ED);
            cyc(9);
            *p = cmp8(*p, v, 0x08);
            if (v != 0x08)
                { cyc(40); return (uint16_t)((a & 0xFF00) | v); }
            *y = set_linked_handler(bank, obj, 0x951B, -1);
            cyc(2);                     /* JMP, not BRA */
        }
    }
    a = 0x0000;
    cyc(68);
    if (!(rd16(b | 0x1AEE) & 0x0001))
        { cyc(124); a = GsuRecordWords88(); } else { cyc(68); }
    f = (uint8_t)(rd8(cyc_then(30, field(bank, obj, 0x25))) | 0x08);
    wr8(field(bank, obj, 0x25), cyc_then(46, f));
    *p = nz8(*p, f);
    return (uint16_t)((a & 0xFF00) | f);
}

void ObjectLinkedHandlerByState_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ObjectLinkedHandlerByState(cr->db, cr->x, cr->a, &cr->y, &p);
    set_p(cr, (uint8_t)(p | P_M));
    cyc(44);
}

void abi_ObjectLinkedHandlerByState(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLinkedHandlerByState_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

#define ATTACH_VALUES 0x098090u

/*
 * @routine 08:8525 ObjectAttachLinked
 * WHAT IT DOES: Links object X to object Y: X's +$4F = Y, Y's +$06 = X, and
 * the block Y's +$4F points at gets X at +$02, kind 5 at +$00, its +$32
 * from the table at $09:8090 and its other fields (+$26-+$30, +$34-+$3A)
 * cleared. The table entry is the word at $70:2396, plus 2 when Y's +$1A
 * is $8E2A, else plus 4, 8 or 6 for $0D2B = 1, 2 or 3.
 * HOW: The block is addressed through Y as a full address in the data
 * bank. Only the $0D2B choices are reached by a script.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the other object. Exit: A = 0
 * (M 8-bit), Z set; X unchanged; Y = the block; $1F4C = Y's +$1A, $1F4E =
 * the block; C (and V after an add) from the last compare or add.
 */
static uint16_t object_attach_linked(uint8_t bank, uint16_t obj, uint16_t other, uint8_t *p, uint16_t *y)
{
    static const uint8_t clear[9] = {0x35, 0x37, 0x39, 0x26, 0x28, 0x2A, 0x2C, 0x2E, 0x30};
    uint32_t b = (uint32_t)bank << 16;
    uint16_t blk, kind, t, add = 0, m;
    unsigned i, s;

    wr16(field(bank, obj, 0x4F), cyc_then(66, other));
    blk = rd16(cyc_then(30, field(bank, other, 0x4F)));
    cyc(24);
    wr16(b | 0x1F4E, blk);
    kind = rd16(cyc_then(30, field(bank, other, 0x1A)));
    cyc(24);
    wr16(b | 0x1F4C, kind);
    wr16(field(bank, other, 0x06), cyc_then(44, obj));
    cyc(24);
    blk = rd16(b | 0x1F4E);
    *y = blk;
    cyc(44);
    wr16(b | (uint16_t)(blk + 0x02), obj);
    cyc(62);
    t = rd16(0x702396);
    cyc(38);
    kind = rd16(b | 0x1F4C);
    *p = (uint8_t)((*p & ~P_C) | (kind >= 0x8E2A ? P_C : 0));
    if (kind == 0x8E2A) {
        add = 2;
    } else {
        cyc(64);
        m = rd16(b | 0x0D2B) & 0x00FF;
        *p = (uint8_t)((*p & ~P_C) | (m >= 1 ? (cyc(32), P_C) : 0));
        if (m == 1)
            add = 4;
        else {
            cyc(40);
            *p = (uint8_t)((*p & ~P_C) | (m >= 2 ? P_C : 0));
            if (m == 2)
                add = 8;
            else {
                *p = (uint8_t)((*p & ~P_C) | (m >= 3 ? (cyc(40), P_C) : 0));
                if (m == 3)
                    add = 6;
            }
        }
        cyc(14);
    }
    if (add) {
        s = (unsigned)t + add;
        *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0)
                       | ((~(t ^ add) & (t ^ (uint16_t)s) & 0x8000) ? P_V : 0));
        t = (uint16_t)s;
    }
    cyc(190);
    wr16(b | (uint16_t)(blk + 0x32), rd16(ATTACH_VALUES + t));
    cyc(60);
    for (i = 0; i < 9; i++)
        { cyc(30); wr16(b | (uint16_t)(blk + clear[i]), 0x0000); }
    cyc(54);
    wr16(b | blk, 0x0005);
    cyc(68);
    wr8(b | (uint16_t)(blk + 0x34), 0x00);
    *p = nz8(*p, 0);
    cyc(44);
    return 0x0000;
}

uint16_t ObjectAttachLinked(uint8_t bank, uint16_t obj, uint16_t other, uint8_t *p)
{
    uint16_t y;

    return object_attach_linked(bank, obj, other, p, &y);
}

void ObjectAttachLinked_cr(Carried *cr)
{
    uint8_t p = cr->p;
    uint16_t y;

    cr->a = object_attach_linked(cr->db, cr->x, cr->y, &p, &y);
    cr->y = y;
    set_p(cr, (uint8_t)(p | P_M));
}

void abi_ObjectAttachLinked(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectAttachLinked_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:F0E1 ObjectGroupsActivate
 * WHAT IT DOES: For each entry in the list at $1AF5 (4 bytes each, ended by
 * a zero first word), takes the object bucket its second word selects and
 * sets the state byte (+$36) of every object in it to 3, skipping objects
 * whose state is negative, and, when the entry's code is $23, objects with
 * bit 4 of +$24 set.
 * HOW: The bucket number (second word AND $07FE) indexes the bucket heads
 * at $7F:0000; objects chain through +$3C (next in bucket). $1F2B keeps the list position and
 * $1F2D the entry's code; X is reloaded from and stored back to direct page
 * $3E around each chain. Data bank $7E inside.
 * Entry: X 16-bit, D = $0000. Exit: A = 0; X = the list position of the
 * terminating entry; Y = 0 when any chain was walked, else unchanged (low
 * bytes only when P restores 8-bit index registers); P and the data bank
 * unchanged.
 */
/* ObjectGroupsActivate's chain walk from object *y: each object's state
   +$36 set to 3 unless negative (or, for code $23, +$24 bit 4 set), up to
   the end of the chain (+$3C). */
static void groups_chain(uint16_t *y, uint32_t c)
{
                                        /* in: SEP, LDA (46); round again: TAY, BNE, SEP, LDA (82) */

    do {
        cyc(c);
        c = 82;
        if (!(rd8(0x7E1F2D) == 0x23 && (rd8(cyc_then(62, 0x7E2024 + *y)) & 0x10))) {
            if (!(rd8(cyc_then(68, 0x7E2036 + *y)) & 0x80))
                { cyc(62); wr8(0x7E2036 + *y, 0x03); }
            else
                cyc(22);                /* BMI */
        } else
            cyc(64);                    /* AND, BEQ, JML */
        cyc(52);
        *y = rd16(0x7E203C + *y);
    } while (*y);
}

/* ObjectGroupsActivate's list walk from the position in $1F2B (7E:F0EE):
   returns the position of the ending entry. */
static uint16_t groups_list(uint16_t *y)
{
    uint16_t pos, code, head, keep;
    uint32_t c = 24;                    /* in; round again 46 */

    for (;;) {
        cyc(c);
        c = 46;
        pos = rd16(0x7E1F2B);
        cyc(30);
        code = rd16(0x7E1AF5 + pos);
        if (!code)
            { cyc(66); return pos; }
        cyc(46);
        wr16(0x7E1F2D, code);
        cyc(30);
        head = rd16(0x7E1AF7 + pos);
        cyc(80);
        wr16(0x7E1F2B, (uint16_t)(pos + 4));
        cyc(70);
        head = rd16(0x7F0000 + (head & 0x07FE));
        if (!head)
            continue;
        *y = head;
        cyc(46);
        keep = rd16(0x00003E);
        groups_chain(y, 46);
        cyc(46);
        wr16(0x00003E, keep);
        cyc(2);
    }
}

uint16_t ObjectGroupsActivate(uint16_t *y)
{
    cyc(178);
    wr16(0x7E1F2B, 0x0000);
    return groups_list(y);
}

void ObjectGroupsActivate_cr(Carried *cr)
{
    cr->x = ObjectGroupsActivate(&cr->y);
    cr->a = 0;
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    } else { cyc(78); }
}

void abi_ObjectGroupsActivate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectGroupsActivate_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:F141 ObjectGroupsActivateClear08
 * WHAT IT DOES: An arm after ObjectGroupsActivate's RTS that nothing
 * reaches: clears bit 1 of object Y's +$08, then goes on with
 * ObjectGroupsActivate's chain walk from the next object (+$3C) and its
 * list walk from $1F2B, $3E taking X. A direct call is its only way in.
 * HOW: SEP #$20 for the AND #$FD, REP #$20, BRA into the chain loop
 * (7E:F131); ends with ObjectGroupsActivate's PLB, PLP, RTS.
 * Entry: X 16-bit, D = $0000, data bank $7E; Y = object; the caller's P
 * and data bank pushed above the return (PHP, PHB). Exit: as
 * ObjectGroupsActivate, P and the data bank those pushed.
 */
void ObjectGroupsActivateClear08(Carried *cr)
{
    cyc(52);
    uint8_t v = (uint8_t)(rd8(0x7E2008 + cr->y) & 0xFD), db, p;

    cyc(46);
    wr8(0x7E2008 + cr->y, v);
    cyc(96);
    cr->y = rd16(0x7E203C + cr->y);
    if (cr->y)
        groups_chain(&cr->y, 82);
    cyc(46);
    wr16(0x00003E, cr->x);
    cyc(24);                            /* JMP */
    cr->x = groups_list(&cr->y);
    cr->a = 0;
    db = rd8((uint16_t)(cpu.s + 1));
    cyc(20);
    p = rd8((uint16_t)(cpu.s + 2));
    cr->db = db;
    r_plp(cr, p);
}

void abi_ObjectGroupsActivateClear08(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectGroupsActivateClear08(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

typedef struct {
    uint16_t a, x, y;
    uint8_t p;
} AreaEnd;

/* The body of BucketTriggersFromArea, with everything its exit leaves. */
static AreaEnd bucket_triggers_from_area(uint8_t bank, uint16_t rec, uint8_t p)
{
    uint32_t c;
    uint32_t b = (uint32_t)bank << 16;
    uint16_t x = 0, y = rec, v, w, n;
    AreaEnd e;

    cyc(68);
    v = (uint16_t)((rd16(b | (uint16_t)(rec + 3)) & 0x00FF) << 8);
    v = (uint16_t)((v >> 1) | (v & 0x8000));
    v = (uint16_t)((v >> 1) | (v & 0x8000));
    cyc(136);
    wr16(0x000094, v);
    cyc(30);
    v = rd16(b | (uint16_t)(rec + 1)) & 0x00FF;
    cyc(48);
    wr16(b | 0x05F2, v);
    cyc(30);
    v = rd16(b | (uint16_t)(rec + 2)) & 0x00FF;
    cyc(92);
    wr16(0x0000A5, (uint16_t)(-v << 1));
    cyc(30);
    v = rd16(b | (uint16_t)(rec + 0)) & 0x00FF;
    cyc(48);
    wr16(b | 0x1F2B, v);
    do {
        cyc(16);
        v = rd16(0x0000A5);
        cyc(16);
        wr16(0x000002, v);
        cyc(24);
        v = rd16(b | 0x1F2B);
        cyc(16);
        wr16(0x000004, v);
        c = 16;                         /* in: LDA; round again: the branch back and LDA (38) */
        do {
            cyc(c);
            c = 38;
            v = rd16(0x000002);
            cyc(38);
            w = rd16(b | 0x1F03);
            if (!((uint16_t)(v + w) & 0xFFC0)) {
                cyc(56);
                v = rd16(0x000094);
                cyc(38);
                w = rd16(b | 0x1F05);
                if (!((uint16_t)(v + w) & 0xF800)) {
                    cyc(70);
                    v = rd16(b | (uint16_t)(y + 4)) & 0x00FF;
                    if (v != 0x002E) {
                        cyc(94);
                        wr16(b | (uint16_t)(0x1AF5 + x), v);
                        cyc(24);
                        v = rd16(b | 0x1F01);
                        cyc(30);
                        w = rd16(0x000002);
                        v = (uint16_t)(v + w);
                        cyc(30);
                        w = rd16(0x000094);
                        v = (uint16_t)(v + w);
                        cyc(30);
                        wr16(b | (uint16_t)(0x1AF7 + x), v);
                        x = (uint16_t)(x + 4);
                    } else { cyc(4); }
                    cyc(20);
                }
            }
            y = (uint16_t)(y + 1);
            cyc(76);
            v = rd16(0x000002);
            cyc(14);
            wr16(0x000002, (uint16_t)(v + 1));
            cyc(16);
            v = rd16(0x000002);
            cyc(14);
            wr16(0x000002, (uint16_t)(v + 1));
            cyc(16);
            n = rd16(0x000004);
            n = (uint16_t)(n - 1);
            cyc(14);
            wr16(0x000004, n);
        } while (n);
        cyc(32);
        v = rd16(0x000094);
        e.a = (uint16_t)(v - 0x0040);
        p = (uint8_t)(p & ~(P_C | P_V));
        if (v >= 0x0040)
            p |= P_C;
        if ((v ^ 0x0040) & (v ^ e.a) & 0x8000)
            p |= P_V;
        cyc(54);
        wr16(0x000094, e.a);
        cyc(24);
        n = rd16(b | 0x05F2);
        n = (uint16_t)(n - 1);
        cyc(14);
        wr16(b | 0x05F2, n);
    } while (cyc_loop(22, n));
    cyc(46);
    wr16(b | (uint16_t)(0x1AF5 + x), 0x0000);
    cyc(30);
    wr16(b | (uint16_t)(0x1AF7 + x), 0x0000);
    e.x = x;
    e.y = y;
    e.p = (uint8_t)((p & ~P_N) | P_Z);
    cyc(42);
    return e;
}

/*
 * @routine 7E:C58D BucketTriggersFromArea
 * WHAT IT DOES: Builds the trigger list at $1AF5 (read by
 * ObjectGroupsActivate) from a rectangle of map cells: for each cell inside
 * the window and not $2E, an entry of the cell's code and the bucket
 * number $1F01 + the cell's column * 2 + its row * $40; the list ends with
 * a zero entry.
 * HOW: The record at A (data bank) holds the columns, rows, first column
 * and first row as bytes, then the cells row by row. Direct page $A5 is
 * -first column * 2 and $94 the first row * $40 (signed, counting down a
 * row at a time); $02 steps the column by 2, $04 and $05F2 count the
 * columns and rows (16-bit; $1F2B keeps the column count). A cell is in
 * the window when $02 + $1F03 is below $40 and $94 + $1F05 below $800.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = record address. Exit: A = the
 * final $94; X = the list size in bytes; Y = A + the cell count; N clear, Z
 * set; C, V from the last row step.
 */
uint16_t BucketTriggersFromArea(uint8_t bank, uint16_t rec)
{
    return bucket_triggers_from_area(bank, rec, 0).x;
}

void BucketTriggersFromArea_cr(Carried *cr)
{
    AreaEnd e = bucket_triggers_from_area(cr->db, cr->a, cr->p);

    cr->a = e.a;
    cr->x = e.x;
    cr->y = e.y;
    set_p(cr, e.p);
}

void abi_BucketTriggersFromArea(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BucketTriggersFromArea_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The window and bucket origin: three words from src to $1F01-$1F06. */
static void triggers_window(uint16_t src)
{
    unsigned i;
    uint16_t v;

    cyc(132);
    for (i = 0; i < 6; i += 2) {
        cyc(24);
        v = rd16(0x7E0000 + src + i);
        cyc(24);
        wr16(0x7E1F01 + i, v);
    }
}

/* The record of BucketTriggersFromView: from the view's Y high byte. */
static uint16_t view_area(uint16_t *x)
{
    cyc(24);
    *x = (uint16_t)((((rd16(0x7E0667) & 0x00FF) >> 3) + 1) & 0xFE);
    cyc(148);
    return rd16(0x7EC61D + *x);
}

/* The record of BucketTriggersFixedArea: by $0D62 and bit 7 of $1AEC. */
static uint16_t fixed_area(void)
{
    cyc(24);
    if (rd16(0x7E0D62) & 0x00FF)
        { cyc(22); return 0xC661; }
    cyc(64);
    return (rd16(0x7E1AEC) & 0x0080) ? (cyc(16), 0xC661) : 0xC696;
}

/* For the abi_ wrappers, after the window is copied: data bank $7E, then
   BucketTriggersFromArea on the record. P on the call: M 16-bit, X 16-bit,
   N, Z from the record address; afterwards M 8-bit and the data bank
   back, N, Z from it. */
static void abi_triggers_from(Carried *cr, uint16_t rec, uint16_t x)
{
    cyc(46);
    uint8_t db = cr->db;

    cr->db = 0x7E;
    cr->a = rec;
    cr->x = x;
    set_p(cr, nz16((uint8_t)(cr->p & ~(P_M | P_X)), rec));
    BucketTriggersFromArea_cr(cr);
    cr->db = db;
    set_p(cr, (uint8_t)(nz8(cr->p, db) | P_M));
    cyc(92);
}

/*
 * @routine 7E:C520 BucketTriggersFromView
 * WHAT IT DOES: Builds the trigger list (BucketTriggersFromArea) from the
 * area record that the high byte of the view's Y position ($0667) selects,
 * with the window and bucket origin from $1EF5-$1EFA.
 * HOW: Copies $1EF5, $1EF7, $1EF9 to $1F01, $1F03, $1F05; the record
 * address is the word at $7E:C61D + ((($0667 >> 3) + 1) AND $FE). Data
 * bank $7E inside.
 * Entry: D = $0000. Exit: as BucketTriggersFromArea, with M 8-bit and N, Z
 * from the data bank pulled back.
 */
void BucketTriggersFromView(void)
{
    uint16_t x;

    triggers_window(0x1EF5);
    BucketTriggersFromArea(0x7E, view_area(&x));
}

void BucketTriggersFromView_cr(Carried *cr)
{
    uint16_t x, rec;

    triggers_window(0x1EF5);
    rec = view_area(&x);
    abi_triggers_from(cr, rec, x);
}

void abi_BucketTriggersFromView(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BucketTriggersFromView_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:C553 BucketTriggersFixedArea
 * WHAT IT DOES: Builds the trigger list (BucketTriggersFromArea) from the
 * area record at $7E:C661 when $0D62 is set or bit 7 of $1AEC is, else from
 * the one at $7E:C696, with the window and bucket origin from $1EFB-$1F00.
 * HOW: Copies $1EFB, $1EFD, $1EFF to $1F01, $1F03, $1F05. Data bank $7E
 * inside.
 * Entry: X 16-bit, D = $0000. Exit: as BucketTriggersFromArea, with M 8-bit
 * and N, Z from the data bank pulled back.
 */
void BucketTriggersFixedArea(void)
{
    triggers_window(0x1EFB);
    BucketTriggersFromArea(0x7E, fixed_area());
}

void BucketTriggersFixedArea_cr(Carried *cr)
{
    uint16_t rec;

    triggers_window(0x1EFB);
    rec = fixed_area();
    cyc(70);
    abi_triggers_from(cr, rec, cr->x);
}

void abi_BucketTriggersFixedArea(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BucketTriggersFixedArea_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

#define OBJ_USED_HEAD 0x7E0329u
#define OBJ_USED_LAST 0x7E032Bu
#define OBJ_FREE_HEAD 0x7E032Du

/*
 * @routine 03:CB5C ObjectAlloc
 * WHAT IT DOES: Takes an object record from the free list ($032D) and links
 * it into the used list (next at +$00, previous at +$02) after the last one
 * taken ($032B), or at the head ($0329) when there is none; it becomes the
 * last one taken. Carry set when it got one, clear when the free list was
 * empty.
 * HOW: Data bank $7E inside, 16-bit; P saved and restored, then the carry
 * set or cleared. With an empty free list it ends at $03:CBAA. The empty
 * case and a following record (a non-zero next) are not reached by a
 * script.
 * Entry: any widths. Exit: P as on entry but C; got one: X = the record,
 * Y = its next, A = the previous last (or the old head when there was none,
 * or X when there is a next); none: X = Y = the last taken, A's low byte
 * = $7E.
 */
int ObjectAlloc(uint16_t *a, uint16_t *x, uint16_t *y)
{
    cyc(178);
    uint16_t last = rd16(OBJ_USED_LAST), rec = rd16(cyc_then(38, OBJ_FREE_HEAD)), next;

    if (!rec) {
        cyc(168);
        *x = *y = last;
        *a = (uint16_t)((*a & 0xFF00) | 0x7E);
        return 0;
    }
    cyc(52);
    wr16(OBJ_FREE_HEAD, cyc_then(24, rd16(0x7E2000 + rec)));
    if (!last) {
        cyc(54);
        *a = rd16(OBJ_USED_HEAD);
        cyc(30);
        wr16(0x7E2000 + rec, *a);
        cyc(30);
        wr16(0x7E2002 + rec, 0x0000);
        cyc(24);
        wr16(OBJ_USED_HEAD, rec);
        cyc(22);                        /* BRA */
    } else {
        cyc(66);
        wr16(0x7E2000 + rec, cyc_then(30, rd16(0x7E2000 + last)));
        cyc(44);
        wr16(0x7E2000 + last, rec);
        cyc(44);
        wr16(0x7E2002 + rec, last);
        *a = last;
    }
    cyc(24);
    wr16(OBJ_USED_LAST, rec);
    cyc(30);
    next = rd16(0x7E2000 + rec);
    if (next) {
        *a = rec;
        cyc(60);
        wr16(0x7E2002 + next, rec);
    } else { cyc(22); }
    cyc(110);
    *x = rec;
    *y = next;
    return 1;
}

void ObjectAlloc_cr(Carried *cr)
{
    int got = ObjectAlloc(&cr->a, &cr->x, &cr->y);

    set_p(cr, (uint8_t)((cr->p & ~P_C) | (got ? (cyc(26), P_C) : (cyc(130), 0))));
}

void abi_ObjectAlloc(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectAlloc_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 03:CCBB ObjectSpawn
 * WHAT IT DOES: Unless the count at $0CEE has reached $2E, takes a new
 * object (ObjectAlloc), clears it (ObjectInit), and sets its +$36 to 3 and
 * its +$04 to $1F2B. Carry set and Y = the object when it got one; carry
 * clear and Y = 0 otherwise.
 * HOW: X is kept in direct page $3E across the calls; the limit is a signed
 * 8-bit compare (BPL).
 * Entry: M 8-bit, X 16-bit, D = $0000. Exit: X unchanged; M 8-bit; got one:
 * A = $1F2B, N, Z from it; none: Y = 0, N, Z from X.
 */
int ObjectSpawn(uint8_t bank, uint16_t *a, uint16_t x, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t n;
    uint16_t ax, rx = 0, ry = 0;
    int got;

    cyc(16);
    wr16(0x00003E, x);                          /* STX $3E, then LDA */
    cyc(24);
    n = rd8(b | 0x0CEE);
    *a = (uint16_t)((*a & 0xFF00) | n);
    ax = *a;
    got = ((uint8_t)(n - 0x2E) & 0x80) && (cyc(94), ObjectAlloc(&ax, &rx, &ry));
    cyc(78);
    (void)rd16(0x00003E);                       /* LDX $3E on both paths */
    if (got) {
        cyc(84);
        *y = rx;
        ObjectInit(rx);
        cyc(46);
        wr8(b | (uint16_t)(0x2036 + rx), 0x03);
        cyc(46);
        *a = rd16(b | 0x1F2B);
        cyc(30);
        wr16(b | (uint16_t)(0x2004 + rx), *a);
        return 1;
    }
    if (((uint8_t)(n - 0x2E) & 0x80))
        *a = ax;
    *y = 0;
    return 0;
}

void ObjectSpawn_cr(Carried *cr)
{
    if (ObjectSpawn(cr->db, &cr->a, cr->x, &cr->y))
        { cyc(22); set_p(cr, (uint8_t)(nz16(cr->p, cr->a) | P_C | P_M)); }
    else
        set_p(cr, (uint8_t)((nz16(cr->p, cr->x) & ~P_C) | P_M));
    cyc(58);
}

void abi_ObjectSpawn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpawn_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 03:B8A5 ObjectFree
 * WHAT IT DOES: Gives object X back: takes it out of its bucket
 * (ObjectBucketRemove) and out of the used list (fixing the head $0329 or
 * its previous record's next, and its next record's previous or the last
 * taken $032B), then puts it at the head of the free list ($032D).
 * HOW: 16-bit, through the caller's data bank; X, Y and P are saved and
 * restored. Freeing the head of the used list is not reached by a script
 * (it also clears the last taken, and writes +$02 of the new head even when
 * there is none).
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A = the old free head (M
 * 8-bit); X, Y, P unchanged except N, Z, which the final pull of X sets
 * from X.
 */
uint16_t ObjectFree(uint8_t bank, uint16_t obj)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t prev, next, head;

    cyc(144);
    ObjectBucketRemove(obj);
    prev = rd16(cyc_then(52, field(bank, obj, 0x02)));
    if (!prev) {
        next = rd16(cyc_then(46, field(bank, obj, 0x00)));
        cyc(24);
        wr16(b | 0x0329, next);
        cyc(54);
        wr16(b | (uint16_t)(0x2002 + next), 0x0000);
        cyc(24);
        wr16(b | 0x032B, 0x0000);
    } else {
        next = rd16(cyc_then(82, field(bank, obj, 0x00)));
        cyc(44);
        wr16(b | (uint16_t)(0x2000 + prev), next);
        if (next)
            { cyc(60); wr16(b | (uint16_t)(0x2002 + next), prev); cyc(36); }   /* BRA, PLX */
        else
            { cyc(46); wr16(b | 0x032B, prev); cyc(14); }                      /* PLX */
    }
    cyc(46);
    head = rd16(b | 0x032D);
    wr16(field(bank, obj, 0x00), cyc_then(30, head));
    cyc(24);
    wr16(b | 0x032D, obj);
    cyc(164);
    return head;
}

void ObjectFree_cr(Carried *cr)
{
    cr->a = ObjectFree(cr->db, cr->x);
    set_p(cr, (cr->p & P_X) ? nz8(cr->p, (uint8_t)cr->x) : (cyc(0), nz16(cr->p, cr->x)));
}

void abi_ObjectFree(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFree_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:B8A1 ObjectFreeLong
 * WHAT IT DOES: ObjectFree for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ObjectFree. Exit: as ObjectFree.
 */
uint16_t ObjectFreeLong(uint8_t bank, uint16_t obj)
{
    cyc(46);
    return cyc_then(44, ObjectFree(bank, obj));
}

void ObjectFreeLong_cr(Carried *cr)
{
    (cyc(46), ObjectFree_cr(cr));
    cyc(44);
}

void abi_ObjectFreeLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFreeLong_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:CCA1 ObjectRelease
 * WHAT IT DOES: Frees object X (ObjectFreeLong), unless it is the object
 * at $034C, which instead counts one up in $035C.
 * HOW: A 16-bit compare of X; the count is an 8-bit increment.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: X, Y unchanged; C from the
 * compare; the object at $034C: N, Z from the new count; otherwise as
 * ObjectFree.
 */
uint16_t ObjectRelease(uint8_t bank, uint16_t obj, uint16_t a, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    cyc(24);
    uint16_t m = rd16(b | 0x034C);
    uint8_t n;

    *p = (uint8_t)((*p & ~P_C) | (obj >= m ? P_C : 0));
    if (obj == m) {
        cyc(40);
        n = (uint8_t)(rd8(b | 0x035C) + 1);
        cyc(6);
        wr8(b | 0x035C, n);
        *p = nz8(*p, n);
        cyc(66);                        /* BRA, RTL */
        return a;
    } else { cyc(84); }
    a = ObjectFreeLong(bank, obj);
    *p = (*p & P_X) ? nz8(*p, (uint8_t)obj) : (cyc(44), nz16(*p, obj));
    return a;
}

void ObjectRelease_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ObjectRelease(cr->db, cr->x, cr->a, &p);
    set_p(cr, p);
}

void abi_ObjectRelease(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRelease_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:AB46 ObjectCallHandler
 * WHAT IT DOES: Runs object Y's handler, the routine at bank +$1C, address
 * +$1A, with X = Y = the object, keeping $1F58 and X, Y across it.
 * HOW: Pushes a return address ($08:AB67, so the handler's RTL lands on
 * $08:AB68), then the handler's bank and address - 1, and RTLs into it.
 * Entry: M 8-bit, X 16-bit; Y = object. The handler is entered with A =
 * its address - 1 (N, Z from that), M 8-bit, X = Y = the object, and must
 * return with M 8-bit. Exit: A low byte = $1F58, A high byte as the handler
 * left it; X, Y unchanged; N, Z from X; C, V as the handler left them.
 */
void ObjectCallHandler(Carried *cr, uint16_t obj)
{
    uint32_t b = (uint32_t)cr->db << 16;
    cyc(84);
    uint8_t keep = rd8(b | 0x1F58), bank = rd8(cyc_then(262, b | (uint16_t)(0x201C + obj)));
    cyc(74);
    uint16_t addr = rd16(b | (uint16_t)(0x201A + obj)), a = (uint16_t)(addr - 1);

    cr->a = a;
    cr->x = cr->y = obj;
    set_p(cr, (uint8_t)(nz16(cr->p, a) | P_M));
    cyc(110);
    carried_call(cr, (uint32_t)bank << 16 | addr, 1);
    cyc(124);
    wr8((uint32_t)cr->db << 16 | 0x1F58, keep);
    cr->a = (uint16_t)((cr->a & 0xFF00) | keep);
}

void ObjectCallHandler_cr(Carried *cr)
{
    uint16_t x = cr->x, y = cr->y;

    ObjectCallHandler(cr, y);
    cr->x = x;
    cr->y = y;
    set_p(cr, nz16(cr->p, x));
}

void abi_ObjectCallHandler(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCallHandler_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Run an object's handler as the dispatchers do it: the handler's address
   (word) and bank are kept at $0354/$0356 and the object at $034C, and the
   handler is entered by an RTL with X = the object, A = its address - 1, M
   8-bit, N and Z from that A. Returns 0 when there is no handler. */
static int run_handler(Carried *cr, uint8_t bank, uint16_t obj, uint16_t word_at, uint16_t bank_at, int need_no_record)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t h;
    uint8_t p = cr->p, hb;

    cyc(82);
    wr8(b | 0x0356, cyc_then(24, rd8(b | (uint16_t)(bank_at + obj))));
    cyc(52);
    h = rd16(b | (uint16_t)(word_at + obj));
    cr->a = h;
    if (!h)
        { cyc(130); return 0; }
    cyc(40);
    wr16(b | 0x0354, h);
    if (need_no_record && (cr->a = rd16(cyc_then(30, b | (uint16_t)(0x2022 + obj)))) != 0)
        return 0;
    cyc(need_no_record ? 40 : 24);                     /* B: BNE not taken */
    wr16(b | 0x034C, obj);
    cyc(182);
    hb = rd8(b | 0x0356);
    cyc(68);
    h = rd16(b | 0x0354);
    cr->a = (uint16_t)(h - 1);
    cr->x = obj;
    set_p(cr, (uint8_t)(nz16(p, cr->a) | P_M));
    cyc(110);
    carried_call(cr, (uint32_t)hb << 16 | h, 1);
    cyc(108);
    return 1;
}

/*
 * @routine 03:CAB8 ObjectRunHandlerA
 * WHAT IT DOES: Runs object X's first handler: the long address at +$1A
 * (word) / +$1C (bank), unless the word is 0.
 * HOW: Keeps the bank at $0356, the word at $0354 and the object at $034C,
 * pushes $03:CAE7 as the handler's return and enters the handler with an RTL
 * (A = its address - 1, M 8-bit).
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A and Y as the handler leaves
 * them (A = 0 with no handler); X, P unchanged.
 */
void ObjectRunHandlerA(Carried *cr, uint8_t bank, uint16_t obj)
{
    run_handler(cr, bank, obj, 0x201A, 0x201C, 0);
}

void ObjectRunHandlerA_cr(Carried *cr)
{
    uint16_t x = cr->x;
    uint8_t p = cr->p;

    run_handler(cr, cr->db, x, 0x201A, 0x201C, 0);
    cr->x = x;
    set_p(cr, p);
}

void abi_ObjectRunHandlerA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRunHandlerA_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:CAEB ObjectRunHandlerB
 * WHAT IT DOES: Runs object X's second handler: the long address at +$1D
 * (word) / +$1F (bank), unless the word is 0 or the object has a Super FX
 * record (+$22).
 * HOW: As ObjectRunHandlerA, returning through $03:CB1F.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A and Y as the handler leaves
 * them (A = 0 with no handler, the record address when there is one); M
 * 8-bit; X, P unchanged.
 */
void ObjectRunHandlerB(Carried *cr, uint8_t bank, uint16_t obj)
{
    run_handler(cr, bank, obj, 0x201D, 0x201F, 1);
}

void ObjectRunHandlerB_cr(Carried *cr)
{
    uint16_t x = cr->x;
    uint8_t p = cr->p;

    cyc(0);
    run_handler(cr, cr->db, x, 0x201D, 0x201F, 1);
    cr->x = x;
    set_p(cr, p);
    cyc(44);
}

void abi_ObjectRunHandlerB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRunHandlerB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ObjectRelease(void);

/*
 * @routine 03:CCB0 ObjectReleaseY
 * WHAT IT DOES: ObjectRelease for the object in Y, called from other banks.
 * HOW: Swaps X and Y around a long call to ObjectRelease (through the
 * stack), so its exit X and Y come back swapped too.
 * Entry: X 16-bit; Y = object. Exit: X = ObjectRelease's exit Y, Y = its
 * exit X; A as it leaves it; N, Z from Y.
 */
void ObjectReleaseY(Carried *cr)
{
    uint16_t t = cr->x;

    cr->x = cr->y;
    cr->y = t;
    (cyc(142), ObjectRelease_cr(cr));
    t = cr->x;
    cr->x = cr->y;
    cr->y = t;
}

void ObjectReleaseY_cr(Carried *cr)
{
    ObjectReleaseY(cr);
    set_p(cr, (cr->p & P_X) ? nz8(cr->p, (uint8_t)cr->y) : (cyc(124), nz16(cr->p, cr->y)));
}

void abi_ObjectReleaseY(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectReleaseY_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D388 ObjectsRunHandlers
 * WHAT IT DOES: The objects' frame: counts the frame in $05E9 (16-bit), then
 * for each object in the used list (from $0329) steps its countdown at +$36:
 * a negative one restarts at 3 and runs the second handler
 * (ObjectRunHandlerB); one above 1 counts down and runs the first handler
 * (ObjectRunHandlerA); one that reaches 0 from 1 clears +$22, +$23 and
 * itself. An object a handler asked to free ($035C nonzero) is freed
 * (ObjectFreeLong) after its next link is read.
 * HOW: Data bank $7E for the walk; $035C is cleared before each object. The
 * handlers are entered with X = the object, Y = the object (the caller's Y
 * for the first) and the countdown's N, Z, C and V as the caller left them.
 * Entry: X 16-bit; any M. Exit: A low byte = the last byte read; X = Y = 0;
 * M 8-bit, X 16-bit; N, Z from the restored data bank; C, V as the handlers
 * leave them.
 */
void ObjectsRunHandlers(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t db = cr->db, v;
    uint16_t x, y = cr->y;

    cyc(46);
    v = (uint8_t)(rd8(b | 0x05E9) + 1);
    cyc(6);
    wr8(b | 0x05E9, v);
    if (!v)
        { cyc(40); wr8(b | 0x05EA, cyc_then(6, (uint8_t)(rd8(b | 0x05EA) + 1))); } else { cyc(22); }
    cr->db = 0x7E;
    b = 0x7E0000;
    cyc(134);
    x = rd16(b | 0x0329);
    while (x) {
        cyc(24);
        wr8(b | 0x035C, 0x00);
        cyc(30);
        v = rd8(b | (uint16_t)(0x2036 + x));
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        if (v && (v & 0x80 || v != 1)) {
            if (v & 0x80) {
                v = 0x03;
                cyc(84);
                wr8(b | (uint16_t)(0x2036 + x), v);
                cr->a = (uint16_t)((cr->a & 0xFF00) | v);
                cr->x = x;
                cr->y = y;
                set_p(cr, (uint8_t)((nz8(cr->p, v) | P_M) & ~P_X));
                (cyc(62), ObjectRunHandlerB_cr(cr));
            } else {
                v = (uint8_t)(v - 1);
                cyc(98);
                wr8(b | (uint16_t)(0x2036 + x), v);
                cr->a = (uint16_t)((cr->a & 0xFF00) | v);
                cr->x = x;
                cr->y = y;
                set_p(cr, (uint8_t)((nz8(cr->p, v) | P_M) & ~P_X));
                (cyc(62), ObjectRunHandlerA_cr(cr));
            }
            b = (uint32_t)cr->db << 16;
            cyc(24);
            v = rd8(b | 0x035C);
            cr->a = (uint16_t)((cr->a & 0xFF00) | v);
            if (v) {
                cyc(52);
                y = rd16(b | (uint16_t)(0x2000 + x));
                cr->x = x;
                cr->y = y;
                (cyc(62), ObjectFreeLong_cr(cr));
                x = y;
                cyc(58);
                continue;
            }
            cyc(46);                    /* $035C 0: on to the next */
        } else {
            if (v) {
                cyc(92);
                wr8(b | (uint16_t)(0x2022 + x), 0x00);
                cyc(30);
                wr8(b | (uint16_t)(0x2023 + x), 0x00);
                cyc(30);
                wr8(b | (uint16_t)(0x2036 + x), 0x00);
            }
            cyc(52);
        }
        y = rd16(b | (uint16_t)(0x2000 + x));
        x = y;
        cyc(36);
    }
    cyc(88);
    cr->x = 0;
    cr->y = 0;
    cr->db = db;
    set_p(cr, (uint8_t)((nz8(cr->p, db) | P_M) & ~P_X));
}

void ObjectsRunHandlers_g(void)
{
    Carried c = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};
    ObjectsRunHandlers(&c);

    cpu.a = c.a;
    cpu.x = c.x;
    cpu.y = c.y;
    cpu.d = c.d;
    cpu.p = c.p;
    cpu.db = c.db;
}

void abi_ObjectsRunHandlers(void)
{
    ObjectsRunHandlers_g();
}

void abi_ObjectAlloc(void);
void abi_ObjectInit(void);
void abi_ObjectRebucket(void);
void abi_CrashGreen(void);

#define OBJ_FIELD(o, f) (0x7E2000u + (uint16_t)((f) + (o)))

/*
 * @routine 08:826E ObjectSpawnFromScript
 * WHAT IT DOES: Makes a new object (ObjectAlloc, ObjectInit) from the
 * course script entry at Y (in the data bank, read at $8000 + Y on): its
 * model (+$04) from +$07, +$20 from +$09, position (+$0C, +$0E, +$10) from
 * +$01, +$03, +$05, script pointer (+$1A-$1C) from +$0B and +$0C, flags
 * (+$06) from +$14, and +$15 from $1AF4; the offsets +$53, +$55 and +$57
 * from +$0E, +$10 and +$12, added to the record that +$20 points to in
 * bank 0 (+$0A, +$0C, +$0E) when bit 9 of the flags is set, and +$53 and
 * +$57 swapped when the low byte of $1AF4 is neither 0 nor $80. Then puts
 * it in its bucket (ObjectRebucket) and sets +$36 to 3 when the first byte
 * of its script is $EA. With no free object it stops the game
 * (CrashGreen); no script has got there.
 * HOW: Long accesses to the object (bank $7E); the bank-0 record and the
 * script byte through the data bank, switched for them and restored.
 * Entry: X 16-bit; Y = script entry. Exit: A = the high byte of the
 * script pointer - $8000 and the script byte (or 3); X = the object; Y
 * unchanged; M 16-bit; N, Z from the data bank pulled back; C from
 * comparing the script byte with $EA; V from subtracting $8000 (set when
 * the script pointer is below $8000).
 */
/* A bank $00 word at a: 16 when it is ROM ($8000 up), whose reads the C states. */
static uint32_t rom_word(uint16_t a)
{
    return a >= 0x8000 ? 16 : 0;
}

/* The same for a 24-bit address: ROM in banks $00-$3F and $80-$BF from $8000. */
static uint32_t rom_word_at(uint32_t a)
{
    uint8_t bank = (uint8_t)(a >> 16);

    return (bank < 0x40 || (bank >= 0x80 && bank < 0xC0)) && (a & 0xFFFF) >= 0x8000 ? 16 : 0;
}

void ObjectSpawnFromScript(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t y = cr->y, o, w, r, t, u;
    uint8_t v, bank;
    uint32_t rm;                        /* the script entry's two-byte reads from ROM, which the C states */

    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(114), ObjectAlloc_cr(cr));
    if (!(cr->p & P_C)) {
        CrashGreen_cr(cr);
        return;
    } else { cyc(98); }
    cr->y = cr->x;
    ObjectInit_cr(cr);
    cr->y = y;
    o = cr->x;
    rm = rom_word_at(b + 0x8000 + y);
    cyc(90);
    wr8(OBJ_FIELD(o, 0x15), cyc_then(32, rd8(0x001AF4)));
    cyc(52);
    wr16(OBJ_FIELD(o, 0x04), cyc_then(32 + rm, rd16(b | (uint16_t)(0x8007 + y))));
    cyc(30);
    wr16(OBJ_FIELD(o, 0x20), cyc_then(32 + rm, rd16(b | (uint16_t)(0x8009 + y))));
    cyc(30);
    wr16(OBJ_FIELD(o, 0x0C), cyc_then(32 + rm, rd16(b | (uint16_t)(0x8001 + y))));
    cyc(30 + rm);
    wr16(OBJ_FIELD(o, 0x0E), cyc_then(32, rd16(b | (uint16_t)(0x8003 + y))));
    cyc(30 + rm);
    wr16(OBJ_FIELD(o, 0x10), cyc_then(32, rd16(b | (uint16_t)(0x8005 + y))));
    cyc(30 + rm);
    wr16(OBJ_FIELD(o, 0x1A), cyc_then(32, rd16(b | (uint16_t)(0x800B + y))));
    cyc(30 + rm);
    wr16(OBJ_FIELD(o, 0x1B), cyc_then(32, rd16(b | (uint16_t)(0x800C + y))));
    cyc(30 + rm);
    w = rd16(b | (uint16_t)(0x8014 + y));
    cyc(32);
    wr16(OBJ_FIELD(o, 0x06), w);
    if (w & 0x0200) {
        cyc(234);
        r = rd16(OBJ_FIELD(o, 0x20));
        /* the words are ROM (two reads of 8 the C states) or work RAM */
        cyc(44);
        wr16(OBJ_FIELD(o, 0x53), cyc_then(32 + rom_word((uint16_t)(0x000A + r)), rd16((uint16_t)(0x000A + r))));
        cyc(30);
        wr16(OBJ_FIELD(o, 0x55), cyc_then(32 + rom_word((uint16_t)(0x000C + r)), rd16((uint16_t)(0x000C + r))));
        cyc(30);
        wr16(OBJ_FIELD(o, 0x57), cyc_then(32 + rom_word((uint16_t)(0x000E + r)), rd16((uint16_t)(0x000E + r))));
        cyc(94);
    } else { cyc(76); }
    t = rd16(b | (uint16_t)(0x800E + y));
    cyc(46 + rm);
    wr16(OBJ_FIELD(o, 0x53), cyc_then(32, (uint16_t)(t + rd16(OBJ_FIELD(o, 0x53)))));
    cyc(30);
    t = rd16(b | (uint16_t)(0x8010 + y));
    cyc(46 + rm);
    wr16(OBJ_FIELD(o, 0x55), cyc_then(32, (uint16_t)(t + rd16(OBJ_FIELD(o, 0x55)))));
    cyc(30);
    t = rd16(b | (uint16_t)(0x8012 + y));
    cyc(46 + rm);
    wr16(OBJ_FIELD(o, 0x57), cyc_then(32, (uint16_t)(t + rd16(OBJ_FIELD(o, 0x57)))));
    cyc(32);
    w = (uint16_t)(rd16(0x001AF4) & 0x00FF);
    if (w != 0x0000 && (cyc(40), w != 0x0080)) {
        cyc(72);
        t = rd16(OBJ_FIELD(o, 0x53));
        cyc(62);
        u = rd16(OBJ_FIELD(o, 0x57));
        cyc(32);
        wr16(OBJ_FIELD(o, 0x53), u);
        cyc(68);
        wr16(OBJ_FIELD(o, 0x57), t);
    } else { cyc(46); }
    cyc(62);
    cr->x = o;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    ObjectRebucket_cr(cr);
    cyc(76);
    bank = rd8(OBJ_FIELD(o, 0x1C));
    cyc(134);
    w = (uint16_t)(rd16(OBJ_FIELD(o, 0x1A)) - 0x8000);
    cyc(104);
    v = rd8((uint32_t)bank << 16 | (uint16_t)(0x8000 + w));
    cr->x = o;
    /* V from SBC #$8000 (set when the pointer is below $8000), C from the
       compare with $EA. */
    set_p(cr, (uint8_t)((cr->p & ~(P_C | P_V)) | (v >= 0xEA ? P_C : (cyc(74), 0)) | ((w & 0x8000) ? P_V : (cyc(8), 0))));
    if (v == 0xEA) {
        cyc(116);
        wr8(OBJ_FIELD(o, 0x36), 0x03);
        v = 0x03;
    }
    cyc(92);
    cr->a = (uint16_t)((w & 0xFF00) | v);
    set_p(cr, (uint8_t)(nz8(cr->p, cr->db) & ~P_M));
}

void abi_ObjectSpawnFromScript(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpawnFromScript(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B0A1 ObjectStep48To8
 * WHAT IT DOES: Counts object X's +$48 up to 8, then copies it to $1F58
 * and, with bit 7 set, to +$5A.
 * HOW: 8-bit accesses through the data bank; at 8 the increment is skipped
 * by a long jump past it.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A low byte = +$48 OR $80;
 * N set, Z clear; C from comparing the old +$48 with 8.
 */
uint8_t ObjectStep48To8(uint8_t bank, uint16_t obj, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    cyc(30);
    uint8_t v = rd8(b | (uint16_t)(0x2048 + obj));

    *p = (uint8_t)((*p & ~P_C) | (v >= 0x08 ? (cyc(64), P_C) : 0));
    if (v != 0x08)
        { cyc(68); wr8(b | (uint16_t)(0x2048 + obj), cyc_then(6, (uint8_t)(rd8(b | (uint16_t)(0x2048 + obj)) + 1))); }
    cyc(30);
    v = rd8(b | (uint16_t)(0x2048 + obj));
    cyc(24);
    wr8(b | 0x1F58, v);
    cyc(24);
    v = (uint8_t)(rd8(b | 0x1F58) | 0x80);
    cyc(46);
    wr8(b | (uint16_t)(0x205A + obj), v);
    *p = nz8(*p, v);
    cyc(42);
    return v;
}

void ObjectStep48To8_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | ObjectStep48To8(cr->db, cr->x, &p));
    set_p(cr, p);
}

void abi_ObjectStep48To8(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectStep48To8_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_RandomStepLong(void);
void abi_ObjectSpawn(void);
void abi_ObjectCopyPositionTo(void);

/*
 * @routine 08:D181 ObjectSpawnChanceD460
 * WHAT IT DOES: With a chance of 78 in 256 (RandomStepLong at $B2 or
 * more), makes a new object (ObjectSpawn, kind word $A158 at $1F2B) with
 * handler $08:D460 at object X's position (ObjectCopyPositionTo) and X as
 * its +$27.
 * HOW: The handler is the long pointer +$1A-$1C; through the data bank.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: no spawn: A low byte = the
 * random value or as ObjectSpawn leaves it; otherwise A low byte as
 * ObjectCopyPositionTo leaves it, N, Z from it; Y = the new object; M
 * 8-bit.
 */
void ObjectSpawnChanceD460(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t y;
    uint8_t a;

    (cyc(62), w_RandomStepLong(cr));
    set_p(cr, (uint8_t)((cr->p & ~P_C) | ((uint8_t)cr->a >= 0xB2 ? P_C : (cyc(14), 0))));
    set_p(cr, nz8(cr->p, (uint8_t)((uint8_t)cr->a - 0xB2)));
    if ((uint8_t)cr->a < 0xB2) {
        cyc(2);
        return;
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->a = 0xA158;
    cyc(108);
    wr16(b | 0x1F2B, 0xA158);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(84), ObjectSpawn_cr(cr));
    if (!(cr->p & P_C))
        return;
    y = cr->y;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(98);
    wr16(b | (uint16_t)(0x201A + y), 0xD460);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x08);
    cyc(68);
    wr8(b | (uint16_t)(0x201C + y), 0x08);
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    a = (uint8_t)cr->a;
    cyc(88);
    wr16(b | (uint16_t)(0x2027 + cr->y), cr->x);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->a = (uint16_t)((cr->x & 0xFF00) | a);
    set_p(cr, nz8(cr->p, a));
    cyc(2);
}

void abi_ObjectSpawnChanceD460(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpawnChanceD460(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:F911 ObjectAdvance5DIfFlag
 * WHAT IT DOES: Adds 2 to object X's word +$5D when $0D62 is set.
 * HOW: A 16-bit add through the data bank.
 * Entry: X 16-bit; X = object. Exit: M 8-bit; when $0D62 is clear, A low
 * byte = 0 with Z set; otherwise A = the new word, N, Z, C, V from the
 * add.
 */
void ObjectAdvance5DIfFlag(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;
    uint16_t w;

    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(46);
    v = rd8(b | 0x0D62);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    if (!v)
        { cyc(64); return; }
    cyc(68);
    w = rd16(b | (uint16_t)(0x205D + cr->x));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_C))));
    cr->a = alu_adc16(w, 0x0002, &cr->p);
    cyc(68);
    wr16(b | (uint16_t)(0x205D + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(64);
}

void abi_ObjectAdvance5DIfFlag(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectAdvance5DIfFlag(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:F927 ObjectReleaseLong
 * WHAT IT DOES: ObjectRelease for callers in other banks and object
 * handler tables.
 * HOW: SEP #$20 and a long call.
 * Entry: X 16-bit; X = object. Exit: as ObjectRelease, M 8-bit.
 */
void ObjectReleaseLong(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(84), ObjectRelease_cr(cr));
    cyc(44);
}

void abi_ObjectReleaseLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectReleaseLong(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ObjectRebucket(void);
void abi_ObjectCopyPositionFrom(void);
void abi_ObjectApplyVelocity(void);

/* PHX, TYX, PLY: X and Y swapped, N and Z from the pulled Y. */
static void swap_xy(Carried *cr)
{
    cyc(120);
    uint16_t t = cr->x;

    cr->x = cr->y;
    cr->y = t;
    set_p(cr, nz16(cr->p, cr->y));
}

/* A new object (kind $A158) with handler $08:EA6B at object X's position,
   X as its +$06 and put in its bucket. */
static void spawn_sparkle(Carried *cr, uint32_t b)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->a = 0xA158;
    cyc(26);
    wr16(b | 0x1F2B, 0xA158);
    set_p(cr, (uint8_t)(nz16(cr->p, 0xA158) | P_M));
    (cyc(84), ObjectSpawn_cr(cr));
    if (!(cr->p & P_C))
        { cyc(92); return; }            /* BCS, JML; then the next spawn's REP and LDA or RTL */
    cr->a = 0xEA6B;
    cyc(98);
    wr16(b | (uint16_t)(0x201A + cr->y), 0xEA6B);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x08);
    cyc(68);
    wr8(b | (uint16_t)(0x201C + cr->y), 0x08);
    cyc(88);
    wr16(b | (uint16_t)(0x2006 + cr->y), cr->x);
    cr->a = (uint16_t)((cr->x & 0xFF00) | 0x08);
    set_p(cr, nz8(cr->p, 0x08));
    (cyc(112), ObjectCopyPositionTo_cr(cr));
    (cyc(22), swap_xy(cr));
    ObjectRebucket_cr(cr);
    swap_xy(cr);
    cyc(4);
}

/*
 * @routine 08:E9F4 ObjectSpawnSparklePair
 * WHAT IT DOES: Makes two objects (kind $A158 at $1F2B) at object X's
 * position, each with handler $08:EA6B (ObjectSparkleStart) and object X
 * as its parent (+$06), and puts each in its bucket (ObjectRebucket).
 * HOW: The handler is the long pointer +$1A-$1C; a failed spawn skips that
 * object. Through the data bank.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: X = object; Y =
 * the last new object (or as ObjectSpawn leaves it); M 8-bit; A and flags
 * as the last step left them.
 */
void ObjectSpawnSparklePair(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    cyc(44);
    spawn_sparkle(cr, b);
    spawn_sparkle(cr, b);
}

void abi_ObjectSpawnSparklePair(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpawnSparklePair(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* +off += (random & mask), 16-bit. */
static void add_random(Carried *cr, uint32_t b, uint16_t off, uint16_t mask)
{
    cyc(62);
    uint16_t w;

    w_RandomStepLong(cr);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_and16(cr, mask);
    cyc(88);
    w = rd16(b | (uint16_t)(off + cr->x));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    cr->a = alu_adc16(cr->a, w, &cr->p);
    cyc(30);
    wr16(b | (uint16_t)(off + cr->x), cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

/*
 * @routine 08:EB07 ObjectSparkleStep
 * WHAT IT DOES: Object X's frame as a sparkle: takes its parent's position
 * (+$06, ObjectCopyPositionFrom), moves it (ObjectApplyVelocity), adds its
 * acceleration (+$27, +$29, +$2B) to its velocity (+$2E, +$30, +$32); when
 * its count +$48 is 0 it is released (ObjectReleaseLong), otherwise counted
 * down and rebucketed.
 * HOW: 16-bit adds through the data bank; the release is a jump.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; as ObjectRebucket
 * or ObjectReleaseLong.
 */
void ObjectSparkleStep(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t w;
    uint8_t v;
    unsigned i;

    cyc(30);
    cr->y = rd16(b | (uint16_t)(0x2006 + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
    (cyc(62), ObjectCopyPositionFrom_cr(cr));
    (cyc(62), ObjectApplyVelocity_cr(cr));
    for (i = 0; i < 3; i++) {
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cyc(52);
        w = rd16(b | (uint16_t)(0x202E + i * 2 + cr->x));
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
        cyc(44);
        cr->a = alu_adc16(w, rd16(b | (uint16_t)(0x2027 + i * 2 + cr->x)), &cr->p);
        cyc(30);
        wr16(b | (uint16_t)(0x202E + i * 2 + cr->x), cr->a);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(22);
    }
    cyc(30);
    v = rd8(b | (uint16_t)(0x2048 + cr->x));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    if (!v) {
        cyc(48);
        ObjectReleaseLong(cr);
        return;
    }
    cyc(52);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2048 + cr->x)) - 1);
    cyc(6);
    wr8(b | (uint16_t)(0x2048 + cr->x), v);
    set_p(cr, nz8(cr->p, v));
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

void abi_ObjectSparkleStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSparkleStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:EA6B ObjectSparkleStart
 * WHAT IT DOES: Object X's first frame as a sparkle: sets bit 3 of +$24,
 * +$5D = $9998, +$18 = a random 0-3 with bit 7 (kept in $1F58), handler
 * $08:EB07 (ObjectSparkleStep), +$5F = $0A, count +$48 = 5, and a random
 * acceleration: +$27 and +$2B get -$3F to +$40 added, +$29 becomes
 * -(+$29 + 0 to $3F); then runs its first step.
 * HOW: RandomStepLong for each random part; falls into ObjectSparkleStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSparkleStep.
 */
void ObjectSparkleStart(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;
    uint16_t w;

    cyc(30);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) | 0x08);
    cyc(46);
    wr8(b | (uint16_t)(0x2024 + cr->x), v);
    cyc(76);
    wr16(b | (uint16_t)(0x205D + cr->x), 0x9998);
    (cyc(84), w_RandomStepLong(cr));
    v = (uint8_t)(cr->a & 0x03);
    cyc(40);
    wr8(b | 0x1F58, v);
    cyc(24);
    v = (uint8_t)(rd8(b | 0x1F58) | 0x80);
    cyc(46);
    wr8(b | (uint16_t)(0x2018 + cr->x), v);
    cyc(76);
    wr16(b | (uint16_t)(0x201A + cr->x), 0xEB07);
    cyc(68);
    wr8(b | (uint16_t)(0x201C + cr->x), 0x08);
    cyc(46);
    wr8(b | (uint16_t)(0x205F + cr->x), 0x0A);
    cyc(46);
    wr8(b | (uint16_t)(0x2048 + cr->x), 0x05);
    add_random(cr, b, 0x2027, 0x007F);
    cyc(22);
    add_random(cr, b, 0x2029, 0x003F);
    cyc(22);
    add_random(cr, b, 0x202B, 0x007F);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(74);
    w = rd16(b | (uint16_t)(0x2027 + cr->x));
    set_p(cr, (uint8_t)(cr->p | (P_C)));
    w = alu_sbc16(w, 0x003F, &cr->p);
    cyc(62);
    wr16(b | (uint16_t)(0x2027 + cr->x), w);
    cyc(74);
    w = (uint16_t)((rd16(b | (uint16_t)(0x2029 + cr->x)) ^ 0xFFFF) + 1);
    cyc(68);
    wr16(b | (uint16_t)(0x2029 + cr->x), w);
    cyc(74);
    w = rd16(b | (uint16_t)(0x202B + cr->x));
    set_p(cr, (uint8_t)(cr->p | (P_C)));
    w = alu_sbc16(w, 0x003F, &cr->p);
    cyc(62);
    wr16(b | (uint16_t)(0x202B + cr->x), w);
    cr->a = w;
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(22), ObjectSparkleStep(cr));
}

void abi_ObjectSparkleStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSparkleStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:D14A ObjectSpawnD460UnlessFlag
 * WHAT IT DOES: Unless $0D62 is set, makes a new object (ObjectSpawn, kind
 * word $A158 at $1F2B) with handler $08:D460 at object X's position
 * (ObjectCopyPositionTo) and X as its +$27, as ObjectSpawnChanceD460.
 * HOW: The handler is the long pointer +$1A-$1C; through the data bank.
 * The no-object exit is not reached in the traces so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSpawnChanceD460;
 * with $0D62 set, A low byte = it, Z clear.
 */
void ObjectSpawnD460UnlessFlag(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v, a;

    cyc(24);
    v = rd8(b | 0x0D62);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    if (v)
        { cyc(6); return; }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->a = 0xA158;
    cyc(86);
    wr16(b | 0x1F2B, 0xA158);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(84), ObjectSpawn_cr(cr));
    if (!(cr->p & P_C))
        { cyc(90); return; }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->a = 0xD460;
    cyc(98);
    wr16(b | (uint16_t)(0x201A + cr->y), 0xD460);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x08);
    cyc(68);
    wr8(b | (uint16_t)(0x201C + cr->y), 0x08);
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    a = (uint8_t)cr->a;
    cyc(88);
    wr16(b | (uint16_t)(0x2027 + cr->y), cr->x);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->a = (uint16_t)((cr->x & 0xFF00) | a);
    set_p(cr, nz8(cr->p, a));
    cyc(34);
}

void abi_ObjectSpawnD460UnlessFlag(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpawnD460UnlessFlag(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_RotateXY(void);
void abi_RotateXZFrom08(void);
void abi_RotateXZ(void);

/* Direct page byte or word at off. */
static uint32_t dp8(Carried *cr, uint8_t off)
{
    return (uint16_t)(cr->d + off);
}

/* A = 0 and BIT #$80: the sign byte at `at` set to $FF when bit 7 of A is
   set (never, A being 0), else cleared. */
static void sign_of_zero(uint32_t at)
{
    uint8_t a = 0x00;

    if (a & 0x80)
        wr8(at, 0xFF);
    else
        { cyc(54); wr8(at, 0x00); }
}

/*
 * @routine 08:B34B ObjectMoveForward
 * WHAT IT DOES: Moves object X forward by its speed: the byte of the table
 * at $08:B32B for its Super FX record's word +$5E (the record at +$22), as
 * the vector (0, speed, 0) turned by the object's angles +$16 (RotateXY),
 * +$14 (RotateXZFrom08) and +$15 (RotateXZ); four times the result is added
 * to its position +$0C, +$0E, +$10.
 * HOW: The vector is kept in direct page $02/$08/$94 (sign bytes $05, $0B,
 * $05F3) and the rotations leave it in $04/$0A/$05F2. The sign bytes of
 * the zero components are set from A = 0, so their $FF stores run only as
 * the XSign and ZSign entries; the early return tests the flags from PLX
 * (X = 0, never an object; reached by a unit call with X = 0).
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: M 8-bit; A = the
 * new +$0E; N, Z, C, V from its add.
 */

void ObjectMoveForward(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t x0 = cr->x;
    uint8_t v;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(82);
    cr->a = rd16(b | (uint16_t)(0x2022 + cr->x));
    cr->x = cr->a;
    cyc(46);
    cr->a = rd16((uint32_t)(0x70005E + cr->x));
    cr->x = cr->a;
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(128);
    v = rd8((uint32_t)(0x08B32B + cr->x));
    cr->x = x0;
    set_p(cr, nz16(cr->p, x0));
    wr8(dp8(cr, 0x08), v);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    if (!x0)
        { cyc(88); return; }
    move_forward_from(cr, 0);
}

void abi_ObjectMoveForward(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectMoveForward(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ObjectMoveForward from its vector on. Arm 1 enters at the $FF store of
   the first component's sign byte ($05), arm 3 at that of the third
   ($05F3): the stores the zero components never take. */
static void move_forward_from(Carried *cr, int arm)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t w;
    uint8_t v;
    unsigned i;
    static const uint8_t angle[3] = {0x16, 0x14, 0x15};

    if (arm == 3) {
        cr->a = (uint16_t)(cr->a | 0x00FF);
        set_p(cr, nz8(cr->p, 0xFF));
        cyc(40);
        wr8(b | 0x05F3, 0xFF);
        cyc(22);                        /* BRA */
        goto turn;
    }
    if (arm == 1) {
        cr->a = (uint16_t)(cr->a | 0x00FF);
        set_p(cr, nz8(cr->p, 0xFF));
        cyc(32);
        wr8(dp8(cr, 0x05), 0xFF);
        cyc(22);                        /* BRA */
    } else {
        wr8(dp8(cr, 0x02), cyc_then(54, 0x00));
        wr8(dp8(cr, 0x04), cyc_then(16, 0x00));
        sign_of_zero(dp8(cr, 0x05));
    }
    v = rd8(cyc_then(16, dp8(cr, 0x08)));
    wr8(dp8(cr, 0x08), cyc_then(16, v));
    wr8(dp8(cr, 0x0A), cyc_then(16, v));
    wr8(dp8(cr, 0x0B), cyc_then(54, (v & 0x80) ? (cyc(10), 0xFF) : 0x00));
    cyc((v & 0x80) ? 54 : 32);          /* BRA (negative), LDA #0, STA */
    wr8(dp8(cr, 0x94), 0x00);
    cyc(24);
    wr8(b | 0x05F2, 0x00);
    cyc(8);
    sign_of_zero(b | 0x05F3);
turn:
    for (i = 0; i < 3; i++) {
        if (i) {
            wr8(dp8(cr, 0x02), cyc_then(16, rd8(cyc_then(16, dp8(cr, 0x04)))));
            wr8(dp8(cr, 0x08), cyc_then(16, rd8(cyc_then(16, dp8(cr, 0x0A)))));
            wr8(dp8(cr, 0x94), cyc_then(16, rd8(cyc_then(24, b | 0x05F2))));
        }
        cyc(30);
        v = rd8(b | (uint16_t)(0x2000 + angle[i] + cr->x));
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        set_p(cr, nz8(cr->p, v));
        if (i == 0)
            { cyc(62); RotateXY_cr(cr); }
        else if (i == 1)
            { cyc(62); RotateXZFrom08_cr(cr); }
        else
            { cyc(62); RotateXZ_cr(cr); }
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    w = (uint16_t)(rd16(cyc_then(38, dp8(cr, 0x04))) << 2);
    wr16(dp8(cr, 0x04), cyc_then(44, w));
    w = (uint16_t)(rd16(cyc_then(16, dp8(cr, 0x0A))) << 2);
    wr16(dp8(cr, 0x0A), cyc_then(44, w));
    cyc(24);
    w = (uint16_t)(rd16(b | 0x05F2) << 2);
    cyc(52);
    wr16(b | 0x05F2, w);
    {
        static const uint16_t to[3] = {0x200C, 0x2010, 0x200E};
        uint32_t from[3];

        from[0] = dp8(cr, 0x04);
        from[1] = b | 0x05F2;
        from[2] = dp8(cr, 0x0A);
        for (i = 0; i < 3; i++) {
            cyc(30);
            w = rd16(b | (uint16_t)(to[i] + cr->x));
            cyc(from[i] < 0x100 ? 30 : 38);  /* CLC, ADC direct page or absolute */
            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
            cr->a = alu_adc16(w, rd16(from[i]), &cr->p);
            cyc(30);
            wr16(b | (uint16_t)(to[i] + cr->x), cr->a);
        }
        cyc(64);
    }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

/*
 * @routine 08:B36F ObjectMoveForwardXSign
 * WHAT IT DOES: ObjectMoveForward entered at the sign store of its first
 * vector component that the zero never takes: direct page $05 = $FF, then
 * the rest as ObjectMoveForward from the speed component on.
 * HOW: LDA #$FF, STA $05, then the join; no code reaches these bytes, a
 * direct call is their only way in.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object; direct page $08 = the
 * speed. Exit: as ObjectMoveForward.
 */
void ObjectMoveForwardXSign(Carried *cr)
{
    move_forward_from(cr, 1);
}

void abi_ObjectMoveForwardXSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectMoveForwardXSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B394 ObjectMoveForwardZSign
 * WHAT IT DOES: ObjectMoveForward entered at the sign store of its third
 * vector component that the zero never takes: $05F3 = $FF, then the turns
 * and the move as ObjectMoveForward.
 * HOW: LDA #$FF, STA $05F3, then the join; no code reaches these bytes, a
 * direct call is their only way in.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = object; the vector in direct
 * page $02, $08, $94 and $05F2 with its sign bytes. Exit: as
 * ObjectMoveForward.
 */
void ObjectMoveForwardZSign(Carried *cr)
{
    move_forward_from(cr, 3);
}

void abi_ObjectMoveForwardZSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectMoveForwardZSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* LDA a, then BIT #$80: the sign byte at `at` is $FF when bit 7 of a is
   set, else 0. */
static void sign_byte(uint32_t at, uint8_t a)
{
    cyc(54);
    wr8(at, (a & 0x80) ? (cyc(10), 0xFF) : 0x00);
}

/* sign_byte, with the BRA over the zero store taken after the $FF store. */
static void sign_byte_bra(uint32_t at, uint8_t a)
{
    sign_byte(at, a);
    if (a & 0x80)
        cyc(22);                                       /* BRA */
}

/* Object X at its parent (+$27)'s position plus an offset turned by the
   parent's angles +$14 (RotateXZFrom08) and +$15 (RotateXZ) and doubled,
   then rebucketed. A trail's offset is (0, -10, +$32), +$32 moving by -5
   after; an attached object's is (0, +$30, +$32), kept at $1F59/$1F5A.
   From stage 1 (the x sign's store of $FF, which cannot run: the x is 0)
   or stage 2 (the trail's y sign's store of 0, which cannot run: the y is
   $F6), with Y the parent and the stores before those made. */
static void parent_place_from(Carried *cr, uint32_t b, int attached, int stage)
{
    uint8_t v;
    uint16_t w;
    unsigned i;
    static const uint16_t to[3] = {0x200C, 0x2010, 0x200E};
    uint32_t from[3];

    if (stage == 0) {
        cyc(30);
        cr->y = rd16(b | (uint16_t)(0x2027 + cr->x));
        if (attached) {
            cyc(30);
            wr8(b | 0x1F59, cyc_then(24, rd8(b | (uint16_t)(0x2030 + cr->x))));
            cyc(30);
            wr8(b | 0x1F5A, cyc_then(24, rd8(b | (uint16_t)(0x2032 + cr->x))));
        } else {
            cyc(30);
            v = rd8(b | (uint16_t)(0x2032 + cr->x));
            cyc(24);
            wr8(b | 0x1F58, v);
        }
        wr8(dp8(cr, 0x02), cyc_then(32, 0x00));
        wr8(dp8(cr, 0x04), cyc_then(16, 0x00));
        sign_byte(dp8(cr, 0x05), 0x00);
    } else if (stage == 1) {
        wr8(dp8(cr, 0x05), cyc_then(32, 0xFF));
        cyc(22);                        /* BRA */
    }
    if (stage <= 1) {
        cyc(16);
        v = attached ? rd8(cyc_then(8, b | 0x1F59)) : 0xF6;
        wr8(dp8(cr, 0x08), cyc_then(16, v));
        wr8(dp8(cr, 0x0A), cyc_then(16, v));
        sign_byte_bra(dp8(cr, 0x0B), v);
    } else {
        wr8(dp8(cr, 0x0B), cyc_then(16, 0x00));
    }
    cyc(24);
    v = rd8(b | (attached ? 0x1F5A : 0x1F58));
    wr8(dp8(cr, 0x94), cyc_then(16, v));
    cyc(24);
    wr8(b | 0x05F2, v);
    cyc(8);
    sign_byte_bra(b | 0x05F3, v);
    for (i = 0; i < 2; i++) {
        if (i) {
            wr8(dp8(cr, 0x02), cyc_then(16, rd8(cyc_then(16, dp8(cr, 0x04)))));
            wr8(dp8(cr, 0x08), cyc_then(16, rd8(cyc_then(16, dp8(cr, 0x0A)))));
            wr8(dp8(cr, 0x94), cyc_then(16, rd8(cyc_then(24, b | 0x05F2))));
        }
        cyc(30);
        v = rd8(b | (uint16_t)(0x2014 + i + cr->y));
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        set_p(cr, nz8(cr->p, v));
        if (i == 0)
            { cyc(62); RotateXZFrom08_cr(cr); }
        else
            { cyc(62); RotateXZ_cr(cr); }
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    w = (uint16_t)(rd16(cyc_then(38, dp8(cr, 0x04))) << 1);
    wr16(dp8(cr, 0x04), cyc_then(30, w));
    w = (uint16_t)(rd16(cyc_then(16, dp8(cr, 0x0A))) << 1);
    wr16(dp8(cr, 0x0A), cyc_then(30, w));
    cyc(24);
    w = (uint16_t)(rd16(b | 0x05F2) << 1);
    cyc(38);
    wr16(b | 0x05F2, w);
    from[0] = dp8(cr, 0x04);
    from[1] = b | 0x05F2;
    from[2] = dp8(cr, 0x0A);
    for (i = 0; i < 3; i++) {
        cyc(30);
        w = rd16(b | (uint16_t)(to[i] + cr->y));
        cyc(from[i] < 0x100 ? 30 : 38);  /* CLC, ADC direct page or absolute */
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
        cr->a = alu_adc16(w, rd16(from[i]), &cr->p);
        cyc(30);
        wr16(b | (uint16_t)(to[i] + cr->x), cr->a);
    }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (!attached) {
        cyc(52);
        v = rd8(b | (uint16_t)(0x2032 + cr->x));
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
        v = alu_adc8(v, 0xFB, &cr->p);
        cyc(60);
        wr8(b | (uint16_t)(0x2032 + cr->x), v);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    } else { cyc(22); }
    cyc(62);
    ObjectRebucket_cr(cr);
    cyc(44);
}

static void parent_place(Carried *cr, uint32_t b, int attached)
{
    parent_place_from(cr, b, attached, 0);
}

/*
 * @routine 08:E8CF ObjectSpawnTrail
 * WHAT IT DOES: Unless $0D62 is set, makes a new object (ObjectSpawn, kind
 * word $A158 at $1F2B) with bit 2 of +$24 set, +$4A = 3, +$48 = $14, +$49 =
 * $0A, handler $08:E919 (ObjectTrailStart) and object X as its parent
 * (+$27).
 * HOW: The handler is the long pointer +$1A-$1C; through the data bank.
 * The no-object exit is not reached in the traces so far.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; Y = the new object
 * (or as ObjectSpawn leaves it); A low byte = 8, high byte from X; or A
 * low byte = $0D62 with it set.
 */
void ObjectSpawnTrail(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    cyc(24);
    v = rd8(b | 0x0D62);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    if (v)
        { cyc(66); return; }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->a = 0xA158;
    cyc(86);
    wr16(b | 0x1F2B, 0xA158);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(84), ObjectSpawn_cr(cr));
    if (!(cr->p & P_C))
        { cyc(92); return; }
    cyc(52);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->y)) | 0x04);
    cyc(46);
    wr8(b | (uint16_t)(0x2024 + cr->y), v);
    cyc(46);
    wr8(b | (uint16_t)(0x204A + cr->y), 0x03);
    cyc(46);
    wr8(b | (uint16_t)(0x2048 + cr->y), 0x14);
    cyc(46);
    wr8(b | (uint16_t)(0x2049 + cr->y), 0x0A);
    cyc(76);
    wr16(b | (uint16_t)(0x201A + cr->y), 0xE919);
    cyc(68);
    wr8(b | (uint16_t)(0x201C + cr->y), 0x08);
    cyc(88);
    wr16(b | (uint16_t)(0x2027 + cr->y), cr->x);
    cr->a = (uint16_t)((cr->x & 0xFF00) | 0x08);
    set_p(cr, nz8(cr->p, 0x08));
    cyc(94);
}

void abi_ObjectSpawnTrail(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpawnTrail(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E919 ObjectTrailStart
 * WHAT IT DOES: A trail object's first frame: its count +$4B = $0C, handler
 * $08:E934 (ObjectTrailStep), +$32 = $D8, then placed by its parent.
 * HOW: Byte and word stores, then the shared placing (see
 * ObjectTrailStep).
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket.
 */
void ObjectTrailStart(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    cyc(46);
    wr8(b | (uint16_t)(0x204B + cr->x), 0x0C);
    cyc(76);
    wr16(b | (uint16_t)(0x201A + cr->x), 0xE934);
    cyc(68);
    wr8(b | (uint16_t)(0x201C + cr->x), 0x08);
    cyc(46);
    wr8(b | (uint16_t)(0x2032 + cr->x), 0xD8);
    cyc(22);
    parent_place(cr, b, 0);
}

void abi_ObjectTrailStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTrailStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E934 ObjectTrailStep
 * WHAT IT DOES: A trail object's frame: sets bit 2 of +$24 and clears +$4A,
 * +$48 and +$49; when its count +$4B is 0 it is released
 * (ObjectReleaseLong), otherwise counted down and put at its parent's
 * position (+$27) plus (0, -10, +$32) turned by the parent's angles +$14
 * and +$15 (RotateXZFrom08, RotateXZ) and doubled, with +$32 moved by -5,
 * and rebucketed.
 * HOW: The offset is kept in direct page $02/$08/$94 (sign bytes $05,
 * $0B, $05F3) and comes back in $04/$0A/$05F2; the release is a jump. The
 * sign stores that cannot run (bit 7 of 0, and of $F6 clear) are kept.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket or
 * ObjectReleaseLong.
 */
void ObjectTrailStep(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    cyc(30);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) | 0x04);
    cyc(46);
    wr8(b | (uint16_t)(0x2024 + cr->x), v);
    cyc(46);
    wr8(b | (uint16_t)(0x204A + cr->x), 0x00);
    cyc(46);
    wr8(b | (uint16_t)(0x2048 + cr->x), 0x00);
    cyc(46);
    wr8(b | (uint16_t)(0x2049 + cr->x), 0x00);
    cyc(30);
    v = rd8(b | (uint16_t)(0x204B + cr->x));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    if (!v) {
        cyc(48);
        ObjectReleaseLong(cr);
        return;
    }
    cyc(52);
    v = (uint8_t)(rd8(b | (uint16_t)(0x204B + cr->x)) - 1);
    cyc(6);
    wr8(b | (uint16_t)(0x204B + cr->x), v);
    parent_place(cr, b, 0);
}

void abi_ObjectTrailStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTrailStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E96A ObjectTrailSignX
 * WHAT IT DOES: ObjectTrailPlace's store of $FF as the offset's x sign
 * ($05), which nothing reaches (the test is of bit 7 of the x, 0); then
 * the rest of the placing.
 * HOW: As ObjectTrailPlace from there.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its parent; $1F58 = its +$32,
 * $02 = $04 = 0. Exit: as ObjectTrailStep.
 */
void ObjectTrailSignX(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0xFF);
    set_p(cr, nz8(cr->p, 0xFF));
    parent_place_from(cr, (uint32_t)cr->db << 16, 0, 1);
}

void abi_ObjectTrailSignX(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTrailSignX(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:E982 ObjectTrailSignY
 * WHAT IT DOES: ObjectTrailPlace's store of 0 as the offset's y sign ($0B),
 * which nothing reaches (the test is of bit 7 of the y, $F6); then the
 * rest of the placing.
 * HOW: As ObjectTrailPlace from there.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its parent; $1F58 = its +$32,
 * $02 = $04 = 0, $05 the x sign, $08 = $0A = the y. Exit: as
 * ObjectTrailStep.
 */
void ObjectTrailSignY(Carried *cr)
{
    parent_place_from(cr, (uint32_t)cr->db << 16, 0, 2);
}

void abi_ObjectTrailSignY(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTrailSignY(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:EBF5 ObjectAttachedSignX
 * WHAT IT DOES: ObjectAttachedPlace's store of $FF as the offset's x sign
 * ($05), which nothing reaches (the test is of bit 7 of the x, 0); then
 * the rest of the placing.
 * HOW: As ObjectAttachedPlace from there.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its parent; $1F59/$1F5A = its
 * +$30/+$32, $02 = $04 = 0. Exit: as ObjectAttachedStep.
 */
void ObjectAttachedSignX(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0xFF);
    set_p(cr, nz8(cr->p, 0xFF));
    parent_place_from(cr, (uint32_t)cr->db << 16, 1, 1);
}

void abi_ObjectAttachedSignX(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectAttachedSignX(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:EB4D ObjectSpawnAttached
 * WHAT IT DOES: Unless $0D62 is set, makes a new object (ObjectSpawn, kind
 * word $A158 at $1F2B) with bit 2 of +$24 set, +$4A = 2, +$48 = 5, +$49 =
 * $0A, handler $08:EBA3 (ObjectAttachedStart), object X as its parent
 * (+$27) and the offset $1F59/$1F5A as its +$30/+$32.
 * HOW: As ObjectSpawnTrail. The no-object exit is not reached in the
 * traces so far.
 * Entry: M 8-bit, X 16-bit; X = object; $1F59/$1F5A = the offset. Exit: M
 * 8-bit; Y = the new object; A low byte = $1F5A; or as ObjectSpawnTrail.
 */
void ObjectSpawnAttached(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    cyc(24);
    v = rd8(b | 0x0D62);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    if (v)
        { cyc(66); return; }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->a = 0xA158;
    cyc(86);
    wr16(b | 0x1F2B, 0xA158);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(84), ObjectSpawn_cr(cr));
    if (!(cr->p & P_C))
        { cyc(92); return; }
    cyc(52);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->y)) | 0x04);
    cyc(46);
    wr8(b | (uint16_t)(0x2024 + cr->y), v);
    cyc(46);
    wr8(b | (uint16_t)(0x204A + cr->y), 0x02);
    cyc(46);
    wr8(b | (uint16_t)(0x2048 + cr->y), 0x05);
    cyc(46);
    wr8(b | (uint16_t)(0x2049 + cr->y), 0x0A);
    cyc(76);
    wr16(b | (uint16_t)(0x201A + cr->y), 0xEBA3);
    cyc(68);
    wr8(b | (uint16_t)(0x201C + cr->y), 0x08);
    cyc(88);
    wr16(b | (uint16_t)(0x2027 + cr->y), cr->x);
    cyc(74);
    wr8(b | (uint16_t)(0x2030 + cr->y), cyc_then(30, rd8(b | 0x1F59)));
    cyc(24);
    v = rd8(b | 0x1F5A);
    cyc(30);
    wr8(b | (uint16_t)(0x2032 + cr->y), v);
    cr->a = (uint16_t)((cr->x & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    cyc(44);
}

void abi_ObjectSpawnAttached(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpawnAttached(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:EBA3 ObjectAttachedStart
 * WHAT IT DOES: An attached object's first frame: count +$4B = $0C,
 * handler $08:EBB9 (ObjectAttachedStep), then placed by its parent.
 * HOW: As ObjectTrailStart.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket.
 */
void ObjectAttachedStart(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    cyc(46);
    wr8(b | (uint16_t)(0x204B + cr->x), 0x0C);
    cyc(76);
    wr16(b | (uint16_t)(0x201A + cr->x), 0xEBB9);
    cyc(68);
    wr8(b | (uint16_t)(0x201C + cr->x), 0x08);
    cyc(22);
    parent_place(cr, b, 1);
}

void abi_ObjectAttachedStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectAttachedStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:EBB9 ObjectAttachedStep
 * WHAT IT DOES: As ObjectTrailStep, with the offset (0, +$30, +$32) kept at
 * $1F59/$1F5A and no drift.
 * HOW: As ObjectTrailStep. The sign stores of the zero x are kept.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket or
 * ObjectReleaseLong.
 */
void ObjectAttachedStep(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    cyc(30);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2024 + cr->x)) | 0x04);
    cyc(46);
    wr8(b | (uint16_t)(0x2024 + cr->x), v);
    cyc(46);
    wr8(b | (uint16_t)(0x204A + cr->x), 0x00);
    cyc(46);
    wr8(b | (uint16_t)(0x2048 + cr->x), 0x00);
    cyc(46);
    wr8(b | (uint16_t)(0x2049 + cr->x), 0x00);
    cyc(30);
    v = rd8(b | (uint16_t)(0x204B + cr->x));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    if (!v) {
        cyc(48);
        ObjectReleaseLong(cr);
        return;
    }
    cyc(52);
    v = (uint8_t)(rd8(b | (uint16_t)(0x204B + cr->x)) - 1);
    cyc(6);
    wr8(b | (uint16_t)(0x204B + cr->x), v);
    parent_place(cr, b, 1);
}

void abi_ObjectAttachedStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectAttachedStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ObjectTableE8AF(void);

/*
 * @routine 08:E798 ObjectPlaceOnParent
 * WHAT IT DOES: Puts object Y on object X: unless bit 0 of Y's +$25 is set
 * (then +$18 = +$19 = $8C), $1F4C = Y's +$48 plus the high byte of X's
 * Super FX record word +$3A * 4, and Y's +$18 and +$19 become the table
 * bytes (ObjectTableE8AF) for $1A75 and $1AB0 with bit 7 set; then Y's
 * position is X's plus Y's offset (+$2E, +$30, +$32) turned by X's angles
 * +$16 (RotateXY), +$14 (RotateXZFrom08) and +$15 (RotateXZ) and doubled.
 * HOW: The offset is kept at $1F58-$1F5A and in direct page $02/$08/$94
 * (sign bytes $05, $0B, $05F3); the table values are read through direct
 * page + X with X set to $1A64 and $1A9F. The $8C path is not reached in
 * the traces so far.
 * Entry: M 8-bit, X 16-bit, D = $0000; X = the parent, Y = the object.
 * Exit: M 8-bit; A = the new +$0E; N, Z, C, V from its add.
 */
void ObjectPlaceOnParent(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;
    uint16_t w, x0;
    unsigned i;
    static const uint16_t to[3] = {0x200C, 0x2010, 0x200E};
    static const uint16_t from_x[2] = {0x1A64, 0x1A9F};
    uint32_t from[3];

    cyc(30);
    v = (uint8_t)(rd8(b | (uint16_t)(0x2025 + cr->y)) & 0x01);
    if (v) {
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0x8C);
        cyc(110);
        wr8(b | (uint16_t)(0x2018 + cr->y), 0x8C);
        cyc(30);
        wr8(b | (uint16_t)(0x2019 + cr->y), 0x8C);
        cyc(22);                        /* BRA */
    } else {
        x0 = cr->x;
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cyc(120);
        w = rd16(b | (uint16_t)(0x2022 + cr->x));
        cr->x = w;
        if (w) {
            cyc(62);
            w = (uint16_t)(rd16((uint32_t)(0x70003A + cr->x)) << 2);
            w = (uint16_t)(w >> 8 | w << 8);
        }
        cr->a = w;
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
        cyc(114);
        v = alu_adc8((uint8_t)w, rd8(b | (uint16_t)(0x2048 + cr->y)), &cr->p);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        cyc(24);
        wr8(b | 0x1F4C, v);
        cr->x = x0;
        cyc(58);
        for (i = 0; i < 2; i++) {
            cyc(76);
            v = rd8((uint16_t)(cr->d + 0x11 + from_x[i]));
            cr->a = (uint16_t)((cr->a & 0xFF00) | v);
            set_p(cr, nz16(cr->p, cr->x));
            (cyc(82), ObjectTableE8AF_cr(cr));
            v = (uint8_t)(cr->a | 0x80);
            cr->a = (uint16_t)((cr->a & 0xFF00) | v);
            cyc(46);
            wr8(b | (uint16_t)(0x2018 + i + cr->y), v);
        }
    }
    for (i = 0; i < 3; i++)
        { cyc(30); wr8(b | (uint16_t)(0x1F58 + i), cyc_then(24, rd8(b | (uint16_t)(0x202E + i * 2 + cr->y)))); }
    cyc(24);
    v = rd8(b | 0x1F58);
    wr8(dp8(cr, 0x02), cyc_then(16, v));
    wr8(dp8(cr, 0x04), cyc_then(16, v));
    sign_byte(dp8(cr, 0x05), v);
    cyc((v & 0x80) ? 46 : 24);          /* BRA (negative), LDA abs */
    v = rd8(b | 0x1F59);
    wr8(dp8(cr, 0x08), cyc_then(16, v));
    wr8(dp8(cr, 0x0A), cyc_then(16, v));
    sign_byte(dp8(cr, 0x0B), v);
    cyc((v & 0x80) ? 46 : 24);          /* BRA (negative), LDA abs */
    v = rd8(b | 0x1F5A);
    wr8(dp8(cr, 0x94), cyc_then(16, v));
    cyc(24);
    wr8(b | 0x05F2, v);
    cyc(8);
    sign_byte(b | 0x05F3, v);
    if (v & 0x80)
        cyc(22);                        /* BRA */
    for (i = 0; i < 3; i++) {
        static const uint8_t angle[3] = {0x16, 0x14, 0x15};

        if (i) {
            wr8(dp8(cr, 0x02), cyc_then(16, rd8(cyc_then(16, dp8(cr, 0x04)))));
            wr8(dp8(cr, 0x08), cyc_then(16, rd8(cyc_then(16, dp8(cr, 0x0A)))));
            wr8(dp8(cr, 0x94), cyc_then(16, rd8(cyc_then(24, b | 0x05F2))));
        }
        cyc(30);
        v = rd8(b | (uint16_t)(0x2000 + angle[i] + cr->x));
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
        set_p(cr, nz8(cr->p, v));
        if (i == 0)
            { cyc(62); RotateXY_cr(cr); }
        else if (i == 1)
            { cyc(62); RotateXZFrom08_cr(cr); }
        else
            { cyc(62); RotateXZ_cr(cr); }
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    w = (uint16_t)(rd16(cyc_then(38, dp8(cr, 0x04))) << 1);
    wr16(dp8(cr, 0x04), cyc_then(30, w));
    w = (uint16_t)(rd16(cyc_then(16, dp8(cr, 0x0A))) << 1);
    wr16(dp8(cr, 0x0A), cyc_then(30, w));
    cyc(24);
    w = (uint16_t)(rd16(b | 0x05F2) << 1);
    cyc(38);
    wr16(b | 0x05F2, w);
    from[0] = dp8(cr, 0x04);
    from[1] = b | 0x05F2;
    from[2] = dp8(cr, 0x0A);
    for (i = 0; i < 3; i++) {
        cyc(30);
        w = rd16(b | (uint16_t)(to[i] + cr->x));
        cyc(from[i] < 0x100 ? 30 : 38);  /* CLC, ADC direct page or absolute */
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
        cr->a = alu_adc16(w, rd16(from[i]), &cr->p);
        cyc(30);
        wr16(b | (uint16_t)(to[i] + cr->y), cr->a);
    }
    cyc(64);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

void abi_ObjectPlaceOnParent(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPlaceOnParent(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:D3DD ObjectLinkEventTrigger2
 * WHAT IT DOES: ObjectLinkEventTrigger for player 2: unless bit 7 of $0DA6
 * is set, when the object two links on from the one at $034A has bit 14
 * set in its first word and $1A26 is non-zero, clears $1A26, sets $0DA6 to
 * $80 and starts bob 1 ($1A1D = $1A, $1A1F = $1A23 = 0, $1A21 = $FE).
 * HOW: As ObjectLinkEventTrigger; a zero $1A26 leaves through its exit.
 * The start is not reached in the traces so far.
 * Entry: any widths. Exit: as ObjectLinkEventTrigger.
 */
void ObjectLinkEventTrigger2(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t f;
    uint16_t a;

    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(46);
    f = (uint8_t)(rd8(b | 0x0DA6) & 0x80);
    cr->a = (uint16_t)((cr->a & 0xFF00) | f);
    set_p(cr, nz8(cr->p, f));
    if (!f) {
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_M | P_X))));
        cyc(78);
        cr->x = rd16(b | 0x034A);
        cyc(30);
        cr->y = rd16(b | (uint16_t)(0x204F + cr->x));
        cyc(30);
        cr->y = rd16(b + 0x204F + cr->y);
        cyc(44);
        a = (uint16_t)(rd16(b | cr->y) & 0x4000);
        cr->a = a;
        set_p(cr, nz16(cr->p, a));
        if (!a) {
            cyc(46);                    /* AND, BEQ taken */
        } else {
            cyc(86);
            a = rd16(b | 0x1A26);
            cr->a = a;
            set_p(cr, nz16(cr->p, a));
            if (!a) {
                cyc(22);                /* BEQ taken */
            } else {
                cyc(64);
                wr16(b | 0x1A26, 0x0000);
                set_p(cr, (uint8_t)(cr->p | (P_M)));
                cyc(62);
                wr8(b | 0x0DA6, 0x80);
                cyc(40);
                wr8(b | 0x1A1D, 0x1A);
                cyc(40);
                wr8(b | 0x1A1F, 0x00);
                cyc(24);
                wr8(b | 0x1A23, 0x00);
                cyc(40);
                wr8(b | 0x1A21, 0xFE);
                cr->a = 0x00FE;
                set_p(cr, nz8(cr->p, 0xFE));
            }
        }
    } else { cyc(38); }                 /* AND, BNE taken */
    cyc(42);
    r_sep(cr, P_M | P_X);
}

void abi_ObjectLinkEventTrigger2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLinkEventTrigger2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:D41D ObjectLinkSpeedSet
 * WHAT IT DOES: ObjectLinkEventTrigger2's last store as its own entry
 * (ViewParamsSpeedLong's JSR): A at $1A21, then 8-bit registers.
 * HOW: STA.W (as wide as M) through the data bank, SEP #$30, RTS.
 * Entry: any widths; data bank $7E. Exit: M and X 8-bit (X and Y high
 * bytes 0); A unchanged.
 */
void ObjectLinkSpeedSet(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    if (cr->p & P_M)
        { cyc(40); wr8(b | 0x1A21, (uint8_t)cr->a); }
    else
        { cyc(40); wr16(b | 0x1A21, cr->a); }
    cyc(86);
    r_sep(cr, P_M | P_X);
}

void abi_ObjectLinkSpeedSet(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLinkSpeedSet(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:D410 ObjectLinkBobStart
 * WHAT IT DOES: ObjectLinkEventTrigger2's bob start as its own entry
 * (ViewParamsBobLong's JSR): bob 1 with A as its kind ($1A1D = A, $1A1F
 * = $1A23 = 0), then ObjectLinkSpeedSet with $FE.
 * HOW: 8-bit stores through the data bank.
 * Entry: M 8-bit (the code is 8-bit); data bank $7E. Exit: M and X 8-bit
 * (X and Y high bytes 0); A low byte = $FE; N set, Z clear.
 */
void ObjectLinkBobStart(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    cyc(70);
    wr8(b | 0x1A1D, (uint8_t)cr->a);
    cyc(40);
    wr8(b | 0x1A1F, 0x00);
    cyc(24);
    wr8(b | 0x1A23, 0x00);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0xFE);
    set_p(cr, nz8(cr->p, 0xFE));
    ObjectLinkSpeedSet(cr);
}

void abi_ObjectLinkBobStart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLinkBobStart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A - m (8-bit) as CMP sets the flags; returns the difference. */
static uint8_t cmp8_flags(Carried *cr, uint8_t a, uint8_t m)
{
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? (cyc(22), P_C) : 0)));
    return (uint8_t)(a - m);
}

/* EOR #$FF, INC A (8-bit): the negation, with N, Z. */
static uint8_t neg8(Carried *cr, uint8_t a)
{
    cyc(24);
    a = (uint8_t)(~a + 1);
    set_p(cr, nz8(cr->p, a));
    return a;
}

/* PHX, LDX #camera, LDA $11,X, EOR #$FF, INC A, STA $1F5A, PLX: minus the
   camera's turn (its byte +$11), kept at $1F5A. */
static void camera_turn_negated(Carried *cr, uint32_t b, uint16_t camera)
{
    uint16_t x = cr->x;
    uint8_t v;

    cr->x = camera;
    cyc(76);
    v = neg8(cr, rd8((uint16_t)(camera + 0x11)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cyc(30);
    wr8(b | 0x1F5A, v);
    cr->x = x;
    set_p(cr, nz16(cr->p, x));
}

/*
 * @routine 08:BFA4 ObjectBillboardTilt
 * WHAT IT DOES: The tilt of object Y's picture as seen by the camera
 * $1A64: half its pitch +$16 plus the camera's +$13, rounded to 4 toward
 * zero and held to -8..8, negated when its heading +$15 is $40 or more
 * from the camera's turn (minus +$11); +$5D = that (sign-extended) + 8 +
 * its +$29; then the table bytes (ObjectTableC056) and +$18 = the turn
 * with bit 7 set; with two players ($0D62) the second camera ($1A9F) the
 * same into +$19.
 * HOW: 8-bit with the halving as CMP #$80 / ROR A and the limits by the
 * sign of CMP; $1F2B, $1F58 and $1F5A hold the parts; the camera fields
 * through direct page + X.
 * Entry: M 8-bit, X 16-bit, D = $0000; Y = the object. Exit: M 8-bit; A =
 * the last +$18/+$19 byte; X as on entry; flags from the last step.
 */
void ObjectBillboardTilt(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t y = cr->y, w;
    uint8_t a, c;

    camera_turn_negated(cr, b, 0x1A64);
    {
        uint16_t x = cr->x;

        cr->x = 0x1A64;
        cyc(22);
        a = rd8(0x1A64 + 0x13);
        cyc(24);
        wr8(b | 0x1F2B, a);
        cr->x = x;
        set_p(cr, nz16(cr->p, x));
    }
    cyc(66);
    a = rd8(b | (uint16_t)(0x2016 + y));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    cyc(38);
    a = alu_adc8(a, rd8(b | 0x1F2B), &cr->p);
    cmp8_flags(cr, a, 0x80);
    c = (uint8_t)(cr->p & P_C);
    a = (uint8_t)(a >> 1 | (c ? 0x80 : 0));
    set_p(cr, nz8(cr->p, a));
    if (a & 0x80) {
        a = neg8(cr, a);
        a &= 0xFC;
        a = neg8(cr, a);
    }
    a &= 0xFC;
    set_p(cr, nz8(cr->p, a));
    if (!(cmp8_flags(cr, a, 0x08) & 0x80))
        a = 0x08;
    if (cmp8_flags(cr, a, 0xF8) & 0x80)
        a = 0xF8;
    cyc(168);
    wr8(b | 0x1F58, a);
    cyc(30);
    a = rd8(b | (uint16_t)(0x2015 + y));
    set_p(cr, (uint8_t)(cr->p | (P_C)));
    cyc(38);
    a = alu_sbc8(a, rd8(b | 0x1F5A), &cr->p);
    if (a & 0x80)
        a = neg8(cr, a);
    cyc(40);
    if (!(cmp8_flags(cr, a, 0x40) & 0x80)) {
        a = neg8(cr, rd8(b | 0x1F58));
        cyc(30);
        wr8(b | 0x1F58, a);
    } else { cyc(4); }
    r_rep(cr, P_M);
    cyc(24);
    r_lda16(cr, b | 0x1F58);
    if (cr->a & 0x0080)
        cr->a |= 0xFF00;
    else
        cr->a &= 0x00FF;
    set_p(cr, nz16(cr->p, cr->a));
    r_add16(cr, 0x0008);
    cyc(152);
    r_add16(cr, rd16(b | (uint16_t)(0x2029 + y)));
    cyc(30);
    wr16(b | (uint16_t)(0x205D + y), cr->a);
    r_sep(cr, P_M);
    ObjectTableC056_cr(cr);
    cyc(24);
    r_lda8(cr, b | 0x1F5A);
    a = (uint8_t)(cr->a | 0x80);
    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    set_p(cr, nz8(cr->p, a));
    cyc(46);
    wr8(b | (uint16_t)(0x2018 + y), a);
    cyc(24);
    r_lda8(cr, b | 0x0D62);
    if (!(uint8_t)cr->a)
        { cyc(64); return; }
    camera_turn_negated(cr, b, 0x1A9F);
    ObjectTableC056_cr(cr);
    r_lda8(cr, b | 0x1F5A);
    a = (uint8_t)(cr->a | 0x80);
    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    set_p(cr, nz8(cr->p, a));
    wr8(b | (uint16_t)(0x2019 + y), a);
    (void)w;
}

void abi_ObjectBillboardTilt(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBillboardTilt(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C37A ObjectSpawnPuff
 * WHAT IT DOES: Counts a hit: $1A28 up by one and the decimal counter at
 * $19EE-$19EF up by one; then makes an object of kind $A158 (ObjectSpawn)
 * at object Y's position, 500 units higher ($FE0C), with +$5D = $9BB4,
 * +$29 = $FE0C, bit 3 of +$24 set, +$3A = 0, +$5F = $14 and +$0A = 7.
 * Without a free object nothing is made (decoded from the ROM bytes, its
 * exit the PLX, RTS at $09:C3EB; unit_cpu reaches it).
 * HOW: SED around the decimal adds; X kept on the stack; the new object
 * comes back in Y.
 * Entry: M 8-bit, X 16-bit; Y = the object. Exit: X restored, N, Z from
 * it; Y = the new object; M 8-bit.
 */
void ObjectSpawnPuff(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t x = cr->x, v;
    uint8_t a;

    cyc(46);
    a = rd8(b | 0x1A28);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    a = alu_adc8(a, 0x01, &cr->p);
    cyc(54);
    wr8(b | 0x1A28, a);
    set_p(cr, (uint8_t)(cr->p | (P_D)));
    cyc(38);
    a = rd8(b | 0x19EE);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    a = alu_adc8(a, 0x01, &cr->p);
    cyc(54);
    wr8(b | 0x19EE, a);
    cyc(24);
    a = rd8(b | 0x19EF);
    a = alu_adc8(a, 0x00, &cr->p);
    cyc(40);
    wr8(b | 0x19EF, a);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_D)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    cr->x = cr->y;
    set_p(cr, nz16(cr->p, cr->x));
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0xA158);
    cyc(68);
    wr16(b | 0x1F2B, 0xA158);
    r_sep(cr, P_M);
    (cyc(84), ObjectSpawn_cr(cr));
    if (!(cr->p & P_C)) {
        cyc(126);
        cr->x = x;
        set_p(cr, nz16(cr->p, x));
        return;
    }
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0x9BB4);
    cyc(38);
    wr16(b | (uint16_t)(0x205D + cr->y), 0x9BB4);
    r_sep(cr, P_M);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0xFE0C);
    cyc(38);
    wr16(b | (uint16_t)(0x2029 + cr->y), 0xFE0C);
    r_sep(cr, P_M);
    cyc(52);
    r_lda8(cr, b | (uint16_t)(0x2024 + cr->y));
    a = (uint8_t)(cr->a | 0x08);
    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    set_p(cr, nz8(cr->p, a));
    cyc(46);
    wr8(b | (uint16_t)(0x2024 + cr->y), a);
    cr->a = (uint16_t)(cr->a & 0xFF00);
    cyc(46);
    wr8(b | (uint16_t)(0x203A + cr->y), 0x00);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x14);
    cyc(46);
    wr8(b | (uint16_t)(0x205F + cr->y), 0x14);
    set_p(cr, nz8(cr->p, 0x14));
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    r_rep(cr, P_M);
    cyc(30);
    r_lda16(cr, b | (uint16_t)(0x200E + cr->y));
    r_add16(cr, 0xFE0C);
    cyc(68);
    wr16(b | (uint16_t)(0x200E + cr->y), cr->a);
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x07);
    cyc(68);
    wr8(b | (uint16_t)(0x200A + cr->y), 0x07);
    cr->x = x;
    set_p(cr, nz16(cr->p, x));
    (void)v;
    cyc(78);
}

void abi_ObjectSpawnPuff(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpawnPuff(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ObjectPoseFromRecord(void);
void abi_GsuRun(void);
void abi_ObjectCarFrame(void);
void abi_ObjectCarRecordNew(void);
void abi_ObjectCarRecordTake(void);

/* An object's field: DB:$2000 + offset + X. */
static uint32_t car_field(Carried *cr, uint16_t off)
{
    return ((uint32_t)cr->db << 16) + off + cr->x;
}

/*
 * @routine 08:B23D ObjectCarFrame
 * WHAT IT DOES: A car object's frame: unless the player's car ($0346) has
 * bit 3 of +$25 and this object's record +$1C has bit 4, steps the record's
 * flag (ObjectRecordFlag70Step), the near-focus test, the pose from the
 * record and the object's bucket; otherwise marks the record (+$16 |=
 * $0420), +$36 = 3 and +$24 bit 5.
 * HOW: 8-bit A for the flags; the record by long accesses with X = +$22.
 * Entry: M 8-bit, X 16-bit, data bank $7E; X = the object. Exit: as
 * ObjectRebucket, or M 8-bit with A = the new +$24.
 */
void ObjectCarFrame(Carried *cr)
{
    uint16_t obj = cr->x;
    uint8_t v;

    cyc(24);
    cr->y = rd16(((uint32_t)cr->db << 16) | 0x0346);
    set_p(cr, nz16(cr->p, cr->y));
    cyc(30);
    v = (uint8_t)(rd8(((uint32_t)cr->db << 16) + 0x2025 + cr->y) & 0x08);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    if (v) {
        r_rep(cr, P_M);
        r_lda16(cr, cyc_then(98, car_field(cr, 0x2022)));
        r_tax16(cr);
        cyc(16);
        r_lda16(cr, 0x70001C + cr->x);
        cr->x = obj;
        set_p(cr, nz16(cr->p, obj));
        r_and16(cr, 0x0010);
        r_sep(cr, P_M);
        if (cr->a) {
            r_rep(cr, P_M);
            r_lda16(cr, cyc_then(162, car_field(cr, 0x2022)));
            r_tax16(cr);
            cyc(16);
            cr->a = (uint16_t)(rd16(0x700016 + cr->x) | 0x0420);
            set_p(cr, nz16(cr->p, cr->a));
            cyc(56);
            wr16(0x700016 + cr->x, cr->a);
            r_sep(cr, P_M);
            cr->x = obj;
            set_p(cr, nz16(cr->p, obj));
            cr->a = (uint16_t)((cr->a & 0xFF00) | 0x03);
            set_p(cr, nz8(cr->p, 0x03));
            wr8(car_field(cr, 0x2036), cyc_then(104, 0x03));
            v = (uint8_t)(rd8(cyc_then(30, car_field(cr, 0x2024))) | 0x20);
            cr->a = (uint16_t)((cr->a & 0xFF00) | v);
            set_p(cr, nz8(cr->p, v));
            wr8(car_field(cr, 0x2024), cyc_then(46, v));
            cyc(44);
            return;
        } else { cyc(32); }
    }
    cyc(126);
    ObjectRecordFlag70Step_cr(cr);
    (cyc(62), ObjectNearFocus_cr(cr));
    w_ObjectPoseFromRecord(cr);
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

void abi_ObjectCarFrame(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarFrame(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_CrashWhite(void);

/*
 * @routine 08:B292 CarSpawnCrash
 * WHAT IT DOES: The crash screen ObjectCarSpawnPlace branches to when the
 * car has no record: CrashWhite.
 * HOW: JSL to CrashWhite, which never returns.
 * Entry: any widths; D = $0000. Exit: never returns.
 */
void CarSpawnCrash(Carried *cr)
{
    (cyc(62), CrashWhite_cr(cr));
}

void abi_CarSpawnCrash(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CarSpawnCrash(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B1FC ObjectCarSpawnPlace
 * WHAT IT DOES: ObjectCarSpawn after the record is made: with no record
 * (+$22 = 0, which ObjectCarRecordNew never leaves: it returns before it
 * would) the crash screen at 08:B292; else the record gets +$64 bit 3 and
 * +$40 bit 9, the object's handler (+$1A) becomes ObjectCarFrame, +$1C = 8,
 * its kind's value set (ObjectSet5DByKind with +$49), and the first frame
 * runs (ObjectCarFrame).
 * HOW: The record through long accesses with X = +$22, the object's X kept.
 * Entry: M 16-bit, X 16-bit, data bank $7E; X = the object. Exit: as
 * ObjectCarFrame.
 */
void ObjectCarSpawnPlace(Carried *cr)
{
    uint16_t obj;
    uint32_t r;

    r_rep(cr, P_M);
    r_lda16(cr, cyc_then(30, car_field(cr, 0x2022)));
    if (!cr->a) {
        cyc(46);
        CarSpawnCrash(cr);
        return;
    }
    obj = cr->x;
    r_tax16(cr);
    r = 0x700000u + cr->x;
    cyc(68);
    cr->a = (uint16_t)(rd16(r + 0x64) | 0x0008);
    set_p(cr, nz16(cr->p, cr->a));
    cyc(56);
    wr16(r + 0x64, cr->a);
    cyc(32);
    cr->a = (uint16_t)(rd16(r + 0x40) | 0x0200);
    set_p(cr, nz16(cr->p, cr->a));
    cyc(56);
    wr16(r + 0x40, cr->a);
    cyc(32);
    cr->a = (uint16_t)(rd16(r + 0x70) | 0x0002);
    set_p(cr, nz16(cr->p, cr->a));
    cr->x = obj;
    set_p(cr, nz16(cr->p, obj));
    r_sep(cr, P_M);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0xB23D);
    wr16(car_field(cr, 0x201A), cyc_then(98, 0xB23D));
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x08);
    set_p(cr, nz8(cr->p, 0x08));
    wr8(car_field(cr, 0x201C), cyc_then(68, 0x08));
    r_lda8(cr, cyc_then(30, car_field(cr, 0x2049)));
    ObjectSet5DByKind_cr(cr);
    (cyc(0), ObjectCarFrame(cr));
}

void abi_ObjectCarSpawnPlace(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarSpawnPlace(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B1E5 ObjectCarSpawn
 * WHAT IT DOES: Starts object X as a car: raises it ($200E - $1E0), makes
 * its Super FX record (ObjectCarRecordNew, A = 4), then as
 * ObjectCarSpawnPlace.
 * HOW: 16-bit for the position; JSL to the record's maker.
 * Entry: M 8-bit, X 16-bit, data bank $7E; X = the object. Exit: as
 * ObjectCarFrame, or the crash screen.
 */
void ObjectCarSpawn(Carried *cr)
{
    r_rep(cr, P_M);
    r_lda16(cr, cyc_then(30, car_field(cr, 0x200E)));
    r_add16(cr, 0xFE20);
    wr16(car_field(cr, 0x200E), cyc_then(68, cr->a));
    r_sep(cr, P_M);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0x0004);
    (cyc(70), ObjectCarRecordNew(cr));
    (cyc(0), ObjectCarSpawnPlace(cr));
}

void abi_ObjectCarSpawn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarSpawn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B99A ObjectCarRecordTake
 * WHAT IT DOES: ObjectCarRecordNew after the program: the record it made
 * ($70:238E; none: a return by the RTL at 08:A033, which nothing reaches,
 * the program always finding one) goes to +$22, $70:2390 to +$04, $70:2394
 * to $1F58 and with bit 7 to +$18, +$24 bit 4, then 08:BA57 and 08:DCB4,
 * +$46 = 3 and +$4D = 1.
 * HOW: 16-bit for the words, 8-bit for the flags.
 * Entry: M 16-bit, X 16-bit, data bank $7E; X = the object. Exit: M 8-bit;
 * A = 1; X, Y as 08:DCB4 left them.
 */
void ObjectCarRecordTake(Carried *cr)
{
    uint8_t v;

    r_rep(cr, P_M);
    cyc(32);
    r_lda16(cr, 0x70238E);
    if (!cr->a) {
        cyc(90);
        /* BRL to the RTL at 08:A033: the routine returns. */
        return;
    }
    wr16(car_field(cr, 0x2022), cyc_then(52, cr->a));
    cyc(32);
    r_lda16(cr, 0x702390);
    wr16(car_field(cr, 0x2004), cyc_then(30, cr->a));
    r_sep(cr, P_M);
    cyc(54);
    r_lda8(cr, 0x702394);
    cyc(24);
    wr8(((uint32_t)cr->db << 16) | 0x1F58, (uint8_t)cr->a);
    cyc(24);
    r_lda8(cr, ((uint32_t)cr->db << 16) | 0x1F58);
    v = (uint8_t)(cr->a | 0x80);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    wr8(car_field(cr, 0x2018), cyc_then(46, v));
    v = (uint8_t)(rd8(cyc_then(30, car_field(cr, 0x2024))) | 0x10);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    wr8(car_field(cr, 0x2024), cyc_then(46, v));
    cyc(62);
    ObjectChildrenSpawn(cr);
    cyc(62);
    ObjectAttachSpawn(cr);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x03);
    set_p(cr, nz8(cr->p, 0x03));
    wr8(car_field(cr, 0x2046), cyc_then(46, 0x03));
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x01);
    set_p(cr, nz8(cr->p, 0x01));
    wr8(car_field(cr, 0x204D), cyc_then(46, 0x01));
    r_sep(cr, P_M);
    cyc(66);
}

void abi_ObjectCarRecordTake(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarRecordTake(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B956 ObjectCarRecordNew
 * WHAT IT DOES: Makes object X's Super FX car record: amount A at
 * $70:2390, the position (+$0C, +$0E, +$10) at $70:0068, $70:002C and
 * $70:002E, +$15 as the high byte at $70:0022, +$4A at $70:2392 and +$27 at
 * $70:238E, then Super FX program $04:8CC6; then as ObjectCarRecordTake.
 * HOW: 16-bit for the words; X and Y kept around the program.
 * Entry: M 16-bit, X 16-bit, data bank $7E; A = amount, X = the object.
 * Exit: as ObjectCarRecordTake.
 */
void ObjectCarRecordNew(Carried *cr)
{
    uint16_t x = cr->x, y = cr->y;

    cyc(32);
    wr16(0x702390, cr->a);
    r_lda16(cr, cyc_then(30, car_field(cr, 0x200C)));
    cyc(32);
    wr16(0x700068, cr->a);
    r_lda16(cr, cyc_then(30, car_field(cr, 0x200E)));
    cyc(32);
    wr16(0x70002C, cr->a);
    r_lda16(cr, cyc_then(30, car_field(cr, 0x2010)));
    cyc(32);
    wr16(0x70002E, cr->a);
    r_lda16(cr, cyc_then(30, car_field(cr, 0x2015)));
    r_and16(cr, 0x00FF);
    r_xba(cr);
    cyc(74);
    wr16(0x700022, cr->a);
    r_lda16(cr, cyc_then(30, car_field(cr, 0x204A)));
    r_and16(cr, 0x00FF);
    cyc(54);
    wr16(0x702392, cr->a);
    r_lda16(cr, cyc_then(30, car_field(cr, 0x2027)));
    cyc(32);
    wr16(0x70238E, cr->a);
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x04);
    set_p(cr, nz8(cr->p, 0x04));
    cr->x = 0x8CC6;
    set_p(cr, nz16(cr->p, cr->x));
    (cyc(184), w_GsuRun(cr));
    cr->y = y;
    cr->x = x;
    set_p(cr, nz16(cr->p, x));
    (cyc(72), ObjectCarRecordTake(cr));
}

void abi_ObjectCarRecordNew(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarRecordNew(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_GsuRecordFromObject(void);
void abi_ObjectChildrenSpawn(void);
void abi_ObjectPoseFromRecord(void);
void abi_GsuRecordFlag4(void);

/*
 * @routine 08:B9DD ObjectCarRecordRestart
 * WHAT IT DOES: Runs the car record program (Super FX 04:8CC6) again on
 * the inputs already in Super FX RAM, then takes what it made: $70:2390 to
 * +$04 and the record ($70:238E; none: a return by the RTL at 08:A033) to
 * +$22; the object's position (+$0C/+$0E/+$10) and heading (+$15 as the
 * high byte) go to the record's +$1E/+$20/+$22/+$24; +$53 = $8C, +$55 =
 * $3C, +$57 = $8C; then GsuRecordFromObject and ObjectChildrenSpawn. No
 * play reaches it.
 * HOW: GsuRun with A = $04, X = $8CC6, X and Y pushed round it; the
 * object in Y and the record in X while copying.
 * Entry: X 16-bit, data bank $7E; X = the object. Exit: as
 * ObjectChildrenSpawn.
 */
void ObjectCarRecordRestart(Carried *cr)
{
    uint16_t x = cr->x, y = cr->y, rec, v;

    r_rep(cr, P_M);
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x04);
    set_p(cr, nz8(cr->p, 0x04));
    cr->x = 0x8CC6;
    set_p(cr, nz16(cr->p, cr->x));
    (cyc(184), w_GsuRun(cr));
    cr->y = y;
    cr->x = x;
    set_p(cr, nz16(cr->p, x));
    r_rep(cr, P_M);
    cyc(104);
    r_lda16(cr, 0x702390);
    wr16(car_field(cr, 0x2004), cyc_then(30, cr->a));
    r_sep(cr, P_M);
    r_rep(cr, P_M);
    cyc(54);
    r_lda16(cr, 0x70238E);
    if (!cr->a) {
        cyc(46 + 44);
        /* BRL to the RTL at 08:A033: the routine returns. */
        return;
    }
    wr16(car_field(cr, 0x2022), cyc_then(52, cr->a));
    cr->y = x;
    cyc(46);
    rec = rd16(0x70238E);
    v = rd16(cyc_then(44, car_field(cr, 0x200C)));
    cyc(32);
    wr16(0x70001E + rec, v);
    v = rd16(cyc_then(30, car_field(cr, 0x200E)));
    cyc(32);
    wr16(0x700020 + rec, v);
    v = rd16(cyc_then(30, car_field(cr, 0x2010)));
    cyc(32);
    wr16(0x700022 + rec, v);
    v = rd16(cyc_then(30, car_field(cr, 0x2015)));
    v = (uint16_t)((v & 0x00FF) << 8);
    cyc(76);
    wr16(0x700024 + rec, v);
    cr->a = v;
    r_sep(cr, P_M);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0x008C);
    wr16(car_field(cr, 0x2053), cyc_then(52, 0x008C));
    r_sep(cr, P_M);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0x003C);
    wr16(car_field(cr, 0x2055), cyc_then(38, 0x003C));
    r_sep(cr, P_M);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0x008C);
    wr16(car_field(cr, 0x2057), cyc_then(38, 0x008C));
    r_sep(cr, P_M);
    w_GsuRecordFromObject(cr);
    (cyc(62), ObjectChildrenSpawn(cr));
    cyc(44);                            /* RTL */
}

void abi_ObjectCarRecordRestart(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarRecordRestart(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B2DE ObjectCarPoseStep
 * WHAT IT DOES: The handler of the car ObjectCarRecordSetup makes: each
 * frame it is rebucketed, posed from its record (ObjectPoseFromRecord) and
 * the record marked (GsuRecordFlag4).
 * HOW: ObjectRebucket, ObjectPoseFromRecord, then JMP to $08:CD7E (JSL
 * GsuRecordFlag4, RTL).
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as GsuRecordFlag4.
 */
void ObjectCarPoseStep(Carried *cr)
{
    (cyc(62), ObjectRebucket_cr(cr));
    (cyc(62), w_ObjectPoseFromRecord(cr));
    (cyc(86), w_GsuRecordFlag4(cr));
    cyc(44);                            /* RTL */
}

void abi_ObjectCarPoseStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarPoseStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B296 ObjectCarRecordSetup
 * WHAT IT DOES: Sets up a car-like object: $A0 higher ($FF60 on +$0E), a
 * car record made with amount 4 (ObjectCarRecordNew), and when it has one
 * its record's +$64 bit 3 and +$70 bit 2 set; handler $08:B2DE
 * (ObjectCarPoseStep), which it runs at once. No course in the scripts
 * places it.
 * HOW: 16-bit adds and ORs in place; falls into ObjectCarPoseStep.
 * Entry: M 8-bit, X 16-bit, data bank $7E; X = object. Exit: as
 * ObjectCarPoseStep.
 */
void ObjectCarRecordSetup(Carried *cr)
{
    uint16_t rec, v;

    r_rep(cr, P_M);
    r_lda16(cr, cyc_then(30, car_field(cr, 0x200E)));
    r_add16(cr, 0xFF60);
    wr16(car_field(cr, 0x200E), cyc_then(68, cr->a));
    r_sep(cr, P_M);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0x0004);
    (cyc(70), ObjectCarRecordNew(cr));
    r_rep(cr, P_M);
    r_lda16(cr, cyc_then(30, car_field(cr, 0x2022)));
    rec = cr->a;
    if (rec) {
        cyc(92);
        v = (uint16_t)(rd16(0x700064 + rec) | 0x0008);
        cr->a = v;
        set_p(cr, nz16(cr->p, v));
        cyc(56);
        wr16(0x700064 + rec, v);
        cyc(32);
        v = (uint16_t)(rd16(0x700070 + rec) | 0x0004);
        cr->a = v;
        cyc(56);
        wr16(0x700070 + rec, v);
        set_p(cr, nz16(cr->p, cr->x));
    }
    r_sep(cr, P_M);
    r_rep(cr, P_M);
    r_lda16_imm(cr, 0xB2DE);
    wr16(car_field(cr, 0x201A), cyc_then(74, 0xB2DE));
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x08);
    set_p(cr, nz8(cr->p, 0x08));
    wr8(car_field(cr, 0x201C), cyc_then(68, 0x08));
    ObjectCarPoseStep(cr);
}

void abi_ObjectCarRecordSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarRecordSetup(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:EC8B ObjectTimedStep
 * WHAT IT DOES: The handler ObjectTimedSetup installs: rebuckets object X;
 * when its count +$4B is 0 it is released (ObjectReleaseLong), otherwise
 * the count goes down by one.
 * HOW: ObjectRebucket, an 8-bit test, the release a long jump, DEC.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: counting: A as ObjectRebucket
 * left it, N, Z from the new count; done: as ObjectReleaseLong.
 */
void ObjectTimedStep(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint8_t v;

    (cyc(62), ObjectRebucket_cr(cr));
    cyc(30);
    v = rd8(b | (uint16_t)(0x204B + cr->x));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    if (!v) {
        cyc(48);
        ObjectReleaseLong(cr);
        return;
    }
    cyc(52);
    v = (uint8_t)(rd8(b | (uint16_t)(0x204B + cr->x)) - 1);
    cyc(6);
    wr8(b | (uint16_t)(0x204B + cr->x), v);
    set_p(cr, nz8(cr->p, v));
    cyc(44);                            /* RTL */
}

void abi_ObjectTimedStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTimedStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:EC77 ObjectTimedSetup
 * WHAT IT DOES: Sets up an object that stays for 21 frames: handler
 * $08:EC8B (ObjectTimedStep), count +$4B = $14, then its first frame. No
 * course in the scripts places it.
 * HOW: Stores, then falls into ObjectTimedStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectTimedStep.
 */
void ObjectTimedSetup(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    r_rep(cr, P_M);
    r_lda16_imm(cr, 0xEC8B);
    cyc(16);
    wr16(b | (uint16_t)(0x201A + cr->x), 0xEC8B);
    r_sep(cr, P_M);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x08);
    set_p(cr, nz8(cr->p, 0x08));
    cyc(68);
    wr8(b | (uint16_t)(0x201C + cr->x), 0x08);
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x14);
    set_p(cr, nz8(cr->p, 0x14));
    cyc(46);
    wr8(b | (uint16_t)(0x204B + cr->x), 0x14);
    ObjectTimedStep(cr);
}

void abi_ObjectTimedSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTimedSetup(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* REP #$20, LDA #h, STA +$1A, SEP #$20, LDA #$08, STA +$1C: object o's
   handler, through the data bank. */
static void set_handler(Carried *cr, uint16_t o, uint16_t h)
{
    uint32_t b = (uint32_t)cr->db << 16;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16_imm(cr, h);
    cyc(38);
    wr16(b | (uint16_t)(0x201A + o), h);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | 0x08);
    set_p(cr, nz8(cr->p, 0x08));
    cyc(68);
    wr8(b | (uint16_t)(0x201C + o), 0x08);
}

/* LDY +off,X: one of object X's children. */
static void child(Carried *cr, uint16_t off)
{
    cyc(30);
    cr->y = rd16(((uint32_t)cr->db << 16) | (uint16_t)(off + cr->x));
    set_p(cr, nz16(cr->p, cr->y));
}

/* ObjectReleaseWhenPassed's release (08:B6C7): the handler $08:F927 for
   the objects in +$2E, +$30, +$32 (twice), +$34 and for X itself. */
static void release(Carried *cr)
{
    cyc(84);
    child(cr, 0x202E);
    set_handler(cr, cr->y, 0xF927);
    child(cr, 0x2030);
    set_handler(cr, cr->y, 0xF927);
    child(cr, 0x2032);
    set_handler(cr, cr->y, 0xF927);
    set_handler(cr, cr->y, 0xF927);
    child(cr, 0x2034);
    set_handler(cr, cr->y, 0xF927);
    set_handler(cr, cr->x, 0xF927);
    cyc(44);                            /* RTL */
}

/* ObjectReleaseWhenPassed from 08:B6BC: the distance in A against $05DC;
   at or past it, the release. */
static void passed_tail(Carried *cr, unsigned extra)
{
    r_cmp16(cr, 0x05DC);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (!(cr->p & P_N))
        release(cr); else { cyc(110 + extra + 44); }  /* JML to the RTL, RTL */
}

/*
 * @routine 08:B680 ObjectReleaseWhenPassed
 * WHAT IT DOES: Gives object X and the objects in its +$2E, +$30, +$32
 * and +$34 the handler $08:F927 when the player ($0346) is $1F40 or more
 * from it along +$10, or $05DC or more past it; otherwise nothing.
 * Nothing in the game calls it; a direct call is its only way in.
 * HOW: |player - object| (EOR #$FFFF, INC) compared signed; the second
 * test only when the object is behind the player (CMP, BMI), so its
 * distance needs no negation (that EOR is ObjectReleaseWhenPassedBehind).
 * The +$32 object gets the handler twice.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X unchanged; Y =
 * the player, or the +$34 object after a release; A and the flags as the
 * last step left them.
 */
void ObjectReleaseWhenPassed(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;

    cyc(24);
    cr->y = rd16(b | 0x0346);
    set_p(cr, nz16(cr->p, cr->y));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, b | (uint16_t)(0x2010 + cr->y));
    cyc(44);
    r_sub16(cr, rd16(b | (uint16_t)(0x2010 + cr->x)));
    if (cr->p & P_N) {
        cyc(32);
        cr->a = (uint16_t)((cr->a ^ 0xFFFF) + 1);
        set_p(cr, nz16(cr->p, cr->a));
    }
    cyc(26);
    r_cmp16(cr, 0x1F40);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (!(cr->p & P_N)) {
        release(cr);
        return;
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(110);
    r_lda16(cr, b | (uint16_t)(0x2010 + cr->x));
    cyc(30);
    r_cmp16(cr, rd16(b | (uint16_t)(0x2010 + cr->y)));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (!(cr->p & P_N))
        { cyc(114); return; }           /* JML to the RTL, RTL */
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(96);
    r_lda16(cr, b | (uint16_t)(0x2010 + cr->y));
    cyc(44);
    r_sub16(cr, rd16(b | (uint16_t)(0x2010 + cr->x)));
    if (cr->p & P_N) {
        cr->a = (uint16_t)((cr->a ^ 0xFFFF) + 1);
        set_p(cr, nz16(cr->p, cr->a));
        passed_tail(cr, 32);                /* BPL not taken, EOR, INC */
    } else
        passed_tail(cr, 0);
}

void abi_ObjectReleaseWhenPassed(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectReleaseWhenPassed(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B6B8 ObjectReleaseWhenPassedBehind
 * WHAT IT DOES: ObjectReleaseWhenPassed's negation of the second distance,
 * which no case reaches (the object is behind the player there, so the
 * distance is positive): A negated, then as ObjectReleaseWhenPassed from
 * its $05DC test. A direct call is its only way in.
 * HOW: EOR #$FFFF, INC; then the test and the release.
 * Entry: M 16-bit, X 16-bit; A = the distance; X = object. Exit: as
 * ObjectReleaseWhenPassed.
 */
void ObjectReleaseWhenPassedBehind(Carried *cr)
{
    cr->a = (uint16_t)((cr->a ^ 0xFFFF) + 1);
    set_p(cr, nz16(cr->p, cr->a));
    (cyc(22), passed_tail(cr, 0));
}

void abi_ObjectReleaseWhenPassedBehind(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectReleaseWhenPassedBehind(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B782 ObjectListRemove
 * WHAT IT DOES: Takes object X out of the list at $7E:FFBD (words ending
 * with 0; its length in bytes at $7E:FFFD): its slot gets the list's last
 * word, which is cleared, and the length goes down by 2 (from 2 when it
 * was 0). An object not in the list changes nothing. Nothing in the game
 * calls it; a direct call is its only way in.
 * HOW: The object in $1F2B; a 16-bit search with X; X and Y kept on the
 * stack.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X and Y unchanged,
 * N and Z from X; A as the search or the new length left it.
 */
void ObjectListRemove(Carried *cr)
{
    uint32_t b = (uint32_t)cr->db << 16;
    uint16_t x0 = cr->x, y0 = cr->y, slot;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(106);
    wr16(b | 0x1F2B, cr->x);
    cr->x = 0;
    set_p(cr, nz16(cr->p, 0));
    for (;;) {
        cyc(56);
        r_lda16(cr, 0x7EFFBD + cr->x);
        if (!cr->a)
            { cyc(22); goto done; }
        cyc(40);
        r_cmp16(cr, rd16(b | 0x1F2B));
        if (cr->p & P_Z)
            break; else { cyc(42); }
        cr->x = (uint16_t)(cr->x + 2);
        set_p(cr, nz16(cr->p, cr->x));
    }
    slot = cr->x;
    cyc(84);
    r_lda16(cr, 0x7EFFFD);
    r_tax16(cr);
    cyc(16);
    r_lda16(cr, 0x7EFFBB + cr->x);
    cr->x = slot;
    set_p(cr, nz16(cr->p, cr->x));
    cyc(68);
    wr16(0x7EFFBD + cr->x, cr->a);
    cyc(32);
    r_lda16(cr, 0x7EFFFD);
    if (!cr->a) {
        cyc(22);
        cr->a = 2;
        set_p(cr, nz16(cr->p, cr->a));
    }
    r_tax16(cr);
    r_lda16_imm(cr, 0x0000);
    cyc(24);
    wr16(0x7EFFBB + cr->x, 0x0000);
    cr->x = (uint16_t)(cr->x - 2);
    set_p(cr, nz16(cr->p, cr->x));
    r_txa16(cr);
    cyc(44);
    wr16(0x7EFFFD, cr->a);
done:
    cyc(138);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->y = y0;
    cr->x = x0;
    set_p(cr, nz16(cr->p, cr->x));
}

void abi_ObjectListRemove(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectListRemove(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B7C6 ObjectListClear
 * WHAT IT DOES: Empties the list at $7E:FFBD: its length $7E:FFFD and its
 * first word 0. Nothing in the game calls it; a direct call is its only
 * way in.
 * HOW: 16-bit stores of 0.
 * Entry: M 8-bit. Exit: M 8-bit; A = 0, Z set.
 */
void ObjectListClear(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16_imm(cr, 0x0000);
    cyc(40);
    wr16(0x7EFFFD, 0x0000);
    cyc(32);
    wr16(0x7EFFBD, 0x0000);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(66);                            /* SEP, RTL */
}

void abi_ObjectListClear(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectListClear(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 08:B846 PlayerLinkSet27
 * WHAT IT DOES: For A = 1 or 2, makes object X the +$27 of that player's
 * object ($0346 or $034A); any other A changes nothing. Nothing in the
 * game calls it; a direct call is its only way in.
 * HOW: SEP #$20, REP #$10; CMP #1, CMP #2; the store long, at $7E:2027.
 * Entry: X = object; A = the player. Exit: M 8-bit, X 16-bit; Y = player
 * 1's object, or player 2's for A = 2; for 1 or 2, X = that object and A
 * = the object stored; the flags as the last step left them.
 */
void PlayerLinkSet27(Carried *cr)
{
    uint32_t b;
    uint8_t a;

    r_sep(cr, P_M);
    r_rep(cr, P_X);
    b = (uint32_t)cr->db << 16;
    cyc(46);
    cr->y = rd16(b | 0x0346);
    set_p(cr, nz16(cr->p, cr->y));
    a = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(a - 0x01)) & ~P_C) | (a >= 0x01 ? (cyc(38), P_C) : 0)));
    if (a != 0x01) {
        set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(a - 0x02)) & ~P_C) | (a >= 0x02 ? P_C : 0)));
        if (a != 0x02)
            { cyc(76); return; }        /* BNE taken; RTL */
        cyc(50);
        cr->y = rd16(b | 0x034A);
        set_p(cr, nz16(cr->p, cr->y));
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_txa16(cr);
    cr->x = cr->y;
    set_p(cr, nz16(cr->p, cr->x));
    cyc(52);
    wr16(0x7E2027 + cr->x, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(66);                            /* SEP, RTL */
}

void abi_PlayerLinkSet27(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PlayerLinkSet27(&r);
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

/* ObjectPoseFromRecord as its adapter leaves the registers. */
static void w_ObjectPoseFromRecord(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ObjectPoseFromRecord(cr->db, cr->x, &p);
    cr->y = cr->x;
    set_p(cr, (uint8_t)(p | P_M));
}

/* GsuRun as its adapter leaves the registers. */
static void w_GsuRun(Carried *cr)
{
    uint8_t mode = GsuRun((uint8_t)cr->a, cr->x);

    cr->a = (uint16_t)((cr->a & 0xFF00) | mode);
    set_p(cr, nz8(cr->p, cr->db));
    cyc(72);                            /* PLB, RTL */
}

/* GsuRecordFromObject as its adapter leaves the registers. */
static void w_GsuRecordFromObject(Carried *cr)
{
    cr->a = GsuRecordFromObject(cr->db, cr->x);
    cr->y = cr->x;
    set_p(cr, (uint8_t)(nz16(cr->p, cr->x) | P_M));
}

/* GsuRecordFlag4 as its adapter leaves the registers. */
static void w_GsuRecordFlag4(Carried *cr)
{
    int has;
    uint16_t a = GsuRecordFlag4(cr->db, cr->x, &has);

    if (has) {
        cyc(8);
        cr->a = a;
        cr->y = cr->x;
        set_p(cr, (uint8_t)((cr->p & ~P_N) | P_Z | P_M));
    } else {
        cyc(8);
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0xFF);
        set_p(cr, (uint8_t)((cr->p & ~P_Z) | P_N | P_M));
    }
    cyc(88);
}
