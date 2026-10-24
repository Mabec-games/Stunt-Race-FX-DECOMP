/*
 * objects_09.c: the course objects' handlers of bank $09: the set-up a
 * course's object list points at (a long pointer per object kind in the
 * course data) and the per-frame handlers they install at +$1A/+$1C (run
 * by ObjectRunHandlerA). Most start with a NOP, as the course data's
 * pointers expect.
 *
 * Objects are at $2000 + X through the data bank (X 16-bit, M 8-bit).
 */
#include "machine.h"
#include "math_calls.h"
#include "cpu_ops.h"
#include "carried.h"
#include "wram_names.h"


void abi_ObjectSpawn(void);
void abi_ObjectCopyPositionTo(void);
void abi_ObjectRebucket(void);
void abi_ObjectSyncFromRecord(void);
void abi_RecordContactSummary(void);
void abi_ObjectRelease(void);
void abi_GsuRecordNew42D(void);
void abi_GsuRecordFlag10(void);
void abi_GsuRecordPosition(void);
void abi_GsuRecordVelocity(void);
void abi_SoundPort0QueuePush(void);

void BreakablePairA(Carried *cr);
void BreakablePairB(Carried *cr);
void BreakablePairC(Carried *cr);
void BreakablePairD(Carried *cr);
void GateFadeUp(Carried *cr);
void GateFlashOnClock(Carried *cr);
void GateSlideStep(Carried *cr);
void LampPostBurst(Carried *cr);
void LampPostHit(Carried *cr);
void LapCrowdMove0(Carried *cr);
void LapCrowdMove1(Carried *cr);
void LapCrowdMove2(Carried *cr);
void LapCrowdSpawn0(Carried *cr);
void LapCrowdSpawn1(Carried *cr);
void LapCrowdSpawn2(Carried *cr);
void LapCrowdStep0(Carried *cr);
void LapCrowdStep1(Carried *cr);
void LapCrowdStep2(Carried *cr);
void LapSignSpawn0(Carried *cr);
void LapSignSpawn1(Carried *cr);
void LapSignSpawn2(Carried *cr);
void LapSignStep0(Carried *cr);
void LapSignStep1(Carried *cr);
void LapSignStep2(Carried *cr);
void MarkerBounceX(Carried *cr);
void MarkerBounceY(Carried *cr);
void ObjectBallDrop(Carried *cr);
void ObjectBallDropRecord(Carried *cr);
void ObjectBallDropStep(Carried *cr);
void ObjectBallFollowPlayer(Carried *cr);
void ObjectBeepGate10(Carried *cr);
void ObjectBeepGate10Record(Carried *cr);
void ObjectBeepGate10Step(Carried *cr);
void ObjectBeepGate11(Carried *cr);
void ObjectBeepGate11Record(Carried *cr);
void ObjectBeepGate11Step(Carried *cr);
void ObjectBellGate(Carried *cr);
void ObjectBellGateRecord(Carried *cr);
void ObjectBellGateStep(Carried *cr);
void ObjectBob(Carried *cr);
void ObjectBobStep(Carried *cr);
void ObjectBonusEndGate(Carried *cr);
void ObjectBonusEndGateRecord(Carried *cr);
void ObjectBonusEndGateStep(Carried *cr);
void ObjectBonusGate(Carried *cr);
void ObjectBonusGateRecord(Carried *cr);
void ObjectBonusGateStep(Carried *cr);
void ObjectBonusMarker(Carried *cr);
void ObjectBonusMarker2(Carried *cr);
void ObjectBonusMarker2Record(Carried *cr);
void ObjectBonusMarker2Step(Carried *cr);
void ObjectBonusMarker5(Carried *cr);
void ObjectBonusMarker5Record(Carried *cr);
void ObjectBonusMarker5Step(Carried *cr);
void ObjectBonusMarkerRecord(Carried *cr);
void ObjectBonusMarkerStep(Carried *cr);
void ObjectBreakable(Carried *cr);
void ObjectBreakableBurst(Carried *cr);
void ObjectBreakableBurst2(Carried *cr);
void ObjectBreakableGate(Carried *cr);
void ObjectBreakableGate2(Carried *cr);
void ObjectBreakableGate2Record(Carried *cr);
void ObjectBreakableGate2Step(Carried *cr);
void ObjectBreakableGateRecord(Carried *cr);
void ObjectBreakableGateStep(Carried *cr);
void ObjectBreakablePiece(Carried *cr);
void ObjectBreakablePieceStep(Carried *cr);
void ObjectBreakableRecord(Carried *cr);
void ObjectBreakableStep(Carried *cr);
void ObjectBrightGate(Carried *cr);
void ObjectBrightGateRecord(Carried *cr);
void ObjectBrightGateStep(Carried *cr);
void ObjectBumpPost(Carried *cr);
void ObjectBumpPostRecord(Carried *cr);
void ObjectBumpPostStep(Carried *cr);
void ObjectBumper(Carried *cr);
void ObjectBumperRecord(Carried *cr);
void ObjectBumperStep(Carried *cr);
void ObjectBurstGate(Carried *cr);
void ObjectBurstGateRecord(Carried *cr);
void ObjectBurstGateStep(Carried *cr);
void ObjectCannonBallFlyLeft(Carried *cr);
void ObjectCannonBallFlyRight(Carried *cr);
void ObjectCannonBallHome(Carried *cr);
void ObjectCannonBallLeft(Carried *cr);
void ObjectCannonBallRight(Carried *cr);
void ObjectCannonBallsLeft(Carried *cr);
void ObjectCannonBallsRight(Carried *cr);
void ObjectCannonGate(Carried *cr);
void ObjectCannonGateRecord(Carried *cr);
void ObjectCannonGateStep(Carried *cr);
void ObjectCarFlagOffGate(Carried *cr);
void ObjectCarFlagOffGateRecord(Carried *cr);
void ObjectCarFlagOffGateStep(Carried *cr);
void ObjectCarFlagOnGate(Carried *cr);
void ObjectCarFlagOnGateRecord(Carried *cr);
void ObjectCarFlagOnGateStep(Carried *cr);
void ObjectChimeGate(Carried *cr);
void ObjectChimeGateRecord(Carried *cr);
void ObjectChimeGateStep(Carried *cr);
void ObjectClear25Bit0(Carried *cr);
void ObjectColour0(Carried *cr);
void ObjectColour1(Carried *cr);
void ObjectColour2(Carried *cr);
void ObjectColour3(Carried *cr);
void ObjectColour4(Carried *cr);
void ObjectColour5(Carried *cr);
void ObjectColour6(Carried *cr);
void ObjectColour7(Carried *cr);
void ObjectCountGate(Carried *cr);
void ObjectCountGateRecord(Carried *cr);
void ObjectCountGateStep(Carried *cr);
void ObjectCountMarker(Carried *cr);
void ObjectCountMarkerRecord(Carried *cr);
void ObjectCountMarkerStep(Carried *cr);
void ObjectCountTrigger(Carried *cr);
void ObjectCountTriggerRecord(Carried *cr);
void ObjectCountTriggerStep(Carried *cr);
void ObjectCountUpTo2(Carried *cr);
void ObjectDarkGate(Carried *cr);
void ObjectDarkGateRecord(Carried *cr);
void ObjectDarkGateStep(Carried *cr);
void ObjectDuoGate(Carried *cr);
void ObjectDuoGateRecord(Carried *cr);
void ObjectDuoGateStep(Carried *cr);
void ObjectFadeGate0(Carried *cr);
void ObjectFadeGate0Record(Carried *cr);
void ObjectFadeGate0Step(Carried *cr);
void ObjectFadeGate1(Carried *cr);
void ObjectFadeGate1Record(Carried *cr);
void ObjectFadeGate1Step(Carried *cr);
void ObjectFadeGate2(Carried *cr);
void ObjectFadeGate2Record(Carried *cr);
void ObjectFadeGate2Step(Carried *cr);
void ObjectFan(Carried *cr);
void ObjectFanCentreStep(Carried *cr);
void ObjectFanStep(Carried *cr);
void ObjectFanStepXSign(Carried *cr);
void ObjectFanStepYSign(Carried *cr);
void ObjectFanStepZSign(Carried *cr);
void ObjectFinaleGate(Carried *cr);
void ObjectFinaleGateRecord(Carried *cr);
void ObjectFinaleGateStep(Carried *cr);
void ObjectFinishGate(Carried *cr);
void ObjectFinishGateRecord(Carried *cr);
void ObjectFinishGateStep(Carried *cr);
void ObjectFireworkBurst(Carried *cr);
void ObjectFireworkShell(Carried *cr);
void ObjectFireworkShellPair(Carried *cr);
void ObjectFireworkShellRecord(Carried *cr);
void ObjectFireworkShellStep(Carried *cr);
void ObjectFloaterSpawner(Carried *cr);
void ObjectFloaterSpawnerStep(Carried *cr);
void ObjectFloaterStep(Carried *cr);
void ObjectFlyingPiece(Carried *cr);
void ObjectFlyingPieceRelease(Carried *cr);
void ObjectFlyingPieceStep(Carried *cr);
void ObjectLampLow(Carried *cr);
void ObjectLampPost(Carried *cr);
void ObjectLampPostLight(Carried *cr);
void ObjectLampPostRecord(Carried *cr);
void ObjectLampPostStep(Carried *cr);
void ObjectLampTower(Carried *cr);
void ObjectLantern(Carried *cr);
void ObjectLanternBurst(Carried *cr);
void ObjectLanternRecord(Carried *cr);
void ObjectLanternStep(Carried *cr);
void ObjectLapBallDown(Carried *cr);
void ObjectLapBallDownRecord(Carried *cr);
void ObjectLapBallDownStep(Carried *cr);
void ObjectLapBallUp(Carried *cr);
void ObjectLapBallUpRecord(Carried *cr);
void ObjectLapBallUpStep(Carried *cr);
void ObjectLapCrowdGate(Carried *cr);
void ObjectLapCrowdGateRecord(Carried *cr);
void ObjectLapCrowdGateStep(Carried *cr);
void ObjectLapFlashGate(Carried *cr);
void ObjectLapFlashGateRecord(Carried *cr);
void ObjectLapFlashGateStep(Carried *cr);
void ObjectLapLight(Carried *cr);
void ObjectLapLightLeft(Carried *cr);
void ObjectLapLightRecord(Carried *cr);
void ObjectLapLightRight(Carried *cr);
void ObjectLapLightStep(Carried *cr);
void ObjectLapSignGate(Carried *cr);
void ObjectLapSignGateRecord(Carried *cr);
void ObjectLapSignGateStep(Carried *cr);
void ObjectPlatform(Carried *cr);
void ObjectPlatformRecord(Carried *cr);
void ObjectPlatformStep(Carried *cr);
void ObjectPopMarker(Carried *cr);
void ObjectPopMarkerStep(Carried *cr);
void ObjectPostA(Carried *cr);
void ObjectPostARecord(Carried *cr);
void ObjectPostAStep(Carried *cr);
void ObjectPostB(Carried *cr);
void ObjectPostBRecord(Carried *cr);
void ObjectPostBStep(Carried *cr);
void ObjectPuffFollow(Carried *cr);
void ObjectRiderSpawn(Carried *cr);
void ObjectRiderStep(Carried *cr);
void ObjectRing(Carried *cr);
void ObjectRingCentreStep(Carried *cr);
void ObjectRingStep(Carried *cr);
void ObjectRingStepXSign(Carried *cr);
void ObjectRingStepYSign(Carried *cr);
void ObjectRingStepZSign(Carried *cr);
void ObjectRisingSpark(Carried *cr);
void ObjectScale20(Carried *cr);
void ObjectScale40(Carried *cr);
void ObjectScaleA0(Carried *cr);
void ObjectScaleE0(Carried *cr);
void ObjectScaleUp6(Carried *cr);
void ObjectScoreCounterSlide(Carried *cr);
void ObjectScoreCounterStep(Carried *cr);
void ObjectScoreCounterTick(Carried *cr);
void ObjectScoreGate(Carried *cr);
void ObjectScoreGateRecord(Carried *cr);
void ObjectScoreGateStep(Carried *cr);
void ObjectShardStep(Carried *cr);
void ObjectShatter(Carried *cr);
void ObjectShatter2(Carried *cr);
void ObjectShatter2Shard1XSign(Carried *cr);
void ObjectShatter2Shard1YSign(Carried *cr);
void ObjectShatter2Shard2XSign(Carried *cr);
void ObjectShatter2Shard2YSign(Carried *cr);
void ObjectShatter2Shard3XSign(Carried *cr);
void ObjectShatter2Shard3YSign(Carried *cr);
void ObjectShatter2Shard4XSign(Carried *cr);
void ObjectShatter2Shard4YSign(Carried *cr);
void ObjectShatter2Shard5XSign(Carried *cr);
void ObjectShatter2Shard5YSign(Carried *cr);
void ObjectShatter2Shard6XSign(Carried *cr);
void ObjectShatter2Shard6YSign(Carried *cr);
void ObjectShatterMarker(Carried *cr);
void ObjectShatterMarkerRecord(Carried *cr);
void ObjectShatterMarkerStep(Carried *cr);
void ObjectShatterShard1XSign(Carried *cr);
void ObjectShatterShard1YSign(Carried *cr);
void ObjectShatterShard2XSign(Carried *cr);
void ObjectShatterShard2YSign(Carried *cr);
void ObjectShatterShard3XSign(Carried *cr);
void ObjectShatterShard3YSign(Carried *cr);
void ObjectShatterShard4XSign(Carried *cr);
void ObjectShatterShard4YSign(Carried *cr);
void ObjectShatterShard5XSign(Carried *cr);
void ObjectShatterShard5YSign(Carried *cr);
void ObjectShatterShard6XSign(Carried *cr);
void ObjectShatterShard6YSign(Carried *cr);
void ObjectSkyBallFall(Carried *cr);
void ObjectSkyBallLaunch(Carried *cr);
void ObjectSkyBallRise(Carried *cr);
void ObjectSkyBallStep(Carried *cr);
void ObjectSlideBitGate(Carried *cr);
void ObjectSlideBitGateRecord(Carried *cr);
void ObjectSlideBitGateStep(Carried *cr);
void ObjectSlideBitsGate(Carried *cr);
void ObjectSlideBitsGateRecord(Carried *cr);
void ObjectSlideBitsGateStep(Carried *cr);
void ObjectSlideChimeGate(Carried *cr);
void ObjectSlideChimeGateRecord(Carried *cr);
void ObjectSlideChimeGateStep(Carried *cr);
void ObjectSlideGate(Carried *cr);
void ObjectSlideGateRecord(Carried *cr);
void ObjectSlideGateStep(Carried *cr);
void ObjectSlideGateUp(Carried *cr);
void ObjectSlideGateUpRecord(Carried *cr);
void ObjectSlideGateUpStep(Carried *cr);
void ObjectSlideLapGate(Carried *cr);
void ObjectSlideLapGateRecord(Carried *cr);
void ObjectSlideLapGateStep(Carried *cr);
void ObjectSlideMeter(Carried *cr);
void ObjectSlideMeterStep(Carried *cr);
void ObjectSlideZone(Carried *cr);
void ObjectSlideZoneRecord(Carried *cr);
void ObjectSlideZoneStep(Carried *cr);
void ObjectSparkFollow(Carried *cr);
void ObjectSpinner(Carried *cr);
void ObjectSpinnerCentreStep(Carried *cr);
void ObjectSpinnerStep(Carried *cr);
void ObjectSpinnerStepXSign(Carried *cr);
void ObjectSpinnerStepYSign(Carried *cr);
void ObjectSpinnerStepZSign(Carried *cr);
void ObjectStarSparkleSpawn(Carried *cr);
void ObjectStarSparkleStep(Carried *cr);
void ObjectSweepGate(Carried *cr);
void ObjectSweepGateRecord(Carried *cr);
void ObjectSweepGateStep(Carried *cr);
void ObjectSwing(Carried *cr);
void ObjectSwingStep(Carried *cr);
void ObjectSwitch(Carried *cr);
void ObjectSwitchB(Carried *cr);
void ObjectSwitchBRecord(Carried *cr);
void ObjectSwitchBStep(Carried *cr);
void ObjectSwitchC(Carried *cr);
void ObjectSwitchCRecord(Carried *cr);
void ObjectSwitchCStep(Carried *cr);
void ObjectSwitchLampB(Carried *cr);
void ObjectSwitchLampBRecord(Carried *cr);
void ObjectSwitchLampBStep(Carried *cr);
void ObjectSwitchLampC(Carried *cr);
void ObjectSwitchLampCRecord(Carried *cr);
void ObjectSwitchLampCStep(Carried *cr);
void ObjectSwitchRecord(Carried *cr);
void ObjectSwitchStep(Carried *cr);
void ObjectTimeGate(Carried *cr);
void ObjectTimeGateRecord(Carried *cr);
void ObjectTimeGateSmall(Carried *cr);
void ObjectTimeGateSmallRecord(Carried *cr);
void ObjectTimeGateStep(Carried *cr);
void ObjectTrailLeftStep(Carried *cr);
void ObjectTrailMidStep(Carried *cr);
void ObjectTrailRightStep(Carried *cr);
void ObjectTrailZone(Carried *cr);
void ObjectTrailZoneRecord(Carried *cr);
void ObjectTrailZoneStep(Carried *cr);
void ObjectWaveBallBig(Carried *cr);
void ObjectWaveBallBigRecord(Carried *cr);
void ObjectWaveBallBigSetup(Carried *cr);
void ObjectWaveBallBigStep(Carried *cr);
void ObjectWaveBallHome(Carried *cr);
void ObjectWaveBallLaunchA(Carried *cr);
void ObjectWaveBallLaunchB(Carried *cr);
void ObjectWaveBallLaunchC(Carried *cr);
void ObjectWaveBallStep(Carried *cr);
void ObjectWaveGateA(Carried *cr);
void ObjectWaveGateAStep(Carried *cr);
void ObjectWaveGateB(Carried *cr);
void ObjectWaveGateBStep(Carried *cr);
void ObjectWaveGateC(Carried *cr);
void ObjectWaveGateCStep(Carried *cr);
void ObjectWaveGateRecord(Carried *cr);
void ObjectWheel(Carried *cr);
void ObjectWheelCentreStep(Carried *cr);
void ObjectWheelStep(Carried *cr);
void ObjectWheelStepXSign(Carried *cr);
void ObjectWheelStepYSign(Carried *cr);
void ObjectWheelStepZSign(Carried *cr);
void ObjectWindowOffGate(Carried *cr);
void ObjectWindowOffGateRecord(Carried *cr);
void ObjectWindowOffGateStep(Carried *cr);
void ObjectWindowOnGate(Carried *cr);
void ObjectWindowOnGateRecord(Carried *cr);
void ObjectWindowOnGateStep(Carried *cr);
void ObjectWindowResetGate(Carried *cr);
void ObjectWindowResetGateRecord(Carried *cr);
void ObjectWindowResetGateStep(Carried *cr);
void ObjectWindowSetGate(Carried *cr);
void ObjectWindowSetGateRecord(Carried *cr);
void ObjectWindowSetGateStep(Carried *cr);
void RailDrop(Carried *cr);
void RailDropStep(Carried *cr);
void RailRise(Carried *cr);
void RailRiseStep(Carried *cr);
void RailStep(Carried *cr);
void RailStop(Carried *cr);
void RailsSpawn(Carried *cr);
void TrailFlip(Carried *cr);
static void add16(Carried *cr, uint32_t addr, uint16_t v);
static void add16_via(Carried *cr, uint32_t dst, uint32_t src);
static void add8(Carried *cr, uint32_t addr, uint8_t v);
static void and8(Carried *cr, uint32_t addr, uint8_t m);
static void andor8(Carried *cr, uint32_t addr, uint8_t a, uint8_t o);
static void asl5(Carried *cr, uint32_t at);
static void axis_from(Carried *cr, uint32_t src, uint32_t lo, uint32_t lo2, uint32_t hi);
static void ball_down_frame(Carried *cr);
static void ball_drop(Carried *cr, uint16_t dz, uint16_t dy, uint8_t t);
static void ball_fly(Carried *cr, uint16_t dx);
static void ball_follow(Carried *cr);
static void ball_home(Carried *cr);
static void ball_rise(Carried *cr);
static void ball_up_frame(Carried *cr);
static void beep_frame(Carried *cr, uint8_t snd);
static void bell_frame(Carried *cr);
static void big_ball_frame(Carried *cr);
static void big_ball_record(Carried *cr);
static void big_ball_setup(Carried *cr);
static int bit25(Carried *cr, uint8_t m);
static void bob_frame(Carried *cr);
static void bonus_end_frame(Carried *cr);
static void bonus_gate_frame(Carried *cr);
static void bonus_marker2_frame(Carried *cr, void (*shatter)(Carried *), uint16_t kind, uint8_t bonus);
static void bonus_marker2_init(Carried *cr);
static void bonus_marker_frame(Carried *cr);
static void bounce_colour(Carried *cr, uint16_t lim, uint16_t pos);
static void breakable_frame(Carried *cr);
static void breakable_gate2_frame(Carried *cr);
static void breakable_gate_frame(Carried *cr);
static void breakable_put(Carried *cr, uint16_t dy, uint16_t dz, uint16_t dx, uint8_t s);
static void breakable_record(Carried *cr);
static void brk_09(Carried *cr, uint16_t next);
static void bump_post_frame(Carried *cr);
static void bump_post_record(Carried *cr);
static void bumper_frame(Carried *cr);
static void bumper_record(Carried *cr);
static void burst_gate_frame(Carried *cr);
static void burst_piece(Carried *cr, uint16_t vx, uint16_t vy, uint16_t vz);
static void burst_piece_frame(Carried *cr);
static void burst_piece_k(Carried *cr, uint16_t kind, uint16_t dy, uint16_t vx, uint16_t vy, uint16_t vz);
static void cannon_ball(Carried *cr, uint16_t h, uint16_t dx, uint16_t dy, uint16_t dz, uint8_t s,
                        uint16_t rise, uint16_t drift, uint8_t life, uint8_t home);
static void cannon_ball_frame(Carried *cr, uint16_t dx);
static void cannon_balls_left(Carried *cr);
static void cannon_balls_right(Carried *cr);
static void car_bits16(Carried *cr, uint16_t n, void (*to)(Carried *));
static void car_flag_frame(Carried *cr, int on, uint8_t snd);
static void chime_frame(Carried *cr);
static void cmp8(Carried *cr, uint8_t m);
static void colour_cycle(Carried *cr, uint8_t n);
static void colour_cycle6(Carried *cr);
static void colour_from_dp0(Carried *cr, uint32_t at);
static int colour_step(Carried *cr, uint8_t n);
static void copy16(Carried *cr, uint32_t dst, uint32_t src);
static void count_gate_frame(Carried *cr);
static void count_marker_frame(Carried *cr);
static void count_marker_record(Carried *cr);
static void count_trigger_frame(Carried *cr);
static void count_trigger_record(Carried *cr);
static void counter_show(Carried *cr, uint16_t o);
static void counter_slide(Carried *cr);
static void counter_tick(Carried *cr);
static void crowd_gate_frame(Carried *cr);
static void crowd_spawn(Carried *cr, uint16_t dx, uint16_t dy, uint16_t dz, uint8_t s15, uint8_t t, uint16_t h);
static uint32_t db(Carried *cr);
static uint8_t dec8(Carried *cr, uint32_t addr);
static uint32_t dp(Carried *cr, uint8_t off);
static void drop_frame(Carried *cr);
static void drop_record(Carried *cr);
static void duo_frame(Carried *cr);
static void ease16(Carried *cr, uint32_t addr, uint32_t target, uint16_t (*to)(uint16_t, int *));
static int ease8(Carried *cr, uint32_t addr, uint8_t target, uint8_t (*to)(uint8_t, int *));
static int ease8p(Carried *cr, uint32_t addr, uint8_t target, uint8_t (*to)(uint8_t, int *), unsigned pre);
static void eor8(Carried *cr, uint32_t addr, uint8_t m);
static void fade_frame(Carried *cr, uint8_t lap, uint16_t step, uint16_t end);
static void fade_set(Carried *cr, uint16_t step, uint16_t end);
static void fan_frame(Carried *cr, int arm, int xz, uint16_t lit, uint8_t snd);
static void finale_frame(Carried *cr);
static void finish_gate_frame(Carried *cr);
static void finish_gate_record(Carried *cr);
static void firework_burst(Carried *cr);
static void flash_frame(Carried *cr, uint16_t flash, uint16_t step, uint16_t end, int rep2);
static void gate42d(Carried *cr, uint16_t s);
static void gate_frame(Carried *cr);
static void gate_record(Carried *cr);
static void handler(Carried *cr, uint16_t o, uint16_t h);
static void handler_none(Carried *cr, uint16_t o);
static void handlers2(Carried *cr, uint16_t r, uint16_t f);
static int hit(Carried *cr);
static void hit_lamp(Carried *cr, uint16_t rec, uint16_t step);
static void hit_lamp_frame(Carried *cr, uint16_t flag);
static void hit_lamp_record(Carried *cr);
static void hit_switch_frame(Carried *cr, uint16_t flag);
static void hit_switch_record(Carried *cr);
static int hit_to(Carried *cr);
static int hit_to_n(Carried *cr, int *late);
static void inc8(Carried *cr, uint32_t addr);
static void lamp(Carried *cr, uint16_t o, uint8_t t);
static void lamp_post_frame(Carried *cr);
static void lantern_frame(Carried *cr);
static void lantern_record(Carried *cr);
static void lap1_fade_frame(Carried *cr);
static void lap_ball_record(Carried *cr, uint16_t kind);
static int lap_eq(Carried *cr, uint16_t f);
static void lap_flash_frame(Carried *cr);
static void lap_gate_frame(Carried *cr);
static void lap_gate_record(Carried *cr);
static int lap_is(Carried *cr, uint8_t lap);
static int lap_is_next(Carried *cr, uint8_t lap);
static int lap_is_x(Carried *cr, uint8_t lap, unsigned pre, unsigned eq, unsigned ne);
static void lap_light(Carried *cr, uint8_t ds);
static void lap_light_frame(Carried *cr);
static int lap_matches_0a(Carried *cr);
static void lap_store(Carried *cr, uint16_t f);
static void lap_take_0a(Carried *cr);
static uint8_t ld8(Carried *cr, uint32_t addr);
static void lda8(Carried *cr, uint8_t v);
static void life_down(Carried *cr, unsigned next);
static void meter_frame(Carried *cr);
static int next_contact(Carried *cr);
static int next_contact_t(Carried *cr, unsigned tail);
static void or8(Carried *cr, uint32_t addr, uint8_t m);
static void or8l(Carried *cr, uint32_t addr, uint8_t m, unsigned more);
static void or8x(Carried *cr, uint32_t addr, uint8_t m, unsigned more);
static void orbit(Carried *cr, int arm, uint8_t r1, uint8_t r2, uint8_t r3, int xz);
static void orbit_add(Carried *cr, uint16_t f, uint32_t by, uint32_t adc);
static void orbit_again(Carried *cr);
static void orbit_axis(Carried *cr, uint8_t r, uint32_t lo, uint32_t lo2, uint32_t hi, uint32_t abs8);
static void orbit_scale_done(Carried *cr);
static void orbiter(Carried *cr, uint16_t h, uint16_t f);
static void piece_frame(Carried *cr);
static void platform_record(Carried *cr);
static void post1_frame(Carried *cr);
static void post2_frame(Carried *cr);
static void post_record(Carried *cr, uint16_t v);
static void rail(Carried *cr, uint16_t dx, uint16_t dz, uint16_t t);
static void rail_drop_frame(Carried *cr);
static int rail_slide(Carried *cr, uint16_t step);
static void record424alt(Carried *cr, uint8_t h);
static void record_at(Carried *cr, uint16_t x, uint16_t y, uint16_t z);
static int released_by_hud(Carried *cr);
static void ring_frame(Carried *cr, int arm);
static void shard(Carried *cr, uint16_t kind, uint8_t s48, uint8_t s49, uint8_t s4a, uint8_t vx, uint8_t vy);
static void shard_from_z(Carried *cr);
static void shatter_from_x(Carried *cr, const uint16_t *kinds, int k, int neg);
static void shatter_from_y(Carried *cr, const uint16_t *kinds, int k, int neg);
static void shatter_marker_frame(Carried *cr);
static void shatter_marker_record(Carried *cr);
static void shatter_rest(Carried *cr, const uint16_t *kinds, int from);
static void shell(Carried *cr, uint16_t dx, uint8_t s14, uint8_t s15, uint8_t s16);
static void shell_frame(Carried *cr);
static void shell_pair(Carried *cr);
static void shell_record(Carried *cr);
static void show48(Carried *cr);
static void sign_arm(Carried *cr, uint32_t hi, int neg);
static void sign_frame(Carried *cr, uint16_t dx, uint16_t dy, uint16_t dz);
static void sign_gate_frame(Carried *cr);
static void sign_spawn(Carried *cr, uint16_t dx, uint16_t dy, uint16_t dz, uint16_t h);
static void sky_ball(Carried *cr);
static void slide1_frame(Carried *cr);
static void slide_bits_frame(Carried *cr, uint8_t and_m, uint8_t or_m, uint8_t snd);
static void slide_chime_frame(Carried *cr);
static void slide_gate_frame(Carried *cr);
static void slide_gate_out_m8(Carried *cr);
static void slide_hold(Carried *cr);
static void slide_lap_frame(Carried *cr);
static void slide_set(Carried *cr, uint16_t step, uint16_t target, uint16_t dir);
static void slide_zone_frame(Carried *cr);
static void sound0(Carried *cr, uint8_t v);
static void sound1(Carried *cr, uint8_t v);
static void spark(Carried *cr, uint8_t t, uint16_t vx, uint16_t vy, uint16_t vz);
static void spark_follow(Carried *cr, uint16_t kind);
static void sparkle(Carried *cr);
static void sparkle_frame(Carried *cr);
static int spawn(Carried *cr, uint16_t kind);
static void spinner_frame(Carried *cr, int arm);
static void st16i(Carried *cr, uint32_t addr, uint16_t v);
static void st16w(Carried *cr, uint32_t addr, uint16_t v);
static void st8i(Carried *cr, uint32_t addr, uint8_t v);
static void st8i_hi(Carried *cr, uint32_t addr, uint8_t v);
static void swap_xy(Carried *cr);
static void swap_xy_less(Carried *cr, unsigned less);
static void sweep0_frame(Carried *cr);
static void swing_frame(Carried *cr);
static void switch_frame(Carried *cr);
static void switch_record(Carried *cr);
static int synced(Carried *cr);
static void time_gate(Carried *cr, uint16_t r, unsigned released, unsigned tail);
static void time_gate_frame(Carried *cr);
static void trail_frame(Carried *cr, uint16_t dz, uint16_t dx, uint16_t mx);
static void trail_zone_frame(Carried *cr);
static void wave_ball(Carried *cr, uint16_t h, uint16_t dx, uint16_t dy, uint16_t dz, uint8_t s,
                      uint16_t rise, uint16_t gx, uint16_t gz, uint8_t life);
static void wave_ball_a(Carried *cr);
static void wave_ball_b(Carried *cr);
static void wave_ball_c(Carried *cr);
static void wave_gate_frame(Carried *cr, int which);
static void wave_gate_record(Carried *cr);
static void wave_home(Carried *cr);
static void window_gate_frame(Carried *cr, int on);
static void window_set_frame(Carried *cr, uint8_t t3460, uint8_t a1, uint8_t o1, uint8_t a2, uint8_t o2,
                             int store2);
void GsuRecordFlag10_cr(Carried *cr);
void GsuRecordFlags8AClear(Carried *cr);
void GsuRecordFlags8ASet(Carried *cr);
void GsuRecordLower82_cr(Carried *cr);
void GsuRecordMode2_cr(Carried *cr);
void GsuRecordNew424Alt_cr(Carried *cr);
void GsuRecordNew424Piece_cr(Carried *cr);
void GsuRecordNew424_cr(Carried *cr);
void GsuRecordNew42D_cr(Carried *cr);
void GsuRecordPosition_cr(Carried *cr);
void GsuRecordRaise6A_cr(Carried *cr);
void GsuRecordSet2AHigh_cr(Carried *cr);
void GsuRecordVelocity_cr(Carried *cr);
void HudCountMove19EECopy_cr(Carried *cr);
void HudCountMove19EE_cr(Carried *cr);
void HudCountUp19EC_cr(Carried *cr);
void ObjectCopy14To_cr(Carried *cr);
void ObjectCopyPositionFrom_cr(Carried *cr);
void ObjectCopyPositionTo_cr(Carried *cr);
void ObjectRebucket_cr(Carried *cr);
void ObjectRelease_cr(Carried *cr);
void ObjectSpawnPuff(Carried *cr);
void ObjectSpawn_cr(Carried *cr);
void ObjectStep48To8_cr(Carried *cr);
void ObjectSyncFromRecord_cr(Carried *cr);
void ObjectSyncToRecord_cr(Carried *cr);
void RecordContactSummary_cr(Carried *cr);
void RecordNextContact_cr(Carried *cr);
void RotateXY_cr(Carried *cr);
void RotateXZFrom08_cr(Carried *cr);
void RotateXZ_cr(Carried *cr);
void SoundPort0QueuePush_cr(Carried *cr);
void SoundPort1QueuePush_cr(Carried *cr);

static uint32_t db(Carried *cr)
{
    return (uint32_t)cr->db << 16;
}

/* Object field `f` ($20xx) of object `o`, through the data bank. */
#define F_c(f, o) (db(cr) | (uint16_t)((f) + (o)))

/* LDA #v with M 8-bit. */
static void lda8(Carried *cr, uint8_t v)
{
    cyc(20);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
}

/* LDA addr, 8-bit; returns the byte. */
static uint8_t ld8(Carried *cr, uint32_t addr)
{
    cyc(8);
    r_lda8(cr, addr);
    return (uint8_t)cr->a;
}

/* LDA #v, STA addr, 8-bit. */
static void st8i(Carried *cr, uint32_t addr, uint8_t v)
{
    lda8(cr, v);
    cyc(20);
    wr8(addr, v);
}

/* REP #$20, LDA #v, STA addr, SEP #$20. */
static void st16i(Carried *cr, uint32_t addr, uint16_t v)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16_imm(cr, v);
    cyc(16);
    wr16(addr, v);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

/* LDA addr, ORA #m, STA addr (8-bit); `more` is the time of any further ORA
   before the store. */
static void or8x(Carried *cr, uint32_t addr, uint8_t m, unsigned more)
{
    cyc(22);
    uint8_t v = (uint8_t)(ld8(cr, addr) | m);

    lda8(cr, v);
    cyc(((addr & 0xFFFF) >= 0x2000 ? 26 : 20) + more);   /* STA abs,X (an object field) or abs */
    wr8(addr, v);
}

static void or8(Carried *cr, uint32_t addr, uint8_t m)
{
    or8x(cr, addr, m, 0);
}

/* LDA long, ORA #m, STA long (8-bit): the stores are 8 more than or8's;
   `more` is the time of any further ORA before the store. */
static void or8l(Carried *cr, uint32_t addr, uint8_t m, unsigned more)
{
    cyc(22);
    uint8_t v = (uint8_t)(ld8(cr, addr) | m);

    lda8(cr, v);
    cyc(28 + more);
    wr8(addr, v);
}

/* LDA addr, AND #m, STA addr (8-bit). */
static void and8(Carried *cr, uint32_t addr, uint8_t m)
{
    cyc(22);
    uint8_t v = (uint8_t)(ld8(cr, addr) & m);

    lda8(cr, v);
    cyc((addr & 0xFFFF) >= 0x2000 ? 26 : 20);
    wr8(addr, v);
}

/* REP #$20, LDA addr, CLC, ADC #v, STA addr, SEP #$20. */
static void add16(Carried *cr, uint32_t addr, uint16_t v)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(30);
    r_lda16(cr, addr);
    r_add16(cr, v);
    cyc(68);
    wr16(addr, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

/* DEC addr (8-bit); returns the new value. */
static uint8_t dec8(Carried *cr, uint32_t addr)
{
    cyc(8);
    uint8_t v = (uint8_t)(rd8(addr) - 1);

    cyc(6);
    wr8(addr, v);
    set_p(cr, nz8(cr->p, v));
    return v;
}

/* INC addr (8-bit). */
static void inc8(Carried *cr, uint32_t addr)
{
    cyc(46);
    uint8_t v = (uint8_t)(rd8(addr) + 1);

    cyc(6);
    wr8(addr, v);
    set_p(cr, nz8(cr->p, v));
}

/* CMP #m (8-bit). */
static void cmp8(Carried *cr, uint8_t m)
{
    uint8_t a = (uint8_t)cr->a;

    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? P_C : 0)));
}

/* The handler of object o: +$1A = h, +$1C = $09. */
static void handler(Carried *cr, uint16_t o, uint16_t h)
{
    st16i(cr, F_c(0x201A, o), h);
    st8i(cr, F_c(0x201C, o), cyc_then(28, 0x09));
}

/* The handler cleared: +$1A = 0, +$1C = 0. */
static void handler_none(Carried *cr, uint16_t o)
{
    st16i(cr, F_c(0x201A, o), cyc_then(22, 0x0000));
    st8i(cr, F_c(0x201C, o), cyc_then(28, 0x00));
    cyc(44);
}

/* A new object of `kind` ($1F2B, ObjectSpawn): 1 when one was made (C
   set), Y = it. */
static int spawn(Carried *cr, uint16_t kind)
{
    st16i(cr, (cyc(16), db(cr)) | W(wram_1F2B), kind);
    (cyc(84), ObjectSpawn_cr(cr));
    cyc(22);
    return (cr->p & P_C) != 0;
}

/* PHX, TYX, PLY: X and Y swapped, N and Z from Y. */
static void swap_xy_less(Carried *cr, unsigned less)
{
    cyc(164 - less);
    uint16_t t = cr->x;

    cr->x = cr->y;
    cr->y = t;
    set_p(cr, nz16(cr->p, cr->y));
}

static void swap_xy(Carried *cr)
{
    swap_xy_less(cr, 0);
}

/* +$24 |= 8, +$3A = 0, +$5F = t: an object lit by its own light ($5D). */
static void lamp(Carried *cr, uint16_t o, uint8_t t)
{
    or8(cr, F_c(0x2024, o), 0x08);
    st8i(cr, F_c(0x203A, o), cyc_then(6, 0x00));
    st8i(cr, F_c(0x205F, o), cyc_then(6, t));
}

/*
 * @routine 09:8B3B ObjectLampTower
 * WHAT IT DOES: Sets up a lamp on a tower: object X 116 units lower
 * ($FF74 on +$0E), its light ($5D = $9971, +$24 bit 3, +$3A = 0, +$5F =
 * $3C), shape $A104, +$09 bit 0 cleared; and, when one can be made, a
 * second light (kind $A120, $5D = $9973, +$5F = $64) at its position $578
 * higher, rebucketed; then no handler.
 * HOW: ObjectSpawn, ObjectCopyPositionTo, ObjectRebucket with X and Y
 * swapped round it. Without a free object the second light is not made.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0.
 */
void ObjectLampTower(Carried *cr)
{
    add16(cr, F_c(0x200E, cr->x), cyc_then(36, 0xFF74));
    st16i(cr, F_c(0x205D, cr->x), cyc_then(44, 0x9971));
    cyc(22);
    lamp(cr, cr->x, 0x3C);
    st16i(cr, F_c(0x2004, cr->x), cyc_then(22, 0xA104));
    and8(cr, F_c(0x2009, cr->x), cyc_then(22, 0xFE));
    if (spawn(cr, 0xA120)) {
        lamp(cr, cr->y, 0x64);
        st16i(cr, F_c(0x205D, cr->y), cyc_then(22, 0x9973));
        (cyc(84), ObjectCopyPositionTo_cr(cr));
        add16(cr, F_c(0x200E, cr->y), cyc_then(22, 0xFA88));
        swap_xy(cr);
        ObjectRebucket_cr(cr);
        (cyc(10), swap_xy_less(cr, 94));
    } else { cyc(26); }                 /* BCS not taken, JML */
    handler_none(cr, cr->x);
}

void abi_ObjectLampTower(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLampTower(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:8BD4 ObjectLampLow
 * WHAT IT DOES: Sets up a lamp: its light ($5D = $9973, +$24 bit 3, +$3A =
 * 0, +$5F = $50), object X 380 units higher ($FE84 on +$0E); then no
 * handler.
 * HOW: Byte and word stores.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 0.
 */
void ObjectLampLow(Carried *cr)
{
    cyc(14);
    lamp(cr, cr->x, 0x50);
    st16i(cr, F_c(0x205D, cr->x), cyc_then(22, 0x9973));
    add16(cr, F_c(0x200E, cr->x), cyc_then(44, 0xFE84));
    cyc(22);
    handler_none(cr, cr->x);
}

void abi_ObjectLampLow(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLampLow(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:8C0F ObjectScale40
 * WHAT IT DOES: Sets object X's scale +$15 to $40.
 * HOW: One store.
 * Entry: M 8-bit; X = object. Exit: A low byte = $40.
 */
void ObjectScale40(Carried *cr)
{
    st8i(cr, F_c(0x2015, cr->x), cyc_then(20, 0x40));
    cyc(44);
}

void abi_ObjectScale40(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectScale40(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:8CD6: the gate's frame: synced from its record; +$25 bit 0 = the
   sync's bit 0 and the record's contact summary's bit 0 together. */
static void gate_frame(Carried *cr)
{
    (cyc(62), ObjectSyncFromRecord_cr(cr));
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if ((uint8_t)cr->a) {
        cyc(12);
        (cyc(62), RecordContactSummary_cr(cr));
        lda8(cr, (uint8_t)(cr->a & 0x01));
        if ((uint8_t)cr->a) {
            cyc(12);
            or8(cr, F_c(0x2025, cr->x), 0x01);
            { cyc(44); return; }
        } else {
            cyc(18);
            and8(cr, F_c(0x2025, cr->x), 0xFE);
            cyc(44);
            return;
        }
    }
    cyc(18);
    and8(cr, F_c(0x2025, cr->x), 0xFE);
    cyc(44);
}

/* 09:8CAF: the gate's record: a new one ($42D: GsuRecordNew42D) with flag
   bit 4, at (+$F00, +$F00, +$F00) (GsuRecordPosition); then its frame. */
static void gate_record(Carried *cr)
{
    (cyc(62), GsuRecordNew42D_cr(cr));
    GsuRecordFlag10_cr(cr);
    cyc(8);
    st16i(cr, W(dp_02), 0x0F00);
    cyc(30);
    st16i(cr, W(dp_08), 0x0F00);
    cyc(30);
    st16i(cr, W(dp_94), 0x0F00);
    (cyc(84), GsuRecordPosition_cr(cr));
    gate_frame(cr);
}

/*
 * @routine 09:8C16 ObjectScoreGate
 * WHAT IT DOES: Sets up a scoring gate: its restart handler (+$1D/+$1F =
 * $09:8CAF, ObjectScoreGateRecord) and frame handler ($09:8CD6,
 * ObjectScoreGateStep); a counter object (kind $A040) at its position
 * $6E0 left, $A0 lower and $F00 further on, linked back (+$27 = X), with
 * handler $09:8CF8 (ObjectScoreCounterStep), +$49 = 2, scale $20, +$5A =
 * $80, count +$48 = 0, +$0A = $50; then the gate's record and frame.
 * HOW: ObjectSpawn and ObjectCopyPositionTo; without a free object only
 * the gate is set up.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectScoreGateStep.
 */
void ObjectScoreGate(Carried *cr)
{
    st16i(cr, F_c(0x201D, cr->x), cyc_then(22, 0x8CAF));
    st8i(cr, F_c(0x201F, cr->x), cyc_then(28, 0x09));
    cyc(22);
    handler(cr, cr->x, 0x8CD6);
    if (spawn(cr, 0xA040)) {
        uint8_t a;

        (cyc(62), ObjectCopyPositionTo_cr(cr));
        a = (uint8_t)cr->a;                     /* PHA */
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cr->a = cr->x;
        set_p(cr, nz16(cr->p, cr->a));
        wr16(F_c(0x2027, cr->y), cyc_then(88, cr->x));
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(52);
        lda8(cr, a);                                /* PLA */
        handler(cr, cr->y, 0x8CF8);
        st8i(cr, F_c(0x2049, cr->y), cyc_then(6, 0x02));
        add16(cr, F_c(0x200C, cr->y), cyc_then(22, 0xF920));
        add16(cr, F_c(0x200E, cr->y), cyc_then(44, 0x00A0));
        add16(cr, F_c(0x2010, cr->y), cyc_then(44, 0x0F00));
        st16i(cr, F_c(0x2015, cr->y), cyc_then(44, 0x0020));
        lda8(cr, 0x00);
        lda8(cr, 0x80);
        wr8(F_c(0x205A, cr->y), cyc_then(44, 0x80));
        st8i(cr, F_c(0x2048, cr->y), cyc_then(6, 0x00));
        st8i(cr, F_c(0x200A, cr->y), cyc_then(6, 0x50));
    } else { cyc(26); }
    gate_record(cr);
}

void abi_ObjectScoreGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectScoreGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:8CAF ObjectScoreGateRecord
 * WHAT IT DOES: The gate's restart: a new Super FX record ($42D) with
 * flag bit 4 at (+$F00, +$F00, +$F00), then ObjectScoreGateStep.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectScoreGateStep.
 */
void ObjectScoreGateRecord(Carried *cr)
{
    gate_record(cr);
}

void abi_ObjectScoreGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectScoreGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:8CD6 ObjectScoreGateStep
 * WHAT IT DOES: The gate's frame: synced from its record; its +$25 bit 0
 * set while the record's sync and contact summary both have bit 0 set,
 * else cleared.
 * HOW: ObjectSyncFromRecord, RecordContactSummary.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte the new
 * +$25.
 */
void ObjectScoreGateStep(Carried *cr)
{
    gate_frame(cr);
}

void abi_ObjectScoreGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectScoreGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:8D94: the count +$48 down (not below 0), shown at +$5A with bit 7
   ($1F58 = the count). */
static void counter_show(Carried *cr, uint16_t o)
{
    uint8_t v;

    v = ld8(cr, cyc_then(22, F_c(0x2048, o)));
    cmp8(cr, 0x00);
    if (v)
        { cyc(60); dec8(cr, F_c(0x2048, o)); } else { cyc(64); }
    cyc(22);
    v = ld8(cr, F_c(0x2048, o));
    wr8(db(cr) | W(wram_1F58), cyc_then(24, v));
    v = (uint8_t)(ld8(cr, (cyc(16), db(cr)) | W(wram_1F58)) | 0x80);
    lda8(cr, v);
    wr8(F_c(0x205A, o), cyc_then(26, v));
}

/* 09:8D34: every third frame (+$49 counting down from 2), by its gate's
   +$25 bit 0 (+$27): passing it sets +$25 bit 0, sound $32 and counts
   +$48 up to 9; else clears it and counts down; shown at +$5A. */
static void counter_tick(Carried *cr)
{
    uint16_t o = cr->x;
    uint8_t v;

    v = ld8(cr, cyc_then(90, F_c(0x2049, o)));
    cmp8(cr, 0x00);
    if (v) {
        cyc(60);
        dec8(cr, F_c(0x2049, o));
        return;
    } else { cyc(70); }
    st8i(cr, F_c(0x2049, o), 0x02);
    cr->y = rd16(cyc_then(30, F_c(0x2027, o)));
    set_p(cr, nz16(cr->p, cr->y));
    lda8(cr, (uint8_t)(ld8(cr, cyc_then(22, F_c(0x2025, cr->y))) & 0x01));
    if (!(uint8_t)cr->a) {
        cyc(44);
        and8(cr, F_c(0x2025, o), 0xFE);
        counter_show(cr, o);
        return;
    } else { cyc(40); }
    lda8(cr, (uint8_t)(ld8(cr, F_c(0x2025, o)) & 0x01));
    if (!(uint8_t)cr->a) {
        cyc(18);
        or8(cr, F_c(0x2025, o), 0x01);
        cyc(58);
        lda8(cr, 0x32);
        SoundPort0QueuePush_cr(cr);
    } else { cyc(44); }
    cyc(22);
    v = ld8(cr, F_c(0x2048, o));
    cmp8(cr, 0x09);
    if (v != 0x09)
        { cyc(22); inc8(cr, F_c(0x2048, o)); } else { cyc(64); }
    cyc(22);
    v = ld8(cr, F_c(0x2048, o));
    wr8(db(cr) | W(wram_1F58), cyc_then(24, v));
    v = (uint8_t)(ld8(cr, (cyc(16), db(cr)) | W(wram_1F58)) | 0x80);
    lda8(cr, v);
    wr8(F_c(0x205A, o), cyc_then(26, v));
}

/*
 * @routine 09:8D34 ObjectScoreCounterTick
 * WHAT IT DOES: Every third frame (+$49 counting down from 2) follows its
 * gate (+$27): while the gate's +$25 bit 0 is set, sets its own (sound $32
 * the first time) and counts +$48 up to 9; else clears it and counts +$48
 * down to 0; shows the count at +$5A with bit 7 ($1F58 = the count).
 * HOW: Byte compares and JMLs inside the bank; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A low byte
 * = the shown count with bit 7, or the countdown's value when it only
 * counted down; N, Z from that.
 */
void ObjectScoreCounterTick(Carried *cr)
{
    counter_tick(cr);
}

void abi_ObjectScoreCounterTick(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectScoreCounterTick(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:8DB1: while +$0A runs down, the scale +$15 eased toward $40
   (ApproachQuarterMin4Byte via $3E) and the object $5A further right;
   then the tick. */
static void counter_slide(Carried *cr)
{
    uint16_t o = cr->x;
    uint8_t v;

    v = ld8(cr, F_c(0x200A, o));
    cmp8(cr, 0x00);
    if (v) {
        cyc(60);
        dec8(cr, F_c(0x200A, o));
        v = ld8(cr, cyc_then(22, F_c(0x2015, o)));
        cyc(16);
        wr8(W(dp_3e), v);
        lda8(cr, 0x40);
        (cyc(58), r_approach_byte(cr, ApproachQuarterMin4Byte));
        wr8(F_c(0x2015, o), cyc_then(30, (uint8_t)cr->a));
        add16(cr, F_c(0x200C, o), cyc_then(22, 0x005A));
    } else { cyc(42); }
    counter_tick(cr);
}

/*
 * @routine 09:8DB1 ObjectScoreCounterSlide
 * WHAT IT DOES: While +$0A runs down, eases the scale +$15 toward $40
 * (ApproachQuarterMin4Byte via $3E) and moves the counter $5A right; then
 * ObjectScoreCounterTick.
 * HOW: 16-bit add for X; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectScoreCounterTick.
 */
void ObjectScoreCounterSlide(Carried *cr)
{
    counter_slide(cr);
}

void abi_ObjectScoreCounterSlide(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectScoreCounterSlide(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:8CF8 ObjectScoreCounterStep
 * WHAT IT DOES: The score counter's frame, by the lap count $0DEA: on the
 * first lap the first time its +$25 bit 1 and shape $B754, then its tick;
 * on the second it slides in (counter_slide); then rebucketed.
 * HOW: The tick (counter_tick) every third frame follows its gate's +$25
 * bit 0. Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectRebucket.
 */
void ObjectScoreCounterStep(Carried *cr)
{
    uint8_t v;

    set_p(cr, (uint8_t)(cr->p | (P_M)));
    v = ld8(cr, (cyc(52), db(cr)) | W(wram_0DEA));
    if (v) {
        cyc(18);
        cmp8(cr, 0x01);
        if (v == 0x01) {
            cyc(58);
            lda8(cr, (uint8_t)(ld8(cr, F_c(0x2025, cr->x)) & 0x02));
            if (!(uint8_t)cr->a) {
                cyc(18);
                or8(cr, F_c(0x2025, cr->x), 0x02);
                st16i(cr, F_c(0x2004, cr->x), cyc_then(22, 0xB754));
            } else { cyc(22); }
            counter_tick(cr);
        } else {
            cmp8(cr, 0x02);
            if (v == 0x02)
                { cyc(136); counter_slide(cr); }
            cyc(20);
        }
        cyc(42);
    }
    cyc(84);
    ObjectRebucket_cr(cr);
    cyc(44);
}

void abi_ObjectScoreCounterStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectScoreCounterStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:8DF8: synced from its record; released when +$0A has run down. */
static void piece_frame(Carried *cr)
{
    (cyc(62), ObjectSyncFromRecord_cr(cr));
    if (dec8(cr, cyc_then(22, F_c(0x200A, cr->x))) == 0)
        { cyc(84); ObjectRelease_cr(cr); cyc(44); } else { cyc(92); }
}

/*
 * @routine 09:8DDF ObjectFlyingPiece
 * WHAT IT DOES: Sets up a flying piece: handler $09:8DF8
 * (ObjectFlyingPieceStep), a Super FX record from 08:CBCC (released at
 * once when there is none), its velocity (GsuRecordVelocity), then its
 * frame.
 * HOW: GsuRecordNew424Piece (08:CBCC); its N flag tells the failure.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as the path leaves it.
 */
void ObjectFlyingPiece(Carried *cr)
{
    cyc(22);
    handler(cr, cr->x, 0x8DF8);
    cyc(62);
    GsuRecordNew424Piece_cr(cr);
    if (cr->p & P_N) {
        (cyc(84), ObjectRelease_cr(cr));
        { cyc(44); return; }
    } else { cyc(16); }
    (cyc(62), GsuRecordVelocity_cr(cr));
    piece_frame(cr);
}

void abi_ObjectFlyingPiece(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFlyingPiece(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:8DF8 ObjectFlyingPieceStep
 * WHAT IT DOES: A flying piece's frame: synced from its record
 * (ObjectSyncFromRecord); released once +$0A runs down.
 * HOW: DEC of the byte.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as the path leaves it.
 */
void ObjectFlyingPieceStep(Carried *cr)
{
    piece_frame(cr);
}

void abi_ObjectFlyingPieceStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFlyingPieceStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:8E0A ObjectFlyingPieceRelease
 * WHAT IT DOES: Releases object X (ObjectRelease): where a flying piece
 * goes when it gets no Super FX record.
 * HOW: JSL ObjectRelease, RTL.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRelease.
 */
void ObjectFlyingPieceRelease(Carried *cr)
{
    (cyc(62), ObjectRelease_cr(cr));
    cyc(44);
}

void abi_ObjectFlyingPieceRelease(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFlyingPieceRelease(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_GsuRecordSet2AHigh(void);

/* REP #$20, X/Y/Z to direct page $02, $08, $94 (each a word), SEP #$20,
   GsuRecordPosition. */
static void record_at(Carried *cr, uint16_t x, uint16_t y, uint16_t z)
{
    cyc(8);
    st16i(cr, W(dp_02), x);
    cyc(30);
    st16i(cr, W(dp_08), y);
    cyc(30);
    st16i(cr, W(dp_94), z);
    (cyc(84), GsuRecordPosition_cr(cr));
}

/* 09:8EAF: a spark: a new object of kind $A0CC lit by $996B (+$5F = t) at
   object X's position, handler $09:8DDF (ObjectFlyingPiece), velocity
   (vx, vy, vz), +$0A = $0F; nothing without a free object. */
static void spark(Carried *cr, uint8_t t, uint16_t vx, uint16_t vy, uint16_t vz)
{
    if (!spawn(cr, 0xA0CC))
        { cyc(26); return; } else { cyc(22); }
    st16i(cr, F_c(0x205D, cr->y), 0x996B);
    cyc(22);
    lamp(cr, cr->y, t);
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    cyc(22);
    handler(cr, cr->y, 0x8DDF);
    st16i(cr, F_c(0x202E, cr->y), cyc_then(22, vx));
    st16i(cr, F_c(0x2030, cr->y), cyc_then(44, vy));
    st16i(cr, F_c(0x2032, cr->y), cyc_then(44, vz));
    st8i(cr, F_c(0x200A, cr->y), cyc_then(28, 0x0F));
}

/* 09:8EAF: the shell's burst: four sparks flying out left, right, back and
   on. */
static void firework_burst(Carried *cr)
{
    cyc(46);
    spark(cr, 0x14, 0x0028, 0xFFAB, 0x0000);
    spark(cr, 0x03, 0xFFD8, 0xFFAB, 0x0000);
    spark(cr, 0x0A, 0x0000, 0xFFA6, 0x0028);
    spark(cr, 0x01, 0x0000, 0xFFA6, 0xFFD8);
}

/*
 * @routine 09:8EAF ObjectFireworkBurst
 * WHAT IT DOES: Bursts a firework into four sparks (kind $A0CC, lit by
 * $996B, handler ObjectFlyingPiece, +$0A = $0F) at object X, flying out at
 * ($28, -$55, 0), (-$28, -$55, 0), (0, -$5A, $28), (0, -$5A, -$28); a spark
 * without a free object is skipped.
 * HOW: ObjectSpawn, ObjectCopyPositionTo for each; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A, Y as
 * the last spark leaves them.
 */
void ObjectFireworkBurst(Carried *cr)
{
    firework_burst(cr);
}

void abi_ObjectFireworkBurst(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFireworkBurst(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:8E5A: the shell's frame: released when +$0A runs down (and goes on);
   its size bytes +$14-$16 grow ($0C added to the words at +$14, +$15 and
   +$16); synced from its record; on a hit (sync and contact bit 0) it is
   released with sound $56 and bursts into four sparks. */
static void shell_frame(Carried *cr)
{
    if (dec8(cr, cyc_then(22, F_c(0x200A, cr->x))) == 0)
        { cyc(84); ObjectRelease_cr(cr); cyc(22); } else { cyc(70); }
    add16(cr, F_c(0x2014, cr->x), 0x000C);
    add16(cr, F_c(0x2015, cr->x), cyc_then(44, 0x000C));
    add16(cr, F_c(0x2016, cr->x), cyc_then(44, 0x000C));
    (cyc(84), ObjectSyncFromRecord_cr(cr));
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if (!(uint8_t)cr->a)
        { cyc(62); return; } else { cyc(12); }
    (cyc(62), RecordContactSummary_cr(cr));
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if (!(uint8_t)cr->a)
        return; else { cyc(74); }
    ObjectRelease_cr(cr);
    cyc(58);
    lda8(cr, 0x56);
    SoundPort0QueuePush_cr(cr);
    firework_burst(cr);
    cyc(86);
}

/* 09:8E2D: the shell's record (08:CBCC), velocity, +$2A high byte 8
   (GsuRecordSet2AHigh) at ($190, $190, $190); then its frame. */
static void shell_record(Carried *cr)
{
    cyc(62);
    GsuRecordNew424Piece_cr(cr);
    (cyc(62), GsuRecordVelocity_cr(cr));
    lda8(cr, 0x08);
    (cyc(58), GsuRecordSet2AHigh_cr(cr));
    cyc(102);
    record_at(cr, 0x0190, 0x0190, 0x0190);
    shell_frame(cr);
}

/*
 * @routine 09:8E0F ObjectFireworkShell
 * WHAT IT DOES: Sets up a firework shell: frame handler $09:8E5A
 * (ObjectFireworkShellStep), restart $09:8E2D (ObjectFireworkShellRecord),
 * then its record and frame.
 * HOW: Word and byte stores; the record from GsuRecordNew424Piece (08:CBCC).
 * Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectFireworkShell(Carried *cr)
{
    cyc(22);
    handler(cr, cr->x, 0x8E5A);
    st16i(cr, F_c(0x201D, cr->x), cyc_then(22, 0x8E2D));
    st8i(cr, F_c(0x201F, cr->x), cyc_then(28, 0x09));
    shell_record(cr);
}

void abi_ObjectFireworkShell(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFireworkShell(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:8E2D ObjectFireworkShellRecord
 * WHAT IT DOES: The shell's restart: a Super FX record (08:CBCC) with its
 * velocity, +$2A's high byte 8, at ($190, $190, $190); then its frame.
 * HOW: GsuRecordVelocity, GsuRecordSet2AHigh, GsuRecordPosition. Reached
 * only by unit_objects_09.
 * Entry: as ObjectFireworkShell. Exit: as ObjectFireworkShell.
 */
void ObjectFireworkShellRecord(Carried *cr)
{
    shell_record(cr);
}

void abi_ObjectFireworkShellRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFireworkShellRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:8E5A ObjectFireworkShellStep
 * WHAT IT DOES: The shell's frame: released when +$0A runs down; its size
 * bytes +$14-$16 grow; on a hit (its record's sync and contact bit 0) it is
 * released with sound $56 and bursts into four sparks (kind $A0CC, lit by
 * $996B, handler ObjectFlyingPiece, +$0A = $0F, flying out at four
 * velocities).
 * HOW: ObjectSyncFromRecord, RecordContactSummary; the sparks by
 * ObjectSpawn and ObjectCopyPositionTo. Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path
 * leaves it.
 */
void ObjectFireworkShellStep(Carried *cr)
{
    shell_frame(cr);
}

void abi_ObjectFireworkShellStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFireworkShellStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:90C3: a firework shell: a new object of kind $B6E4 at object X's
   position, handler $09:8E0F (ObjectFireworkShell), moved (dx, -$500,
   -$1900), velocity (0, 0, $15E), size bytes (s14, s15, s16), +$0A = $1E. */
static void shell(Carried *cr, uint16_t dx, uint8_t s14, uint8_t s15, uint8_t s16)
{
    if (!spawn(cr, 0xB6E4))
        { cyc(26); return; }
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    cyc(22);
    handler(cr, cr->y, 0x8E0F);
    add16(cr, F_c(0x200E, cr->y), cyc_then(22, 0xFB00));
    add16(cr, F_c(0x2010, cr->y), cyc_then(44, 0xE700));
    add16(cr, F_c(0x200C, cr->y), cyc_then(44, dx));
    st16i(cr, F_c(0x202E, cr->y), cyc_then(44, 0x0000));
    st16i(cr, F_c(0x2030, cr->y), cyc_then(44, 0x0000));
    st16i(cr, F_c(0x2032, cr->y), cyc_then(44, 0x015E));
    st8i(cr, F_c(0x2014, cr->y), cyc_then(28, s14));
    st8i(cr, F_c(0x2015, cr->y), cyc_then(6, s15));
    st8i(cr, F_c(0x2016, cr->y), cyc_then(6, s16));
    st8i(cr, F_c(0x200A, cr->y), cyc_then(6, 0x1E));
}

/* 09:90C3: the finish gate's two firework shells, $3C0 right and $3C0
   left. */
static void shell_pair(Carried *cr)
{
    cyc(46);
    shell(cr, 0x03C0, 0x3C, 0x32, 0x23);
    shell(cr, 0xFC40, 0x78, 0x3C, 0x0C);
}

/*
 * @routine 09:90C3 ObjectFireworkShellPair
 * WHAT IT DOES: Fires the finish gate's two firework shells (kind $B6E4,
 * handler ObjectFireworkShell), $3C0 to each side, $500 up, $1900 back,
 * rising at $15E, with their sizes and +$0A = $1E.
 * HOW: ObjectSpawn, ObjectCopyPositionTo for each; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A, Y as
 * the second shell leaves them.
 */
void ObjectFireworkShellPair(Carried *cr)
{
    shell_pair(cr);
}

void abi_ObjectFireworkShellPair(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFireworkShellPair(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:908D: the finish gate's frame: synced from its record; on a hit
   (sync and contact bit 0) on the second lap ($0DEA = 2), the first time
   (+$25 bit 0), sound $11 and two firework shells. */
static void finish_gate_frame(Carried *cr)
{
    (cyc(62), ObjectSyncFromRecord_cr(cr));
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if (!(uint8_t)cr->a)
        { cyc(62); return; } else { cyc(12); }
    (cyc(62), RecordContactSummary_cr(cr));
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if (!(uint8_t)cr->a)
        { cyc(62); return; } else { cyc(50); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    ld8(cr, db(cr) | W(wram_0DEA));
    cmp8(cr, 0x02);
    if ((uint8_t)cr->a != 0x02)
        { cyc(82); return; } else { cyc(54); }
    lda8(cr, (uint8_t)(ld8(cr, F_c(0x2025, cr->x)) & 0x01));
    if ((uint8_t)cr->a)
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    cyc(58);
    lda8(cr, 0x11);
    SoundPort0QueuePush_cr(cr);
    shell_pair(cr);
    cyc(86);
}

/* 09:9066: the finish gate's record ($42D) with flag bit 4 at ($1680,
   $1680, $1680); then its frame. */
static void finish_gate_record(Carried *cr)
{
    (cyc(62), GsuRecordNew42D_cr(cr));
    GsuRecordFlag10_cr(cr);
    record_at(cr, 0x1680, 0x1680, 0x1680);
    finish_gate_frame(cr);
}

/*
 * @routine 09:9048 ObjectFinishGate
 * WHAT IT DOES: Sets up the finish gate's fireworks: restart $09:9066
 * (ObjectFinishGateRecord) and frame handler $09:908D
 * (ObjectFinishGateStep), then its record and frame.
 * HOW: Word and byte stores. Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectFinishGate(Carried *cr)
{
    st16i(cr, F_c(0x201D, cr->x), cyc_then(22, 0x9066));
    st8i(cr, F_c(0x201F, cr->x), cyc_then(28, 0x09));
    cyc(22);
    handler(cr, cr->x, 0x908D);
    finish_gate_record(cr);
}

void abi_ObjectFinishGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFinishGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9066 ObjectFinishGateRecord
 * WHAT IT DOES: The finish gate's restart: a Super FX record ($42D) with
 * flag bit 4 at ($1680, $1680, $1680), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_objects_09.
 * Entry: as ObjectFinishGate. Exit: as ObjectFinishGate.
 */
void ObjectFinishGateRecord(Carried *cr)
{
    finish_gate_record(cr);
}

void abi_ObjectFinishGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFinishGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:908D ObjectFinishGateStep
 * WHAT IT DOES: The finish gate's frame: synced from its record; when it is
 * hit (sync and contact bit 0) on the second lap for the first time (+$25
 * bit 0), sound $11 and two firework shells (kind $B6E4, handler
 * ObjectFireworkShell) launched beside it.
 * HOW: ObjectSyncFromRecord, RecordContactSummary, ObjectSpawn. Reached
 * only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectFinishGateStep(Carried *cr)
{
    finish_gate_frame(cr);
}

void abi_ObjectFinishGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFinishGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_GsuRecordNew424(void);
void abi_GsuRecordNew424Alt(void);
void abi_GsuRecordMode2(void);
void abi_RecordNextContact(void);
void abi_GsuRecordRaise6A(void);

/* LDA addr, CLC, ADC #v, STA addr (8-bit). */
static void add8(Carried *cr, uint32_t addr, uint8_t v)
{
    cyc(22);
    uint8_t r = ld8(cr, addr);

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    r = alu_adc8(r, v, &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | r);
    cyc(60);
    wr8(addr, r);
}

/* LDA addr, STA $3E, LDA #target, JSL to; STA addr: a byte eased toward
   the target by the routine at `to`; `pre` is the time before the LDA's
   read. Returns C. */
static int ease8p(Carried *cr, uint32_t addr, uint8_t target, uint8_t (*to)(uint8_t, int *), unsigned pre)
{
    cyc(pre);
    wr8(W(dp_3e), cyc_then(16, ld8(cr, addr)));
    cyc(58);
    lda8(cr, target);
    r_approach_byte(cr, to);
    cyc(30);
    wr8(addr, (uint8_t)cr->a);
    return (cr->p & P_C) != 0;
}

static int ease8(Carried *cr, uint32_t addr, uint8_t target, uint8_t (*to)(uint8_t, int *))
{
    return ease8p(cr, addr, target, to, 40);
}

/* The two set-up stores of a two-handler object: restart +$1D/+$1F = r,
   frame +$1A/+$1C = f. */
static void handlers2(Carried *cr, uint16_t r, uint16_t f)
{
    st16i(cr, F_c(0x201D, cr->x), cyc_then(22, r));
    st8i(cr, F_c(0x201F, cr->x), cyc_then(28, 0x09));
    cyc(22);
    handler(cr, cr->x, f);
}

/* Hit by a car: the record's sync and contact summary both with bit 0
   (ObjectSyncFromRecord, RecordContactSummary, each AND #1). */
static int hit(Carried *cr)
{
    (cyc(62), ObjectSyncFromRecord_cr(cr));
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if (!(uint8_t)cr->a)
        return 0; else { cyc(38); }
    cyc(36);
    RecordContactSummary_cr(cr);
    cyc(0);
    lda8(cr, (uint8_t)(cr->a & 0x01));
    return (uint8_t)cr->a != 0;
}

/* 09:91F6: the bumper's frame: while +$48 counts down, it bulges (+$15 by
   6, +$14 and +$16 by $0A); after, +$14 eases back (ApproachHalfMin2Byte)
   and, once that holds, +$16 too while +$15 grows by $0C; then a hit sets
   +$25 bit 0 the first time (sound $54, +$48 = 6), no hit clears it. */
static void bumper_frame(Carried *cr)
{
    uint8_t v = ld8(cr, cyc_then(22, F_c(0x2048, cr->x)));

    if (v) {
        cyc(22);
        add8(cr, F_c(0x2015, cr->x), 0x06);
        add8(cr, F_c(0x2014, cr->x), 0x0A);
        add8(cr, F_c(0x2016, cr->x), 0x0A);
        add8(cr, F_c(0x2048, cr->x), 0xFF);
        cyc(24);                        /* JMP */
    } else if (!(cyc(30), ease8(cr, F_c(0x2014, cr->x), 0x00, ApproachHalfMin2Byte))) {
        cyc(4);
        ease8(cr, F_c(0x2016, cr->x), 0x00, ApproachHalfMin2Byte);
        add8(cr, F_c(0x2015, cr->x), 0x0C);
    } else { cyc(48); }
    if (!hit(cr)) {
        cyc(18);
        and8(cr, F_c(0x2025, cr->x), 0xFE);
        cyc(44);
        return;
    } else { cyc(34); }
    lda8(cr, (uint8_t)(ld8(cr, F_c(0x2025, cr->x)) & 0x01));
    if ((uint8_t)cr->a)
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    cyc(58);
    lda8(cr, 0x54);
    SoundPort0QueuePush_cr(cr);
    st8i(cr, F_c(0x2048, cr->x), cyc_then(6, 0x06));
    cyc(44);
}

/* 09:91E8: the bumper's record ($424) with flag bit 4 and +$2A high 1;
   then its frame. */
static void bumper_record(Carried *cr)
{
    cyc(62);
    GsuRecordNew424_cr(cr);
    (cyc(62), GsuRecordFlag10_cr(cr));
    cyc(58);
    lda8(cr, 0x01);
    GsuRecordSet2AHigh_cr(cr);
    (cyc(102), bumper_frame(cr));
}

/*
 * @routine 09:91CA ObjectBumper
 * WHAT IT DOES: Sets up a bumper: restart $09:91E8 (ObjectBumperRecord),
 * frame $09:91F6 (ObjectBumperStep), then its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBumper(Carried *cr)
{
    handlers2(cr, 0x91E8, 0x91F6);
    bumper_record(cr);
}

void abi_ObjectBumper(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBumper(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:91E8 ObjectBumperRecord
 * WHAT IT DOES: The bumper's restart: a Super FX record ($424) with flag
 * bit 4 and +$2A's high byte 1, then its frame.
 * HOW: GsuRecordNew424, GsuRecordFlag10, GsuRecordSet2AHigh.
 * Entry: as ObjectBumper. Exit: as ObjectBumper.
 */
void ObjectBumperRecord(Carried *cr)
{
    bumper_record(cr);
}

void abi_ObjectBumperRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBumperRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:91F6 ObjectBumperStep
 * WHAT IT DOES: The bumper's frame: while +$48 counts down it bulges
 * (+$15 by 6, +$14 and +$16 by $0A a frame); then it settles back (+$14
 * and +$16 eased to 0 by ApproachHalfMin2Byte, +$15 growing by $0C); a car
 * hitting it sets +$25 bit 0 the first time with sound $54 and a new
 * bulge (+$48 = 6); no hit clears the bit.
 * HOW: ObjectSyncFromRecord, RecordContactSummary.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit.
 */
void ObjectBumperStep(Carried *cr)
{
    bumper_frame(cr);
}

void abi_ObjectBumperStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBumperStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:92BC: the switch's frame: when on (+$25 bit 0), +$48 eased toward 3
   (03:D07C); +$48 shown at +$5A with bit 7; off, a hit turns it on (+$15
   by $0C, sound $55). */
static void switch_frame(Carried *cr)
{
    uint8_t v;

    lda8(cr, (uint8_t)(ld8(cr, cyc_then(22, F_c(0x2025, cr->x))) & 0x01));
    if ((uint8_t)cr->a)
        cyc(ease8(cr, F_c(0x2048, cr->x), 0x03, ApproachSixteenthMin16Byte) ? 70 : 44);   /* BCC taken or not, JML */
    else
        cyc(66);
    v = ld8(cr, F_c(0x2048, cr->x));
    wr8(db(cr) | W(wram_1F58), cyc_then(24, v));
    v = (uint8_t)(ld8(cr, (cyc(16), db(cr)) | W(wram_1F58)) | 0x80);
    lda8(cr, v);
    wr8(F_c(0x205A, cr->x), cyc_then(26, v));
    lda8(cr, (uint8_t)(ld8(cr, cyc_then(22, F_c(0x2025, cr->x))) & 0x01));
    if ((uint8_t)cr->a)
        { cyc(88); return; } else { cyc(18); }
    if (!hit(cr))
        { cyc(62); return; } else { cyc(12); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    add8(cr, F_c(0x2015, cr->x), 0x0C);
    cyc(58);
    lda8(cr, 0x55);
    SoundPort0QueuePush_cr(cr);
    cyc(44);
}

/* 09:92AE: the switch's record ($424) with flag bit 4 and +$2A high 1;
   then its frame. */
static void switch_record(Carried *cr)
{
    (cyc(62), GsuRecordNew424_cr(cr));
    (cyc(62), GsuRecordFlag10_cr(cr));
    cyc(58);
    lda8(cr, 0x01);
    GsuRecordSet2AHigh_cr(cr);
    (cyc(102), switch_frame(cr));
}

/*
 * @routine 09:9289 ObjectSwitch
 * WHAT IT DOES: Sets up a switch: restart $09:92AE (ObjectSwitchRecord),
 * frame $09:92BC (ObjectSwitchStep), +$5A = $80; then its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectSwitch(Carried *cr)
{
    handlers2(cr, 0x92AE, 0x92BC);
    lda8(cr, 0x00);
    lda8(cr, 0x80);
    wr8(F_c(0x205A, cr->x), cyc_then(22, 0x80));
    switch_record(cr);
}

void abi_ObjectSwitch(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwitch(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:92AE ObjectSwitchRecord
 * WHAT IT DOES: The switch's restart: a Super FX record ($424) with flag
 * bit 4 and +$2A's high byte 1, then its frame.
 * HOW: GsuRecordNew424, GsuRecordFlag10, GsuRecordSet2AHigh.
 * Entry: as ObjectSwitch. Exit: as ObjectSwitch.
 */
void ObjectSwitchRecord(Carried *cr)
{
    switch_record(cr);
}

void abi_ObjectSwitchRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwitchRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:92BC ObjectSwitchStep
 * WHAT IT DOES: The switch's frame: once on (+$25 bit 0), its count +$48
 * eases toward 3 (03:D07C); the count is shown at +$5A with bit 7; while
 * off, a car hitting it turns it on (+$15 by $0C, sound $55). The bytes at
 * 09:931C (ObjectClear25Bit0) are reached by no code in it.
 * HOW: ObjectSyncFromRecord, RecordContactSummary,
 * ApproachSixteenthMin16Byte (03:D07C).
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit.
 */
void ObjectSwitchStep(Carried *cr)
{
    switch_frame(cr);
}

void abi_ObjectSwitchStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwitchStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:931C ObjectClear25Bit0
 * WHAT IT DOES: Clears bit 0 of object X's +$25 (the switch's "on").
 * HOW: LDA / AND #$FE / STA, 8-bit; RTL.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A low byte = the new +$25;
 * N, Z from it.
 */
void ObjectClear25Bit0(Carried *cr)
{
    and8(cr, F_c(0x2025, cr->x), 0xFE);
    cyc(44);
}

void abi_ObjectClear25Bit0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectClear25Bit0(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:934D: the platform's record ($424, +$0A $90E8) in mode 2 with +$2A
   high $50 at ($500, $1180, $500); then its frame (synced). */
static void platform_record(Carried *cr)
{
    (cyc(62), GsuRecordNew424Alt_cr(cr));
    (cyc(62), GsuRecordMode2_cr(cr));
    lda8(cr, 0x50);
    (cyc(58), GsuRecordSet2AHigh_cr(cr));
    cyc(102);
    record_at(cr, 0x0500, 0x1180, 0x0500);
    (cyc(62), ObjectSyncFromRecord_cr(cr));
    cyc(44);
}

/*
 * @routine 09:9325 ObjectPlatform
 * WHAT IT DOES: Sets up a platform: +$20 = $EC60, restart $09:934D
 * (ObjectPlatformRecord), frame $09:937A (ObjectPlatformStep); then its
 * record (mode 2, +$2A high $50, at ($500, $1180, $500)) and sync.
 * HOW: Word and byte stores. Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectSyncFromRecord.
 */
void ObjectPlatform(Carried *cr)
{
    st16i(cr, F_c(0x2020, cr->x), cyc_then(22, 0xEC60));
    cyc(22);
    handlers2(cr, 0x934D, 0x937A);
    platform_record(cr);
}

void abi_ObjectPlatform(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPlatform(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:934D ObjectPlatformRecord
 * WHAT IT DOES: The platform's restart: its Super FX record (GsuRecordNew424Alt,
 * GsuRecordMode2, +$2A high $50, at ($500, $1180, $500)), then synced.
 * HOW: As ObjectPlatform. Reached only by unit_objects_09.
 * Entry: as ObjectPlatform. Exit: as ObjectPlatform.
 */
void ObjectPlatformRecord(Carried *cr)
{
    platform_record(cr);
}

void abi_ObjectPlatformRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPlatformRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:937A ObjectPlatformStep
 * WHAT IT DOES: The platform's frame: synced from its record.
 * HOW: ObjectSyncFromRecord. Reached only by unit_objects_09.
 * Entry: as ObjectSyncFromRecord. Exit: as ObjectSyncFromRecord.
 */
void ObjectPlatformStep(Carried *cr)
{
    (cyc(62), ObjectSyncFromRecord_cr(cr));
    cyc(44);
}

void abi_ObjectPlatformStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPlatformStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:93C4: the first switch post's frame: a hit squashes +$14 toward $33
   and sets bit 0 of $1A07. */
static void post1_frame(Carried *cr)
{
    if (!hit(cr))
        { cyc(62); return; }
    ease8p(cr, F_c(0x2014, cr->x), 0x33, ApproachHalfMin2Byte, 34);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    or8(cr, (cyc(16), db(cr)) | W(wram_1A07), 0x01);
    cyc(44);
}

/* 09:939D: a post's record ($42D) with flag bit 4 at (v, v, v). */
static void post_record(Carried *cr, uint16_t v)
{
    cyc(62);
    GsuRecordNew42D_cr(cr);
    GsuRecordFlag10_cr(cr);
    record_at(cr, v, v, v);
}

/*
 * @routine 09:937F ObjectPostA
 * WHAT IT DOES: Sets up the first of a gate's two posts: restart $09:939D
 * (ObjectPostARecord), frame $09:93C4 (ObjectPostAStep); its record ($42D,
 * flag bit 4, at ($1E0, $1E0, $1E0)) and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectPostA(Carried *cr)
{
    handlers2(cr, 0x939D, 0x93C4);
    post_record(cr, 0x01E0);
    post1_frame(cr);
}

void abi_ObjectPostA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPostA(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:939D ObjectPostARecord
 * WHAT IT DOES: The first post's restart: its record ($42D, flag bit 4, at
 * ($1E0, $1E0, $1E0)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectPostA. Exit: as ObjectPostA.
 */
void ObjectPostARecord(Carried *cr)
{
    post_record(cr, 0x01E0);
    post1_frame(cr);
}

void abi_ObjectPostARecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPostARecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:93C4 ObjectPostAStep
 * WHAT IT DOES: The first post's frame: when a car hits it, +$14 eases
 * toward $33 (ApproachHalfMin2Byte) and bit 0 of $1A07 is set.
 * HOW: ObjectSyncFromRecord, RecordContactSummary.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit.
 */
void ObjectPostAStep(Carried *cr)
{
    post1_frame(cr);
}

void abi_ObjectPostAStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPostAStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ObjectSkyBallStep(void);

/* 09:9496: the sky ball: a new object of kind $AA34 linked to post X
   (+$27), handler $09:9520 (ObjectSkyBallStep), $C0 above and $2BC0 on,
   scale $80, +$48 = 0, +$29 and +$2B cleared, +$0A = +$49 = $0A, sound
   $0E; nothing without a free object. */
static void sky_ball(Carried *cr)
{
    uint8_t a;

    cyc(46);
    if (!spawn(cr, 0xAA34))
        { cyc(26); return; }
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    a = (uint8_t)cr->a;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->a = cr->x;
    set_p(cr, nz16(cr->p, cr->a));
    wr16(F_c(0x2027, cr->y), cyc_then(88, cr->x));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(52);
    lda8(cr, a);
    handler(cr, cr->y, 0x9520);
    add16(cr, F_c(0x200C, cr->y), cyc_then(22, 0x0000));
    add16(cr, F_c(0x200E, cr->y), cyc_then(44, 0xFF40));
    add16(cr, F_c(0x2010, cr->y), cyc_then(44, 0x2BC0));
    st8i(cr, F_c(0x2015, cr->y), cyc_then(28, 0x80));
    st8i(cr, F_c(0x2048, cr->y), cyc_then(6, 0x00));
    st16i(cr, F_c(0x2029, cr->y), cyc_then(22, 0x0000));
    st16i(cr, F_c(0x202B, cr->y), cyc_then(44, 0x0000));
    st8i(cr, F_c(0x200A, cr->y), cyc_then(28, 0x0A));
    st8i(cr, F_c(0x2049, cr->y), cyc_then(6, 0x0A));
    cyc(58);
    lda8(cr, 0x0E);
    SoundPort0QueuePush_cr(cr);
}

/*
 * @routine 09:9496 ObjectSkyBallLaunch
 * WHAT IT DOES: Launches the sky ball: a new object of kind $AA34 linked to
 * object X (+$27), handler ObjectSkyBallStep, $C0 up and $2BC0 on, scale
 * $80, +$0A = +$49 = $0A, sound $0E; nothing without a free object.
 * HOW: ObjectSpawn, ObjectCopyPositionTo; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; Y = the
 * new object; A low byte as the sound push leaves it.
 */
void ObjectSkyBallLaunch(Carried *cr)
{
    sky_ball(cr);
}

void abi_ObjectSkyBallLaunch(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSkyBallLaunch(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:9432: the second post's frame: a hit sets its +$25 bit 0 and bit 1 of
   $1A07 the first time; with both posts hit ($1A07 = 3), the first time
   (+$25 bit 1) the sky ball, its target +$27 = this post's +$10 - $2300. */
static void post2_frame(Carried *cr)
{
    if (!hit(cr))
        { cyc(62); return; } else { cyc(40); }
    lda8(cr, cyc_then(18, (uint8_t)(ld8(cr, F_c(0x2025, cr->x)) & 0x01)));
    if (!(uint8_t)cr->a) {
        or8(cr, F_c(0x2025, cr->x), 0x01);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        or8(cr, (cyc(16), db(cr)) | W(wram_1A07), 0x02);
    } else { cyc(26); }
    cyc(16);
    ld8(cr, db(cr) | W(wram_1A07));
    cmp8(cr, 0x03);
    if ((uint8_t)cr->a != 0x03)
        { cyc(82); return; } else { cyc(54); }
    lda8(cr, (uint8_t)(ld8(cr, F_c(0x2025, cr->x)) & 0x02));
    if ((uint8_t)cr->a)
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x02);
    sky_ball(cr);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(94, F_c(0x2010, cr->x)));
    wr16(F_c(0x2027, cr->y), cyc_then(30, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    add16(cr, F_c(0x2027, cr->y), cyc_then(44, 0xDD00));
    cyc(66);
}

/*
 * @routine 09:93ED ObjectPostB
 * WHAT IT DOES: Sets up the second post: restart $09:940B
 * (ObjectPostBRecord), frame $09:9432 (ObjectPostBStep); its record ($42D,
 * flag bit 4, at ($F00, $F00, $F00)) and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectPostB(Carried *cr)
{
    handlers2(cr, 0x940B, 0x9432);
    post_record(cr, 0x0F00);
    post2_frame(cr);
}

void abi_ObjectPostB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPostB(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:940B ObjectPostBRecord
 * WHAT IT DOES: The second post's restart: its record ($42D, flag bit 4,
 * at ($F00, $F00, $F00)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectPostB. Exit: as ObjectPostB.
 */
void ObjectPostBRecord(Carried *cr)
{
    post_record(cr, 0x0F00);
    post2_frame(cr);
}

void abi_ObjectPostBRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPostBRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9432 ObjectPostBStep
 * WHAT IT DOES: The second post's frame: when a car hits it, its +$25 bit
 * 0 and bit 1 of $1A07 the first time; once both posts are hit ($1A07 =
 * 3), the first time (its +$25 bit 1), a sky ball (kind $AA34, handler
 * ObjectSkyBallStep) whose target +$27 is this post's +$10 less $2300.
 * HOW: ObjectSyncFromRecord, RecordContactSummary, ObjectSpawn. The sky
 * ball's spawn is reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path
 * leaves it.
 */
void ObjectPostBStep(Carried *cr)
{
    post2_frame(cr);
}

void abi_ObjectPostBStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPostBStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 03:CDC1 (ApproachHalfMin2): a word eased toward A. */
static uint16_t ease_cdc1(uint16_t target, int *moved)
{
    cyc(62);
    return ApproachHalfMin2(target, moved);
}

/* REP #$20, LDA addr, STA $3E, LDA target, JSL to, STA addr, SEP #$20. */
static void ease16(Carried *cr, uint32_t addr, uint32_t target, uint16_t (*to)(uint16_t, int *))
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, addr);
    cyc(16);
    wr16(W(dp_3e), cr->a);
    cyc(30);
    r_lda16(cr, target);
    r_approach(cr, to);
    cyc(30);
    wr16(addr, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

/* 09:9599: the ball follows the player ($0346): its X eased toward the
   player's (ApproachQuarterMin4), its Z toward the player's (03:CDC1),
   then $380 back. */
static void ball_follow(Carried *cr)
{
    cr->y = rd16(cyc_then(70, db(cr) | W(wram_0346)));
    set_p(cr, nz16(cr->p, cr->y));
    ease16(cr, F_c(0x200C, cr->x), F_c(0x200C, cr->y), ApproachQuarterMin4);
    (cyc(22), ease16(cr, F_c(0x2010, cr->x), F_c(0x2010, cr->y), ease_cdc1));
    add16(cr, F_c(0x2010, cr->x), cyc_then(44, 0xFC80));
}

/*
 * @routine 09:9599 ObjectBallFollowPlayer
 * WHAT IT DOES: Moves object X toward the player ($0346): its X eased toward
 * the player's (ApproachQuarterMin4), its Z toward the player's
 * (ApproachHalfMin2), then $380 back.
 * HOW: 16-bit; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; Y = the
 * player's object; A as the last add leaves it.
 */
void ObjectBallFollowPlayer(Carried *cr)
{
    ball_follow(cr);
}

void abi_ObjectBallFollowPlayer(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBallFollowPlayer(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:95D1 / 95FC: the ball drops (dz, dy) with +$14 eased toward t
   (03:D07C). */
static void ball_drop(Carried *cr, uint16_t dz, uint16_t dy, uint8_t t)
{
    add16(cr, F_c(0x2010, cr->x), cyc_then(72, dz));
    add16(cr, F_c(0x200E, cr->x), cyc_then(44, dy));
    (cyc(4), ease8(cr, F_c(0x2014, cr->x), t, ApproachSixteenthMin16Byte));
}

/* 09:95FC: the rising ball's step, its life +$0A counted down and the
   ball released when it runs out. */
static void ball_rise(Carried *cr)
{
    ball_drop(cr, 0xFFB0, 0xFF10, 0xC0);
    if (dec8(cr, cyc_then(22, F_c(0x200A, cr->x))) == 0)
        { cyc(84); ObjectRelease_cr(cr); cyc(42); } else { cyc(90); }   /* RTS */
}

/*
 * @routine 09:95FC ObjectSkyBallRise
 * WHAT IT DOES: The rising sky ball's step: $50 back, $F0 up, +$14 eased
 * toward $C0; released (ObjectRelease) when +$0A runs out.
 * HOW: As ObjectSkyBallFall, then a byte DEC.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectSkyBallFall.
 */
void ObjectSkyBallRise(Carried *cr)
{
    ball_rise(cr);
}

void abi_ObjectSkyBallRise(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSkyBallRise(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:95D1 ObjectSkyBallFall
 * WHAT IT DOES: The falling sky ball's step: $180 back, $1E up, +$14 eased
 * toward $F8 (ApproachSixteenthMin16Byte).
 * HOW: 16-bit adds, then the byte eased; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A low byte
 * = the new +$14.
 */
void ObjectSkyBallFall(Carried *cr)
{
    ball_drop(cr, 0xFE80, 0xFFE2, 0xF8);
}

void abi_ObjectSkyBallFall(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSkyBallFall(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9520 ObjectSkyBallStep
 * WHAT IT DOES: The sky ball's frame: while its +$10 has not passed its
 * target +$27 it rises ($50 back, $F0 up, +$14 toward $C0) and is released
 * when +$0A runs out; past it, for +$49 frames it follows the player, then
 * the first time (+$25 bit 0) it drops a ball (kind $B7A8, handler
 * $09:9634, ObjectBallDrop) and keeps falling ($180 back, $1E up, +$14
 * toward $F8); rebucketed each frame.
 * HOW: A 16-bit compare (BEQ or BMI); ApproachQuarterMin4, ApproachHalfMin2 (03:CDC1)
 * and ApproachSixteenthMin16Byte (03:D07C) ease the moves. Reached only by
 * unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket.
 */
void ObjectSkyBallStep(Carried *cr)
{
    uint16_t z, t;

    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(66, F_c(0x2010, cr->x)));
    wr16(db(cr) | W(wram_1F4C), cyc_then(24, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(74, F_c(0x2027, cr->x)));
    t = cr->a;
    z = rd16(cyc_then(24, db(cr) | W(wram_1F4C)));
    r_cmp16(cr, z);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (t == z || (cyc(78), (cr->p & P_N) ? (cyc(4), 1) : 0)) {
        uint8_t v = ld8(cr, F_c(0x2049, cr->x));

        if (v) {
            cyc(44);
            dec8(cr, F_c(0x2049, cr->x));
            ball_follow(cr);
            cyc(86);                    /* SEP, RTS, BRA */
        } else {
            cyc(70);
            lda8(cr, (uint8_t)(ld8(cr, F_c(0x2025, cr->x)) & 0x01));
            if (!(uint8_t)cr->a) {
                cyc(18);
                uint16_t sx = cr->x, sy = cr->y;

                or8(cr, F_c(0x2025, cr->x), 0x01);
                cyc(60);
                if (spawn(cr, 0xB7A8)) {
                    (cyc(62), ObjectCopyPositionTo_cr(cr));
                    cyc(22);
                    handler(cr, cr->y, 0x9634);
                } else { cyc(26); }
                cyc(28);
                cr->y = sy;
                cr->x = sx;
                set_p(cr, nz16(cr->p, sx));
            }
            cyc(40);
            ball_drop(cr, 0xFE80, 0xFFE2, 0xF8);
            cyc(64);                    /* RTS, BRA */
        }
    } else {
        cyc(4);
        ball_rise(cr);
    }
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

void abi_ObjectSkyBallStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSkyBallStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ObjectBallDropStep(void);

/* 09:967F: the dropped ball's frame: it grows (+$15, +$14 by $10); on a
   car's contact (the record's contacts, RecordNextContact, with bit 0) it
   raises that record's +$6A by $40 (GsuRecordRaise6A), sound $25, leaves
   a spark (kind $B7A8 at the player $0346's position, +$0A = $0A, +$27 =
   0, +$29 = the player, handler $09:AE60) and is released. */
static void drop_frame(Carried *cr)
{
    uint16_t sx;

    add8(cr, F_c(0x2015, cr->x), 0x10);
    add8(cr, F_c(0x2014, cr->x), 0x10);
    (cyc(62), ObjectSyncFromRecord_cr(cr));
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if (!(uint8_t)cr->a)
        { cyc(62); return; }
    cyc(74);
    for (;;) {
        RecordNextContact_cr(cr);
        if (!(cr->p & P_C))
            return; else { cyc(16); }
        lda8(cr, (uint8_t)(cr->a & 0x01));
        if ((uint8_t)cr->a)
            { cyc(70); break; }
        cyc(80);                        /* BEQ taken, JSL */
    }
    lda8(cr, 0x40);
    GsuRecordRaise6A_cr(cr);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(80);
    lda8(cr, 0x25);
    SoundPort0QueuePush_cr(cr);
    sx = cr->x;                                 /* PHX */
    cr->x = cr->y;                              /* TYX */
    set_p(cr, nz16(cr->p, cr->x));
    cyc(44);
    if (spawn(cr, 0xB7A8)) {
        uint8_t a;

        cr->x = rd16(cyc_then(24, db(cr) | W(wram_0346)));
        set_p(cr, nz16(cr->p, cr->x));
        st8i(cr, F_c(0x200A, cr->y), cyc_then(6, 0x0A));
        (cyc(62), ObjectCopyPositionTo_cr(cr));
        st16i(cr, F_c(0x2027, cr->y), cyc_then(22, 0x0000));
        a = (uint8_t)cr->a;
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cr->a = cr->x;
        set_p(cr, nz16(cr->p, cr->a));
        wr16(F_c(0x2029, cr->y), cyc_then(110, cr->x));
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(52);
        lda8(cr, a);
        handler(cr, cr->y, 0xAE60);
    } else { cyc(26); }
    cyc(36);
    cr->x = sx;                                 /* PLX */
    set_p(cr, nz16(cr->p, sx));
    (cyc(62), ObjectRelease_cr(cr));
    cyc(44);
}

/* 09:9652: the dropped ball's record ($42D) with flag bit 4, +$2A high
   $14, at ($A0, $A0, $A0); then its frame. */
static void drop_record(Carried *cr)
{
    (cyc(62), GsuRecordNew42D_cr(cr));
    GsuRecordFlag10_cr(cr);
    lda8(cr, 0x14);
    (cyc(58), GsuRecordSet2AHigh_cr(cr));
    cyc(102);
    record_at(cr, 0x00A0, 0x00A0, 0x00A0);
    drop_frame(cr);
}

/*
 * @routine 09:9634 ObjectBallDrop
 * WHAT IT DOES: Sets up a dropped ball: restart $09:9652
 * (ObjectBallDropRecord), frame $09:967F (ObjectBallDropStep); its record
 * and frame.
 * HOW: Word and byte stores. Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBallDrop(Carried *cr)
{
    handlers2(cr, 0x9652, 0x967F);
    drop_record(cr);
}

void abi_ObjectBallDrop(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBallDrop(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9652 ObjectBallDropRecord
 * WHAT IT DOES: The dropped ball's restart: its record ($42D, flag bit 4,
 * +$2A high $14, at ($A0, $A0, $A0)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordSet2AHigh,
 * GsuRecordPosition. Reached only by unit_objects_09.
 * Entry: as ObjectBallDrop. Exit: as ObjectBallDrop.
 */
void ObjectBallDropRecord(Carried *cr)
{
    drop_record(cr);
}

void abi_ObjectBallDropRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBallDropRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:967F ObjectBallDropStep
 * WHAT IT DOES: The dropped ball's frame: it grows (+$15 and +$14 by $10);
 * on a car's contact it raises that car's record +$6A by $40, plays sound
 * $25, leaves a spark (kind $B7A8 at the player's position, handler
 * $09:AE60) and is released.
 * HOW: ObjectSyncFromRecord, then RecordNextContact over the contacts.
 * Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectBallDropStep(Carried *cr)
{
    drop_frame(cr);
}

void abi_ObjectBallDropStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBallDropStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* REP #$20, LDA src, STA $1F4C, then LDA dst, CLC, ADC $1F4C, STA dst,
   SEP #$20: dst += src through $1F4C. */
static void add16_via(Carried *cr, uint32_t dst, uint32_t src)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(74);
    r_lda16(cr, src);
    wr16(db(cr) | W(wram_1F4C), cyc_then(24, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(74);
    r_lda16(cr, dst);
    r_add16(cr, rd16(cyc_then(38, db(cr) | W(wram_1F4C))));
    cyc(30);
    wr16(dst, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

/* A cannon ball: a new object of kind $AA34 linked to object X (+$27),
   handler h, moved (dx, dy, dz), scale s, +$48 = 0, its rise +$29 and
   drift +$2B, life +$0A, homing frames +$49, sound $0E. */
static void cannon_ball(Carried *cr, uint16_t h, uint16_t dx, uint16_t dy, uint16_t dz, uint8_t s,
                        uint16_t rise, uint16_t drift, uint8_t life, uint8_t home)
{
    uint8_t a;

    if (!spawn(cr, 0xAA34))
        { cyc(26); return; }
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    a = (uint8_t)cr->a;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->a = cr->x;
    set_p(cr, nz16(cr->p, cr->a));
    wr16(F_c(0x2027, cr->y), cyc_then(88, cr->x));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(52);
    lda8(cr, a);
    handler(cr, cr->y, h);
    add16(cr, F_c(0x200C, cr->y), cyc_then(22, dx));
    add16(cr, F_c(0x200E, cr->y), cyc_then(44, dy));
    add16(cr, F_c(0x2010, cr->y), cyc_then(44, dz));
    st8i(cr, F_c(0x2015, cr->y), cyc_then(28, s));
    st8i(cr, F_c(0x2048, cr->y), cyc_then(6, 0x00));
    st16i(cr, F_c(0x2029, cr->y), cyc_then(22, rise));
    st16i(cr, F_c(0x202B, cr->y), cyc_then(44, drift));
    st8i(cr, F_c(0x200A, cr->y), cyc_then(28, life));
    st8i(cr, F_c(0x2049, cr->y), cyc_then(6, home));
    cyc(58);
    lda8(cr, 0x0E);
    SoundPort0QueuePush_cr(cr);
}

/* 09:9746: the lap gate's frame: a hit when the lap $0DEA differs from its
   +$0A takes the lap; lap 0 fires three balls to the left ($09:9ACB),
   lap 2 three to the right ($09:9AE8). */
/* 09:9793: the first lap's three cannon balls (ObjectCannonBallLeft),
   $3C00 to the left. */
static void cannon_balls_left(Carried *cr)
{
    cannon_ball(cr, 0x9ACB, 0xC400, 0xF880, 0x0500, 0xC0, 0xFD60, 0x0000, 0x05, 0x09);
    cannon_ball(cr, 0x9ACB, 0xC400, 0xFC40, 0xF880, 0xC0, 0xFD40, 0xFE00, 0x05, 0x09);
    cannon_ball(cr, 0x9ACB, 0xC400, 0xF9C0, 0x0000, 0xC0, 0xFD20, 0x0200, 0x05, 0x09);
}

/*
 * @routine 09:9793 ObjectCannonBallsLeft
 * WHAT IT DOES: Fires the first lap's three cannon balls (kind via
 * ObjectSpawn, handler ObjectCannonBallLeft) from object X, $3C00 to the
 * left, at three heights and spreads.
 * HOW: ObjectSpawn, ObjectCopyPositionTo for each; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A, Y as
 * the third ball leaves them.
 */
void ObjectCannonBallsLeft(Carried *cr)
{
    cannon_balls_left(cr);
}

void abi_ObjectCannonBallsLeft(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCannonBallsLeft(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:992F: the third lap's three cannon balls (ObjectCannonBallRight),
   $2580 to the right. */
static void cannon_balls_right(Carried *cr)
{
    cannon_ball(cr, 0x9AE8, 0x2580, 0xF880, 0x0500, 0x40, 0xFDC0, 0x0000, 0x12, 0x06);
    cannon_ball(cr, 0x9AE8, 0x2580, 0xFC40, 0xF880, 0x40, 0xFDA0, 0xFF00, 0x12, 0x06);
    cannon_ball(cr, 0x9AE8, 0x2580, 0xF9C0, 0x0000, 0x40, 0xFDC0, 0x0100, 0x12, 0x06);
}

/*
 * @routine 09:992F ObjectCannonBallsRight
 * WHAT IT DOES: As ObjectCannonBallsLeft for the third lap: handler
 * ObjectCannonBallRight, $2580 to the right.
 * HOW: As ObjectCannonBallsLeft.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectCannonBallsLeft.
 */
void ObjectCannonBallsRight(Carried *cr)
{
    cannon_balls_right(cr);
}

void abi_ObjectCannonBallsRight(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCannonBallsRight(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

static void lap_gate_frame(Carried *cr)
{
    uint8_t lap;

    if (!hit(cr))
        { cyc(62); return; } else { cyc(34); }
    wr8(db(cr) | W(wram_1F58), cyc_then(24, ld8(cr, F_c(0x200A, cr->x))));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    lap = ld8(cr, (cyc(38), db(cr)) | W(wram_0DEA));
    {
        uint8_t m = rd8(cyc_then(24, db(cr) | W(wram_1F58)));

        set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(lap - m)) & ~P_C) | (lap >= m ? (cyc(66), P_C) : (cyc(32), 0))));
        if (lap == m)
            return;
    }
    wr8(db(cr) | W(wram_1F58), cyc_then(24, ld8(cr, db(cr) | W(wram_0DEA))));
    wr8(F_c(0x200A, cr->x), cyc_then(30, ld8(cr, (cyc(16), db(cr)) | W(wram_1F58))));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    lap = ld8(cr, (cyc(38), db(cr)) | W(wram_0DEA));
    cyc(84);
    cmp8(cr, 0x00);
    if (lap == 0x00) {
        cannon_balls_left(cr);
        { cyc(110); return; }
    } else { cyc(54); }
    cmp8(cr, 0x01);
    if (lap == 0x01)
        return; else { cyc(10); }
    cmp8(cr, 0x02);
    if (lap != 0x02)
        { cyc(16); return; }
    cannon_balls_right(cr);
    cyc(86);
}

/* 09:971F: the lap gate's record ($42D) with flag bit 4 at ($F00, $F00,
   $F00); then its frame. */
static void lap_gate_record(Carried *cr)
{
    post_record(cr, 0x0F00);
    lap_gate_frame(cr);
}

/*
 * @routine 09:96FC ObjectCannonGate
 * WHAT IT DOES: Sets up a cannon gate: restart $09:971F
 * (ObjectCannonGateRecord), frame $09:9746 (ObjectCannonGateStep), its lap
 * +$0A = 4; then its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectCannonGate(Carried *cr)
{
    handlers2(cr, 0x971F, 0x9746);
    st8i(cr, F_c(0x200A, cr->x), cyc_then(6, 0x04));
    lap_gate_record(cr);
}

void abi_ObjectCannonGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCannonGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:971F ObjectCannonGateRecord
 * WHAT IT DOES: The cannon gate's restart: its record ($42D, flag bit 4, at
 * ($F00, $F00, $F00)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectCannonGate. Exit: as ObjectCannonGate.
 */
void ObjectCannonGateRecord(Carried *cr)
{
    lap_gate_record(cr);
}

void abi_ObjectCannonGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCannonGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9746 ObjectCannonGateStep
 * WHAT IT DOES: The cannon gate's frame: when a car hits it on a lap ($0DEA)
 * other than the one it last fired on (+$0A), it takes that lap; on lap 0
 * it fires three cannon balls to one side (kind $AA34, handler
 * ObjectCannonBallLeft), on lap 2 three to the other (ObjectCannonBallRight),
 * each with its own drop, rise and drift, sound $0E.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; the lap compared through
 * $1F58. The firing is reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectCannonGateStep(Carried *cr)
{
    lap_gate_frame(cr);
}

void abi_ObjectCannonGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCannonGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:9B6D: the ball homes on its gate (Y): X and Z eased toward the gate's
   (ApproachQuarterMin4), Z += +$2B; Y eased toward the gate's (03:CDC1),
   Y += +$29. */
static void ball_home(Carried *cr)
{
    (cyc(46), ease16(cr, F_c(0x200C, cr->x), F_c(0x200C, cr->y), ApproachQuarterMin4));
    (cyc(22), ease16(cr, F_c(0x2010, cr->x), F_c(0x2010, cr->y), ApproachQuarterMin4));
    add16_via(cr, F_c(0x2010, cr->x), F_c(0x202B, cr->x));
    (cyc(22), ease16(cr, F_c(0x200E, cr->x), F_c(0x200E, cr->y), ease_cdc1));
    add16_via(cr, F_c(0x200E, cr->x), F_c(0x2029, cr->x));
}

/*
 * @routine 09:9B6D ObjectCannonBallHome
 * WHAT IT DOES: The cannon ball homes on its gate (Y): X and Z eased toward
 * the gate's (ApproachQuarterMin4), Z += +$2B; Y eased toward the gate's
 * (ApproachHalfMin2), Y += +$29.
 * HOW: 16-bit eases and adds; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A as the
 * last add leaves it.
 */
void ObjectCannonBallHome(Carried *cr)
{
    ball_home(cr);
}

void abi_ObjectCannonBallHome(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCannonBallHome(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:9B05 / 9B39: the ball flies: Z += +$2B, X += dx; released when +$0A
   runs out. */
static void ball_fly(Carried *cr, uint16_t dx)
{
    add16_via(cr, F_c(0x2010, cr->x), F_c(0x202B, cr->x));
    add16(cr, F_c(0x200C, cr->x), cyc_then(44, dx));
    if (dec8(cr, cyc_then(44, F_c(0x200A, cr->x))) == 0)
        { cyc(84); ObjectRelease_cr(cr); cyc(42); } else { cyc(90); }   /* RTS */
}

/*
 * @routine 09:9B39 ObjectCannonBallFlyRight
 * WHAT IT DOES: As ObjectCannonBallFlyLeft with X -= $380.
 * HOW: As ObjectCannonBallFlyLeft.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectCannonBallFlyRight(Carried *cr)
{
    ball_fly(cr, 0xFC80);
}

void abi_ObjectCannonBallFlyRight(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCannonBallFlyRight(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9B05 ObjectCannonBallFlyLeft
 * WHAT IT DOES: The left cannon ball flies: Z += +$2B, X += $100; released
 * (ObjectRelease) when +$0A runs out.
 * HOW: 16-bit adds, a byte DEC; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectCannonBallFlyLeft(Carried *cr)
{
    ball_fly(cr, 0x0100);
}

void abi_ObjectCannonBallFlyLeft(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCannonBallFlyLeft(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The ball's frame: Y = its gate; for +$49 frames it homes, then it flies
   (dx a frame); rebucketed. */
static void cannon_ball_frame(Carried *cr, uint16_t dx)
{
    uint8_t v;

    cr->y = rd16(cyc_then(44, F_c(0x2027, cr->x)));
    set_p(cr, nz16(cr->p, cr->y));
    v = ld8(cr, cyc_then(22, F_c(0x2049, cr->x)));
    if (v) {
        cyc(44);
        dec8(cr, F_c(0x2049, cr->x));
        ball_home(cr);
        cyc(148);
    } else {
        cyc(72);
        ball_fly(cr, dx);
        cyc(62);
    }
    ObjectRebucket_cr(cr);
    cyc(44);
}

/*
 * @routine 09:9ACB ObjectCannonBallLeft
 * WHAT IT DOES: A cannon ball's frame: for +$49 frames it homes on its gate
 * (+$27; X and Z eased to the gate's, then its drift +$2B and rise +$29
 * added), then it flies off ($100 a frame along X, drifting) until its life
 * +$0A runs out; rebucketed.
 * HOW: ApproachQuarterMin4, ApproachHalfMin2 (03:CDC1), ObjectRelease,
 * ObjectRebucket. Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket.
 */
void ObjectCannonBallLeft(Carried *cr)
{
    cannon_ball_frame(cr, 0x0100);
}

void abi_ObjectCannonBallLeft(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCannonBallLeft(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9AE8 ObjectCannonBallRight
 * WHAT IT DOES: As ObjectCannonBallLeft, flying off the other way ($380 a
 * frame back along X).
 * HOW: As ObjectCannonBallLeft. Reached only by unit_objects_09.
 * Entry: as ObjectCannonBallLeft. Exit: as ObjectCannonBallLeft.
 */
void ObjectCannonBallRight(Carried *cr)
{
    cannon_ball_frame(cr, 0xFC80);
}

void abi_ObjectCannonBallRight(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCannonBallRight(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ObjectSyncToRecord(void);

/* As hit(), with the object's position given to its record
   (ObjectSyncToRecord) instead. */
static int hit_to_n(Carried *cr, int *late)
{
    (cyc(62), ObjectSyncToRecord_cr(cr));
    cyc(4);
    cr->a = (uint16_t)((cr->a & 0xFF00) | (cr->a & 0x01));
    set_p(cr, nz8(cr->p, (uint8_t)cr->a));
    *late = 0;
    if (!(uint8_t)cr->a)
        return 0; else { cyc(28); }
    (cyc(62), RecordContactSummary_cr(cr));
    cyc(28);
    lda8(cr, (uint8_t)(cr->a & 0x01));
    *late = 1;
    return (uint8_t)cr->a != 0;
}

static int hit_to(Carried *cr)
{
    int late;

    return hit_to_n(cr, &late);
}

/* A wave ball: a new object of kind $AA34 linked to gate X (+$27),
   handler h, moved (dx, dy, dz), scale s, rise +$29 = rise; the gate moves
   (gx, gz); life +$0A, sound $0E. Nothing without a free object. */
static void wave_ball(Carried *cr, uint16_t h, uint16_t dx, uint16_t dy, uint16_t dz, uint8_t s,
                      uint16_t rise, uint16_t gx, uint16_t gz, uint8_t life)
{
    uint8_t a;

    cyc(46);
    if (!spawn(cr, 0xAA34))
        { cyc(26); return; }
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    a = (uint8_t)cr->a;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->a = cr->x;
    set_p(cr, nz16(cr->p, cr->a));
    wr16(F_c(0x2027, cr->y), cyc_then(88, cr->x));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(52);
    lda8(cr, a);
    handler(cr, cr->y, h);
    add16(cr, F_c(0x200C, cr->y), cyc_then(22, dx));
    add16(cr, F_c(0x200E, cr->y), cyc_then(44, dy));
    add16(cr, F_c(0x2010, cr->y), cyc_then(44, dz));
    st8i(cr, F_c(0x2015, cr->y), cyc_then(28, s));
    st16i(cr, F_c(0x2029, cr->y), cyc_then(22, rise));
    add16(cr, F_c(0x200C, cr->x), cyc_then(44, gx));
    add16(cr, F_c(0x2010, cr->x), cyc_then(44, gz));
    st8i(cr, F_c(0x200A, cr->y), cyc_then(28, life));
    cyc(58);
    lda8(cr, 0x0E);
    SoundPort0QueuePush_cr(cr);
}

/* 09:9CE4: the first gate's big wave ball (ObjectWaveBallBig). */
static void wave_ball_a(Carried *cr)
{
    wave_ball(cr, 0x9F05, 0xDA80, 0xFE80, 0xDD00, 0xE0, 0xFF00, 0x0F00, 0x0F00, 0x0F);
}

/*
 * @routine 09:9CE4 ObjectWaveBallLaunchA
 * WHAT IT DOES: Launches the first wave gate's big ball (kind $AA34, handler
 * ObjectWaveBallBig) linked to gate X: $2580 left, $180 up, $2300 back,
 * scale $E0, rise -$100, life $0F; the gate moves $F00 on in X and Z; sound
 * $0E. Nothing without a free object.
 * HOW: ObjectSpawn, ObjectCopyPositionTo, 16-bit adds; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; Y = the
 * new object; A low byte as the sound push leaves it.
 */
void ObjectWaveBallLaunchA(Carried *cr)
{
    wave_ball_a(cr);
}

void abi_ObjectWaveBallLaunchA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWaveBallLaunchA(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:9D76: the second gate's wave ball (ObjectWaveBallStep), $1E00 back. */
static void wave_ball_b(Carried *cr)
{
    wave_ball(cr, 0x9E9A, 0x2300, 0xFC40, 0xE200, 0x20, 0xFF60, 0xF100, 0x0F00, 0x0A);
}

/*
 * @routine 09:9D76 ObjectWaveBallLaunchB
 * WHAT IT DOES: As ObjectWaveBallLaunchA for the second gate: handler
 * ObjectWaveBallStep, $2300 right, $3C0 up, $1E00 back, scale $20, rise
 * -$A0, life $0A; the gate moves $F00 back in X and on in Z.
 * HOW: As ObjectWaveBallLaunchA.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectWaveBallLaunchA.
 */
void ObjectWaveBallLaunchB(Carried *cr)
{
    wave_ball_b(cr);
}

void abi_ObjectWaveBallLaunchB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWaveBallLaunchB(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:9E08: the third gate's wave ball, $1E00 on. */
static void wave_ball_c(Carried *cr)
{
    wave_ball(cr, 0x9E9A, 0x2300, 0xFC40, 0x1E00, 0x60, 0xFF60, 0xF100, 0xF100, 0x0C);
}

/*
 * @routine 09:9E08 ObjectWaveBallLaunchC
 * WHAT IT DOES: As ObjectWaveBallLaunchB for the third gate: the ball $1E00
 * on, scale $60, life $0C; the gate moves $F00 back in X and Z.
 * HOW: As ObjectWaveBallLaunchA.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectWaveBallLaunchA.
 */
void ObjectWaveBallLaunchC(Carried *cr)
{
    wave_ball_c(cr);
}

void abi_ObjectWaveBallLaunchC(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWaveBallLaunchC(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A wave gate's frame: on a hit on lap 1, the first time (+$25 bit 0),
   its wave ball. */
static void wave_gate_frame(Carried *cr, int which)
{
    if (!hit_to(cr))
        { cyc(78); return; }
    ld8(cr, db(cr) | W(wram_0DEA));
    cyc(54);
    cmp8(cr, 0x01);
    if ((uint8_t)cr->a != 0x01)
        { cyc(28); return; }
    lda8(cr, (uint8_t)(ld8(cr, F_c(0x2025, cr->x)) & 0x01));
    if ((uint8_t)cr->a)
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    if (which == 0)
        wave_ball_a(cr);
    else if (which == 1)
        wave_ball_b(cr);
    else
        wave_ball_c(cr);
    cyc(86);
}

/* 09:9BF5: the wave gate's record ($42D) with flag bit 4 at ($F00, $F00,
   $F00); then the first gate's frame. */
static void wave_gate_record(Carried *cr)
{
    post_record(cr, 0x0F00);
    wave_gate_frame(cr, 0);
}

/*
 * @routine 09:9BD7 ObjectWaveGateA
 * WHAT IT DOES: Sets up the first wave gate: restart $09:9BF5
 * (ObjectWaveGateRecord), frame $09:9C1C (ObjectWaveGateAStep); its record
 * and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectWaveGateA(Carried *cr)
{
    handlers2(cr, 0x9BF5, 0x9C1C);
    wave_gate_record(cr);
}

void abi_ObjectWaveGateA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWaveGateA(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9BF5 ObjectWaveGateRecord
 * WHAT IT DOES: A wave gate's restart (all three use it): its record
 * ($42D, flag bit 4, at ($F00, $F00, $F00)), then the first gate's frame
 * (ObjectWaveGateAStep).
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectWaveGateA. Exit: as ObjectWaveGateA.
 */
void ObjectWaveGateRecord(Carried *cr)
{
    wave_gate_record(cr);
}

void abi_ObjectWaveGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWaveGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9C1C ObjectWaveGateAStep
 * WHAT IT DOES: The first wave gate's frame: when a car hits it on lap 1
 * ($0DEA), the first time (+$25 bit 0), a wave ball (kind $AA34, handler
 * ObjectWaveBallBig) $2580 left, $180 up and $2300 back, and the gate moves
 * $F00 on in X and Z.
 * HOW: ObjectSyncToRecord, RecordContactSummary, ObjectSpawn.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectWaveGateAStep(Carried *cr)
{
    wave_gate_frame(cr, 0);
}

void abi_ObjectWaveGateAStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWaveGateAStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9C4A ObjectWaveGateB
 * WHAT IT DOES: Sets up the second wave gate: restart $09:9BF5
 * (ObjectWaveGateRecord), frame $09:9C69 (ObjectWaveGateBStep).
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 9.
 */
void ObjectWaveGateB(Carried *cr)
{
    handlers2(cr, 0x9BF5, 0x9C69);
    cyc(44);
}

void abi_ObjectWaveGateB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWaveGateB(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9C69 ObjectWaveGateBStep
 * WHAT IT DOES: As ObjectWaveGateAStep with a smaller ball (handler
 * ObjectWaveBallStep, $2300 right, $3C0 up, $1E00 back, scale $20, life
 * $A); the gate moves $F00 back in X and on in Z.
 * HOW: As ObjectWaveGateAStep.
 * Entry: as ObjectWaveGateAStep. Exit: as ObjectWaveGateAStep.
 */
void ObjectWaveGateBStep(Carried *cr)
{
    wave_gate_frame(cr, 1);
}

void abi_ObjectWaveGateBStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWaveGateBStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9C97 ObjectWaveGateC
 * WHAT IT DOES: Sets up the third wave gate: restart $09:9BF5
 * (ObjectWaveGateRecord), frame $09:9CB6 (ObjectWaveGateCStep).
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = 9.
 */
void ObjectWaveGateC(Carried *cr)
{
    handlers2(cr, 0x9BF5, 0x9CB6);
    cyc(44);
}

void abi_ObjectWaveGateC(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWaveGateC(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9CB6 ObjectWaveGateCStep
 * WHAT IT DOES: As ObjectWaveGateBStep with the ball $1E00 on (scale $60,
 * life $C); the gate moves $F00 back in X and Z.
 * HOW: As ObjectWaveGateAStep.
 * Entry: as ObjectWaveGateAStep. Exit: as ObjectWaveGateAStep.
 */
void ObjectWaveGateCStep(Carried *cr)
{
    wave_gate_frame(cr, 2);
}

void abi_ObjectWaveGateCStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWaveGateCStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:9EA6: the ball homes on Y (X, Z, Y eased by ApproachQuarterMin4),
   rises by +$29, and is released when +$0A runs out. */
static void wave_home(Carried *cr)
{
    (cyc(46), ease16(cr, F_c(0x200C, cr->x), F_c(0x200C, cr->y), ApproachQuarterMin4));
    (cyc(22), ease16(cr, F_c(0x2010, cr->x), F_c(0x2010, cr->y), ApproachQuarterMin4));
    (cyc(22), ease16(cr, F_c(0x200E, cr->x), F_c(0x200E, cr->y), ApproachQuarterMin4));
    add16_via(cr, F_c(0x200E, cr->x), F_c(0x2029, cr->x));
    if (dec8(cr, cyc_then(44, F_c(0x200A, cr->x))) == 0)
        { cyc(84); ObjectRelease_cr(cr); cyc(42); } else { cyc(90); }   /* RTS */
}

/*
 * @routine 09:9EA6 ObjectWaveBallHome
 * WHAT IT DOES: The wave ball homes on Y: X, Z and Y eased toward Y's
 * (ApproachQuarterMin4), rises by +$29, and is released when +$0A runs out.
 * HOW: 16-bit eases and adds, a byte DEC; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectWaveBallHome(Carried *cr)
{
    wave_home(cr);
}

void abi_ObjectWaveBallHome(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWaveBallHome(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9E9A ObjectWaveBallStep
 * WHAT IT DOES: A wave ball's frame: it homes on its gate (+$27; X, Z and
 * Y eased by ApproachQuarterMin4), rises by +$29, is released when its
 * life +$0A runs out; rebucketed.
 * HOW: The NOP first, as the course pointers expect. Reached only by
 * unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket.
 */
void ObjectWaveBallStep(Carried *cr)
{
    cr->y = rd16(cyc_then(44, F_c(0x2027, cr->x)));
    set_p(cr, nz16(cr->p, cr->y));
    wave_home(cr);
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

void abi_ObjectWaveBallStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWaveBallStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:A03F: a sparkle's frame: released when +$0A runs out; its frame
   +$18 steps through 0-5 (bit 7 kept); rebucketed. */
static void sparkle_frame(Carried *cr)
{
    uint8_t v;

    if (dec8(cr, cyc_then(22, F_c(0x200A, cr->x))) == 0)
        { cyc(84); ObjectRelease_cr(cr); cyc(22); } else { cyc(70); }
    v = ld8(cr, F_c(0x2018, cr->x));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    v = alu_adc8(v, 0x01, &cr->p);
    if (!(v & 0x80)) {
        cyc(24);
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
        v = alu_adc8(v, 0x06, &cr->p);
    }
    v &= 0x7F;
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cmp8(cr, 0x06);
    if (v >= 0x06) {
        cyc(8);
        set_p(cr, (uint8_t)(cr->p | (P_C)));
        v = alu_sbc8(v, 0x06, &cr->p);
    }
    v |= 0x80;
    lda8(cr, v);
    wr8(F_c(0x2018, cr->x), cyc_then(132, v));
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

/* 09:9FE1: a sparkle: a new object of kind $A158 lit by $997C (+$5F
   $64), frame +$18 = $82, at object X's position $C8 back, life 8,
   handler $09:A03F (ObjectStarSparkleStep). */
static void sparkle(Carried *cr)
{
    cyc(68);
    if (!spawn(cr, 0xA158))
        { cyc(26); return; } else { cyc(22); }
    st16i(cr, F_c(0x205D, cr->y), 0x997C);
    lda8(cr, 0x02);
    lda8(cr, 0x82);
    wr8(F_c(0x2018, cr->y), cyc_then(44, 0x82));
    lamp(cr, cr->y, 0x64);
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    add16(cr, F_c(0x2010, cr->y), cyc_then(22, 0xFF38));
    st8i(cr, F_c(0x200A, cr->y), cyc_then(28, 0x08));
    cyc(22);
    handler(cr, cr->y, 0xA03F);
}

/*
 * @routine 09:9FE1 ObjectStarSparkleSpawn
 * WHAT IT DOES: Leaves a sparkle: a new object of kind $A158 lit by $997C
 * (+$5F = $64), frame +$18 = $82, at object X's position $C8 back, life 8,
 * handler ObjectStarSparkleStep; nothing without a free object.
 * HOW: ObjectSpawn, ObjectCopyPositionTo; ends RTS.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; Y = the
 * new object; A low byte = 9.
 */
void ObjectStarSparkleSpawn(Carried *cr)
{
    sparkle(cr);
}

void abi_ObjectStarSparkleSpawn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectStarSparkleSpawn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:9F63: the big wave ball's frame: once hit (+$25 bit 0) it grows
   (+$16, +$15 by $18), flies ($32 back, $28A right, $FA up) and leaves a
   sparkle each frame; before, it homes on its gate. A hit (its position
   given to the record) sets +$25 bit 0 the first time with sound $59. */
static void big_ball_frame(Carried *cr)
{
    int late;

    lda8(cr, (uint8_t)(ld8(cr, cyc_then(22, F_c(0x2025, cr->x))) & 0x01));
    if ((uint8_t)cr->a) {
        cyc(18);
        add8(cr, F_c(0x2016, cr->x), 0x18);
        add8(cr, F_c(0x2015, cr->x), 0x18);
        add16(cr, F_c(0x2010, cr->x), cyc_then(22, 0xFFCE));
        add16(cr, F_c(0x200C, cr->x), cyc_then(44, 0x028A));
        add16(cr, F_c(0x200E, cr->x), cyc_then(44, 0xFF06));
        sparkle(cr);
        cyc(64);
    } else {
        uint16_t sy = cr->y;

        cr->y = rd16(cyc_then(104, F_c(0x2027, cr->x)));
        set_p(cr, nz16(cr->p, cr->y));
        wave_home(cr);
        cr->y = sy;
        set_p(cr, nz16(cr->p, sy));
        cyc(36);
    }
    if (!hit_to_n(cr, &late))
        { cyc(late ? 34 : 78); return; } else { cyc(6); }
    lda8(cr, cyc_then(18, (uint8_t)(ld8(cr, F_c(0x2025, cr->x)) & 0x01)));
    if ((uint8_t)cr->a)
        { cyc(70); return; }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    cyc(58);
    lda8(cr, 0x59);
    SoundPort0QueuePush_cr(cr);
    cyc(44);
}

/* 09:9F3C: the big ball's record ($42D) with flag bit 4 at ($320, $320,
   $320); then its frame. */
static void big_ball_record(Carried *cr)
{
    post_record(cr, 0x0320);
    big_ball_frame(cr);
}

/* 09:9F14: restart $09:9F3C, frame $09:9F63, rise +$29 = -$50; then the
   record and frame. */
static void big_ball_setup(Carried *cr)
{
    handlers2(cr, 0x9F3C, 0x9F63);
    st16i(cr, F_c(0x2029, cr->x), cyc_then(22, 0xFFB0));
    (cyc(22), big_ball_record(cr));
}

/*
 * @routine 09:9F05 ObjectWaveBallBig
 * WHAT IT DOES: The first wave gate's ball: its handler $09:9F14
 * (ObjectWaveBallBigSetup), then that set-up at once.
 * HOW: Word and byte stores. Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectWaveBallBig(Carried *cr)
{
    cyc(22);
    handler(cr, cr->x, 0x9F14);
    big_ball_setup(cr);
}

void abi_ObjectWaveBallBig(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWaveBallBig(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9F14 ObjectWaveBallBigSetup
 * WHAT IT DOES: The big wave ball's set-up: restart $09:9F3C
 * (ObjectWaveBallBigRecord), frame $09:9F63 (ObjectWaveBallBigStep), rise
 * +$29 = -$50; its record ($42D, flag bit 4, at ($320, $320, $320)) and
 * frame.
 * HOW: Word and byte stores. Reached only by unit_objects_09.
 * Entry: as ObjectWaveBallBig. Exit: as ObjectWaveBallBig.
 */
void ObjectWaveBallBigSetup(Carried *cr)
{
    big_ball_setup(cr);
}

void abi_ObjectWaveBallBigSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWaveBallBigSetup(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9F3C ObjectWaveBallBigRecord
 * WHAT IT DOES: The big wave ball's restart: its record ($42D, flag bit 4,
 * at ($320, $320, $320)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_objects_09.
 * Entry: as ObjectWaveBallBig. Exit: as ObjectWaveBallBig.
 */
void ObjectWaveBallBigRecord(Carried *cr)
{
    big_ball_record(cr);
}

void abi_ObjectWaveBallBigRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWaveBallBigRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:9F63 ObjectWaveBallBigStep
 * WHAT IT DOES: The big wave ball's frame: once a car has hit it (+$25 bit
 * 0) it grows (+$16, +$15 by $18), flies off ($32 back, $28A right, $FA
 * up) and leaves a sparkle (kind $A158, handler ObjectStarSparkleStep) each
 * frame; until then it homes on its gate. A hit sets +$25 bit 0 the first
 * time with sound $59.
 * HOW: ObjectSyncToRecord, RecordContactSummary. Reached only by
 * unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectWaveBallBigStep(Carried *cr)
{
    big_ball_frame(cr);
}

void abi_ObjectWaveBallBigStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWaveBallBigStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A03F ObjectStarSparkleStep
 * WHAT IT DOES: A sparkle's frame: released when its life +$0A runs out;
 * its picture frame +$18 steps through 0-5 (bit 7 kept); rebucketed.
 * HOW: Byte adds and compares. Reached only by unit_objects_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket.
 */
void ObjectStarSparkleStep(Carried *cr)
{
    sparkle_frame(cr);
}

void abi_ObjectStarSparkleStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectStarSparkleStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A0FA ObjectLanternBurst
 * WHAT IT DOES: Bursts the lantern into four sparks (kind $A0CC, lit by
 * $996B, handler ObjectFlyingPiece, +$0A = $0F) flying out at ($28, -$2D,
 * 0), (-$28, -$2D, 0), (0, -$32, $28) and (0, -$32, -$28); a spark is
 * skipped without a free object.
 * HOW: Four ObjectSpawn blocks, each from object X's position.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last spark
 * leaves it.
 */
void ObjectLanternBurst(Carried *cr)
{
    cyc(46);
    spark(cr, 0x14, 0x0028, 0xFFD3, 0x0000);
    spark(cr, 0x03, 0xFFD8, 0xFFD3, 0x0000);
    spark(cr, 0x0A, 0x0000, 0xFFCE, 0x0028);
    spark(cr, 0x01, 0x0000, 0xFFCE, 0xFFD8);
}

void abi_ObjectLanternBurst(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLanternBurst(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:A0C0: the lantern's frame: when a car hits it the first time (+$25
   bit 0), sound $10, it is released and bursts into four sparks; no hit
   clears the bit. */
static void lantern_frame(Carried *cr)
{
    if (!hit(cr)) {
        cyc(18);
        and8(cr, F_c(0x2025, cr->x), 0xFE);
        { cyc(44); return; }
    } else { cyc(34); }
    lda8(cr, (uint8_t)(ld8(cr, F_c(0x2025, cr->x)) & 0x01));
    if ((uint8_t)cr->a)
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    cyc(58);
    lda8(cr, 0x10);
    SoundPort0QueuePush_cr(cr);
    (cyc(62), ObjectRelease_cr(cr));
    ObjectLanternBurst(cr);
    cyc(86);                            /* RTS, RTL */
}

/* 09:A0B2: the lantern's record ($424, +$0A $90E8) with flag bit 4 and
   +$2A high 2; then its frame. */
static void lantern_record(Carried *cr)
{
    (cyc(62), GsuRecordNew424Alt_cr(cr));
    (cyc(62), GsuRecordFlag10_cr(cr));
    lda8(cr, 0x02);
    (cyc(58), GsuRecordSet2AHigh_cr(cr));
    (cyc(102), lantern_frame(cr));
}

/*
 * @routine 09:A06A ObjectLantern
 * WHAT IT DOES: Sets up a lantern: $78 higher, lit by $996D (+$24 bit 3,
 * +$3A = 0, +$5F = $64), restart $09:A0B2 (ObjectLanternRecord), frame
 * $09:A0C0 (ObjectLanternStep); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectLantern(Carried *cr)
{
    add16(cr, F_c(0x200E, cr->x), cyc_then(22, 0xFF88));
    st16i(cr, F_c(0x205D, cr->x), cyc_then(44, 0x996D));
    cyc(22);
    lamp(cr, cr->x, 0x64);
    handlers2(cr, 0xA0B2, 0xA0C0);
    lantern_record(cr);
}

void abi_ObjectLantern(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLantern(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A0B2 ObjectLanternRecord
 * WHAT IT DOES: The lantern's restart: its record (GsuRecordNew424Alt, flag
 * bit 4, +$2A high 2), then its frame.
 * HOW: GsuRecordNew424Alt, GsuRecordFlag10, GsuRecordSet2AHigh.
 * Entry: as ObjectLantern. Exit: as ObjectLantern.
 */
void ObjectLanternRecord(Carried *cr)
{
    lantern_record(cr);
}

void abi_ObjectLanternRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLanternRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A0C0 ObjectLanternStep
 * WHAT IT DOES: The lantern's frame: when a car hits it the first time
 * (+$25 bit 0), sound $10, it is released and bursts into four sparks
 * (kind $A0CC, handler ObjectFlyingPiece); no hit clears the bit.
 * HOW: ObjectSyncFromRecord, RecordContactSummary, ObjectSpawn.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectLanternStep(Carried *cr)
{
    lantern_frame(cr);
}

void abi_ObjectLanternStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLanternStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ---- 09:A293-B0A0: the course gates and markers ---- */

void abi_SoundPort1QueuePush(void);
void abi_GsuRecordLower82(void);
void abi_ObjectCopyPositionFrom(void);
void abi_ObjectStep48To8(void);
void abi_HudCountMove19EECopy(void);
void abi_GsuRecordFlags8ASet(void);
void abi_GsuRecordFlags8AClear(void);

/* LDA #v, STA addr with M already 16-bit. */
static void st16w(Carried *cr, uint32_t addr, uint16_t v)
{
    r_lda16_imm(cr, v);
    cyc(10);
    wr16(addr, v);
}

/* LDA #v, JSL SoundPort1QueuePush. */
static void sound1(Carried *cr, uint8_t v)
{
    cyc(58);
    lda8(cr, v);
    SoundPort1QueuePush_cr(cr);
}

/* LDA #v, JSL SoundPort0QueuePush. */
static void sound0(Carried *cr, uint8_t v)
{
    cyc(58);
    lda8(cr, v);
    SoundPort0QueuePush_cr(cr);
}

/* LDA +$25, AND #m: whether bit m of the gate's +$25 is set. */
static int bit25(Carried *cr, uint8_t m)
{
    lda8(cr, (uint8_t)(ld8(cr, cyc_then(22, F_c(0x2025, cr->x))) & m));
    return (uint8_t)cr->a != 0;
}

/* SEP #$20, LDA $0DEA, CMP #lap: whether this is lap `lap`. */
static int lap_is(Carried *cr, uint8_t lap)
{
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    ld8(cr, (cyc(14), db(cr)) | W(wram_0DEA));
    cyc(10);
    cmp8(cr, lap);
    cyc(38);
    return (uint8_t)cr->a == lap;
}

/* As lap_is, with the time before the read of $0DEA (`pre`) and after the
   CMP when the lap is the one (`eq`) or is not (`ne`) given by the caller:
   they differ with the branch the original takes and with what follows. */
static int lap_is_x(Carried *cr, uint8_t lap, unsigned pre, unsigned eq, unsigned ne)
{
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    ld8(cr, (cyc(pre), db(cr)) | W(wram_0DEA));
    cyc(10);
    cmp8(cr, lap);
    cyc((uint8_t)cr->a == lap ? eq : ne);
    return (uint8_t)cr->a == lap;
}

/* As lap_is, for a caller whose BNE not taken goes straight on to the next
   LDA: the 38 is spent only when the lap differs. */
static int lap_is_next(Carried *cr, uint8_t lap)
{
    return lap_is_x(cr, lap, 14, 0, 38);
}

/* A gate's record: a new one ($42D) with flag bit 4 at (s, s, s). */
static void gate42d(Carried *cr, uint16_t s)
{
    cyc(62);
    GsuRecordNew42D_cr(cr);
    GsuRecordFlag10_cr(cr);
    record_at(cr, s, s, s);
}

/* ObjectSyncFromRecord, AND #1: whether the record has contacts. */
static int synced(Carried *cr)
{
    (cyc(62), ObjectSyncFromRecord_cr(cr));
    cyc(10);
    cr->a = (uint16_t)((cr->a & 0xFF00) | (cr->a & 0x01));
    set_p(cr, nz8(cr->p, (uint8_t)cr->a));
    return (uint8_t)cr->a != 0;
}

/* RecordNextContact until a contact whose flags AND #1 are set: 1 when
   one is found, 0 when the list runs out (carry clear). `tail` is the time
   of the BCC, AND and BEQ that follow a contact found, and of the first
   instructions after them, which differs by caller. */
static int next_contact_t(Carried *cr, unsigned tail)
{
    cyc(84);
    for (;;) {
        RecordNextContact_cr(cr);
        if (!(cr->p & P_C))
            return 0; else { cyc(tail); }
        lda8(cr, (uint8_t)(cr->a & 0x01));
        if ((uint8_t)cr->a)
            { cyc(0); return 1; }
        cyc(116 - tail - 20);           /* BCC, AND, BEQ taken, JSL */
    }
}

static int next_contact(Carried *cr)
{
    return next_contact_t(cr, 50);
}

/* REP #$20, A = n (16-bit), JSL to: a car record's +$8A bits set or
   cleared (08:B72E / 08:B743). */
static void car_bits16(Carried *cr, uint16_t n, void (*to)(Carried *))
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16_imm(cr, n);
    to(cr);
}

/* The slide's step, target and direction ($19F8, $19FA, $19FC), M
   16-bit. */
static void slide_set(Carried *cr, uint16_t step, uint16_t target, uint16_t dir)
{
    st16w(cr, db(cr) | W(slide_step), step);
    st16w(cr, db(cr) | W(slide_target), target);
    st16w(cr, db(cr) | W(slide_direction), dir);
}

/* The fade's step $1A00 and end $1A02, M 16-bit. */
static void fade_set(Carried *cr, uint16_t step, uint16_t end)
{
    st16w(cr, db(cr) | W(wram_1A00), step);
    st16w(cr, db(cr) | W(wram_1A02), end);
}

/* SEP #$20, $1A04 = $FF, $1A05 = 0. */
static void slide_hold(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    st8i(cr, (cyc(22), db(cr)) | W(wram_1A04), 0xFF);
    st8i(cr, db(cr) | W(slide_wobble_count), 0x00);
}

/* 09:A2D8: the chime gate's frame: a car's hit, the first time (+$25 bit
   0, clearing bit 1), sound $13 on port 1; no hit, the first time (+$25
   bit 1, clearing bit 0), sound $0E. */
static void chime_frame(Carried *cr)
{
    if (hit(cr)) {
        cyc(12);
        if (bit25(cr, 0x01))
            { cyc(88); return; } else { cyc(18); }
        or8(cr, F_c(0x2025, cr->x), 0x01);
        and8(cr, F_c(0x2025, cr->x), 0xFD);
        sound1(cr, 0x13);
        { cyc(44); return; }
    } else { cyc(18); }
    if (bit25(cr, 0x02))
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x02);
    and8(cr, F_c(0x2025, cr->x), 0xFE);
    sound1(cr, 0x0E);
    cyc(44);
}

/*
 * @routine 09:A293 ObjectChimeGate
 * WHAT IT DOES: Sets up a chime gate: restart $09:A2B1
 * (ObjectChimeGateRecord), frame $09:A2D8 (ObjectChimeGateStep); its record
 * and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectChimeGate(Carried *cr)
{
    handlers2(cr, 0xA2B1, 0xA2D8);
    gate42d(cr, 0x0F00);
    chime_frame(cr);
}

void abi_ObjectChimeGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectChimeGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A2B1 ObjectChimeGateRecord
 * WHAT IT DOES: The chime gate's restart: its record ($42D, flag bit 4, at
 * ($F00, $F00, $F00)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectChimeGate. Exit: as ObjectChimeGate.
 */
void ObjectChimeGateRecord(Carried *cr)
{
    gate42d(cr, 0x0F00);
    chime_frame(cr);
}

void abi_ObjectChimeGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectChimeGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A2D8 ObjectChimeGateStep
 * WHAT IT DOES: The chime gate's frame: when a car enters it (contact bit
 * 0) the first time, sound $13 on port 1; when it leaves, sound $0E; +$25
 * bit 0 marks inside, bit 1 outside.
 * HOW: ObjectSyncFromRecord, RecordContactSummary, SoundPort1QueuePush.
 * Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectChimeGateStep(Carried *cr)
{
    chime_frame(cr);
}

void abi_ObjectChimeGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectChimeGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:A371 / A408: a flash gate's frame: a car's hit, the first time (+$25
   bit 0), flashes the screen ($19C5 = flash step) and starts a fade
   ($1A00 step, $1A02 end; $0DA5 bits 0 and 5); no hit clears the bit.
   `rep2`: a second REP #$20 before the fade's stores. */
static void flash_frame(Carried *cr, uint16_t flash, uint16_t step, uint16_t end, int rep2)
{
    if (hit(cr)) {
        cyc(12);
        if (bit25(cr, 0x01))
            { cyc(88); return; } else { cyc(18); }
        or8(cr, F_c(0x2025, cr->x), 0x01);
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cyc(30);
        st16w(cr, W(wram_19C5), flash);
        if (rep2)
            cyc(22);                            /* the bright gate's second REP */
        fade_set(cr, step, end);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        or8x(cr, (cyc(16), db(cr)) | W(slide_flags), 0x21, 16);
        { cyc(44); return; }
    } else { cyc(40); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    and8(cr, F_c(0x2025, cr->x), 0xFE);
    cyc(44);
}

/*
 * @routine 09:A32C ObjectBrightGate
 * WHAT IT DOES: Sets up a bright gate: restart $09:A34A
 * (ObjectBrightGateRecord), frame $09:A371 (ObjectBrightGateStep); its
 * record and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBrightGate(Carried *cr)
{
    handlers2(cr, 0xA34A, 0xA371);
    gate42d(cr, 0x0780);
    flash_frame(cr, 0x0020, 0x0200, 0x2000, 1);
}

void abi_ObjectBrightGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBrightGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A34A ObjectBrightGateRecord
 * WHAT IT DOES: The bright gate's restart: its record ($42D, flag bit 4,
 * at ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectBrightGate. Exit: as ObjectBrightGate.
 */
void ObjectBrightGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    flash_frame(cr, 0x0020, 0x0200, 0x2000, 1);
}

void abi_ObjectBrightGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBrightGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A371 ObjectBrightGateStep
 * WHAT IT DOES: The bright gate's frame: a car's hit, the first time (+$25
 * bit 0), brightens the screen (flash step $19C5 = $20, fade by $200 up to
 * $2000, $0DA5 bits 0 and 5); no hit clears the bit.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; the flash word by a long
 * store. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectBrightGateStep(Carried *cr)
{
    flash_frame(cr, 0x0020, 0x0200, 0x2000, 1);
}

void abi_ObjectBrightGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBrightGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A3C3 ObjectDarkGate
 * WHAT IT DOES: Sets up a dark gate: restart $09:A3E1
 * (ObjectDarkGateRecord), frame $09:A408 (ObjectDarkGateStep); its record
 * and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectDarkGate(Carried *cr)
{
    handlers2(cr, 0xA3E1, 0xA408);
    gate42d(cr, 0x0780);
    flash_frame(cr, 0xFFE0, 0xFE00, 0xFE00, 0);
}

void abi_ObjectDarkGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectDarkGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A3E1 ObjectDarkGateRecord
 * WHAT IT DOES: The dark gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectDarkGate. Exit: as ObjectDarkGate.
 */
void ObjectDarkGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    flash_frame(cr, 0xFFE0, 0xFE00, 0xFE00, 0);
}

void abi_ObjectDarkGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectDarkGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A408 ObjectDarkGateStep
 * WHAT IT DOES: The dark gate's frame: a car's hit, the first time (+$25
 * bit 0), darkens the screen (flash step $19C5 = -$20, fade by -$200 down
 * to $FE00, $0DA5 bits 0 and 5); no hit clears the bit.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; the flash word by a long
 * store.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectDarkGateStep(Carried *cr)
{
    flash_frame(cr, 0xFFE0, 0xFE00, 0xFE00, 0);
}

void abi_ObjectDarkGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectDarkGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:A49D / A52C: a car flag gate's frame: a car's contact (bit 0), the
   first time (+$25 bit 0), sets or clears bit 1 of the car's record +$8A
   (unless $7F:3460 is set) and plays `snd` on port 1; no contact clears
   the bit. `on` picks set (08:B72E) or clear (08:B743). */
static void car_flag_frame(Carried *cr, int on, uint8_t snd)
{
    if (!synced(cr) || !next_contact_t(cr, 28)) {
        cyc(28);
        and8(cr, F_c(0x2025, cr->x), 0xFE);
        cyc(44);
        return;
    }
    if (bit25(cr, 0x01))
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(46);
    if (!ld8(cr, W(wram_7F3460)))
        { cyc(16); car_bits16(cr, 0x0002, on ? (cyc(12), GsuRecordFlags8ASet) : GsuRecordFlags8AClear); } else { cyc(44); }
    if (!on)
        { cyc(22); set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M))); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    sound1(cr, snd);
    cyc(44);
}

/*
 * @routine 09:A458 ObjectCarFlagOffGate
 * WHAT IT DOES: Sets up a gate that takes bit 1 of a car's record +$8A
 * away: restart $09:A476 (ObjectCarFlagOffGateRecord), frame $09:A49D
 * (ObjectCarFlagOffGateStep); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectCarFlagOffGate(Carried *cr)
{
    handlers2(cr, 0xA476, 0xA49D);
    gate42d(cr, 0x0780);
    car_flag_frame(cr, 0, 0x11);
}

void abi_ObjectCarFlagOffGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarFlagOffGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A476 ObjectCarFlagOffGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectCarFlagOffGate. Exit: as ObjectCarFlagOffGate.
 */
void ObjectCarFlagOffGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    car_flag_frame(cr, 0, 0x11);
}

void abi_ObjectCarFlagOffGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarFlagOffGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A49D ObjectCarFlagOffGateStep
 * WHAT IT DOES: The gate's frame: the first car contact (contact flags bit
 * 0; +$25 bit 0 marks it) clears bit 1 of that car's record +$8A unless
 * $7F:3460 is set, and plays sound $11 on port 1; no contact clears the
 * mark.
 * HOW: ObjectSyncFromRecord, then RecordNextContact over the contacts;
 * 08:B743 with A = 2 (16-bit) and Y = the touched record.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectCarFlagOffGateStep(Carried *cr)
{
    car_flag_frame(cr, 0, 0x11);
}

void abi_ObjectCarFlagOffGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarFlagOffGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A4E7 ObjectCarFlagOnGate
 * WHAT IT DOES: Sets up a gate that gives a car's record +$8A bit 1:
 * restart $09:A505 (ObjectCarFlagOnGateRecord), frame $09:A52C
 * (ObjectCarFlagOnGateStep); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectCarFlagOnGate(Carried *cr)
{
    handlers2(cr, 0xA505, 0xA52C);
    gate42d(cr, 0x0780);
    car_flag_frame(cr, 1, 0x10);
}

void abi_ObjectCarFlagOnGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarFlagOnGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A505 ObjectCarFlagOnGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectCarFlagOnGate. Exit: as ObjectCarFlagOnGate.
 */
void ObjectCarFlagOnGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    car_flag_frame(cr, 1, 0x10);
}

void abi_ObjectCarFlagOnGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarFlagOnGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A52C ObjectCarFlagOnGateStep
 * WHAT IT DOES: The gate's frame: the first car contact (contact flags bit
 * 0; +$25 bit 0 marks it) sets bit 1 of that car's record +$8A unless
 * $7F:3460 is set, and plays sound $10 on port 1; no contact clears the
 * mark.
 * HOW: ObjectSyncFromRecord, then RecordNextContact over the contacts;
 * 08:B72E with A = 2 (16-bit) and Y = the touched record.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectCarFlagOnGateStep(Carried *cr)
{
    car_flag_frame(cr, 1, 0x10);
}

void abi_ObjectCarFlagOnGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCarFlagOnGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:A5B9: the lap 1 slide gate's frame: on lap 1 the first car contact
   (+$25 bit 0) clears bit 1 of the car's record +$8A and starts the slide
   down ($0DA5 bits 5 and 7: step $40 to -$40, direction $8000) and a fade
   ($1A00 = $20 up to $2000). */
static void slide1_frame(Carried *cr)
{
    if (!synced(cr) || !next_contact_t(cr, 52))
        { cyc(72); return; }
    if (!lap_is_next(cr, 0x01))
        { cyc(34); return; } else { cyc(22); }
    if (bit25(cr, 0x01))
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(24);
    or8l(cr, W(slide_flags), 0xA0, 16);
    car_bits16(cr, 0x0002, GsuRecordFlags8AClear);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    fade_set(cr, 0x0020, 0x2000);
    slide_set(cr, 0x0040, 0xFFC0, 0x8000);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(66);
}

/*
 * @routine 09:A574 ObjectSlideGate
 * WHAT IT DOES: Sets up the lap 1 slide gate: restart $09:A592
 * (ObjectSlideGateRecord), frame $09:A5B9 (ObjectSlideGateStep); its record
 * and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectSlideGate(Carried *cr)
{
    handlers2(cr, 0xA592, 0xA5B9);
    gate42d(cr, 0x0780);
    slide1_frame(cr);
}

void abi_ObjectSlideGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A592 ObjectSlideGateRecord
 * WHAT IT DOES: The slide gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectSlideGate. Exit: as ObjectSlideGate.
 */
void ObjectSlideGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    slide1_frame(cr);
}

void abi_ObjectSlideGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A5B9 ObjectSlideGateStep
 * WHAT IT DOES: The slide gate's frame: on lap 1 ($0DEA) the first car
 * contact (+$25 bit 0 marks it) clears bit 1 of the car's record +$8A,
 * starts the slide down ($0DA5 bits 5 and 7; step $40 to target -$40,
 * direction $8000) and a fade (step $20 up to $2000).
 * HOW: ObjectSyncFromRecord, RecordNextContact; 08:B743 with A = 2;
 * $0DA5 by long accesses.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectSlideGateStep(Carried *cr)
{
    slide1_frame(cr);
}

void abi_ObjectSlideGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A6BE GateFlashOnClock
 * WHAT IT DOES: When bit 4 of the race clock's fraction $0E63 is clear,
 * sweeps the screen ($0DA5 bit 4) with sound $36.
 * HOW: Four LSRs and AND #1 (the carry is the fraction's bit 3); $0DA5 by
 * long accesses. Reached only by unit_gates_09.
 * Entry: X 16-bit. Exit: M 8-bit; carry = bit 3 of $0E63; bit 4 set: A
 * low byte = 1; else as SoundPort0QueuePush leaves it.
 */
void GateFlashOnClock(Carried *cr)
{
    uint8_t v;

    set_p(cr, (uint8_t)(cr->p | (P_M)));
    v = ld8(cr, (cyc(84), db(cr)) | W(race_time_fraction));
    set_p(cr, (uint8_t)((cr->p & ~P_C) | ((v & 0x08) ? P_C : (cyc(70), 0))));
    lda8(cr, (uint8_t)((v >> 4) & 0x01));
    if ((uint8_t)cr->a)
        { cyc(46); return; }
    or8l(cr, W(slide_flags), 0x10, 0);
    sound0(cr, 0x36);
    cyc(42);
}

void abi_GateFlashOnClock(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GateFlashOnClock(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A6DC GateFadeUp
 * WHAT IT DOES: Starts a fade ($0DA5 bit 5) by $80 up to $2000.
 * HOW: $0DA5 by long accesses; the words through the data bank.
 * Reached only by unit_gates_09.
 * Entry: X 16-bit. Exit: M 8-bit; A = $2000; N, Z from it.
 */
void GateFadeUp(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(70);
    or8l(cr, W(slide_flags), 0x20, 0);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(22);
    fade_set(cr, 0x0080, 0x2000);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

void abi_GateFadeUp(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GateFadeUp(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:A666: the lap gate's frame: a car's hit on lap 0 or 1, the first time
   (+$25 bit 0), GateFlashOnClock; on lap 2, the first time (+$25 bit 1),
   GateFadeUp and the gate is released. */
static void lap_flash_frame(Carried *cr)
{
    uint8_t lap;

    if (!hit(cr))
        { cyc(62); return; } else { cyc(50); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    lap = ld8(cr, db(cr) | W(wram_0DEA));
    cyc(38);
    cmp8(cr, 0x00);
    if (lap != 0x00) {
        cyc(32);
        cmp8(cr, 0x01);
        if (lap != 0x01) {
            cyc(22);
            cmp8(cr, 0x02);
            if (lap != 0x02)
                { cyc(72); return; } else { cyc(10); }
            if (bit25(cr, 0x02))
                { cyc(88); return; } else { cyc(18); }
            or8(cr, F_c(0x2025, cr->x), 0x02);
            GateFadeUp(cr);
            (cyc(126), ObjectRelease_cr(cr));
            { cyc(44); return; }
        }
    }
    if (bit25(cr, 0x01))
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    GateFlashOnClock(cr);
    cyc(68);
}

/*
 * @routine 09:A621 ObjectLapFlashGate
 * WHAT IT DOES: Sets up a gate that flashes on laps 0 and 1 and fades on
 * lap 2: restart $09:A63F (ObjectLapFlashGateRecord), frame $09:A666
 * (ObjectLapFlashGateStep); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectLapFlashGate(Carried *cr)
{
    handlers2(cr, 0xA63F, 0xA666);
    gate42d(cr, 0x0780);
    lap_flash_frame(cr);
}

void abi_ObjectLapFlashGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapFlashGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A63F ObjectLapFlashGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectLapFlashGate. Exit: as ObjectLapFlashGate.
 */
void ObjectLapFlashGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    lap_flash_frame(cr);
}

void abi_ObjectLapFlashGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapFlashGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A666 ObjectLapFlashGateStep
 * WHAT IT DOES: The gate's frame: a car's hit on lap 0 or 1 ($0DEA), the
 * first time (+$25 bit 0), sweeps the screen on even clock ticks
 * (GateFlashOnClock); on lap 2, the first time (+$25 bit 1), a fade
 * (GateFadeUp) and the gate is released.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; the lap compared with
 * 0, 1 and 2.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectLapFlashGateStep(Carried *cr)
{
    lap_flash_frame(cr);
}

void abi_ObjectLapFlashGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapFlashGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:A73E: a car's hit on lap 0, the first time (+$25 bit 0): a sweep
   ($0DA5 bit 4) with sound $36, and the gate is released; no hit clears
   the bit. */
static void sweep0_frame(Carried *cr)
{
    if (!hit(cr)) {
        cyc(18);
        and8(cr, F_c(0x2025, cr->x), 0xFE);
        { cyc(44); return; }
    } else { cyc(36); }
    if (!lap_is_next(cr, 0x00))
        { cyc(34); return; } else { cyc(22); }
    if (bit25(cr, 0x01))
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    cyc(2);
    or8l(cr, W(slide_flags), 0x10, 0);
    sound0(cr, 0x36);
    (cyc(62), ObjectRelease_cr(cr));
    cyc(44);
}

/*
 * @routine 09:A6F9 ObjectSweepGate
 * WHAT IT DOES: Sets up the lap 0 sweep gate: restart $09:A717
 * (ObjectSweepGateRecord), frame $09:A73E (ObjectSweepGateStep); its record
 * and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectSweepGate(Carried *cr)
{
    handlers2(cr, 0xA717, 0xA73E);
    gate42d(cr, 0x0780);
    sweep0_frame(cr);
}

void abi_ObjectSweepGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSweepGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A717 ObjectSweepGateRecord
 * WHAT IT DOES: The sweep gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectSweepGate. Exit: as ObjectSweepGate.
 */
void ObjectSweepGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    sweep0_frame(cr);
}

void abi_ObjectSweepGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSweepGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A73E ObjectSweepGateStep
 * WHAT IT DOES: The sweep gate's frame: a car's hit on lap 0, the first
 * time (+$25 bit 0), sweeps the screen ($0DA5 bit 4) with sound $36 and
 * releases the gate; no hit clears the bit.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; $0DA5 by long accesses.
 * Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectSweepGateStep(Carried *cr)
{
    sweep0_frame(cr);
}

void abi_ObjectSweepGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSweepGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:A7CD: the slide chime gate's frame: synced and summed (the summary's
   bit 0 is a hit): a hit, the first time (+$25 bit 1, clearing bit 2),
   sound $13 on port 1; then on lap 1 the first time (+$25 bit 0) the slide
   down ($0DA5 bits 5 and 7, step $80 to -$80, direction $8000, $1A04 =
   $FF) and a fade ($20 up to $2000). No hit: the first time (+$25 bit 2)
   sound $0E; +$25 bit 1 cleared. */
static void slide_chime_frame(Carried *cr)
{
    (cyc(62), ObjectSyncFromRecord_cr(cr));
    (cyc(62), RecordContactSummary_cr(cr));
    cyc(12);
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if ((uint8_t)cr->a) {
        if (!bit25(cr, 0x02)) {
            cyc(18);
            or8(cr, F_c(0x2025, cr->x), 0x02);
            and8(cr, F_c(0x2025, cr->x), 0xFB);
            sound1(cr, 0x13);
        } else { cyc(44); }
        cyc(24);
        if (!lap_is_next(cr, 0x01))
            { cyc(34); return; } else { cyc(22); }
        if (bit25(cr, 0x01))
            { cyc(88); return; } else { cyc(18); }
        or8(cr, F_c(0x2025, cr->x), 0x01);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(24);
        or8l(cr, W(slide_flags), 0xA0, 0);
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cyc(22);
        slide_set(cr, 0x0080, 0xFF80, 0x8000);
        slide_hold(cr);
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cyc(22);
        fade_set(cr, 0x0020, 0x2000);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        { cyc(66); return; }
    } else { cyc(28); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (!bit25(cr, 0x04)) {
        cyc(18);
        or8(cr, F_c(0x2025, cr->x), 0x04);
        sound1(cr, 0x0E);
    } else { cyc(44); }
    and8(cr, F_c(0x2025, cr->x), 0xFD);
    cyc(44);
}

/*
 * @routine 09:A788 ObjectSlideChimeGate
 * WHAT IT DOES: Sets up the slide chime gate: restart $09:A7A6
 * (ObjectSlideChimeGateRecord), frame $09:A7CD (ObjectSlideChimeGateStep);
 * its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectSlideChimeGate(Carried *cr)
{
    handlers2(cr, 0xA7A6, 0xA7CD);
    gate42d(cr, 0x0F00);
    slide_chime_frame(cr);
}

void abi_ObjectSlideChimeGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideChimeGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A7A6 ObjectSlideChimeGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($F00, $F00, $F00)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectSlideChimeGate. Exit: as ObjectSlideChimeGate.
 */
void ObjectSlideChimeGateRecord(Carried *cr)
{
    gate42d(cr, 0x0F00);
    slide_chime_frame(cr);
}

void abi_ObjectSlideChimeGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideChimeGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A7CD ObjectSlideChimeGateStep
 * WHAT IT DOES: The gate's frame: a car entering (contact summary bit 0)
 * the first time plays sound $13 on port 1 (+$25 bit 1 marks inside); on
 * lap 1 the first entry (+$25 bit 0) also starts the slide down (step $80
 * to -$80, $1A04 = $FF) and a fade ($20 up to $2000). Leaving plays sound
 * $0E the first time (+$25 bit 2 marks outside).
 * HOW: ObjectSyncFromRecord, RecordContactSummary (not tested on the
 * sync); $0DA5 by long accesses.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectSlideChimeGateStep(Carried *cr)
{
    slide_chime_frame(cr);
}

void abi_ObjectSlideChimeGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideChimeGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:A8BC / A94C: on lap `lap` the first car contact (+$25 bit 0) starts
   a fade ($0DA5 bit 5, $1A00 = step, $1A02 = end). */
static void fade_frame(Carried *cr, uint8_t lap, uint16_t step, uint16_t end)
{
    if (!synced(cr) || !next_contact_t(cr, 52))
        { cyc(72); return; }
    if (!lap_is_next(cr, lap))
        { cyc(34); return; } else { cyc(22); }
    if (bit25(cr, 0x01))
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(24);
    or8l(cr, W(slide_flags), 0x20, 0);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(22);
    fade_set(cr, step, end);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(66);
}

/*
 * @routine 09:A877 ObjectFadeGate0
 * WHAT IT DOES: Sets up the lap 0 fade gate: restart $09:A895
 * (ObjectFadeGate0Record), frame $09:A8BC (ObjectFadeGate0Step); its record
 * and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectFadeGate0(Carried *cr)
{
    handlers2(cr, 0xA895, 0xA8BC);
    gate42d(cr, 0x0780);
    fade_frame(cr, 0x00, 0x000A, 0x1FFE);
}

void abi_ObjectFadeGate0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFadeGate0(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A895 ObjectFadeGate0Record
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectFadeGate0. Exit: as ObjectFadeGate0.
 */
void ObjectFadeGate0Record(Carried *cr)
{
    gate42d(cr, 0x0780);
    fade_frame(cr, 0x00, 0x000A, 0x1FFE);
}

void abi_ObjectFadeGate0Record(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFadeGate0Record(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A8BC ObjectFadeGate0Step
 * WHAT IT DOES: The gate's frame: on lap 0 the first car contact (+$25
 * bit 0 marks it) starts a slow fade ($0DA5 bit 5, step $0A up to $1FFE).
 * HOW: ObjectSyncFromRecord, RecordNextContact; $0DA5 by long accesses.
 * Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectFadeGate0Step(Carried *cr)
{
    fade_frame(cr, 0x00, 0x000A, 0x1FFE);
}

void abi_ObjectFadeGate0Step(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFadeGate0Step(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A907 ObjectFadeGate2
 * WHAT IT DOES: Sets up the lap 2 fade gate: restart $09:A925
 * (ObjectFadeGate2Record), frame $09:A94C (ObjectFadeGate2Step); its record
 * and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectFadeGate2(Carried *cr)
{
    handlers2(cr, 0xA925, 0xA94C);
    gate42d(cr, 0x0780);
    fade_frame(cr, 0x02, 0x0020, 0x1F20);
}

void abi_ObjectFadeGate2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFadeGate2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A925 ObjectFadeGate2Record
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectFadeGate2. Exit: as ObjectFadeGate2.
 */
void ObjectFadeGate2Record(Carried *cr)
{
    gate42d(cr, 0x0780);
    fade_frame(cr, 0x02, 0x0020, 0x1F20);
}

void abi_ObjectFadeGate2Record(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFadeGate2Record(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A94C ObjectFadeGate2Step
 * WHAT IT DOES: The gate's frame: on lap 2 the first car contact (+$25
 * bit 0 marks it) starts a fade ($0DA5 bit 5, step $20 up to $1F20).
 * HOW: ObjectSyncFromRecord, RecordNextContact; $0DA5 by long accesses;
 * its no-contact exit is the RTL of 09:A8BC's code (09:A906). Reached only
 * by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectFadeGate2Step(Carried *cr)
{
    fade_frame(cr, 0x02, 0x0020, 0x1F20);
}

void abi_ObjectFadeGate2Step(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFadeGate2Step(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:A9DC: while a car is in the zone, the slide runs up (step $100 to
   $1000, direction 0, $7F:3470 = $7FFF, $0DA5 bit 7); otherwise it runs
   back down (to -$100, direction $8000, $1A04 = $FF). */
static void slide_zone_frame(Carried *cr)
{
    if (hit(cr)) {
        cyc(42);
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        st16w(cr, W(slide_done), 0x7FFF);
        slide_set(cr, 0x0100, 0x1000, 0x0000);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        or8(cr, (cyc(16), db(cr)) | W(slide_flags), 0x80);
        { cyc(44); return; }
    } else { cyc(40); }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    slide_set(cr, 0x0100, 0xFF00, 0x8000);
    slide_hold(cr);
    cyc(44);
}

/*
 * @routine 09:A997 ObjectSlideZone
 * WHAT IT DOES: Sets up a slide zone: restart $09:A9B5
 * (ObjectSlideZoneRecord), frame $09:A9DC (ObjectSlideZoneStep); its record
 * and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectSlideZone(Carried *cr)
{
    handlers2(cr, 0xA9B5, 0xA9DC);
    gate42d(cr, 0x0F00);
    slide_zone_frame(cr);
}

void abi_ObjectSlideZone(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideZone(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A9B5 ObjectSlideZoneRecord
 * WHAT IT DOES: The zone's restart: its record ($42D, flag bit 4, at
 * ($F00, $F00, $F00)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectSlideZone. Exit: as ObjectSlideZone.
 */
void ObjectSlideZoneRecord(Carried *cr)
{
    gate42d(cr, 0x0F00);
    slide_zone_frame(cr);
}

void abi_ObjectSlideZoneRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideZoneRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:A9DC ObjectSlideZoneStep
 * WHAT IT DOES: The zone's frame: with a car's hit the slide runs up (step
 * $100 to $1000, direction 0, done mark $7F:3470 = $7FFF, $0DA5 bit 7);
 * without, back down (step $100 to -$100, direction $8000, $1A04 = $FF).
 * HOW: ObjectSyncFromRecord, RecordContactSummary; word stores.
 * Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectSlideZoneStep(Carried *cr)
{
    slide_zone_frame(cr);
}

void abi_ObjectSlideZoneStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideZoneStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:AA78: a car's contact on lap 0 counts $1A06 up, sets bit 0 of the
   car's record +$8A (08:B72E), sweeps the screen with sound $36 unless
   bits 4-5 of the clock fraction $0E63 are clear, and releases the
   marker. */
static void bonus_marker_frame(Carried *cr)
{
    uint8_t v;

    if (!synced(cr) || !next_contact_t(cr, 52))
        { cyc(72); return; }
    if (!lap_is_next(cr, 0x00))
        { cyc(34); return; }
    inc8(cr, db(cr) | W(wram_1A06));
    lda8(cr, 0x01);
    GsuRecordFlags8ASet(cr);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    v = ld8(cr, (cyc(16), db(cr)) | W(race_time_fraction));
    set_p(cr, (uint8_t)((cr->p & ~P_C) | ((v & 0x08) ? P_C : (cyc(62), 0))));
    lda8(cr, (uint8_t)((v >> 4) & 0x03));
    if ((uint8_t)cr->a) {
        or8(cr, db(cr) | W(slide_flags), 0x10);
        sound0(cr, 0x36);
    } else { cyc(12); }
    (cyc(62), ObjectRelease_cr(cr));
    cyc(44);
}

/*
 * @routine 09:AA33 ObjectBonusMarker
 * WHAT IT DOES: Sets up a bonus marker: restart $09:AA51
 * (ObjectBonusMarkerRecord), frame $09:AA78 (ObjectBonusMarkerStep); its
 * record and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBonusMarker(Carried *cr)
{
    handlers2(cr, 0xAA51, 0xAA78);
    gate42d(cr, 0x0780);
    bonus_marker_frame(cr);
}

void abi_ObjectBonusMarker(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBonusMarker(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:AA51 ObjectBonusMarkerRecord
 * WHAT IT DOES: The marker's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectBonusMarker. Exit: as ObjectBonusMarker.
 */
void ObjectBonusMarkerRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    bonus_marker_frame(cr);
}

void abi_ObjectBonusMarkerRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBonusMarkerRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:AA78 ObjectBonusMarkerStep
 * WHAT IT DOES: The marker's frame: a car's contact on lap 0 counts the
 * markers at $1A06 up, sets bit 0 of the car's record +$8A, sweeps the
 * screen ($0DA5 bit 4) with sound $36 unless bits 4-5 of the clock
 * fraction $0E63 are clear, and releases the marker.
 * HOW: ObjectSyncFromRecord, RecordNextContact; 08:B72E with A low byte 1.
 * Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectBonusMarkerStep(Carried *cr)
{
    bonus_marker_frame(cr);
}

void abi_ObjectBonusMarkerStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBonusMarkerStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:AB01: a car's contact sets the window slide's bit 2 ($0DA5, bit 3
   cleared); the first time (+$25 bit 0) sound $10 on port 1 and, on lap 0
   with markers taken ($1A06), bit 0 of the car's record +$8A set. No
   contact clears the mark. */
static void bonus_gate_frame(Carried *cr)
{
    uint8_t v;

    if (!synced(cr) || !next_contact_t(cr, 28)) {
        cyc(28);
        and8(cr, F_c(0x2025, cr->x), 0xFE);
        { cyc(44); return; }
    } else { cyc(38); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    v = (uint8_t)(ld8(cr, db(cr) | W(slide_flags)) & 0xF7);
    lda8(cr, (uint8_t)(v | 0x04));
    wr8(db(cr) | W(slide_flags), cyc_then(36, (uint8_t)(v | 0x04)));
    if (bit25(cr, 0x01))
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    sound1(cr, 0x10);
    if (!lap_is_x(cr, 0x00, 38, 0, 38))
        { cyc(34); return; } else { cyc(38); }
    if (!ld8(cr, db(cr) | W(wram_1A06)))
        { cyc(66); return; } else { cyc(16); }
    lda8(cr, 0x01);
    GsuRecordFlags8ASet(cr);
    cyc(22);
}

/*
 * @routine 09:AABC ObjectBonusGate
 * WHAT IT DOES: Sets up the bonus gate: restart $09:AADA
 * (ObjectBonusGateRecord), frame $09:AB01 (ObjectBonusGateStep); its record
 * and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBonusGate(Carried *cr)
{
    handlers2(cr, 0xAADA, 0xAB01);
    gate42d(cr, 0x0780);
    bonus_gate_frame(cr);
}

void abi_ObjectBonusGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBonusGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:AADA ObjectBonusGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectBonusGate. Exit: as ObjectBonusGate.
 */
void ObjectBonusGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    bonus_gate_frame(cr);
}

void abi_ObjectBonusGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBonusGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:AB01 ObjectBonusGateStep
 * WHAT IT DOES: The gate's frame: a car's contact sets bit 2 of $0DA5
 * (clearing bit 3); the first contact (+$25 bit 0 marks it) plays sound
 * $10 on port 1 and, on lap 0 with markers taken ($1A06), sets bit 0 of
 * the car's record +$8A. No contact clears the mark.
 * HOW: ObjectSyncFromRecord, RecordNextContact; 08:B72E with A low byte 1.
 * Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectBonusGateStep(Carried *cr)
{
    bonus_gate_frame(cr);
}

void abi_ObjectBonusGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBonusGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:AB9B: a car's contact sets bits 1 and 3 of $0DA5; the first time
   (+$25 bit 0) sound $0F on port 1 and, unless on lap 2, with markers
   taken ($1A06), bit 0 of the car's record +$8A cleared. No contact clears
   the mark. */
static void bonus_end_frame(Carried *cr)
{
    if (!synced(cr) || !next_contact_t(cr, 28)) {
        cyc(28);
        and8(cr, F_c(0x2025, cr->x), 0xFE);
        { cyc(44); return; }
    } else { cyc(16); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    or8(cr, db(cr) | W(slide_flags), 0x0A);
    if (bit25(cr, 0x01))
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    sound1(cr, 0x0F);
    if (lap_is_x(cr, 0x02, 38, 38, 0))
        { cyc(34); return; } else { cyc(38); }
    if (!ld8(cr, db(cr) | W(wram_1A06)))
        { cyc(66); return; } else { cyc(16); }
    car_bits16(cr, 0x0001, GsuRecordFlags8AClear);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(44);
}

/*
 * @routine 09:AB56 ObjectBonusEndGate
 * WHAT IT DOES: Sets up the bonus end gate: restart $09:AB74
 * (ObjectBonusEndGateRecord), frame $09:AB9B (ObjectBonusEndGateStep); its
 * record and frame.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBonusEndGate(Carried *cr)
{
    handlers2(cr, 0xAB74, 0xAB9B);
    gate42d(cr, 0x0780);
    bonus_end_frame(cr);
}

void abi_ObjectBonusEndGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBonusEndGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:AB74 ObjectBonusEndGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: as ObjectBonusEndGate. Exit: as ObjectBonusEndGate.
 */
void ObjectBonusEndGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    bonus_end_frame(cr);
}

void abi_ObjectBonusEndGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBonusEndGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:AB9B ObjectBonusEndGateStep
 * WHAT IT DOES: The gate's frame: a car's contact sets bits 1 and 3 of
 * $0DA5; the first contact (+$25 bit 0 marks it) plays sound $0F on port 1
 * and, unless on lap 2, with markers taken ($1A06), clears bit 0 of the
 * car's record +$8A. No contact clears the mark.
 * HOW: ObjectSyncFromRecord, RecordNextContact; 08:B743 with A = 1.
 * Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectBonusEndGateStep(Carried *cr)
{
    bonus_end_frame(cr);
}

void abi_ObjectBonusEndGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBonusEndGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* REP #$20, LDA $0D2B (the HUD mode), and with a mode (two players) SEP
   #$20 and ObjectRelease: 1 when released. */
static int released_by_hud(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(46, db(cr) | W(hud_mode)));
    if (!cr->a)
        { cyc(44); return 0; } else { cyc(100); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    ObjectRelease_cr(cr);
    return 1;
}

/*
 * @routine 09:ABF3 ObjectPopMarker
 * WHAT IT DOES: In a one-player race (HUD mode $0D2B = 0) puts a pop marker
 * (kind $B7FC) at object X's position $1E0 higher with its scale +$15,
 * handler $09:AD50 (ObjectPopMarkerStep), and clears X's handler; with a
 * HUD mode X is released.
 * HOW: ObjectSpawn, ObjectCopyPositionTo; starts with a NOP, as the course
 * data's pointers expect.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; released:
 * as ObjectRelease leaves it; else A low byte = 0, Z set.
 */
void ObjectPopMarker(Carried *cr)
{
    if ((cyc(14), released_by_hud(cr)))
        { cyc(44); return; }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (spawn(cr, 0xB7FC)) {
        cyc(62);
        ObjectCopyPositionTo_cr(cr);
        ld8(cr, cyc_then(22, F_c(0x2015, cr->x)));
        wr8(F_c(0x2015, cr->y), cyc_then(30, (uint8_t)cr->a));
        add16(cr, F_c(0x200E, cr->y), cyc_then(22, 0xFE20));
        cyc(44);
        handler(cr, cr->y, 0xAD50);
    } else { cyc(26); }
    handler_none(cr, cr->x);
}

void abi_ObjectPopMarker(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPopMarker(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:AD07: a car's hit on a lap other than the one in +$48 takes that lap
   in +$48 (through $1F58), plays sound $41 and adds $19E6 to the race
   time $0E70 in BCD. */
static void time_gate_frame(Carried *cr)
{
    uint8_t v, r;

    if (!hit(cr))
        { cyc(62); return; } else { cyc(34); }
    v = ld8(cr, F_c(0x2048, cr->x));
    wr8(db(cr) | W(wram_1F58), cyc_then(24, v));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    v = ld8(cr, (cyc(38), db(cr)) | W(wram_0DEA));
    r = rd8(cyc_then(24, db(cr) | W(wram_1F58)));
    cyc(22);
    cmp8(cr, r);
    if (v == r)
        { cyc(44); return; } else { cyc(32); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    v = ld8(cr, db(cr) | W(wram_0DEA));
    wr8(db(cr) | W(wram_1F58), cyc_then(24, v));
    v = ld8(cr, (cyc(16), db(cr)) | W(wram_1F58));
    wr8(F_c(0x2048, cr->x), cyc_then(30, v));
    sound0(cr, 0x41);
    set_p(cr, (uint8_t)(cr->p | (P_D)));
    v = ld8(cr, (cyc(30), db(cr)) | W(race_time_bcd));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    r = rd8(cyc_then(38, db(cr) | W(wram_19E6)));
    v = alu_adc8(v, r, &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    wr8(db(cr) | W(race_time_bcd), cyc_then(24, v));
    v = ld8(cr, (cyc(16), db(cr)) | W(wram_0E71));
    v = alu_adc8(v, 0x00, &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    wr8(db(cr) | W(wram_0E71), cyc_then(40, v));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_D)));
    cyc(58);
}

/* A time gate's set-up in a one-player race: restart r, frame $09:AD07
   (ObjectTimeGateStep), +$48 = 4 (no lap taken yet). */
static void time_gate(Carried *cr, uint16_t r, unsigned released, unsigned tail)
{
    if (released_by_hud(cr))
        { cyc(released); return; }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    handlers2(cr, r, 0xAD07);
    st8i(cr, F_c(0x2048, cr->x), cyc_then(6, 0x04));
    cyc(tail);
}

/*
 * @routine 09:AC4F ObjectTimeGate
 * WHAT IT DOES: Sets up a time gate in a one-player race: restart $09:AC83
 * (ObjectTimeGateRecord), frame $09:AD07 (ObjectTimeGateStep), +$48 = 4;
 * with a HUD mode ($0D2B) it is released.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A low
 * byte = 4, or as ObjectRelease leaves it.
 */
void ObjectTimeGate(Carried *cr)
{
    time_gate(cr, 0xAC83, 0, 0);
    cyc(44);
}

void abi_ObjectTimeGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTimeGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:AC83 ObjectTimeGateRecord
 * WHAT IT DOES: The time gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)).
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * GsuRecordPosition leaves it, M 8-bit.
 */
void ObjectTimeGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    cyc(44);
}

void abi_ObjectTimeGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTimeGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:ACAB ObjectTimeGateSmall
 * WHAT IT DOES: Sets up a small time gate in a one-player race: restart
 * $09:ACDF (ObjectTimeGateSmallRecord), frame $09:AD07
 * (ObjectTimeGateStep), +$48 = 4; with a HUD mode ($0D2B) it is released.
 * HOW: Word and byte stores. Reached only by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A low
 * byte = 4, or as ObjectRelease leaves it.
 */
void ObjectTimeGateSmall(Carried *cr)
{
    time_gate(cr, 0xACDF, 44, 44);
}

void abi_ObjectTimeGateSmall(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTimeGateSmall(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:ACDF ObjectTimeGateSmallRecord
 * WHAT IT DOES: The small time gate's restart: its record ($42D, flag bit
 * 4, at ($3C0, $3C0, $3C0)).
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_gates_09.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * GsuRecordPosition leaves it, M 8-bit.
 */
void ObjectTimeGateSmallRecord(Carried *cr)
{
    gate42d(cr, 0x03C0);
    cyc(44);
}

void abi_ObjectTimeGateSmallRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTimeGateSmallRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:AD07 ObjectTimeGateStep
 * WHAT IT DOES: The time gates' frame: a car's hit on a lap ($0DEA) other
 * than the one it last counted (+$48) counts this one, plays sound $41 and
 * adds the bonus $19E6 to the race time $0E70-$0E71 (BCD).
 * HOW: ObjectSyncFromRecord, RecordContactSummary; the laps compared
 * through $1F58; SED, ADC, CLD.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it; D flag clear.
 */
void ObjectTimeGateStep(Carried *cr)
{
    time_gate_frame(cr);
}

void abi_ObjectTimeGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTimeGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:AD50 ObjectPopMarkerStep
 * WHAT IT DOES: The pop marker's frame: its size +$14 shrinks by 8.
 * HOW: ADC #$F8; ObjectRebucket.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket leaves it.
 */
void ObjectPopMarkerStep(Carried *cr)
{
    add8(cr, F_c(0x2014, cr->x), 0xF8);
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

void abi_ObjectPopMarkerStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPopMarkerStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* +$0A against the lap $0DEA (through $1F58): 1 when equal. */
static int lap_matches_0a(Carried *cr)
{
    uint8_t v, r;

    v = ld8(cr, cyc_then(0, F_c(0x200A, cr->x)));
    wr8(db(cr) | W(wram_1F58), cyc_then(24, v));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    v = ld8(cr, (cyc(38), db(cr)) | W(wram_0DEA));
    r = rd8(cyc_then(24, db(cr) | W(wram_1F58)));
    cyc(22);
    cmp8(cr, r);
    return v == r;
}

/* +$0A = the lap $0DEA (through $1F58), +$04 = $A040 (the lit model). */
static void lap_take_0a(Carried *cr)
{
    uint8_t v;

    v = ld8(cr, db(cr) | W(wram_0DEA));
    wr8(db(cr) | W(wram_1F58), cyc_then(24, v));
    v = ld8(cr, (cyc(16), db(cr)) | W(wram_1F58));
    wr8(F_c(0x200A, cr->x), cyc_then(30, v));
    st16i(cr, F_c(0x2004, cr->x), cyc_then(22, 0xA040));
    cyc(80);
}

/* PHX, TYX: a spark of `kind` at the player $0346's position following
   it (+$29 = the player, +$27 = 0, +$0A = $0A, handler $09:AE60
   ObjectSparkFollow); PLX. */
static void spark_follow(Carried *cr, uint16_t kind)
{
    uint16_t sx = cr->x;

    cr->x = cr->y;
    set_p(cr, nz16(cr->p, cr->x));
    cyc(44);
    if (spawn(cr, kind)) {
        uint8_t a;

        cr->x = rd16(cyc_then(24, db(cr) | W(wram_0346)));
        set_p(cr, nz16(cr->p, cr->x));
        st8i(cr, F_c(0x200A, cr->y), cyc_then(6, 0x0A));
        (cyc(62), ObjectCopyPositionTo_cr(cr));
        st16i(cr, F_c(0x2027, cr->y), cyc_then(22, 0x0000));
        a = (uint8_t)cr->a;
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cr->a = cr->x;
        set_p(cr, nz16(cr->p, cr->a));
        wr16(F_c(0x2029, cr->y), cyc_then(110, cr->x));
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(52);
        lda8(cr, a);
        handler(cr, cr->y, 0xAE60);
    } else { cyc(26); }
    cyc(36);                            /* PLX */
    cr->x = sx;
    set_p(cr, nz16(cr->p, sx));
}

/* A lap ball's record ($42D, flag bit 4, +$2A high $14); its model +$04
   lit ($A040) when +$0A is this lap, else `kind`. */
static void lap_ball_record(Carried *cr, uint16_t kind)
{
    cyc(62);
    GsuRecordNew42D_cr(cr);
    GsuRecordFlag10_cr(cr);
    cyc(58);
    lda8(cr, 0x14);
    GsuRecordSet2AHigh_cr(cr);
    if ((cyc(124), lap_matches_0a(cr)))
        { cyc(16); st16i(cr, F_c(0x2004, cr->x), 0xA040); cyc(46); }          /* SEP, JMP */
    else
        { cyc(22); st16i(cr, F_c(0x2004, cr->x), kind); cyc(22); }            /* SEP */
}

/* 09:ADB6: the up ball's frame: it grows (+$15, +$14 by $10); on a car's
   contact with flags 1 or 2 (AND #$0B) on a lap it has not lit yet (+$0A)
   it takes the lap, lights up, raises that car's record +$6A by $40,
   plays sound $25 and leaves a following spark ($B7A8). */
static void ball_up_frame(Carried *cr)
{
    add8(cr, F_c(0x2015, cr->x), 0x10);
    add8(cr, F_c(0x2014, cr->x), 0x10);
    if (!synced(cr))
        { cyc(66); return; } else { cyc(10); }
    for (;;) {
        cyc(80);
        RecordNextContact_cr(cr);
        if (!(cr->p & P_C))
            { cyc(72); return; } else { cyc(72); }
        lda8(cr, (uint8_t)(cr->a & 0x0B));
        cmp8(cr, 0x01);
        if ((uint8_t)cr->a == 0x01)
            break; else { cyc(10); }
        cmp8(cr, 0x02);
        if ((uint8_t)cr->a == 0x02)
            { cyc(22); break; }
    }
    if (lap_matches_0a(cr))
        { cyc(44); return; } else { cyc(10); }
    lap_take_0a(cr);
    lda8(cr, 0x40);
    GsuRecordRaise6A_cr(cr);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(22);
    sound0(cr, 0x25);
    spark_follow(cr, 0xB7A8);
    cyc(44);
}

/*
 * @routine 09:AD5E ObjectLapBallUp
 * WHAT IT DOES: Sets up an up ball: restart $09:AD81 (ObjectLapBallUpRecord),
 * frame $09:ADB6 (ObjectLapBallUpStep), +$0A = 4 (no lap lit); its record
 * and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectLapBallUp(Carried *cr)
{
    handlers2(cr, 0xAD81, 0xADB6);
    st8i(cr, F_c(0x200A, cr->x), cyc_then(6, 0x04));
    lap_ball_record(cr, 0xB7A8);
    ball_up_frame(cr);
}

void abi_ObjectLapBallUp(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapBallUp(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:AD81 ObjectLapBallUpRecord
 * WHAT IT DOES: The up ball's restart: its record ($42D, flag bit 4, +$2A
 * high $14), its model lit ($A040) when +$0A is the current lap, else
 * $B7A8; then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordSet2AHigh; the lap
 * compared through $1F58.
 * Entry: as ObjectLapBallUp. Exit: as ObjectLapBallUp.
 */
void ObjectLapBallUpRecord(Carried *cr)
{
    lap_ball_record(cr, 0xB7A8);
    ball_up_frame(cr);
}

void abi_ObjectLapBallUpRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapBallUpRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:ADB6 ObjectLapBallUpStep
 * WHAT IT DOES: The up ball's frame: it grows (+$15 and +$14 by $10); on a
 * car's contact with flags 1 or 2 (AND #$0B) on a lap it has not lit
 * (+$0A), it takes the lap, lights up ($A040), raises that car's record
 * +$6A by $40, plays sound $25 and leaves a spark (kind $B7A8) following
 * the player.
 * HOW: ObjectSyncFromRecord, RecordNextContact, GsuRecordRaise6A,
 * ObjectSpawn.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectLapBallUpStep(Carried *cr)
{
    ball_up_frame(cr);
}

void abi_ObjectLapBallUpStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapBallUpStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/*
 * @routine 09:AE60 ObjectSparkFollow
 * WHAT IT DOES: A following spark's frame: at its object's (+$29) position,
 * released when +$0A runs out; it rises by $C0 and a speed +$27 that grows
 * by $28 each frame, and grows (+$15, +$14 by $10).
 * HOW: ObjectCopyPositionFrom; the speed added through $1F4C;
 * ObjectRebucket.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
 * leaves it.
 */
void ObjectSparkFollow(Carried *cr)
{
    cr->y = rd16(cyc_then(30, F_c(0x2029, cr->x)));
    set_p(cr, nz16(cr->p, cr->y));
    (cyc(62), ObjectCopyPositionFrom_cr(cr));
    (cyc(22), life_down(cr, 22));
    add16(cr, F_c(0x200E, cr->x), 0xFF40);
    add16(cr, F_c(0x2027, cr->x), cyc_then(44, 0xFFD8));
    add16_via(cr, F_c(0x200E, cr->x), F_c(0x2027, cr->x));
    add8(cr, F_c(0x2015, cr->x), cyc_then(22, 0x10));
    add8(cr, F_c(0x2014, cr->x), 0x10);
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

void abi_ObjectSparkFollow(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSparkFollow(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:AF17: the down ball's frame: as the up ball's (contact flags bit 0),
   lowering the car's record +$82 by $40 (GsuRecordLower82) with sound $1B
   and a following spark $B7C4. */
static void ball_down_frame(Carried *cr)
{
    add8(cr, F_c(0x2015, cr->x), 0x10);
    add8(cr, F_c(0x2014, cr->x), 0x10);
    if (!synced(cr))
        { cyc(34); return; }
    if ((cyc(6), !next_contact(cr)))
        { cyc(40); return; }
    if (lap_matches_0a(cr))
        { cyc(12); return; } else { cyc(10); }
    lap_take_0a(cr);
    lda8(cr, 0x40);
    GsuRecordLower82_cr(cr);
    cyc(72);
    sound0(cr, 0x1B);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(22);
    spark_follow(cr, 0xB7C4);
    cyc(12);
}

/*
 * @routine 09:AEBF ObjectLapBallDown
 * WHAT IT DOES: Sets up a down ball: restart $09:AEE2
 * (ObjectLapBallDownRecord), frame $09:AF17 (ObjectLapBallDownStep), +$0A
 * = 4 (no lap lit); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectLapBallDown(Carried *cr)
{
    handlers2(cr, 0xAEE2, 0xAF17);
    st8i(cr, F_c(0x200A, cr->x), cyc_then(6, 0x04));
    lap_ball_record(cr, 0xB7C4);
    ball_down_frame(cr);
    cyc(32);
}

void abi_ObjectLapBallDown(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapBallDown(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:AEE2 ObjectLapBallDownRecord
 * WHAT IT DOES: The down ball's restart: its record ($42D, flag bit 4,
 * +$2A high $14), its model lit ($A040) when +$0A is the current lap, else
 * $B7C4; then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordSet2AHigh; the lap
 * compared through $1F58.
 * Entry: as ObjectLapBallDown. Exit: as ObjectLapBallDown.
 */
void ObjectLapBallDownRecord(Carried *cr)
{
    lap_ball_record(cr, 0xB7C4);
    ball_down_frame(cr);
    cyc(32);
}

void abi_ObjectLapBallDownRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapBallDownRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:AF17 ObjectLapBallDownStep
 * WHAT IT DOES: The down ball's frame: it grows (+$15 and +$14 by $10); on
 * a car's contact (flags bit 0) on a lap it has not lit (+$0A), it takes
 * the lap, lights up ($A040), lowers that car's record +$82 by $40, plays
 * sound $1B and leaves a spark (kind $B7C4) following the player.
 * HOW: ObjectSyncFromRecord, RecordNextContact, GsuRecordLower82,
 * ObjectSpawn.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectLapBallDownStep(Carried *cr)
{
    ball_down_frame(cr);
    cyc(32);
}

void abi_ObjectLapBallDownStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapBallDownStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:AFFC: a car's hit, the first time (+$25 bit 0), moves the count
   $19EE to $19F0 (HudCountMove19EE's copy) and sets $19ED = 8. */
static void count_gate_frame(Carried *cr)
{
    if (!hit(cr))
        { cyc(62); return; } else { cyc(12); }
    if (bit25(cr, 0x01))
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    (cyc(46), HudCountMove19EECopy_cr(cr));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    st8i(cr, (cyc(22), db(cr)) | W(wram_19ED), 0x08);
    cyc(44);
}

/*
 * @routine 09:AFB7 ObjectCountGate
 * WHAT IT DOES: Sets up the count gate: restart $09:AFD5
 * (ObjectCountGateRecord), frame $09:AFFC (ObjectCountGateStep); its record
 * and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectCountGate(Carried *cr)
{
    handlers2(cr, 0xAFD5, 0xAFFC);
    gate42d(cr, 0x0780);
    count_gate_frame(cr);
}

void abi_ObjectCountGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCountGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:AFD5 ObjectCountGateRecord
 * WHAT IT DOES: The count gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectCountGate. Exit: as ObjectCountGate.
 */
void ObjectCountGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    count_gate_frame(cr);
}

void abi_ObjectCountGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCountGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:AFFC ObjectCountGateStep
 * WHAT IT DOES: The count gate's frame: a car's hit, the first time (+$25
 * bit 0), moves the count at $19EE to $19F0 (clearing $19EE, $19F2 = 1)
 * and sets $19ED to 8.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; JSR into
 * HudCountMove19EE at its copy (09:C0AB).
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectCountGateStep(Carried *cr)
{
    count_gate_frame(cr);
}

void abi_ObjectCountGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCountGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:B065: the counting marker's record ($424, +$0A $90E8) in mode 2 with
   +$2A high $14 at ($500, $500, $500); then its frame. */
static void count_marker_record(Carried *cr)
{
    cyc(62);
    GsuRecordNew424Alt_cr(cr);
    (cyc(62), GsuRecordMode2_cr(cr));
    cyc(58);
    lda8(cr, 0x14);
    GsuRecordSet2AHigh_cr(cr);
    cyc(102);
    record_at(cr, 0x0500, 0x0500, 0x0500);
}

/* 09:B092: while $19E8 is set, ObjectStep48To8; then synced. */
static void count_marker_frame(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (ld8(cr, (cyc(38), db(cr)) | W(wram_19E8)))
        { cyc(62); ObjectStep48To8_cr(cr); } else { cyc(22); }
    cyc(62);
    ObjectSyncFromRecord_cr(cr);
    cyc(44);
}

/*
 * @routine 09:B02A ObjectCountMarker
 * WHAT IT DOES: Sets up the counting marker: restart $09:B065
 * (ObjectCountMarkerRecord), frame $09:B092 (ObjectCountMarkerStep), +$20
 * = $ECD0, shown as 0 (+$5A = $80, twice), +$48 = 4; its record and frame.
 * HOW: Word and byte stores; LDA #0, ORA #$80 for +$5A.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectCountMarker(Carried *cr)
{
    handlers2(cr, 0xB065, 0xB092);
    st16i(cr, F_c(0x2020, cr->x), cyc_then(22, 0xECD0));
    lda8(cr, 0x00);
    lda8(cr, 0x80);
    wr8(F_c(0x205A, cr->x), cyc_then(44, 0x80));
    st8i(cr, F_c(0x2048, cr->x), cyc_then(6, 0x04));
    lda8(cr, 0x00);
    lda8(cr, 0x80);
    wr8(F_c(0x205A, cr->x), cyc_then(22, 0x80));
    count_marker_record(cr);
    count_marker_frame(cr);
}

void abi_ObjectCountMarker(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCountMarker(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B065 ObjectCountMarkerRecord
 * WHAT IT DOES: The marker's restart: its record (GsuRecordNew424Alt) in
 * mode 2 with +$2A high $14 at ($500, $500, $500), then its frame.
 * HOW: GsuRecordNew424Alt, GsuRecordMode2, GsuRecordSet2AHigh,
 * GsuRecordPosition.
 * Entry: as ObjectCountMarker. Exit: as ObjectCountMarker.
 */
void ObjectCountMarkerRecord(Carried *cr)
{
    count_marker_record(cr);
    count_marker_frame(cr);
}

void abi_ObjectCountMarkerRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCountMarkerRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B092 ObjectCountMarkerStep
 * WHAT IT DOES: The marker's frame: while $19E8 is set its count +$48
 * steps (ObjectStep48To8); then it is synced from its record.
 * HOW: JSR ObjectStep48To8; ObjectSyncFromRecord.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectSyncFromRecord leaves it.
 */
void ObjectCountMarkerStep(Carried *cr)
{
    count_marker_frame(cr);
}

void abi_ObjectCountMarkerStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCountMarkerStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_HudCountMove19EE(void);

/* 09:B0DC: the trigger's record (GsuRecordNew42D, GsuRecordFlag10) at
   ($3C0, $3C0, $3C0). */
static void count_trigger_record(Carried *cr)
{
    (cyc(62), GsuRecordNew42D_cr(cr));
    GsuRecordFlag10_cr(cr);
    record_at(cr, 0x03C0, 0x03C0, 0x03C0);
}

/* 09:B103: synced from its record; at a car's contact (the record's
   contacts, RecordNextContact, with bit 0), +$25 bit 0 set and the count
   moved (HudCountMove19EE) the first time, then $19E8 = 1. */
static void count_trigger_frame(Carried *cr)
{
    (cyc(62), ObjectSyncFromRecord_cr(cr));
    cyc(18);
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if (!(uint8_t)cr->a)
        { cyc(44); return; } else { cyc(56); }
    for (;;) {
        RecordNextContact_cr(cr);
        if (!(cr->p & P_C))
            return; else { cyc(50); }
        lda8(cr, (uint8_t)(cr->a & 0x01));
        if ((uint8_t)cr->a)
            break;
    }
    lda8(cr, (uint8_t)(ld8(cr, F_c(0x2025, cr->x)) & 0x01));
    if (!(uint8_t)cr->a) {
        cyc(18);
        or8(cr, F_c(0x2025, cr->x), 0x01);
        (cyc(46), HudCountMove19EE_cr(cr));
    } else { cyc(44); }
    cyc(22);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    st8i(cr, db(cr) | W(wram_19E8), 0x01);
    cyc(44);
}

/*
 * @routine 09:B0BE ObjectCountTrigger
 * WHAT IT DOES: Sets up the object that starts the counting markers:
 * restart $09:B0DC (ObjectCountTriggerRecord), frame $09:B103
 * (ObjectCountTriggerStep); its record and frame.
 * HOW: Word and byte stores, then falls into ObjectCountTriggerRecord.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectCountTrigger(Carried *cr)
{
    handlers2(cr, 0xB0DC, 0xB103);
    count_trigger_record(cr);
    count_trigger_frame(cr);
}

void abi_ObjectCountTrigger(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCountTrigger(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B0DC ObjectCountTriggerRecord
 * WHAT IT DOES: The trigger's restart: its record from the first pool
 * (GsuRecordNew42D), marked (GsuRecordFlag10), at ($3C0, $3C0, $3C0),
 * then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition; falls into
 * ObjectCountTriggerStep. A failed record is not checked.
 * Entry: as ObjectCountTrigger. Exit: as ObjectCountTrigger.
 */
void ObjectCountTriggerRecord(Carried *cr)
{
    count_trigger_record(cr);
    count_trigger_frame(cr);
}

void abi_ObjectCountTriggerRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCountTriggerRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B103 ObjectCountTriggerStep
 * WHAT IT DOES: The trigger's frame: synced from its record; the first
 * time a car touches it (a contact with bit 0 among the record's) it sets
 * its +$25 bit 0 and moves the count (HudCountMove19EE); at every such
 * contact $19E8 = 1, which starts ObjectCountMarkerStep's count.
 * HOW: ObjectSyncFromRecord and RecordNextContact, each tested AND #1;
 * the already-set case jumps (JML) past the count to the $19E8 store.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: untouched: as
 * ObjectSyncFromRecord or RecordNextContact leaves it; touched: A low byte
 * = 1, M 8-bit, N, Z from it.
 */
void ObjectCountTriggerStep(Carried *cr)
{
    count_trigger_frame(cr);
}

void abi_ObjectCountTriggerStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCountTriggerStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ---- 09:C438-D13E: bobbing, breakable, swinging, orbiting objects ---- */

void abi_RotateXY(void);
void abi_RotateXZFrom08(void);
void abi_RotateXZ(void);
void abi_GsuRecordNew424Piece(void);
void abi_ObjectBreakableBurst(void);

/* Direct page byte or word at off. */
static uint32_t dp(Carried *cr, uint8_t off)
{
    cyc(4);
    return (uint16_t)(cr->d + off);
}

/* LDA addr, EOR #m, STA addr (8-bit). */
static void eor8(Carried *cr, uint32_t addr, uint8_t m)
{
    uint8_t v = (uint8_t)(ld8(cr, addr) ^ m);

    lda8(cr, v);
    cyc(26);
    wr8(addr, v);
}

/* NOP, LDA #v, ORA #$80, STA +f: a value shown with bit 7. */
static void st8i_hi(Carried *cr, uint32_t addr, uint8_t v)
{
    lda8(cr, v);
    lda8(cr, (uint8_t)(v | 0x80));
    cyc(6);
    wr8(addr, (uint8_t)(v | 0x80));
}

/* LDA +f, STA $1F58, SEP #$20, LDA $0DEA, CMP $1F58: 1 when the lap
   $0DEA is the one in field f. */
static int lap_eq(Carried *cr, uint16_t f)
{
    uint8_t v, r;

    v = ld8(cr, cyc_then(22, F_c(f, cr->x)));
    wr8(db(cr) | W(wram_1F58), cyc_then(24, v));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    v = ld8(cr, (cyc(38), db(cr)) | W(wram_0DEA));
    r = rd8(cyc_then(24, db(cr) | W(wram_1F58)));
    cmp8(cr, r);
    return v == r;
}

/* SEP #$20, field f = the lap $0DEA through $1F58. */
static void lap_store(Carried *cr, uint16_t f)
{
    uint8_t v;

    set_p(cr, (uint8_t)(cr->p | (P_M)));
    v = ld8(cr, (cyc(28), db(cr)) | W(wram_0DEA));
    wr8(db(cr) | W(wram_1F58), cyc_then(24, v));
    v = ld8(cr, (cyc(16), db(cr)) | W(wram_1F58));
    wr8(F_c(f, cr->x), cyc_then(30, v));
}

/*
 * @routine 09:C438 ObjectCountUpTo2
 * WHAT IT DOES: Counts object X's +$48 up, to at most 2, and shows it at
 * +$5A with bit 7 ($1F58 = the count).
 * HOW: CMP #2 and INC; the count through $1F58.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A low byte = the count | $80;
 * N, Z from it.
 */
void ObjectCountUpTo2(Carried *cr)
{
    uint8_t v;

    v = ld8(cr, cyc_then(22, F_c(0x2048, cr->x)));
    cyc(22);
    cmp8(cr, 0x02);
    if (v != 0x02)
        inc8(cr, F_c(0x2048, cr->x)); else { cyc(42); }
    cyc(22);
    v = ld8(cr, F_c(0x2048, cr->x));
    wr8(db(cr) | W(wram_1F58), cyc_then(24, v));
    v = ld8(cr, (cyc(16), db(cr)) | W(wram_1F58));
    st8i_hi(cr, F_c(0x205A, cr->x), v);
    cyc(42);
}

void abi_ObjectCountUpTo2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectCountUpTo2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:C478: the bob: the speed +$29 added to the offset +$27, turning back
   at $10 and -$10; the height +$0E moved by the offset (through $1F4C) and
   the scale +$15 grown by 4. */
static void bob_frame(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(52, F_c(0x2027, cr->x)));
    r_add16(cr, rd16(cyc_then(44, F_c(0x2029, cr->x))));
    wr16(F_c(0x2027, cr->x), cyc_then(30, cr->a));
    r_cmp16(cr, 0x0010);
    if (cr->a == 0x0010 || (cyc(34), (r_cmp16(cr, 0xFFF0), cr->a == 0xFFF0))) {
        r_lda16(cr, cyc_then(76, F_c(0x2029, cr->x)));
        cr->a ^= 0xFFFF;
        set_p(cr, nz16(cr->p, cr->a));
        cr->a++;
        set_p(cr, nz16(cr->p, cr->a));
        wr16(F_c(0x2029, cr->x), cyc_then(68, cr->a));
    } else { cyc(52); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    add16_via(cr, F_c(0x200E, cr->x), F_c(0x2027, cr->x));
    add8(cr, F_c(0x2015, cr->x), cyc_then(22, 0x04));
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

/*
 * @routine 09:C455 ObjectBob
 * WHAT IT DOES: Sets up a bobbing object: offset +$27 = 1, speed +$29 = 1,
 * frame $09:C478 (ObjectBobStep); then its frame.
 * HOW: Word and byte stores. Reached only by unit_objects_09c.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBob(Carried *cr)
{
    st16i(cr, F_c(0x2027, cr->x), cyc_then(22, 0x0001));
    st16i(cr, F_c(0x2029, cr->x), cyc_then(44, 0x0001));
    cyc(44);
    handler(cr, cr->x, 0xC478);
    bob_frame(cr);
}

void abi_ObjectBob(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBob(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C478 ObjectBobStep
 * WHAT IT DOES: The bob's frame: the speed +$29 is added to the offset
 * +$27, the speed turning round when the offset reaches $10 or -$10; the
 * height +$0E moves by the offset and the scale +$15 grows by 4.
 * HOW: 16-bit ADC, CMP, EOR #$FFFF + INC; the offset added through $1F4C;
 * ObjectRebucket. Reached only by unit_objects_09c.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
 * leaves it.
 */
void ObjectBobStep(Carried *cr)
{
    bob_frame(cr);
}

void abi_ObjectBobStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBobStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:C4E1: a burst piece's frame: synced; released when +$0A runs out; it
   grows (+$14, +$15, +$16 by $18). */
static void burst_piece_frame(Carried *cr)
{
    (cyc(62), ObjectSyncFromRecord_cr(cr));
    if (!dec8(cr, cyc_then(22, F_c(0x200A, cr->x))))
        { cyc(84); ObjectRelease_cr(cr); } else { cyc(48); }
    add8(cr, F_c(0x2014, cr->x), 0x18);
    add8(cr, F_c(0x2015, cr->x), 0x18);
    add8(cr, F_c(0x2016, cr->x), 0x18);
    cyc(44);
}

/*
 * @routine 09:C4C0 ObjectBreakablePiece
 * WHAT IT DOES: Sets up a piece of a burst (09:D79C's): frame $09:C4E1
 * (ObjectBreakablePieceStep), a flying piece's record (GsuRecordNew424Piece)
 * with its velocity and +$2A high 0, then its frame; with no record free it
 * is released.
 * HOW: GsuRecordNew424Piece (N set: none), GsuRecordVelocity,
 * GsuRecordSet2AHigh.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame or
 * ObjectRelease leaves it.
 */
void ObjectBreakablePiece(Carried *cr)
{
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(44);
    handler(cr, cr->x, 0xC4E1);
    (cyc(62), GsuRecordNew424Piece_cr(cr));
    if (cr->p & P_N) {
        cyc(84);
        ObjectRelease_cr(cr);
        { cyc(44); return; }
    } else { cyc(16); }
    (cyc(62), GsuRecordVelocity_cr(cr));
    lda8(cr, 0x00);
    (cyc(58), GsuRecordSet2AHigh_cr(cr));
    (cyc(102), burst_piece_frame(cr));
}

void abi_ObjectBreakablePiece(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakablePiece(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C4E1 ObjectBreakablePieceStep
 * WHAT IT DOES: A burst piece's frame: synced from its record, released
 * when its life +$0A runs out, and it grows (+$14, +$15, +$16 by $18).
 * HOW: ObjectSyncFromRecord, DEC, ADCs.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: A low byte = the
 * new +$16; N, Z, C, V from its add.
 */
void ObjectBreakablePieceStep(Carried *cr)
{
    burst_piece_frame(cr);
}

void abi_ObjectBreakablePieceStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakablePieceStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:C560: the breakable's frame: its life +$0A runs out (released); it
   grows by $0C (the words at +$14, +$15, +$16); a car's hit releases it
   with sound $5A and bursts it into eight pieces (09:D79C). */
static void breakable_frame(Carried *cr)
{
    if (!dec8(cr, cyc_then(22, F_c(0x200A, cr->x))))
        { cyc(84); ObjectRelease_cr(cr); cyc(22); } else { cyc(70); }
    add16(cr, F_c(0x2014, cr->x), 0x000C);
    add16(cr, F_c(0x2015, cr->x), cyc_then(44, 0x000C));
    add16(cr, F_c(0x2016, cr->x), cyc_then(44, 0x000C));
    if (!(cyc(22), hit(cr)))
        { cyc(62); return; } else { cyc(36); }
    (cyc(38), ObjectRelease_cr(cr));
    sound0(cr, 0x5A);
    (cyc(62), ObjectBreakableBurst(cr));
    cyc(44);
}

/* 09:C531: the breakable's record (GsuRecordNew424Piece; none: released)
   with flag bit 4, +$2A high $46, at ($C0, $C0, $C0); then its frame. */
static void breakable_record(Carried *cr)
{
    (cyc(62), GsuRecordNew424Piece_cr(cr));
    if (cr->p & P_N) {
        cyc(84);
        ObjectRelease_cr(cr);
        { cyc(44); return; }
    } else { cyc(16); }
    (cyc(62), GsuRecordFlag10_cr(cr));
    lda8(cr, 0x46);
    (cyc(58), GsuRecordSet2AHigh_cr(cr));
    cyc(102);
    record_at(cr, 0x00C0, 0x00C0, 0x00C0);
    breakable_frame(cr);
}

/*
 * @routine 09:C513 ObjectBreakable
 * WHAT IT DOES: Sets up a breakable object: frame $09:C560
 * (ObjectBreakableStep), restart $09:C531 (ObjectBreakableRecord); its
 * record and frame.
 * HOW: Word and byte stores (the frame's first). Reached only by
 * unit_objects_09c.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBreakable(Carried *cr)
{
    cyc(22);
    handler(cr, cr->x, 0xC560);
    st16i(cr, F_c(0x201D, cr->x), cyc_then(22, 0xC531));
    st8i(cr, F_c(0x201F, cr->x), cyc_then(28, 0x09));
    breakable_record(cr);
}

void abi_ObjectBreakable(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakable(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C531 ObjectBreakableRecord
 * WHAT IT DOES: The breakable's restart: a flying piece's record
 * (GsuRecordNew424Piece) with flag bit 4, +$2A high $46, at ($C0, $C0,
 * $C0), then its frame; with no record free it is released.
 * HOW: GsuRecordNew424Piece (N set: none; the release is 09:C50E's),
 * GsuRecordFlag10, GsuRecordSet2AHigh, GsuRecordPosition.
 * Reached only by unit_objects_09c.
 * Entry: as ObjectBreakable. Exit: as ObjectBreakable.
 */
void ObjectBreakableRecord(Carried *cr)
{
    breakable_record(cr);
}

void abi_ObjectBreakableRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakableRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C560 ObjectBreakableStep
 * WHAT IT DOES: The breakable's frame: released when its life +$0A runs
 * out; it grows by $0C (the words at +$14, +$15 and +$16 in turn); a car's
 * hit releases it with sound $5A and bursts it into eight pieces
 * (ObjectBreakableBurst).
 * HOW: DEC, 16-bit ADCs, ObjectSyncFromRecord, RecordContactSummary.
 * Reached only by unit_objects_09c.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectBreakableStep(Carried *cr)
{
    breakable_frame(cr);
}

void abi_ObjectBreakableStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakableStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C5B6 ObjectColour0
 * WHAT IT DOES: Sets object X's colour +$18 to 0 with bit 7.
 * HOW: LDA #0, ORA #$80; starts with a NOP, as the course data's pointers
 * expect. Reached only by unit_objects_09c.
 * Entry: M 8-bit; X = object. Exit: A low byte = $80; N set.
 */
void ObjectColour0(Carried *cr)
{
    st8i_hi(cr, F_c(0x2018, cr->x), cyc_then(30, 0x00));
    cyc(44);
}

void abi_ObjectColour0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectColour0(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C5BF ObjectColour1
 * WHAT IT DOES: Sets object X's colour +$18 to 1 with bit 7.
 * HOW: LDA #1, ORA #$80; starts with a NOP, as the course data's pointers
 * expect. Reached only by unit_objects_09c.
 * Entry: M 8-bit; X = object. Exit: A low byte = $81; N set.
 */
void ObjectColour1(Carried *cr)
{
    st8i_hi(cr, F_c(0x2018, cr->x), cyc_then(30, 0x01));
    cyc(44);
}

void abi_ObjectColour1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectColour1(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C5C8 ObjectColour2
 * WHAT IT DOES: Sets object X's colour +$18 to 2 with bit 7.
 * HOW: LDA #2, ORA #$80; starts with a NOP, as the course data's pointers
 * expect.
 * Entry: M 8-bit; X = object. Exit: A low byte = $82; N set.
 */
void ObjectColour2(Carried *cr)
{
    st8i_hi(cr, F_c(0x2018, cr->x), cyc_then(30, 0x02));
    cyc(44);
}

void abi_ObjectColour2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectColour2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C5D1 ObjectColour3
 * WHAT IT DOES: Sets object X's colour +$18 to 3 with bit 7.
 * HOW: LDA #3, ORA #$80; starts with a NOP, as the course data's pointers
 * expect.
 * Entry: M 8-bit; X = object. Exit: A low byte = $83; N set.
 */
void ObjectColour3(Carried *cr)
{
    st8i_hi(cr, F_c(0x2018, cr->x), cyc_then(30, 0x03));
    cyc(44);
}

void abi_ObjectColour3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectColour3(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C5DA ObjectColour4
 * WHAT IT DOES: Sets object X's colour +$18 to 4 with bit 7.
 * HOW: LDA #4, ORA #$80; starts with a NOP, as the course data's pointers
 * expect.
 * Entry: M 8-bit; X = object. Exit: A low byte = $84; N set.
 */
void ObjectColour4(Carried *cr)
{
    st8i_hi(cr, F_c(0x2018, cr->x), cyc_then(30, 0x04));
    cyc(44);
}

void abi_ObjectColour4(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectColour4(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C5E3 ObjectColour5
 * WHAT IT DOES: Sets object X's colour +$18 to 5 with bit 7.
 * HOW: LDA #5, ORA #$80; starts with a NOP, as the course data's pointers
 * expect.
 * Entry: M 8-bit; X = object. Exit: A low byte = $85; N set.
 */
void ObjectColour5(Carried *cr)
{
    st8i_hi(cr, F_c(0x2018, cr->x), cyc_then(30, 0x05));
    cyc(44);
}

void abi_ObjectColour5(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectColour5(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C5EC ObjectColour6
 * WHAT IT DOES: Sets object X's colour +$18 to 6 with bit 7.
 * HOW: LDA #6, ORA #$80; starts with a NOP, as the course data's pointers
 * expect.
 * Entry: M 8-bit; X = object. Exit: A low byte = $86; N set.
 */
void ObjectColour6(Carried *cr)
{
    st8i_hi(cr, F_c(0x2018, cr->x), cyc_then(30, 0x06));
    cyc(44);
}

void abi_ObjectColour6(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectColour6(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C5F5 ObjectColour7
 * WHAT IT DOES: Sets object X's colour +$18 to 7 with bit 7.
 * HOW: LDA #7, ORA #$80; starts with a NOP, as the course data's pointers
 * expect.
 * Entry: M 8-bit; X = object. Exit: A low byte = $87; N set.
 */
void ObjectColour7(Carried *cr)
{
    st8i_hi(cr, F_c(0x2018, cr->x), cyc_then(30, 0x07));
    cyc(44);
}

void abi_ObjectColour7(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectColour7(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C5FE ObjectScaleE0
 * WHAT IT DOES: Sets object X's scale +$15 to $E0.
 * HOW: One store; starts with a NOP, as the course data's pointers expect.
 * Entry: M 8-bit; X = object. Exit: A low byte = $E0.
 */
void ObjectScaleE0(Carried *cr)
{
    st8i(cr, F_c(0x2015, cr->x), cyc_then(20, 0xE0));
    cyc(44);
}

void abi_ObjectScaleE0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectScaleE0(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C605 ObjectScaleA0
 * WHAT IT DOES: Sets object X's scale +$15 to $A0.
 * HOW: One store; starts with a NOP, as the course data's pointers expect.
 * Entry: M 8-bit; X = object. Exit: A low byte = $A0.
 */
void ObjectScaleA0(Carried *cr)
{
    st8i(cr, F_c(0x2015, cr->x), cyc_then(20, 0xA0));
    cyc(44);
}

void abi_ObjectScaleA0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectScaleA0(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C60C ObjectScale20
 * WHAT IT DOES: Sets object X's scale +$15 to $20.
 * HOW: One store; starts with a NOP, as the course data's pointers expect.
 * Entry: M 8-bit; X = object. Exit: A low byte = $20.
 */
void ObjectScale20(Carried *cr)
{
    st8i(cr, F_c(0x2015, cr->x), cyc_then(20, 0x20));
    cyc(44);
}

void abi_ObjectScale20(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectScale20(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:C62D: the swing's frame: by +$25 bit 0 it moves $28 left with its
   scale +$15 eased toward $40, or $28 right toward $C0
   (ApproachEighthMin8Byte); +$0A counts down, and at 0 the side flips and
   +$0A = $50. */
static void swing_frame(Carried *cr)
{
    if (bit25(cr, 0x01)) {
        cyc(66);
        add16(cr, F_c(0x200C, cr->x), 0x0028);
        (cyc(4), ease8(cr, F_c(0x2015, cr->x), 0xC0, ApproachEighthMin8Byte));
        cyc(22);
    } else {
        cyc(40);
        add16(cr, F_c(0x200C, cr->x), 0xFFD8);
        (cyc(4), ease8(cr, F_c(0x2015, cr->x), 0x40, ApproachEighthMin8Byte));
        cyc(46);                        /* JMP */
    }
    if (ld8(cr, F_c(0x200A, cr->x))) {
        cyc(44);
        dec8(cr, F_c(0x200A, cr->x));
        cyc(86);                        /* JMP, JSL */
    } else {
        cyc(70);
        eor8(cr, F_c(0x2025, cr->x), 0x01);
        st8i(cr, F_c(0x200A, cr->x), cyc_then(6, 0x50));
        cyc(62);                        /* JSL */
    }
    ObjectRebucket_cr(cr);
    cyc(44);
}

/*
 * @routine 09:C613 ObjectSwing
 * WHAT IT DOES: Sets up a swinging object: scale +$15 = 0, +$0A = $28,
 * frame $09:C62D (ObjectSwingStep); then its frame.
 * HOW: Byte and word stores; starts with a NOP, as the course data's
 * pointers expect.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectSwing(Carried *cr)
{
    st8i(cr, F_c(0x2015, cr->x), cyc_then(20, 0x00));
    st8i(cr, F_c(0x200A, cr->x), cyc_then(6, 0x28));
    cyc(22);
    handler(cr, cr->x, 0xC62D);
    swing_frame(cr);
}

void abi_ObjectSwing(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwing(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C62D ObjectSwingStep
 * WHAT IT DOES: The swing's frame: it moves $28 to one side (+$25 bit 0:
 * right) with its scale +$15 eased toward $40 (left) or $C0 (right); every
 * $50 frames (+$0A) it turns to the other side.
 * HOW: 16-bit ADC on +$0C; ApproachEighthMin8Byte through $3E;
 * ObjectRebucket.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
 * leaves it.
 */
void ObjectSwingStep(Carried *cr)
{
    swing_frame(cr);
}

void abi_ObjectSwingStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwingStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A new object of `kind` at X's position moved (dx, dy, dz), scale +$15 =
   s15, +$14 = $F8, life +$0A = t, handler h, sound $52; nothing without a
   free object. */
static void crowd_spawn(Carried *cr, uint16_t dx, uint16_t dy, uint16_t dz, uint8_t s15, uint8_t t, uint16_t h)
{
    cyc(28);
    if (!spawn(cr, 0xB770))
        { cyc(68); return; }
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    add16(cr, F_c(0x200C, cr->y), cyc_then(22, dx));
    add16(cr, F_c(0x200E, cr->y), cyc_then(44, dy));
    add16(cr, F_c(0x2010, cr->y), cyc_then(44, dz));
    st8i(cr, F_c(0x2015, cr->y), cyc_then(28, s15));
    st8i(cr, F_c(0x2014, cr->y), cyc_then(6, 0xF8));
    st8i(cr, F_c(0x200A, cr->y), cyc_then(6, t));
    cyc(22);
    handler(cr, cr->y, h);
    sound0(cr, 0x52);
    cyc(42);
}

/*
 * @routine 09:C694 LapCrowdSpawn0
 * WHAT IT DOES: The lap crowd gate's lap 0 crowd: kind $B770 at the gate
 * moved ($F00, -$1C0, -$2800), scale $40, life $1C, handler $09:C7C9
 * (LapCrowdStep0), sound $52; nothing without a free object.
 * HOW: ObjectSpawn, ObjectCopyPositionTo, 16-bit ADCs. Reached only by
 * unit_objects_09c.
 * Entry: M 8-bit, X 16-bit; X = the gate. Exit: as SoundPort0QueuePush or
 * ObjectSpawn leaves it.
 */
void LapCrowdSpawn0(Carried *cr)
{
    crowd_spawn(cr, 0x0F00, 0xFE40, 0xD800, 0x40, 0x1C, 0xC7C9);
}

void abi_LapCrowdSpawn0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapCrowdSpawn0(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C6FB LapCrowdSpawn1
 * WHAT IT DOES: The lap 1 crowd: kind $B770 moved (-$A00, -$1E0, -$2BC0),
 * scale $E0, life $16, handler $09:C7FC (LapCrowdStep1), sound $52.
 * HOW: As LapCrowdSpawn0. Reached only by unit_objects_09c.
 * Entry: as LapCrowdSpawn0. Exit: as LapCrowdSpawn0.
 */
void LapCrowdSpawn1(Carried *cr)
{
    crowd_spawn(cr, 0xF600, 0xFE20, 0xD440, 0xE0, 0x16, 0xC7FC);
}

void abi_LapCrowdSpawn1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapCrowdSpawn1(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C762 LapCrowdSpawn2
 * WHAT IT DOES: The lap 2 crowd: kind $B770 moved ($3C0, -$1E0, -$2F80),
 * scale 0, life $16, handler $09:C83D (LapCrowdStep2), sound $52.
 * HOW: As LapCrowdSpawn0. Reached only by unit_objects_09c.
 * Entry: as LapCrowdSpawn0. Exit: as LapCrowdSpawn0.
 */
void LapCrowdSpawn2(Carried *cr)
{
    crowd_spawn(cr, 0x03C0, 0xFE20, 0xD080, 0x00, 0x16, 0xC83D);
}

void abi_LapCrowdSpawn2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapCrowdSpawn2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* DEC +$0A; at 0, ObjectRelease. `next` is the time of what follows on both
   paths (REP 22, JSL 62, RTS 42); the skipped release costs BEQ and JML. */
static void life_down(Carried *cr, unsigned next)
{
    if (!dec8(cr, F_c(0x200A, cr->x)))
        { cyc(84); ObjectRelease_cr(cr); cyc(next); } else { cyc(48 + next); }
}

/*
 * @routine 09:C7D2 LapCrowdMove0
 * WHAT IT DOES: Moves the lap 0 crowd by (-$B4, -$0A, 0); released when its
 * life +$0A runs out.
 * HOW: 16-bit ADCs, DEC. Reached only by unit_objects_09c.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as DEC or ObjectRelease
 * leaves it.
 */
void LapCrowdMove0(Carried *cr)
{
    add16(cr, F_c(0x200C, cr->x), cyc_then(82, 0xFF4C));
    add16(cr, F_c(0x200E, cr->x), cyc_then(44, 0xFFF6));
    (cyc(44), life_down(cr, 42));
}

void abi_LapCrowdMove0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapCrowdMove0(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C805 LapCrowdMove1
 * WHAT IT DOES: Moves the lap 1 crowd by ($104, -$0A, $C8); released when
 * its life +$0A runs out.
 * HOW: 16-bit ADCs, DEC. Reached only by unit_objects_09c.
 * Entry: as LapCrowdMove0. Exit: as LapCrowdMove0.
 */
void LapCrowdMove1(Carried *cr)
{
    add16(cr, F_c(0x200C, cr->x), cyc_then(82, 0x0104));
    add16(cr, F_c(0x200E, cr->x), cyc_then(44, 0xFFF6));
    add16(cr, F_c(0x2010, cr->x), cyc_then(44, 0x00C8));
    (cyc(44), life_down(cr, 42));
}

void abi_LapCrowdMove1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapCrowdMove1(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C846 LapCrowdMove2
 * WHAT IT DOES: Moves the lap 2 crowd by (0, -$0D, $C8); released when its
 * life +$0A runs out.
 * HOW: 16-bit ADCs, DEC. Reached only by unit_objects_09c.
 * Entry: as LapCrowdMove0. Exit: as LapCrowdMove0.
 */
void LapCrowdMove2(Carried *cr)
{
    add16(cr, F_c(0x200E, cr->x), cyc_then(82, 0xFFF3));
    add16(cr, F_c(0x2010, cr->x), cyc_then(44, 0x00C8));
    (cyc(44), life_down(cr, 42));
}

void abi_LapCrowdMove2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapCrowdMove2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C7C9 LapCrowdStep0
 * WHAT IT DOES: The lap 0 crowd's frame: LapCrowdMove0, then ObjectRebucket.
 * HOW: Starts with a NOP, as the handler pointers expect. Reached only by
 * unit_objects_09c.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket leaves it.
 */
void LapCrowdStep0(Carried *cr)
{
    LapCrowdMove0(cr);
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

void abi_LapCrowdStep0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapCrowdStep0(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C7FC LapCrowdStep1
 * WHAT IT DOES: The lap 1 crowd's frame: LapCrowdMove1, then ObjectRebucket.
 * HOW: Starts with a NOP. Reached only by unit_objects_09c.
 * Entry: as LapCrowdStep0. Exit: as LapCrowdStep0.
 */
void LapCrowdStep1(Carried *cr)
{
    LapCrowdMove1(cr);
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

void abi_LapCrowdStep1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapCrowdStep1(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C83D LapCrowdStep2
 * WHAT IT DOES: The lap 2 crowd's frame: LapCrowdMove2, then ObjectRebucket.
 * HOW: Starts with a NOP. Reached only by unit_objects_09c.
 * Entry: as LapCrowdStep0. Exit: as LapCrowdStep0.
 */
void LapCrowdStep2(Carried *cr)
{
    LapCrowdMove2(cr);
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

void abi_LapCrowdStep2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapCrowdStep2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:C8BA: synced to its record (ObjectSyncToRecord); a car's hit on a lap
   not counted yet (+$48) counts it and brings out that lap's crowd (laps 0
   to 2). */
static void crowd_gate_frame(Carried *cr)
{
    cyc(62);
    uint8_t lap;

    ObjectSyncToRecord_cr(cr);
    cyc(12);
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if (!(uint8_t)cr->a)
        { cyc(50); return; }
    (cyc(62), RecordContactSummary_cr(cr));
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if (!(uint8_t)cr->a)
        return; else { cyc(12); }
    if (lap_eq(cr, 0x2048))
        { cyc(66); return; } else { cyc(26); }
    lap_store(cr, 0x2048);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    lap = ld8(cr, (cyc(38), db(cr)) | W(wram_0DEA));
    cyc(56);
    cmp8(cr, 0x00);
    if (lap == 0x00) {
        LapCrowdSpawn0(cr);
        { cyc(68); return; }
    } else { cyc(32); }
    cmp8(cr, 0x01);
    if (lap == 0x01) {
        LapCrowdSpawn1(cr);
        { cyc(68); return; }
    } else { cyc(32); }
    cmp8(cr, 0x02);
    if (lap == 0x02)
        LapCrowdSpawn2(cr);
    cyc(44);
}

/*
 * @routine 09:C870 ObjectLapCrowdGate
 * WHAT IT DOES: Sets up the lap crowd gate: restart $09:C893
 * (ObjectLapCrowdGateRecord), frame $09:C8BA (ObjectLapCrowdGateStep), +$48
 * = 4 (no lap counted); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectLapCrowdGate(Carried *cr)
{
    handlers2(cr, 0xC893, 0xC8BA);
    st8i(cr, F_c(0x2048, cr->x), cyc_then(6, 0x04));
    gate42d(cr, 0x0780);
    crowd_gate_frame(cr);
}

void abi_ObjectLapCrowdGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapCrowdGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C893 ObjectLapCrowdGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectLapCrowdGate. Exit: as ObjectLapCrowdGate.
 */
void ObjectLapCrowdGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    crowd_gate_frame(cr);
}

void abi_ObjectLapCrowdGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapCrowdGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C8BA ObjectLapCrowdGateStep
 * WHAT IT DOES: The gate's frame: synced to its record; a car's hit on a lap
 * ($0DEA) it has not counted (+$48) counts that lap and, on laps 0, 1 and
 * 2, brings out that lap's crowd (LapCrowdSpawn0-2).
 * HOW: ObjectSyncToRecord, RecordContactSummary; the laps compared through
 * $1F58; JSRs.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectLapCrowdGateStep(Carried *cr)
{
    crowd_gate_frame(cr);
}

void abi_ObjectLapCrowdGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapCrowdGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C90C ObjectFloaterSpawner
 * WHAT IT DOES: Sets object X's frame to $09:C956 (ObjectFloaterSpawnerStep)
 * and puts a floater (kind $B3B8) $6E0 lower with life +$0A = $28 and
 * handler $09:C95C (ObjectFloaterStep); no floater without a free object.
 * HOW: ObjectSpawn, ObjectCopyPositionTo.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last store
 * or ObjectSpawn leaves it.
 */
void ObjectFloaterSpawner(Carried *cr)
{
    cyc(22);
    handler(cr, cr->x, 0xC956);
    if (!spawn(cr, 0xB3B8))
        { cyc(70); return; }
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    add16(cr, F_c(0x200E, cr->y), cyc_then(22, 0x06E0));
    st8i(cr, F_c(0x200A, cr->y), cyc_then(28, 0x28));
    cyc(22);
    handler(cr, cr->y, 0xC95C);
    cyc(44);
}

void abi_ObjectFloaterSpawner(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFloaterSpawner(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C956 ObjectFloaterSpawnerStep
 * WHAT IT DOES: The spawner's frame: its scale +$15 = $40.
 * HOW: One store.
 * Entry: M 8-bit; X = object. Exit: A low byte = $40.
 */
void ObjectFloaterSpawnerStep(Carried *cr)
{
    st8i(cr, F_c(0x2015, cr->x), cyc_then(6, 0x40));
    cyc(44);
}

void abi_ObjectFloaterSpawnerStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFloaterSpawnerStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C95C ObjectFloaterStep
 * WHAT IT DOES: The floater's frame: rebucketed, its scale +$15 grows by 8;
 * it rises $14 (+$25 bit 0 clear) or sinks $14; every $28 frames (+$0A) the
 * direction flips.
 * HOW: ObjectRebucket first; 16-bit ADC on +$0E; EOR #1.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last store
 * leaves it.
 */
void ObjectFloaterStep(Carried *cr)
{
    (cyc(62), ObjectRebucket_cr(cr));
    add8(cr, F_c(0x2015, cr->x), cyc_then(0, 0x08));
    if (bit25(cr, 0x01))
        { cyc(66); add16(cr, F_c(0x200E, cr->x), 0xFFEC); cyc(44); /* SEP */ }
    else
        { cyc(40); add16(cr, F_c(0x200E, cr->x), 0x0014); cyc(68); /* SEP, JMP */ }
    if (ld8(cr, F_c(0x200A, cr->x))) {
        cyc(44);
        dec8(cr, F_c(0x200A, cr->x));
        cyc(68);                        /* JMP, RTL */
        return;
    } else { cyc(70); }
    eor8(cr, F_c(0x2025, cr->x), 0x01);
    st8i(cr, F_c(0x200A, cr->x), cyc_then(6, 0x28));
    cyc(44);
}

void abi_ObjectFloaterStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFloaterStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* LDA #r, STA lo, STA lo2, BIT #$80: the sign byte `hi` = $FF for a
   negative r, else 0 (r is a constant, so one of the two stores never
   runs). */
static void orbit_axis(Carried *cr, uint8_t r, uint32_t lo, uint32_t lo2, uint32_t hi, uint32_t abs8)
{
    /* abs8: 8 more a store when lo2 and hi are absolute ($05F2/$05F3) */
    lda8(cr, r);
    wr8(lo, r);
    cyc(16 + abs8);
    wr8(lo2, r);
    if (r & 0x80) {
        cyc(32);                        /* BIT #$80, BEQ not taken */
        lda8(cr, 0xFF);
        cyc(16 + abs8);
        wr8(hi, 0xFF);
        cyc(22);                        /* BRA */
    } else {
        cyc(38 + 16 + abs8);            /* BIT #$80, BEQ to the STZ */
        wr8(hi, 0x00);
    }
}

/* A sign byte's other arm: LDA #$FF, STA hi for a negative value, STZ hi
   for a positive one. */
static void sign_arm(Carried *cr, uint32_t hi, int neg)
{
    if (neg) {
        lda8(cr, 0xFF);
        cyc(8);
        wr8(hi, 0xFF);
        cyc(22);                        /* BRA */
    } else {
        cyc(12);
        wr8(hi, 0x00);
    }
}

/* LDA lo, STA hi (8-bit): the turned vector back as the next input. */
static void orbit_again(Carried *cr)
{
    wr8(dp(cr, 0x02), cyc_then(16, ld8(cr, dp(cr, 0x04))));
    wr8(dp(cr, 0x08), cyc_then(16, ld8(cr, dp(cr, 0x0A))));
    cyc(12);
    wr8(dp(cr, 0x94), cyc_then(16, ld8(cr, db(cr) | W(wram_05F2))));
}

/* A word at `at` shifted left five times (16-bit). */
static void asl5(Carried *cr, uint32_t at)
{
    int i;

    cyc(12);
    r_lda16(cr, at);
    for (i = 0; cyc_loop(14, i < 5); i++)
        r_asl16(cr);
    cyc((at & 0xFFFF) > 0xFF ? 24 : 16);   /* STA abs or dp */
    wr16(at, cr->a);
}

/* dst field of X = src field of Y + the word at `by` (16-bit). */
static void orbit_add(Carried *cr, uint16_t f, uint32_t by, uint32_t adc)
{
    r_lda16(cr, cyc_then(26, F_c(f, cr->y)));
    r_add16(cr, rd16(cyc_then(adc, by)));  /* CLC, ADC: 30 direct page, 38 absolute */
    wr16(F_c(f, cr->x), cyc_then(30, cr->a));
}

/* The orbit: Y = the centre (+$27); the vector (r1, r2, r3) turned by the
   centre's angle +$16 (RotateXY) and, with `xz`, +$14 (RotateXZFrom08) and
   +$15 (RotateXZ); 32 times the result from the centre's position is X's
   position. Direct page $02/$08/$94 in, $04/$0A/$05F2 out. With arm 1-3
   it is entered at the sign store of r1, r2 or r3 that the constant never
   takes (Y already the centre). */
static void orbit(Carried *cr, int arm, uint8_t r1, uint8_t r2, uint8_t r3, int xz)
{
    if (arm == 0) {
        cr->y = rd16(cyc_then(30, F_c(0x2027, cr->x)));
        set_p(cr, nz16(cr->p, cr->y));
        orbit_axis(cr, r1, dp(cr, 0x02), dp(cr, 0x04), dp(cr, 0x05), 0);
    } else if (arm == 1) {
        sign_arm(cr, dp(cr, 0x05), !(r1 & 0x80));
    }
    if (arm <= 1)
        orbit_axis(cr, r2, dp(cr, 0x08), dp(cr, 0x0A), dp(cr, 0x0B), 0);
    else if (arm == 2)
        sign_arm(cr, dp(cr, 0x0B), !(r2 & 0x80)); else { cyc(4); }
    cyc(8);
    if (arm <= 2)
        orbit_axis(cr, r3, dp(cr, 0x94), db(cr) | W(wram_05F2), db(cr) | W(rot_z_high), 8);
    else
        sign_arm(cr, db(cr) | W(rot_z_high), !(r3 & 0x80));
    cyc(22);
    ld8(cr, F_c(0x2016, cr->y));
    (cyc(62), RotateXY_cr(cr));
    orbit_again(cr);
    if (xz) {
        cyc(22);
        ld8(cr, F_c(0x2014, cr->y));
        (cyc(62), RotateXZFrom08_cr(cr));
        orbit_again(cr);
        ld8(cr, cyc_then(22, F_c(0x2015, cr->y)));
        (cyc(62), RotateXZ_cr(cr));
    }
    cyc(22);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    asl5(cr, dp(cr, 0x04));
    asl5(cr, dp(cr, 0x0A));
    asl5(cr, (cyc(12), db(cr)) | W(wram_05F2));
    orbit_add(cr, 0x200C, dp(cr, 0x04), 30);
    orbit_add(cr, 0x2010, (cyc(4), db(cr)) | W(wram_05F2), 38);
    orbit_add(cr, 0x200E, dp(cr, 0x0A), 30);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

/* An orbiter's set-up: a centre of kind $A040 at X's position with handler
   h (+$27 of X = the centre), X's frame f; no centre without a free
   object. */
static void orbiter(Carried *cr, uint16_t h, uint16_t f)
{
    if (spawn(cr, 0xA040)) {
        cyc(62);
        uint8_t a;

        ObjectCopyPositionTo_cr(cr);
        a = (uint8_t)cr->a;
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cr->a = cr->y;
        set_p(cr, nz16(cr->p, cr->a));
        wr16(F_c(0x2027, cr->x), cyc_then(88, cr->y));
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(52);
        lda8(cr, a);
        handler(cr, cr->y, h);
    } else { cyc(26); }
    cyc(22);
    handler(cr, cr->x, f);
}

/* LDA +15 of Y, STA +15 of X; ObjectRebucket. */
static void orbit_scale_done(Carried *cr)
{
    ld8(cr, cyc_then(44, F_c(0x2015, cr->y)));
    wr8(F_c(0x2015, cr->x), cyc_then(30, (uint8_t)cr->a));
}

/* 09:C9F0: circles its centre at radius ($64, 0, 0) turned three ways. */
static void ring_frame(Carried *cr, int arm)
{
    orbit(cr, arm, 0x64, 0x00, 0x00, 1);
    orbit_scale_done(cr);
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

/*
 * @routine 09:C9B0 ObjectRing
 * WHAT IT DOES: Sets up a ring piece circling a centre: the centre (kind
 * $A040, handler $09:CAA3 ObjectRingCentreStep) at its position, its frame
 * $09:C9F0 (ObjectRingStep); then that frame.
 * HOW: ObjectSpawn, ObjectCopyPositionTo; +$27 = the centre. Reached only
 * by unit_objects_09c.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectRing(Carried *cr)
{
    orbiter(cr, 0xCAA3, 0xC9F0);
    ring_frame(cr, 0);
}

void abi_ObjectRing(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRing(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C9F0 ObjectRingStep
 * WHAT IT DOES: The ring piece's frame: at its centre (+$27) plus ($64, 0,
 * 0) turned by the centre's angles +$16, +$14 and +$15, times 32; it takes
 * the centre's scale +$15.
 * HOW: RotateXY, RotateXZFrom08, RotateXZ through direct page; five ASLs;
 * ObjectRebucket. The sign bytes come from constants: their $FF stores
 * (09:C9FD, 09:CA0F, 09:CA22) run only as the XSign, YSign and ZSign
 * entries. Reached only by unit_objects_09c.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
 * leaves it.
 */
void ObjectRingStep(Carried *cr)
{
    ring_frame(cr, 0);
}

void abi_ObjectRingStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRingStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C9FD ObjectRingStepXSign
 * WHAT IT DOES: ObjectRingStep entered at the sign store of its first
 * offset component that the constant $64 never takes: $05 = $FF, then the
 * rest of the frame as ObjectRingStep.
 * HOW: LDA #$FF and the store (a positive constant takes the STZ), then
 * the join; no code reaches these bytes, a direct call is their only way
 * in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its centre; D = $0000; DB =
 * $7E. Exit: as ObjectRingStep.
 */
void ObjectRingStepXSign(Carried *cr)
{
    ring_frame(cr, 1);
}

void abi_ObjectRingStepXSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRingStepXSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CA0F ObjectRingStepYSign
 * WHAT IT DOES: ObjectRingStep entered at the sign store of its second
 * offset component that the constant $00 never takes: $0B = $FF, then the
 * rest of the frame as ObjectRingStep.
 * HOW: LDA #$FF and the store (a positive constant takes the STZ), then
 * the join; no code reaches these bytes, a direct call is their only way
 * in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its centre; D = $0000; DB =
 * $7E. Exit: as ObjectRingStep.
 */
void ObjectRingStepYSign(Carried *cr)
{
    ring_frame(cr, 2);
}

void abi_ObjectRingStepYSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRingStepYSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CA22 ObjectRingStepZSign
 * WHAT IT DOES: ObjectRingStep entered at the sign store of its third
 * offset component that the constant $00 never takes: $05F3 = $FF, then
 * the rest of the frame as ObjectRingStep.
 * HOW: LDA #$FF and the store (a positive constant takes the STZ), then
 * the join; no code reaches these bytes, a direct call is their only way
 * in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its centre; D = $0000; DB =
 * $7E. Exit: as ObjectRingStep.
 */
void ObjectRingStepZSign(Carried *cr)
{
    ring_frame(cr, 3);
}

void abi_ObjectRingStepZSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRingStepZSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CAA3 ObjectRingCentreStep
 * WHAT IT DOES: The ring centre's frame: its scale +$15 grows by 1.
 * HOW: ADC; ObjectRebucket. Reached only by unit_objects_09c.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket leaves it.
 */
void ObjectRingCentreStep(Carried *cr)
{
    add8(cr, F_c(0x2015, cr->x), 0x01);
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

void abi_ObjectRingCentreStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRingCentreStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:CAF8: circles its centre at ($28, $0A, 0); its colour +$5A steps
   through 0-5 with bit 7. */
static void spinner_frame(Carried *cr, int arm)
{
    uint8_t v;

    orbit(cr, arm, 0x28, 0x0A, 0x00, 1);
    orbit_scale_done(cr);
    v = ld8(cr, cyc_then(22, F_c(0x205A, cr->x)));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    v = alu_adc8(v, 0x01, &cr->p);
    if (!(v & 0x80)) {
        cyc(24);
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
        v = alu_adc8(v, 0x06, &cr->p);
    }
    v &= 0x7F;
    lda8(cr, v);
    cmp8(cr, 0x06);
    if (v >= 0x06) {
        cyc(8);
        set_p(cr, (uint8_t)(cr->p | (P_C)));
        v = alu_sbc8(v, 0x06, &cr->p);
    }
    v |= 0x80;
    lda8(cr, v);
    wr8(F_c(0x205A, cr->x), cyc_then(112, v));
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

/*
 * @routine 09:CAB1 ObjectSpinner
 * WHAT IT DOES: Sets up a spinner: colour +$5A = 0 with bit 7, a centre
 * (kind $A040, handler $09:CBC4 ObjectSpinnerCentreStep) at its position,
 * its frame $09:CAF8 (ObjectSpinnerStep); then that frame.
 * HOW: ObjectSpawn, ObjectCopyPositionTo; +$27 = the centre.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectSpinner(Carried *cr)
{
    lda8(cr, 0x00);
    lda8(cr, 0x80);
    wr8(F_c(0x205A, cr->x), cyc_then(22, 0x80));
    orbiter(cr, 0xCBC4, 0xCAF8);
    spinner_frame(cr, 0);
}

void abi_ObjectSpinner(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpinner(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CAF8 ObjectSpinnerStep
 * WHAT IT DOES: The spinner's frame: at its centre (+$27) plus ($28, $0A,
 * 0) turned by the centre's angles, times 32, with the centre's scale; its
 * colour +$5A steps on through 0-5 (bit 7 kept).
 * HOW: As ObjectRingStep; the colour by ADC, AND #$7F, SBC #6. The sign
 * bytes come from constants: their $FF stores (09:CB05, 09:CB17, 09:CB2A)
 * run only as the XSign, YSign and ZSign entries.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
 * leaves it.
 */
void ObjectSpinnerStep(Carried *cr)
{
    spinner_frame(cr, 0);
}

void abi_ObjectSpinnerStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpinnerStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CB05 ObjectSpinnerStepXSign
 * WHAT IT DOES: ObjectSpinnerStep entered at the sign store of its first
 * offset component that the constant $28 never takes: $05 = $FF, then the
 * rest of the frame as ObjectSpinnerStep.
 * HOW: LDA #$FF and the store (a positive constant takes the STZ), then
 * the join; no code reaches these bytes, a direct call is their only way
 * in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its centre; D = $0000; DB =
 * $7E. Exit: as ObjectSpinnerStep.
 */
void ObjectSpinnerStepXSign(Carried *cr)
{
    spinner_frame(cr, 1);
}

void abi_ObjectSpinnerStepXSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpinnerStepXSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CB17 ObjectSpinnerStepYSign
 * WHAT IT DOES: ObjectSpinnerStep entered at the sign store of its second
 * offset component that the constant $0A never takes: $0B = $FF, then the
 * rest of the frame as ObjectSpinnerStep.
 * HOW: LDA #$FF and the store (a positive constant takes the STZ), then
 * the join; no code reaches these bytes, a direct call is their only way
 * in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its centre; D = $0000; DB =
 * $7E. Exit: as ObjectSpinnerStep.
 */
void ObjectSpinnerStepYSign(Carried *cr)
{
    spinner_frame(cr, 2);
}

void abi_ObjectSpinnerStepYSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpinnerStepYSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CB2A ObjectSpinnerStepZSign
 * WHAT IT DOES: ObjectSpinnerStep entered at the sign store of its third
 * offset component that the constant $00 never takes: $05F3 = $FF, then
 * the rest of the frame as ObjectSpinnerStep.
 * HOW: LDA #$FF and the store (a positive constant takes the STZ), then
 * the join; no code reaches these bytes, a direct call is their only way
 * in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its centre; D = $0000; DB =
 * $7E. Exit: as ObjectSpinnerStep.
 */
void ObjectSpinnerStepZSign(Carried *cr)
{
    spinner_frame(cr, 3);
}

void abi_ObjectSpinnerStepZSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpinnerStepZSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CBC4 ObjectSpinnerCentreStep
 * WHAT IT DOES: The spinner centre's frame: scale +$15 down by 8, +$14 up by
 * $0C.
 * HOW: ADCs; ObjectRebucket.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket leaves it.
 */
void ObjectSpinnerCentreStep(Carried *cr)
{
    add8(cr, F_c(0x2015, cr->x), 0xF8);
    add8(cr, F_c(0x2014, cr->x), 0x0C);
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

void abi_ObjectSpinnerCentreStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSpinnerCentreStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:CC20: a car's hit, the first time (+$25 bit 0), rings sound $2E; no
   hit clears the bit. */
static void bell_frame(Carried *cr)
{
    if (hit(cr)) {
        cyc(12);
        if (bit25(cr, 0x01))
            { cyc(88); return; } else { cyc(18); }
        or8(cr, F_c(0x2025, cr->x), 0x01);
        sound0(cr, 0x2E);
        { cyc(44); return; }
    } else { cyc(18); }
    and8(cr, F_c(0x2025, cr->x), 0xFE);
    cyc(44);
}

/*
 * @routine 09:CBDB ObjectBellGate
 * WHAT IT DOES: Sets up a bell gate: restart $09:CBF9 (ObjectBellGateRecord),
 * frame $09:CC20 (ObjectBellGateStep); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBellGate(Carried *cr)
{
    handlers2(cr, 0xCBF9, 0xCC20);
    gate42d(cr, 0x0F00);
    bell_frame(cr);
}

void abi_ObjectBellGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBellGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CBF9 ObjectBellGateRecord
 * WHAT IT DOES: The bell gate's restart: its record ($42D, flag bit 4, at
 * ($F00, $F00, $F00)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectBellGate. Exit: as ObjectBellGate.
 */
void ObjectBellGateRecord(Carried *cr)
{
    gate42d(cr, 0x0F00);
    bell_frame(cr);
}

void abi_ObjectBellGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBellGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CC20 ObjectBellGateStep
 * WHAT IT DOES: The bell gate's frame: a car's hit, the first time (+$25 bit
 * 0), plays sound $2E; no hit clears the bit.
 * HOW: ObjectSyncFromRecord, RecordContactSummary.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectBellGateStep(Carried *cr)
{
    bell_frame(cr);
}

void abi_ObjectBellGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBellGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A lap sign: kind $ABD8 at X's position moved (dx, dy, dz), life +$0A =
   $14, handler h; nothing without a free object. */
static void sign_spawn(Carried *cr, uint16_t dx, uint16_t dy, uint16_t dz, uint16_t h)
{
    cyc(70);
    if (!spawn(cr, 0xABD8))
        { cyc(68); return; }
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    add16(cr, F_c(0x200C, cr->y), cyc_then(22, dx));
    add16(cr, F_c(0x200E, cr->y), cyc_then(44, dy));
    add16(cr, F_c(0x2010, cr->y), cyc_then(44, dz));
    st8i(cr, F_c(0x200A, cr->y), cyc_then(28, 0x14));
    cyc(22);
    handler(cr, cr->y, h);
    cyc(42);
}

/*
 * @routine 09:CCEF LapSignSpawn0
 * WHAT IT DOES: The lap sign gate's lap 0 sign: kind $ABD8 moved ($A00,
 * -$100, $140), life $14, handler $09:CDF4 (LapSignStep0); nothing without
 * a free object.
 * HOW: ObjectSpawn, ObjectCopyPositionTo, 16-bit ADCs. Reached only by
 * unit_objects_09c.
 * Entry: M 8-bit, X 16-bit; X = the gate. Exit: as the last store or
 * ObjectSpawn leaves it.
 */
void LapSignSpawn0(Carried *cr)
{
    sign_spawn(cr, 0x0A00, 0xFF00, 0x0140, 0xCDF4);
}

void abi_LapSignSpawn0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapSignSpawn0(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CD46 LapSignSpawn1
 * WHAT IT DOES: The lap 1 sign: kind $ABD8 moved (-$A00, -$100, $140), life
 * $14, handler $09:CE30 (LapSignStep1).
 * HOW: As LapSignSpawn0. Reached only by unit_objects_09c.
 * Entry: as LapSignSpawn0. Exit: as LapSignSpawn0.
 */
void LapSignSpawn1(Carried *cr)
{
    sign_spawn(cr, 0xF600, 0xFF00, 0x0140, 0xCE30);
}

void abi_LapSignSpawn1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapSignSpawn1(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CD9D LapSignSpawn2
 * WHAT IT DOES: The lap 2 sign: kind $ABD8 at the gate (moved by 0), life
 * $14, handler $09:CE6C (LapSignStep2).
 * HOW: As LapSignSpawn0. Reached only by unit_objects_09c.
 * Entry: as LapSignSpawn0. Exit: as LapSignSpawn0.
 */
void LapSignSpawn2(Carried *cr)
{
    sign_spawn(cr, 0x0000, 0x0000, 0x0000, 0xCE6C);
}

void abi_LapSignSpawn2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapSignSpawn2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A lap sign's frame: moved (dx, dy, dz), released when +$0A runs out,
   rebucketed. */
static void sign_frame(Carried *cr, uint16_t dx, uint16_t dy, uint16_t dz)
{
    add16(cr, F_c(0x200C, cr->x), cyc_then(22, dx));
    add16(cr, F_c(0x200E, cr->x), cyc_then(44, dy));
    add16(cr, F_c(0x2010, cr->x), cyc_then(44, dz));
    (cyc(44), life_down(cr, 62));
    ObjectRebucket_cr(cr);
    cyc(44);
}

/*
 * @routine 09:CDF4 LapSignStep0
 * WHAT IT DOES: The lap 0 sign's frame: it slides -$E0 sideways and is
 * released when its life +$0A runs out.
 * HOW: 16-bit ADCs (two of 0), DEC, ObjectRebucket. Reached only by
 * unit_objects_09c.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket leaves it.
 */
void LapSignStep0(Carried *cr)
{
    sign_frame(cr, 0xFF20, 0x0000, 0x0000);
}

void abi_LapSignStep0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapSignStep0(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CE30 LapSignStep1
 * WHAT IT DOES: The lap 1 sign's frame: it slides $E0 sideways and is
 * released when its life +$0A runs out.
 * HOW: As LapSignStep0. Reached only by unit_objects_09c.
 * Entry: as LapSignStep0. Exit: as LapSignStep0.
 */
void LapSignStep1(Carried *cr)
{
    sign_frame(cr, 0x00E0, 0x0000, 0x0000);
}

void abi_LapSignStep1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapSignStep1(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CE6C LapSignStep2
 * WHAT IT DOES: The lap 2 sign's frame: it rises $20 and is released when
 * its life +$0A runs out.
 * HOW: As LapSignStep0. Reached only by unit_objects_09c.
 * Entry: as LapSignStep0. Exit: as LapSignStep0.
 */
void LapSignStep2(Carried *cr)
{
    sign_frame(cr, 0x0000, 0xFFE0, 0x0000);
}

void abi_LapSignStep2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapSignStep2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:CC9D: a car's hit on a lap not counted yet (+$48) counts it and puts
   up that lap's sign (laps 0 to 2). */
static void sign_gate_frame(Carried *cr)
{
    uint8_t lap;

    if (!hit(cr))
        { cyc(62); return; } else { cyc(12); }
    if (lap_eq(cr, 0x2048))
        { cyc(66); return; } else { cyc(26); }
    lap_store(cr, 0x2048);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    lap = ld8(cr, (cyc(38), db(cr)) | W(wram_0DEA));
    cyc(14);
    cmp8(cr, 0x00);
    if (lap == 0x00) {
        LapSignSpawn0(cr);
        { cyc(68); return; }
    } else { cyc(32); }
    cmp8(cr, 0x01);
    if (lap == 0x01) {
        LapSignSpawn1(cr);
        { cyc(68); return; }
    } else { cyc(32); }
    cmp8(cr, 0x02);
    if (lap == 0x02)
        { LapSignSpawn2(cr); cyc(44); return; }
    cyc(86);
}

/*
 * @routine 09:CC53 ObjectLapSignGate
 * WHAT IT DOES: Sets up the lap sign gate: restart $09:CC76
 * (ObjectLapSignGateRecord), frame $09:CC9D (ObjectLapSignGateStep), +$48 =
 * 0; its record and frame.
 * HOW: Word and byte stores. Reached only by unit_objects_09c.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectLapSignGate(Carried *cr)
{
    handlers2(cr, 0xCC76, 0xCC9D);
    st8i(cr, F_c(0x2048, cr->x), cyc_then(6, 0x00));
    gate42d(cr, 0x0F00);
    sign_gate_frame(cr);
}

void abi_ObjectLapSignGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapSignGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CC76 ObjectLapSignGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at ($F00,
 * $F00, $F00)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_objects_09c.
 * Entry: as ObjectLapSignGate. Exit: as ObjectLapSignGate.
 */
void ObjectLapSignGateRecord(Carried *cr)
{
    gate42d(cr, 0x0F00);
    sign_gate_frame(cr);
}

void abi_ObjectLapSignGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapSignGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CC9D ObjectLapSignGateStep
 * WHAT IT DOES: The gate's frame: a car's hit on a lap ($0DEA) it has not
 * counted (+$48) counts it and, on laps 0, 1 and 2, puts up that lap's
 * sign (LapSignSpawn0-2).
 * HOW: ObjectSyncFromRecord, RecordContactSummary; the laps compared
 * through $1F58; JSRs. Reached only by unit_objects_09c.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectLapSignGateStep(Carried *cr)
{
    sign_gate_frame(cr);
}

void abi_ObjectLapSignGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapSignGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:CEE8 / D023: circles its centre at ($64, $46, 0) (turned by +$16
   only, or three ways with `xz`) taking the centre's angle +$16; at angles
   up to $3A or from $C0 (signed, against $3A and $C0) it is the plain model
   $A040 and +$25 bit 0 clears, between them model `lit` and the first
   time (+$25 bit 0) sound `snd` (none when 0); the angle steps by $AA. */
static void fan_frame(Carried *cr, int arm, int xz, uint16_t lit, uint8_t snd)
{
    uint8_t a;

    orbit(cr, arm, 0x64, 0x46, 0x00, xz);
    ld8(cr, cyc_then(44, F_c(0x2016, cr->y)));
    wr8(F_c(0x2016, cr->x), cyc_then(30, (uint8_t)cr->a));
    a = ld8(cr, cyc_then(22, F_c(0x2016, cr->x)));
    cmp8(cr, 0x3A);
    if (a == 0x3A || (cyc(50), ((uint8_t)(a - 0x3A) & 0x80)))
        goto plain; else { cyc(52); }
    ld8(cr, F_c(0x2016, cr->x));
    cmp8(cr, 0xC0);
    if (!(a == 0xC0 || (cyc(76), ((uint8_t)(a - 0xC0) & 0x80))))
        goto plain;
    st16i(cr, F_c(0x2004, cr->x), lit);
    cyc(22);
    if (!bit25(cr, 0x01)) {
        cyc(18);
        or8(cr, F_c(0x2025, cr->x), 0x01);
        if (snd)
            sound0(cr, snd);
    } else { cyc(44); }
    goto step;
plain:
    cyc(26);
    st16i(cr, F_c(0x2004, cr->x), 0xA040);
    cyc(22);
    and8(cr, F_c(0x2025, cr->x), 0xFE);
cyc(24);
step:
    add8(cr, F_c(0x2016, cr->x), 0xAA);
    cyc(62);
    ObjectRebucket_cr(cr);
    cyc(44);
}

/*
 * @routine 09:CEA8 ObjectFan
 * WHAT IT DOES: Sets up a fan blade circling a centre: the centre (kind
 * $A040, handler $09:CFD5 ObjectFanCentreStep), its frame $09:CEE8
 * (ObjectFanStep); then that frame.
 * HOW: ObjectSpawn, ObjectCopyPositionTo; +$27 = the centre.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectFan(Carried *cr)
{
    orbiter(cr, 0xCFD5, 0xCEE8);
    fan_frame(cr, 0, 0, 0xAF04, 0x00);
}

void abi_ObjectFan(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFan(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CEE8 ObjectFanStep
 * WHAT IT DOES: The blade's frame: at its centre (+$27) plus ($64, $46, 0)
 * turned by the centre's angle +$16, times 32, with that angle; at angles
 * from $3B to $BF (signed compares with $3A and $C0) it shows model $AF04
 * (+$25 bit 0 set), otherwise $A040 (bit 0 cleared); its angle then steps
 * by $AA.
 * HOW: RotateXY only; five ASLs; ObjectRebucket. The sign bytes come from
 * constants: their $FF stores (09:CEF5, 09:CF07, 09:CF1A) run
 * only as the XSign, YSign and ZSign entries.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
 * leaves it.
 */
void ObjectFanStep(Carried *cr)
{
    fan_frame(cr, 0, 0, 0xAF04, 0x00);
}

void abi_ObjectFanStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFanStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CEF5 ObjectFanStepXSign
 * WHAT IT DOES: ObjectFanStep entered at the sign store of its first
 * offset component that the constant $64 never takes: $05 = $FF, then the
 * rest of the frame as ObjectFanStep.
 * HOW: LDA #$FF and the store (a positive constant takes the STZ), then
 * the join; no code reaches these bytes, a direct call is their only way
 * in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its centre; D = $0000; DB =
 * $7E. Exit: as ObjectFanStep.
 */
void ObjectFanStepXSign(Carried *cr)
{
    fan_frame(cr, 1, 0, 0xAF04, 0x00);
}

void abi_ObjectFanStepXSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFanStepXSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CF07 ObjectFanStepYSign
 * WHAT IT DOES: ObjectFanStep entered at the sign store of its second
 * offset component that the constant $46 never takes: $0B = $FF, then the
 * rest of the frame as ObjectFanStep.
 * HOW: LDA #$FF and the store (a positive constant takes the STZ), then
 * the join; no code reaches these bytes, a direct call is their only way
 * in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its centre; D = $0000; DB =
 * $7E. Exit: as ObjectFanStep.
 */
void ObjectFanStepYSign(Carried *cr)
{
    fan_frame(cr, 2, 0, 0xAF04, 0x00);
}

void abi_ObjectFanStepYSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFanStepYSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CF1A ObjectFanStepZSign
 * WHAT IT DOES: ObjectFanStep entered at the sign store of its third
 * offset component that the constant $00 never takes: $05F3 = $FF, then
 * the rest of the frame as ObjectFanStep.
 * HOW: LDA #$FF and the store (a positive constant takes the STZ), then
 * the join; no code reaches these bytes, a direct call is their only way
 * in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its centre; D = $0000; DB =
 * $7E. Exit: as ObjectFanStep.
 */
void ObjectFanStepZSign(Carried *cr)
{
    fan_frame(cr, 3, 0, 0xAF04, 0x00);
}

void abi_ObjectFanStepZSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFanStepZSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CFD5 ObjectFanCentreStep
 * WHAT IT DOES: The fan centre's frame: its angle +$16 turns by -8.
 * HOW: ADC; ObjectRebucket.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket leaves it.
 */
void ObjectFanCentreStep(Carried *cr)
{
    add8(cr, F_c(0x2016, cr->x), 0xF8);
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

void abi_ObjectFanCentreStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFanCentreStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:CFE3 ObjectWheel
 * WHAT IT DOES: Sets up a wheel piece circling a centre: the centre (kind
 * $A040, handler $09:D131 ObjectWheelCentreStep), its frame $09:D023
 * (ObjectWheelStep); then that frame.
 * HOW: ObjectSpawn, ObjectCopyPositionTo; +$27 = the centre.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectWheel(Carried *cr)
{
    orbiter(cr, 0xD131, 0xD023);
    fan_frame(cr, 0, 1, 0xAF20, 0x38);
}

void abi_ObjectWheel(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWheel(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:D023 ObjectWheelStep
 * WHAT IT DOES: The wheel piece's frame: as ObjectFanStep, turned three ways
 * (+$16, +$14, +$15), lit as model $AF20 with sound $38 the first time.
 * HOW: RotateXY, RotateXZFrom08, RotateXZ; five ASLs; ObjectRebucket. The
 * sign bytes come from constants: their $FF stores (09:D030, 09:D042,
 * 09:D055) run only as the XSign, YSign and ZSign entries.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
 * leaves it.
 */
void ObjectWheelStep(Carried *cr)
{
    fan_frame(cr, 0, 1, 0xAF20, 0x38);
}

void abi_ObjectWheelStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWheelStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:D030 ObjectWheelStepXSign
 * WHAT IT DOES: ObjectWheelStep entered at the sign store of its first
 * offset component that the constant $64 never takes: $05 = $FF, then the
 * rest of the frame as ObjectWheelStep.
 * HOW: LDA #$FF and the store (a positive constant takes the STZ), then
 * the join; no code reaches these bytes, a direct call is their only way
 * in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its centre; D = $0000; DB =
 * $7E. Exit: as ObjectWheelStep.
 */
void ObjectWheelStepXSign(Carried *cr)
{
    fan_frame(cr, 1, 1, 0xAF20, 0x38);
}

void abi_ObjectWheelStepXSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWheelStepXSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:D042 ObjectWheelStepYSign
 * WHAT IT DOES: ObjectWheelStep entered at the sign store of its second
 * offset component that the constant $46 never takes: $0B = $FF, then the
 * rest of the frame as ObjectWheelStep.
 * HOW: LDA #$FF and the store (a positive constant takes the STZ), then
 * the join; no code reaches these bytes, a direct call is their only way
 * in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its centre; D = $0000; DB =
 * $7E. Exit: as ObjectWheelStep.
 */
void ObjectWheelStepYSign(Carried *cr)
{
    fan_frame(cr, 2, 1, 0xAF20, 0x38);
}

void abi_ObjectWheelStepYSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWheelStepYSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:D055 ObjectWheelStepZSign
 * WHAT IT DOES: ObjectWheelStep entered at the sign store of its third
 * offset component that the constant $00 never takes: $05F3 = $FF, then
 * the rest of the frame as ObjectWheelStep.
 * HOW: LDA #$FF and the store (a positive constant takes the STZ), then
 * the join; no code reaches these bytes, a direct call is their only way
 * in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = its centre; D = $0000; DB =
 * $7E. Exit: as ObjectWheelStep.
 */
void ObjectWheelStepZSign(Carried *cr)
{
    fan_frame(cr, 3, 1, 0xAF20, 0x38);
}

void abi_ObjectWheelStepZSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWheelStepZSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:D131 ObjectWheelCentreStep
 * WHAT IT DOES: The wheel centre's frame: its angle +$16 turns by 8.
 * HOW: ADC; ObjectRebucket.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket leaves it.
 */
void ObjectWheelCentreStep(Carried *cr)
{
    add8(cr, F_c(0x2016, cr->x), 0x08);
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

void abi_ObjectWheelCentreStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWheelCentreStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* One piece of the breakable's burst: kind $ABF4 at X's position, handler
   $09:C4C0 (ObjectBreakablePiece), the height moved by 0, velocity (vx, vy,
   vz), life +$0A = $0F; nothing without a free object. */
static void burst_piece(Carried *cr, uint16_t vx, uint16_t vy, uint16_t vz)
{
    if (!spawn(cr, 0xABF4))
        { cyc(26); return; }
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    cyc(22);
    handler(cr, cr->y, 0xC4C0);
    add16(cr, F_c(0x200E, cr->y), cyc_then(22, 0x0000));
    st16i(cr, F_c(0x202E, cr->y), cyc_then(44, vx));
    st16i(cr, F_c(0x2030, cr->y), cyc_then(44, vy));
    st16i(cr, F_c(0x2032, cr->y), cyc_then(44, vz));
    st8i(cr, F_c(0x200A, cr->y), cyc_then(28, 0x0F));
}

/*
 * @routine 09:D79C ObjectBreakableBurst
 * WHAT IT DOES: Bursts the breakable into eight pieces (kind $ABF4, handler
 * ObjectBreakablePiece, life $0F) flying out at ($1B3, -5, $302), ($186, -7,
 * $2B2), (-$15E, -$0A, 0), (-$1A4, -$0A, $316), ($258, -$0A, $212), (-$127,
 * -$14, $19F), (-$1A4, -5, 0) and (-$104, -$0A, -$244); a piece is skipped
 * without a free object.
 * HOW: Eight ObjectSpawn blocks from object X's position. Reached only by
 * unit_objects_09c.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last piece
 * leaves it.
 */
void ObjectBreakableBurst(Carried *cr)
{
    burst_piece(cr, 0x01B3, 0xFFFB, 0x0302);
    burst_piece(cr, 0x0186, 0xFFF9, 0x02B2);
    burst_piece(cr, 0xFEA2, 0xFFF6, 0x0000);
    burst_piece(cr, 0xFE5C, 0xFFF6, 0x0316);
    burst_piece(cr, 0x0258, 0xFFF6, 0x0212);
    burst_piece(cr, 0xFED9, 0xFFEC, 0x019F);
    burst_piece(cr, 0xFE5C, 0xFFFB, 0x0000);
    burst_piece(cr, 0xFEFC, 0xFFF6, 0xFDBC);
    cyc(44);
}

void abi_ObjectBreakableBurst(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakableBurst(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ---- 09:D13F-E3E3: rails, breakable lap gates, bursts, lamp posts ---- */

void abi_GsuRecordNew424(void);

/* The colour +$5A stepped on through 0 to n-1 (bit 7 kept): ADC #1, and
   from a value without bit 7, ADC #n first; AND #$7F, CMP #n. Returns
   whether it reached n (carry), A = the masked step. */
static int colour_step(Carried *cr, uint8_t n)
{
    uint8_t v;

    v = ld8(cr, cyc_then(22, F_c(0x205A, cr->x)));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    v = alu_adc8(v, 0x01, &cr->p);
    if (!(v & 0x80)) {
        cyc(24);
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
        v = alu_adc8(v, n, &cr->p);
    }
    v &= 0x7F;
    lda8(cr, v);
    cmp8(cr, n);
    return v >= n;
}

/* The colour +$5A stepped through 0-5 (SBC #6 at 6). */
static void colour_cycle6(Carried *cr)
{
    uint8_t v;

    if (colour_step(cr, 0x06)) {
        cyc(8);
        set_p(cr, (uint8_t)(cr->p | (P_C)));
        v = alu_sbc8((uint8_t)cr->a, 0x06, &cr->p);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    }
    v = (uint8_t)(cr->a | 0x80);
    lda8(cr, v);
    wr8(F_c(0x205A, cr->x), cyc_then(112, v));
}

/* LDA dp $00, ORA #$80, STA field f of object o: the colour from the
   direct page's first byte. */
static void colour_from_dp0(Carried *cr, uint32_t at)
{
    cyc(4);
    uint8_t v = (uint8_t)(ld8(cr, dp(cr, 0x00)) | 0x80);

    lda8(cr, v);
    cyc(26);
    wr8(at, v);
}

/* REP #$20, LDA src, STA dst, SEP #$20 (16-bit copy). */
static void copy16(Carried *cr, uint32_t dst, uint32_t src)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(52);
    r_lda16(cr, src);
    cyc((dst & 0xFFFF) >= 0x2000 ? 30 : 24);   /* STA abs,X (an object field) or abs */
    wr16(dst, cr->a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

/* The rail's slide: +$10 stepped by `step` toward the target +$27
   (through $1F4C); returns 1 when it was already there (nothing stored),
   else steps and syncs to the record (ObjectSyncToRecord). */
static int rail_slide(Carried *cr, uint16_t step)
{
    uint16_t t;

    copy16(cr, db(cr) | W(wram_1F4C), F_c(0x2027, cr->x));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(74, F_c(0x2010, cr->x)));
    t = rd16(cyc_then(24, db(cr) | W(wram_1F4C)));
    r_cmp16(cr, t);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (cr->a == t)
        { cyc(70); return 1; }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    if (cr->p & P_N)
        r_add16(cr, step);
    else
        { cyc(10); r_sub16(cr, step); }
    wr16(F_c(0x2010, cr->x), cyc_then(172, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    (cyc(84), ObjectSyncToRecord_cr(cr));
    return 0;
}

/*
 * @routine 09:D3FC RailStop
 * WHAT IT DOES: The rail has reached its target: its frame becomes $09:D40C
 * (RailDrop).
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: A low byte = 9.
 */
void RailStop(Carried *cr)
{
    cyc(22);
    handler(cr, cr->x, 0xD40C);
    cyc(44);
}

void abi_RailStop(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RailStop(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:D3AF RailStep
 * WHAT IT DOES: A rail's frame: its colour +$5A steps through 0-5; it
 * slides $280 at a time toward its target +$27 (+$10), and there its frame
 * becomes RailDrop.
 * HOW: ADC/AND/SBC on the colour; a 16-bit CMP, then ADC or SBC #$280 by
 * N; ObjectSyncToRecord; at the target a JML into RailStop.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectSyncToRecord or RailStop leaves it.
 */
void RailStep(Carried *cr)
{
    colour_cycle6(cr);
    if (rail_slide(cr, 0x0280))
        RailStop(cr); else { cyc(44); }
}

void abi_RailStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RailStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:D42C: the drop's frame: the colour counts up to 6 (then the rise,
   09:D460); until then +$10 grows by $40 and it syncs to its record. */

/*
 * @routine 09:D460 RailRise
 * WHAT IT DOES: The rail rises back: its target +$27 = +$10 + $500, the
 * colour from direct page $00, model +$04 = $B2D8, frame $09:D498
 * (RailRiseStep); then that frame.
 * HOW: 16-bit copy and ADC; word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void abi_RailRiseStep(void);

void RailRise(Carried *cr)
{
    copy16(cr, F_c(0x2027, cr->x), F_c(0x2010, cr->x));
    colour_from_dp0(cr, cyc_then(22, F_c(0x205A, cr->x)));
    st16i(cr, F_c(0x2004, cr->x), cyc_then(22, 0xB2D8));
    add16(cr, F_c(0x2027, cr->x), cyc_then(44, 0x0500));
    cyc(44);
    handler(cr, cr->x, 0xD498);
    RailRiseStep(cr);
}

void abi_RailRise(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RailRise(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:D498 RailRiseStep
 * WHAT IT DOES: The rising rail's frame: its colour steps through 0-5; it
 * slides $140 at a time toward its target +$27, and there it is released.
 * HOW: As RailStep with $140; at the target ObjectRelease.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectSyncToRecord or ObjectRelease leaves it.
 */
void RailRiseStep(Carried *cr)
{
    colour_cycle6(cr);
    if (rail_slide(cr, 0x0140))
        { cyc(62); ObjectRelease_cr(cr); cyc(44); } else { cyc(44); }
}

void abi_RailRiseStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RailRiseStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

static void rail_drop_frame(Carried *cr)
{
    uint8_t v;

    if (colour_step(cr, 0x07)) {
        lda8(cr, 0x07);
        lda8(cr, 0x06);
        lda8(cr, 0x86);
        wr8(F_c(0x205A, cr->x), cyc_then(96, 0x86));
        (cyc(24), RailRise(cr));
        return;
    }
    v = (uint8_t)(cr->a | 0x80);
    lda8(cr, v);
    wr8(F_c(0x205A, cr->x), cyc_then(112, v));
    add16(cr, F_c(0x2010, cr->x), cyc_then(22, 0x0040));
    (cyc(84), ObjectSyncToRecord_cr(cr));
    cyc(44);
}

/*
 * @routine 09:D40C RailDrop
 * WHAT IT DOES: The rail drops: colour from direct page $00, model +$04 =
 * $B2F4, frame $09:D42C (RailDropStep); then that frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void RailDrop(Carried *cr)
{
    colour_from_dp0(cr, F_c(0x205A, cr->x));
    st16i(cr, F_c(0x2004, cr->x), cyc_then(22, 0xB2F4));
    cyc(44);
    handler(cr, cr->x, 0xD42C);
    rail_drop_frame(cr);
}

void abi_RailDrop(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RailDrop(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:D42C RailDropStep
 * WHAT IT DOES: The dropping rail's frame: its colour counts on, and at 7
 * (set to 6) the rail rises back (RailRise); until then it moves $40 on
 * (+$10) and syncs to its record.
 * HOW: ADC/AND/CMP #7 on the colour; LDA #7, DEC A for the last one; a
 * JMP into RailRise.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as
 * ObjectSyncToRecord or RailRise leaves it.
 */
void RailDropStep(Carried *cr)
{
    rail_drop_frame(cr);
}

void abi_RailDropStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RailDropStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* One rail: kind $B2D8 coloured from direct page $00, scale $80, at X's
   position moved (dx, 0, dz), its target +$27 = its +$10 + t, handler
   $09:D3AF (RailStep); nothing without a free object. */
static void rail(Carried *cr, uint16_t dx, uint16_t dz, uint16_t t)
{
    if (!spawn(cr, 0xB2D8))
        { cyc(26); return; }
    colour_from_dp0(cr, F_c(0x205A, cr->y));
    st8i(cr, F_c(0x2015, cr->y), cyc_then(6, 0x80));
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    add16(cr, F_c(0x200C, cr->y), cyc_then(22, dx));
    add16(cr, F_c(0x2010, cr->y), cyc_then(44, dz));
    copy16(cr, F_c(0x2027, cr->y), cyc_then(22, F_c(0x2010, cr->y)));
    add16(cr, F_c(0x2027, cr->y), cyc_then(44, t));
    cyc(44);
    handler(cr, cr->y, 0xD3AF);
}

/*
 * @routine 09:D1AB RailsSpawn
 * WHAT IT DOES: Puts up the five rails of the finale gate: kind $B2D8 at
 * object X moved (0, 0), (-$280, -$280), (-$500, 0), (-$640, -$500) and
 * (-$8C0, 0) on X and Z, each sliding on to its +$10 + $1180, $1400,
 * $1180, $1680 and $1180 (RailStep); a rail is skipped without a free
 * object.
 * HOW: Five ObjectSpawn blocks.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last rail
 * leaves it.
 */
void RailsSpawn(Carried *cr)
{
    rail(cr, 0x0000, 0x0000, 0x1180);
    rail(cr, 0xFD80, 0xFD80, 0x1400);
    rail(cr, 0xFB00, 0x0000, 0x1180);
    rail(cr, 0xF9C0, 0xFB00, 0x1680);
    rail(cr, 0xF740, 0x0000, 0x1180);
    cyc(42);
}

void abi_RailsSpawn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RailsSpawn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:D184: a car's hit on lap 2 puts up the rails (RailsSpawn), plays sound
   $33 and releases the gate. */
static void finale_frame(Carried *cr)
{
    if (!hit(cr))
        { cyc(62); return; } else { cyc(36); }
    if (!lap_is(cr, 0x02))
        { cyc(34); return; } else { cyc(30); }
    RailsSpawn(cr);
    cyc(0);
    sound0(cr, 0x33);
    (cyc(62), ObjectRelease_cr(cr));
    cyc(44);
}

/*
 * @routine 09:D13F ObjectFinaleGate
 * WHAT IT DOES: Sets up the finale gate: restart $09:D15D
 * (ObjectFinaleGateRecord), frame $09:D184 (ObjectFinaleGateStep); its
 * record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectFinaleGate(Carried *cr)
{
    handlers2(cr, 0xD15D, 0xD184);
    gate42d(cr, 0x2A80);
    finale_frame(cr);
}

void abi_ObjectFinaleGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFinaleGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:D15D ObjectFinaleGateRecord
 * WHAT IT DOES: The finale gate's restart: its record ($42D, flag bit 4, at
 * ($2A80, $2A80, $2A80)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectFinaleGate. Exit: as ObjectFinaleGate.
 */
void ObjectFinaleGateRecord(Carried *cr)
{
    gate42d(cr, 0x2A80);
    finale_frame(cr);
}

void abi_ObjectFinaleGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFinaleGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:D184 ObjectFinaleGateStep
 * WHAT IT DOES: The finale gate's frame: a car's hit on lap 2 ($0DEA) puts
 * up the rails (RailsSpawn), plays sound $33 and releases the gate.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; JSR RailsSpawn.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectFinaleGateStep(Carried *cr)
{
    finale_frame(cr);
}

void abi_ObjectFinaleGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFinaleGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:D4EA ObjectScaleUp6
 * WHAT IT DOES: Grows object X's scale +$15 by 6.
 * HOW: CLC, ADC. Reached only by unit_objects_09d.
 * Entry: M 8-bit; X = object. Exit: A low byte = the new scale; N, Z, C, V
 * from the add.
 */
void ObjectScaleUp6(Carried *cr)
{
    add8(cr, F_c(0x2015, cr->x), 0x06);
    cyc(44);
}

void abi_ObjectScaleUp6(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectScaleUp6(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A breakable (kind $B4D0, handler $09:C513 ObjectBreakable) at X's
   position moved (dx, dy, dz) at rest, life +$0A = $1C; X's own size
   bytes +$14-$16 set to s; nothing without a free object. */
static void breakable_put(Carried *cr, uint16_t dy, uint16_t dz, uint16_t dx, uint8_t s)
{
    if (!spawn(cr, 0xB4D0))
        { cyc(26); return; }
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    cyc(22);
    handler(cr, cr->y, 0xC513);
    add16(cr, F_c(0x200E, cr->y), cyc_then(22, dy));
    add16(cr, F_c(0x2010, cr->y), cyc_then(44, dz));
    add16(cr, F_c(0x200C, cr->y), cyc_then(44, dx));
    st16i(cr, F_c(0x202E, cr->y), cyc_then(44, 0x0000));
    st16i(cr, F_c(0x2030, cr->y), cyc_then(44, 0x0000));
    st16i(cr, F_c(0x2032, cr->y), cyc_then(44, 0x0000));
    st8i(cr, F_c(0x2014, cr->x), cyc_then(28, s));
    st8i(cr, F_c(0x2015, cr->x), cyc_then(6, s));
    st8i(cr, F_c(0x2016, cr->x), cyc_then(6, s));
    st8i(cr, F_c(0x200A, cr->y), cyc_then(6, 0x1C));
}

/*
 * @routine 09:D58E BreakablePairA
 * WHAT IT DOES: Puts two breakables (ObjectBreakable) by object X: moved
 * (+$B40, -$320, -$680) and (+$F00, -$3C0, -$6E0) on X, Y and Z; X's size
 * bytes become $5A, then $23.
 * HOW: Two ObjectSpawn blocks. Reached only by unit_objects_09d.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last
 * block leaves it.
 */
void BreakablePairA(Carried *cr)
{
    breakable_put(cr, 0xFCE0, 0xF980, 0x0B40, 0x5A);
    breakable_put(cr, 0xFC40, 0xF920, 0x0F00, 0x23);
    cyc(42);
}

void abi_BreakablePairA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BreakablePairA(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:D695 BreakablePairB
 * WHAT IT DOES: Puts two breakables by object X: moved (+$500, -$640,
 * -$3C0) and (+$B40, -$780, -$280); X's size bytes become $5A, then $23.
 * HOW: As BreakablePairA. Nothing in the game reaches it; reached only by
 * unit_objects_09d.
 * Entry: as BreakablePairA. Exit: as BreakablePairA.
 */
void BreakablePairB(Carried *cr)
{
    breakable_put(cr, 0xF9C0, 0xFC40, 0x0500, 0x5A);
    breakable_put(cr, 0xF880, 0xFD80, 0x0B40, 0x23);
    cyc(42);
}

void abi_BreakablePairB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BreakablePairB(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:DDDE BreakablePairC
 * WHAT IT DOES: Puts two breakables by object X: moved (-$1180, -$500,
 * -$780) and (-$F00, -$5A0, -$6E0); X's size bytes become $5A, then $23.
 * HOW: As BreakablePairA. Reached only by unit_objects_09d.
 * Entry: as BreakablePairA. Exit: as BreakablePairA.
 */
void BreakablePairC(Carried *cr)
{
    breakable_put(cr, 0xFB00, 0xF880, 0xEE80, 0x5A);
    breakable_put(cr, 0xFA60, 0xF920, 0xF100, 0x23);
    cyc(42);
}

void abi_BreakablePairC(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BreakablePairC(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:DEE5 BreakablePairD
 * WHAT IT DOES: Puts two breakables by object X: moved (-$1180, -$500,
 * -$4C0) and (-$F00, -$5A0, -$5A0); X's size bytes become $5A, then $23.
 * HOW: As BreakablePairA. Reached only by unit_objects_09d.
 * Entry: as BreakablePairA. Exit: as BreakablePairA.
 */
void BreakablePairD(Carried *cr)
{
    breakable_put(cr, 0xFB00, 0xFB40, 0xEE80, 0x5A);
    breakable_put(cr, 0xFA60, 0xFA60, 0xF100, 0x23);
    cyc(42);
}

void abi_BreakablePairD(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BreakablePairD(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:D53E: a car's hit on a lap not counted yet (+$0A) counts it; on lap 2
   two breakables (BreakablePairA) and sound $53. */
static void breakable_gate_frame(Carried *cr)
{
    uint8_t lap;

    if (!hit(cr))
        { cyc(62); return; } else { cyc(12); }
    if (lap_eq(cr, 0x200A))
        { cyc(66); return; } else { cyc(4); }
    lap_store(cr, 0x200A);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    lap = ld8(cr, (cyc(38), db(cr)) | W(wram_0DEA));
    cyc(62);
    cmp8(cr, 0x00);
    if (lap == 0x00)
        { cyc(44); return; } else { cyc(32); }
    cmp8(cr, 0x01);
    if (lap == 0x01)
        { cyc(44); return; } else { cyc(26); }
    cmp8(cr, 0x02);
    if (lap != 0x02)
        { cyc(44); return; } else { cyc(28); }
    BreakablePairA(cr);
    sound0(cr, 0x53);
    cyc(44);
}

/*
 * @routine 09:D4F4 ObjectBreakableGate
 * WHAT IT DOES: Sets up the breakable gate: restart $09:D517
 * (ObjectBreakableGateRecord), frame $09:D53E (ObjectBreakableGateStep),
 * +$0A = 4 (no lap counted); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBreakableGate(Carried *cr)
{
    handlers2(cr, 0xD517, 0xD53E);
    st8i(cr, F_c(0x200A, cr->x), cyc_then(6, 0x04));
    gate42d(cr, 0x1E00);
    breakable_gate_frame(cr);
}

void abi_ObjectBreakableGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakableGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:D517 ObjectBreakableGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($1E00, $1E00, $1E00)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectBreakableGate. Exit: as ObjectBreakableGate.
 */
void ObjectBreakableGateRecord(Carried *cr)
{
    gate42d(cr, 0x1E00);
    breakable_gate_frame(cr);
}

void abi_ObjectBreakableGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakableGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:D53E ObjectBreakableGateStep
 * WHAT IT DOES: The gate's frame: a car's hit on a lap ($0DEA) it has not
 * counted (+$0A) counts it, and on lap 2 puts up two breakables
 * (BreakablePairA) with sound $53.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; the laps compared
 * through $1F58.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectBreakableGateStep(Carried *cr)
{
    breakable_gate_frame(cr);
}

void abi_ObjectBreakableGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakableGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* One piece of a burst: kind `kind` at X's position moved dy, handler
   $09:C4C0 (ObjectBreakablePiece), velocity (vx, vy, vz), life +$0A = $0F;
   nothing without a free object. */
static void burst_piece_k(Carried *cr, uint16_t kind, uint16_t dy, uint16_t vx, uint16_t vy, uint16_t vz)
{
    if (!spawn(cr, kind))
        { cyc(26); return; }
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    cyc(22);
    handler(cr, cr->y, 0xC4C0);
    add16(cr, F_c(0x200E, cr->y), cyc_then(22, dy));
    st16i(cr, F_c(0x202E, cr->y), cyc_then(44, vx));
    st16i(cr, F_c(0x2030, cr->y), cyc_then(44, vy));
    st16i(cr, F_c(0x2032, cr->y), cyc_then(44, vz));
    st8i(cr, F_c(0x200A, cr->y), cyc_then(28, 0x0F));
}

/*
 * @routine 09:DA5D ObjectBreakableBurst2
 * WHAT IT DOES: As ObjectBreakableBurst, the pieces alternating kinds $AC10
 * and $ABF4 (the last two both $AC10).
 * HOW: Eight ObjectSpawn blocks from object X's position.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last piece
 * leaves it.
 */
void ObjectBreakableBurst2(Carried *cr)
{
    burst_piece_k(cr, 0xAC10, 0x0000, 0x01B3, 0xFFFB, 0x0302);
    burst_piece_k(cr, 0xABF4, 0x0000, 0x0186, 0xFFF9, 0x02B2);
    burst_piece_k(cr, 0xAC10, 0x0000, 0xFEA2, 0xFFF6, 0x0000);
    burst_piece_k(cr, 0xABF4, 0x0000, 0xFE5C, 0xFFF6, 0x0316);
    burst_piece_k(cr, 0xAC10, 0x0000, 0x0258, 0xFFF6, 0x0212);
    burst_piece_k(cr, 0xABF4, 0x0000, 0xFED9, 0xFFEC, 0x019F);
    burst_piece_k(cr, 0xAC10, 0x0000, 0xFE5C, 0xFFFB, 0x0000);
    burst_piece_k(cr, 0xAC10, 0x0000, 0xFEFC, 0xFFF6, 0xFDBC);
    cyc(44);
}

void abi_ObjectBreakableBurst2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakableBurst2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:DD68: a car's hit on a lap not counted yet (+$0A) counts it: lap 0, a
   sweep with sound $36 on even clock ticks; lap 1, BreakablePairC; lap 2,
   BreakablePairD; each pair with sound $53. */
static void breakable_gate2_frame(Carried *cr)
{
    uint8_t lap, v;

    if (!hit(cr))
        { cyc(62); return; } else { cyc(12); }
    if (lap_eq(cr, 0x200A))
        { cyc(66); return; }
    cyc(4);
    lap_store(cr, 0x200A);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    lap = ld8(cr, (cyc(38), db(cr)) | W(wram_0DEA));
    cyc(76);
    cmp8(cr, 0x00);
    if (lap == 0x00) {
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        v = ld8(cr, db(cr) | W(race_time_fraction));
        set_p(cr, (uint8_t)((cr->p & ~P_C) | ((v & 0x08) ? P_C : (cyc(70), 0))));
        lda8(cr, (uint8_t)((v >> 4) & 0x01));
        if ((uint8_t)cr->a)
            { cyc(48); return; }
        or8l(cr, W(slide_flags), 0x10, 0);
        sound0(cr, 0x36);
        { cyc(68); return; }
    } else { cyc(40); }
    cmp8(cr, 0x01);
    if (lap == 0x01) {
        BreakablePairC(cr);
        sound0(cr, 0x53);
        { cyc(68); return; }
    } else { cyc(32); }
    cmp8(cr, 0x02);
    if (lap != 0x02)
        { cyc(16); return; }
    BreakablePairD(cr);
    sound0(cr, 0x53);
    cyc(44);
}

/*
 * @routine 09:DD1E ObjectBreakableGate2
 * WHAT IT DOES: Sets up the second breakable gate: restart $09:DD41
 * (ObjectBreakableGate2Record), frame $09:DD68 (ObjectBreakableGate2Step),
 * +$0A = 4; its record and frame.
 * HOW: Word and byte stores. Reached only by unit_objects_09d.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBreakableGate2(Carried *cr)
{
    handlers2(cr, 0xDD41, 0xDD68);
    st8i(cr, F_c(0x200A, cr->x), cyc_then(6, 0x04));
    gate42d(cr, 0x0780);
    breakable_gate2_frame(cr);
}

void abi_ObjectBreakableGate2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakableGate2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:DD41 ObjectBreakableGate2Record
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_objects_09d.
 * Entry: as ObjectBreakableGate2. Exit: as ObjectBreakableGate2.
 */
void ObjectBreakableGate2Record(Carried *cr)
{
    gate42d(cr, 0x0780);
    breakable_gate2_frame(cr);
}

void abi_ObjectBreakableGate2Record(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakableGate2Record(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:DD68 ObjectBreakableGate2Step
 * WHAT IT DOES: The gate's frame: a car's hit on a lap ($0DEA) it has not
 * counted (+$0A) counts it: on lap 0 a sweep ($0DA5 bit 4) with sound $36
 * when bit 4 of the clock fraction $0E63 is clear; on laps 1 and 2 two
 * breakables (BreakablePairC, BreakablePairD) with sound $53.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; the laps through $1F58;
 * $0DA5 by long accesses. Reached only by unit_objects_09d.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectBreakableGate2Step(Carried *cr)
{
    breakable_gate2_frame(cr);
}

void abi_ObjectBreakableGate2Step(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBreakableGate2Step(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:E018: the two-player gate's frame: with contacts, the summary's bit 0
   (player 1) the first time (+$25 bit 0) plays sound $5A for player 0
   ($1A5D = 0), its bit 1 the first time (+$25 bit 1) for player 1; each
   bit cleared when its contact is gone; no contacts clears both. */
static void duo_frame(Carried *cr)
{
    if (!synced(cr)) {
        cyc(28);
        and8(cr, F_c(0x2025, cr->x), 0xFE);
        and8(cr, F_c(0x2025, cr->x), 0xFD);
        cyc(44);
        return;
    } else { cyc(84); }
    RecordContactSummary_cr(cr);
    cyc(12);
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if (!(uint8_t)cr->a) {
        and8(cr, F_c(0x2025, cr->x), 0xFE);
        cyc(24);
    } else { cyc(6); if (!bit25(cr, 0x01)) {
        cyc(18);
        or8(cr, F_c(0x2025, cr->x), 0x01);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        st8i(cr, (cyc(22), db(cr)) | W(current_player), 0x00);
        sound0(cr, 0x5A);
    } else { cyc(44); } }
    cyc(62);
    RecordContactSummary_cr(cr);
    lda8(cr, (uint8_t)(cr->a & 0x02));
    if (!(uint8_t)cr->a) {
        cyc(12);
        and8(cr, F_c(0x2025, cr->x), 0xFD);
        cyc(66);
        return;
    } else { cyc(18); }
    if (bit25(cr, 0x02))
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x02);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    st8i(cr, (cyc(22), db(cr)) | W(current_player), 0x01);
    sound0(cr, 0x5A);
    cyc(44);
}

/* 09:E00A: a gate's record ($424, +$0A $90E8) with flag bit 4 and +$2A
   high h. */
static void record424alt(Carried *cr, uint8_t h)
{
    (cyc(62), GsuRecordNew424Alt_cr(cr));
    (cyc(62), GsuRecordFlag10_cr(cr));
    cyc(58);
    lda8(cr, h);
    GsuRecordSet2AHigh_cr(cr);
}

/*
 * @routine 09:DFEC ObjectDuoGate
 * WHAT IT DOES: Sets up the two-player gate: restart $09:E00A
 * (ObjectDuoGateRecord), frame $09:E018 (ObjectDuoGateStep); its record
 * and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectDuoGate(Carried *cr)
{
    handlers2(cr, 0xE00A, 0xE018);
    record424alt(cr, 0x14);
    (cyc(102), duo_frame(cr));
}

void abi_ObjectDuoGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectDuoGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E00A ObjectDuoGateRecord
 * WHAT IT DOES: The gate's restart: its record (GsuRecordNew424Alt, flag
 * bit 4, +$2A high $14), then its frame.
 * HOW: GsuRecordNew424Alt, GsuRecordFlag10, GsuRecordSet2AHigh.
 * Entry: as ObjectDuoGate. Exit: as ObjectDuoGate.
 */
void ObjectDuoGateRecord(Carried *cr)
{
    record424alt(cr, 0x14);
    (cyc(102), duo_frame(cr));
}

void abi_ObjectDuoGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectDuoGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E018 ObjectDuoGateStep
 * WHAT IT DOES: The two-player gate's frame: with contacts, the contact
 * summary's bit 0, the first time (+$25 bit 0), plays sound $5A for player
 * 0 ($1A5D = 0) and its bit 1, the first time (+$25 bit 1), for player 1;
 * a bit whose contact is gone is cleared, and both with no contacts.
 * HOW: ObjectSyncFromRecord, RecordContactSummary twice.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectDuoGateStep(Carried *cr)
{
    duo_frame(cr);
}

void abi_ObjectDuoGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectDuoGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:E0C8: a car's hit, the first time (+$25 bit 0), bursts the gate
   (ObjectBreakableBurst2) with sound $5A; no hit clears the bit. */
static void burst_gate_frame(Carried *cr)
{
    if (!hit(cr)) {
        cyc(18);
        and8(cr, F_c(0x2025, cr->x), 0xFE);
        cyc(44);
        return;
    } else { cyc(12); }
    if (bit25(cr, 0x01))
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    (cyc(62), ObjectBreakableBurst2(cr));
    sound0(cr, 0x5A);
    cyc(44);
}

/*
 * @routine 09:E097 ObjectBurstGate
 * WHAT IT DOES: Sets up a burst gate: restart $09:E0BA
 * (ObjectBurstGateRecord), frame $09:E0C8 (ObjectBurstGateStep), +$14 =
 * direct page $80; its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBurstGate(Carried *cr)
{
    handlers2(cr, 0xE0BA, 0xE0C8);
    cyc(4);
    ld8(cr, dp(cr, 0x80));
    wr8(F_c(0x2014, cr->x), cyc_then(30, (uint8_t)cr->a));
    record424alt(cr, 0x14);
    (cyc(102), burst_gate_frame(cr));
}

void abi_ObjectBurstGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBurstGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E0BA ObjectBurstGateRecord
 * WHAT IT DOES: The gate's restart: its record (GsuRecordNew424Alt, flag
 * bit 4, +$2A high $14), then its frame.
 * HOW: GsuRecordNew424Alt, GsuRecordFlag10, GsuRecordSet2AHigh.
 * Entry: as ObjectBurstGate. Exit: as ObjectBurstGate.
 */
void ObjectBurstGateRecord(Carried *cr)
{
    record424alt(cr, 0x14);
    (cyc(102), burst_gate_frame(cr));
}

void abi_ObjectBurstGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBurstGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E0C8 ObjectBurstGateStep
 * WHAT IT DOES: The burst gate's frame: a car's hit, the first time (+$25
 * bit 0), bursts it into eight pieces (ObjectBreakableBurst2) with sound
 * $5A; no hit clears the bit.
 * HOW: ObjectSyncFromRecord, RecordContactSummary.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectBurstGateStep(Carried *cr)
{
    burst_gate_frame(cr);
}

void abi_ObjectBurstGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBurstGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E0FF ObjectLampPost
 * WHAT IT DOES: Sets up a lamp post: $8C higher, lit by $9971 (+$24 bit 3,
 * +$3A = 0, +$5F = $64), frame $09:E185 (ObjectLampPostLight), and its lamp
 * (kind $A0E8, lit by $9973) $26C above it; no lamp without a free object.
 * HOW: ObjectSpawn, ObjectCopyPositionTo; the lamp rebucketed with X and
 * Y swapped (PHX, TYX, PLY) around ObjectRebucket. Reached only by
 * unit_objects_09d.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last swap
 * or ObjectSpawn leaves it.
 */
void ObjectLampPost(Carried *cr)
{
    add16(cr, F_c(0x200E, cr->x), cyc_then(22, 0xFF74));
    st16i(cr, F_c(0x205D, cr->x), cyc_then(44, 0x9971));
    cyc(22);
    lamp(cr, cr->x, 0x64);
    cyc(22);
    handler(cr, cr->x, 0xE185);
    if (!spawn(cr, 0xA0E8))
        { cyc(70); return; }
    lamp(cr, cr->y, 0x64);
    st16i(cr, F_c(0x205D, cr->y), cyc_then(22, 0x9973));
    (cyc(84), ObjectCopyPositionTo_cr(cr));
    add16(cr, F_c(0x200E, cr->y), cyc_then(22, 0xFD94));
    swap_xy(cr);
    ObjectRebucket_cr(cr);
    swap_xy_less(cr, 84);
    cyc(44);
}

void abi_ObjectLampPost(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLampPost(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E1FF LampPostBurst
 * WHAT IT DOES: Bursts a lamp post into four pieces (kind $ABF4, handler
 * ObjectBreakablePiece, life $0F) at heights -$5A, -$A0, -$5A and -$B4,
 * flying out at ($23, -2, $0A), (-$1E, -1, $0F), ($1E, -2, -$0D) and
 * (-$23, -1, -$0A).
 * HOW: Four ObjectSpawn blocks from object X's position. Reached only by
 * unit_objects_09d.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last piece
 * leaves it.
 */
void LampPostBurst(Carried *cr)
{
    cyc(62);
    burst_piece_k(cr, 0xABF4, 0xFFA6, 0x0023, 0xFFFE, 0x000A);
    burst_piece_k(cr, 0xABF4, 0xFF60, 0xFFE2, 0xFFFF, 0x000F);
    burst_piece_k(cr, 0xABF4, 0xFFA6, 0x001E, 0xFFFE, 0xFFF3);
    burst_piece_k(cr, 0xABF4, 0xFF4C, 0xFFDD, 0xFFFF, 0xFFF6);
}

void abi_LampPostBurst(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LampPostBurst(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E1CE LampPostHit
 * WHAT IT DOES: A car hits the lamp post: the first time ever (+$25 bit 2)
 * it bursts (LampPostBurst); the first time of this contact (+$25 bit 0)
 * sound $58.
 * HOW: Bit tests and ORs on +$25. Reached only by unit_objects_09d.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last step
 * leaves it.
 */
void LampPostHit(Carried *cr)
{
    if (!bit25(cr, 0x04)) {
        cyc(18);
        or8(cr, F_c(0x2025, cr->x), 0x04);
        LampPostBurst(cr);
    }
    cyc(44);
    if (bit25(cr, 0x01))
        { cyc(86); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    sound0(cr, 0x58);
    cyc(42);
}

void abi_LampPostHit(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LampPostHit(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:E1B1: a car's hit, LampPostHit; no hit clears +$25 bit 0. */
static void lamp_post_frame(Carried *cr)
{
    if (hit(cr)) {
        cyc(58);
        LampPostHit(cr);
        { cyc(44); return; }
    } else { cyc(18); }
    and8(cr, F_c(0x2025, cr->x), 0xFE);
    cyc(44);
}

/*
 * @routine 09:E185 ObjectLampPostLight
 * WHAT IT DOES: The lamp post's first frame: restart $09:E1A3
 * (ObjectLampPostRecord), frame $09:E1B1 (ObjectLampPostStep); its record
 * and frame.
 * HOW: Word and byte stores. Reached only by unit_objects_09d.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectLampPostLight(Carried *cr)
{
    handlers2(cr, 0xE1A3, 0xE1B1);
    record424alt(cr, 0x08);
    (cyc(102), lamp_post_frame(cr));
}

void abi_ObjectLampPostLight(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLampPostLight(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E1A3 ObjectLampPostRecord
 * WHAT IT DOES: The lamp post's restart: its record (GsuRecordNew424Alt,
 * flag bit 4, +$2A high 8), then its frame.
 * HOW: GsuRecordNew424Alt, GsuRecordFlag10, GsuRecordSet2AHigh. Reached
 * only by unit_objects_09d.
 * Entry: as ObjectLampPostLight. Exit: as ObjectLampPostLight.
 */
void ObjectLampPostRecord(Carried *cr)
{
    record424alt(cr, 0x08);
    (cyc(102), lamp_post_frame(cr));
}

void abi_ObjectLampPostRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLampPostRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E1B1 ObjectLampPostStep
 * WHAT IT DOES: The lamp post's frame: a car's hit runs LampPostHit (burst
 * the first time, sound $58 each new contact); no hit clears +$25 bit 0.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; JSR LampPostHit.
 * Reached only by unit_objects_09d.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectLampPostStep(Carried *cr)
{
    lamp_post_frame(cr);
}

void abi_ObjectLampPostStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLampPostStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:E391: while +$48 counts down, the bump grows (+$15 by 6); a car's hit,
   the first time (+$25 bit 0), sound $59 and +$48 = 6; no hit clears the
   bit. */
static void bump_post_frame(Carried *cr)
{
    if (ld8(cr, cyc_then(22, F_c(0x2048, cr->x)))) {
        cyc(22);
        add8(cr, F_c(0x2015, cr->x), 0x06);
        add8(cr, F_c(0x2048, cr->x), 0xFF);
    } else { cyc(48); }
    if (!hit(cr)) {
        cyc(18);
        and8(cr, F_c(0x2025, cr->x), 0xFE);
        cyc(44);
        return;
    } else { cyc(12); }
    if (bit25(cr, 0x01))
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    sound0(cr, 0x59);
    st8i(cr, F_c(0x2048, cr->x), cyc_then(6, 0x06));
    cyc(44);
}

/* 09:E383: the bump post's record ($424) with flag bit 4 and +$2A high 1. */
static void bump_post_record(Carried *cr)
{
    (cyc(62), GsuRecordNew424_cr(cr));
    (cyc(62), GsuRecordFlag10_cr(cr));
    lda8(cr, 0x01);
    (cyc(58), GsuRecordSet2AHigh_cr(cr));
}

/*
 * @routine 09:E360 ObjectBumpPost
 * WHAT IT DOES: Sets up a bump post: restart $09:E383 (ObjectBumpPostRecord),
 * frame $09:E391 (ObjectBumpPostStep), +$48 = 0; its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBumpPost(Carried *cr)
{
    handlers2(cr, 0xE383, 0xE391);
    st8i(cr, F_c(0x2048, cr->x), cyc_then(6, 0x00));
    bump_post_record(cr);
    (cyc(102), bump_post_frame(cr));
}

void abi_ObjectBumpPost(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBumpPost(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E383 ObjectBumpPostRecord
 * WHAT IT DOES: The bump post's restart: its record ($424, flag bit 4,
 * +$2A high 1), then its frame.
 * HOW: GsuRecordNew424, GsuRecordFlag10, GsuRecordSet2AHigh.
 * Entry: as ObjectBumpPost. Exit: as ObjectBumpPost.
 */
void ObjectBumpPostRecord(Carried *cr)
{
    bump_post_record(cr);
    (cyc(102), bump_post_frame(cr));
}

void abi_ObjectBumpPostRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBumpPostRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E391 ObjectBumpPostStep
 * WHAT IT DOES: The bump post's frame: while +$48 counts down it bulges
 * (+$15 by 6); a car's hit, the first time (+$25 bit 0), plays sound $59
 * and sets +$48 = 6; no hit clears the bit.
 * HOW: ADCs; ObjectSyncFromRecord, RecordContactSummary.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectBumpPostStep(Carried *cr)
{
    bump_post_frame(cr);
}

void abi_ObjectBumpPostStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBumpPostStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ---- 09:E3E4-ECDF: slide gates, meter, trails, window and slide gates ---- */


/*
 * @routine 09:E47D GateSlideStep
 * WHAT IT DOES: The slide gate's lap 1 step: the first time (+$25 bit 0)
 * the meter $1A29 = 1 with sound $5F; then +$0C slides $280 at a time
 * toward the target +$27, and once there $1A29 = 2.
 * HOW: A 16-bit CMP through $1F4C, then ADC or SBC #$280 by N.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A as the last
 * step leaves it.
 */
void GateSlideStep(Carried *cr)
{
    uint16_t t;

    if (!bit25(cr, 0x01)) {
        cyc(18);
        or8(cr, F_c(0x2025, cr->x), 0x01);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        st8i(cr, (cyc(22), db(cr)) | W(wram_1A29), 0x01);
        sound0(cr, 0x5F);
    } else { cyc(44); }
    copy16(cr, db(cr) | W(wram_1F4C), F_c(0x2027, cr->x));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(74, F_c(0x200C, cr->x)));
    t = rd16(cyc_then(24, db(cr) | W(wram_1F4C)));
    r_cmp16(cr, t);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (cr->a == t) {
        cyc(92);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        st8i(cr, db(cr) | W(wram_1A29), 0x02);
        return;
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    if (cr->p & P_N)
        r_add16(cr, 0x0280);
    else
        { cyc(10); r_sub16(cr, 0x0280); }
    wr16(F_c(0x200C, cr->x), cyc_then(172, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(44);
}

void abi_GateSlideStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    GateSlideStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:E441: synced to its record; a car's hit on lap 1 runs GateSlideStep,
   on lap 2 eases the scale +$15 toward $80 (ApproachHalfMin2Byte). */
static void slide_lap_frame(Carried *cr)
{
    cyc(62);
    uint8_t lap;

    ObjectSyncToRecord_cr(cr);
    cyc(12);
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if (!(uint8_t)cr->a)
        { cyc(50); return; } else { cyc(62); }
    RecordContactSummary_cr(cr);
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if (!(uint8_t)cr->a)
        return; else { cyc(50); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    lap = ld8(cr, db(cr) | W(wram_0DEA));
    cyc(62);
    cmp8(cr, 0x00);
    if (lap == 0x00)
        { cyc(44); return; } else { cyc(22); }
    cmp8(cr, 0x01);
    if (lap == 0x01) {
        cyc(32);
        GateSlideStep(cr);
        cyc(110);
        return;
    }
    cmp8(cr, 0x02);
    if (lap == 0x02) {
        ease8(cr, F_c(0x2015, cr->x), 0x80, ApproachHalfMin2Byte);
        cyc(44);
    } else { cyc(80); }
}

/*
 * @routine 09:E3E4 ObjectSlideLapGate
 * WHAT IT DOES: Sets up the sliding lap gate: restart $09:E41A
 * (ObjectSlideLapGateRecord), frame $09:E441 (ObjectSlideLapGateStep), its
 * slide target +$27 = +$0C - $2080; its record and frame.
 * HOW: Word and byte stores; a 16-bit copy and ADC.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectSlideLapGate(Carried *cr)
{
    handlers2(cr, 0xE41A, 0xE441);
    copy16(cr, F_c(0x2027, cr->x), F_c(0x200C, cr->x));
    add16(cr, F_c(0x2027, cr->x), cyc_then(44, 0xDF80));
    cyc(22);
    gate42d(cr, 0x2A80);
    slide_lap_frame(cr);
}

void abi_ObjectSlideLapGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideLapGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E41A ObjectSlideLapGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($2A80, $2A80, $2A80)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectSlideLapGate. Exit: as ObjectSlideLapGate.
 */
void ObjectSlideLapGateRecord(Carried *cr)
{
    gate42d(cr, 0x2A80);
    slide_lap_frame(cr);
}

void abi_ObjectSlideLapGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideLapGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E441 ObjectSlideLapGateStep
 * WHAT IT DOES: The gate's frame: synced to its record; a car's hit on lap
 * 1 slides it toward its target (GateSlideStep), on lap 2 eases its scale
 * +$15 toward $80.
 * HOW: ObjectSyncToRecord, RecordContactSummary; ApproachHalfMin2Byte
 * through $3E.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectSlideLapGateStep(Carried *cr)
{
    slide_lap_frame(cr);
}

void abi_ObjectSlideLapGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideLapGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* +$48 to $1F58, then +$5A = it with bit 7. */
static void show48(Carried *cr)
{
    uint8_t v;

    v = ld8(cr, F_c(0x2048, cr->x));
    wr8(db(cr) | W(wram_1F58), cyc_then(24, v));
    v = ld8(cr, (cyc(16), db(cr)) | W(wram_1F58));
    st8i_hi(cr, F_c(0x205A, cr->x), v);
}

/* 09:E4F2: the meter follows $1A29: 1 counts +$48 up to 7, 2 down to 0,
   shown at +$5A; 0 leaves it. */
static void meter_frame(Carried *cr)
{
    uint8_t v;

    set_p(cr, (uint8_t)(cr->p | (P_M)));
    v = ld8(cr, (cyc(38), db(cr)) | W(wram_1A29));
    if (v) {
        int up = v == 0x01;

        cyc(70);
        cmp8(cr, 0x01);
        if (v == 0x01) {
            v = ld8(cr, F_c(0x2048, cr->x));
            cyc(22);
            cmp8(cr, 0x07);
            if (v != 0x07)
                inc8(cr, F_c(0x2048, cr->x)); else { cyc(42); }
        } else {
            cyc(6);
            v = ld8(cr, F_c(0x2048, cr->x));
            cmp8(cr, 0x00);
            if (v != 0x00)
                { cyc(60); dec8(cr, F_c(0x2048, cr->x)); } else { cyc(64); }
        }
        cyc(22);
        show48(cr);
        cyc(up ? 84 : 62);              /* BRA (counting up), JSL */
    } else { cyc(84); }
    ObjectRebucket_cr(cr);
    cyc(44);
}

/*
 * @routine 09:E4D6 ObjectSlideMeter
 * WHAT IT DOES: Sets up the slide meter: frame $09:E4F2
 * (ObjectSlideMeterStep), +$48 = 0, +$5A = 0 with bit 7; then its frame.
 * HOW: Word and byte stores; starts with a NOP, as the course data's
 * pointers expect.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
 * leaves it.
 */
void ObjectSlideMeter(Carried *cr)
{
    cyc(36);
    handler(cr, cr->x, 0xE4F2);
    st8i(cr, F_c(0x2048, cr->x), cyc_then(6, 0x00));
    st8i_hi(cr, F_c(0x205A, cr->x), cyc_then(16, 0x00));
    meter_frame(cr);
}

void abi_ObjectSlideMeter(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideMeter(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E4F2 ObjectSlideMeterStep
 * WHAT IT DOES: The meter's frame: while the slide gate runs ($1A29 = 1)
 * +$48 counts up to 7, once it is there ($1A29 = 2) back down to 0; shown
 * at +$5A with bit 7.
 * HOW: CMP, INC/DEC; the count through $1F58; ObjectRebucket.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
 * leaves it.
 */
void ObjectSlideMeterStep(Carried *cr)
{
    meter_frame(cr);
}

void abi_ObjectSlideMeterStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideMeterStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:E581: $19E7 = 1 while a car hits it, else 0. */
static void trail_zone_frame(Carried *cr)
{
    if (hit(cr)) {
        cyc(34);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        st8i(cr, db(cr) | W(wram_19E7), 0x01);
        { cyc(44); return; }
    }
    wr8(db(cr) | W(wram_19E7), cyc_then(42, 0x00));
    cyc(44);
}

/*
 * @routine 09:E53C ObjectTrailZone
 * WHAT IT DOES: Sets up the trail zone: restart $09:E55A
 * (ObjectTrailZoneRecord), frame $09:E581 (ObjectTrailZoneStep); its record
 * and frame.
 * HOW: Word and byte stores. Reached only by unit_objects_09e.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectTrailZone(Carried *cr)
{
    handlers2(cr, 0xE55A, 0xE581);
    gate42d(cr, 0x1680);
    trail_zone_frame(cr);
}

void abi_ObjectTrailZone(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTrailZone(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E55A ObjectTrailZoneRecord
 * WHAT IT DOES: The zone's restart: its record ($42D, flag bit 4, at
 * ($1680, $1680, $1680)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only
 * by unit_objects_09e.
 * Entry: as ObjectTrailZone. Exit: as ObjectTrailZone.
 */
void ObjectTrailZoneRecord(Carried *cr)
{
    gate42d(cr, 0x1680);
    trail_zone_frame(cr);
}

void abi_ObjectTrailZoneRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTrailZoneRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E581 ObjectTrailZoneStep
 * WHAT IT DOES: The zone's frame: $19E7 = 1 while a car hits it, else 0
 * (the trails follow the player while it is set).
 * HOW: ObjectSyncFromRecord, RecordContactSummary; STZ. Reached only by
 * unit_objects_09e.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectTrailZoneStep(Carried *cr)
{
    trail_zone_frame(cr);
}

void abi_ObjectTrailZoneStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTrailZoneStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E59D TrailFlip
 * WHAT IT DOES: When the player's ($0346) record speed word (+$4A) is
 * negative, turns object X round: the word at its +$15 grows by $80.
 * HOW: PHX, TXY; the player's record through $7E:2022,X and $70:004A,X
 * (long, 16-bit); PLX. Reached only by unit_objects_09e.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X unchanged; Y =
 * the object; N, Z from X.
 */
void TrailFlip(Carried *cr)
{
    uint16_t sx = cr->x;

    cr->y = cr->x;
    set_p(cr, nz16(cr->p, cr->y));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->x = rd16(cyc_then(136, db(cr) | W(wram_0346)));
    set_p(cr, nz16(cr->p, cr->x));
    cyc(32);
    r_lda16(cr, (W(obj_22_long) + cr->x) & 0xFFFFFF);
    cr->x = cr->a;
    set_p(cr, nz16(cr->p, cr->x));
    cyc(46);
    r_lda16(cr, (0x70004A + cr->x) & 0xFFFFFF);
    if (cr->a & 0x8000)
        { cyc(16); add16(cr, F_c(0x2015, cr->y), 0x0080); } else { cyc(22); }
    cyc(106);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->x = sx;
    set_p(cr, nz16(cr->p, sx));
}

void abi_TrailFlip(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TrailFlip(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A trail's frame: while $19E7 is set it takes the player's ($0346) scale
   +$15 turned round ($80), eases its Z toward the player's
   (ApproachHalfMin2) then moves dz, its X toward the player's
   (ApproachQuarterMin4) then moves dx, life +$0A = $1A, TrailFlip, angle
   +$16 = 0; otherwise it counts +$0A down while moving (0 dropping by
   $100 a frame, and mx sideways). */
static void trail_frame(Carried *cr, uint16_t dz, uint16_t dx, uint16_t mx)
{
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (ld8(cr, (cyc(38), db(cr)) | W(wram_19E7))) {
        cr->y = rd16(cyc_then(40, db(cr) | W(wram_0346)));
        set_p(cr, nz16(cr->p, cr->y));
        ld8(cr, cyc_then(22, F_c(0x2015, cr->y)));
        wr8(F_c(0x2015, cr->x), cyc_then(30, (uint8_t)cr->a));
        add8(cr, F_c(0x2015, cr->x), 0x80);
        ease16(cr, F_c(0x2010, cr->x), F_c(0x2010, cr->y), ease_cdc1);
        add16(cr, F_c(0x2010, cr->x), cyc_then(44, dz));
        (cyc(22), ease16(cr, F_c(0x200C, cr->x), F_c(0x200C, cr->y), ApproachQuarterMin4));
        if (dx)
            { cyc(44); add16(cr, F_c(0x200C, cr->x), dx); }
        cyc(28);
        st8i(cr, F_c(0x200A, cr->x), 0x1A);
        TrailFlip(cr);
        st8i(cr, F_c(0x2016, cr->x), 0x00);
        (cyc(62), ObjectRebucket_cr(cr));
        { cyc(44); return; }
    } else { cyc(44); }
    if (ld8(cr, F_c(0x200A, cr->x))) {
        cyc(22);
        add8(cr, F_c(0x200A, cr->x), 0xFF);
        add16(cr, F_c(0x2010, cr->x), cyc_then(22, 0xFF00));
        if (mx)
            { cyc(44); add16(cr, F_c(0x200C, cr->x), mx); }
    } else { cyc(26); }
    cyc(84);
    ObjectRebucket_cr(cr);
    cyc(44);
}

/*
 * @routine 09:E5BD ObjectTrailMidStep
 * WHAT IT DOES: The middle trail's frame: while $19E7 is set it follows the
 * player ($0346), 3 behind, turned round, with life $1A (TrailFlip turns it
 * the player's way); otherwise it falls back $100 a frame while its life
 * +$0A lasts.
 * HOW: ApproachHalfMin2 and ApproachQuarterMin4 through $3E; JSR TrailFlip;
 * ObjectRebucket. Reached only by unit_objects_09e.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
 * leaves it.
 */
void ObjectTrailMidStep(Carried *cr)
{
    trail_frame(cr, 0xFFFD, 0x0000, 0x0000);
}

void abi_ObjectTrailMidStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTrailMidStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E641 ObjectTrailLeftStep
 * WHAT IT DOES: The left trail's frame: as ObjectTrailMidStep, 3 ahead and $1E
 * to the left; falling back it also drifts $40 right.
 * HOW: As ObjectTrailMidStep. Reached only by unit_objects_09e.
 * Entry: as ObjectTrailMidStep. Exit: as ObjectTrailMidStep.
 */
void ObjectTrailLeftStep(Carried *cr)
{
    trail_frame(cr, 0x0003, 0xFFE2, 0x0040);
}

void abi_ObjectTrailLeftStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTrailLeftStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E6E1 ObjectTrailRightStep
 * WHAT IT DOES: The right trail's frame: as ObjectTrailMidStep, 3 ahead and $1E
 * to the right; falling back it also drifts $40 left.
 * HOW: As ObjectTrailMidStep. Reached only by unit_objects_09e.
 * Entry: as ObjectTrailMidStep. Exit: as ObjectTrailMidStep.
 */
void ObjectTrailRightStep(Carried *cr)
{
    trail_frame(cr, 0x0003, 0x001E, 0xFFC0);
}

void abi_ObjectTrailRightStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectTrailRightStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:E7C6: a car's hit on lap 1, the first time (+$25 bit 0), starts a
   fade ($0DA5 bit 5, $80 up to $2000), leaving M 16-bit. */
static void lap1_fade_frame(Carried *cr)
{
    if (!hit(cr))
        { cyc(62); return; } else { cyc(36); }
    if (!lap_is_next(cr, 0x01))
        { cyc(34); return; } else { cyc(22); }
    if (bit25(cr, 0x01))
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(24);
    or8l(cr, W(slide_flags), 0x20, 0);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(22);
    fade_set(cr, 0x0080, 0x2000);
    cyc(44);
}

/*
 * @routine 09:E781 ObjectFadeGate1
 * WHAT IT DOES: Sets up the lap 1 fade gate: restart $09:E79F
 * (ObjectFadeGate1Record), frame $09:E7C6 (ObjectFadeGate1Step); its record
 * and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectFadeGate1(Carried *cr)
{
    handlers2(cr, 0xE79F, 0xE7C6);
    gate42d(cr, 0x0780);
    lap1_fade_frame(cr);
}

void abi_ObjectFadeGate1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFadeGate1(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E79F ObjectFadeGate1Record
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectFadeGate1. Exit: as ObjectFadeGate1.
 */
void ObjectFadeGate1Record(Carried *cr)
{
    gate42d(cr, 0x0780);
    lap1_fade_frame(cr);
}

void abi_ObjectFadeGate1Record(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFadeGate1Record(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E7C6 ObjectFadeGate1Step
 * WHAT IT DOES: The gate's frame: a car's hit on lap 1, the first time (+$25
 * bit 0), starts a fade ($0DA5 bit 5, step $80 up to $2000).
 * HOW: ObjectSyncFromRecord, RecordContactSummary; $0DA5 by long accesses.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it (after the fade with M 16-bit, A = $2000).
 */
void ObjectFadeGate1Step(Carried *cr)
{
    lap1_fade_frame(cr);
}

void abi_ObjectFadeGate1Step(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectFadeGate1Step(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:E852 / E8C4: with contacts, contact summary bit 0 (player 1) sets or
   clears bit 6 of $0DA5, bit 1 (player 2) bit 6 of $0DA7. */
static void window_gate_frame(Carried *cr, int on)
{
    if (!synced(cr))
        { cyc(72); return; } else { cyc(84); }
    RecordContactSummary_cr(cr);
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if ((uint8_t)cr->a) {
        cyc(28);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        if (on)
            or8(cr, db(cr) | W(slide_flags), 0x40);
        else
            and8(cr, db(cr) | W(slide_flags), 0xBF);
        cyc(62);
    } else { cyc(80); }
    RecordContactSummary_cr(cr);
    lda8(cr, (uint8_t)(cr->a & 0x02));
    if (!(uint8_t)cr->a)
        { cyc(62); return; }
    cyc(28);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (on)
        or8(cr, db(cr) | W(wram_0DA7), 0x40);
    else
        and8(cr, db(cr) | W(wram_0DA7), 0xBF);
    cyc(44);
}

/*
 * @routine 09:E80D ObjectWindowOnGate
 * WHAT IT DOES: Sets up the gate: restart $09:E82B (ObjectWindowOnGateRecord), frame
 * $09:E852 (ObjectWindowOnGateStep); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectWindowOnGate(Carried *cr)
{
    handlers2(cr, 0xE82B, 0xE852);
    gate42d(cr, 0x0780);
    window_gate_frame(cr, 1);
}

void abi_ObjectWindowOnGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWindowOnGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E82B ObjectWindowOnGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectWindowOnGate. Exit: as ObjectWindowOnGate.
 */
void ObjectWindowOnGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    window_gate_frame(cr, 1);
}

void abi_ObjectWindowOnGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWindowOnGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E852 ObjectWindowOnGateStep
 * WHAT IT DOES: The gate's frame: with contacts, a car (contact summary bit 0, player 1) sets bit 6 of $0DA5, one with
 * bit 1 (player 2) bit 6 of $0DA7.
 * HOW: ObjectSyncFromRecord, RecordContactSummary twice.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectWindowOnGateStep(Carried *cr)
{
    window_gate_frame(cr, 1);
}

void abi_ObjectWindowOnGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWindowOnGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E87F ObjectWindowOffGate
 * WHAT IT DOES: Sets up the gate: restart $09:E89D (ObjectWindowOffGateRecord), frame
 * $09:E8C4 (ObjectWindowOffGateStep); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectWindowOffGate(Carried *cr)
{
    handlers2(cr, 0xE89D, 0xE8C4);
    gate42d(cr, 0x0780);
    window_gate_frame(cr, 0);
}

void abi_ObjectWindowOffGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWindowOffGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E89D ObjectWindowOffGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectWindowOffGate. Exit: as ObjectWindowOffGate.
 */
void ObjectWindowOffGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    window_gate_frame(cr, 0);
}

void abi_ObjectWindowOffGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWindowOffGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E8C4 ObjectWindowOffGateStep
 * WHAT IT DOES: The gate's frame: with contacts, a car (contact summary bit 0, player 1) clears bit 6 of $0DA5, one with
 * bit 1 (player 2) bit 6 of $0DA7.
 * HOW: ObjectSyncFromRecord, RecordContactSummary twice.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectWindowOffGateStep(Carried *cr)
{
    window_gate_frame(cr, 0);
}

void abi_ObjectWindowOffGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWindowOffGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:E936 / E9AE: a car's hit, the first time (+$25 bit 0), sound `snd` on
   port 1; no hit clears the bit. */
static void beep_frame(Carried *cr, uint8_t snd)
{
    if (!hit(cr)) {
        cyc(18);
        and8(cr, F_c(0x2025, cr->x), 0xFE);
        { cyc(44); return; }
    } else { cyc(12); }
    if (bit25(cr, 0x01))
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    sound1(cr, snd);
    cyc(44);
}

/* 09:EA26 / EAA8: a car's hit sets the window slide bits ($0DA5: `or`
   after AND `and`); the first time (+$25 bit 0) sound `snd` on port 1; no
   hit clears the bit. */
static void slide_bits_frame(Carried *cr, uint8_t and_m, uint8_t or_m, uint8_t snd)
{
    uint8_t v;

    if (!hit(cr)) {
        cyc(18);
        and8(cr, F_c(0x2025, cr->x), 0xFE);
        cyc(44);
        return;
    } else { cyc(28); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (and_m == 0xFF) {
        or8(cr, db(cr) | W(slide_flags), or_m);
    } else {
        cyc(22);
        v = (uint8_t)(ld8(cr, db(cr) | W(slide_flags)) & and_m);
        lda8(cr, (uint8_t)(v | or_m));
        wr8(db(cr) | W(slide_flags), cyc_then(36, (uint8_t)(v | or_m)));
    }
    if (bit25(cr, 0x01))
        { cyc(88); return; } else { cyc(18); }
    or8(cr, F_c(0x2025, cr->x), 0x01);
    sound1(cr, snd);
    cyc(44);
}

/* LDA addr, AND #a, ORA #o, STA addr (8-bit). */
static void andor8(Carried *cr, uint32_t addr, uint8_t a, uint8_t o)
{
    cyc(16);
    uint8_t v = (uint8_t)(ld8(cr, addr) & a);

    lda8(cr, v);
    lda8(cr, (uint8_t)(v | o));
    cyc(16);
    wr8(addr, (uint8_t)(v | o));
}

/* 09:EB2C / EBAE: with contacts, contact summary bit 0 (player 1) sets
   $7F:3460 = t3460 and $0DA5 = (and a1, ora o1), bit 1 (player 2) sets
   $7F:3460 = t3460 (if store2) and $0DA7 = (and a2, ora o2). */
static void window_set_frame(Carried *cr, uint8_t t3460, uint8_t a1, uint8_t o1, uint8_t a2, uint8_t o2,
                             int store2)
{
    if (!synced(cr))
        { cyc(72); return; } else { cyc(22); }
    (cyc(62), RecordContactSummary_cr(cr));
    lda8(cr, (uint8_t)(cr->a & 0x01));
    if ((uint8_t)cr->a) {
        cyc(42);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        st8i(cr, W(wram_7F3460), t3460);
        andor8(cr, db(cr) | W(slide_flags), a1, o1);
        cyc(62);
    } else { cyc(80); }
    RecordContactSummary_cr(cr);
    lda8(cr, (uint8_t)(cr->a & 0x02));
    if (!(uint8_t)cr->a)
        { cyc(62); return; }
    if (store2) {
        cyc(42);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        st8i(cr, W(wram_7F3460), t3460);
    } else {
        cyc(34);                                /* SEP, no store */
        set_p(cr, (uint8_t)(cr->p | (P_M)));
    }
    andor8(cr, db(cr) | W(wram_0DA7), a2, o2);
    cyc(44);
}

/*
 * @routine 09:E8F1 ObjectBeepGate11
 * WHAT IT DOES: Sets up the gate: restart $09:E90F (ObjectBeepGate11Record), frame
 * $09:E936 (ObjectBeepGate11Step); its record and frame.
 * HOW: Word and byte stores. Reached only by
 * unit_objects_09e.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBeepGate11(Carried *cr)
{
    handlers2(cr, 0xE90F, 0xE936);
    gate42d(cr, 0x0780);
    beep_frame(cr, 0x11);
}

void abi_ObjectBeepGate11(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBeepGate11(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E90F ObjectBeepGate11Record
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only by
 * unit_objects_09e.
 * Entry: as ObjectBeepGate11. Exit: as ObjectBeepGate11.
 */
void ObjectBeepGate11Record(Carried *cr)
{
    gate42d(cr, 0x0780);
    beep_frame(cr, 0x11);
}

void abi_ObjectBeepGate11Record(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBeepGate11Record(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E936 ObjectBeepGate11Step
 * WHAT IT DOES: The gate's frame: a car's hit, the first time (+$25 bit 0), plays sound $11 on port 1; no
 * hit clears the bit.
 * HOW: ObjectSyncFromRecord, RecordContactSummary. Reached only by
 * unit_objects_09e.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectBeepGate11Step(Carried *cr)
{
    beep_frame(cr, 0x11);
}

void abi_ObjectBeepGate11Step(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBeepGate11Step(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E969 ObjectBeepGate10
 * WHAT IT DOES: Sets up the gate: restart $09:E987 (ObjectBeepGate10Record), frame
 * $09:E9AE (ObjectBeepGate10Step); its record and frame.
 * HOW: Word and byte stores. Reached only by
 * unit_objects_09e.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBeepGate10(Carried *cr)
{
    handlers2(cr, 0xE987, 0xE9AE);
    gate42d(cr, 0x0780);
    beep_frame(cr, 0x10);
}

void abi_ObjectBeepGate10(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBeepGate10(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E987 ObjectBeepGate10Record
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only by
 * unit_objects_09e.
 * Entry: as ObjectBeepGate10. Exit: as ObjectBeepGate10.
 */
void ObjectBeepGate10Record(Carried *cr)
{
    gate42d(cr, 0x0780);
    beep_frame(cr, 0x10);
}

void abi_ObjectBeepGate10Record(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBeepGate10Record(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E9AE ObjectBeepGate10Step
 * WHAT IT DOES: The gate's frame: a car's hit, the first time (+$25 bit 0), plays sound $10 on port 1; no
 * hit clears the bit.
 * HOW: ObjectSyncFromRecord, RecordContactSummary. Reached only by
 * unit_objects_09e.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectBeepGate10Step(Carried *cr)
{
    beep_frame(cr, 0x10);
}

void abi_ObjectBeepGate10Step(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBeepGate10Step(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E9E1 ObjectSlideBitsGate
 * WHAT IT DOES: Sets up the gate: restart $09:E9FF (ObjectSlideBitsGateRecord), frame
 * $09:EA26 (ObjectSlideBitsGateStep); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectSlideBitsGate(Carried *cr)
{
    handlers2(cr, 0xE9FF, 0xEA26);
    gate42d(cr, 0x0780);
    slide_bits_frame(cr, 0xFF, 0x0A, 0x0F);
}

void abi_ObjectSlideBitsGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideBitsGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:E9FF ObjectSlideBitsGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectSlideBitsGate. Exit: as ObjectSlideBitsGate.
 */
void ObjectSlideBitsGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    slide_bits_frame(cr, 0xFF, 0x0A, 0x0F);
}

void abi_ObjectSlideBitsGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideBitsGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:EA26 ObjectSlideBitsGateStep
 * WHAT IT DOES: The gate's frame: a car's hit sets bits 1 and 3 of $0DA5; the first time (+$25 bit 0)
 * sound $0F on port 1; no hit clears the bit.
 * HOW: ObjectSyncFromRecord, RecordContactSummary.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectSlideBitsGateStep(Carried *cr)
{
    slide_bits_frame(cr, 0xFF, 0x0A, 0x0F);
}

void abi_ObjectSlideBitsGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideBitsGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:EA63 ObjectSlideBitGate
 * WHAT IT DOES: Sets up the gate: restart $09:EA81 (ObjectSlideBitGateRecord), frame
 * $09:EAA8 (ObjectSlideBitGateStep); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectSlideBitGate(Carried *cr)
{
    handlers2(cr, 0xEA81, 0xEAA8);
    gate42d(cr, 0x0780);
    slide_bits_frame(cr, 0xF7, 0x04, 0x10);
}

void abi_ObjectSlideBitGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideBitGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:EA81 ObjectSlideBitGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectSlideBitGate. Exit: as ObjectSlideBitGate.
 */
void ObjectSlideBitGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    slide_bits_frame(cr, 0xF7, 0x04, 0x10);
}

void abi_ObjectSlideBitGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideBitGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:EAA8 ObjectSlideBitGateStep
 * WHAT IT DOES: The gate's frame: a car's hit sets bit 2 of $0DA5 (clearing bit 3); the first time (+$25
 * bit 0) sound $10 on port 1; no hit clears the bit.
 * HOW: ObjectSyncFromRecord, RecordContactSummary.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectSlideBitGateStep(Carried *cr)
{
    slide_bits_frame(cr, 0xF7, 0x04, 0x10);
}

void abi_ObjectSlideBitGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideBitGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:EAE7 ObjectWindowSetGate
 * WHAT IT DOES: Sets up the gate: restart $09:EB05 (ObjectWindowSetGateRecord), frame
 * $09:EB2C (ObjectWindowSetGateStep); its record and frame.
 * HOW: Word and byte stores. Reached only by
 * unit_objects_09e.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectWindowSetGate(Carried *cr)
{
    handlers2(cr, 0xEB05, 0xEB2C);
    gate42d(cr, 0x0780);
    window_set_frame(cr, 0x1F, 0x00, 0x44, 0x00, 0xC0, 1);
}

void abi_ObjectWindowSetGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWindowSetGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:EB05 ObjectWindowSetGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only by
 * unit_objects_09e.
 * Entry: as ObjectWindowSetGate. Exit: as ObjectWindowSetGate.
 */
void ObjectWindowSetGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    window_set_frame(cr, 0x1F, 0x00, 0x44, 0x00, 0xC0, 1);
}

void abi_ObjectWindowSetGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWindowSetGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:EB2C ObjectWindowSetGateStep
 * WHAT IT DOES: The gate's frame: with contacts, a car (contact summary bit 0, player 1) sets $7F:3460 =
 * $1F and $0DA5 = $44, one with bit 1 (player 2) $7F:3460 = $1F and $0DA7
 * = $C0.
 * HOW: ObjectSyncFromRecord, RecordContactSummary twice; AND #0 then ORA. Reached only by
 * unit_objects_09e.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectWindowSetGateStep(Carried *cr)
{
    window_set_frame(cr, 0x1F, 0x00, 0x44, 0x00, 0xC0, 1);
}

void abi_ObjectWindowSetGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWindowSetGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:EB69 ObjectWindowResetGate
 * WHAT IT DOES: Sets up the gate: restart $09:EB87 (ObjectWindowResetGateRecord), frame
 * $09:EBAE (ObjectWindowResetGateStep); its record and frame.
 * HOW: Word and byte stores. Reached only by
 * unit_objects_09e.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectWindowResetGate(Carried *cr)
{
    handlers2(cr, 0xEB87, 0xEBAE);
    gate42d(cr, 0x0780);
    window_set_frame(cr, 0x00, 0x00, 0x0A, 0xBF, 0x08, 0);
}

void abi_ObjectWindowResetGate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWindowResetGate(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:EB87 ObjectWindowResetGateRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition. Reached only by
 * unit_objects_09e.
 * Entry: as ObjectWindowResetGate. Exit: as ObjectWindowResetGate.
 */
void ObjectWindowResetGateRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    window_set_frame(cr, 0x00, 0x00, 0x0A, 0xBF, 0x08, 0);
}

void abi_ObjectWindowResetGateRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWindowResetGateRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:EBAE ObjectWindowResetGateStep
 * WHAT IT DOES: The gate's frame: with contacts, a car (contact summary bit 0, player 1) sets $7F:3460 =
 * 0 and $0DA5 = $0A, one with bit 1 (player 2) $7F:3460 = 0 and bit 3 of
 * $0DA7 (bit 6 cleared).
 * HOW: ObjectSyncFromRecord, RecordContactSummary twice; AND then ORA. Reached only by
 * unit_objects_09e.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectWindowResetGateStep(Carried *cr)
{
    window_set_frame(cr, 0x00, 0x00, 0x0A, 0xBF, 0x08, 0);
}

void abi_ObjectWindowResetGateStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectWindowResetGateStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* BRK in bank $09 with the next instruction at `next`: PB, PC and P
   pushed, I set and D clear, and the vector's handler (InterruptReturn,
   00:FE90) at once returns by RTI: P, PC and PB pulled back. */
static void brk_09(Carried *cr, uint16_t next)
{
    uint8_t p = cr->p;

    wr8((uint16_t)cpu.s, 0x09);
    wr8((uint16_t)(cpu.s - 1), (uint8_t)(next >> 8));
    wr8((uint16_t)(cpu.s - 2), (uint8_t)next);
    wr8((uint16_t)(cpu.s - 3), p);
    cpu.s = (uint16_t)(cpu.s - 4);
    set_p(cr, (uint8_t)((cr->p | P_I) & ~P_D));
    r_plp(cr, rd8((uint16_t)(cpu.s + 1)));
    rd8((uint16_t)(cpu.s + 2));
    rd8((uint16_t)(cpu.s + 3));
    rd8((uint16_t)(cpu.s + 4));
    cpu.s = (uint16_t)(cpu.s + 4);
}

void RasterIrq(Carried *cr);

/* 09:EC71 reached with M 16-bit (a hit while the slide is held at $7FFF):
   the in mark's byte code runs as other instructions. LDA +$25 (a word),
   AND #$F001, TSB $5C (a word), STY $EC,X (a word), ORA #$25BD, then JSR
   $0109: in bank $09 that is work RAM, the interrupt stubs (copied there
   at the start: three NOPs, JSL RasterIrq, RTI). The raster interrupt's
   handler runs as a subroutine and returns; the RTI then pulls P (the low
   byte of the JSR's return, $80), PC and PB from the stack, two of them
   from this routine's caller's frame: the CPU leaves the routine there
   (cpu_leave_to: pc where it lands). From ObjectRunHandlerA's dispatch
   that is $CA:E7EC, ROM data, not code: the original game runs off its
   program on this path. Not reached in the traces so far. */
static void slide_gate_held_m16(Carried *cr)
{
    uint16_t w, m, lo, hi;
    uint8_t bank;

    r_lda16(cr, F_c(0x2025, cr->x));
    r_and16(cr, 0xF001);
    m = rd16((uint16_t)(cr->d + 0x5C));
    cr->p = (uint8_t)((cr->p & ~P_Z) | ((cr->a & m) ? 0 : P_Z));
    wr16((uint16_t)(cr->d + 0x5C), (uint16_t)(m | cr->a));
    wr16((uint16_t)(cr->d + 0xEC + cr->x), cr->y);
    cr->a = (uint16_t)(cr->a | 0x25BD);
    cr->p = nz16(cr->p, cr->a);
    /* JSR $0109: the return $09:EC80 pushed */
    wr8((uint16_t)cpu.s, 0xEC);
    wr8((uint16_t)(cpu.s - 1), 0x80);
    cpu.s = (uint16_t)(cpu.s - 2);
    /* $010C: JSL $7E:A2D9, its return $09:010F pushed and pulled */
    wr8((uint16_t)cpu.s, 0x09);
    wr8((uint16_t)(cpu.s - 1), 0x01);
    wr8((uint16_t)(cpu.s - 2), 0x0F);
    cpu.s = (uint16_t)(cpu.s - 3);
    RasterIrq(cr);
    rd8((uint16_t)(cpu.s + 1));
    rd8((uint16_t)(cpu.s + 2));
    rd8((uint16_t)(cpu.s + 3));
    cpu.s = (uint16_t)(cpu.s + 3);
    /* $0110: RTI */
    r_plp(cr, rd8((uint16_t)(cpu.s + 1)));
    lo = rd8((uint16_t)(cpu.s + 2));
    hi = rd8((uint16_t)(cpu.s + 3));
    bank = rd8((uint16_t)(cpu.s + 4));
    cpu.s = (uint16_t)(cpu.s + 4);
    w = (uint16_t)(lo | hi << 8);
    cpu_leave_to((uint32_t)bank << 16 | w);
}

/* 09:ECB9 reached with M 8-bit (no hit while the screen flashes): the out
   mark's word code runs as other instructions. LDA +$25 and AND #$02 take
   one byte each, so the $00 that follows is a BRK (and the next byte its
   signature); then TSB $5C, CMP $BD09EC,X, AND $20, ORA #$02, a second
   BRK, AND $20, LDA +$25, AND #$FE, SBC $20259D,X (long, X), and on as
   written: SEP #$20, sound $10, the RTL. */
static void slide_gate_out_m8(Carried *cr)
{
    uint8_t a, m;

    a = ld8(cr, F_c(0x2025, cr->x));
    lda8(cr, (uint8_t)(a & 0x02));
    brk_09(cr, 0xECC0);
    cyc(86);
    m = rd8((uint16_t)(cr->d + 0x5C));
    set_p(cr, (uint8_t)((cr->p & ~P_Z) | (((uint8_t)cr->a & m) ? 0 : P_Z)));
    cyc(6);
    wr8((uint16_t)(cr->d + 0x5C), (uint8_t)(m | (uint8_t)cr->a));
    cyc(32);
    m = rd8((0xBD09ECu + cr->x) & 0xFFFFFF);
    a = (uint8_t)cr->a;
    set_p(cr, (uint8_t)((nz8(cr->p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? P_C : 0)));
    cyc(16);
    lda8(cr, (uint8_t)(a & rd8((uint16_t)(cr->d + 0x20))));
    lda8(cr, (uint8_t)((uint8_t)cr->a | 0x02));
    brk_09(cr, 0xECCC);
    cyc(66);
    lda8(cr, cyc_then(2, (uint8_t)((uint8_t)cr->a & rd8((uint16_t)(cr->d + 0x20)))));
    a = ld8(cr, F_c(0x2025, cr->x));
    lda8(cr, (uint8_t)(a & 0xFE));
    cyc(28);
    a = alu_sbc8((uint8_t)cr->a, rd8((0x20259Du + cr->x) & 0xFFFFFF), &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(6);
    sound1(cr, 0x10);
}

/* 09:EC2A: the slide gate's frame. A car's hit, unless the screen is
   flashing ($0DA5 bit 0) or the slide is held ($7F:3470 = $7FFF), starts
   the slide up ($7F:3470 = $800, step $100 to $700, direction 0, $0DA5 bit
   7); the first time (+$25 bit 0, clearing bit 1) sound $11 on port 1. No
   hit, unless held, sends the slide back down (to -$100, direction $8000);
   the first time (+$25 bit 1, clearing bit 0, as words) sound $10.
   Two paths reach code with the other register width than the code was
   written for: a hit while the slide is held comes to 09:EC71 with M
   16-bit, and no hit while flashing comes to 09:ECB9 with M 8-bit; the
   original runs their operand bytes as other instructions
   (slide_gate_held_m16, slide_gate_out_m8). */
static void slide_gate_frame(Carried *cr)
{
    uint16_t v;

    if (hit(cr)) {
        cyc(28);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        lda8(cr, (uint8_t)(ld8(cr, db(cr) | W(slide_flags)) & 0x01));
        if (!(uint8_t)cr->a) {
            set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
            cyc(66);
            r_lda16(cr, W(slide_done));
            r_cmp16(cr, 0x7FFF);
            if (cr->a == 0x7FFF) {
                slide_gate_held_m16(cr);
                return;
            } else { cyc(66); }
            cyc(4);
            st16w(cr, W(slide_done), 0x0800);
            slide_set(cr, 0x0100, 0x0700, 0x0000);
            set_p(cr, (uint8_t)(cr->p | (P_M)));
            or8(cr, (cyc(16), db(cr)) | W(slide_flags), 0x80);
        } else { cyc(18); }
        if (bit25(cr, 0x01))
            { cyc(44); return; } else { cyc(18); }
        or8(cr, F_c(0x2025, cr->x), 0x01);
        and8(cr, F_c(0x2025, cr->x), 0xFD);
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(22);
        sound1(cr, 0x11);
        return;
    } else { cyc(34); }
    lda8(cr, (uint8_t)(ld8(cr, db(cr) | W(slide_flags)) & 0x01));
    if ((uint8_t)cr->a) {
        cyc(40);
        slide_gate_out_m8(cr);
        return;
    }
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(66);
    r_lda16(cr, W(slide_done));
    r_cmp16(cr, 0x7FFF);
    if (cr->a != 0x7FFF)
        { cyc(40); slide_set(cr, 0x0100, 0xFF00, 0x8000); } else { cyc(46); }
    r_lda16(cr, cyc_then(30, F_c(0x2025, cr->x)));
    r_and16(cr, 0x0002);
    if (cr->a)
        { cyc(70); return; }
    v = (uint16_t)(rd16(cyc_then(74, F_c(0x2025, cr->x))) | 0x0002);
    cr->a = v;
    set_p(cr, nz16(cr->p, v));
    wr16(F_c(0x2025, cr->x), cyc_then(54, v));
    v = (uint16_t)(rd16(cyc_then(30, F_c(0x2025, cr->x))) & 0xFFFE);
    cr->a = v;
    set_p(cr, nz16(cr->p, v));
    wr16(F_c(0x2025, cr->x), cyc_then(54, v));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(22);
    sound1(cr, 0x10);
}

/*
 * @routine 09:EBE5 ObjectSlideGateUp
 * WHAT IT DOES: Sets up the slide gate: restart $09:EC03
 * (ObjectSlideGateUpRecord), frame $09:EC2A (ObjectSlideGateUpStep); its
 * record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectSlideGateUp(Carried *cr)
{
    handlers2(cr, 0xEC03, 0xEC2A);
    gate42d(cr, 0x0780);
    slide_gate_frame(cr);
    cyc(44);
}

void abi_ObjectSlideGateUp(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideGateUp(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:EC03 ObjectSlideGateUpRecord
 * WHAT IT DOES: The gate's restart: its record ($42D, flag bit 4, at
 * ($780, $780, $780)), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordPosition.
 * Entry: as ObjectSlideGateUp. Exit: as ObjectSlideGateUp.
 */
void ObjectSlideGateUpRecord(Carried *cr)
{
    gate42d(cr, 0x0780);
    slide_gate_frame(cr);
    cyc(44);
}

void abi_ObjectSlideGateUpRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideGateUpRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:EC2A ObjectSlideGateUpStep
 * WHAT IT DOES: The slide gate's frame: a car's hit starts the slide up
 * (unless the screen flashes or the slide is held at $7FFF) and plays
 * sound $11 the first time; no hit sends it back down (unless held) and
 * plays sound $10 the first time; +$25 bits 0 and 1 mark in and out.
 * HOW: ObjectSyncFromRecord, RecordContactSummary; the out mark's +$25
 * accesses are words (M 16-bit). A hit while held and no hit while
 * flashing reach code with the other register width (09:EC71, 09:ECB9):
 * the operand bytes run as other instructions.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectSlideGateUpStep(Carried *cr)
{
    slide_gate_frame(cr);
    cyc(44);
}

void abi_ObjectSlideGateUpStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSlideGateUpStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ---- 09:B30B, 09:C11F-C437: the shatter, bouncing markers, lap lights ---- */

void abi_ObjectCopy14To(void);
void abi_ObjectSpawnPuff(void);

/* The colour +$5A stepped through 0 to n-1 with bit 7 (SBC #n at n). */
static void colour_cycle(Carried *cr, uint8_t n)
{
    uint8_t v;

    cyc(22);
    if (colour_step(cr, n)) {
        cyc(8);
        set_p(cr, (uint8_t)(cr->p | (P_C)));
        v = alu_sbc8((uint8_t)cr->a, n, &cr->p);
        cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    }
    v = (uint8_t)(cr->a | 0x80);
    lda8(cr, v);
    wr8(F_c(0x205A, cr->x), cyc_then(112, v));
}

/* LDA src, STA lo, STA lo2, BIT #$80: the sign byte `hi` = $FF for a
   negative value, else 0. */
static void axis_from(Carried *cr, uint32_t src, uint32_t lo, uint32_t lo2, uint32_t hi)
{
    uint32_t w2 = (lo2 & 0xFFFF) > 0xFF ? 8 : 0, wh = (hi & 0xFFFF) > 0xFF ? 8 : 0;

    cyc((src & 0xFFFF) < 0x100 ? 0 : 4);        /* LDA dp or abs */
    uint8_t r = ld8(cr, src);

    cyc(16);
    wr8(lo, r);
    cyc(16 + w2);
    wr8(lo2, r);
    if (r & 0x80) {
        lda8(cr, 0xFF);
        cyc(44 + wh);
        wr8(hi, 0xFF);
        cyc(22);                        /* BRA */
    } else {
        cyc(54 + wh);
        wr8(hi, 0x00);
    }
}

/* One shard: a new object of `kind` with spin bytes +$48-$4A, at X's
   position and size (+$14-$16), placed at 4 times the vector (vx, vy, dp
   $00) turned by X's angles +$16, +$14 and +$15 (RotateXY,
   RotateXZFrom08, RotateXZ; vx and vy through $1F58/$1F59), life +$0A =
   6, handler $09:BDE1; nothing without a free object. */

static void shard(Carried *cr, uint16_t kind, uint8_t s48, uint8_t s49, uint8_t s4a, uint8_t vx, uint8_t vy)
{
    if (!spawn(cr, kind))
        { cyc(26); return; } else { cyc(6); }
    st8i(cr, F_c(0x2048, cr->y), s48);
    st8i(cr, F_c(0x2049, cr->y), cyc_then(6, s49));
    st8i(cr, F_c(0x204A, cr->y), cyc_then(6, s4a));
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    (cyc(62), ObjectCopy14To_cr(cr));
    st8i(cr, (cyc(0), db(cr)) | W(wram_1F58), vx);
    st8i(cr, db(cr) | W(wram_1F59), vy);
    axis_from(cr, db(cr) | W(wram_1F58), dp(cr, 0x02), dp(cr, 0x04), dp(cr, 0x05));
    axis_from(cr, db(cr) | W(wram_1F59), dp(cr, 0x08), dp(cr, 0x0A), dp(cr, 0x0B));
    shard_from_z(cr);
}

/* A shard from its third component on: direct page $00's byte and sign,
   the three turns, times 4, the placement at X's position, life 6 and the
   handler. */
static void shard_from_z(Carried *cr)
{
    int i;

    axis_from(cr, dp(cr, 0x00), dp(cr, 0x94), db(cr) | W(wram_05F2), db(cr) | W(rot_z_high));
    ld8(cr, cyc_then(22, F_c(0x2016, cr->x)));
    (cyc(62), RotateXY_cr(cr));
    orbit_again(cr);
    ld8(cr, cyc_then(22, F_c(0x2014, cr->x)));
    (cyc(62), RotateXZFrom08_cr(cr));
    orbit_again(cr);
    ld8(cr, cyc_then(22, F_c(0x2015, cr->x)));
    (cyc(62), RotateXZ_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    {
        cyc(22);
        static const uint8_t lo[2] = {0x04, 0x0A};

        for (i = 0; i < 2; i++) {
            r_lda16(cr, cyc_then(12, dp(cr, lo[i])));
            r_asl16(cr);
            r_asl16(cr);
            wr16(dp(cr, lo[i]), cyc_then(40, cr->a));
        }
    }
    r_lda16(cr, cyc_then(24, db(cr) | W(wram_05F2)));
    r_asl16(cr);
    r_asl16(cr);
    wr16(db(cr) | W(wram_05F2), cyc_then(52, cr->a));
    r_lda16(cr, cyc_then(30, F_c(0x200C, cr->x)));
    r_add16(cr, rd16(cyc_then(26, dp(cr, 0x04))));
    wr16(F_c(0x200C, cr->y), cyc_then(30, cr->a));
    r_lda16(cr, cyc_then(30, F_c(0x2010, cr->x)));
    r_add16(cr, rd16(cyc_then(38, db(cr) | W(wram_05F2))));
    wr16(F_c(0x2010, cr->y), cyc_then(30, cr->a));
    r_lda16(cr, cyc_then(30, F_c(0x200E, cr->x)));
    r_add16(cr, rd16(cyc_then(26, dp(cr, 0x0A))));
    wr16(F_c(0x200E, cr->y), cyc_then(30, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    st8i(cr, F_c(0x200A, cr->y), cyc_then(28, 0x06));
    cyc(22);
    handler(cr, cr->y, 0xBDE1);
}

/* The six shards' spin bytes +$48-$4A and offsets (x, y), and each
   shatter's kinds. */
static const uint8_t shatter_args[6][5] = {
    {0xEE, 0xF3, 0xF0, 0xF0, 0xBA},
    {0xEE, 0x0D, 0xF0, 0xE5, 0xD8},
    {0x00, 0x0C, 0xFC, 0x00, 0xEE},
    {0x12, 0x0D, 0x10, 0x1B, 0xD8},
    {0x12, 0xF3, 0x10, 0x10, 0xBA},
    {0x00, 0x0C, 0x04, 0x00, 0x0F},
};
static const uint16_t shatter1_kinds[6] = {0xB08C, 0xB0A8, 0xB0C4, 0xB0E0, 0xB0FC, 0xB118};
static const uint16_t shatter2_kinds[6] = {0xB134, 0xB150, 0xB16C, 0xB188, 0xB1A4, 0xB1C0};

/* The shards from index from to the last. */
static void shatter_rest(Carried *cr, const uint16_t *kinds, int from)
{
    int k;

    for (k = from; k < 6; k++)
        shard(cr, kinds[k], shatter_args[k][0], shatter_args[k][1], shatter_args[k][2],
              shatter_args[k][3], shatter_args[k][4]);
    cyc(42);
}

/* Shard k entered at its x sign store (direct page $05): the y offset, the
   rest of the shard, the shards after it. */
static void shatter_from_x(Carried *cr, const uint16_t *kinds, int k, int neg)
{
    sign_arm(cr, dp(cr, 0x05), neg);
    axis_from(cr, db(cr) | W(wram_1F59), dp(cr, 0x08), dp(cr, 0x0A), dp(cr, 0x0B));
    shard_from_z(cr);
    shatter_rest(cr, kinds, k + 1);
}

/* Shard k entered at its y sign store (direct page $0B). */
static void shatter_from_y(Carried *cr, const uint16_t *kinds, int k, int neg)
{
    sign_arm(cr, dp(cr, 0x0B), neg);
    shard_from_z(cr);
    shatter_rest(cr, kinds, k + 1);
}

/*
 * @routine 09:B30B ObjectShatter
 * WHAT IT DOES: Shatters object X into six shards (kinds $B08C, $B0A8,
 * $B0C4, $B0E0, $B0FC, $B118, handler $09:BDE1), each with its spin bytes
 * +$48-$4A, at object X's position plus its own offset ((-$10, -$46),
 * (-$1B, -$28), (0, -$12), ($1B, -$28), ($10, -$46), (0, $0F), the third
 * component direct page $00) turned by X's angles and times 4, life 6; a
 * shard is skipped without a free object.
 * HOW: Six ObjectSpawn blocks; ObjectCopyPositionTo, ObjectCopy14To;
 * RotateXY, RotateXZFrom08, RotateXZ through direct page. The offsets are
 * constants (stored and read back through $1F58/$1F59), so the sign
 * stores not taken for them (09:B351, B364, B438, B44B, ...) are only
 * reached by direct calls, as ObjectShatterShard1XSign and the rest.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last
 * shard leaves it.
 */
void ObjectShatter(Carried *cr)
{
    shatter_rest(cr, shatter1_kinds, 0);
}

void abi_ObjectShatter(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatter(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B351 ObjectShatterShard1XSign
 * WHAT IT DOES: Shard 1 of ObjectShatter from its x sign store on: direct
 * page $05 = 0 (as for a positive x offset), then the rest of that shard
 * (the y and third components' bytes, the turns, the placement, life and
 * handler) and the shards after it.
 * HOW: The x offset of shard 1 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter.
 */
void ObjectShatterShard1XSign(Carried *cr)
{
    shatter_from_x(cr, shatter1_kinds, 0, 0);
}

void abi_ObjectShatterShard1XSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatterShard1XSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B364 ObjectShatterShard1YSign
 * WHAT IT DOES: Shard 1 of ObjectShatter from its y sign store on: direct
 * page $0B = 0 (as for a positive y offset), then the rest of that shard
 * (the third component's byte, the turns, the placement, life and handler)
 * and the shards after it.
 * HOW: The y offset of shard 1 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter.
 */
void ObjectShatterShard1YSign(Carried *cr)
{
    shatter_from_y(cr, shatter1_kinds, 0, 0);
}

void abi_ObjectShatterShard1YSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatterShard1YSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B438 ObjectShatterShard2XSign
 * WHAT IT DOES: Shard 2 of ObjectShatter from its x sign store on: direct
 * page $05 = 0 (as for a positive x offset), then the rest of that shard
 * (the y and third components' bytes, the turns, the placement, life and
 * handler) and the shards after it.
 * HOW: The x offset of shard 2 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter.
 */
void ObjectShatterShard2XSign(Carried *cr)
{
    shatter_from_x(cr, shatter1_kinds, 1, 0);
}

void abi_ObjectShatterShard2XSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatterShard2XSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B44B ObjectShatterShard2YSign
 * WHAT IT DOES: Shard 2 of ObjectShatter from its y sign store on: direct
 * page $0B = 0 (as for a positive y offset), then the rest of that shard
 * (the third component's byte, the turns, the placement, life and handler)
 * and the shards after it.
 * HOW: The y offset of shard 2 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter.
 */
void ObjectShatterShard2YSign(Carried *cr)
{
    shatter_from_y(cr, shatter1_kinds, 1, 0);
}

void abi_ObjectShatterShard2YSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatterShard2YSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B519 ObjectShatterShard3XSign
 * WHAT IT DOES: Shard 3 of ObjectShatter from its x sign store on: direct
 * page $05 = $FF (as for a negative x offset), then the rest of that shard
 * (the y and third components' bytes, the turns, the placement, life and
 * handler) and the shards after it.
 * HOW: The x offset of shard 3 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter.
 */
void ObjectShatterShard3XSign(Carried *cr)
{
    shatter_from_x(cr, shatter1_kinds, 2, 1);
}

void abi_ObjectShatterShard3XSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatterShard3XSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B532 ObjectShatterShard3YSign
 * WHAT IT DOES: Shard 3 of ObjectShatter from its y sign store on: direct
 * page $0B = 0 (as for a positive y offset), then the rest of that shard
 * (the third component's byte, the turns, the placement, life and handler)
 * and the shards after it.
 * HOW: The y offset of shard 3 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter.
 */
void ObjectShatterShard3YSign(Carried *cr)
{
    shatter_from_y(cr, shatter1_kinds, 2, 0);
}

void abi_ObjectShatterShard3YSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatterShard3YSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B600 ObjectShatterShard4XSign
 * WHAT IT DOES: Shard 4 of ObjectShatter from its x sign store on: direct
 * page $05 = $FF (as for a negative x offset), then the rest of that shard
 * (the y and third components' bytes, the turns, the placement, life and
 * handler) and the shards after it.
 * HOW: The x offset of shard 4 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter.
 */
void ObjectShatterShard4XSign(Carried *cr)
{
    shatter_from_x(cr, shatter1_kinds, 3, 1);
}

void abi_ObjectShatterShard4XSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatterShard4XSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B619 ObjectShatterShard4YSign
 * WHAT IT DOES: Shard 4 of ObjectShatter from its y sign store on: direct
 * page $0B = 0 (as for a positive y offset), then the rest of that shard
 * (the third component's byte, the turns, the placement, life and handler)
 * and the shards after it.
 * HOW: The y offset of shard 4 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter.
 */
void ObjectShatterShard4YSign(Carried *cr)
{
    shatter_from_y(cr, shatter1_kinds, 3, 0);
}

void abi_ObjectShatterShard4YSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatterShard4YSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B6E7 ObjectShatterShard5XSign
 * WHAT IT DOES: Shard 5 of ObjectShatter from its x sign store on: direct
 * page $05 = $FF (as for a negative x offset), then the rest of that shard
 * (the y and third components' bytes, the turns, the placement, life and
 * handler) and the shards after it.
 * HOW: The x offset of shard 5 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter.
 */
void ObjectShatterShard5XSign(Carried *cr)
{
    shatter_from_x(cr, shatter1_kinds, 4, 1);
}

void abi_ObjectShatterShard5XSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatterShard5XSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B700 ObjectShatterShard5YSign
 * WHAT IT DOES: Shard 5 of ObjectShatter from its y sign store on: direct
 * page $0B = 0 (as for a positive y offset), then the rest of that shard
 * (the third component's byte, the turns, the placement, life and handler)
 * and the shards after it.
 * HOW: The y offset of shard 5 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter.
 */
void ObjectShatterShard5YSign(Carried *cr)
{
    shatter_from_y(cr, shatter1_kinds, 4, 0);
}

void abi_ObjectShatterShard5YSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatterShard5YSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B7CE ObjectShatterShard6XSign
 * WHAT IT DOES: Shard 6 of ObjectShatter from its x sign store on: direct
 * page $05 = $FF (as for a negative x offset), then the rest of that shard
 * (the y and third components' bytes, the turns, the placement, life and
 * handler) and the shards after it.
 * HOW: The x offset of shard 6 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter.
 */
void ObjectShatterShard6XSign(Carried *cr)
{
    shatter_from_x(cr, shatter1_kinds, 5, 1);
}

void abi_ObjectShatterShard6XSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatterShard6XSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B7E1 ObjectShatterShard6YSign
 * WHAT IT DOES: Shard 6 of ObjectShatter from its y sign store on: direct
 * page $0B = $FF (as for a negative y offset), then the rest of that shard
 * (the third component's byte, the turns, the placement, life and handler)
 * and the shards after it.
 * HOW: The y offset of shard 6 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter.
 */
void ObjectShatterShard6YSign(Carried *cr)
{
    shatter_from_y(cr, shatter1_kinds, 5, 1);
}

void abi_ObjectShatterShard6YSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatterShard6YSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/* The bounce: +$29 added to +$27, turning back at +-lim; field `pos`
   moved by +$27 (through $1F4C); the colour steps through 0-9; the scale
   +$15 grows by 8. */
static void bounce_colour(Carried *cr, uint16_t lim, uint16_t pos)
{
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(52, F_c(0x2027, cr->x)));
    r_add16(cr, rd16(cyc_then(44, F_c(0x2029, cr->x))));
    wr16(F_c(0x2027, cr->x), cyc_then(30, cr->a));
    r_cmp16(cr, lim);
    if (cr->a == lim || (cyc(34), (r_cmp16(cr, (uint16_t)-lim), cr->a == (uint16_t)-lim))) {
        r_lda16(cr, cyc_then(76, F_c(0x2029, cr->x)));
        cr->a ^= 0xFFFF;
        set_p(cr, nz16(cr->p, cr->a));
        cr->a++;
        set_p(cr, nz16(cr->p, cr->a));
        wr16(F_c(0x2029, cr->x), cyc_then(68, cr->a));
    } else { cyc(52); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    add16_via(cr, F_c(pos, cr->x), F_c(0x2027, cr->x));
    colour_cycle(cr, 0x0A);
    add8(cr, F_c(0x2015, cr->x), 0x08);
    cyc(42);
}

/*
 * @routine 09:C11F MarkerBounceY
 * WHAT IT DOES: Bounces object X up and down: the speed +$29 is added to
 * the offset +$27, turning round at $0E and -$0E, and the height +$0E moves
 * by the offset; its colour +$5A steps through 0-9 and its scale +$15
 * grows by 8.
 * HOW: 16-bit ADC/CMP/EOR+INC; the offset added through $1F4C.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = the
 * new scale; N, Z, C, V from its add.
 */
void MarkerBounceY(Carried *cr)
{
    bounce_colour(cr, 0x000E, 0x200E);
}

void abi_MarkerBounceY(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MarkerBounceY(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C17C MarkerBounceX
 * WHAT IT DOES: As MarkerBounceY, sideways: turning at $30 and -$30, the
 * offset moving +$0C.
 * HOW: As MarkerBounceY. Nothing in the game reaches it; reached only by
 * unit_objects_09f.
 * Entry: as MarkerBounceY. Exit: as MarkerBounceY.
 */
void MarkerBounceX(Carried *cr)
{
    bounce_colour(cr, 0x0030, 0x200C);
}

void abi_MarkerBounceX(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MarkerBounceX(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:C225: the marker bounces (MarkerBounceY), syncs to its record; a car's
   contact (flags bit 0) shatters it (ObjectShatter) and releases it. */
static void shatter_marker_frame(Carried *cr)
{
    (cyc(46), MarkerBounceY(cr));
    (cyc(62), ObjectSyncToRecord_cr(cr));
    cyc(10);
    cr->a = (uint16_t)((cr->a & 0xFF00) | (cr->a & 0x01));
    set_p(cr, nz8(cr->p, (uint8_t)cr->a));
    if (!(uint8_t)cr->a)
        { cyc(72); return; }
    if (!next_contact(cr))
        { cyc(72); return; } else { cyc(54); }
    {
        uint16_t sx = cr->x;

        ObjectShatter(cr);
        cr->x = sx;
        set_p(cr, nz16(cr->p, sx));
    }
    (cyc(98), ObjectRelease_cr(cr));
    cyc(44);
}

/* 09:C217: the marker's record ($42D, flag bit 4, +$2A high 1). */
static void shatter_marker_record(Carried *cr)
{
    cyc(62);
    GsuRecordNew42D_cr(cr);
    GsuRecordFlag10_cr(cr);
    cyc(58);
    lda8(cr, 0x01);
    GsuRecordSet2AHigh_cr(cr);
}

/*
 * @routine 09:C1D9 ObjectShatterMarker
 * WHAT IT DOES: Sets up a shattering marker: colour 0 with bit 7, offset
 * +$27 = 0, speed +$29 = 2, +$48 = 1, restart $09:C217
 * (ObjectShatterMarkerRecord), frame $09:C225 (ObjectShatterMarkerStep);
 * its record and frame.
 * HOW: Word and byte stores. Nothing in the game reaches it; reached only
 * by unit_objects_09f.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectShatterMarker(Carried *cr)
{
    st8i_hi(cr, F_c(0x205A, cr->x), cyc_then(16, 0x00));
    st16i(cr, F_c(0x2027, cr->x), cyc_then(22, 0x0000));
    st16i(cr, F_c(0x2029, cr->x), cyc_then(44, 0x0002));
    st8i(cr, F_c(0x2048, cr->x), cyc_then(28, 0x01));
    handlers2(cr, 0xC217, 0xC225);
    shatter_marker_record(cr);
    (cyc(102), shatter_marker_frame(cr));
}

void abi_ObjectShatterMarker(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatterMarker(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C217 ObjectShatterMarkerRecord
 * WHAT IT DOES: The marker's restart: its record ($42D, flag bit 4, +$2A
 * high 1), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordSet2AHigh. Reached only
 * by unit_objects_09f.
 * Entry: as ObjectShatterMarker. Exit: as ObjectShatterMarker.
 */
void ObjectShatterMarkerRecord(Carried *cr)
{
    shatter_marker_record(cr);
    (cyc(102), shatter_marker_frame(cr));
}

void abi_ObjectShatterMarkerRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatterMarkerRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C225 ObjectShatterMarkerStep
 * WHAT IT DOES: The marker's frame: it bounces (MarkerBounceY) and syncs to
 * its record; a car's contact (flags bit 0) shatters it (ObjectShatter) and
 * releases it.
 * HOW: ObjectSyncToRecord, RecordNextContact; PHX/PLX around the JSR.
 * Reached only by unit_objects_09f.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectShatterMarkerStep(Carried *cr)
{
    shatter_marker_frame(cr);
}

void abi_ObjectShatterMarkerStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatterMarkerStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A lap light's set-up in a one-player race: colour 0 with bit 7, +$0A =
   4 (no lap), the scale +$15 moved by ds (none when 0), restart $09:C30A,
   frame $09:C312; with a HUD mode released. */
static void lap_light(Carried *cr, uint8_t ds)
{
    if ((cyc(14), released_by_hud(cr)))
        { cyc(44); return; } else { cyc(16); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    st8i_hi(cr, F_c(0x205A, cr->x), 0x00);
    st8i(cr, F_c(0x200A, cr->x), cyc_then(6, 0x04));
    if (ds)
        add8(cr, F_c(0x2015, cr->x), ds);
    handlers2(cr, 0xC30A, 0xC312);
    cyc(44);
}

/*
 * @routine 09:C244 ObjectLapLight
 * WHAT IT DOES: Sets up a lap light in a one-player race: colour 0 with bit
 * 7, +$0A = 4, restart $09:C30A (ObjectLapLightRecord), frame $09:C312
 * (ObjectLapLightStep); with a HUD mode ($0D2B) it is released.
 * HOW: Word and byte stores; starts with a NOP, as the course data's
 * pointers expect.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A low
 * byte = 9, or as ObjectRelease leaves it.
 */
void ObjectLapLight(Carried *cr)
{
    lap_light(cr, 0x00);
}

void abi_ObjectLapLight(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapLight(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C280 ObjectLapLightLeft
 * WHAT IT DOES: Sets up a lap light in a one-player race: colour 0 with bit
 * 7, +$0A = 4, its scale +$15 down by $20, restart $09:C30A (ObjectLapLightRecord), frame $09:C312
 * (ObjectLapLightStep); with a HUD mode ($0D2B) it is released.
 * HOW: Word and byte stores; starts with a NOP, as the course data's
 * pointers expect.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A low
 * byte = 9, or as ObjectRelease leaves it.
 */
void ObjectLapLightLeft(Carried *cr)
{
    lap_light(cr, 0xE0);
}

void abi_ObjectLapLightLeft(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapLightLeft(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C2C5 ObjectLapLightRight
 * WHAT IT DOES: Sets up a lap light in a one-player race: colour 0 with bit
 * 7, +$0A = 4, its scale +$15 up by $20, restart $09:C30A (ObjectLapLightRecord), frame $09:C312
 * (ObjectLapLightStep); with a HUD mode ($0D2B) it is released.
 * HOW: Word and byte stores; starts with a NOP, as the course data's
 * pointers expect.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: M 8-bit; A low
 * byte = 9, or as ObjectRelease leaves it.
 */
void ObjectLapLightRight(Carried *cr)
{
    lap_light(cr, 0x20);
}

void abi_ObjectLapLightRight(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapLightRight(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:C312: a new lap ($0DEA against +$0A) clears the light (+$25 bit 0,
   colour 0, +$48 = 0); lit, it syncs and counts +$48 up to 2
   (ObjectCountUpTo2); unlit, a car's hit lights it for this lap (+$0A),
   sound $5C, and puts a puff at the player ($0346, ObjectSpawnPuff). */
static void lap_light_frame(Carried *cr)
{
    if (!lap_eq(cr, 0x200A)) {
        cyc(16);
        and8(cr, F_c(0x2025, cr->x), 0xFE);
        st8i_hi(cr, F_c(0x205A, cr->x), cyc_then(16, 0x00));
        st8i(cr, F_c(0x2048, cr->x), cyc_then(6, 0x00));
    } else { cyc(22); }
    if (bit25(cr, 0x01)) {
        cyc(106);
        ObjectSyncFromRecord_cr(cr);
        (cyc(46), ObjectCountUpTo2(cr));
        { cyc(44); return; }
    } else { cyc(18); }
    if (!hit(cr))
        { cyc(62); return; } else { cyc(0); }
    lap_store(cr, 0x200A);
    sound0(cr, 0x5C);
    or8(cr, F_c(0x2025, cr->x), 0x01);
    cr->y = rd16(cyc_then(24, db(cr) | W(wram_0346)));
    set_p(cr, nz16(cr->p, cr->y));
    (cyc(46), ObjectSpawnPuff(cr));
    cyc(44);
}

/*
 * @routine 09:C30A ObjectLapLightRecord
 * WHAT IT DOES: The lap light's restart: its record ($42D, flag bit 4),
 * then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectLapLightRecord(Carried *cr)
{
    cyc(62);
    GsuRecordNew42D_cr(cr);
    GsuRecordFlag10_cr(cr);
    lap_light_frame(cr);
}

void abi_ObjectLapLightRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapLightRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C312 ObjectLapLightStep
 * WHAT IT DOES: The lap light's frame: on a new lap ($0DEA against +$0A) it
 * goes out (+$25 bit 0, colour 0, +$48 = 0); lit, it syncs and counts +$48
 * up to 2; unlit, a car's hit lights it for this lap with sound $5C and a
 * puff at the player ($0346).
 * HOW: The laps through $1F58; ObjectSyncFromRecord, RecordContactSummary;
 * JSR ObjectSpawnPuff with Y = the player, JSR ObjectCountUpTo2.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectLapLightStep(Carried *cr)
{
    lap_light_frame(cr);
}

void abi_ObjectLapLightStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectLapLightStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C3ED ObjectPuffFollow
 * WHAT IT DOES: A puff following the player ($0346): at its position,
 * colour 1 with bit 7, its rise speed +$29 eased toward -$640
 * (ApproachEighthMin8) and added to its height +$0E (through $1F4E);
 * released when its life +$0A runs out.
 * HOW: ObjectCopyPositionFrom, ObjectRebucket first; DEC. Nothing in the
 * game reaches it; reached only by unit_objects_09f.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as DEC or
 * ObjectRelease leaves it.
 */
void ObjectPuffFollow(Carried *cr)
{
    cr->y = rd16(cyc_then(24, db(cr) | W(wram_0346)));
    set_p(cr, nz16(cr->p, cr->y));
    (cyc(62), ObjectCopyPositionFrom_cr(cr));
    cyc(22);
    (cyc(40), ObjectRebucket_cr(cr));
    st8i_hi(cr, F_c(0x2018, cr->x), cyc_then(16, 0x01));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(52, F_c(0x2029, cr->x)));
    cyc(16);
    wr16(W(dp_3e), cr->a);
    r_lda16_imm(cr, 0xF9C0);
    r_approach(cr, ApproachEighthMin8);
    wr16(F_c(0x2029, cr->x), cyc_then(30, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    copy16(cr, (cyc(22), db(cr)) | W(wram_1F4E), F_c(0x2029, cr->x));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    r_lda16(cr, cyc_then(74, F_c(0x200E, cr->x)));
    r_add16(cr, rd16(cyc_then(38, db(cr) | W(wram_1F4E))));
    wr16(F_c(0x200E, cr->x), cyc_then(30, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(44);
    if (!dec8(cr, F_c(0x200A, cr->x)))
        { cyc(84); ObjectRelease_cr(cr); cyc(44); }
    else
        cyc(92);
}

void abi_ObjectPuffFollow(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPuffFollow(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ---- 09:B876-C071: the second shatter, shards, bonus markers ---- */

void abi_HudCountUp19EC(void);

/*
 * @routine 09:B876 ObjectShatter2
 * WHAT IT DOES: As ObjectShatter with the shard kinds $B134, $B150, $B16C,
 * $B188, $B1A4 and $B1C0.
 * HOW: As ObjectShatter; the untaken sign stores for the constant offsets
 * (09:B8BC, B8CF, B9A3, B9B6, ...) are only reached by direct calls, as
 * ObjectShatter2Shard1XSign and the rest.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the last
 * shard leaves it.
 */
void ObjectShatter2(Carried *cr)
{
    shatter_rest(cr, shatter2_kinds, 0);
}

void abi_ObjectShatter2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatter2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B8BC ObjectShatter2Shard1XSign
 * WHAT IT DOES: Shard 1 of ObjectShatter2 from its x sign store on: direct
 * page $05 = 0 (as for a positive x offset), then the rest of that shard
 * (the y and third components' bytes, the turns, the placement, life and
 * handler) and the shards after it.
 * HOW: The x offset of shard 1 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter2.
 */
void ObjectShatter2Shard1XSign(Carried *cr)
{
    shatter_from_x(cr, shatter2_kinds, 0, 0);
}

void abi_ObjectShatter2Shard1XSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatter2Shard1XSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B8CF ObjectShatter2Shard1YSign
 * WHAT IT DOES: Shard 1 of ObjectShatter2 from its y sign store on: direct
 * page $0B = 0 (as for a positive y offset), then the rest of that shard
 * (the third component's byte, the turns, the placement, life and handler)
 * and the shards after it.
 * HOW: The y offset of shard 1 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter2.
 */
void ObjectShatter2Shard1YSign(Carried *cr)
{
    shatter_from_y(cr, shatter2_kinds, 0, 0);
}

void abi_ObjectShatter2Shard1YSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatter2Shard1YSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B9A3 ObjectShatter2Shard2XSign
 * WHAT IT DOES: Shard 2 of ObjectShatter2 from its x sign store on: direct
 * page $05 = 0 (as for a positive x offset), then the rest of that shard
 * (the y and third components' bytes, the turns, the placement, life and
 * handler) and the shards after it.
 * HOW: The x offset of shard 2 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter2.
 */
void ObjectShatter2Shard2XSign(Carried *cr)
{
    shatter_from_x(cr, shatter2_kinds, 1, 0);
}

void abi_ObjectShatter2Shard2XSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatter2Shard2XSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B9B6 ObjectShatter2Shard2YSign
 * WHAT IT DOES: Shard 2 of ObjectShatter2 from its y sign store on: direct
 * page $0B = 0 (as for a positive y offset), then the rest of that shard
 * (the third component's byte, the turns, the placement, life and handler)
 * and the shards after it.
 * HOW: The y offset of shard 2 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter2.
 */
void ObjectShatter2Shard2YSign(Carried *cr)
{
    shatter_from_y(cr, shatter2_kinds, 1, 0);
}

void abi_ObjectShatter2Shard2YSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatter2Shard2YSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:BA84 ObjectShatter2Shard3XSign
 * WHAT IT DOES: Shard 3 of ObjectShatter2 from its x sign store on: direct
 * page $05 = $FF (as for a negative x offset), then the rest of that shard
 * (the y and third components' bytes, the turns, the placement, life and
 * handler) and the shards after it.
 * HOW: The x offset of shard 3 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter2.
 */
void ObjectShatter2Shard3XSign(Carried *cr)
{
    shatter_from_x(cr, shatter2_kinds, 2, 1);
}

void abi_ObjectShatter2Shard3XSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatter2Shard3XSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:BA9D ObjectShatter2Shard3YSign
 * WHAT IT DOES: Shard 3 of ObjectShatter2 from its y sign store on: direct
 * page $0B = 0 (as for a positive y offset), then the rest of that shard
 * (the third component's byte, the turns, the placement, life and handler)
 * and the shards after it.
 * HOW: The y offset of shard 3 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter2.
 */
void ObjectShatter2Shard3YSign(Carried *cr)
{
    shatter_from_y(cr, shatter2_kinds, 2, 0);
}

void abi_ObjectShatter2Shard3YSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatter2Shard3YSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:BB6B ObjectShatter2Shard4XSign
 * WHAT IT DOES: Shard 4 of ObjectShatter2 from its x sign store on: direct
 * page $05 = $FF (as for a negative x offset), then the rest of that shard
 * (the y and third components' bytes, the turns, the placement, life and
 * handler) and the shards after it.
 * HOW: The x offset of shard 4 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter2.
 */
void ObjectShatter2Shard4XSign(Carried *cr)
{
    shatter_from_x(cr, shatter2_kinds, 3, 1);
}

void abi_ObjectShatter2Shard4XSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatter2Shard4XSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:BB84 ObjectShatter2Shard4YSign
 * WHAT IT DOES: Shard 4 of ObjectShatter2 from its y sign store on: direct
 * page $0B = 0 (as for a positive y offset), then the rest of that shard
 * (the third component's byte, the turns, the placement, life and handler)
 * and the shards after it.
 * HOW: The y offset of shard 4 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter2.
 */
void ObjectShatter2Shard4YSign(Carried *cr)
{
    shatter_from_y(cr, shatter2_kinds, 3, 0);
}

void abi_ObjectShatter2Shard4YSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatter2Shard4YSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:BC52 ObjectShatter2Shard5XSign
 * WHAT IT DOES: Shard 5 of ObjectShatter2 from its x sign store on: direct
 * page $05 = $FF (as for a negative x offset), then the rest of that shard
 * (the y and third components' bytes, the turns, the placement, life and
 * handler) and the shards after it.
 * HOW: The x offset of shard 5 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter2.
 */
void ObjectShatter2Shard5XSign(Carried *cr)
{
    shatter_from_x(cr, shatter2_kinds, 4, 1);
}

void abi_ObjectShatter2Shard5XSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatter2Shard5XSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:BC6B ObjectShatter2Shard5YSign
 * WHAT IT DOES: Shard 5 of ObjectShatter2 from its y sign store on: direct
 * page $0B = 0 (as for a positive y offset), then the rest of that shard
 * (the third component's byte, the turns, the placement, life and handler)
 * and the shards after it.
 * HOW: The y offset of shard 5 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter2.
 */
void ObjectShatter2Shard5YSign(Carried *cr)
{
    shatter_from_y(cr, shatter2_kinds, 4, 0);
}

void abi_ObjectShatter2Shard5YSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatter2Shard5YSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:BD39 ObjectShatter2Shard6XSign
 * WHAT IT DOES: Shard 6 of ObjectShatter2 from its x sign store on: direct
 * page $05 = $FF (as for a negative x offset), then the rest of that shard
 * (the y and third components' bytes, the turns, the placement, life and
 * handler) and the shards after it.
 * HOW: The x offset of shard 6 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter2.
 */
void ObjectShatter2Shard6XSign(Carried *cr)
{
    shatter_from_x(cr, shatter2_kinds, 5, 1);
}

void abi_ObjectShatter2Shard6XSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatter2Shard6XSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:BD4C ObjectShatter2Shard6YSign
 * WHAT IT DOES: Shard 6 of ObjectShatter2 from its y sign store on: direct
 * page $0B = $FF (as for a negative y offset), then the rest of that shard
 * (the third component's byte, the turns, the placement, life and handler)
 * and the shards after it.
 * HOW: The y offset of shard 6 is a constant of the other sign, so no code
 * reaches these bytes; a direct call is their only way in.
 * Entry: M 8-bit, X 16-bit; X = object, Y = the shard object; D = $0000;
 * DB = $7E. Exit: as ObjectShatter2.
 */
void ObjectShatter2Shard6YSign(Carried *cr)
{
    shatter_from_y(cr, shatter2_kinds, 5, 1);
}

void abi_ObjectShatter2Shard6YSign(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShatter2Shard6YSign(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:BDE1 ObjectShardStep
 * WHAT IT DOES: A shard's frame: it flies by 4 times its spin vector (+$48,
 * +$49 through $1F58/$1F59, direct page $00) turned by its angles +$14
 * (RotateXZFrom08) and +$15 (RotateXZ), grows (+$15 by $12), and is
 * released when its life +$0A runs out.
 * HOW: Direct page $02/$08/$94 in, $04/$0A/$05F2 out; two ASLs; DEC;
 * ObjectRebucket.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
 * leaves it.
 */
void ObjectShardStep(Carried *cr)
{
    uint8_t v;
    int i;
    static const uint8_t lo[2] = {0x04, 0x0A};

    v = ld8(cr, cyc_then(22, F_c(0x2048, cr->x)));
    wr8(db(cr) | W(wram_1F58), cyc_then(24, v));
    v = ld8(cr, cyc_then(22, F_c(0x2049, cr->x)));
    wr8(db(cr) | W(wram_1F59), cyc_then(24, v));
    axis_from(cr, db(cr) | W(wram_1F58), dp(cr, 0x02), dp(cr, 0x04), dp(cr, 0x05));
    axis_from(cr, db(cr) | W(wram_1F59), dp(cr, 0x08), dp(cr, 0x0A), dp(cr, 0x0B));
    axis_from(cr, dp(cr, 0x00), dp(cr, 0x94), db(cr) | W(wram_05F2), db(cr) | W(rot_z_high));
    ld8(cr, cyc_then(22, F_c(0x2014, cr->x)));
    (cyc(62), RotateXZFrom08_cr(cr));
    orbit_again(cr);
    ld8(cr, cyc_then(22, F_c(0x2015, cr->x)));
    (cyc(62), RotateXZ_cr(cr));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cyc(22);
    for (i = 0; i < 2; i++) {
        r_lda16(cr, cyc_then(12, dp(cr, lo[i])));
        r_asl16(cr);
        r_asl16(cr);
        wr16(dp(cr, lo[i]), cyc_then(40, cr->a));
    }
    r_lda16(cr, cyc_then(24, db(cr) | W(wram_05F2)));
    r_asl16(cr);
    r_asl16(cr);
    wr16(db(cr) | W(wram_05F2), cyc_then(52, cr->a));
    r_lda16(cr, cyc_then(30, F_c(0x200C, cr->x)));
    r_add16(cr, rd16(cyc_then(26, dp(cr, 0x04))));
    wr16(F_c(0x200C, cr->x), cyc_then(30, cr->a));
    r_lda16(cr, cyc_then(30, F_c(0x2010, cr->x)));
    r_add16(cr, rd16(cyc_then(38, db(cr) | W(wram_05F2))));
    wr16(F_c(0x2010, cr->x), cyc_then(30, cr->a));
    r_lda16(cr, cyc_then(30, F_c(0x200E, cr->x)));
    r_add16(cr, rd16(cyc_then(26, dp(cr, 0x0A))));
    wr16(F_c(0x200E, cr->x), cyc_then(30, cr->a));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    add8(cr, F_c(0x2015, cr->x), cyc_then(22, 0x12));
    (cyc(22), life_down(cr, 62));
    ObjectRebucket_cr(cr);
    cyc(44);
}

void abi_ObjectShardStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectShardStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:BE92 ObjectRisingSpark
 * WHAT IT DOES: A bonus spark's frame: at its object's (+$29) position,
 * released when its life +$0A runs out; it rises by $80 and a speed +$27
 * that grows by $28 each frame, and grows (+$15 by $12).
 * HOW: ObjectCopyPositionFrom; the speed added through $1F4C;
 * ObjectRebucket.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as ObjectRebucket
 * leaves it.
 */
void ObjectRisingSpark(Carried *cr)
{
    cr->y = rd16(cyc_then(30, F_c(0x2029, cr->x)));
    set_p(cr, nz16(cr->p, cr->y));
    (cyc(62), ObjectCopyPositionFrom_cr(cr));
    (cyc(22), life_down(cr, 22));
    add16(cr, F_c(0x200E, cr->x), 0xFF80);
    add16(cr, F_c(0x2027, cr->x), cyc_then(44, 0xFFD8));
    add16_via(cr, F_c(0x200E, cr->x), F_c(0x2027, cr->x));
    add8(cr, F_c(0x2015, cr->x), cyc_then(22, 0x12));
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

void abi_ObjectRisingSpark(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRisingSpark(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* 09:BF34 / BFF6: the marker bounces and syncs to its record; a car's
   contact (flags bit 0) counts $19EC up (HudCountUp19EC), shatters it, puts
   a rising spark of `kind` at the player ($0346; +$0A = $0A, +$27 = 0,
   +$29 = the player, handler $09:BE92), adds `bonus` to the count $19EE
   (BCD) and releases it. */
static void bonus_marker2_frame(Carried *cr, void (*shatter)(Carried *), uint16_t kind, uint8_t bonus)
{
    uint16_t sx;
    uint8_t v;

    (cyc(46), MarkerBounceY(cr));
    (cyc(62), ObjectSyncToRecord_cr(cr));
    cyc(10);
    cr->a = (uint16_t)((cr->a & 0xFF00) | (cr->a & 0x01));
    set_p(cr, nz8(cr->p, (uint8_t)cr->a));
    if (!(uint8_t)cr->a)
        { cyc(72); return; }
    if (!next_contact(cr))
        return; else { cyc(24); }
    HudCountUp19EC_cr(cr);
    sx = cr->x;
    (cyc(76), shatter(cr));
    cr->x = sx;
    set_p(cr, nz16(cr->p, sx));
    cyc(66);
    if (spawn(cr, kind)) {
        uint8_t a;

        cr->x = rd16(cyc_then(24, db(cr) | W(wram_0346)));
        set_p(cr, nz16(cr->p, cr->x));
        st8i(cr, F_c(0x200A, cr->y), cyc_then(6, 0x0A));
        (cyc(62), ObjectCopyPositionTo_cr(cr));
        st16i(cr, F_c(0x2027, cr->y), cyc_then(22, 0x0000));
        a = (uint8_t)cr->a;
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
        cr->a = cr->x;
        set_p(cr, nz16(cr->p, cr->a));
        wr16(F_c(0x2029, cr->y), cyc_then(110, cr->x));
        set_p(cr, (uint8_t)(cr->p | (P_M)));
        cyc(52);
        lda8(cr, a);
        handler(cr, cr->y, 0xBE92);
    } else { cyc(26); }
    cyc(88);
    cr->x = sx;
    set_p(cr, nz16(cr->p, sx));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    set_p(cr, (uint8_t)(cr->p | (P_D)));
    v = ld8(cr, db(cr) | W(wram_19EE));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    v = alu_adc8(v, bonus, &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    wr8(db(cr) | W(wram_19EE), cyc_then(54, v));
    v = ld8(cr, (cyc(16), db(cr)) | W(wram_19EF));
    v = alu_adc8(v, 0x00, &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    wr8(db(cr) | W(wram_19EF), cyc_then(40, v));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_D)));
    (cyc(76), ObjectRelease_cr(cr));
    cyc(44);
}

/* The bonus marker's start: colour 0 with bit 7, offset +$27 = 0, speed
   +$29 = 2. */
static void bonus_marker2_init(Carried *cr)
{
    st8i_hi(cr, F_c(0x205A, cr->x), cyc_then(16, 0x00));
    st16i(cr, F_c(0x2027, cr->x), cyc_then(22, 0x0000));
    st16i(cr, F_c(0x2029, cr->x), cyc_then(44, 0x0002));
}

/*
 * @routine 09:BEE8 ObjectBonusMarker2
 * WHAT IT DOES: Sets up a bonus marker worth 2: colour 0, offset +$27 = 0,
 * speed +$29 = 2, +$48 = 1, restart $09:BF26 (ObjectBonusMarker2Record),
 * frame $09:BF34 (ObjectBonusMarker2Step); its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBonusMarker2(Carried *cr)
{
    bonus_marker2_init(cr);
    st8i(cr, F_c(0x2048, cr->x), cyc_then(28, 0x01));
    handlers2(cr, 0xBF26, 0xBF34);
    shatter_marker_record(cr);
    cyc(102);
    bonus_marker2_frame(cr, ObjectShatter, 0xB674, 0x02);
}

void abi_ObjectBonusMarker2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBonusMarker2(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:BF26 ObjectBonusMarker2Record
 * WHAT IT DOES: The marker's restart: its record ($42D, flag bit 4, +$2A
 * high 1), then its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10, GsuRecordSet2AHigh.
 * Entry: as ObjectBonusMarker2. Exit: as ObjectBonusMarker2.
 */
void ObjectBonusMarker2Record(Carried *cr)
{
    shatter_marker_record(cr);
    cyc(102);
    bonus_marker2_frame(cr, ObjectShatter, 0xB674, 0x02);
}

void abi_ObjectBonusMarker2Record(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBonusMarker2Record(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:BF34 ObjectBonusMarker2Step
 * WHAT IT DOES: The marker's frame: it bounces (MarkerBounceY) and syncs to
 * its record; a car's contact counts $19EC up, shatters it (ObjectShatter),
 * sends up a spark (kind $B674) at the player, adds 2 to the count $19EE
 * (BCD) and releases it.
 * HOW: ObjectSyncToRecord, RecordNextContact; JSRs HudCountUp19EC and
 * ObjectShatter; SED, ADC, CLD.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectBonusMarker2Step(Carried *cr)
{
    bonus_marker2_frame(cr, ObjectShatter, 0xB674, 0x02);
}

void abi_ObjectBonusMarker2Step(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBonusMarker2Step(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:BFB0 ObjectBonusMarker5
 * WHAT IT DOES: Sets up a bonus marker worth 5: colour 0, offset +$27 = 0,
 * speed +$29 = 2, restart $09:BFEE (ObjectBonusMarker5Record), frame
 * $09:BFF6 (ObjectBonusMarker5Step), +$48 = 1; its record and frame.
 * HOW: Word and byte stores.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the frame
 * leaves it.
 */
void ObjectBonusMarker5(Carried *cr)
{
    bonus_marker2_init(cr);
    cyc(22);
    handlers2(cr, 0xBFEE, 0xBFF6);
    st8i(cr, F_c(0x2048, cr->x), cyc_then(6, 0x01));
    (cyc(62), GsuRecordNew42D_cr(cr));
    GsuRecordFlag10_cr(cr);
    bonus_marker2_frame(cr, ObjectShatter2, 0xB604, 0x05);
}

void abi_ObjectBonusMarker5(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBonusMarker5(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:BFEE ObjectBonusMarker5Record
 * WHAT IT DOES: The marker's restart: its record ($42D, flag bit 4), then
 * its frame.
 * HOW: GsuRecordNew42D, GsuRecordFlag10.
 * Entry: as ObjectBonusMarker5. Exit: as ObjectBonusMarker5.
 */
void ObjectBonusMarker5Record(Carried *cr)
{
    cyc(62);
    GsuRecordNew42D_cr(cr);
    GsuRecordFlag10_cr(cr);
    bonus_marker2_frame(cr, ObjectShatter2, 0xB604, 0x05);
}

void abi_ObjectBonusMarker5Record(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBonusMarker5Record(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:BFF6 ObjectBonusMarker5Step
 * WHAT IT DOES: The marker's frame: as ObjectBonusMarker2Step with
 * ObjectShatter2, a spark of kind $B604 and 5 added to $19EE.
 * HOW: As ObjectBonusMarker2Step.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: as the path leaves
 * it.
 */
void ObjectBonusMarker5Step(Carried *cr)
{
    bonus_marker2_frame(cr, ObjectShatter2, 0xB604, 0x05);
}

void abi_ObjectBonusMarker5Step(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectBonusMarker5Step(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C0FE ObjectRiderStep
 * WHAT IT DOES: The handler of the object ObjectRiderSpawn makes: once its
 * parent (+$27) has +$25 bit 0 set it is released (ObjectRelease);
 * otherwise its heading +$15 turns by 4 and it is rebucketed.
 * HOW: An 8-bit test of the parent's +$25; JML to the release at
 * $09:C11A.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectRebucket or
 * ObjectRelease.
 */
void ObjectRiderStep(Carried *cr)
{
    uint16_t par = rd16(cyc_then(30, F_c(0x2027, cr->x)));
    uint8_t v;

    cr->y = par;
    set_p(cr, nz16(cr->p, par));
    v = (uint8_t)(ld8(cr, cyc_then(22, F_c(0x2025, par))) & 0x01);
    lda8(cr, v);
    if (v) {
        cyc(44);
        (cyc(62), ObjectRelease_cr(cr));
        { cyc(44); return; }
    } else { cyc(40); }
    v = ld8(cr, F_c(0x2015, cr->x));
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_C)));
    v = alu_adc8(v, 0x04, &cr->p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    wr8(F_c(0x2015, cr->x), cyc_then(60, v));
    (cyc(62), ObjectRebucket_cr(cr));
    cyc(44);
}

void abi_ObjectRiderStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRiderStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:C0BE ObjectRiderSpawn
 * WHAT IT DOES: Makes a rider (kind $AD44) at object X's position $A0
 * lower (+$0E + $A0), with X as its parent (+$27) and the handler
 * $09:C0FE (ObjectRiderStep); nothing without a free object. Nothing in
 * the game calls it.
 * HOW: ObjectSpawn, ObjectCopyPositionTo; A's low byte pushed and pulled
 * (8-bit) round the parent's store (TXA, 16-bit). Ends in RTS.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; made: A low byte =
 * $09; none: as ObjectSpawn.
 */
void ObjectRiderSpawn(Carried *cr)
{
    uint8_t lo;

    if (!spawn(cr, 0xAD44))
        { cyc(68); return; }
    (cyc(62), ObjectCopyPositionTo_cr(cr));
    add16(cr, F_c(0x200E, cr->y), cyc_then(22, 0x00A0));
    lo = (uint8_t)cr->a;
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->a = cr->x;
    wr16(F_c(0x2027, cr->y), cyc_then(110, cr->x));
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | lo);
    set_p(cr, nz8(cr->p, lo));
    cyc(72);
    handler(cr, cr->y, 0xC0FE);
    cyc(42);
}

void abi_ObjectRiderSpawn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectRiderSpawn(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_ObjectStep48To8(void);
void abi_HudCountMove19EE(void);

/* A switch lamp's set-up: +$1D = rec (its record set-up), handler step,
   shape +$20 = $ECD0, +$5A = $80 (dark), count +$48 = 4, +$5A = $80
   again. */
static void hit_lamp(Carried *cr, uint16_t rec, uint16_t step)
{
    st16i(cr, F_c(0x201D, cr->x), cyc_then(22, rec));
    st8i(cr, F_c(0x201F, cr->x), cyc_then(28, 0x09));
    cyc(22);
    handler(cr, cr->x, step);
    st16i(cr, F_c(0x2020, cr->x), cyc_then(22, 0xECD0));
    lda8(cr, 0x00);
    lda8(cr, 0x80);
    wr8(F_c(0x205A, cr->x), cyc_then(44, 0x80));
    st8i(cr, F_c(0x2048, cr->x), cyc_then(6, 0x04));
    lda8(cr, 0x00);
    lda8(cr, 0x80);
    wr8(F_c(0x205A, cr->x), cyc_then(22, 0x80));
}

/* The switch lamp's record: GsuRecordNew424Alt in mode 2 with $14 in the
   high byte of +$2A, at ($500, $500, $500). */
static void hit_lamp_record(Carried *cr)
{
    (cyc(62), GsuRecordNew424Alt_cr(cr));
    (cyc(62), GsuRecordMode2_cr(cr));
    lda8(cr, 0x14);
    (cyc(58), GsuRecordSet2AHigh_cr(cr));
    cyc(102);
    record_at(cr, 0x0500, 0x0500, 0x0500);
}

/* The switch lamp's frame: while the switch byte at `flag` is set, it
   lights up a step (ObjectStep48To8); then it follows its record. */
static void hit_lamp_frame(Carried *cr, uint16_t flag)
{
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    if (ld8(cr, (cyc(38), db(cr)) | flag))
        { cyc(62); ObjectStep48To8_cr(cr); } else { cyc(22); }
    cyc(62);
    ObjectSyncFromRecord_cr(cr);
    cyc(44);
}

/* The switch's record: GsuRecordNew42D with flag $10 at ($3C0, $3C0,
   $3C0). */
static void hit_switch_record(Carried *cr)
{
    (cyc(62), GsuRecordNew42D_cr(cr));
    GsuRecordFlag10_cr(cr);
    record_at(cr, 0x03C0, 0x03C0, 0x03C0);
}

/* The switch's frame: on a car's contact (bit 0 of a contact's flags) the
   switch byte at `flag` = 1, and the first time (+$25 bit 0 clear) +$25
   bit 0 is set and the count moved (HudCountMove19EE). */
static void hit_switch_frame(Carried *cr, uint16_t flag)
{
    (cyc(62), ObjectSyncFromRecord_cr(cr));
    cyc(10);
    cr->a = (uint16_t)((cr->a & 0xFF00) | (cr->a & 0x01));
    set_p(cr, nz8(cr->p, (uint8_t)cr->a));
    if (!(uint8_t)cr->a || !next_contact_t(cr, 28))
        { cyc(72); return; } else { cyc(22); }
    lda8(cr, (uint8_t)(ld8(cr, F_c(0x2025, cr->x)) & 0x01));
    if (!(uint8_t)cr->a) {
        cyc(18);
        or8(cr, F_c(0x2025, cr->x), 0x01);
        (cyc(46), HudCountMove19EE_cr(cr));
        cyc(22);                        /* SEP */
    } else { cyc(66); }
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    st8i(cr, db(cr) | flag, 0x01);
    cyc(44);
}

/*
 * @routine 09:B19B ObjectSwitchLampBStep
 * WHAT IT DOES: The frame of a lamp that lights up once switch B ($19E9)
 * has been hit: a step brighter (ObjectStep48To8) while $19E9 is set, then
 * it follows its record.
 * HOW: An 8-bit test of $19E9; ObjectSyncFromRecord.
 * Entry: X 16-bit; X = object. Exit: M 8-bit; as ObjectSyncFromRecord.
 */
void ObjectSwitchLampBStep(Carried *cr)
{
    hit_lamp_frame(cr, 0x19E9);
}

void abi_ObjectSwitchLampBStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwitchLampBStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B16E ObjectSwitchLampBRecord
 * WHAT IT DOES: The lamp's record (GsuRecordNew424Alt, mode 2, $14 in the
 * high byte of +$2A, at $500, $500, $500), then its first frame
 * (ObjectSwitchLampBStep). Also its +$1D pointer.
 * HOW: Falls into ObjectSwitchLampBStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSwitchLampBStep.
 */
void ObjectSwitchLampBRecord(Carried *cr)
{
    hit_lamp_record(cr);
    ObjectSwitchLampBStep(cr);
}

void abi_ObjectSwitchLampBRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwitchLampBRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B133 ObjectSwitchLampB
 * WHAT IT DOES: Sets up a lamp switch B lights: +$1D = $09:B16E, handler
 * $09:B19B (ObjectSwitchLampBStep), shape $ECD0, dark (+$5A = $80), +$48 =
 * 4; then its record and frame. No course in the scripts places it.
 * HOW: Stores, then falls into ObjectSwitchLampBRecord.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSwitchLampBStep.
 */
void ObjectSwitchLampB(Carried *cr)
{
    hit_lamp(cr, 0xB16E, 0xB19B);
    ObjectSwitchLampBRecord(cr);
}

void abi_ObjectSwitchLampB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwitchLampB(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B1EF ObjectSwitchBStep
 * WHAT IT DOES: The frame of switch B: it follows its record; on a car's
 * contact $19E9 = 1, and the first time its +$25 bit 0 is set and the
 * count moved (HudCountMove19EE).
 * HOW: ObjectSyncFromRecord, then RecordNextContact over the contacts; the
 * repeat hit is a long jump past the count.
 * Entry: M 8-bit, X 16-bit; X = object; D = $0000. Exit: hit: A low byte
 * = 1; otherwise as the last call leaves it.
 */
void ObjectSwitchBStep(Carried *cr)
{
    hit_switch_frame(cr, 0x19E9);
}

void abi_ObjectSwitchBStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwitchBStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B1C8 ObjectSwitchBRecord
 * WHAT IT DOES: Switch B's record (GsuRecordNew42D, flag $10, at $3C0,
 * $3C0, $3C0), then its first frame (ObjectSwitchBStep). Also its +$1D
 * pointer.
 * HOW: Falls into ObjectSwitchBStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSwitchBStep.
 */
void ObjectSwitchBRecord(Carried *cr)
{
    hit_switch_record(cr);
    ObjectSwitchBStep(cr);
}

void abi_ObjectSwitchBRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwitchBRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B1AA ObjectSwitchB
 * WHAT IT DOES: Sets up switch B: +$1D = $09:B1C8, handler $09:B1EF
 * (ObjectSwitchBStep); then its record and frame. No course in the
 * scripts places it.
 * HOW: Stores, then falls into ObjectSwitchBRecord.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSwitchBStep.
 */
void ObjectSwitchB(Carried *cr)
{
    st16i(cr, F_c(0x201D, cr->x), cyc_then(22, 0xB1C8));
    st8i(cr, F_c(0x201F, cr->x), cyc_then(28, 0x09));
    cyc(22);
    handler(cr, cr->x, 0xB1EF);
    ObjectSwitchBRecord(cr);
}

void abi_ObjectSwitchB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwitchB(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B287 ObjectSwitchLampCStep
 * WHAT IT DOES: As ObjectSwitchLampBStep for switch C ($19EA).
 * HOW: As ObjectSwitchLampBStep.
 * Entry: as ObjectSwitchLampBStep. Exit: as ObjectSwitchLampBStep.
 */
void ObjectSwitchLampCStep(Carried *cr)
{
    hit_lamp_frame(cr, 0x19EA);
}

void abi_ObjectSwitchLampCStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwitchLampCStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B25A ObjectSwitchLampCRecord
 * WHAT IT DOES: As ObjectSwitchLampBRecord, then ObjectSwitchLampCStep.
 * HOW: Falls into ObjectSwitchLampCStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSwitchLampCStep.
 */
void ObjectSwitchLampCRecord(Carried *cr)
{
    hit_lamp_record(cr);
    ObjectSwitchLampCStep(cr);
}

void abi_ObjectSwitchLampCRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwitchLampCRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B21F ObjectSwitchLampC
 * WHAT IT DOES: As ObjectSwitchLampB for switch C: +$1D = $09:B25A,
 * handler $09:B287 (ObjectSwitchLampCStep). No course in the scripts
 * places it.
 * HOW: Stores, then falls into ObjectSwitchLampCRecord.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSwitchLampCStep.
 */
void ObjectSwitchLampC(Carried *cr)
{
    hit_lamp(cr, 0xB25A, 0xB287);
    ObjectSwitchLampCRecord(cr);
}

void abi_ObjectSwitchLampC(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwitchLampC(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B2DB ObjectSwitchCStep
 * WHAT IT DOES: As ObjectSwitchBStep for switch C: a hit sets $19EA = 1.
 * HOW: As ObjectSwitchBStep.
 * Entry: as ObjectSwitchBStep. Exit: as ObjectSwitchBStep.
 */
void ObjectSwitchCStep(Carried *cr)
{
    hit_switch_frame(cr, 0x19EA);
}

void abi_ObjectSwitchCStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwitchCStep(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B2B4 ObjectSwitchCRecord
 * WHAT IT DOES: As ObjectSwitchBRecord, then ObjectSwitchCStep.
 * HOW: Falls into ObjectSwitchCStep.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSwitchCStep.
 */
void ObjectSwitchCRecord(Carried *cr)
{
    hit_switch_record(cr);
    ObjectSwitchCStep(cr);
}

void abi_ObjectSwitchCRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwitchCRecord(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 09:B296 ObjectSwitchC
 * WHAT IT DOES: As ObjectSwitchB for switch C: +$1D = $09:B2B4, handler
 * $09:B2DB (ObjectSwitchCStep). No course in the scripts places it.
 * HOW: Stores, then falls into ObjectSwitchCRecord.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSwitchCStep.
 */
void ObjectSwitchC(Carried *cr)
{
    st16i(cr, F_c(0x201D, cr->x), cyc_then(22, 0xB2B4));
    st8i(cr, F_c(0x201F, cr->x), cyc_then(28, 0x09));
    cyc(22);
    handler(cr, cr->x, 0xB2DB);
    ObjectSwitchCRecord(cr);
}

void abi_ObjectSwitchC(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSwitchC(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

