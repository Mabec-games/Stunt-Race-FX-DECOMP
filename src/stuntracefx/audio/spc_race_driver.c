/*
 * spc_race_driver.c: the race sound driver (N-SPC), the SPC700 code
 * uploaded from the block list at $19:C27C, as C on the SPC700 model
 * (spc.h): each access in the original order.
 */
#include "spc.h"
#include "spc_ops.h"

void abi_SpcReset(void);
void abi_SpcPortExchange(void);
void abi_SpcNoteOn(void);
void abi_SpcDspWriteVoice(void);
void abi_SpcDspWrite(void);
void abi_SpcReadPatternWord(void);
void abi_SpcMusicPause(void);
void abi_SpcMusicFrame(void);
void abi_SpcVcmdDispatch(void);
void abi_SpcTrackRead(void);
void abi_SpcTrackAdvance(void);
void abi_SpcSetInstrument(void);
void abi_SpcLoadInstrument(void);
void abi_SpcSetPan(void);
void abi_SpcSubroutineRestart(void);
void abi_SpcSetEchoDelay(void);
void abi_SpcSlideCheck(void);
void abi_SpcSlideToNote(void);
void abi_SpcLoadPitch(void);
void abi_SpcFadeDelta(void);
void abi_SpcNegateIfNeg(void);
void abi_SpcVoiceEffects(void);
void abi_SpcVoiceVolumeWrite(void);
void abi_SpcVoiceVolumeLR(void);
void abi_SpcVoiceTick(void);
void abi_SpcVoiceBetweenTicks(void);
void abi_SpcAddScaledDelta(void);
void abi_SpcAddDelta(void);
void abi_SpcTremoloBetweenTicks(void);
void abi_SpcTremoloFactor(void);
void abi_SpcVolumeCalc(void);
void abi_SpcSfxFrame(void);
void abi_SpcPort1Remap(void);
void abi_SpcChannelGuards(void);
void abi_SpcStaggeredChecks(void);
void abi_SpcGameStateTick(void);
void abi_SpcCommand4C(void);
void abi_SpcSoundQueuePush(void);
void abi_SpcCommandTimers(void);
void abi_SpcTempoBend(void);
void abi_SpcTempoBendStart(void);
void abi_SpcEchoTransition(void);
void abi_SpcEchoTransitionStep(void);
void abi_SpcPort1Clamp(void);
void abi_SpcCommandFilter(void);
void abi_SpcPitchRampB(void);
void abi_SpcPitchRampBUp(void);
void abi_SpcPitchRampBDown(void);
void abi_SpcPitchRampA(void);
void abi_SpcPitchRampAUp(void);
void abi_SpcPitchRampADown(void);
void abi_SpcPortCommand(void);
void abi_SpcPort1Command(void);
void abi_SpcSfxStopIf(void);
void abi_SpcSfxStop(void);
void abi_SpcEchoModeA(void);
void abi_SpcSfxChannelResume(void);
void abi_SpcSfxPointer(void);
void abi_SpcSfxChannelPause(void);
void abi_SpcSfxStopAll(void);
void abi_SpcSfxStopRequest(void);
void abi_SpcFadeOutStart(void);
void abi_SpcMusicFade(void);
void abi_SpcKeyOffAll(void);
void abi_SpcCommandChannelCheck(void);
void abi_SpcCommandBlock(void);
void abi_SpcSfxHolds(void);
void abi_SpcSfxActivate(void);
void abi_SpcSfxLookup(void);
void abi_SpcSfxChannel(void);
void abi_SpcSfxMode(void);
void abi_SpcSfxChannelsFrame(void);
void abi_SpcSfxChannelLoad(void);
void abi_SpcSfxChannelSave(void);
void abi_SpcSfxChannelStep(void);
void abi_SpcSfxTakeVoice(void);
void abi_SpcSfxMarkVoice(void);
void abi_SpcVoiceMask(void);
void abi_SpcVoiceMaskNot(void);
void abi_SpcSfxRelease(void);
void abi_SpcKeyOnVoice(void);
void abi_SpcKeyOffVoice(void);
void abi_SpcSfxRead(void);
void abi_SpcSfxEngineScale(void);
void abi_SpcSfxStart(void);
void abi_SpcSfxPitchSlide(void);
void abi_SpcSfxVolumeSlide(void);
void abi_SpcSfxFadeVolume(void);
void abi_SpcSfxVolumeAdjust(void);
void abi_SpcSfxSpecial(void);
void abi_SpcEngineVolume(void);
void abi_SpcEngineDamp(void);
void abi_SpcEngineToggle(void);
void abi_SpcEngineRevTimer(void);
void abi_SpcSfxPanScale(void);
void abi_SpcMul16(void);
void abi_SpcDiv16(void);
void abi_SpcIndexRight(void);
void abi_SpcIndexLeft(void);
void abi_SpcNoiseSample(void);
void abi_SpcNoiseSampleInit(void);
void abi_SpcReload(void);
void abi_SpcSfxFrameHoldReset(void);
void abi_SpcSfxStopMatchOld(void);
void abi_SpcSfxReadNoAge(void);

/*
 * @routine 19:C301 SpcReload
 * WHAT IT DOES: The driver's own upload loop, the IPL ROM's transfer protocol: $AA and $BB on ports 0 and 1, then, once port 0 reads $CC, block after block: the address from ports 2-3, each byte from port 1 stored there and acknowledged by writing its count back to port 0, a new block when port 0 jumps ahead; a block whose port 1 byte is 0 ends the transfer, CONTROL gets $31 (timer 0 on, the port latches cleared, the IPL ROM unmapped) and the driver restarts (SpcReset), whatever address came with it.
 * HOW: Polling loops on the ports; the store through the pointer $14/$15, its high byte stepped at each page.
 * Entry: jumped to at $1AEC (the race driver) or $19F8 (the battle driver), all keys off, for a reload. Exit: never returns (SpcReset).
 * @spc 0675
 * @spcuses 0675-06C3,0800-09EF,09F6-107A,10B0-16D8,16F1-1B8C,1B92-1C04,1C07-1DC1,1DCE-23EB,23FB-241B,2441-26D6,3FCA-4000,CA75-CAA8,CAC0-CB04,EDF0-EE20
 */
void abi_SpcReload(void)
{
    /* 0675 mov a,#$AA */
    spc.a = spc_nz(0xAA);
    spc_cyc(2);
    /* 0677 mov.w $00F4,a */
    spc_st_cyc(0x00F4, spc.a, 4, 1);
    /* 067A mov a,#$BB */
    spc.a = spc_nz(0xBB);
    spc_cyc(2);
    /* 067C mov.w $00F5,a */
    spc_st_cyc(0x00F5, spc.a, 4, 1);
L067F:
    /* 067F mov.w a,$00F4 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x00F4));
    /* 0682 cmp a,#$CC */
    spc_cmp(spc.a, 0xCC);
    spc_cyc(2);
    /* 0684 bne $067F */
    if (!spc.z) { spc_cyc(4); goto L067F; }
    spc_cyc(2);
    /* 0686 bra $06A8 */
    spc_cyc(4);
    goto L06A8;
L0688:
    /* 0688 mov.w y,$00F4 */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x00F4));
    /* 068B bne $0688 */
    if (!spc.z) { spc_cyc(4); goto L0688; }
    spc_cyc(2);
L068D:
    /* 068D cmp.w y,$00F4 */
    spc_cyc(4);
    spc_cmp(spc.y, spc_rd(0x00F4));
    /* 0690 bne $06A1 */
    if (!spc.z) { spc_cyc(4); goto L06A1; }
    spc_cyc(2);
    /* 0692 mov.w a,$00F5 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x00F5));
    /* 0695 mov.w $00F4,y */
    spc_st_cyc(0x00F4, spc.y, 4, 1);
    /* 0698 mov ($14)+y,a */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x14)); p |= (spc_cyc(1), spc_rd(spc_dp(0x15))) << 8;
      spc_st_cyc((uint16_t)(p + spc.y), spc.a, 2, 1); }
    /* 069A inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 069B bne $068D */
    if (!spc.z) { spc_cyc(4); goto L068D; }
    spc_cyc(2);
    /* 069D inc.b $15 */
    spc_rmw_cyc(spc_dp(0x15), spc_inc, 3, 1);
    /* 069F bra $068D */
    spc_cyc(4);
    goto L068D;
L06A1:
    /* 06A1 bpl $068D */
    if (!spc.n) { spc_cyc(4); goto L068D; }
    spc_cyc(2);
    /* 06A3 cmp.w y,$00F4 */
    spc_cyc(4);
    spc_cmp(spc.y, spc_rd(0x00F4));
    /* 06A6 bpl $068D */
    if (!spc.n) { spc_cyc(4); goto L068D; }
    spc_cyc(2);
L06A8:
    /* 06A8 mov.w a,$00F6 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x00F6));
    /* 06AB mov.w y,$00F7 */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x00F7));
    /* 06AE movw $14,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x14));
    spc_cyc(1);
    spc_wr(spc_dp(0x14), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x15), spc.y);
    /* 06B0 mov.w y,$00F4 */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x00F4));
    /* 06B3 mov.w a,$00F5 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x00F5));
    /* 06B6 mov.w $00F4,y */
    spc_st_cyc(0x00F4, spc.y, 4, 1);
    /* 06B9 bne $0688 */
    if (!spc.z) { spc_cyc(4); goto L0688; }
    spc_cyc(2);
    /* 06BB mov x,#$31 */
    spc.x = spc_nz(0x31);
    spc_cyc(2);
    /* 06BD mov.w $00F1,x */
    spc_st_cyc(0x00F1, spc.x, 4, 1);
    /* 06C0 jmp $0800 */
    { spc_cyc(3); abi_SpcReset(); return; }
}

/*
 * @routine 19:C48C SpcReset
 * WHAT IT DOES: The driver's start: the stack at $01CF, direct page $00-$DF cleared, the echo delay set to 1 (SpcSetEchoDelay), RAM $0200-$058F cleared, every voice's pitch high byte 1, the master volumes MVOLL/MVOLR $60 and the sample directory DIR at $3F00; the ports cleared, timer 0 at $10 (the tempo $53 too), the port bytes $04-$07 zero and every voice keyed off. Then the main loop forever: the DSP registers in the list at $3F9F written from the RAM bytes the list at $06C2 names (the echo ones only once the echo delay has settled), KON/KOF cleared, the noise LFSR $18/$19 stepped, a wait for timer 0, then the sound effects (SpcSfxFrame) when $43 + ticks * $20 carries, and the music (SpcMusicFrame) with the four port exchanges (SpcPortExchange) when $51 + ticks * tempo carries; otherwise, with a song playing, the between-tick work for each voice with a track (SpcVoiceBetweenTicks).
 * HOW: Runs at SPC $0800 (the block lists' start address). Clears by MOV (X+),A and indexed loops; DSP writes through SpcDspWrite. The LFSR: EOR of the two bytes, two LSRs, NOTC, ROR of both. The echo settle count $4C steps on each sound effect tick until it reaches $4D.
 * Entry: from the uploader (SpcReload or the IPL ROM). Exit: never returns.
 * @spc 0800
 * @spcuses 0800-09EF,09F6-107A,10B0-16D8,16F1-1B8C,1B92-1C04,1C07-1DC1,1DCE-23EB,23FB-2419,2441-26D6,3FCA-4000,CA75-CAA8,CAC0-CB04,EDF0-EE20
 */
void abi_SpcReset(void)
{
    /* 0800 clrp */
    spc.p = 0;
    spc_cyc(2);
    /* 0801 mov x,#$CF */
    spc.x = spc_nz(0xCF);
    spc_cyc(2);
    /* 0803 mov sp,x */
    spc.sp = spc.x;
    spc_cyc(2);
    /* 0804 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0806 mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
L0807:
    /* 0807 mov (x+),a */
    spc_cyc(4);
    spc_wr(spc_dp(spc.x), spc.a);
    spc.x++;
    /* 0808 cmp x,#$E0 */
    spc_cmp(spc.x, 0xE0);
    spc_cyc(2);
    /* 080A bne $0807 */
    if (!spc.z) { spc_cyc(4); goto L0807; }
    spc_cyc(2);
    /* 080C inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
    /* 080D call $0D73 */
    spc_call_cyc(abi_SpcSetEchoDelay, 0x0810, 5, 1, 2);
    /* 0810 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0812 mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
L0813:
    /* 0813 mov.w $0200+x,a */
    spc_st_cyc((uint16_t)(0x0200 + spc.x), spc.a, 5, 1);
    /* 0816 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 0817 bne $0813 */
    if (!spc.z) { spc_cyc(4); goto L0813; }
    spc_cyc(2);
L0819:
    /* 0819 mov.w $0300+x,a */
    spc_st_cyc((uint16_t)(0x0300 + spc.x), spc.a, 5, 1);
    /* 081C inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 081D bne $0819 */
    if (!spc.z) { spc_cyc(4); goto L0819; }
    spc_cyc(2);
L081F:
    /* 081F mov.w $0400+x,a */
    spc_st_cyc((uint16_t)(0x0400 + spc.x), spc.a, 5, 1);
    /* 0822 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 0823 bne $081F */
    if (!spc.z) { spc_cyc(4); goto L081F; }
    spc_cyc(2);
L0825:
    /* 0825 mov.w $0500+x,a */
    spc_st_cyc((uint16_t)(0x0500 + spc.x), spc.a, 5, 1);
    /* 0828 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 0829 cmp x,#$90 */
    spc_cmp(spc.x, 0x90);
    spc_cyc(2);
    /* 082B bne $0825 */
    if (!spc.z) { spc_cyc(4); goto L0825; }
    spc_cyc(2);
    /* 082D mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 082E inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
L082F:
    /* 082F mov.w $0361+x,a */
    spc_st_cyc((uint16_t)(0x0361 + spc.x), spc.a, 5, 1);
    /* 0832 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 0833 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 0834 cmp x,#$10 */
    spc_cmp(spc.x, 0x10);
    spc_cyc(2);
    /* 0836 bne $082F */
    if (!spc.z) { spc_cyc(4); goto L082F; }
    spc_cyc(2);
    /* 0838 mov a,#$60 */
    spc.a = spc_nz(0x60);
    spc_cyc(2);
    /* 083A mov y,#$0C */
    spc.y = spc_nz(0x0C);
    spc_cyc(2);
    /* 083C call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x083F, 5, 1, 2);
    /* 083F mov y,#$1C */
    spc.y = spc_nz(0x1C);
    spc_cyc(2);
    /* 0841 call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x0844, 5, 1, 2);
    /* 0844 mov a,#$3F */
    spc.a = spc_nz(0x3F);
    spc_cyc(2);
    /* 0846 mov y,#$5D */
    spc.y = spc_nz(0x5D);
    spc_cyc(2);
    /* 0848 call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x084B, 5, 1, 2);
    /* 084B mov a,#$30 */
    spc.a = spc_nz(0x30);
    spc_cyc(2);
    /* 084D mov.w $00F1,a */
    spc_st_cyc(0x00F1, spc.a, 4, 1);
    /* 0850 mov a,#$10 */
    spc.a = spc_nz(0x10);
    spc_cyc(2);
    /* 0852 mov.w $00FA,a */
    spc_st_cyc(0x00FA, spc.a, 4, 1);
    /* 0855 mov.b $53,a */
    spc_st_cyc(spc_dp(0x53), spc.a, 3, 1);
    /* 0857 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 0859 mov.w $00F1,a */
    spc_st_cyc(0x00F1, spc.a, 4, 1);
    /* 085C dec a */
    spc.a = spc_dec(spc.a);
    spc_cyc(2);
    /* 085D mov.b $04,a */
    spc_st_cyc(spc_dp(0x04), spc.a, 3, 1);
    /* 085F mov.b $05,a */
    spc_st_cyc(spc_dp(0x05), spc.a, 3, 1);
    /* 0861 mov.b $06,a */
    spc_st_cyc(spc_dp(0x06), spc.a, 3, 1);
    /* 0863 mov.b $07,a */
    spc_st_cyc(spc_dp(0x07), spc.a, 3, 1);
    /* 0865 dec a */
    spc.a = spc_dec(spc.a);
    spc_cyc(2);
    /* 0866 mov.b $46,a */
    spc_st_cyc(spc_dp(0x46), spc.a, 3, 1);
L0868:
    /* 0868 mov y,#$0A */
    spc.y = spc_nz(0x0A);
    spc_cyc(2);
L086A:
    /* 086A cmp y,#$05 */
    spc_cmp(spc.y, 0x05);
    spc_cyc(2);
    /* 086C beq $0875 */
    if (spc.z) { spc_cyc(4); goto L0875; }
    spc_cyc(2);
    /* 086E bcs $0878 */
    if (spc.c) { spc_cyc(4); goto L0878; }
    spc_cyc(2);
    /* 0870 cmp $4C,$4D */
    spc_cyc(3);
    { uint8_t r = spc_rd(spc_dp(0x4D)); spc_cmp((spc_cyc(2), spc_rd(spc_dp(0x4C))), r); }
    spc_cyc(1);
    /* 0873 bne $0886 */
    if (!spc.z) { spc_cyc(4); goto L0886; }
    spc_cyc(2);
L0875:
    /* 0875 bbs $4C.7,$0886 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x4C)) >> 7 & 1)) { spc_cyc(4); goto L0886; }
    spc_cyc(2);
L0878:
    /* 0878 mov.w a,$3F9F+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x3F9F + spc.y)));
    /* 087B mov.w $00F2,a */
    spc_st_cyc(0x00F2, spc.a, 4, 1);
    /* 087E mov.w a,$06C2+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x06C2 + spc.y)));
    /* 0881 mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 0882 mov a,(x) */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(spc.x)));
    /* 0883 mov.w $00F3,a */
    spc_st_cyc(0x00F3, spc.a, 4, 1);
L0886:
    /* 0886 dbnz y,$086A */
    if (--spc.y) { spc_cyc(6); goto L086A; }
    spc_cyc(4);
    /* 0888 mov.b $45,y */
    spc_st_cyc(spc_dp(0x45), spc.y, 3, 1);
    /* 088A mov.b $46,y */
    spc_st_cyc(spc_dp(0x46), spc.y, 3, 1);
    /* 088C mov.b a,$18 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x18)));
    /* 088E eor.b a,$19 */
    spc_cyc(3);
    spc.a = spc_eor(spc.a, spc_rd(spc_dp(0x19)));
    /* 0890 lsr a */
    spc.a = spc_lsr(spc.a);
    spc_cyc(2);
    /* 0891 lsr a */
    spc.a = spc_lsr(spc.a);
    spc_cyc(2);
    /* 0892 notc */
    spc.c ^= 1;
    spc_cyc(3);
    /* 0893 ror.b $18 */
    spc_rmw_cyc(spc_dp(0x18), spc_ror, 3, 1);
    /* 0895 ror.b $19 */
    spc_rmw_cyc(spc_dp(0x19), spc_ror, 3, 1);
L0897:
    /* 0897 mov.w y,$00FD */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x00FD));
    /* 089A beq $0897 */
    if (spc.z) { spc_cyc(4); goto L0897; }
    spc_cyc(2);
    /* 089C push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 089D mov a,#$20 */
    spc.a = spc_nz(0x20);
    spc_cyc(2);
    /* 089F mul ya */
    spc_mul();
    spc_cyc(9);
    /* 08A0 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 08A1 adc.b a,$43 */
    spc_cyc(3);
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0x43)));
    /* 08A3 mov.b $43,a */
    spc_st_cyc(spc_dp(0x43), spc.a, 3, 1);
    /* 08A5 bcc $08B1 */
    if (!spc.c) { spc_cyc(4); goto L08B1; }
    spc_cyc(2);
    /* 08A7 call $10B0 */
    spc_call_cyc(abi_SpcSfxFrame, 0x08AA, 5, 1, 2);
    /* 08AA cmp $4C,$4D */
    spc_cyc(3);
    { uint8_t r = spc_rd(spc_dp(0x4D)); spc_cmp((spc_cyc(2), spc_rd(spc_dp(0x4C))), r); }
    spc_cyc(1);
    /* 08AD beq $08B1 */
    if (spc.z) { spc_cyc(4); goto L08B1; }
    spc_cyc(2);
    /* 08AF inc.b $4C */
    spc_rmw_cyc(spc_dp(0x4C), spc_inc, 3, 1);
L08B1:
    /* 08B1 mov.b a,$53 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x53)));
    /* 08B3 pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 08B4 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 08B5 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 08B6 adc.b a,$51 */
    spc_cyc(3);
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0x51)));
    /* 08B8 mov.b $51,a */
    spc_st_cyc(spc_dp(0x51), spc.a, 3, 1);
    /* 08BA bcc $08C9 */
    if (!spc.c) { spc_cyc(4); goto L08C9; }
    spc_cyc(2);
    /* 08BC call $0A6F */
    spc_call_cyc(abi_SpcMusicFrame, 0x08BF, 5, 1, 2);
    /* 08BF mov x,#$03 */
    spc.x = spc_nz(0x03);
    spc_cyc(2);
L08C1:
    /* 08C1 call $08E2 */
    spc_call_cyc(abi_SpcPortExchange, 0x08C4, 5, 1, 2);
    /* 08C4 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 08C5 bpl $08C1 */
    if (!spc.n) { spc_cyc(4); goto L08C1; }
    spc_cyc(2);
    /* 08C7 bra $0868 */
    spc_cyc(4);
    goto L0868;
L08C9:
    /* 08C9 mov.b a,$1C */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1C)));
    /* 08CB beq $0868 */
    if (spc.z) { spc_cyc(4); goto L0868; }
    spc_cyc(2);
    /* 08CD mov x,#$00 */
    spc.x = spc_nz(0x00);
    spc_cyc(2);
    /* 08CF mov $47,#$01 */
    spc_st_cyc(spc_dp(0x47), 0x01, 4, 1);
L08D2:
    /* 08D2 mov.b a,$31+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x31 + spc.x))));
    /* 08D4 beq $08D9 */
    if (spc.z) { spc_cyc(4); goto L08D9; }
    spc_cyc(2);
    /* 08D6 call $0FD6 */
    spc_call_cyc(abi_SpcVoiceBetweenTicks, 0x08D9, 5, 1, 2);
L08D9:
    /* 08D9 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 08DA inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 08DB asl.b $47 */
    spc_rmw_cyc(spc_dp(0x47), spc_asl, 3, 1);
    /* 08DD bne $08D2 */
    if (!spc.z) { spc_cyc(4); goto L08D2; }
    spc_cyc(2);
    /* 08DF jmp $0868 */
    spc_cyc(3);
    goto L0868;
}

/*
 * @routine 19:C56E SpcPortExchange
 * WHAT IT DOES: Port X's exchange with the S-CPU: the byte $04+X goes out on port X, then the port is read until two reads agree and kept at $0558+X.
 * HOW: MOV to $F4+X, then a read-compare loop on $F4+X.
 * Entry: X = port (0-3). Exit: A = the byte read; flags from the last compare.
 * @spc 08E2
 * @spcuses 08E2-08F3
 */
void abi_SpcPortExchange(void)
{
    /* 08E2 mov.b a,$04+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x04 + spc.x))));
    /* 08E4 mov.w $00F4+x,a */
    spc_st_cyc((uint16_t)(0x00F4 + spc.x), spc.a, 5, 1);
L08E7:
    /* 08E7 mov.w a,$00F4+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x00F4 + spc.x)));
    /* 08EA cmp.w a,$00F4+x */
    spc_cyc(5);
    spc_cmp(spc.a, spc_rd((uint16_t)(0x00F4 + spc.x)));
    /* 08ED bne $08E7 */
    if (!spc.z) { spc_cyc(4); goto L08E7; }
    spc_cyc(2);
    /* 08EF mov.w $0558+x,a */
    spc_st_cyc((uint16_t)(0x0558 + spc.x), spc.a, 5, 1);
    /* 08F2 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:C57F SpcNoteOn
 * WHAT IT DOES: Plays note Y on voice X: percussion notes ($CA and up) first set their instrument (SpcSetInstrument) and play as note $24; ties and rests ($C8, $C9) and voices taken by a sound effect do nothing. Otherwise the pitch is the note plus the global and voice transposes, the tuning goes in, vibrato's phase restarts, vibrato, tremolo and their fades reset, the voice is marked for key-on and a volume update, a pitch envelope (VCMD $EB/$EC) starts if set, then the DSP pitch is written.
 * HOW: The pitch is a note number with a fraction ($0360/$0361); the DSP pitch comes from the 12-entry table at $06CD by octave (DIV by $18) and interpolation, times the instrument's multiplier $0220/$0221, written to VxPITCH through SpcDspWriteVoice. The code after $095D is also the pitch write the vibrato and slide paths jump back to.
 * Entry: X = voice * 2, $47 = the voice's bit, Y = note. Exit: A, Y as the last DSP write left them; X kept.
 * @spc 08F3
 * @spcuses 08F2-09EF,0BBE-0C17,0DDC-0E1B
 */
void abi_SpcNoteOn(void)
{
    /* 08F3 cmp y,#$CA */
    spc_cmp(spc.y, 0xCA);
    spc_cyc(2);
    /* 08F5 bcc $08FC */
    if (!spc.c) { spc_cyc(4); goto L08FC; }
    spc_cyc(2);
    /* 08F7 call $0BBE */
    spc_call_cyc(abi_SpcSetInstrument, 0x08FA, 5, 1, 2);
    /* 08FA mov y,#$A4 */
    spc.y = spc_nz(0xA4);
    spc_cyc(2);
L08FC:
    /* 08FC cmp y,#$C8 */
    spc_cmp(spc.y, 0xC8);
    spc_cyc(2);
    /* 08FE bcs $08F2 */
    if (spc.c) { spc_cyc(4); goto L08F2; }
    spc_cyc(2);
    /* 0900 mov.b a,$1A */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1A)));
    /* 0902 and.b a,$47 */
    spc_cyc(3);
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0x47)));
    /* 0904 bne $08F2 */
    if (!spc.z) { spc_cyc(4); goto L08F2; }
    spc_cyc(2);
    /* 0906 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 0907 and a,#$7F */
    spc.a = spc_and(spc.a, 0x7F);
    spc_cyc(2);
    /* 0909 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 090A adc.b a,$50 */
    spc_cyc(3);
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0x50)));
    /* 090C clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 090D adc.w a,$02F0+x */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x02F0 + spc.x)));
    /* 0910 mov.w $0361+x,a */
    spc_st_cyc((uint16_t)(0x0361 + spc.x), spc.a, 5, 1);
    /* 0913 mov.w a,$0381+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0381 + spc.x)));
    /* 0916 mov.w $0360+x,a */
    spc_st_cyc((uint16_t)(0x0360 + spc.x), spc.a, 5, 1);
    /* 0919 mov.w a,$02B1+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x02B1 + spc.x)));
    /* 091C lsr a */
    spc.a = spc_lsr(spc.a);
    spc_cyc(2);
    /* 091D mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 091F ror a */
    spc.a = spc_ror(spc.a);
    spc_cyc(2);
    /* 0920 mov.w $02A0+x,a */
    spc_st_cyc((uint16_t)(0x02A0 + spc.x), spc.a, 5, 1);
    /* 0923 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0925 mov.b $B0+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0xB0 + spc.x)), spc.a, 4, 1);
    /* 0927 mov.w $0100+x,a */
    spc_st_cyc((uint16_t)(0x0100 + spc.x), spc.a, 5, 1);
    /* 092A mov.w $02D0+x,a */
    spc_st_cyc((uint16_t)(0x02D0 + spc.x), spc.a, 5, 1);
    /* 092D mov.b $C0+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0xC0 + spc.x)), spc.a, 4, 1);
    /* 092F or $5E,$47 */
    spc_cyc(3);
    { uint8_t r = spc_rd(spc_dp(0x47)); uint8_t l = (spc_cyc(2), spc_rd(spc_dp(0x5E))); spc_cyc(1); spc_wr(spc_dp(0x5E), spc_or(l, r)); }
    /* 0932 or $45,$47 */
    spc_cyc(3);
    { uint8_t r = spc_rd(spc_dp(0x47)); uint8_t l = (spc_cyc(2), spc_rd(spc_dp(0x45))); spc_cyc(1); spc_wr(spc_dp(0x45), spc_or(l, r)); }
    /* 0935 mov.w a,$0280+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0280 + spc.x)));
    /* 0938 mov.b $A0+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0xA0 + spc.x)), spc.a, 4, 1);
    /* 093A beq $095A */
    if (spc.z) { spc_cyc(4); goto L095A; }
    spc_cyc(2);
    /* 093C mov.w a,$0281+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0281 + spc.x)));
    /* 093F mov.b $A1+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0xA1 + spc.x)), spc.a, 4, 1);
    /* 0941 mov.w a,$0290+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0290 + spc.x)));
    /* 0944 bne $0950 */
    if (!spc.z) { spc_cyc(4); goto L0950; }
    spc_cyc(2);
    /* 0946 mov.w a,$0361+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0361 + spc.x)));
    /* 0949 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 094A sbc.w a,$0291+x */
    spc_cyc(5);
    spc.a = spc_sbc(spc.a, spc_rd((uint16_t)(0x0291 + spc.x)));
    /* 094D mov.w $0361+x,a */
    spc_st_cyc((uint16_t)(0x0361 + spc.x), spc.a, 5, 1);
L0950:
    /* 0950 mov.w a,$0291+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0291 + spc.x)));
    /* 0953 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 0954 adc.w a,$0361+x */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x0361 + spc.x)));
    /* 0957 call $0DDC */
    spc_call_cyc(abi_SpcSlideToNote, 0x095A, 5, 1, 2);
L095A:
    /* 095A call $0DF4 */
    spc_call_cyc(abi_SpcLoadPitch, 0x095D, 5, 1, 2);
    /* 095D mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 095F mov.b a,$11 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x11)));
    /* 0961 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 0962 sbc a,#$34 */
    spc.a = spc_sbc(spc.a, 0x34);
    spc_cyc(2);
    /* 0964 bcs $096F */
    if (spc.c) { spc_cyc(4); goto L096F; }
    spc_cyc(2);
    /* 0966 mov.b a,$11 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x11)));
    /* 0968 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 0969 sbc a,#$13 */
    spc.a = spc_sbc(spc.a, 0x13);
    spc_cyc(2);
    /* 096B bcs $0973 */
    if (spc.c) { spc_cyc(4); goto L0973; }
    spc_cyc(2);
    /* 096D dec y */
    spc.y = spc_dec(spc.y);
    spc_cyc(2);
    /* 096E asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
L096F:
    /* 096F addw ya,$10 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x10)); v |= (spc_cyc(2), spc_rd(spc_dp(0x11))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 0971 movw $10,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x10));
    spc_cyc(1);
    spc_wr(spc_dp(0x10), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x11), spc.y);
L0973:
    /* 0973 push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 0974 mov.b a,$11 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x11)));
    /* 0976 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 0977 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 0979 mov x,#$18 */
    spc.x = spc_nz(0x18);
    spc_cyc(2);
    /* 097B div ya,x */
    spc_div();
    spc_cyc(12);
    /* 097C mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 097D mov.w a,$06CE+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x06CE + spc.y)));
    /* 0980 mov.b $15,a */
    spc_st_cyc(spc_dp(0x15), spc.a, 3, 1);
    /* 0982 mov.w a,$06CD+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x06CD + spc.y)));
    /* 0985 mov.b $14,a */
    spc_st_cyc(spc_dp(0x14), spc.a, 3, 1);
    /* 0987 mov.w a,$06D0+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x06D0 + spc.y)));
    /* 098A push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 098B mov.w a,$06CF+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x06CF + spc.y)));
    /* 098E pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 098F subw ya,$14 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x14)); v |= (spc_cyc(2), spc_rd(spc_dp(0x15))) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 0991 mov.b y,$10 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x10)));
    /* 0993 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 0994 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 0995 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 0997 addw ya,$14 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x14)); v |= (spc_cyc(2), spc_rd(spc_dp(0x15))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 0999 mov.b $15,y */
    spc_st_cyc(spc_dp(0x15), spc.y, 3, 1);
    /* 099B asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 099C rol.b $15 */
    spc_rmw_cyc(spc_dp(0x15), spc_rol, 3, 1);
    /* 099E mov.b $14,a */
    spc_st_cyc(spc_dp(0x14), spc.a, 3, 1);
    /* 09A0 bra $09A6 */
    spc_cyc(4);
    goto L09A6;
L09A2:
    /* 09A2 lsr.b $15 */
    spc_rmw_cyc(spc_dp(0x15), spc_lsr, 3, 1);
    /* 09A4 ror a */
    spc.a = spc_ror(spc.a);
    spc_cyc(2);
    /* 09A5 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
L09A6:
    /* 09A6 cmp x,#$06 */
    spc_cmp(spc.x, 0x06);
    spc_cyc(2);
    /* 09A8 bne $09A2 */
    if (!spc.z) { spc_cyc(4); goto L09A2; }
    spc_cyc(2);
    /* 09AA mov.b $14,a */
    spc_st_cyc(spc_dp(0x14), spc.a, 3, 1);
    /* 09AC pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 09AD mov.w a,$0220+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0220 + spc.x)));
    /* 09B0 mov.b y,$15 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x15)));
    /* 09B2 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 09B3 movw $16,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x16));
    spc_cyc(1);
    spc_wr(spc_dp(0x16), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x17), spc.y);
    /* 09B5 mov.w a,$0220+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0220 + spc.x)));
    /* 09B8 mov.b y,$14 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x14)));
    /* 09BA mul ya */
    spc_mul();
    spc_cyc(9);
    /* 09BB push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 09BC mov.w a,$0221+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0221 + spc.x)));
    /* 09BF mov.b y,$14 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x14)));
    /* 09C1 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 09C2 addw ya,$16 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x16)); v |= (spc_cyc(2), spc_rd(spc_dp(0x17))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 09C4 movw $16,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x16));
    spc_cyc(1);
    spc_wr(spc_dp(0x16), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x17), spc.y);
    /* 09C6 mov.w a,$0221+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0221 + spc.x)));
    /* 09C9 mov.b y,$15 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x15)));
    /* 09CB mul ya */
    spc_mul();
    spc_cyc(9);
    /* 09CC mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 09CD pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 09CE addw ya,$16 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x16)); v |= (spc_cyc(2), spc_rd(spc_dp(0x17))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 09D0 movw $16,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x16));
    spc_cyc(1);
    spc_wr(spc_dp(0x16), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x17), spc.y);
    /* 09D2 mov a,x */
    spc.a = spc_nz(spc.x);
    spc_cyc(2);
    /* 09D3 xcn a */
    spc.a = spc_xcn(spc.a);
    spc_cyc(5);
    /* 09D4 lsr a */
    spc.a = spc_lsr(spc.a);
    spc_cyc(2);
    /* 09D5 or a,#$02 */
    spc.a = spc_or(spc.a, 0x02);
    spc_cyc(2);
    /* 09D7 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 09D8 mov.b a,$16 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x16)));
    /* 09DA call $09E0 */
    spc_call_cyc(abi_SpcDspWriteVoice, 0x09DD, 5, 1, 2);
    /* 09DD inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 09DE mov.b a,$17 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x17)));
    { abi_SpcDspWriteVoice(); return; }
L08F2:
    /* 08F2 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:C66C SpcDspWriteVoice
 * WHAT IT DOES: Writes A to DSP register Y unless the current voice ($47) belongs to a sound effect ($1A).
 * HOW: A tested against $1A on the stack, then as SpcDspWrite.
 * Entry: Y = DSP register, A = value. Exit: A, Y kept.
 * @spc 09E0
 * @spcuses 09E0-09EF
 */
void abi_SpcDspWriteVoice(void)
{
    /* 09E0 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 09E1 mov.b a,$47 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x47)));
    /* 09E3 and.b a,$1A */
    spc_cyc(3);
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0x1A)));
    /* 09E5 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 09E6 bne $09EE */
    if (!spc.z) { spc_cyc(4); goto L09EE; }
    { spc_cyc(2); abi_SpcDspWrite(); return; }
L09EE:
    /* 09EE ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:C674 SpcDspWrite
 * WHAT IT DOES: Writes A to DSP register Y.
 * HOW: $F2 = Y, $F3 = A.
 * Entry: Y = DSP register, A = value. Exit: A, Y kept.
 * @spc 09E8
 * @spcuses 09E8-09EF
 */
void abi_SpcDspWrite(void)
{
    /* 09E8 mov.w $00F2,y */
    spc_st_cyc(0x00F2, spc.y, 4, 1);
    /* 09EB mov.w $00F3,a */
    spc_st_cyc(0x00F3, spc.a, 4, 1);
    /* 09EE ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:C682 SpcReadPatternWord
 * WHAT IT DOES: Reads the next word of the song's pattern list at $40 and steps past it. The code that follows is reached by branches from SpcMusicFrame: the song start (the request $1B kept as $1C and cleared, the pattern list from the song table $07F4, a two-tick delay $0C, then every voice no sound effect holds keyed off), and the tracks' reset (instrument, volume $FF, pan $0A, vibrato and tremolo off, transposes and fades cleared, master volume $C0, tempo $20).
 * HOW: MOV A,($40)+Y twice with INCW; the word in YA.
 * Entry: none. Exit: YA = the word; Z, N from the high byte.
 * @spc 09F6
 * @spcuses 09F6-0A04
 */
void abi_SpcReadPatternWord(void)
{
    /* 09F6 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 09F8 mov a,($40)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x40)); p |= (spc_cyc(1), spc_rd(spc_dp(0x41))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 09FA incw $40 */
    spc_incw_cyc(0x40, +1, 3, 1, 1, 1);
    /* 09FC push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 09FD mov a,($40)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x40)); p |= (spc_cyc(1), spc_rd(spc_dp(0x41))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 09FF incw $40 */
    spc_incw_cyc(0x40, +1, 3, 1, 1, 1);
    /* 0A01 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0A02 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 0A03 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:C6EC SpcMusicPause
 * WHAT IT DOES: Stops the music once: sets bit 1 of $29 (nothing if already set), clears the song request and the song, and keys off every voice no sound effect holds.
 * HOW: BBS, SET1, then the release code in SpcReadPatternWord's tail (TSET of the free voices into the key-off mask $46).
 * Entry: none. Exit: A = the free voices' mask.
 * @spc 0A60
 * @spcuses 0A3B-0A3C,0A58-0A6D
 */
void abi_SpcMusicPause(void)
{
    /* 0A60 bbs $29.1,$0A3B */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x29)) >> 1 & 1)) { spc_cyc(4); goto L0A3B; }
    spc_cyc(2);
    /* 0A63 set1 $29.1 */
    spc_bit_set_cyc(0x29, 1, 1, 3, 1);
    /* 0A65 mov $1B,#$00 */
    spc_st_cyc(spc_dp(0x1B), 0x00, 4, 1);
    /* 0A68 mov $1C,#$00 */
    spc_st_cyc(spc_dp(0x1C), 0x00, 4, 1);
    /* 0A6B bra $0A58 */
    spc_cyc(4);
    goto L0A58;
L0A3B:
    /* 0A3B ret */
    spc_ret_cyc(4, 1);
    return;
L0A58:
    /* 0A58 mov.b a,$1A */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1A)));
    /* 0A5A eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    spc_cyc(2);
    /* 0A5C tset $0046,a */
    spc_tset_cyc(0x0046, 1, 4, 1, 1);
    /* 0A5F ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:C6FB SpcMusicFrame
 * WHAT IT DOES: One tick of the music: with bit 0 of $29 set it only keys off the voices no sound effect holds. Otherwise a new song request (one not already playing, $0584) starts its song; the pattern list advances when the tracks end (a zero word ends the song, a count word loops), each track's commands are read (note lengths, gate and velocity bytes, voice commands through SpcVcmdDispatch, notes through SpcNoteOn), notes held count down (SpcVoiceTick), pitch slides are checked (SpcSlideCheck); then the tempo, echo volume and master volume fades step and every playing voice's fades and modulation are applied (SpcVoiceEffects).
 * HOW: Track pointers at $30+X, timers $70/$71, subroutine counts $80; the gate and velocity come from the tables $0650 and $0658.
 * Entry: none. Exit: A, X, Y changed.
 * @spc 0A6F
 * @spcuses 08F2-09EF,09F6-0FD6,1047-104F,105C-107A,3FCA-4000
 */
void abi_SpcMusicFrame(void)
{
    /* 0A6F bbs $29.0,$0A58 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x29)) >> 0 & 1)) { spc_cyc(4); goto L0A58; }
    spc_cyc(2);
    /* 0A72 mov.b a,$1B */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1B)));
    /* 0A74 beq $0A7B */
    if (spc.z) { spc_cyc(4); goto L0A7B; }
    spc_cyc(2);
    /* 0A76 cmp.w a,$0584 */
    spc_cyc(4);
    spc_cmp(spc.a, spc_rd(0x0584));
    /* 0A79 bne $0A3E */
    if (!spc.z) { spc_cyc(4); goto L0A3E; }
    spc_cyc(2);
L0A7B:
    /* 0A7B mov.w $0584,a */
    spc_st_cyc(0x0584, spc.a, 4, 1);
    /* 0A7E mov $1B,#$00 */
    spc_st_cyc(spc_dp(0x1B), 0x00, 4, 1);
L0A81:
    /* 0A81 mov.b a,$1C */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1C)));
    /* 0A83 beq $0A3B */
    if (spc.z) { spc_cyc(4); goto L0A3B; }
    spc_cyc(2);
    /* 0A85 mov.b a,$0C */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x0C)));
    /* 0A87 beq $0AD5 */
    if (spc.z) { spc_cyc(4); goto L0AD5; }
    spc_cyc(2);
    /* 0A89 dbnz $0C,$0A6D */
    spc_cyc(3);
    { uint8_t v = (uint8_t)(spc_rd(spc_dp(0x0C)) - 1); spc_cyc(1); spc_wr(spc_dp(0x0C), v); if (v) { spc_cyc(3); goto L0A6D; } }
    spc_cyc(1);
L0A8C:
    /* 0A8C call $09F6 */
    spc_call_cyc(abi_SpcReadPatternWord, 0x0A8F, 5, 1, 2);
    /* 0A8F bne $0AA5 */
    if (!spc.z) { spc_cyc(4); goto L0AA5; }
    spc_cyc(2);
    /* 0A91 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0A92 beq $0A3C */
    if (spc.z) { spc_cyc(4); goto L0A3C; }
    spc_cyc(2);
    /* 0A94 dec.b $42 */
    spc_rmw_cyc(spc_dp(0x42), spc_dec, 3, 1);
    /* 0A96 bpl $0A9A */
    if (!spc.n) { spc_cyc(4); goto L0A9A; }
    spc_cyc(2);
    /* 0A98 mov.b $42,a */
    spc_st_cyc(spc_dp(0x42), spc.a, 3, 1);
L0A9A:
    /* 0A9A call $09F6 */
    spc_call_cyc(abi_SpcReadPatternWord, 0x0A9D, 5, 1, 2);
    /* 0A9D mov.b x,$42 */
    spc_cyc(3);
    spc.x = spc_nz(spc_rd(spc_dp(0x42)));
    /* 0A9F beq $0A8C */
    if (spc.z) { spc_cyc(4); goto L0A8C; }
    spc_cyc(2);
    /* 0AA1 movw $40,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x40));
    spc_cyc(1);
    spc_wr(spc_dp(0x40), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x41), spc.y);
    /* 0AA3 bra $0A8C */
    spc_cyc(4);
    goto L0A8C;
L0AA5:
    /* 0AA5 movw $16,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x16));
    spc_cyc(1);
    spc_wr(spc_dp(0x16), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x17), spc.y);
    /* 0AA7 mov y,#$0F */
    spc.y = spc_nz(0x0F);
    spc_cyc(2);
L0AA9:
    /* 0AA9 mov a,($16)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x16)); p |= (spc_cyc(1), spc_rd(spc_dp(0x17))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 0AAB mov.w $0030+y,a */
    spc_st_cyc((uint16_t)(0x0030 + spc.y), spc.a, 5, 1);
    /* 0AAE dec y */
    spc.y = spc_dec(spc.y);
    spc_cyc(2);
    /* 0AAF bpl $0AA9 */
    if (!spc.n) { spc_cyc(4); goto L0AA9; }
    spc_cyc(2);
    /* 0AB1 mov x,#$00 */
    spc.x = spc_nz(0x00);
    spc_cyc(2);
    /* 0AB3 mov $47,#$01 */
    spc_st_cyc(spc_dp(0x47), 0x01, 4, 1);
L0AB6:
    /* 0AB6 mov.b a,$31+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x31 + spc.x))));
    /* 0AB8 beq $0AC4 */
    if (spc.z) { spc_cyc(4); goto L0AC4; }
    spc_cyc(2);
    /* 0ABA mov.w a,$0211+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0211 + spc.x)));
    /* 0ABD bne $0AC4 */
    if (!spc.z) { spc_cyc(4); goto L0AC4; }
    spc_cyc(2);
    /* 0ABF mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0AC1 call $0BBE */
    spc_call_cyc(abi_SpcSetInstrument, 0x0AC4, 5, 1, 2);
L0AC4:
    /* 0AC4 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0AC6 mov.b $80+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0x80 + spc.x)), spc.a, 4, 1);
    /* 0AC8 mov.b $90+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0x90 + spc.x)), spc.a, 4, 1);
    /* 0ACA mov.b $91+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0x91 + spc.x)), spc.a, 4, 1);
    /* 0ACC inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
    /* 0ACD mov.b $70+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0x70 + spc.x)), spc.a, 4, 1);
    /* 0ACF inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 0AD0 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 0AD1 asl.b $47 */
    spc_rmw_cyc(spc_dp(0x47), spc_asl, 3, 1);
    /* 0AD3 bne $0AB6 */
    if (!spc.z) { spc_cyc(4); goto L0AB6; }
    spc_cyc(2);
L0AD5:
    /* 0AD5 mov x,#$00 */
    spc.x = spc_nz(0x00);
    spc_cyc(2);
    /* 0AD7 mov.b $5E,x */
    spc_st_cyc(spc_dp(0x5E), spc.x, 3, 1);
    /* 0AD9 mov $47,#$01 */
    spc_st_cyc(spc_dp(0x47), 0x01, 4, 1);
L0ADC:
    /* 0ADC mov.b $44,x */
    spc_st_cyc(spc_dp(0x44), spc.x, 3, 1);
    /* 0ADE mov.b a,$31+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x31 + spc.x))));
    /* 0AE0 beq $0B4F */
    if (spc.z) { spc_cyc(4); goto L0B4F; }
    spc_cyc(2);
    /* 0AE2 dec.b $70+x */
    spc_rmw_cyc(spc_dp((uint8_t)(0x70 + spc.x)), spc_dec, 4, 1);
    /* 0AE4 bne $0B49 */
    if (!spc.z) { spc_cyc(4); goto L0B49; }
    spc_cyc(2);
L0AE6:
    /* 0AE6 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0AE9, 5, 1, 2);
    /* 0AE9 bne $0B06 */
    if (!spc.z) { spc_cyc(4); goto L0B06; }
    spc_cyc(2);
    /* 0AEB mov.b a,$80+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x80 + spc.x))));
    /* 0AED beq $0A8C */
    if (spc.z) { spc_cyc(4); goto L0A8C; }
    spc_cyc(2);
    /* 0AEF call $0D05 */
    spc_call_cyc(abi_SpcSubroutineRestart, 0x0AF2, 5, 1, 2);
    /* 0AF2 dec.b $80+x */
    spc_rmw_cyc(spc_dp((uint8_t)(0x80 + spc.x)), spc_dec, 4, 1);
    /* 0AF4 bne $0AE6 */
    if (!spc.z) { spc_cyc(4); goto L0AE6; }
    spc_cyc(2);
    /* 0AF6 mov.w a,$0230+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0230 + spc.x)));
    /* 0AF9 mov.b $30+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0x30 + spc.x)), spc.a, 4, 1);
    /* 0AFB mov.w a,$0231+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0231 + spc.x)));
    /* 0AFE mov.b $31+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0x31 + spc.x)), spc.a, 4, 1);
    /* 0B00 bra $0AE6 */
    spc_cyc(4);
    goto L0AE6;
L0B02:
    /* 0B02 clr1 $2C.0 */
    spc_bit_set_cyc(0x2C, 0, 0, 3, 1);
    /* 0B04 bra $0A8C */
    spc_cyc(4);
    goto L0A8C;
L0B06:
    /* 0B06 bbs $2C.0,$0B02 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2C)) >> 0 & 1)) { spc_cyc(4); goto L0B02; }
    spc_cyc(2);
    /* 0B09 bmi $0B2B */
    if (spc.n) { spc_cyc(4); goto L0B2B; }
    spc_cyc(2);
    /* 0B0B mov.w $0200+x,a */
    spc_st_cyc((uint16_t)(0x0200 + spc.x), spc.a, 5, 1);
    /* 0B0E call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0B11, 5, 1, 2);
    /* 0B11 bmi $0B2B */
    if (spc.n) { spc_cyc(4); goto L0B2B; }
    spc_cyc(2);
    /* 0B13 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 0B14 xcn a */
    spc.a = spc_xcn(spc.a);
    spc_cyc(5);
    /* 0B15 and a,#$07 */
    spc.a = spc_and(spc.a, 0x07);
    spc_cyc(2);
    /* 0B17 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0B18 mov.w a,$0650+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0650 + spc.y)));
    /* 0B1B mov.w $0201+x,a */
    spc_st_cyc((uint16_t)(0x0201 + spc.x), spc.a, 5, 1);
    /* 0B1E pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 0B1F and a,#$0F */
    spc.a = spc_and(spc.a, 0x0F);
    spc_cyc(2);
    /* 0B21 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0B22 mov.w a,$0658+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0658 + spc.y)));
    /* 0B25 mov.w $0210+x,a */
    spc_st_cyc((uint16_t)(0x0210 + spc.x), spc.a, 5, 1);
    /* 0B28 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0B2B, 5, 1, 2);
L0B2B:
    /* 0B2B cmp a,#$E0 */
    spc_cmp(spc.a, 0xE0);
    spc_cyc(2);
    /* 0B2D bcc $0B34 */
    if (!spc.c) { spc_cyc(4); goto L0B34; }
    spc_cyc(2);
    /* 0B2F call $0BA2 */
    spc_call_cyc(abi_SpcVcmdDispatch, 0x0B32, 5, 1, 2);
    /* 0B32 bra $0AE6 */
    spc_cyc(4);
    goto L0AE6;
L0B34:
    /* 0B34 call $08F3 */
    spc_call_cyc(abi_SpcNoteOn, 0x0B37, 5, 1, 2);
    /* 0B37 mov.w a,$0200+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0200 + spc.x)));
    /* 0B3A mov.b $70+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0x70 + spc.x)), spc.a, 4, 1);
    /* 0B3C mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0B3D mov.w a,$0201+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0201 + spc.x)));
    /* 0B40 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 0B41 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 0B42 bne $0B45 */
    if (!spc.z) { spc_cyc(4); goto L0B45; }
    spc_cyc(2);
    /* 0B44 inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
L0B45:
    /* 0B45 mov.b $71+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0x71 + spc.x)), spc.a, 4, 1);
    /* 0B47 bra $0B4C */
    spc_cyc(4);
    goto L0B4C;
L0B49:
    /* 0B49 call $0EE4 */
    spc_call_cyc(abi_SpcVoiceTick, 0x0B4C, 5, 1, 2);
L0B4C:
    /* 0B4C call $0DBC */
    spc_call_cyc(abi_SpcSlideCheck, 0x0B4F, 5, 1, 2);
L0B4F:
    /* 0B4F inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 0B50 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 0B51 asl.b $47 */
    spc_rmw_cyc(spc_dp(0x47), spc_asl, 3, 1);
    /* 0B53 bne $0ADC */
    if (!spc.z) { spc_cyc(4); goto L0ADC; }
    spc_cyc(2);
    /* 0B55 mov.b a,$54 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x54)));
    /* 0B57 beq $0B64 */
    if (spc.z) { spc_cyc(4); goto L0B64; }
    spc_cyc(2);
    /* 0B59 movw ya,$56 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x56)); v |= (spc_cyc(2), spc_rd(spc_dp(0x57))) << 8; spc_set_ya(spc_nz16(v)); }
    /* 0B5B addw ya,$52 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x52)); v |= (spc_cyc(2), spc_rd(spc_dp(0x53))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 0B5D dbnz $54,$0B62 */
    spc_cyc(3);
    { uint8_t v = (uint8_t)(spc_rd(spc_dp(0x54)) - 1); spc_cyc(1); spc_wr(spc_dp(0x54), v); if (v) { spc_cyc(3); goto L0B62; } }
    spc_cyc(1);
    /* 0B60 movw ya,$54 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x54)); v |= (spc_cyc(2), spc_rd(spc_dp(0x55))) << 8; spc_set_ya(spc_nz16(v)); }
L0B62:
    /* 0B62 movw $52,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x52));
    spc_cyc(1);
    spc_wr(spc_dp(0x52), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x53), spc.y);
L0B64:
    /* 0B64 mov.b a,$68 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x68)));
    /* 0B66 beq $0B7D */
    if (spc.z) { spc_cyc(4); goto L0B7D; }
    spc_cyc(2);
    /* 0B68 movw ya,$64 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x64)); v |= (spc_cyc(2), spc_rd(spc_dp(0x65))) << 8; spc_set_ya(spc_nz16(v)); }
    /* 0B6A addw ya,$60 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x60)); v |= (spc_cyc(2), spc_rd(spc_dp(0x61))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 0B6C movw $60,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x60));
    spc_cyc(1);
    spc_wr(spc_dp(0x60), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x61), spc.y);
    /* 0B6E movw ya,$66 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x66)); v |= (spc_cyc(2), spc_rd(spc_dp(0x67))) << 8; spc_set_ya(spc_nz16(v)); }
    /* 0B70 addw ya,$62 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x62)); v |= (spc_cyc(2), spc_rd(spc_dp(0x63))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 0B72 dbnz $68,$0B7B */
    spc_cyc(3);
    { uint8_t v = (uint8_t)(spc_rd(spc_dp(0x68)) - 1); spc_cyc(1); spc_wr(spc_dp(0x68), v); if (v) { spc_cyc(3); goto L0B7B; } }
    spc_cyc(1);
    /* 0B75 movw ya,$68 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x68)); v |= (spc_cyc(2), spc_rd(spc_dp(0x69))) << 8; spc_set_ya(spc_nz16(v)); }
    /* 0B77 movw $60,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x60));
    spc_cyc(1);
    spc_wr(spc_dp(0x60), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x61), spc.y);
    /* 0B79 mov.b y,$6A */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x6A)));
L0B7B:
    /* 0B7B movw $62,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x62));
    spc_cyc(1);
    spc_wr(spc_dp(0x62), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x63), spc.y);
L0B7D:
    /* 0B7D mov.b a,$5A */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x5A)));
    /* 0B7F beq $0B8F */
    if (spc.z) { spc_cyc(4); goto L0B8F; }
    spc_cyc(2);
    /* 0B81 movw ya,$5C */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x5C)); v |= (spc_cyc(2), spc_rd(spc_dp(0x5D))) << 8; spc_set_ya(spc_nz16(v)); }
    /* 0B83 addw ya,$58 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x58)); v |= (spc_cyc(2), spc_rd(spc_dp(0x59))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 0B85 dbnz $5A,$0B8A */
    spc_cyc(3);
    { uint8_t v = (uint8_t)(spc_rd(spc_dp(0x5A)) - 1); spc_cyc(1); spc_wr(spc_dp(0x5A), v); if (v) { spc_cyc(3); goto L0B8A; } }
    spc_cyc(1);
    /* 0B88 movw ya,$5A */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x5A)); v |= (spc_cyc(2), spc_rd(spc_dp(0x5B))) << 8; spc_set_ya(spc_nz16(v)); }
L0B8A:
    /* 0B8A movw $58,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x58));
    spc_cyc(1);
    spc_wr(spc_dp(0x58), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x59), spc.y);
    /* 0B8C mov $5E,#$FF */
    spc_st_cyc(spc_dp(0x5E), 0xFF, 4, 1);
L0B8F:
    /* 0B8F mov x,#$00 */
    spc.x = spc_nz(0x00);
    spc_cyc(2);
    /* 0B91 mov $47,#$01 */
    spc_st_cyc(spc_dp(0x47), 0x01, 4, 1);
L0B94:
    /* 0B94 mov.b a,$31+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x31 + spc.x))));
    /* 0B96 beq $0B9B */
    if (spc.z) { spc_cyc(4); goto L0B9B; }
    spc_cyc(2);
    /* 0B98 call $0E1B */
    spc_call_cyc(abi_SpcVoiceEffects, 0x0B9B, 5, 1, 2);
L0B9B:
    /* 0B9B inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 0B9C inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 0B9D asl.b $47 */
    spc_rmw_cyc(spc_dp(0x47), spc_asl, 3, 1);
    /* 0B9F bne $0B94 */
    if (!spc.z) { spc_cyc(4); goto L0B94; }
    spc_cyc(2);
    /* 0BA1 ret */
    spc_ret_cyc(4, 1);
    return;
L0A04:
    /* 0A04 mov x,#$0E */
    spc.x = spc_nz(0x0E);
    spc_cyc(2);
    /* 0A06 mov $47,#$80 */
    spc_st_cyc(spc_dp(0x47), 0x80, 4, 1);
L0A09:
    /* 0A09 mov a,#$FF */
    spc.a = spc_nz(0xFF);
    spc_cyc(2);
    /* 0A0B mov.w $0301+x,a */
    spc_st_cyc((uint16_t)(0x0301 + spc.x), spc.a, 5, 1);
    /* 0A0E mov a,#$0A */
    spc.a = spc_nz(0x0A);
    spc_cyc(2);
    /* 0A10 call $0C17 */
    spc_call_cyc(abi_SpcSetPan, 0x0A13, 5, 1, 2);
    /* 0A13 mov.w $0211+x,a */
    spc_st_cyc((uint16_t)(0x0211 + spc.x), spc.a, 5, 1);
    /* 0A16 mov.w $0381+x,a */
    spc_st_cyc((uint16_t)(0x0381 + spc.x), spc.a, 5, 1);
    /* 0A19 mov.w $02F0+x,a */
    spc_st_cyc((uint16_t)(0x02F0 + spc.x), spc.a, 5, 1);
    /* 0A1C mov.w $0280+x,a */
    spc_st_cyc((uint16_t)(0x0280 + spc.x), spc.a, 5, 1);
    /* 0A1F mov.b $B1+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0xB1 + spc.x)), spc.a, 4, 1);
    /* 0A21 mov.b $C1+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0xC1 + spc.x)), spc.a, 4, 1);
    /* 0A23 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 0A24 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 0A25 lsr.b $47 */
    spc_rmw_cyc(spc_dp(0x47), spc_lsr, 3, 1);
    /* 0A27 bne $0A09 */
    if (!spc.z) { spc_cyc(4); goto L0A09; }
    spc_cyc(2);
    /* 0A29 mov.b $5A,a */
    spc_st_cyc(spc_dp(0x5A), spc.a, 3, 1);
    /* 0A2B mov.b $68,a */
    spc_st_cyc(spc_dp(0x68), spc.a, 3, 1);
    /* 0A2D mov.b $54,a */
    spc_st_cyc(spc_dp(0x54), spc.a, 3, 1);
    /* 0A2F mov.b $50,a */
    spc_st_cyc(spc_dp(0x50), spc.a, 3, 1);
    /* 0A31 mov.b $42,a */
    spc_st_cyc(spc_dp(0x42), spc.a, 3, 1);
    /* 0A33 mov.b $5F,a */
    spc_st_cyc(spc_dp(0x5F), spc.a, 3, 1);
    /* 0A35 mov $59,#$C0 */
    spc_st_cyc(spc_dp(0x59), 0xC0, 4, 1);
    /* 0A38 mov $53,#$20 */
    spc_st_cyc(spc_dp(0x53), 0x20, 4, 1);
L0A3B:
    /* 0A3B ret */
    spc_ret_cyc(4, 1);
    return;
L0A3C:
    /* 0A3C bra $0A60 */
    { spc_cyc(4); abi_SpcMusicPause(); return; }
L0A3E:
    /* 0A3E mov.w $0584,a */
    spc_st_cyc(0x0584, spc.a, 4, 1);
    /* 0A41 mov.b a,$1B */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1B)));
    /* 0A43 beq $0A81 */
    if (spc.z) { spc_cyc(4); goto L0A81; }
    spc_cyc(2);
    /* 0A45 mov.b $1C,a */
    spc_st_cyc(spc_dp(0x1C), spc.a, 3, 1);
    /* 0A47 mov $1B,#$00 */
    spc_st_cyc(spc_dp(0x1B), 0x00, 4, 1);
    /* 0A4A asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 0A4B mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 0A4C mov.w a,$07F5+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x07F5 + spc.x)));
    /* 0A4F mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0A50 mov.w a,$07F4+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x07F4 + spc.x)));
    /* 0A53 movw $40,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x40));
    spc_cyc(1);
    spc_wr(spc_dp(0x40), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x41), spc.y);
    /* 0A55 mov $0C,#$02 */
    spc_st_cyc(spc_dp(0x0C), 0x02, 4, 1);
L0A58:
    /* 0A58 mov.b a,$1A */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1A)));
    /* 0A5A eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    spc_cyc(2);
    /* 0A5C tset $0046,a */
    spc_tset_cyc(0x0046, 1, 4, 1, 1);
    /* 0A5F ret */
    spc_ret_cyc(4, 1);
    return;
L0A6D:
    /* 0A6D bra $0A04 */
    spc_cyc(4);
    goto L0A04;
}

/*
 * @routine 19:C82E SpcVcmdDispatch
 * WHAT IT DOES: Runs voice command A ($E0-$FA) for voice X: its handler from the table at $3F0A (by the command), with its first parameter byte read when the parameter count at $102F says there is one.
 * HOW: The handler's address minus one is pushed, then the RET at the end of SpcTrackRead (or SpcTrackAdvance's tail) enters it; the handlers follow SpcSetPan's code (SpcVcmdPanFade and on).
 * Entry: A = command, X = voice * 2. Exit: as the handler leaves it.
 * @spc 0BA2
 * @spcuses 09E8-09EF,0BA2-0BBE,0C25-0DBC,0DCC-0DF4,0DFF-0E1B,3FCA-4000
 */
void abi_SpcVcmdDispatch(void)
{
    /* 0BA2 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 0BA3 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0BA4 mov.w a,$3F0B+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x3F0B + spc.y)));
    /* 0BA7 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 0BA8 mov.w a,$3F0A+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x3F0A + spc.y)));
    /* 0BAB push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 0BAC mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 0BAD lsr a */
    spc.a = spc_lsr(spc.a);
    spc_cyc(2);
    /* 0BAE mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0BAF mov.w a,$102F+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x102F + spc.y)));
    /* 0BB2 beq $0BBC */
    if (spc.z) { spc_cyc(4); goto L0BBC; }
    spc_cyc(2);
    /* 0BB4 mov a,($30+x) */
    spc_cyc(4);
    { uint16_t p = spc_rd(spc_dp((uint8_t)(0x30 + spc.x))); p |= (spc_cyc(1), spc_rd(spc_dp((uint8_t)(0x30 + spc.x + 1)))) << 8;
      spc_cyc(1);
      spc.a = spc_nz(spc_rd(p)); }
    /* 0BB6 inc.b $30+x */
    spc_rmw_cyc(spc_dp((uint8_t)(0x30 + spc.x)), spc_inc, 4, 1);
    /* 0BB8 bne $0BBC */
    if (!spc.z) { spc_cyc(4); goto L0BBC; }
    spc_cyc(2);
    /* 0BBA inc.b $31+x */
    spc_rmw_cyc(spc_dp((uint8_t)(0x31 + spc.x)), spc_inc, 4, 1);
L0BBC:
    /* 0BBC mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0BBD ret */
    switch (spc_ret_cyc(4, 1)) {
    case 0x0BBE: { abi_SpcSetInstrument(); return; }
    case 0x0C17: { abi_SpcSetPan(); return; }
    case 0x0C25: goto L0C25;
    case 0x0C3E: goto L0C3E;
    case 0x0C4A: goto L0C4A;
    case 0x0C55: goto L0C55;
    case 0x0C65: goto L0C65;
    case 0x0C6A: goto L0C6A;
    case 0x0C7C: goto L0C7C;
    case 0x0C81: goto L0C81;
    case 0x0C93: goto L0C93;
    case 0x0C96: goto L0C96;
    case 0x0C9A: goto L0C9A;
    case 0x0CA6: goto L0CA6;
    case 0x0CA9: goto L0CA9;
    case 0x0CAD: goto L0CAD;
    case 0x0CC3: goto L0CC3;
    case 0x0CC7: goto L0CC7;
    case 0x0CD0: goto L0CD0;
    case 0x0CE9: goto L0CE9;
    case 0x0CED: goto L0CED;
    case 0x0D10: goto L0D10;
    case 0x0D29: goto L0D29;
    case 0x0D4A: goto L0D4A;
    case 0x0D51: goto L0D51;
    case 0x0DB9: goto L0DB9;
    case 0x0DCC: goto L0DCC;
    }
    return;
L0C25:
    /* 0C25 mov.b $91+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0x91 + spc.x)), spc.a, 4, 1);
    /* 0C27 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 0C28 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0C2B, 5, 1, 2);
    /* 0C2B mov.w $0350+x,a */
    spc_st_cyc((uint16_t)(0x0350 + spc.x), spc.a, 5, 1);
    /* 0C2E setc */
    spc.c = 1;
    spc_cyc(2);
    /* 0C2F sbc.w a,$0331+x */
    spc_cyc(5);
    spc.a = spc_sbc(spc.a, spc_rd((uint16_t)(0x0331 + spc.x)));
    /* 0C32 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 0C33 call $0DFF */
    spc_call_cyc(abi_SpcFadeDelta, 0x0C36, 5, 1, 2);
    /* 0C36 mov.w $0340+x,a */
    spc_st_cyc((uint16_t)(0x0340 + spc.x), spc.a, 5, 1);
    /* 0C39 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 0C3A mov.w $0341+x,a */
    spc_st_cyc((uint16_t)(0x0341 + spc.x), spc.a, 5, 1);
    /* 0C3D ret */
    spc_ret_cyc(4, 1);
    return;
L0C3E:
    /* 0C3E mov.w $02B0+x,a */
    spc_st_cyc((uint16_t)(0x02B0 + spc.x), spc.a, 5, 1);
    /* 0C41 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0C44, 5, 1, 2);
    /* 0C44 mov.w $02A1+x,a */
    spc_st_cyc((uint16_t)(0x02A1 + spc.x), spc.a, 5, 1);
    /* 0C47 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0C4A, 5, 1, 2);
L0C4A:
    /* 0C4A mov.b $B1+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0xB1 + spc.x)), spc.a, 4, 1);
    /* 0C4C mov.w $02C1+x,a */
    spc_st_cyc((uint16_t)(0x02C1 + spc.x), spc.a, 5, 1);
    /* 0C4F mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0C51 mov.w $02B1+x,a */
    spc_st_cyc((uint16_t)(0x02B1 + spc.x), spc.a, 5, 1);
    /* 0C54 ret */
    spc_ret_cyc(4, 1);
    return;
L0C55:
    /* 0C55 mov.w $02B1+x,a */
    spc_st_cyc((uint16_t)(0x02B1 + spc.x), spc.a, 5, 1);
    /* 0C58 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 0C59 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 0C5B mov.b a,$B1+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0xB1 + spc.x))));
    /* 0C5D pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 0C5E div ya,x */
    spc_div();
    spc_cyc(12);
    /* 0C5F mov.b x,$44 */
    spc_cyc(3);
    spc.x = spc_nz(spc_rd(spc_dp(0x44)));
    /* 0C61 mov.w $02C0+x,a */
    spc_st_cyc((uint16_t)(0x02C0 + spc.x), spc.a, 5, 1);
    /* 0C64 ret */
    spc_ret_cyc(4, 1);
    return;
L0C65:
    /* 0C65 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0C67 movw $58,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x58));
    spc_cyc(1);
    spc_wr(spc_dp(0x58), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x59), spc.y);
    /* 0C69 ret */
    spc_ret_cyc(4, 1);
    return;
L0C6A:
    /* 0C6A mov.b $5A,a */
    spc_st_cyc(spc_dp(0x5A), spc.a, 3, 1);
    /* 0C6C call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0C6F, 5, 1, 2);
    /* 0C6F mov.b $5B,a */
    spc_st_cyc(spc_dp(0x5B), spc.a, 3, 1);
    /* 0C71 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 0C72 sbc.b a,$59 */
    spc_cyc(3);
    spc.a = spc_sbc(spc.a, spc_rd(spc_dp(0x59)));
    /* 0C74 mov.b x,$5A */
    spc_cyc(3);
    spc.x = spc_nz(spc_rd(spc_dp(0x5A)));
    /* 0C76 call $0DFF */
    spc_call_cyc(abi_SpcFadeDelta, 0x0C79, 5, 1, 2);
    /* 0C79 movw $5C,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x5C));
    spc_cyc(1);
    spc_wr(spc_dp(0x5C), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x5D), spc.y);
    /* 0C7B ret */
    spc_ret_cyc(4, 1);
    return;
L0C7C:
    /* 0C7C mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0C7E movw $52,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x52));
    spc_cyc(1);
    spc_wr(spc_dp(0x52), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x53), spc.y);
    /* 0C80 ret */
    spc_ret_cyc(4, 1);
    return;
L0C81:
    /* 0C81 mov.b $54,a */
    spc_st_cyc(spc_dp(0x54), spc.a, 3, 1);
    /* 0C83 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0C86, 5, 1, 2);
    /* 0C86 mov.b $55,a */
    spc_st_cyc(spc_dp(0x55), spc.a, 3, 1);
    /* 0C88 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 0C89 sbc.b a,$53 */
    spc_cyc(3);
    spc.a = spc_sbc(spc.a, spc_rd(spc_dp(0x53)));
    /* 0C8B mov.b x,$54 */
    spc_cyc(3);
    spc.x = spc_nz(spc_rd(spc_dp(0x54)));
    /* 0C8D call $0DFF */
    spc_call_cyc(abi_SpcFadeDelta, 0x0C90, 5, 1, 2);
    /* 0C90 movw $56,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x56));
    spc_cyc(1);
    spc_wr(spc_dp(0x56), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x57), spc.y);
    /* 0C92 ret */
    spc_ret_cyc(4, 1);
    return;
L0C93:
    /* 0C93 mov.b $50,a */
    spc_st_cyc(spc_dp(0x50), spc.a, 3, 1);
    /* 0C95 ret */
    spc_ret_cyc(4, 1);
    return;
L0C96:
    /* 0C96 mov.w $02F0+x,a */
    spc_st_cyc((uint16_t)(0x02F0 + spc.x), spc.a, 5, 1);
    /* 0C99 ret */
    spc_ret_cyc(4, 1);
    return;
L0C9A:
    /* 0C9A mov.w $02E0+x,a */
    spc_st_cyc((uint16_t)(0x02E0 + spc.x), spc.a, 5, 1);
    /* 0C9D call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0CA0, 5, 1, 2);
    /* 0CA0 mov.w $02D1+x,a */
    spc_st_cyc((uint16_t)(0x02D1 + spc.x), spc.a, 5, 1);
    /* 0CA3 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0CA6, 5, 1, 2);
L0CA6:
    /* 0CA6 mov.b $C1+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0xC1 + spc.x)), spc.a, 4, 1);
    /* 0CA8 ret */
    spc_ret_cyc(4, 1);
    return;
L0CA9:
    /* 0CA9 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 0CAB bra $0CAF */
    spc_cyc(4);
    goto L0CAF;
L0CAD:
    /* 0CAD mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
L0CAF:
    /* 0CAF mov.w $0290+x,a */
    spc_st_cyc((uint16_t)(0x0290 + spc.x), spc.a, 5, 1);
    /* 0CB2 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 0CB3 mov.w $0281+x,a */
    spc_st_cyc((uint16_t)(0x0281 + spc.x), spc.a, 5, 1);
    /* 0CB6 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0CB9, 5, 1, 2);
    /* 0CB9 mov.w $0280+x,a */
    spc_st_cyc((uint16_t)(0x0280 + spc.x), spc.a, 5, 1);
    /* 0CBC call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0CBF, 5, 1, 2);
    /* 0CBF mov.w $0291+x,a */
    spc_st_cyc((uint16_t)(0x0291 + spc.x), spc.a, 5, 1);
    /* 0CC2 ret */
    spc_ret_cyc(4, 1);
    return;
L0CC3:
    /* 0CC3 mov.w $0280+x,a */
    spc_st_cyc((uint16_t)(0x0280 + spc.x), spc.a, 5, 1);
    /* 0CC6 ret */
    spc_ret_cyc(4, 1);
    return;
L0CC7:
    /* 0CC7 mov.w $0301+x,a */
    spc_st_cyc((uint16_t)(0x0301 + spc.x), spc.a, 5, 1);
    /* 0CCA mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0CCC mov.w $0300+x,a */
    spc_st_cyc((uint16_t)(0x0300 + spc.x), spc.a, 5, 1);
    /* 0CCF ret */
    spc_ret_cyc(4, 1);
    return;
L0CD0:
    /* 0CD0 mov.b $90+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0x90 + spc.x)), spc.a, 4, 1);
    /* 0CD2 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 0CD3 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0CD6, 5, 1, 2);
    /* 0CD6 mov.w $0320+x,a */
    spc_st_cyc((uint16_t)(0x0320 + spc.x), spc.a, 5, 1);
    /* 0CD9 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 0CDA sbc.w a,$0301+x */
    spc_cyc(5);
    spc.a = spc_sbc(spc.a, spc_rd((uint16_t)(0x0301 + spc.x)));
    /* 0CDD pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 0CDE call $0DFF */
    spc_call_cyc(abi_SpcFadeDelta, 0x0CE1, 5, 1, 2);
    /* 0CE1 mov.w $0310+x,a */
    spc_st_cyc((uint16_t)(0x0310 + spc.x), spc.a, 5, 1);
    /* 0CE4 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 0CE5 mov.w $0311+x,a */
    spc_st_cyc((uint16_t)(0x0311 + spc.x), spc.a, 5, 1);
    /* 0CE8 ret */
    spc_ret_cyc(4, 1);
    return;
L0CE9:
    /* 0CE9 mov.w $0381+x,a */
    spc_st_cyc((uint16_t)(0x0381 + spc.x), spc.a, 5, 1);
    /* 0CEC ret */
    spc_ret_cyc(4, 1);
    return;
L0CED:
    /* 0CED mov.w $0240+x,a */
    spc_st_cyc((uint16_t)(0x0240 + spc.x), spc.a, 5, 1);
    /* 0CF0 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0CF3, 5, 1, 2);
    /* 0CF3 mov.w $0241+x,a */
    spc_st_cyc((uint16_t)(0x0241 + spc.x), spc.a, 5, 1);
    /* 0CF6 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0CF9, 5, 1, 2);
    /* 0CF9 mov.b $80+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0x80 + spc.x)), spc.a, 4, 1);
    /* 0CFB mov.b a,$30+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x30 + spc.x))));
    /* 0CFD mov.w $0230+x,a */
    spc_st_cyc((uint16_t)(0x0230 + spc.x), spc.a, 5, 1);
    /* 0D00 mov.b a,$31+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x31 + spc.x))));
    /* 0D02 mov.w $0231+x,a */
    spc_st_cyc((uint16_t)(0x0231 + spc.x), spc.a, 5, 1);
    { abi_SpcSubroutineRestart(); return; }
L0D10:
    /* 0D10 mov.b $4F,a */
    spc_st_cyc(spc_dp(0x4F), spc.a, 3, 1);
    /* 0D12 and.b a,$1A */
    spc_cyc(3);
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0x1A)));
    /* 0D14 eor.b a,$4F */
    spc_cyc(3);
    spc.a = spc_eor(spc.a, spc_rd(spc_dp(0x4F)));
    /* 0D16 mov.b $4A,a */
    spc_st_cyc(spc_dp(0x4A), spc.a, 3, 1);
    /* 0D18 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0D1B, 5, 1, 2);
    /* 0D1B mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0D1D movw $60,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x60));
    spc_cyc(1);
    spc_wr(spc_dp(0x60), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x61), spc.y);
    /* 0D1F call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0D22, 5, 1, 2);
    /* 0D22 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0D24 movw $62,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x62));
    spc_cyc(1);
    spc_wr(spc_dp(0x62), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x63), spc.y);
    /* 0D26 clr1 $48.5 */
    spc_bit_set_cyc(0x48, 5, 0, 3, 1);
    /* 0D28 ret */
    spc_ret_cyc(4, 1);
    return;
L0D29:
    /* 0D29 mov.b $68,a */
    spc_st_cyc(spc_dp(0x68), spc.a, 3, 1);
    /* 0D2B call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0D2E, 5, 1, 2);
    /* 0D2E mov.b $69,a */
    spc_st_cyc(spc_dp(0x69), spc.a, 3, 1);
    /* 0D30 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 0D31 sbc.b a,$61 */
    spc_cyc(3);
    spc.a = spc_sbc(spc.a, spc_rd(spc_dp(0x61)));
    /* 0D33 mov.b x,$68 */
    spc_cyc(3);
    spc.x = spc_nz(spc_rd(spc_dp(0x68)));
    /* 0D35 call $0DFF */
    spc_call_cyc(abi_SpcFadeDelta, 0x0D38, 5, 1, 2);
    /* 0D38 movw $64,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x64));
    spc_cyc(1);
    spc_wr(spc_dp(0x64), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x65), spc.y);
    /* 0D3A call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0D3D, 5, 1, 2);
    /* 0D3D mov.b $6A,a */
    spc_st_cyc(spc_dp(0x6A), spc.a, 3, 1);
    /* 0D3F setc */
    spc.c = 1;
    spc_cyc(2);
    /* 0D40 sbc.b a,$63 */
    spc_cyc(3);
    spc.a = spc_sbc(spc.a, spc_rd(spc_dp(0x63)));
    /* 0D42 mov.b x,$68 */
    spc_cyc(3);
    spc.x = spc_nz(spc_rd(spc_dp(0x68)));
    /* 0D44 call $0DFF */
    spc_call_cyc(abi_SpcFadeDelta, 0x0D47, 5, 1, 2);
    /* 0D47 movw $66,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x66));
    spc_cyc(1);
    spc_wr(spc_dp(0x66), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x67), spc.y);
    /* 0D49 ret */
    spc_ret_cyc(4, 1);
    return;
L0D4A:
    /* 0D4A movw $60,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x60));
    spc_cyc(1);
    spc_wr(spc_dp(0x60), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x61), spc.y);
    /* 0D4C movw $62,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x62));
    spc_cyc(1);
    spc_wr(spc_dp(0x62), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x63), spc.y);
    /* 0D4E set1 $48.5 */
    spc_bit_set_cyc(0x48, 5, 1, 3, 1);
    /* 0D50 ret */
    spc_ret_cyc(4, 1);
    return;
L0D51:
    /* 0D51 call $0D73 */
    spc_call_cyc(abi_SpcSetEchoDelay, 0x0D54, 5, 1, 2);
    /* 0D54 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0D57, 5, 1, 2);
    /* 0D57 mov.b $4E,a */
    spc_st_cyc(spc_dp(0x4E), spc.a, 3, 1);
    /* 0D59 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0D5C, 5, 1, 2);
    /* 0D5C mov y,#$08 */
    spc.y = spc_nz(0x08);
    spc_cyc(2);
    /* 0D5E mul ya */
    spc_mul();
    spc_cyc(9);
    /* 0D5F mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 0D60 mov y,#$0F */
    spc.y = spc_nz(0x0F);
    spc_cyc(2);
L0D62:
    /* 0D62 mov.w a,$3FAA+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x3FAA + spc.x)));
    /* 0D65 call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x0D68, 5, 1, 2);
    /* 0D68 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 0D69 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 0D6A clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 0D6B adc a,#$10 */
    spc.a = spc_adc(spc.a, 0x10);
    spc_cyc(2);
    /* 0D6D mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0D6E bpl $0D62 */
    if (!spc.n) { spc_cyc(4); goto L0D62; }
    spc_cyc(2);
    /* 0D70 mov.b x,$44 */
    spc_cyc(3);
    spc.x = spc_nz(spc_rd(spc_dp(0x44)));
    /* 0D72 ret */
    spc_ret_cyc(4, 1);
    return;
L0DB9:
    /* 0DB9 mov.b $5F,a */
    spc_st_cyc(spc_dp(0x5F), spc.a, 3, 1);
    /* 0DBB ret */
    spc_ret_cyc(4, 1);
    return;
L0DCC:
    /* 0DCC mov.b $A1+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0xA1 + spc.x)), spc.a, 4, 1);
    /* 0DCE call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0DD1, 5, 1, 2);
    /* 0DD1 mov.b $A0+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0xA0 + spc.x)), spc.a, 4, 1);
    /* 0DD3 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0DD6, 5, 1, 2);
    /* 0DD6 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 0DD7 adc.b a,$50 */
    spc_cyc(3);
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0x50)));
    /* 0DD9 adc.w a,$02F0+x */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x02F0 + spc.x)));
    { abi_SpcSlideToNote(); return; }
}

/*
 * @routine 19:C840 SpcTrackRead
 * WHAT IT DOES: Reads the next byte of voice X's track and steps the pointer.
 * HOW: MOV A,($30+X), then SpcTrackAdvance.
 * Entry: X = voice * 2. Exit: A = Y = the byte; N, Z from it.
 * @spc 0BB4
 * @spcuses 0BB4-0BBE
 */
void abi_SpcTrackRead(void)
{
    /* 0BB4 mov a,($30+x) */
    spc_cyc(4);
    { uint16_t p = spc_rd(spc_dp((uint8_t)(0x30 + spc.x))); p |= (spc_cyc(1), spc_rd(spc_dp((uint8_t)(0x30 + spc.x + 1)))) << 8;
      spc_cyc(1);
      spc.a = spc_nz(spc_rd(p)); }
    { abi_SpcTrackAdvance(); return; }
}

/*
 * @routine 19:C842 SpcTrackAdvance
 * WHAT IT DOES: Steps voice X's track pointer by one, keeping A.
 * HOW: INC of the pointer's low byte, carrying into the high byte.
 * Entry: X = voice * 2. Exit: Y = A.
 * @spc 0BB6
 * @spcuses 0BB6-0BBE
 */
void abi_SpcTrackAdvance(void)
{
    /* 0BB6 inc.b $30+x */
    spc_rmw_cyc(spc_dp((uint8_t)(0x30 + spc.x)), spc_inc, 4, 1);
    /* 0BB8 bne $0BBC */
    if (!spc.z) { spc_cyc(4); goto L0BBC; }
    spc_cyc(2);
    /* 0BBA inc.b $31+x */
    spc_rmw_cyc(spc_dp((uint8_t)(0x31 + spc.x)), spc_inc, 4, 1);
L0BBC:
    /* 0BBC mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0BBD ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:C84A SpcSetInstrument
 * WHAT IT DOES: Voice command $E0: voice X's instrument A is kept at $0211+X and loaded (SpcLoadInstrument).
 * HOW: Falls into SpcLoadInstrument.
 * Entry: A = instrument, X = voice * 2. Exit: as SpcLoadInstrument.
 * @spc 0BBE
 * @spcuses 0BBE-0C17
 */
void abi_SpcSetInstrument(void)
{
    /* 0BBE mov.w $0211+x,a */
    spc_st_cyc((uint16_t)(0x0211 + spc.x), spc.a, 5, 1);
    { abi_SpcLoadInstrument(); return; }
}

/*
 * @routine 19:C84D SpcLoadInstrument
 * WHAT IT DOES: Loads instrument A into voice X: its six bytes at $0590 + 6 * A (percussion instruments from $CA on map from the percussion base $5F): the sample number, ADSR and gain to the voice's DSP registers (a sample number with bit 7 set selects noise with its low five bits as the noise clock instead), and the pitch multiplier to $0220/$0221. Nothing for a voice a sound effect holds.
 * HOW: The DSP registers VxSRCN-VxGAIN by an index loop; noise through $48 (FLG) and $49.
 * Entry: A = instrument, X = voice * 2, $47 = its bit. Exit: X kept.
 * @spc 0BC1
 * @spcuses 0BC1-0C17
 */
void abi_SpcLoadInstrument(void)
{
    /* 0BC1 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0BC2 bpl $0BCA */
    if (!spc.n) { spc_cyc(4); goto L0BCA; }
    spc_cyc(2);
    /* 0BC4 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 0BC5 sbc a,#$CA */
    spc.a = spc_sbc(spc.a, 0xCA);
    spc_cyc(2);
    /* 0BC7 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 0BC8 adc.b a,$5F */
    spc_cyc(3);
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0x5F)));
L0BCA:
    /* 0BCA mov y,#$06 */
    spc.y = spc_nz(0x06);
    spc_cyc(2);
    /* 0BCC mul ya */
    spc_mul();
    spc_cyc(9);
    /* 0BCD movw $14,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x14));
    spc_cyc(1);
    spc_wr(spc_dp(0x14), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x15), spc.y);
    /* 0BCF clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 0BD0 adc $14,#$90 */
    spc_cyc(4);
    spc_wr_cyc(spc_dp(0x14), spc_adc(spc_rd(spc_dp(0x14)), 0x90), 1);
    /* 0BD3 adc $15,#$05 */
    spc_cyc(4);
    spc_wr_cyc(spc_dp(0x15), spc_adc(spc_rd(spc_dp(0x15)), 0x05), 1);
    /* 0BD6 mov.b a,$1A */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1A)));
    /* 0BD8 and.b a,$47 */
    spc_cyc(3);
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0x47)));
    /* 0BDA bne $0C16 */
    if (!spc.z) { spc_cyc(4); goto L0C16; }
    spc_cyc(2);
    /* 0BDC push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 0BDD mov a,x */
    spc.a = spc_nz(spc.x);
    spc_cyc(2);
    /* 0BDE xcn a */
    spc.a = spc_xcn(spc.a);
    spc_cyc(5);
    /* 0BDF lsr a */
    spc.a = spc_lsr(spc.a);
    spc_cyc(2);
    /* 0BE0 or a,#$04 */
    spc.a = spc_or(spc.a, 0x04);
    spc_cyc(2);
    /* 0BE2 mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 0BE3 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 0BE5 mov a,($14)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x14)); p |= (spc_cyc(1), spc_rd(spc_dp(0x15))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 0BE7 bpl $0BF7 */
    if (!spc.n) { spc_cyc(4); goto L0BF7; }
    spc_cyc(2);
    /* 0BE9 and a,#$1F */
    spc.a = spc_and(spc.a, 0x1F);
    spc_cyc(2);
    /* 0BEB and $48,#$20 */
    spc_cyc(4);
    spc_wr_cyc(spc_dp(0x48), spc_and(spc_rd(spc_dp(0x48)), 0x20), 1);
    /* 0BEE tset $0048,a */
    spc_tset_cyc(0x0048, 1, 4, 1, 1);
    /* 0BF1 or $49,$47 */
    spc_cyc(3);
    { uint8_t r = spc_rd(spc_dp(0x47)); uint8_t l = (spc_cyc(2), spc_rd(spc_dp(0x49))); spc_cyc(1); spc_wr(spc_dp(0x49), spc_or(l, r)); }
    /* 0BF4 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 0BF5 bra $0BFE */
    spc_cyc(4);
    goto L0BFE;
L0BF7:
    /* 0BF7 mov.b a,$47 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x47)));
    /* 0BF9 tclr $0049,a */
    spc_tset_cyc(0x0049, 0, 4, 1, 1);
L0BFC:
    /* 0BFC mov a,($14)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x14)); p |= (spc_cyc(1), spc_rd(spc_dp(0x15))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
L0BFE:
    /* 0BFE mov.w $00F2,x */
    spc_st_cyc(0x00F2, spc.x, 4, 1);
    /* 0C01 mov.w $00F3,a */
    spc_st_cyc(0x00F3, spc.a, 4, 1);
    /* 0C04 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 0C05 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 0C06 cmp y,#$04 */
    spc_cmp(spc.y, 0x04);
    spc_cyc(2);
    /* 0C08 bne $0BFC */
    if (!spc.z) { spc_cyc(4); goto L0BFC; }
    spc_cyc(2);
    /* 0C0A pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 0C0B mov a,($14)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x14)); p |= (spc_cyc(1), spc_rd(spc_dp(0x15))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 0C0D mov.w $0221+x,a */
    spc_st_cyc((uint16_t)(0x0221 + spc.x), spc.a, 5, 1);
    /* 0C10 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 0C11 mov a,($14)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x14)); p |= (spc_cyc(1), spc_rd(spc_dp(0x15))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 0C13 mov.w $0220+x,a */
    spc_st_cyc((uint16_t)(0x0220 + spc.x), spc.a, 5, 1);
L0C16:
    /* 0C16 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:C8A3 SpcSetPan
 * WHAT IT DOES: Voice command $E1: voice X's pan A (bits 0-4 the position, bits 6-7 inverting either side) is kept and its fade cleared. The voice command handlers follow it: pan fade, vibrato on/off/fade, master volume and fade, tempo and fade, transposes, tremolo on/off, the pitch envelope, volume and fade, tuning, subroutine calls, echo volume, fade, off and setup.
 * HOW: Stores to $0351, $0331 and $0330.
 * Entry: A = pan, X = voice * 2. Exit: A = 0.
 * @spc 0C17
 * @spcuses 0C17-0C25
 */
void abi_SpcSetPan(void)
{
    /* 0C17 mov.w $0351+x,a */
    spc_st_cyc((uint16_t)(0x0351 + spc.x), spc.a, 5, 1);
    /* 0C1A and a,#$1F */
    spc.a = spc_and(spc.a, 0x1F);
    spc_cyc(2);
    /* 0C1C mov.w $0331+x,a */
    spc_st_cyc((uint16_t)(0x0331 + spc.x), spc.a, 5, 1);
    /* 0C1F mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0C21 mov.w $0330+x,a */
    spc_st_cyc((uint16_t)(0x0330 + spc.x), spc.a, 5, 1);
    /* 0C24 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:C991 SpcSubroutineRestart
 * WHAT IT DOES: Points voice X's track back at its subroutine's start ($0240/$0241), for a repeat.
 * HOW: Two byte copies.
 * Entry: X = voice * 2. Exit: A = the start's high byte.
 * @spc 0D05
 * @spcuses 0D05-0D10
 */
void abi_SpcSubroutineRestart(void)
{
    /* 0D05 mov.w a,$0240+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0240 + spc.x)));
    /* 0D08 mov.b $30+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0x30 + spc.x)), spc.a, 4, 1);
    /* 0D0A mov.w a,$0241+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0241 + spc.x)));
    /* 0D0D mov.b $31+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0x31 + spc.x)), spc.a, 4, 1);
    /* 0D0F ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:C9FF SpcSetEchoDelay
 * WHAT IT DOES: Sets the echo delay to A ($4D): when EDL differs, the DSP registers named by the first four entries of the list at $3F9F are zeroed, echo writes are turned off in FLG ($48 with bit 5), EDL is written and $4C starts the count SpcReset waits on before writing the echo registers again; in every case the echo buffer start ESA = $40 - 8 * delay.
 * HOW: EDL ($7D) read back to compare; ESA ($6D) by ASLs, EOR and ADC.
 * Entry: A = delay (0-15). Exit: A, Y as the last DSP write left them.
 * @spc 0D73
 * @spcuses 09E8-09EF,0D73-0DB9
 */
void abi_SpcSetEchoDelay(void)
{
    /* 0D73 mov.b $4D,a */
    spc_st_cyc(spc_dp(0x4D), spc.a, 3, 1);
    /* 0D75 mov y,#$7D */
    spc.y = spc_nz(0x7D);
    spc_cyc(2);
    /* 0D77 mov.w $00F2,y */
    spc_st_cyc(0x00F2, spc.y, 4, 1);
    /* 0D7A mov.w a,$00F3 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x00F3));
    /* 0D7D cmp.b a,$4D */
    spc_cyc(3);
    spc_cmp(spc.a, spc_rd(spc_dp(0x4D)));
    /* 0D7F beq $0DAC */
    if (spc.z) { spc_cyc(4); goto L0DAC; }
    spc_cyc(2);
    /* 0D81 and a,#$0F */
    spc.a = spc_and(spc.a, 0x0F);
    spc_cyc(2);
    /* 0D83 eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    spc_cyc(2);
    /* 0D85 bbc $4C.7,$0D8B */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x4C)) >> 7 & 1)) { spc_cyc(4); goto L0D8B; }
    spc_cyc(2);
    /* 0D88 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 0D89 adc.b a,$4C */
    spc_cyc(3);
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0x4C)));
L0D8B:
    /* 0D8B mov.b $4C,a */
    spc_st_cyc(spc_dp(0x4C), spc.a, 3, 1);
    /* 0D8D mov y,#$04 */
    spc.y = spc_nz(0x04);
    spc_cyc(2);
L0D8F:
    /* 0D8F mov.w a,$3F9F+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x3F9F + spc.y)));
    /* 0D92 mov.w $00F2,a */
    spc_st_cyc(0x00F2, spc.a, 4, 1);
    /* 0D95 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0D97 mov.w $00F3,a */
    spc_st_cyc(0x00F3, spc.a, 4, 1);
    /* 0D9A dbnz y,$0D8F */
    if (--spc.y) { spc_cyc(6); goto L0D8F; }
    spc_cyc(4);
    /* 0D9C mov.b a,$48 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x48)));
    /* 0D9E or a,#$20 */
    spc.a = spc_or(spc.a, 0x20);
    spc_cyc(2);
    /* 0DA0 mov y,#$6C */
    spc.y = spc_nz(0x6C);
    spc_cyc(2);
    /* 0DA2 call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x0DA5, 5, 1, 2);
    /* 0DA5 mov.b a,$4D */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x4D)));
    /* 0DA7 mov y,#$7D */
    spc.y = spc_nz(0x7D);
    spc_cyc(2);
    /* 0DA9 call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x0DAC, 5, 1, 2);
L0DAC:
    /* 0DAC asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 0DAD asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 0DAE asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 0DAF eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    spc_cyc(2);
    /* 0DB1 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 0DB2 adc a,#$3F */
    spc.a = spc_adc(spc.a, 0x3F);
    spc_cyc(2);
    /* 0DB4 mov y,#$6D */
    spc.y = spc_nz(0x6D);
    spc_cyc(2);
    /* 0DB6 jmp $09E8 */
    { spc_cyc(3); abi_SpcDspWrite(); return; }
}

/*
 * @routine 19:CA48 SpcSlideCheck
 * WHAT IT DOES: When voice X's next track byte is a pitch slide ($F9) and no slide runs, reads its delay, length and target note and starts the slide (SpcSlideStart's code).
 * HOW: Peeks at ($30+X); the slide delta from SpcFadeDelta.
 * Entry: X = voice * 2. Exit: A, Y changed.
 * @spc 0DBC
 * @spcuses 0BB4-0BBE,0DBC-0DF4,0DFF-0E1B
 */
void abi_SpcSlideCheck(void)
{
    /* 0DBC mov.b a,$A0+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0xA0 + spc.x))));
    /* 0DBE bne $0DF3 */
    if (!spc.z) { spc_cyc(4); goto L0DF3; }
    spc_cyc(2);
    /* 0DC0 mov a,($30+x) */
    spc_cyc(4);
    { uint16_t p = spc_rd(spc_dp((uint8_t)(0x30 + spc.x))); p |= (spc_cyc(1), spc_rd(spc_dp((uint8_t)(0x30 + spc.x + 1)))) << 8;
      spc_cyc(1);
      spc.a = spc_nz(spc_rd(p)); }
    /* 0DC2 cmp a,#$F9 */
    spc_cmp(spc.a, 0xF9);
    spc_cyc(2);
    /* 0DC4 bne $0DF3 */
    if (!spc.z) { spc_cyc(4); goto L0DF3; }
    spc_cyc(2);
    /* 0DC6 call $0BB6 */
    spc_call_cyc(abi_SpcTrackAdvance, 0x0DC9, 5, 1, 2);
    /* 0DC9 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0DCC, 5, 1, 2);
    /* 0DCC mov.b $A1+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0xA1 + spc.x)), spc.a, 4, 1);
    /* 0DCE call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0DD1, 5, 1, 2);
    /* 0DD1 mov.b $A0+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0xA0 + spc.x)), spc.a, 4, 1);
    /* 0DD3 call $0BB4 */
    spc_call_cyc(abi_SpcTrackRead, 0x0DD6, 5, 1, 2);
    /* 0DD6 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 0DD7 adc.b a,$50 */
    spc_cyc(3);
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0x50)));
    /* 0DD9 adc.w a,$02F0+x */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x02F0 + spc.x)));
    { abi_SpcSlideToNote(); return; }
L0DF3:
    /* 0DF3 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:CA68 SpcSlideToNote
 * WHAT IT DOES: Starts voice X's pitch slide to note A (bits 0-6): the target kept at $0380 and the per-step delta over the slide's steps $A0 at $0370/$0371.
 * HOW: SpcFadeDelta on target minus pitch; X from the steps through the stack.
 * Entry: A = target note, X = voice * 2. Exit: X = voice * 2; YA = delta.
 * @spc 0DDC
 * @spcuses 0DDC-0DF4,0DFF-0E1B
 */
void abi_SpcSlideToNote(void)
{
    /* 0DDC and a,#$7F */
    spc.a = spc_and(spc.a, 0x7F);
    spc_cyc(2);
    /* 0DDE mov.w $0380+x,a */
    spc_st_cyc((uint16_t)(0x0380 + spc.x), spc.a, 5, 1);
    /* 0DE1 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 0DE2 sbc.w a,$0361+x */
    spc_cyc(5);
    spc.a = spc_sbc(spc.a, spc_rd((uint16_t)(0x0361 + spc.x)));
    /* 0DE5 mov.b y,$A0+x */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(spc_dp((uint8_t)(0xA0 + spc.x))));
    /* 0DE7 push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 0DE8 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 0DE9 call $0DFF */
    spc_call_cyc(abi_SpcFadeDelta, 0x0DEC, 5, 1, 2);
    /* 0DEC mov.w $0370+x,a */
    spc_st_cyc((uint16_t)(0x0370 + spc.x), spc.a, 5, 1);
    /* 0DEF mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 0DF0 mov.w $0371+x,a */
    spc_st_cyc((uint16_t)(0x0371 + spc.x), spc.a, 5, 1);
    /* 0DF3 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:CA80 SpcLoadPitch
 * WHAT IT DOES: Copies voice X's pitch $0360/$0361 to $10/$11.
 * HOW: Two byte copies.
 * Entry: X = voice * 2. Exit: A = the pitch's low byte.
 * @spc 0DF4
 * @spcuses 0DF4-0DFF
 */
void abi_SpcLoadPitch(void)
{
    /* 0DF4 mov.w a,$0361+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0361 + spc.x)));
    /* 0DF7 mov.b $11,a */
    spc_st_cyc(spc_dp(0x11), spc.a, 3, 1);
    /* 0DF9 mov.w a,$0360+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0360 + spc.x)));
    /* 0DFC mov.b $10,a */
    spc_st_cyc(spc_dp(0x10), spc.a, 3, 1);
    /* 0DFE ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:CA8B SpcFadeDelta
 * WHAT IT DOES: The per-step delta of a fade: the signed difference A (with the borrow C from the subtraction that made it) divided by the step count X, as a 16-bit value.
 * HOW: The sign kept in $12 (NOTC, ROR), the magnitude divided in two DIVs (whole, then fraction), negated back through SpcNegateIfNeg; X reloaded from $44.
 * Entry: A = difference, C = no borrow, X = steps. Exit: YA = delta; X = $44.
 * @spc 0DFF
 * @spcuses 0DFF-0E1B
 */
void abi_SpcFadeDelta(void)
{
    /* 0DFF notc */
    spc.c ^= 1;
    spc_cyc(3);
    /* 0E00 ror.b $12 */
    spc_rmw_cyc(spc_dp(0x12), spc_ror, 3, 1);
    /* 0E02 bpl $0E07 */
    if (!spc.n) { spc_cyc(4); goto L0E07; }
    spc_cyc(2);
    /* 0E04 eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    spc_cyc(2);
    /* 0E06 inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
L0E07:
    /* 0E07 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 0E09 div ya,x */
    spc_div();
    spc_cyc(12);
    /* 0E0A push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 0E0B mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0E0D div ya,x */
    spc_div();
    spc_cyc(12);
    /* 0E0E pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 0E0F mov.b x,$44 */
    spc_cyc(3);
    spc.x = spc_nz(spc_rd(spc_dp(0x44)));
    { abi_SpcNegateIfNeg(); return; }
}

/*
 * @routine 19:CA9D SpcNegateIfNeg
 * WHAT IT DOES: Negates YA when bit 7 of $12 is set.
 * HOW: 0 - YA through $0E and $14.
 * Entry: YA = value, $12 = sign. Exit: YA.
 * @spc 0E11
 * @spcuses 0E11-0E1B
 */
void abi_SpcNegateIfNeg(void)
{
    /* 0E11 bbc $12.7,$0E1A */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x12)) >> 7 & 1)) { spc_cyc(4); goto L0E1A; }
    spc_cyc(2);
    /* 0E14 movw $14,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x14));
    spc_cyc(1);
    spc_wr(spc_dp(0x14), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x15), spc.y);
    /* 0E16 movw ya,$0E */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x0E)); v |= (spc_cyc(2), spc_rd(spc_dp(0x0F))) << 8; spc_set_ya(spc_nz16(v)); }
    /* 0E18 subw ya,$14 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x14)); v |= (spc_cyc(2), spc_rd(spc_dp(0x15))) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
L0E1A:
    /* 0E1A ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:CAA7 SpcVoiceEffects
 * WHAT IT DOES: Voice X's per-tick modulation: the volume fade steps, tremolo moves the volume, the volume is recomputed (SpcVolumeCalc), the pan fade steps; then the volume and pan go to the DSP (SpcVoiceVolumeWrite).
 * HOW: 16-bit fades as low/high byte pairs; tremolo by its phase and depth.
 * Entry: X = voice * 2, $47 = its bit. Exit: as SpcVoiceVolumeWrite.
 * @spc 0E1B
 * @spcuses 09E0-09EF,0E1B-0EE4,105C-107A
 */
void abi_SpcVoiceEffects(void)
{
    /* 0E1B mov.b a,$90+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x90 + spc.x))));
    /* 0E1D beq $0E43 */
    if (spc.z) { spc_cyc(4); goto L0E43; }
    spc_cyc(2);
    /* 0E1F or $5E,$47 */
    spc_cyc(3);
    { uint8_t r = spc_rd(spc_dp(0x47)); uint8_t l = (spc_cyc(2), spc_rd(spc_dp(0x5E))); spc_cyc(1); spc_wr(spc_dp(0x5E), spc_or(l, r)); }
    /* 0E22 dec.b $90+x */
    spc_rmw_cyc(spc_dp((uint8_t)(0x90 + spc.x)), spc_dec, 4, 1);
    /* 0E24 bne $0E30 */
    if (!spc.z) { spc_cyc(4); goto L0E30; }
    spc_cyc(2);
    /* 0E26 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0E28 mov.w $0300+x,a */
    spc_st_cyc((uint16_t)(0x0300 + spc.x), spc.a, 5, 1);
    /* 0E2B mov.w a,$0320+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0320 + spc.x)));
    /* 0E2E bra $0E40 */
    spc_cyc(4);
    goto L0E40;
L0E30:
    /* 0E30 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 0E31 mov.w a,$0300+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0300 + spc.x)));
    /* 0E34 adc.w a,$0310+x */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x0310 + spc.x)));
    /* 0E37 mov.w $0300+x,a */
    spc_st_cyc((uint16_t)(0x0300 + spc.x), spc.a, 5, 1);
    /* 0E3A mov.w a,$0301+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0301 + spc.x)));
    /* 0E3D adc.w a,$0311+x */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x0311 + spc.x)));
L0E40:
    /* 0E40 mov.w $0301+x,a */
    spc_st_cyc((uint16_t)(0x0301 + spc.x), spc.a, 5, 1);
L0E43:
    /* 0E43 mov.b y,$C1+x */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(spc_dp((uint8_t)(0xC1 + spc.x))));
    /* 0E45 beq $0E6A */
    if (spc.z) { spc_cyc(4); goto L0E6A; }
    spc_cyc(2);
    /* 0E47 mov.w a,$02E0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x02E0 + spc.x)));
    /* 0E4A cbne $C0+x,$0E68 */
    spc_cyc(4);
    if (spc.a != spc_rd(spc_dp((uint8_t)(0xC0 + spc.x)))) { spc_cyc(4); goto L0E68; }
    spc_cyc(2);
    /* 0E4D or $5E,$47 */
    spc_cyc(3);
    { uint8_t r = spc_rd(spc_dp(0x47)); uint8_t l = (spc_cyc(2), spc_rd(spc_dp(0x5E))); spc_cyc(1); spc_wr(spc_dp(0x5E), spc_or(l, r)); }
    /* 0E50 mov.w a,$02D0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x02D0 + spc.x)));
    /* 0E53 bpl $0E5C */
    if (!spc.n) { spc_cyc(4); goto L0E5C; }
    spc_cyc(2);
    /* 0E55 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 0E56 bne $0E5C */
    if (!spc.z) { spc_cyc(4); goto L0E5C; }
    spc_cyc(2);
    /* 0E58 mov a,#$80 */
    spc.a = spc_nz(0x80);
    spc_cyc(2);
    /* 0E5A bra $0E60 */
    spc_cyc(4);
    goto L0E60;
L0E5C:
    /* 0E5C clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 0E5D adc.w a,$02D1+x */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x02D1 + spc.x)));
L0E60:
    /* 0E60 mov.w $02D0+x,a */
    spc_st_cyc((uint16_t)(0x02D0 + spc.x), spc.a, 5, 1);
    /* 0E63 call $105C */
    spc_call_cyc(abi_SpcTremoloFactor, 0x0E66, 5, 1, 2);
    /* 0E66 bra $0E6F */
    spc_cyc(4);
    goto L0E6F;
L0E68:
    /* 0E68 inc.b $C0+x */
    spc_rmw_cyc(spc_dp((uint8_t)(0xC0 + spc.x)), spc_inc, 4, 1);
L0E6A:
    /* 0E6A mov a,#$FF */
    spc.a = spc_nz(0xFF);
    spc_cyc(2);
    /* 0E6C call $1067 */
    spc_call_cyc(abi_SpcVolumeCalc, 0x0E6F, 5, 1, 2);
L0E6F:
    /* 0E6F mov.b a,$91+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x91 + spc.x))));
    /* 0E71 beq $0E97 */
    if (spc.z) { spc_cyc(4); abi_SpcVoiceVolumeWrite(); return; }
    spc_cyc(2);
    /* 0E73 or $5E,$47 */
    spc_cyc(3);
    { uint8_t r = spc_rd(spc_dp(0x47)); uint8_t l = (spc_cyc(2), spc_rd(spc_dp(0x5E))); spc_cyc(1); spc_wr(spc_dp(0x5E), spc_or(l, r)); }
    /* 0E76 dec.b $91+x */
    spc_rmw_cyc(spc_dp((uint8_t)(0x91 + spc.x)), spc_dec, 4, 1);
    /* 0E78 bne $0E84 */
    if (!spc.z) { spc_cyc(4); goto L0E84; }
    spc_cyc(2);
    /* 0E7A mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0E7C mov.w $0330+x,a */
    spc_st_cyc((uint16_t)(0x0330 + spc.x), spc.a, 5, 1);
    /* 0E7F mov.w a,$0350+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0350 + spc.x)));
    /* 0E82 bra $0E94 */
    spc_cyc(4);
    goto L0E94;
L0E84:
    /* 0E84 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 0E85 mov.w a,$0330+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0330 + spc.x)));
    /* 0E88 adc.w a,$0340+x */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x0340 + spc.x)));
    /* 0E8B mov.w $0330+x,a */
    spc_st_cyc((uint16_t)(0x0330 + spc.x), spc.a, 5, 1);
    /* 0E8E mov.w a,$0331+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0331 + spc.x)));
    /* 0E91 adc.w a,$0341+x */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x0341 + spc.x)));
L0E94:
    /* 0E94 mov.w $0331+x,a */
    spc_st_cyc((uint16_t)(0x0331 + spc.x), spc.a, 5, 1);
    { abi_SpcVoiceVolumeWrite(); return; }
}

/*
 * @routine 19:CB23 SpcVoiceVolumeWrite
 * WHAT IT DOES: When voice X's volume or pan changed ($5E), writes its left and right volumes (SpcVoiceVolumeLR) from its pan.
 * HOW: The pan $0330/$0331 to $10/$11, then SpcVoiceVolumeLR.
 * Entry: X = voice * 2, $47 = its bit. Exit: as SpcVoiceVolumeLR.
 * @spc 0E97
 * @spcuses 09E0-09EF,0E97-0EE4
 */
void abi_SpcVoiceVolumeWrite(void)
{
    /* 0E97 mov.b a,$47 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x47)));
    /* 0E99 and.b a,$5E */
    spc_cyc(3);
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0x5E)));
    /* 0E9B beq $0EE3 */
    if (spc.z) { spc_cyc(4); goto L0EE3; }
    spc_cyc(2);
    /* 0E9D mov.w a,$0331+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0331 + spc.x)));
    /* 0EA0 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0EA1 mov.w a,$0330+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0330 + spc.x)));
    /* 0EA4 movw $10,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x10));
    spc_cyc(1);
    spc_wr(spc_dp(0x10), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x11), spc.y);
    { abi_SpcVoiceVolumeLR(); return; }
L0EE3:
    /* 0EE3 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:CB32 SpcVoiceVolumeLR
 * WHAT IT DOES: Writes voice X's VxVOLL and VxVOLR: the pan curve at $107A interpolated at the pan $10/$11, times the voice's volume, negated for a side the pan flags invert; the right side from 20 minus the pan.
 * HOW: Two passes over $12 (the register), MUL for the interpolation and the volume.
 * Entry: $10/$11 = pan, X = voice * 2. Exit: A, Y changed.
 * @spc 0EA6
 * @spcuses 09E0-09EF,0EA6-0EE4
 */
void abi_SpcVoiceVolumeLR(void)
{
    /* 0EA6 mov a,x */
    spc.a = spc_nz(spc.x);
    spc_cyc(2);
    /* 0EA7 xcn a */
    spc.a = spc_xcn(spc.a);
    spc_cyc(5);
    /* 0EA8 lsr a */
    spc.a = spc_lsr(spc.a);
    spc_cyc(2);
    /* 0EA9 mov.b $12,a */
    spc_st_cyc(spc_dp(0x12), spc.a, 3, 1);
L0EAB:
    /* 0EAB mov.b y,$11 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x11)));
    /* 0EAD mov.w a,$107B+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x107B + spc.y)));
    /* 0EB0 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 0EB1 sbc.w a,$107A+y */
    spc_cyc(5);
    spc.a = spc_sbc(spc.a, spc_rd((uint16_t)(0x107A + spc.y)));
    /* 0EB4 mov.b y,$10 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x10)));
    /* 0EB6 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 0EB7 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 0EB8 mov.b y,$11 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x11)));
    /* 0EBA clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 0EBB adc.w a,$107A+y */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x107A + spc.y)));
    /* 0EBE mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0EBF mov.w a,$0321+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0321 + spc.x)));
    /* 0EC2 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 0EC3 mov.w a,$0351+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0351 + spc.x)));
    /* 0EC6 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 0EC7 bbc $12.0,$0ECB */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x12)) >> 0 & 1)) { spc_cyc(4); goto L0ECB; }
    spc_cyc(2);
    /* 0ECA asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
L0ECB:
    /* 0ECB mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 0ECC bcc $0ED1 */
    if (!spc.c) { spc_cyc(4); goto L0ED1; }
    spc_cyc(2);
    /* 0ECE eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    spc_cyc(2);
    /* 0ED0 inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
L0ED1:
    /* 0ED1 mov.b y,$12 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x12)));
    /* 0ED3 call $09E0 */
    spc_call_cyc(abi_SpcDspWriteVoice, 0x0ED6, 5, 1, 2);
    /* 0ED6 mov y,#$14 */
    spc.y = spc_nz(0x14);
    spc_cyc(2);
    /* 0ED8 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 0EDA subw ya,$10 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x10)); v |= (spc_cyc(2), spc_rd(spc_dp(0x11))) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 0EDC movw $10,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x10));
    spc_cyc(1);
    spc_wr(spc_dp(0x10), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x11), spc.y);
    /* 0EDE inc.b $12 */
    spc_rmw_cyc(spc_dp(0x12), spc_inc, 3, 1);
    /* 0EE0 bbc $12.1,$0EAB */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x12)) >> 1 & 1)) { spc_cyc(4); goto L0EAB; }
    spc_cyc(2);
    /* 0EE3 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:CB70 SpcVoiceTick
 * WHAT IT DOES: Voice X between its notes: at the gate time, key-off unless the next note is a tie (the track is scanned ahead through voice commands, subroutine ends and returns); the pitch slide steps; vibrato moves the pitch; the pitch is written when it changed.
 * HOW: The look-ahead skips command parameters by the table at $0FAF; the slide and vibrato through the pitch code at $095D.
 * Entry: X = voice * 2, $47 = its bit. Exit: A, Y changed.
 * @spc 0EE4
 * @spcuses 095D-09EF,0DF4-0DFF,0E11-0E1B,0EE4-0FD6,1047-104F
 */
void abi_SpcVoiceTick(void)
{
    /* 0EE4 mov.b a,$71+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x71 + spc.x))));
    /* 0EE6 beq $0F4D */
    if (spc.z) { spc_cyc(4); goto L0F4D; }
    spc_cyc(2);
    /* 0EE8 dec.b $71+x */
    spc_rmw_cyc(spc_dp((uint8_t)(0x71 + spc.x)), spc_dec, 4, 1);
    /* 0EEA beq $0EF1 */
    if (spc.z) { spc_cyc(4); goto L0EF1; }
    spc_cyc(2);
    /* 0EEC mov a,#$02 */
    spc.a = spc_nz(0x02);
    spc_cyc(2);
    /* 0EEE cbne $70+x,$0F4D */
    spc_cyc(4);
    if (spc.a != spc_rd(spc_dp((uint8_t)(0x70 + spc.x)))) { spc_cyc(4); goto L0F4D; }
    spc_cyc(2);
L0EF1:
    /* 0EF1 mov.b a,$80+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x80 + spc.x))));
    /* 0EF3 mov.b $17,a */
    spc_st_cyc(spc_dp(0x17), spc.a, 3, 1);
    /* 0EF5 mov.b a,$30+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x30 + spc.x))));
    /* 0EF7 mov.b y,$31+x */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(spc_dp((uint8_t)(0x31 + spc.x))));
L0EF9:
    /* 0EF9 movw $14,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x14));
    spc_cyc(1);
    spc_wr(spc_dp(0x14), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x15), spc.y);
    /* 0EFB mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
L0EFD:
    /* 0EFD mov a,($14)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x14)); p |= (spc_cyc(1), spc_rd(spc_dp(0x15))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 0EFF beq $0F1F */
    if (spc.z) { spc_cyc(4); goto L0F1F; }
    spc_cyc(2);
    /* 0F01 bmi $0F0A */
    if (spc.n) { spc_cyc(4); goto L0F0A; }
    spc_cyc(2);
L0F03:
    /* 0F03 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 0F04 bmi $0F46 */
    if (spc.n) { spc_cyc(4); goto L0F46; }
    spc_cyc(2);
    /* 0F06 mov a,($14)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x14)); p |= (spc_cyc(1), spc_rd(spc_dp(0x15))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 0F08 bpl $0F03 */
    if (!spc.n) { spc_cyc(4); goto L0F03; }
    spc_cyc(2);
L0F0A:
    /* 0F0A cmp a,#$C8 */
    spc_cmp(spc.a, 0xC8);
    spc_cyc(2);
    /* 0F0C beq $0F4D */
    if (spc.z) { spc_cyc(4); goto L0F4D; }
    spc_cyc(2);
    /* 0F0E cmp a,#$EF */
    spc_cmp(spc.a, 0xEF);
    spc_cyc(2);
    /* 0F10 beq $0F3B */
    if (spc.z) { spc_cyc(4); goto L0F3B; }
    spc_cyc(2);
    /* 0F12 cmp a,#$E0 */
    spc_cmp(spc.a, 0xE0);
    spc_cyc(2);
    /* 0F14 bcc $0F46 */
    if (!spc.c) { spc_cyc(4); goto L0F46; }
    spc_cyc(2);
    /* 0F16 push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 0F17 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0F18 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 0F19 adc.w a,$0FAF+y */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x0FAF + spc.y)));
    /* 0F1C mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0F1D bra $0EFD */
    spc_cyc(4);
    goto L0EFD;
L0F1F:
    /* 0F1F mov.b a,$17 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x17)));
    /* 0F21 beq $0F46 */
    if (spc.z) { spc_cyc(4); goto L0F46; }
    spc_cyc(2);
    /* 0F23 dec.b $17 */
    spc_rmw_cyc(spc_dp(0x17), spc_dec, 3, 1);
    /* 0F25 bne $0F31 */
    if (!spc.z) { spc_cyc(4); goto L0F31; }
    spc_cyc(2);
    /* 0F27 mov.w a,$0231+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0231 + spc.x)));
    /* 0F2A push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 0F2B mov.w a,$0230+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0230 + spc.x)));
    /* 0F2E pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 0F2F bra $0EF9 */
    spc_cyc(4);
    goto L0EF9;
L0F31:
    /* 0F31 mov.w a,$0241+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0241 + spc.x)));
    /* 0F34 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 0F35 mov.w a,$0240+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0240 + spc.x)));
    /* 0F38 pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 0F39 bra $0EF9 */
    spc_cyc(4);
    goto L0EF9;
L0F3B:
    /* 0F3B inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 0F3C mov a,($14)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x14)); p |= (spc_cyc(1), spc_rd(spc_dp(0x15))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 0F3E push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 0F3F inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 0F40 mov a,($14)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x14)); p |= (spc_cyc(1), spc_rd(spc_dp(0x15))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 0F42 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0F43 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 0F44 bra $0EF9 */
    spc_cyc(4);
    goto L0EF9;
L0F46:
    /* 0F46 mov.b a,$47 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x47)));
    /* 0F48 mov y,#$5C */
    spc.y = spc_nz(0x5C);
    spc_cyc(2);
    /* 0F4A call $09E0 */
    spc_call_cyc(abi_SpcDspWriteVoice, 0x0F4D, 5, 1, 2);
L0F4D:
    /* 0F4D clr1 $13.7 */
    spc_bit_set_cyc(0x13, 7, 0, 3, 1);
    /* 0F4F mov.b a,$A0+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0xA0 + spc.x))));
    /* 0F51 beq $0F7F */
    if (spc.z) { spc_cyc(4); goto L0F7F; }
    spc_cyc(2);
    /* 0F53 mov.b a,$A1+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0xA1 + spc.x))));
    /* 0F55 beq $0F5B */
    if (spc.z) { spc_cyc(4); goto L0F5B; }
    spc_cyc(2);
    /* 0F57 dec.b $A1+x */
    spc_rmw_cyc(spc_dp((uint8_t)(0xA1 + spc.x)), spc_dec, 4, 1);
    /* 0F59 bra $0F7F */
    spc_cyc(4);
    goto L0F7F;
L0F5B:
    /* 0F5B set1 $13.7 */
    spc_bit_set_cyc(0x13, 7, 1, 3, 1);
    /* 0F5D dec.b $A0+x */
    spc_rmw_cyc(spc_dp((uint8_t)(0xA0 + spc.x)), spc_dec, 4, 1);
    /* 0F5F bne $0F6C */
    if (!spc.z) { spc_cyc(4); goto L0F6C; }
    spc_cyc(2);
    /* 0F61 mov.w a,$0381+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0381 + spc.x)));
    /* 0F64 mov.w $0360+x,a */
    spc_st_cyc((uint16_t)(0x0360 + spc.x), spc.a, 5, 1);
    /* 0F67 mov.w a,$0380+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0380 + spc.x)));
    /* 0F6A bra $0F7C */
    spc_cyc(4);
    goto L0F7C;
L0F6C:
    /* 0F6C clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 0F6D mov.w a,$0360+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0360 + spc.x)));
    /* 0F70 adc.w a,$0370+x */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x0370 + spc.x)));
    /* 0F73 mov.w $0360+x,a */
    spc_st_cyc((uint16_t)(0x0360 + spc.x), spc.a, 5, 1);
    /* 0F76 mov.w a,$0361+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0361 + spc.x)));
    /* 0F79 adc.w a,$0371+x */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x0371 + spc.x)));
L0F7C:
    /* 0F7C mov.w $0361+x,a */
    spc_st_cyc((uint16_t)(0x0361 + spc.x), spc.a, 5, 1);
L0F7F:
    /* 0F7F call $0DF4 */
    spc_call_cyc(abi_SpcLoadPitch, 0x0F82, 5, 1, 2);
    /* 0F82 mov.b a,$B1+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0xB1 + spc.x))));
    /* 0F84 beq $0FD2 */
    if (spc.z) { spc_cyc(4); goto L0FD2; }
    spc_cyc(2);
    /* 0F86 mov.w a,$02B0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x02B0 + spc.x)));
    /* 0F89 cbne $B0+x,$0FD0 */
    spc_cyc(4);
    if (spc.a != spc_rd(spc_dp((uint8_t)(0xB0 + spc.x)))) { spc_cyc(4); goto L0FD0; }
    spc_cyc(2);
    /* 0F8C mov.w a,$0100+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0100 + spc.x)));
    /* 0F8F cmp.w a,$02B1+x */
    spc_cyc(5);
    spc_cmp(spc.a, spc_rd((uint16_t)(0x02B1 + spc.x)));
    /* 0F92 bne $0F99 */
    if (!spc.z) { spc_cyc(4); goto L0F99; }
    spc_cyc(2);
    /* 0F94 mov.w a,$02C1+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x02C1 + spc.x)));
    /* 0F97 bra $0FA6 */
    spc_cyc(4);
    goto L0FA6;
L0F99:
    /* 0F99 setp */
    spc.p = 1;
    spc_cyc(2);
    /* 0F9A inc.b $00+x */
    spc_rmw_cyc(spc_dp((uint8_t)(0x00 + spc.x)), spc_inc, 4, 1);
    /* 0F9C clrp */
    spc.p = 0;
    spc_cyc(2);
    /* 0F9D mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0F9E beq $0FA2 */
    if (spc.z) { spc_cyc(4); goto L0FA2; }
    spc_cyc(2);
    /* 0FA0 mov.b a,$B1+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0xB1 + spc.x))));
L0FA2:
    /* 0FA2 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 0FA3 adc.w a,$02C0+x */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x02C0 + spc.x)));
L0FA6:
    /* 0FA6 mov.b $B1+x,a */
    spc_st_cyc(spc_dp((uint8_t)(0xB1 + spc.x)), spc.a, 4, 1);
    /* 0FA8 mov.w a,$02A0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x02A0 + spc.x)));
    /* 0FAB clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 0FAC adc.w a,$02A1+x */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x02A1 + spc.x)));
    /* 0FAF mov.w $02A0+x,a */
    spc_st_cyc((uint16_t)(0x02A0 + spc.x), spc.a, 5, 1);
    /* 0FB2 mov.b $12,a */
    spc_st_cyc(spc_dp(0x12), spc.a, 3, 1);
    /* 0FB4 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 0FB5 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 0FB6 bcc $0FBA */
    if (!spc.c) { spc_cyc(4); goto L0FBA; }
    spc_cyc(2);
    /* 0FB8 eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    spc_cyc(2);
L0FBA:
    /* 0FBA mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0FBB mov.b a,$B1+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0xB1 + spc.x))));
    /* 0FBD cmp a,#$F1 */
    spc_cmp(spc.a, 0xF1);
    spc_cyc(2);
    /* 0FBF bcc $0FC6 */
    if (!spc.c) { spc_cyc(4); goto L0FC6; }
    spc_cyc(2);
    /* 0FC1 and a,#$0F */
    spc.a = spc_and(spc.a, 0x0F);
    spc_cyc(2);
    /* 0FC3 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 0FC4 bra $0FCA */
    spc_cyc(4);
    goto L0FCA;
L0FC6:
    /* 0FC6 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 0FC7 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 0FC8 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
L0FCA:
    /* 0FCA call $1047 */
    spc_call_cyc(abi_SpcAddDelta, 0x0FCD, 5, 1, 2);
L0FCD:
    /* 0FCD jmp $095D */
    spc_cyc(3);
    goto L095D;
L0FD0:
    /* 0FD0 inc.b $B0+x */
    spc_rmw_cyc(spc_dp((uint8_t)(0xB0 + spc.x)), spc_inc, 4, 1);
L0FD2:
    /* 0FD2 bbs $13.7,$0FCD */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x13)) >> 7 & 1)) { spc_cyc(4); goto L0FCD; }
    spc_cyc(2);
    /* 0FD5 ret */
    spc_ret_cyc(4, 1);
    return;
L095D:
    /* 095D mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 095F mov.b a,$11 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x11)));
    /* 0961 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 0962 sbc a,#$34 */
    spc.a = spc_sbc(spc.a, 0x34);
    spc_cyc(2);
    /* 0964 bcs $096F */
    if (spc.c) { spc_cyc(4); goto L096F; }
    spc_cyc(2);
    /* 0966 mov.b a,$11 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x11)));
    /* 0968 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 0969 sbc a,#$13 */
    spc.a = spc_sbc(spc.a, 0x13);
    spc_cyc(2);
    /* 096B bcs $0973 */
    if (spc.c) { spc_cyc(4); goto L0973; }
    spc_cyc(2);
    /* 096D dec y */
    spc.y = spc_dec(spc.y);
    spc_cyc(2);
    /* 096E asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
L096F:
    /* 096F addw ya,$10 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x10)); v |= (spc_cyc(2), spc_rd(spc_dp(0x11))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 0971 movw $10,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x10));
    spc_cyc(1);
    spc_wr(spc_dp(0x10), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x11), spc.y);
L0973:
    /* 0973 push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 0974 mov.b a,$11 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x11)));
    /* 0976 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 0977 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 0979 mov x,#$18 */
    spc.x = spc_nz(0x18);
    spc_cyc(2);
    /* 097B div ya,x */
    spc_div();
    spc_cyc(12);
    /* 097C mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 097D mov.w a,$06CE+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x06CE + spc.y)));
    /* 0980 mov.b $15,a */
    spc_st_cyc(spc_dp(0x15), spc.a, 3, 1);
    /* 0982 mov.w a,$06CD+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x06CD + spc.y)));
    /* 0985 mov.b $14,a */
    spc_st_cyc(spc_dp(0x14), spc.a, 3, 1);
    /* 0987 mov.w a,$06D0+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x06D0 + spc.y)));
    /* 098A push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 098B mov.w a,$06CF+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x06CF + spc.y)));
    /* 098E pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 098F subw ya,$14 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x14)); v |= (spc_cyc(2), spc_rd(spc_dp(0x15))) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 0991 mov.b y,$10 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x10)));
    /* 0993 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 0994 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 0995 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 0997 addw ya,$14 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x14)); v |= (spc_cyc(2), spc_rd(spc_dp(0x15))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 0999 mov.b $15,y */
    spc_st_cyc(spc_dp(0x15), spc.y, 3, 1);
    /* 099B asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 099C rol.b $15 */
    spc_rmw_cyc(spc_dp(0x15), spc_rol, 3, 1);
    /* 099E mov.b $14,a */
    spc_st_cyc(spc_dp(0x14), spc.a, 3, 1);
    /* 09A0 bra $09A6 */
    spc_cyc(4);
    goto L09A6;
L09A2:
    /* 09A2 lsr.b $15 */
    spc_rmw_cyc(spc_dp(0x15), spc_lsr, 3, 1);
    /* 09A4 ror a */
    spc.a = spc_ror(spc.a);
    spc_cyc(2);
    /* 09A5 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
L09A6:
    /* 09A6 cmp x,#$06 */
    spc_cmp(spc.x, 0x06);
    spc_cyc(2);
    /* 09A8 bne $09A2 */
    if (!spc.z) { spc_cyc(4); goto L09A2; }
    spc_cyc(2);
    /* 09AA mov.b $14,a */
    spc_st_cyc(spc_dp(0x14), spc.a, 3, 1);
    /* 09AC pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 09AD mov.w a,$0220+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0220 + spc.x)));
    /* 09B0 mov.b y,$15 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x15)));
    /* 09B2 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 09B3 movw $16,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x16));
    spc_cyc(1);
    spc_wr(spc_dp(0x16), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x17), spc.y);
    /* 09B5 mov.w a,$0220+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0220 + spc.x)));
    /* 09B8 mov.b y,$14 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x14)));
    /* 09BA mul ya */
    spc_mul();
    spc_cyc(9);
    /* 09BB push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 09BC mov.w a,$0221+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0221 + spc.x)));
    /* 09BF mov.b y,$14 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x14)));
    /* 09C1 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 09C2 addw ya,$16 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x16)); v |= (spc_cyc(2), spc_rd(spc_dp(0x17))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 09C4 movw $16,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x16));
    spc_cyc(1);
    spc_wr(spc_dp(0x16), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x17), spc.y);
    /* 09C6 mov.w a,$0221+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0221 + spc.x)));
    /* 09C9 mov.b y,$15 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x15)));
    /* 09CB mul ya */
    spc_mul();
    spc_cyc(9);
    /* 09CC mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 09CD pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 09CE addw ya,$16 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x16)); v |= (spc_cyc(2), spc_rd(spc_dp(0x17))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 09D0 movw $16,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x16));
    spc_cyc(1);
    spc_wr(spc_dp(0x16), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x17), spc.y);
    /* 09D2 mov a,x */
    spc.a = spc_nz(spc.x);
    spc_cyc(2);
    /* 09D3 xcn a */
    spc.a = spc_xcn(spc.a);
    spc_cyc(5);
    /* 09D4 lsr a */
    spc.a = spc_lsr(spc.a);
    spc_cyc(2);
    /* 09D5 or a,#$02 */
    spc.a = spc_or(spc.a, 0x02);
    spc_cyc(2);
    /* 09D7 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 09D8 mov.b a,$16 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x16)));
    /* 09DA call $09E0 */
    spc_call_cyc(abi_SpcDspWriteVoice, 0x09DD, 5, 1, 2);
    /* 09DD inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 09DE mov.b a,$17 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x17)));
    { abi_SpcDspWriteVoice(); return; }
}

/*
 * @routine 19:CC62 SpcVoiceBetweenTicks
 * WHAT IT DOES: Voice X's smoothing between music ticks: the pan fade, pitch slide and vibrato moved by the part of a tick elapsed ($51), and the DSP volume and pitch written when they moved.
 * HOW: SpcAddScaledDelta for each running fade; the vibrato through SpcVoiceTick's code at $0FB2.
 * Entry: X = voice * 2, $47 = its bit. Exit: A, Y changed.
 * @spc 0FD6
 * @spcuses 095D-09EF,0DF4-0DFF,0E11-0E1B,0EA6-0EE4,0FB2-0FD0,0FD2-107A
 */
void abi_SpcVoiceBetweenTicks(void)
{
    /* 0FD6 clr1 $13.7 */
    spc_bit_set_cyc(0x13, 7, 0, 3, 1);
    /* 0FD8 mov.b a,$C1+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0xC1 + spc.x))));
    /* 0FDA beq $0FE5 */
    if (spc.z) { spc_cyc(4); goto L0FE5; }
    spc_cyc(2);
    /* 0FDC mov.w a,$02E0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x02E0 + spc.x)));
    /* 0FDF cbne $C0+x,$0FE5 */
    spc_cyc(4);
    if (spc.a != spc_rd(spc_dp((uint8_t)(0xC0 + spc.x)))) { spc_cyc(4); goto L0FE5; }
    spc_cyc(2);
    /* 0FE2 call $104F */
    spc_call_cyc(abi_SpcTremoloBetweenTicks, 0x0FE5, 5, 1, 2);
L0FE5:
    /* 0FE5 mov.w a,$0331+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0331 + spc.x)));
    /* 0FE8 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0FE9 mov.w a,$0330+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0330 + spc.x)));
    /* 0FEC movw $10,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x10));
    spc_cyc(1);
    spc_wr(spc_dp(0x10), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x11), spc.y);
    /* 0FEE mov.b a,$91+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x91 + spc.x))));
    /* 0FF0 beq $0FFC */
    if (spc.z) { spc_cyc(4); goto L0FFC; }
    spc_cyc(2);
    /* 0FF2 mov.w a,$0341+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0341 + spc.x)));
    /* 0FF5 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0FF6 mov.w a,$0340+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0340 + spc.x)));
    /* 0FF9 call $1031 */
    spc_call_cyc(abi_SpcAddScaledDelta, 0x0FFC, 5, 1, 2);
L0FFC:
    /* 0FFC bbc $13.7,$1002 */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x13)) >> 7 & 1)) { spc_cyc(4); goto L1002; }
    spc_cyc(2);
    /* 0FFF call $0EA6 */
    spc_call_cyc(abi_SpcVoiceVolumeLR, 0x1002, 5, 1, 2);
L1002:
    /* 1002 clr1 $13.7 */
    spc_bit_set_cyc(0x13, 7, 0, 3, 1);
    /* 1004 call $0DF4 */
    spc_call_cyc(abi_SpcLoadPitch, 0x1007, 5, 1, 2);
    /* 1007 mov.b a,$A0+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0xA0 + spc.x))));
    /* 1009 beq $1019 */
    if (spc.z) { spc_cyc(4); goto L1019; }
    spc_cyc(2);
    /* 100B mov.b a,$A1+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0xA1 + spc.x))));
    /* 100D bne $1019 */
    if (!spc.z) { spc_cyc(4); goto L1019; }
    spc_cyc(2);
    /* 100F mov.w a,$0371+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0371 + spc.x)));
    /* 1012 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 1013 mov.w a,$0370+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0370 + spc.x)));
    /* 1016 call $1031 */
    spc_call_cyc(abi_SpcAddScaledDelta, 0x1019, 5, 1, 2);
L1019:
    /* 1019 mov.b a,$B1+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0xB1 + spc.x))));
    /* 101B beq $0FD2 */
    if (spc.z) { spc_cyc(4); goto L0FD2; }
    spc_cyc(2);
    /* 101D mov.w a,$02B0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x02B0 + spc.x)));
    /* 1020 cbne $B0+x,$0FD2 */
    spc_cyc(4);
    if (spc.a != spc_rd(spc_dp((uint8_t)(0xB0 + spc.x)))) { spc_cyc(4); goto L0FD2; }
    spc_cyc(2);
    /* 1023 mov.b y,$51 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x51)));
    /* 1025 mov.w a,$02A1+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x02A1 + spc.x)));
    /* 1028 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 1029 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 102A clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 102B adc.w a,$02A0+x */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x02A0 + spc.x)));
    /* 102E jmp $0FB2 */
    spc_cyc(3);
    goto L0FB2;
L095D:
    /* 095D mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 095F mov.b a,$11 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x11)));
    /* 0961 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 0962 sbc a,#$34 */
    spc.a = spc_sbc(spc.a, 0x34);
    spc_cyc(2);
    /* 0964 bcs $096F */
    if (spc.c) { spc_cyc(4); goto L096F; }
    spc_cyc(2);
    /* 0966 mov.b a,$11 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x11)));
    /* 0968 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 0969 sbc a,#$13 */
    spc.a = spc_sbc(spc.a, 0x13);
    spc_cyc(2);
    /* 096B bcs $0973 */
    if (spc.c) { spc_cyc(4); goto L0973; }
    spc_cyc(2);
    /* 096D dec y */
    spc.y = spc_dec(spc.y);
    spc_cyc(2);
    /* 096E asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
L096F:
    /* 096F addw ya,$10 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x10)); v |= (spc_cyc(2), spc_rd(spc_dp(0x11))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 0971 movw $10,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x10));
    spc_cyc(1);
    spc_wr(spc_dp(0x10), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x11), spc.y);
L0973:
    /* 0973 push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 0974 mov.b a,$11 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x11)));
    /* 0976 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 0977 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 0979 mov x,#$18 */
    spc.x = spc_nz(0x18);
    spc_cyc(2);
    /* 097B div ya,x */
    spc_div();
    spc_cyc(12);
    /* 097C mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 097D mov.w a,$06CE+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x06CE + spc.y)));
    /* 0980 mov.b $15,a */
    spc_st_cyc(spc_dp(0x15), spc.a, 3, 1);
    /* 0982 mov.w a,$06CD+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x06CD + spc.y)));
    /* 0985 mov.b $14,a */
    spc_st_cyc(spc_dp(0x14), spc.a, 3, 1);
    /* 0987 mov.w a,$06D0+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x06D0 + spc.y)));
    /* 098A push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 098B mov.w a,$06CF+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x06CF + spc.y)));
    /* 098E pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 098F subw ya,$14 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x14)); v |= (spc_cyc(2), spc_rd(spc_dp(0x15))) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 0991 mov.b y,$10 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x10)));
    /* 0993 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 0994 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 0995 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 0997 addw ya,$14 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x14)); v |= (spc_cyc(2), spc_rd(spc_dp(0x15))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 0999 mov.b $15,y */
    spc_st_cyc(spc_dp(0x15), spc.y, 3, 1);
    /* 099B asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 099C rol.b $15 */
    spc_rmw_cyc(spc_dp(0x15), spc_rol, 3, 1);
    /* 099E mov.b $14,a */
    spc_st_cyc(spc_dp(0x14), spc.a, 3, 1);
    /* 09A0 bra $09A6 */
    spc_cyc(4);
    goto L09A6;
L09A2:
    /* 09A2 lsr.b $15 */
    spc_rmw_cyc(spc_dp(0x15), spc_lsr, 3, 1);
    /* 09A4 ror a */
    spc.a = spc_ror(spc.a);
    spc_cyc(2);
    /* 09A5 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
L09A6:
    /* 09A6 cmp x,#$06 */
    spc_cmp(spc.x, 0x06);
    spc_cyc(2);
    /* 09A8 bne $09A2 */
    if (!spc.z) { spc_cyc(4); goto L09A2; }
    spc_cyc(2);
    /* 09AA mov.b $14,a */
    spc_st_cyc(spc_dp(0x14), spc.a, 3, 1);
    /* 09AC pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 09AD mov.w a,$0220+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0220 + spc.x)));
    /* 09B0 mov.b y,$15 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x15)));
    /* 09B2 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 09B3 movw $16,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x16));
    spc_cyc(1);
    spc_wr(spc_dp(0x16), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x17), spc.y);
    /* 09B5 mov.w a,$0220+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0220 + spc.x)));
    /* 09B8 mov.b y,$14 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x14)));
    /* 09BA mul ya */
    spc_mul();
    spc_cyc(9);
    /* 09BB push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 09BC mov.w a,$0221+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0221 + spc.x)));
    /* 09BF mov.b y,$14 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x14)));
    /* 09C1 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 09C2 addw ya,$16 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x16)); v |= (spc_cyc(2), spc_rd(spc_dp(0x17))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 09C4 movw $16,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x16));
    spc_cyc(1);
    spc_wr(spc_dp(0x16), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x17), spc.y);
    /* 09C6 mov.w a,$0221+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0221 + spc.x)));
    /* 09C9 mov.b y,$15 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x15)));
    /* 09CB mul ya */
    spc_mul();
    spc_cyc(9);
    /* 09CC mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 09CD pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 09CE addw ya,$16 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x16)); v |= (spc_cyc(2), spc_rd(spc_dp(0x17))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 09D0 movw $16,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x16));
    spc_cyc(1);
    spc_wr(spc_dp(0x16), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x17), spc.y);
    /* 09D2 mov a,x */
    spc.a = spc_nz(spc.x);
    spc_cyc(2);
    /* 09D3 xcn a */
    spc.a = spc_xcn(spc.a);
    spc_cyc(5);
    /* 09D4 lsr a */
    spc.a = spc_lsr(spc.a);
    spc_cyc(2);
    /* 09D5 or a,#$02 */
    spc.a = spc_or(spc.a, 0x02);
    spc_cyc(2);
    /* 09D7 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 09D8 mov.b a,$16 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x16)));
    /* 09DA call $09E0 */
    spc_call_cyc(abi_SpcDspWriteVoice, 0x09DD, 5, 1, 2);
    /* 09DD inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 09DE mov.b a,$17 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x17)));
    { abi_SpcDspWriteVoice(); return; }
L0FB2:
    /* 0FB2 mov.b $12,a */
    spc_st_cyc(spc_dp(0x12), spc.a, 3, 1);
    /* 0FB4 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 0FB5 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 0FB6 bcc $0FBA */
    if (!spc.c) { spc_cyc(4); goto L0FBA; }
    spc_cyc(2);
    /* 0FB8 eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    spc_cyc(2);
L0FBA:
    /* 0FBA mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 0FBB mov.b a,$B1+x */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0xB1 + spc.x))));
    /* 0FBD cmp a,#$F1 */
    spc_cmp(spc.a, 0xF1);
    spc_cyc(2);
    /* 0FBF bcc $0FC6 */
    if (!spc.c) { spc_cyc(4); goto L0FC6; }
    spc_cyc(2);
    /* 0FC1 and a,#$0F */
    spc.a = spc_and(spc.a, 0x0F);
    spc_cyc(2);
    /* 0FC3 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 0FC4 bra $0FCA */
    spc_cyc(4);
    goto L0FCA;
L0FC6:
    /* 0FC6 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 0FC7 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 0FC8 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
L0FCA:
    /* 0FCA call $1047 */
    spc_call_cyc(abi_SpcAddDelta, 0x0FCD, 5, 1, 2);
L0FCD:
    /* 0FCD jmp $095D */
    spc_cyc(3);
    goto L095D;
L0FD2:
    /* 0FD2 bbs $13.7,$0FCD */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x13)) >> 7 & 1)) { spc_cyc(4); goto L0FCD; }
    spc_cyc(2);
    /* 0FD5 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:CCBD SpcAddScaledDelta
 * WHAT IT DOES: Adds the 16-bit delta YA scaled by the tick fraction $51 to the value at $10/$11, and marks the voice changed (bit 7 of $13).
 * HOW: Two MULs by $51, the sign kept in $12; then SpcAddDelta.
 * Entry: YA = delta, $10/$11 = value. Exit: $10/$11 updated.
 * @spc 1031
 * @spcuses 0E11-0E1B,1031-104F
 */
void abi_SpcAddScaledDelta(void)
{
    /* 1031 set1 $13.7 */
    spc_bit_set_cyc(0x13, 7, 1, 3, 1);
    /* 1033 mov.b $12,y */
    spc_st_cyc(spc_dp(0x12), spc.y, 3, 1);
    /* 1035 call $0E11 */
    spc_call_cyc(abi_SpcNegateIfNeg, 0x1038, 5, 1, 2);
    /* 1038 push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 1039 mov.b y,$51 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x51)));
    /* 103B mul ya */
    spc_mul();
    spc_cyc(9);
    /* 103C mov.b $14,y */
    spc_st_cyc(spc_dp(0x14), spc.y, 3, 1);
    /* 103E mov $15,#$00 */
    spc_st_cyc(spc_dp(0x15), 0x00, 4, 1);
    /* 1041 mov.b y,$51 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x51)));
    /* 1043 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 1044 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 1045 addw ya,$14 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x14)); v |= (spc_cyc(2), spc_rd(spc_dp(0x15))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    { abi_SpcAddDelta(); return; }
}

/*
 * @routine 19:CCD3 SpcAddDelta
 * WHAT IT DOES: Adds YA (negated when $12 bit 7 is set) to $10/$11.
 * HOW: SpcNegateIfNeg, ADDW.
 * Entry: YA, $12. Exit: YA = $10/$11 = the sum.
 * @spc 1047
 * @spcuses 0E11-0E1B,1047-104F
 */
void abi_SpcAddDelta(void)
{
    /* 1047 call $0E11 */
    spc_call_cyc(abi_SpcNegateIfNeg, 0x104A, 5, 1, 2);
    /* 104A addw ya,$10 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x10)); v |= (spc_cyc(2), spc_rd(spc_dp(0x11))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 104C movw $10,ya */
    spc_cyc(3);
    spc_rd(spc_dp(0x10));
    spc_cyc(1);
    spc_wr(spc_dp(0x10), spc.a);
    spc_cyc(1);
    spc_wr(spc_dp(0x11), spc.y);
    /* 104E ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:CCDB SpcTremoloBetweenTicks
 * WHAT IT DOES: Voice X's tremolo moved by the part of a tick elapsed ($51), then its volume factor (SpcTremoloFactor's code).
 * HOW: The rate $02D1 times $51 added to the phase $02D0; sets bit 7 of $13.
 * Entry: X = voice * 2. Exit: as SpcVolumeCalc.
 * @spc 104F
 * @spcuses 104F-107A
 */
void abi_SpcTremoloBetweenTicks(void)
{
    /* 104F set1 $13.7 */
    spc_bit_set_cyc(0x13, 7, 1, 3, 1);
    /* 1051 mov.b y,$51 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x51)));
    /* 1053 mov.w a,$02D1+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x02D1 + spc.x)));
    /* 1056 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 1057 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 1058 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 1059 adc.w a,$02D0+x */
    spc_cyc(5);
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x02D0 + spc.x)));
    { abi_SpcTremoloFactor(); return; }
}

/*
 * @routine 19:CCE8 SpcTremoloFactor
 * WHAT IT DOES: The tremolo's volume factor from its phase A: a triangle of the phase times the depth $C1, inverted, then the volume (SpcVolumeCalc).
 * HOW: ASL with the carry folding the wave, MUL by the depth.
 * Entry: A = phase, X = voice * 2. Exit: as SpcVolumeCalc.
 * @spc 105C
 * @spcuses 105C-107A
 */
void abi_SpcTremoloFactor(void)
{
    /* 105C asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 105D bcc $1061 */
    if (!spc.c) { spc_cyc(4); goto L1061; }
    spc_cyc(2);
    /* 105F eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    spc_cyc(2);
L1061:
    /* 1061 mov.b y,$C1+x */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(spc_dp((uint8_t)(0xC1 + spc.x))));
    /* 1063 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 1064 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 1065 eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    { spc_cyc(2); abi_SpcVolumeCalc(); return; }
}

/*
 * @routine 19:CCF3 SpcVolumeCalc
 * WHAT IT DOES: Voice X's output volume $0321: A (the tremolo factor, $FF for none) times the global scale $0565, the note velocity and the voice volume, squared.
 * HOW: A chain of MULs keeping the high bytes.
 * Entry: A = tremolo factor, X = voice * 2. Exit: A = the volume.
 * @spc 1067
 * @spcuses 1067-107A
 */
void abi_SpcVolumeCalc(void)
{
    /* 1067 mov.w y,$0565 */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x0565));
    /* 106A mul ya */
    spc_mul();
    spc_cyc(9);
    /* 106B mov.w a,$0210+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0210 + spc.x)));
    /* 106E mul ya */
    spc_mul();
    spc_cyc(9);
    /* 106F mov.w a,$0301+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0301 + spc.x)));
    /* 1072 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 1073 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 1074 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 1075 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 1076 mov.w $0321+x,a */
    spc_st_cyc((uint16_t)(0x0321 + spc.x), spc.a, 5, 1);
    /* 1079 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:CD3A SpcSfxFrame
 * WHAT IT DOES: The game's sound tick: a new port 0 byte (one differing from the last, $04) becomes the command $00 (else $00 = 0); ports 1-3 copied to $01-$03 and $05-$07; a stop request (bit 1 of $29) clears bits 0 and 1; then the tick's game sound work in order (command remaps, echo and tempo effects, port commands, channel stops, the sound queue, timers, pitch and fade ramps, noise sample) and the sound effect channels (through the queue start at $1C07).
 * HOW: A chain of CALLs, then SpcPort1Command and a JMP to the queue start.
 * Entry: none (called from SpcReset's loop). Exit: as the sound effect channels leave it.
 * @spc 10B0
 * @spcuses 09E0-09EF,0A3B-0A3C,0A58-0A6D,0BC1-0C17,0E97-0EE4,10B0-16D8,16F1-1B8C,1B92-1C04,1C07-1DC1,1DCE-23EB,23FB-2419,2441-26D6,CA75-CAA8,CAC0-CB04,EDF0-EE20
 */
void abi_SpcSfxFrame(void)
{
    /* 10B0 mov.w a,$0558 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0558));
    /* 10B3 cmp.b a,$04 */
    spc_cyc(3);
    spc_cmp(spc.a, spc_rd(spc_dp(0x04)));
    /* 10B5 beq $10C0 */
    if (spc.z) { spc_cyc(4); goto L10C0; }
    spc_cyc(2);
    /* 10B7 mov.w a,$0558 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0558));
    /* 10BA mov.b $04,a */
    spc_st_cyc(spc_dp(0x04), spc.a, 3, 1);
    /* 10BC mov.b $00,a */
    spc_st_cyc(spc_dp(0x00), spc.a, 3, 1);
    /* 10BE bra $10C3 */
    spc_cyc(4);
    goto L10C3;
L10C0:
    /* 10C0 mov $00,#$00 */
    spc_st_cyc(spc_dp(0x00), 0x00, 4, 1);
L10C3:
    /* 10C3 mov.w a,$0559 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0559));
    /* 10C6 mov.b $05,a */
    spc_st_cyc(spc_dp(0x05), spc.a, 3, 1);
    /* 10C8 mov.b $01,a */
    spc_st_cyc(spc_dp(0x01), spc.a, 3, 1);
    /* 10CA mov.w a,$055A */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x055A));
    /* 10CD mov.b $06,a */
    spc_st_cyc(spc_dp(0x06), spc.a, 3, 1);
    /* 10CF mov.b $02,a */
    spc_st_cyc(spc_dp(0x02), spc.a, 3, 1);
    /* 10D1 mov.w a,$055B */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x055B));
    /* 10D4 mov.b $07,a */
    spc_st_cyc(spc_dp(0x07), spc.a, 3, 1);
    /* 10D6 mov.b $03,a */
    spc_st_cyc(spc_dp(0x03), spc.a, 3, 1);
    /* 10D8 bbc $29.1,$10DF */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x29)) >> 1 & 1)) { spc_cyc(4); goto L10DF; }
    spc_cyc(2);
    /* 10DB clr1 $29.0 */
    spc_bit_set_cyc(0x29, 0, 0, 3, 1);
    /* 10DD clr1 $29.1 */
    spc_bit_set_cyc(0x29, 1, 0, 3, 1);
L10DF:
    /* 10DF call $111B */
    spc_call_cyc(abi_SpcPort1Remap, 0x10E2, 5, 1, 2);
    /* 10E2 call $15C4 */
    spc_call_cyc(abi_SpcEchoTransition, 0x10E5, 5, 1, 2);
    /* 10E5 call $1AB0 */
    spc_call_cyc(abi_SpcMusicFade, 0x10E8, 5, 1, 2);
    /* 10E8 call $17A5 */
    spc_call_cyc(abi_SpcPortCommand, 0x10EB, 5, 1, 2);
    /* 10EB call $112F */
    spc_call_cyc(abi_SpcChannelGuards, 0x10EE, 5, 1, 2);
    /* 10EE call $1B7A */
    spc_call_cyc(abi_SpcCommandBlock, 0x10F1, 5, 1, 2);
    /* 10F1 call $1B2C */
    spc_call_cyc(abi_SpcCommandChannelCheck, 0x10F4, 5, 1, 2);
    /* 10F4 call $1523 */
    spc_call_cyc(abi_SpcTempoBend, 0x10F7, 5, 1, 2);
    /* 10F7 call $117C */
    spc_call_cyc(abi_SpcStaggeredChecks, 0x10FA, 5, 1, 2);
    /* 10FA call $11F8 */
    spc_call_cyc(abi_SpcGameStateTick, 0x10FD, 5, 1, 2);
    /* 10FD call $1B92 */
    spc_call_cyc(abi_SpcSfxHolds, 0x1100, 5, 1, 2);
    /* 1100 call $16F1 */
    spc_call_cyc(abi_SpcPitchRampB, 0x1103, 5, 1, 2);
    /* 1103 call $174B */
    spc_call_cyc(abi_SpcPitchRampA, 0x1106, 5, 1, 2);
    /* 1106 call $16C1 */
    spc_call_cyc(abi_SpcCommandFilter, 0x1109, 5, 1, 2);
    /* 1109 call $13FA */
    spc_call_cyc(abi_SpcCommand4C, 0x110C, 5, 1, 2);
    /* 110C call $1436 */
    spc_call_cyc(abi_SpcCommandTimers, 0x110F, 5, 1, 2);
    /* 110F call $CAC0 */
    spc_call_cyc(abi_SpcNoiseSample, 0x1112, 5, 1, 2);
    /* 1112 call $16B1 */
    spc_call_cyc(abi_SpcPort1Clamp, 0x1115, 5, 1, 2);
    /* 1115 call $17DF */
    spc_call_cyc(abi_SpcPort1Command, 0x1118, 5, 1, 2);
    /* 1118 jmp $1C07 */
    spc_cyc(3);
    goto L1C07;
L1C07:
    /* 1C07 mov.b a,$1D */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 1C09 call $1424 */
    spc_call_cyc(abi_SpcSoundQueuePush, 0x1C0C, 5, 1, 2);
    /* 1C0C mov $1D,#$00 */
    spc_st_cyc(spc_dp(0x1D), 0x00, 4, 1);
    /* 1C0F cmp $1D,#$00 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x00);
    spc_cyc(1);
    /* 1C12 beq $1C1B */
    if (spc.z) { spc_cyc(4); goto L1C1B; }
    spc_cyc(2);
    /* 1C14 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1C16 mov.w $057B,a */
    spc_st_cyc(0x057B, spc.a, 4, 1);
    /* 1C19 bra $1C25 */
    spc_cyc(4);
    goto L1C25;
L1C1B:
    /* 1C1B mov.w a,$057B */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x057B));
    /* 1C1E cmp a,#$FF */
    spc_cmp(spc.a, 0xFF);
    spc_cyc(2);
    /* 1C20 beq $1C25 */
    if (spc.z) { spc_cyc(4); goto L1C25; }
    spc_cyc(2);
    /* 1C22 inc.w $057B */
    spc_rmw_cyc(0x057B, spc_inc, 4, 1);
L1C25:
    /* 1C25 cmp $1D,#$00 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x00);
    spc_cyc(1);
    /* 1C28 bne $1C54 */
    if (!spc.z) { spc_cyc(4); goto L1C54; }
    spc_cyc(2);
    /* 1C2A mov.w a,$057B */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x057B));
    /* 1C2D beq $1C54 */
    if (spc.z) { spc_cyc(4); goto L1C54; }
    spc_cyc(2);
    /* 1C2F cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 1C31 beq $1C54 */
    if (spc.z) { spc_cyc(4); goto L1C54; }
    spc_cyc(2);
    /* 1C33 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    spc_cyc(2);
    /* 1C35 beq $1C54 */
    if (spc.z) { spc_cyc(4); goto L1C54; }
    spc_cyc(2);
    /* 1C37 mov.w a,$0588 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0588));
    /* 1C3A beq $1C54 */
    if (spc.z) { spc_cyc(4); goto L1C54; }
    spc_cyc(2);
    /* 1C3C push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 1C3D mov x,#$01 */
    spc.x = spc_nz(0x01);
    spc_cyc(2);
L1C3F:
    /* 1C3F mov.w a,$0588+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0588 + spc.x)));
    /* 1C42 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 1C43 mov.w $0588+x,a */
    spc_st_cyc((uint16_t)(0x0588 + spc.x), spc.a, 5, 1);
    /* 1C46 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 1C47 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 1C48 cmp x,#$08 */
    spc_cmp(spc.x, 0x08);
    spc_cyc(2);
    /* 1C4A bne $1C3F */
    if (!spc.z) { spc_cyc(4); goto L1C3F; }
    spc_cyc(2);
    /* 1C4C pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 1C4D mov x,#$00 */
    spc.x = spc_nz(0x00);
    spc_cyc(2);
    /* 1C4F mov.w $057B,x */
    spc_st_cyc(0x057B, spc.x, 4, 1);
    /* 1C52 bra $1C63 */
    spc_cyc(4);
    goto L1C63;
L1C54:
    /* 1C54 mov.b a,$1D */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 1C56 cmp.b a,$1E */
    spc_cyc(3);
    spc_cmp(spc.a, spc_rd(spc_dp(0x1E)));
    /* 1C58 bne $1C5D */
    if (!spc.z) { spc_cyc(4); goto L1C5D; }
    spc_cyc(2);
L1C5A:
    /* 1C5A jmp $1CCD */
    { spc_cyc(3); abi_SpcSfxChannelsFrame(); return; }
L1C5D:
    /* 1C5D mov.b $1E,a */
    spc_st_cyc(spc_dp(0x1E), spc.a, 3, 1);
    /* 1C5F mov.b a,$1D */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 1C61 beq $1C5A */
    if (spc.z) { spc_cyc(4); goto L1C5A; }
    spc_cyc(2);
L1C63:
    /* 1C63 mov.b $23,a */
    spc_st_cyc(spc_dp(0x23), spc.a, 3, 1);
    /* 1C65 call $1C8A */
    spc_call_cyc(abi_SpcSfxLookup, 0x1C68, 5, 1, 2);
    /* 1C68 call $1C9E */
    spc_call_cyc(abi_SpcSfxChannel, 0x1C6B, 5, 1, 2);
    /* 1C6B call $1CA5 */
    spc_call_cyc(abi_SpcSfxMode, 0x1C6E, 5, 1, 2);
    /* 1C6E call $1C84 */
    spc_call_cyc(abi_SpcSfxActivate, 0x1C71, 5, 1, 2);
    /* 1C71 call $1CED */
    spc_call_cyc(abi_SpcSfxChannelSave, 0x1C74, 5, 1, 2);
    /* 1C74 mov.b a,$23 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x23)));
    /* 1C76 mov.w $0410+x,a */
    spc_st_cyc((uint16_t)(0x0410 + spc.x), spc.a, 5, 1);
    /* 1C79 push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 1C7A call $1CCD */
    spc_call_cyc(abi_SpcSfxChannelsFrame, 0x1C7D, 5, 1, 2);
    /* 1C7D pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 1C7E mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1C80 mov.w $0410+x,a */
    spc_st_cyc((uint16_t)(0x0410 + spc.x), spc.a, 5, 1);
    /* 1C83 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:CDA5 SpcPort1Remap
 * WHAT IT DOES: With bit 7 of $2D set, port 1 command $09 becomes $12 and $0A becomes $0C.
 * HOW: CMP and MOV on $01.
 * Entry: none. Exit: flags from the last compare.
 * @spc 111B
 * @spcuses 111B-112F
 */
void abi_SpcPort1Remap(void)
{
    /* 111B bbc $2D.7,$112E */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x2D)) >> 7 & 1)) { spc_cyc(4); goto L112E; }
    spc_cyc(2);
    /* 111E cmp $01,#$09 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x01)), 0x09);
    spc_cyc(1);
    /* 1121 bne $1126 */
    if (!spc.z) { spc_cyc(4); goto L1126; }
    spc_cyc(2);
    /* 1123 mov $01,#$12 */
    spc_st_cyc(spc_dp(0x01), 0x12, 4, 1);
L1126:
    /* 1126 cmp $01,#$0A */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x01)), 0x0A);
    spc_cyc(1);
    /* 1129 bne $112E */
    if (!spc.z) { spc_cyc(4); goto L112E; }
    spc_cyc(2);
    /* 112B mov $01,#$0C */
    spc_st_cyc(spc_dp(0x01), 0x0C, 4, 1);
L112E:
    /* 112E ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:CDB9 SpcChannelGuards
 * WHAT IT DOES: With bit 6 of $2D, stops channel 5 unless it plays sound $43; with bit 5, channel 7 stops when track $0397 is 1, 4, 7, $0A or $0D and channel 6 when channel 6's sound $0396 is $72-$76.
 * HOW: Compare chains into SpcSfxStop.
 * Entry: none. Exit: as SpcSfxStop or a RET.
 * @spc 112F
 * @spcuses 112E-113E,1941-1951
 */
void abi_SpcChannelGuards(void)
{
    /* 112F bbc $2D.6,$112E */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x2D)) >> 6 & 1)) { spc_cyc(4); goto L112E; }
    spc_cyc(2);
    /* 1132 mov.w a,$0395 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0395));
    /* 1135 cmp a,#$43 */
    spc_cmp(spc.a, 0x43);
    spc_cyc(2);
    /* 1137 beq $112E */
    if (spc.z) { spc_cyc(4); goto L112E; }
    spc_cyc(2);
    /* 1139 mov x,#$05 */
    spc.x = spc_nz(0x05);
    spc_cyc(2);
    /* 113B jmp $194C */
    { spc_cyc(3); abi_SpcSfxStop(); return; }
L112E:
    /* 112E ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:CE06 SpcStaggeredChecks
 * WHAT IT DOES: Steps the counter $0581 through 0-$16 each tick and at 3, 9, $0C, $0F, $12 and $15 does one check: restarting a looping sound (channels by bits of $2A and $2D, stop through SpcSfxStopIf with $20 = the step) or queueing sound $17, $25 or $21; outside song 1, $2A is cleared.
 * HOW: A compare chain on the counter; the stop helpers are SpcSfxStopIf's entries.
 * Entry: none. Exit: as the chosen path leaves it.
 * @spc 117C
 * @spcuses 113E-11F8,1424-1436,1908-1951
 */
void abi_SpcStaggeredChecks(void)
{
    /* 117C inc.w $0581 */
    spc_rmw_cyc(0x0581, spc_inc, 4, 1);
    /* 117F mov.w a,$0581 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0581));
    /* 1182 cmp a,#$03 */
    spc_cmp(spc.a, 0x03);
    spc_cyc(2);
    /* 1184 beq $11AC */
    if (spc.z) { spc_cyc(4); goto L11AC; }
    spc_cyc(2);
    /* 1186 cmp a,#$09 */
    spc_cmp(spc.a, 0x09);
    spc_cyc(2);
    /* 1188 beq $113E */
    if (spc.z) { spc_cyc(4); goto L113E; }
    spc_cyc(2);
    /* 118A cmp a,#$0C */
    spc_cmp(spc.a, 0x0C);
    spc_cyc(2);
    /* 118C beq $11BC */
    if (spc.z) { spc_cyc(4); goto L11BC; }
    spc_cyc(2);
    /* 118E cmp a,#$0F */
    spc_cmp(spc.a, 0x0F);
    spc_cyc(2);
    /* 1190 beq $11CA */
    if (spc.z) { spc_cyc(4); goto L11CA; }
    spc_cyc(2);
    /* 1192 cmp a,#$12 */
    spc_cmp(spc.a, 0x12);
    spc_cyc(2);
    /* 1194 beq $11E1 */
    if (spc.z) { spc_cyc(4); goto L11E1; }
    spc_cyc(2);
    /* 1196 cmp a,#$15 */
    spc_cmp(spc.a, 0x15);
    spc_cyc(2);
    /* 1198 beq $11EA */
    if (spc.z) { spc_cyc(4); goto L11EA; }
    spc_cyc(2);
    /* 119A sbc a,#$16 */
    spc.a = spc_sbc(spc.a, 0x16);
    spc_cyc(2);
    /* 119C bmi $11A3 */
    if (spc.n) { spc_cyc(4); goto L11A3; }
    spc_cyc(2);
    /* 119E mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 11A0 mov.w $0581,a */
    spc_st_cyc(0x0581, spc.a, 4, 1);
L11A3:
    /* 11A3 cmp $1C,#$01 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1C)), 0x01);
    spc_cyc(1);
    /* 11A6 beq $11AB */
    if (spc.z) { spc_cyc(4); goto L11AB; }
    spc_cyc(2);
    /* 11A8 mov $2A,#$00 */
    spc_st_cyc(spc_dp(0x2A), 0x00, 4, 1);
L11AB:
    /* 11AB ret */
    spc_ret_cyc(4, 1);
    return;
L11AC:
    /* 11AC bbc $2D.3,$11A3 */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x2D)) >> 3 & 1)) { spc_cyc(4); goto L11A3; }
    spc_cyc(2);
    /* 11AF mov.w a,$0394 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0394));
    /* 11B2 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    spc_cyc(2);
    /* 11B4 bne $11A3 */
    if (!spc.z) { spc_cyc(4); goto L11A3; }
    spc_cyc(2);
    /* 11B6 mov $20,#$08 */
    spc_st_cyc(spc_dp(0x20), 0x08, 4, 1);
    /* 11B9 jmp $1908 */
    { spc_cyc(3); abi_SpcSfxStopIf(); return; }
L11BC:
    /* 11BC bbs $2A.0,$11C5 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2A)) >> 0 & 1)) { spc_cyc(4); goto L11C5; }
    spc_cyc(2);
    /* 11BF mov $20,#$0C */
    spc_st_cyc(spc_dp(0x20), 0x0C, 4, 1);
    /* 11C2 jmp $192B */
    spc_cyc(3);
    goto L192B;
L11C5:
    /* 11C5 mov a,#$17 */
    spc.a = spc_nz(0x17);
    spc_cyc(2);
    /* 11C7 jmp $1424 */
    { spc_cyc(3); abi_SpcSoundQueuePush(); return; }
L11CA:
    /* 11CA bbs $2A.2,$11D3 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2A)) >> 2 & 1)) { spc_cyc(4); goto L11D3; }
    spc_cyc(2);
    /* 11CD mov $20,#$0A */
    spc_st_cyc(spc_dp(0x20), 0x0A, 4, 1);
    /* 11D0 jmp $1915 */
    spc_cyc(3);
    goto L1915;
L11D3:
    /* 11D3 bbs $2A.0,$11A3 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2A)) >> 0 & 1)) { spc_cyc(4); goto L11A3; }
    spc_cyc(2);
    /* 11D6 bbs $2A.7,$11A3 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2A)) >> 7 & 1)) { spc_cyc(4); goto L11A3; }
    spc_cyc(2);
    /* 11D9 bbs $2A.4,$11A3 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2A)) >> 4 & 1)) { spc_cyc(4); goto L11A3; }
    spc_cyc(2);
    /* 11DC mov a,#$25 */
    spc.a = spc_nz(0x25);
    spc_cyc(2);
    /* 11DE jmp $1424 */
    { spc_cyc(3); abi_SpcSoundQueuePush(); return; }
L11E1:
    /* 11E1 bbs $2A.4,$11A3 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2A)) >> 4 & 1)) { spc_cyc(4); goto L11A3; }
    spc_cyc(2);
    /* 11E4 mov $20,#$0B */
    spc_st_cyc(spc_dp(0x20), 0x0B, 4, 1);
    /* 11E7 jmp $1920 */
    spc_cyc(3);
    goto L1920;
L11EA:
    /* 11EA bbs $2A.7,$11F3 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2A)) >> 7 & 1)) { spc_cyc(4); goto L11F3; }
    spc_cyc(2);
    /* 11ED mov $20,#$0E */
    spc_st_cyc(spc_dp(0x20), 0x0E, 4, 1);
    /* 11F0 jmp $1936 */
    spc_cyc(3);
    goto L1936;
L11F3:
    /* 11F3 mov a,#$21 */
    spc.a = spc_nz(0x21);
    spc_cyc(2);
    /* 11F5 jmp $1424 */
    { spc_cyc(3); abi_SpcSoundQueuePush(); return; }
L1915:
    /* 1915 clr1 $2A.2 */
    spc_bit_set_cyc(0x2A, 2, 0, 3, 1);
    /* 1917 mov.w a,$0395 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0395));
    /* 191A cmp a,#$25 */
    spc_cmp(spc.a, 0x25);
    spc_cyc(2);
    /* 191C bne $194B */
    if (!spc.z) { spc_cyc(4); goto L194B; }
    spc_cyc(2);
    /* 191E bra $193F */
    spc_cyc(4);
    goto L193F;
L1920:
    /* 1920 clr1 $2A.4 */
    spc_bit_set_cyc(0x2A, 4, 0, 3, 1);
    /* 1922 mov.w a,$0395 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0395));
    /* 1925 cmp a,#$15 */
    spc_cmp(spc.a, 0x15);
    spc_cyc(2);
    /* 1927 bne $194B */
    if (!spc.z) { spc_cyc(4); goto L194B; }
    spc_cyc(2);
    /* 1929 bra $193F */
    spc_cyc(4);
    goto L193F;
L192B:
    /* 192B clr1 $2A.0 */
    spc_bit_set_cyc(0x2A, 0, 0, 3, 1);
    /* 192D mov.w a,$0395 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0395));
    /* 1930 cmp a,#$17 */
    spc_cmp(spc.a, 0x17);
    spc_cyc(2);
    /* 1932 bne $194B */
    if (!spc.z) { spc_cyc(4); goto L194B; }
    spc_cyc(2);
    /* 1934 bra $193F */
    spc_cyc(4);
    goto L193F;
L1936:
    /* 1936 clr1 $2A.7 */
    spc_bit_set_cyc(0x2A, 7, 0, 3, 1);
    /* 1938 mov.w a,$0395 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0395));
    /* 193B cmp a,#$21 */
    spc_cmp(spc.a, 0x21);
    spc_cyc(2);
    /* 193D bne $194B */
    if (!spc.z) { spc_cyc(4); goto L194B; }
    spc_cyc(2);
L193F:
    /* 193F mov x,#$05 */
    spc.x = spc_nz(0x05);
    spc_cyc(2);
    /* 1941 mov.w a,$0408+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 1944 beq $194B */
    if (spc.z) { spc_cyc(4); goto L194B; }
    spc_cyc(2);
    /* 1946 mov.b a,$20 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 1948 mov.w $03E0+x,a */
    spc_st_cyc((uint16_t)(0x03E0 + spc.x), spc.a, 5, 1);
L194B:
    /* 194B ret */
    spc_ret_cyc(4, 1);
    return;
L113E:
    /* 113E bbc $2D.5,$116F */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x2D)) >> 5 & 1)) { spc_cyc(4); goto L116F; }
    spc_cyc(2);
    /* 1141 mov.w a,$0397 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0397));
    /* 1144 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 1146 beq $1170 */
    if (spc.z) { spc_cyc(4); goto L1170; }
    spc_cyc(2);
    /* 1148 cmp a,#$04 */
    spc_cmp(spc.a, 0x04);
    spc_cyc(2);
    /* 114A beq $1170 */
    if (spc.z) { spc_cyc(4); goto L1170; }
    spc_cyc(2);
    /* 114C cmp a,#$07 */
    spc_cmp(spc.a, 0x07);
    spc_cyc(2);
    /* 114E beq $1170 */
    if (spc.z) { spc_cyc(4); goto L1170; }
    spc_cyc(2);
    /* 1150 cmp a,#$0A */
    spc_cmp(spc.a, 0x0A);
    spc_cyc(2);
    /* 1152 beq $1170 */
    if (spc.z) { spc_cyc(4); goto L1170; }
    spc_cyc(2);
    /* 1154 cmp a,#$0D */
    spc_cmp(spc.a, 0x0D);
    spc_cyc(2);
    /* 1156 beq $1170 */
    if (spc.z) { spc_cyc(4); goto L1170; }
    spc_cyc(2);
L1158:
    /* 1158 mov.w a,$0396 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0396));
    /* 115B cmp a,#$72 */
    spc_cmp(spc.a, 0x72);
    spc_cyc(2);
    /* 115D beq $1177 */
    if (spc.z) { spc_cyc(4); goto L1177; }
    spc_cyc(2);
    /* 115F cmp a,#$73 */
    spc_cmp(spc.a, 0x73);
    spc_cyc(2);
    /* 1161 beq $1177 */
    if (spc.z) { spc_cyc(4); goto L1177; }
    spc_cyc(2);
    /* 1163 cmp a,#$74 */
    spc_cmp(spc.a, 0x74);
    spc_cyc(2);
    /* 1165 beq $1177 */
    if (spc.z) { spc_cyc(4); goto L1177; }
    spc_cyc(2);
    /* 1167 cmp a,#$75 */
    spc_cmp(spc.a, 0x75);
    spc_cyc(2);
    /* 1169 beq $1177 */
    if (spc.z) { spc_cyc(4); goto L1177; }
    spc_cyc(2);
    /* 116B cmp a,#$76 */
    spc_cmp(spc.a, 0x76);
    spc_cyc(2);
    /* 116D beq $1177 */
    if (spc.z) { spc_cyc(4); goto L1177; }
    spc_cyc(2);
L116F:
    /* 116F ret */
    spc_ret_cyc(4, 1);
    return;
L1170:
    /* 1170 mov x,#$07 */
    spc.x = spc_nz(0x07);
    spc_cyc(2);
    /* 1172 call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x1175, 5, 1, 2);
    /* 1175 bra $1158 */
    spc_cyc(4);
    goto L1158;
L1177:
    /* 1177 mov x,#$06 */
    spc.x = spc_nz(0x06);
    spc_cyc(2);
    /* 1179 jmp $194C */
    { spc_cyc(3); abi_SpcSfxStop(); return; }
}

/*
 * @routine 19:CE82 SpcGameStateTick
 * WHAT IT DOES: The game state's sound rules each tick: the countdown flags in $2D from the bytes $EE40 and $CB10; the counters $056A/$056B after song request 5; song request 3 $E6 ticks after request 2 ($0567); a tempo-dependent value $0586; the finish music trigger ($055F, $0582, pattern list $EE48 with key-up $50); then the port commands $39, $32, $03 (stopping channels, pausing the music), the timer $0579 and port 1 clears for some channel 5 sounds.
 * HOW: Compare chains on $1D, $01, $0394-$0397; SpcSfxStop, SpcSfxStopRequest, SpcMusicPause, SpcSoundQueuePush.
 * Entry: none. Exit: A, X, Y changed.
 * @spc 11F8
 * @spcuses 09E0-09EF,0A3B-0A3C,0A58-0A6D,0BC1-0C17,0E97-0EE4,11F8-13FA,1424-1436,1908-1915,1941-1951,1A92-1AAA,1D63-1D69,1E0F-1E89,1E93-1E9D
 */
void abi_SpcGameStateTick(void)
{
    /* 11F8 mov.w a,$EE40 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0xEE40));
    /* 11FB cmp a,#$98 */
    spc_cmp(spc.a, 0x98);
    spc_cyc(2);
    /* 11FD bne $1204 */
    if (!spc.z) { spc_cyc(4); goto L1204; }
    spc_cyc(2);
    /* 11FF bbc $2D.4,$1204 */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x2D)) >> 4 & 1)) { spc_cyc(4); goto L1204; }
    spc_cyc(2);
    /* 1202 set1 $2D.7 */
    spc_bit_set_cyc(0x2D, 7, 1, 3, 1);
L1204:
    /* 1204 mov.w a,$CB10 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0xCB10));
    /* 1207 cmp a,#$14 */
    spc_cmp(spc.a, 0x14);
    spc_cyc(2);
    /* 1209 beq $1211 */
    if (spc.z) { spc_cyc(4); goto L1211; }
    spc_cyc(2);
    /* 120B mov a,#$04 */
    spc.a = spc_nz(0x04);
    spc_cyc(2);
    /* 120D clr1 $2D.4 */
    spc_bit_set_cyc(0x2D, 4, 0, 3, 1);
    /* 120F bra $1215 */
    spc_cyc(4);
    goto L1215;
L1211:
    /* 1211 mov a,#$1E */
    spc.a = spc_nz(0x1E);
    spc_cyc(2);
    /* 1213 set1 $2D.4 */
    spc_bit_set_cyc(0x2D, 4, 1, 3, 1);
L1215:
    /* 1215 mov.w $1C05,a */
    spc_st_cyc(0x1C05, spc.a, 4, 1);
    /* 1218 mov.w a,$056A */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x056A));
    /* 121B cmp a,#$FF */
    spc_cmp(spc.a, 0xFF);
    spc_cyc(2);
    /* 121D beq $1222 */
    if (spc.z) { spc_cyc(4); goto L1222; }
    spc_cyc(2);
    /* 121F inc.w $056A */
    spc_rmw_cyc(0x056A, spc_inc, 4, 1);
L1222:
    /* 1222 mov.b a,$1B */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1B)));
    /* 1224 cmp a,#$05 */
    spc_cmp(spc.a, 0x05);
    spc_cyc(2);
    /* 1226 beq $122A */
    if (spc.z) { spc_cyc(4); goto L122A; }
    spc_cyc(2);
    /* 1228 bra $1232 */
    spc_cyc(4);
    goto L1232;
L122A:
    /* 122A mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 122C mov.w $056A,a */
    spc_st_cyc(0x056A, spc.a, 4, 1);
    /* 122F mov.w $056B,a */
    spc_st_cyc(0x056B, spc.a, 4, 1);
L1232:
    /* 1232 mov.w a,$056A */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x056A));
    /* 1235 bmi $1239 */
    if (spc.n) { spc_cyc(4); goto L1239; }
    spc_cyc(2);
    /* 1237 bra $1243 */
    spc_cyc(4);
    goto L1243;
L1239:
    /* 1239 mov.w a,$056B */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x056B));
    /* 123C cmp a,#$FF */
    spc_cmp(spc.a, 0xFF);
    spc_cyc(2);
    /* 123E beq $1243 */
    if (spc.z) { spc_cyc(4); goto L1243; }
    spc_cyc(2);
    /* 1240 inc.w $056B */
    spc_rmw_cyc(0x056B, spc_inc, 4, 1);
L1243:
    /* 1243 cmp $1D,#$35 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x35);
    spc_cyc(1);
    /* 1246 bne $124D */
    if (!spc.z) { spc_cyc(4); goto L124D; }
    spc_cyc(2);
    /* 1248 mov a,#$FF */
    spc.a = spc_nz(0xFF);
    spc_cyc(2);
    /* 124A mov.w $056B,a */
    spc_st_cyc(0x056B, spc.a, 4, 1);
L124D:
    /* 124D cmp $1B,#$02 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1B)), 0x02);
    spc_cyc(1);
    /* 1250 bne $1257 */
    if (!spc.z) { spc_cyc(4); goto L1257; }
    spc_cyc(2);
    /* 1252 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 1254 mov.w $0567,a */
    spc_st_cyc(0x0567, spc.a, 4, 1);
L1257:
    /* 1257 mov.w a,$0567 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0567));
    /* 125A beq $126E */
    if (spc.z) { spc_cyc(4); goto L126E; }
    spc_cyc(2);
    /* 125C inc.w $0567 */
    spc_rmw_cyc(0x0567, spc_inc, 4, 1);
    /* 125F mov.w a,$0567 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0567));
    /* 1262 cmp a,#$E6 */
    spc_cmp(spc.a, 0xE6);
    spc_cyc(2);
    /* 1264 bne $126E */
    if (!spc.z) { spc_cyc(4); goto L126E; }
    spc_cyc(2);
    /* 1266 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1268 mov.w $0567,a */
    spc_st_cyc(0x0567, spc.a, 4, 1);
    /* 126B mov $1B,#$03 */
    spc_st_cyc(spc_dp(0x1B), 0x03, 4, 1);
L126E:
    /* 126E mov.b a,$53 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x53)));
    /* 1270 cmp a,#$1B */
    spc_cmp(spc.a, 0x1B);
    spc_cyc(2);
    /* 1272 beq $1288 */
    if (spc.z) { spc_cyc(4); goto L1288; }
    spc_cyc(2);
    /* 1274 cmp a,#$1D */
    spc_cmp(spc.a, 0x1D);
    spc_cyc(2);
    /* 1276 beq $128C */
    if (spc.z) { spc_cyc(4); goto L128C; }
    spc_cyc(2);
    /* 1278 cmp a,#$1E */
    spc_cmp(spc.a, 0x1E);
    spc_cyc(2);
    /* 127A beq $1290 */
    if (spc.z) { spc_cyc(4); goto L1290; }
    spc_cyc(2);
    /* 127C cmp a,#$29 */
    spc_cmp(spc.a, 0x29);
    spc_cyc(2);
    /* 127E beq $1294 */
    if (spc.z) { spc_cyc(4); goto L1294; }
    spc_cyc(2);
    /* 1280 cmp a,#$2A */
    spc_cmp(spc.a, 0x2A);
    spc_cyc(2);
    /* 1282 beq $1298 */
    if (spc.z) { spc_cyc(4); goto L1298; }
    spc_cyc(2);
    /* 1284 mov a,#$8E */
    spc.a = spc_nz(0x8E);
    spc_cyc(2);
    /* 1286 bra $129C */
    spc_cyc(4);
    goto L129C;
L1288:
    /* 1288 mov a,#$73 */
    spc.a = spc_nz(0x73);
    spc_cyc(2);
    /* 128A bra $129C */
    spc_cyc(4);
    goto L129C;
L128C:
    /* 128C mov a,#$6A */
    spc.a = spc_nz(0x6A);
    spc_cyc(2);
    /* 128E bra $129C */
    spc_cyc(4);
    goto L129C;
L1290:
    /* 1290 mov a,#$67 */
    spc.a = spc_nz(0x67);
    spc_cyc(2);
    /* 1292 bra $129C */
    spc_cyc(4);
    goto L129C;
L1294:
    /* 1294 mov a,#$92 */
    spc.a = spc_nz(0x92);
    spc_cyc(2);
    /* 1296 bra $129C */
    spc_cyc(4);
    goto L129C;
L1298:
    /* 1298 mov a,#$93 */
    spc.a = spc_nz(0x93);
    spc_cyc(2);
    /* 129A bra $129C */
    spc_cyc(4);
    goto L129C;
L129C:
    /* 129C mov.w $0586,a */
    spc_st_cyc(0x0586, spc.a, 4, 1);
    /* 129F mov.w a,$0397 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0397));
    /* 12A2 cmp a,#$0D */
    spc_cmp(spc.a, 0x0D);
    spc_cyc(2);
    /* 12A4 beq $1315 */
    if (spc.z) { spc_cyc(4); goto L1315; }
    spc_cyc(2);
    /* 12A6 mov.w a,$055F */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x055F));
    /* 12A9 bne $12B7 */
    if (!spc.z) { spc_cyc(4); goto L12B7; }
    spc_cyc(2);
    /* 12AB cmp $01,#$1D */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x01)), 0x1D);
    spc_cyc(1);
    /* 12AE bne $1315 */
    if (!spc.z) { spc_cyc(4); goto L1315; }
    spc_cyc(2);
    /* 12B0 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 12B2 mov.w $055F,a */
    spc_st_cyc(0x055F, spc.a, 4, 1);
    /* 12B5 bra $1315 */
    spc_cyc(4);
    goto L1315;
L12B7:
    /* 12B7 mov.w a,$0582 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0582));
    /* 12BA beq $12CA */
    if (spc.z) { spc_cyc(4); goto L12CA; }
    spc_cyc(2);
    /* 12BC dec.w $0582 */
    spc_rmw_cyc(0x0582, spc_dec, 4, 1);
    /* 12BF mov.w a,$0582 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0582));
    /* 12C2 cmp a,#$04 */
    spc_cmp(spc.a, 0x04);
    spc_cyc(2);
    /* 12C4 beq $12FA */
    if (spc.z) { spc_cyc(4); goto L12FA; }
    spc_cyc(2);
    /* 12C6 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 12C8 beq $130B */
    if (spc.z) { spc_cyc(4); goto L130B; }
    spc_cyc(2);
L12CA:
    /* 12CA cmp $1D,#$30 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x30);
    spc_cyc(1);
    /* 12CD bne $1315 */
    if (!spc.z) { spc_cyc(4); goto L1315; }
    spc_cyc(2);
    /* 12CF inc.w $055F */
    spc_rmw_cyc(0x055F, spc_inc, 4, 1);
    /* 12D2 mov.w a,$055F */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x055F));
    /* 12D5 cmp a,#$03 */
    spc_cmp(spc.a, 0x03);
    spc_cyc(2);
    /* 12D7 bne $1315 */
    if (!spc.z) { spc_cyc(4); goto L1315; }
    spc_cyc(2);
    /* 12D9 mov $40,#$48 */
    spc_st_cyc(spc_dp(0x40), 0x48, 4, 1);
    /* 12DC mov $41,#$EE */
    spc_st_cyc(spc_dp(0x41), 0xEE, 4, 1);
    /* 12DF set1 $2C.0 */
    spc_bit_set_cyc(0x2C, 0, 1, 3, 1);
    /* 12E1 mov.w a,$0586 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0586));
    /* 12E4 mov.w $0582,a */
    spc_st_cyc(0x0582, spc.a, 4, 1);
    /* 12E7 mov x,#$00 */
    spc.x = spc_nz(0x00);
    spc_cyc(2);
    /* 12E9 call $1D63 */
    spc_call_cyc(abi_SpcSfxTakeVoice, 0x12EC, 5, 1, 2);
    /* 12EC inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 12ED call $1D63 */
    spc_call_cyc(abi_SpcSfxTakeVoice, 0x12F0, 5, 1, 2);
    /* 12F0 mov.b a,$59 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x59)));
    /* 12F2 mov.w $0587,a */
    spc_st_cyc(0x0587, spc.a, 4, 1);
    /* 12F5 mov $59,#$BE */
    spc_st_cyc(spc_dp(0x59), 0xBE, 4, 1);
    /* 12F8 bra $1315 */
    spc_cyc(4);
    goto L1315;
L12FA:
    /* 12FA mov.w a,$0587 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0587));
    /* 12FD mov.b $59,a */
    spc_st_cyc(spc_dp(0x59), spc.a, 3, 1);
    /* 12FF mov x,#$00 */
    spc.x = spc_nz(0x00);
    spc_cyc(2);
    /* 1301 call $1E31 */
    spc_call_cyc(abi_SpcSfxRelease, 0x1304, 5, 1, 2);
    /* 1304 mov x,#$01 */
    spc.x = spc_nz(0x01);
    spc_cyc(2);
    /* 1306 call $1E31 */
    spc_call_cyc(abi_SpcSfxRelease, 0x1309, 5, 1, 2);
    /* 1309 bra $1315 */
    spc_cyc(4);
    goto L1315;
L130B:
    /* 130B inc.b $50 */
    spc_rmw_cyc(spc_dp(0x50), spc_inc, 3, 1);
    /* 130D mov $40,#$48 */
    spc_st_cyc(spc_dp(0x40), 0x48, 4, 1);
    /* 1310 mov $41,#$EE */
    spc_st_cyc(spc_dp(0x41), 0xEE, 4, 1);
    /* 1313 set1 $2C.0 */
    spc_bit_set_cyc(0x2C, 0, 1, 3, 1);
L1315:
    /* 1315 cmp $01,#$1D */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x01)), 0x1D);
    spc_cyc(1);
    /* 1318 bne $1324 */
    if (!spc.z) { spc_cyc(4); goto L1324; }
    spc_cyc(2);
    /* 131A mov.w a,$0397 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0397));
    /* 131D cmp a,#$0D */
    spc_cmp(spc.a, 0x0D);
    spc_cyc(2);
    /* 131F bne $1324 */
    if (!spc.z) { spc_cyc(4); goto L1324; }
    spc_cyc(2);
    /* 1321 mov $01,#$00 */
    spc_st_cyc(spc_dp(0x01), 0x00, 4, 1);
L1324:
    /* 1324 cmp $1B,#$01 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1B)), 0x01);
    spc_cyc(1);
    /* 1327 bne $133A */
    if (!spc.z) { spc_cyc(4); goto L133A; }
    spc_cyc(2);
    /* 1329 mov.w a,$055F */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x055F));
    /* 132C beq $133A */
    if (spc.z) { spc_cyc(4); goto L133A; }
    spc_cyc(2);
    /* 132E mov.w a,$0397 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0397));
    /* 1331 cmp a,#$0D */
    spc_cmp(spc.a, 0x0D);
    spc_cyc(2);
    /* 1333 beq $133A */
    if (spc.z) { spc_cyc(4); goto L133A; }
    spc_cyc(2);
    /* 1335 mov a,#$02 */
    spc.a = spc_nz(0x02);
    spc_cyc(2);
    /* 1337 call $1424 */
    spc_call_cyc(abi_SpcSoundQueuePush, 0x133A, 5, 1, 2);
L133A:
    /* 133A mov.w a,$056A */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x056A));
    /* 133D cmp a,#$F0 */
    spc_cmp(spc.a, 0xF0);
    spc_cyc(2);
    /* 133F bne $1357 */
    if (!spc.z) { spc_cyc(4); goto L1357; }
    spc_cyc(2);
    /* 1341 mov.w a,$055F */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x055F));
    /* 1344 beq $1357 */
    if (spc.z) { spc_cyc(4); goto L1357; }
    spc_cyc(2);
    /* 1346 mov.w a,$0397 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0397));
    /* 1349 cmp a,#$0D */
    spc_cmp(spc.a, 0x0D);
    spc_cyc(2);
    /* 134B beq $1357 */
    if (spc.z) { spc_cyc(4); goto L1357; }
    spc_cyc(2);
    /* 134D cmp $1C,#$05 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1C)), 0x05);
    spc_cyc(1);
    /* 1350 bne $1357 */
    if (!spc.z) { spc_cyc(4); goto L1357; }
    spc_cyc(2);
    /* 1352 mov a,#$24 */
    spc.a = spc_nz(0x24);
    spc_cyc(2);
    /* 1354 call $1424 */
    spc_call_cyc(abi_SpcSoundQueuePush, 0x1357, 5, 1, 2);
L1357:
    /* 1357 cmp $1D,#$39 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x39);
    spc_cyc(1);
    /* 135A bne $1376 */
    if (!spc.z) { spc_cyc(4); goto L1376; }
    spc_cyc(2);
    /* 135C mov $2A,#$00 */
    spc_st_cyc(spc_dp(0x2A), 0x00, 4, 1);
    /* 135F set1 $2D.3 */
    spc_bit_set_cyc(0x2D, 3, 1, 3, 1);
    /* 1361 set1 $2D.6 */
    spc_bit_set_cyc(0x2D, 6, 1, 3, 1);
    /* 1363 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 1365 mov.w $0579,a */
    spc_st_cyc(0x0579, spc.a, 4, 1);
    /* 1368 mov x,#$07 */
    spc.x = spc_nz(0x07);
    spc_cyc(2);
    /* 136A call $1A92 */
    spc_call_cyc(abi_SpcSfxStopRequest, 0x136D, 5, 1, 2);
    /* 136D dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 136E call $1A92 */
    spc_call_cyc(abi_SpcSfxStopRequest, 0x1371, 5, 1, 2);
    /* 1371 mov x,#$08 */
    spc.x = spc_nz(0x08);
    spc_cyc(2);
    /* 1373 call $1908 */
    spc_call_cyc(abi_SpcSfxStopIf, 0x1376, 5, 1, 2);
L1376:
    /* 1376 cmp $1D,#$32 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x32);
    spc_cyc(1);
    /* 1379 bne $139C */
    if (!spc.z) { spc_cyc(4); goto L139C; }
    spc_cyc(2);
    /* 137B set1 $2D.5 */
    spc_bit_set_cyc(0x2D, 5, 1, 3, 1);
    /* 137D mov x,#$04 */
    spc.x = spc_nz(0x04);
    spc_cyc(2);
    /* 137F call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x1382, 5, 1, 2);
    /* 1382 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1384 mov.b $2A,a */
    spc_st_cyc(spc_dp(0x2A), spc.a, 3, 1);
    /* 1386 mov.b $1C,a */
    spc_st_cyc(spc_dp(0x1C), spc.a, 3, 1);
    /* 1388 call $0A60 */
    spc_call_cyc(abi_SpcMusicPause, 0x138B, 5, 1, 2);
    /* 138B set1 $2D.3 */
    spc_bit_set_cyc(0x2D, 3, 1, 3, 1);
    /* 138D set1 $2D.6 */
    spc_bit_set_cyc(0x2D, 6, 1, 3, 1);
    /* 138F mov x,#$02 */
    spc.x = spc_nz(0x02);
    spc_cyc(2);
    /* 1391 call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x1394, 5, 1, 2);
    /* 1394 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 1395 call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x1398, 5, 1, 2);
    /* 1398 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 1399 call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x139C, 5, 1, 2);
L139C:
    /* 139C cmp $1D,#$03 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x03);
    spc_cyc(1);
    /* 139F bne $13B3 */
    if (!spc.z) { spc_cyc(4); goto L13B3; }
    spc_cyc(2);
    /* 13A1 set1 $2D.5 */
    spc_bit_set_cyc(0x2D, 5, 1, 3, 1);
    /* 13A3 mov x,#$05 */
    spc.x = spc_nz(0x05);
    spc_cyc(2);
    /* 13A5 call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x13A8, 5, 1, 2);
    /* 13A8 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 13A9 call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x13AC, 5, 1, 2);
    /* 13AC set1 $2D.3 */
    spc_bit_set_cyc(0x2D, 3, 1, 3, 1);
    /* 13AE mov $2A,#$00 */
    spc_st_cyc(spc_dp(0x2A), 0x00, 4, 1);
    /* 13B1 set1 $2D.6 */
    spc_bit_set_cyc(0x2D, 6, 1, 3, 1);
L13B3:
    /* 13B3 mov.w a,$0579 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0579));
    /* 13B6 beq $13CA */
    if (spc.z) { spc_cyc(4); goto L13CA; }
    spc_cyc(2);
    /* 13B8 inc.w $0579 */
    spc_rmw_cyc(0x0579, spc_inc, 4, 1);
    /* 13BB mov.w a,$0579 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0579));
    /* 13BE setc */
    spc.c = 1;
    spc_cyc(2);
    /* 13BF sbc a,#$78 */
    spc.a = spc_sbc(spc.a, 0x78);
    spc_cyc(2);
    /* 13C1 bmi $13CA */
    if (spc.n) { spc_cyc(4); goto L13CA; }
    spc_cyc(2);
    /* 13C3 set1 $2D.5 */
    spc_bit_set_cyc(0x2D, 5, 1, 3, 1);
    /* 13C5 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 13C7 mov.w $0579,a */
    spc_st_cyc(0x0579, spc.a, 4, 1);
L13CA:
    /* 13CA mov.w a,$0395 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0395));
    /* 13CD cmp a,#$17 */
    spc_cmp(spc.a, 0x17);
    spc_cyc(2);
    /* 13CF beq $13D7 */
    if (spc.z) { spc_cyc(4); goto L13D7; }
    spc_cyc(2);
    /* 13D1 cmp a,#$21 */
    spc_cmp(spc.a, 0x21);
    spc_cyc(2);
    /* 13D3 beq $13D7 */
    if (spc.z) { spc_cyc(4); goto L13D7; }
    spc_cyc(2);
    /* 13D5 bra $13E6 */
    spc_cyc(4);
    goto L13E6;
L13D7:
    /* 13D7 cmp $01,#$09 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x01)), 0x09);
    spc_cyc(1);
    /* 13DA beq $13E3 */
    if (spc.z) { spc_cyc(4); goto L13E3; }
    spc_cyc(2);
    /* 13DC cmp $01,#$0D */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x01)), 0x0D);
    spc_cyc(1);
    /* 13DF beq $13E3 */
    if (spc.z) { spc_cyc(4); goto L13E3; }
    spc_cyc(2);
    /* 13E1 bra $13E6 */
    spc_cyc(4);
    goto L13E6;
L13E3:
    /* 13E3 mov $01,#$00 */
    spc_st_cyc(spc_dp(0x01), 0x00, 4, 1);
L13E6:
    /* 13E6 mov.w a,$0397 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0397));
    /* 13E9 cmp a,#$32 */
    spc_cmp(spc.a, 0x32);
    spc_cyc(2);
    /* 13EB bne $13F9 */
    if (!spc.z) { spc_cyc(4); goto L13F9; }
    spc_cyc(2);
    /* 13ED mov.w a,$0394 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0394));
    /* 13F0 cmp a,#$38 */
    spc_cmp(spc.a, 0x38);
    spc_cyc(2);
    /* 13F2 bne $13F9 */
    if (!spc.z) { spc_cyc(4); goto L13F9; }
    spc_cyc(2);
    /* 13F4 mov x,#$04 */
    spc.x = spc_nz(0x04);
    spc_cyc(2);
    /* 13F6 call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x13F9, 5, 1, 2);
L13F9:
    /* 13F9 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D084 SpcCommand4C
 * WHAT IT DOES: Port command $4C: channels 7 and 6 stopped with their fades (SpcSfxStopRequest), 5, 4 and 3 stopped, the timer $0579 started, bits 3 and 6 of $2D set, $2A cleared, the music paused.
 * HOW: CALLs with X counting down.
 * Entry: none. Exit: as SpcMusicPause.
 * @spc 13FA
 * @spcuses 0A3B-0A3C,0A58-0A6D,13FA-1424,1941-1951,1A92-1AAA
 */
void abi_SpcCommand4C(void)
{
    /* 13FA cmp $1D,#$4C */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x4C);
    spc_cyc(1);
    /* 13FD bne $1423 */
    if (!spc.z) { spc_cyc(4); goto L1423; }
    spc_cyc(2);
    /* 13FF mov x,#$07 */
    spc.x = spc_nz(0x07);
    spc_cyc(2);
    /* 1401 call $1A92 */
    spc_call_cyc(abi_SpcSfxStopRequest, 0x1404, 5, 1, 2);
    /* 1404 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 1405 call $1A92 */
    spc_call_cyc(abi_SpcSfxStopRequest, 0x1408, 5, 1, 2);
    /* 1408 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 1409 call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x140C, 5, 1, 2);
    /* 140C dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 140D call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x1410, 5, 1, 2);
    /* 1410 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 1411 call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x1414, 5, 1, 2);
    /* 1414 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 1416 mov.w $0579,a */
    spc_st_cyc(0x0579, spc.a, 4, 1);
    /* 1419 set1 $2D.3 */
    spc_bit_set_cyc(0x2D, 3, 1, 3, 1);
    /* 141B mov $2A,#$00 */
    spc_st_cyc(spc_dp(0x2A), 0x00, 4, 1);
    /* 141E call $0A60 */
    spc_call_cyc(abi_SpcMusicPause, 0x1421, 5, 1, 2);
    /* 1421 set1 $2D.6 */
    spc_bit_set_cyc(0x2D, 6, 1, 3, 1);
L1423:
    /* 1423 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D0AE SpcSoundQueuePush
 * WHAT IT DOES: Appends sound A to the zero-ended queue at $0588.
 * HOW: Scans for the first zero byte; X kept on the stack.
 * Entry: A = sound. Exit: A, X kept.
 * @spc 1424
 * @spcuses 1424-1436
 */
void abi_SpcSoundQueuePush(void)
{
    /* 1424 push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 1425 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 1426 mov x,#$00 */
    spc.x = spc_nz(0x00);
    spc_cyc(2);
L1428:
    /* 1428 mov.w a,$0588+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0588 + spc.x)));
    /* 142B beq $1430 */
    if (spc.z) { spc_cyc(4); goto L1430; }
    spc_cyc(2);
    /* 142D inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 142E bra $1428 */
    spc_cyc(4);
    goto L1428;
L1430:
    /* 1430 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 1431 mov.w $0588+x,a */
    spc_st_cyc((uint16_t)(0x0588 + spc.x), spc.a, 5, 1);
    /* 1434 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 1435 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D0C0 SpcCommandTimers
 * WHAT IT DOES: Unless stopped (bit 0 of $29): command $3E starts the count $0568, which at $2A queues sound $0F or $2F by the track $0397; commands $20 and $16 mark bits 4 and 5 of $2C, become command $1B or $27 by the track and start the count $055D, which at $11 queues $20 or $16; command $37 counts $0580, and command $30 queues $43 when that count is $0A; command $29 queues itself again after a random delay ($0560, $0561).
 * HOW: Byte counters in RAM; SpcSoundQueuePush.
 * Entry: none. Exit: A, X, Y changed.
 * @spc 1436
 * @spcuses 1424-1523
 */
void abi_SpcCommandTimers(void)
{
    /* 1436 bbs $29.0,$1435 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x29)) >> 0 & 1)) { spc_cyc(4); goto L1435; }
    spc_cyc(2);
    /* 1439 cmp $1D,#$3E */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x3E);
    spc_cyc(1);
    /* 143C bne $1443 */
    if (!spc.z) { spc_cyc(4); goto L1443; }
    spc_cyc(2);
    /* 143E mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 1440 mov.w $0568,a */
    spc_st_cyc(0x0568, spc.a, 4, 1);
L1443:
    /* 1443 mov.w a,$0568 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0568));
    /* 1446 beq $144B */
    if (spc.z) { spc_cyc(4); goto L144B; }
    spc_cyc(2);
    /* 1448 inc.w $0568 */
    spc_rmw_cyc(0x0568, spc_inc, 4, 1);
L144B:
    /* 144B mov.w a,$0568 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0568));
    /* 144E cmp a,#$2A */
    spc_cmp(spc.a, 0x2A);
    spc_cyc(2);
    /* 1450 bne $1475 */
    if (!spc.z) { spc_cyc(4); goto L1475; }
    spc_cyc(2);
    /* 1452 mov.w a,$0397 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0397));
    /* 1455 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 1457 beq $146B */
    if (spc.z) { spc_cyc(4); goto L146B; }
    spc_cyc(2);
    /* 1459 cmp a,#$04 */
    spc_cmp(spc.a, 0x04);
    spc_cyc(2);
    /* 145B beq $1467 */
    if (spc.z) { spc_cyc(4); goto L1467; }
    spc_cyc(2);
    /* 145D cmp a,#$07 */
    spc_cmp(spc.a, 0x07);
    spc_cyc(2);
    /* 145F beq $146B */
    if (spc.z) { spc_cyc(4); goto L146B; }
    spc_cyc(2);
    /* 1461 cmp a,#$0A */
    spc_cmp(spc.a, 0x0A);
    spc_cyc(2);
    /* 1463 beq $1467 */
    if (spc.z) { spc_cyc(4); goto L1467; }
    spc_cyc(2);
    /* 1465 bra $1470 */
    spc_cyc(4);
    goto L1470;
L1467:
    /* 1467 mov a,#$0F */
    spc.a = spc_nz(0x0F);
    spc_cyc(2);
    /* 1469 bra $146D */
    spc_cyc(4);
    goto L146D;
L146B:
    /* 146B mov a,#$2F */
    spc.a = spc_nz(0x2F);
    spc_cyc(2);
L146D:
    /* 146D call $1424 */
    spc_call_cyc(abi_SpcSoundQueuePush, 0x1470, 5, 1, 2);
L1470:
    /* 1470 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1472 mov.w $0568,a */
    spc_st_cyc(0x0568, spc.a, 4, 1);
L1475:
    /* 1475 cmp $1D,#$20 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x20);
    spc_cyc(1);
    /* 1478 beq $1481 */
    if (spc.z) { spc_cyc(4); goto L1481; }
    spc_cyc(2);
    /* 147A cmp $1D,#$16 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x16);
    spc_cyc(1);
    /* 147D beq $1485 */
    if (spc.z) { spc_cyc(4); goto L1485; }
    spc_cyc(2);
    /* 147F bra $14A3 */
    spc_cyc(4);
    goto L14A3;
L1481:
    /* 1481 set1 $2C.4 */
    spc_bit_set_cyc(0x2C, 4, 1, 3, 1);
    /* 1483 bra $1487 */
    spc_cyc(4);
    goto L1487;
L1485:
    /* 1485 set1 $2C.5 */
    spc_bit_set_cyc(0x2C, 5, 1, 3, 1);
L1487:
    /* 1487 mov.w a,$0397 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0397));
    /* 148A cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 148C beq $149B */
    if (spc.z) { spc_cyc(4); goto L149B; }
    spc_cyc(2);
    /* 148E cmp a,#$04 */
    spc_cmp(spc.a, 0x04);
    spc_cyc(2);
    /* 1490 beq $1496 */
    if (spc.z) { spc_cyc(4); goto L1496; }
    spc_cyc(2);
    /* 1492 cmp a,#$07 */
    spc_cmp(spc.a, 0x07);
    spc_cyc(2);
    /* 1494 beq $149B */
    if (spc.z) { spc_cyc(4); goto L149B; }
    spc_cyc(2);
L1496:
    /* 1496 mov $1D,#$1B */
    spc_st_cyc(spc_dp(0x1D), 0x1B, 4, 1);
    /* 1499 bra $149E */
    spc_cyc(4);
    goto L149E;
L149B:
    /* 149B mov $1D,#$27 */
    spc_st_cyc(spc_dp(0x1D), 0x27, 4, 1);
L149E:
    /* 149E mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 14A0 mov.w $055D,a */
    spc_st_cyc(0x055D, spc.a, 4, 1);
L14A3:
    /* 14A3 mov.w a,$055D */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x055D));
    /* 14A6 beq $14AB */
    if (spc.z) { spc_cyc(4); goto L14AB; }
    spc_cyc(2);
    /* 14A8 inc.w $055D */
    spc_rmw_cyc(0x055D, spc_inc, 4, 1);
L14AB:
    /* 14AB mov.w a,$055D */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x055D));
    /* 14AE cmp a,#$11 */
    spc_cmp(spc.a, 0x11);
    spc_cyc(2);
    /* 14B0 bne $14CF */
    if (!spc.z) { spc_cyc(4); goto L14CF; }
    spc_cyc(2);
    /* 14B2 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 14B4 mov.w $055D,a */
    spc_st_cyc(0x055D, spc.a, 4, 1);
    /* 14B7 bbs $2C.4,$14BF */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2C)) >> 4 & 1)) { spc_cyc(4); goto L14BF; }
    spc_cyc(2);
    /* 14BA bbs $2C.5,$14C8 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2C)) >> 5 & 1)) { spc_cyc(4); goto L14C8; }
    spc_cyc(2);
    /* 14BD bra $14CF */
    spc_cyc(4);
    goto L14CF;
L14BF:
    /* 14BF clr1 $2C.4 */
    spc_bit_set_cyc(0x2C, 4, 0, 3, 1);
    /* 14C1 mov a,#$20 */
    spc.a = spc_nz(0x20);
    spc_cyc(2);
    /* 14C3 call $1424 */
    spc_call_cyc(abi_SpcSoundQueuePush, 0x14C6, 5, 1, 2);
    /* 14C6 bra $14CF */
    spc_cyc(4);
    goto L14CF;
L14C8:
    /* 14C8 clr1 $2C.5 */
    spc_bit_set_cyc(0x2C, 5, 0, 3, 1);
    /* 14CA mov a,#$16 */
    spc.a = spc_nz(0x16);
    spc_cyc(2);
    /* 14CC call $1424 */
    spc_call_cyc(abi_SpcSoundQueuePush, 0x14CF, 5, 1, 2);
L14CF:
    /* 14CF cmp $1D,#$37 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x37);
    spc_cyc(1);
    /* 14D2 bne $14D7 */
    if (!spc.z) { spc_cyc(4); goto L14D7; }
    spc_cyc(2);
    /* 14D4 inc.w $0580 */
    spc_rmw_cyc(0x0580, spc_inc, 4, 1);
L14D7:
    /* 14D7 cmp $1D,#$30 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x30);
    spc_cyc(1);
    /* 14DA bne $14ED */
    if (!spc.z) { spc_cyc(4); goto L14ED; }
    spc_cyc(2);
    /* 14DC mov.w a,$0580 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0580));
    /* 14DF cmp a,#$0A */
    spc_cmp(spc.a, 0x0A);
    spc_cyc(2);
    /* 14E1 bne $14E8 */
    if (!spc.z) { spc_cyc(4); goto L14E8; }
    spc_cyc(2);
    /* 14E3 mov a,#$43 */
    spc.a = spc_nz(0x43);
    spc_cyc(2);
    /* 14E5 call $1424 */
    spc_call_cyc(abi_SpcSoundQueuePush, 0x14E8, 5, 1, 2);
L14E8:
    /* 14E8 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 14EA mov.w $0580,a */
    spc_st_cyc(0x0580, spc.a, 4, 1);
L14ED:
    /* 14ED mov.w a,$0560 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0560));
    /* 14F0 bne $150A */
    if (!spc.z) { spc_cyc(4); goto L150A; }
    spc_cyc(2);
    /* 14F2 mov.b a,$1D */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 14F4 cmp a,#$29 */
    spc_cmp(spc.a, 0x29);
    spc_cyc(2);
    /* 14F6 beq $14FA */
    if (spc.z) { spc_cyc(4); goto L14FA; }
    spc_cyc(2);
    /* 14F8 bra $1522 */
    spc_cyc(4);
    goto L1522;
L14FA:
    /* 14FA mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 14FC mov.w $0560,a */
    spc_st_cyc(0x0560, spc.a, 4, 1);
    /* 14FF mov.b a,$18 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x18)));
    /* 1501 and a,#$3F */
    spc.a = spc_and(spc.a, 0x3F);
    spc_cyc(2);
    /* 1503 adc a,#$1E */
    spc.a = spc_adc(spc.a, 0x1E);
    spc_cyc(2);
    /* 1505 mov.w $0561,a */
    spc_st_cyc(0x0561, spc.a, 4, 1);
    /* 1508 bra $1522 */
    spc_cyc(4);
    goto L1522;
L150A:
    /* 150A inc.w $0560 */
    spc_rmw_cyc(0x0560, spc_inc, 4, 1);
    /* 150D cmp.w a,$0561 */
    spc_cyc(4);
    spc_cmp(spc.a, spc_rd(0x0561));
    /* 1510 bne $1522 */
    if (!spc.z) { spc_cyc(4); goto L1522; }
    spc_cyc(2);
    /* 1512 mov a,#$29 */
    spc.a = spc_nz(0x29);
    spc_cyc(2);
    /* 1514 call $1424 */
    spc_call_cyc(abi_SpcSoundQueuePush, 0x1517, 5, 1, 2);
    /* 1517 mov $1D,#$29 */
    spc_st_cyc(spc_dp(0x1D), 0x29, 4, 1);
    /* 151A mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 151C mov.w $0560,a */
    spc_st_cyc(0x0560, spc.a, 4, 1);
    /* 151F mov.w $0561,a */
    spc_st_cyc(0x0561, spc.a, 4, 1);
L1522:
    /* 1522 ret */
    spc_ret_cyc(4, 1);
    return;
L1435:
    /* 1435 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D1AD SpcTempoBend
 * WHAT IT DOES: The tempo bend (SpcTempoBendStart decides it): while bit 2 of $2C is set, each tick lowers the transpose by the step count / $15 and the tempo by an accumulated amount from its saved values ($0566, $0578); bit 3 restores them and stops.
 * HOW: Word accumulator $0553/$0554, DIV by $15.
 * Entry: none. Exit: A = the tempo high byte.
 * @spc 1523
 * @spcuses 1523-15C4
 */
void abi_SpcTempoBend(void)
{
    /* 1523 call $15A1 */
    spc_call_cyc(abi_SpcTempoBendStart, 0x1526, 5, 1, 2);
    /* 1526 bbs $2C.2,$152D */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2C)) >> 2 & 1)) { spc_cyc(4); goto L152D; }
    spc_cyc(2);
    /* 1529 bbs $2C.3,$1538 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2C)) >> 3 & 1)) { spc_cyc(4); goto L1538; }
    spc_cyc(2);
L152C:
    /* 152C ret */
    spc_ret_cyc(4, 1);
    return;
L152D:
    /* 152D mov.w a,$0555 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0555));
    /* 1530 beq $1554 */
    if (spc.z) { spc_cyc(4); goto L1554; }
    spc_cyc(2);
    /* 1532 cmp a,#$FE */
    spc_cmp(spc.a, 0xFE);
    spc_cyc(2);
    /* 1534 beq $152C */
    if (spc.z) { spc_cyc(4); goto L152C; }
    spc_cyc(2);
    /* 1536 bra $156C */
    spc_cyc(4);
    goto L156C;
L1538:
    /* 1538 clr1 $2C.2 */
    spc_bit_set_cyc(0x2C, 2, 0, 3, 1);
    /* 153A clr1 $2C.3 */
    spc_bit_set_cyc(0x2C, 3, 0, 3, 1);
    /* 153C mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 153E mov.w $0555,a */
    spc_st_cyc(0x0555, spc.a, 4, 1);
    /* 1541 mov.w $0553,a */
    spc_st_cyc(0x0553, spc.a, 4, 1);
    /* 1544 mov.w $0554,a */
    spc_st_cyc(0x0554, spc.a, 4, 1);
    /* 1547 mov.w $0569,a */
    spc_st_cyc(0x0569, spc.a, 4, 1);
    /* 154A mov.w a,$0578 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0578));
    /* 154D mov.b $50,a */
    spc_st_cyc(spc_dp(0x50), spc.a, 3, 1);
    /* 154F mov.w a,$0566 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0566));
    /* 1552 bra $159E */
    spc_cyc(4);
    goto L159E;
L1554:
    /* 1554 mov a,#$04 */
    spc.a = spc_nz(0x04);
    spc_cyc(2);
    /* 1556 mov.w $0554,a */
    spc_st_cyc(0x0554, spc.a, 4, 1);
    /* 1559 mov.b a,$53 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x53)));
    /* 155B mov.w $0566,a */
    spc_st_cyc(0x0566, spc.a, 4, 1);
    /* 155E mov.b a,$50 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x50)));
    /* 1560 mov.w $0578,a */
    spc_st_cyc(0x0578, spc.a, 4, 1);
    /* 1563 mov y,#$7F */
    spc.y = spc_nz(0x7F);
    spc_cyc(2);
    /* 1565 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 1566 mov x,#$FE */
    spc.x = spc_nz(0xFE);
    spc_cyc(2);
    /* 1568 div ya,x */
    spc_div();
    spc_cyc(12);
    /* 1569 mov.w $0569,a */
    spc_st_cyc(0x0569, spc.a, 4, 1);
L156C:
    /* 156C inc.w $0555 */
    spc_rmw_cyc(0x0555, spc_inc, 4, 1);
    /* 156F mov.w a,$0555 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0555));
    /* 1572 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 1574 mov x,#$15 */
    spc.x = spc_nz(0x15);
    spc_cyc(2);
    /* 1576 div ya,x */
    spc_div();
    spc_cyc(12);
    /* 1577 mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 1579 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 157A mov.w a,$0578 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0578));
    /* 157D sbc.b a,$20 */
    spc_cyc(3);
    spc.a = spc_sbc(spc.a, spc_rd(spc_dp(0x20)));
    /* 157F mov.b $50,a */
    spc_st_cyc(spc_dp(0x50), spc.a, 3, 1);
    /* 1581 mov.w a,$0569 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0569));
    /* 1584 mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 1586 mov $21,#$00 */
    spc_st_cyc(spc_dp(0x21), 0x00, 4, 1);
    /* 1589 mov.w a,$0553 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0553));
    /* 158C mov.w y,$0554 */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x0554));
    /* 158F addw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 1591 mov.w $0553,a */
    spc_st_cyc(0x0553, spc.a, 4, 1);
    /* 1594 mov.w $0554,y */
    spc_st_cyc(0x0554, spc.y, 4, 1);
    /* 1597 mov.w a,$0566 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0566));
    /* 159A setc */
    spc.c = 1;
    spc_cyc(2);
    /* 159B sbc.w a,$0554 */
    spc_cyc(4);
    spc.a = spc_sbc(spc.a, spc_rd(0x0554));
L159E:
    /* 159E mov.b $53,a */
    spc_st_cyc(spc_dp(0x53), spc.a, 3, 1);
    /* 15A0 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D22B SpcTempoBendStart
 * WHAT IT DOES: Command $2C during song 1 starts the tempo bend (bit 2 of $2C); commands $30 and $3C during a bend end it (bit 3).
 * HOW: SET1/CLR1 on $2C.
 * Entry: none. Exit: flags from the compares.
 * @spc 15A1
 * @spcuses 15A1-15C4
 */
void abi_SpcTempoBendStart(void)
{
    /* 15A1 mov.b a,$1D */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 15A3 cmp a,#$2C */
    spc_cmp(spc.a, 0x2C);
    spc_cyc(2);
    /* 15A5 beq $15B5 */
    if (spc.z) { spc_cyc(4); goto L15B5; }
    spc_cyc(2);
    /* 15A7 mov.w x,$0555 */
    spc_cyc(4);
    spc.x = spc_nz(spc_rd(0x0555));
    /* 15AA beq $15BE */
    if (spc.z) { spc_cyc(4); goto L15BE; }
    spc_cyc(2);
    /* 15AC cmp a,#$30 */
    spc_cmp(spc.a, 0x30);
    spc_cyc(2);
    /* 15AE beq $15BF */
    if (spc.z) { spc_cyc(4); goto L15BF; }
    spc_cyc(2);
    /* 15B0 cmp a,#$3C */
    spc_cmp(spc.a, 0x3C);
    spc_cyc(2);
    /* 15B2 beq $15BF */
    if (spc.z) { spc_cyc(4); goto L15BF; }
    spc_cyc(2);
    /* 15B4 ret */
    spc_ret_cyc(4, 1);
    return;
L15B5:
    /* 15B5 cmp $1C,#$01 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1C)), 0x01);
    spc_cyc(1);
    /* 15B8 bne $15BE */
    if (!spc.z) { spc_cyc(4); goto L15BE; }
    spc_cyc(2);
    /* 15BA set1 $2C.2 */
    spc_bit_set_cyc(0x2C, 2, 1, 3, 1);
    /* 15BC clr1 $2C.3 */
    spc_bit_set_cyc(0x2C, 3, 0, 3, 1);
L15BE:
    /* 15BE ret */
    spc_ret_cyc(4, 1);
    return;
L15BF:
    /* 15BF clr1 $2C.2 */
    spc_bit_set_cyc(0x2C, 2, 0, 3, 1);
    /* 15C1 set1 $2C.3 */
    spc_bit_set_cyc(0x2C, 3, 1, 3, 1);
    /* 15C3 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D24E SpcEchoTransition
 * WHAT IT DOES: The echo transition by the state $0551 (0 idle; bit 0 or 1 of $28 starts it): the echo voices, feedback and volumes saved ($0570-$0573), then the step count $0550 runs up or down (SpcEchoTransitionStep) with every voice echoing ($4A bits 6, 7) and the feedback and echo volume interpolated from the saved values toward $64 by the count; state 3 restores the saved settings.
 * HOW: MUL and DIV by $3C for the interpolation.
 * Entry: none (unless bit 0 of $29). Exit: A, X, Y changed.
 * @spc 15C4
 * @spcuses 15C4-16B1
 */
void abi_SpcEchoTransition(void)
{
    /* 15C4 bbs $29.0,$15E3 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x29)) >> 0 & 1)) { spc_cyc(4); goto L15E3; }
    spc_cyc(2);
    /* 15C7 mov.w a,$0550 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0550));
    /* 15CA bne $1618 */
    if (!spc.z) { spc_cyc(4); goto L1618; }
    spc_cyc(2);
    /* 15CC mov.w a,$0551 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0551));
    /* 15CF cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 15D1 beq $15E4 */
    if (spc.z) { spc_cyc(4); goto L15E4; }
    spc_cyc(2);
    /* 15D3 cmp a,#$03 */
    spc_cmp(spc.a, 0x03);
    spc_cyc(2);
    /* 15D5 beq $15FE */
    if (spc.z) { spc_cyc(4); goto L15FE; }
    spc_cyc(2);
    /* 15D7 bbs $28.0,$15DE */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x28)) >> 0 & 1)) { spc_cyc(4); goto L15DE; }
    spc_cyc(2);
    /* 15DA bbs $28.1,$15DE */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x28)) >> 1 & 1)) { spc_cyc(4); goto L15DE; }
    spc_cyc(2);
    /* 15DD ret */
    spc_ret_cyc(4, 1);
    return;
L15DE:
    /* 15DE mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 15E0 mov.w $0551,a */
    spc_st_cyc(0x0551, spc.a, 4, 1);
L15E3:
    /* 15E3 ret */
    spc_ret_cyc(4, 1);
    return;
L15E4:
    /* 15E4 mov.b a,$4A */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x4A)));
    /* 15E6 mov.w $0570,a */
    spc_st_cyc(0x0570, spc.a, 4, 1);
    /* 15E9 mov.b a,$4E */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x4E)));
    /* 15EB mov.w $0571,a */
    spc_st_cyc(0x0571, spc.a, 4, 1);
    /* 15EE mov.b a,$61 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x61)));
    /* 15F0 mov.w $0572,a */
    spc_st_cyc(0x0572, spc.a, 4, 1);
    /* 15F3 mov.b a,$63 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x63)));
    /* 15F5 mov.w $0573,a */
    spc_st_cyc(0x0573, spc.a, 4, 1);
    /* 15F8 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 15FA mov.w $0550,a */
    spc_st_cyc(0x0550, spc.a, 4, 1);
    /* 15FD ret */
    spc_ret_cyc(4, 1);
    return;
L15FE:
    /* 15FE mov.w a,$0570 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0570));
    /* 1601 mov.b $4A,a */
    spc_st_cyc(spc_dp(0x4A), spc.a, 3, 1);
    /* 1603 mov.w a,$0571 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0571));
    /* 1606 mov.b $4E,a */
    spc_st_cyc(spc_dp(0x4E), spc.a, 3, 1);
    /* 1608 mov.w a,$0572 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0572));
    /* 160B mov.b $61,a */
    spc_st_cyc(spc_dp(0x61), spc.a, 3, 1);
    /* 160D mov.w a,$0573 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0573));
    /* 1610 mov.b $63,a */
    spc_st_cyc(spc_dp(0x63), spc.a, 3, 1);
    /* 1612 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1614 mov.w $0551,a */
    spc_st_cyc(0x0551, spc.a, 4, 1);
    /* 1617 ret */
    spc_ret_cyc(4, 1);
    return;
L1618:
    /* 1618 mov $4A,#$00 */
    spc_st_cyc(spc_dp(0x4A), 0x00, 4, 1);
    /* 161B set1 $4A.7 */
    spc_bit_set_cyc(0x4A, 7, 1, 3, 1);
    /* 161D set1 $4A.6 */
    spc_bit_set_cyc(0x4A, 6, 1, 3, 1);
    /* 161F call $1624 */
    spc_call_cyc(abi_SpcEchoTransitionStep, 0x1622, 5, 1, 2);
    /* 1622 bra $167C */
    spc_cyc(4);
    goto L167C;
L167C:
    /* 167C mov.w a,$0550 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0550));
    /* 167F mov y,#$B0 */
    spc.y = spc_nz(0xB0);
    spc_cyc(2);
    /* 1681 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 1682 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 1683 eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    spc_cyc(2);
    /* 1685 mov.w $0552,a */
    spc_st_cyc(0x0552, spc.a, 4, 1);
    /* 1688 mov a,#$64 */
    spc.a = spc_nz(0x64);
    spc_cyc(2);
    /* 168A setc */
    spc.c = 1;
    spc_cyc(2);
    /* 168B sbc.w a,$0571 */
    spc_cyc(4);
    spc.a = spc_sbc(spc.a, spc_rd(0x0571));
    /* 168E mov.w y,$0550 */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x0550));
    /* 1691 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 1692 mov x,#$3C */
    spc.x = spc_nz(0x3C);
    spc_cyc(2);
    /* 1694 div ya,x */
    spc_div();
    spc_cyc(12);
    /* 1695 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 1696 adc.w a,$0571 */
    spc_cyc(4);
    spc.a = spc_adc(spc.a, spc_rd(0x0571));
    /* 1699 mov.b $4E,a */
    spc_st_cyc(spc_dp(0x4E), spc.a, 3, 1);
    /* 169B mov a,#$64 */
    spc.a = spc_nz(0x64);
    spc_cyc(2);
    /* 169D setc */
    spc.c = 1;
    spc_cyc(2);
    /* 169E sbc.w a,$0572 */
    spc_cyc(4);
    spc.a = spc_sbc(spc.a, spc_rd(0x0572));
    /* 16A1 mov.w y,$0550 */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x0550));
    /* 16A4 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 16A5 mov x,#$3C */
    spc.x = spc_nz(0x3C);
    spc_cyc(2);
    /* 16A7 div ya,x */
    spc_div();
    spc_cyc(12);
    /* 16A8 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 16A9 adc.w a,$0572 */
    spc_cyc(4);
    spc.a = spc_adc(spc.a, spc_rd(0x0572));
    /* 16AC mov.b $61,a */
    spc_st_cyc(spc_dp(0x61), spc.a, 3, 1);
    /* 16AE mov.b $63,a */
    spc_st_cyc(spc_dp(0x63), spc.a, 3, 1);
    /* 16B0 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D2AE SpcEchoTransitionStep
 * WHAT IT DOES: Steps the echo transition count $0550 by the state $0551 and the bits of $28 (up to $1E or $3C, down to $3C), changing the state at the ends.
 * HOW: Compare chains.
 * Entry: none. Exit: flags from the last compare.
 * @spc 1624
 * @spcuses 1624-167C
 */
void abi_SpcEchoTransitionStep(void)
{
    /* 1624 mov.w a,$0551 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0551));
    /* 1627 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 1629 beq $163E */
    if (spc.z) { spc_cyc(4); goto L163E; }
    spc_cyc(2);
    /* 162B cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    spc_cyc(2);
    /* 162D beq $1656 */
    if (spc.z) { spc_cyc(4); goto L1656; }
    spc_cyc(2);
    /* 162F dec.w $0550 */
    spc_rmw_cyc(0x0550, spc_dec, 4, 1);
    /* 1632 mov.w a,$0550 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0550));
    /* 1635 cmp a,#$3C */
    spc_cmp(spc.a, 0x3C);
    spc_cyc(2);
    /* 1637 beq $163A */
    if (spc.z) { spc_cyc(4); goto L163A; }
    spc_cyc(2);
    /* 1639 ret */
    spc_ret_cyc(4, 1);
    return;
L163A:
    /* 163A bbs $28.1,$1650 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x28)) >> 1 & 1)) { spc_cyc(4); goto L1650; }
    spc_cyc(2);
    /* 163D ret */
    spc_ret_cyc(4, 1);
    return;
L163E:
    /* 163E inc.w $0550 */
    spc_rmw_cyc(0x0550, spc_inc, 4, 1);
    /* 1641 mov.w a,$0550 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0550));
    /* 1644 cmp a,#$1E */
    spc_cmp(spc.a, 0x1E);
    spc_cyc(2);
    /* 1646 beq $164D */
    if (spc.z) { spc_cyc(4); goto L164D; }
    spc_cyc(2);
    /* 1648 cmp a,#$3C */
    spc_cmp(spc.a, 0x3C);
    spc_cyc(2);
    /* 164A beq $1650 */
    if (spc.z) { spc_cyc(4); goto L1650; }
    spc_cyc(2);
    /* 164C ret */
    spc_ret_cyc(4, 1);
    return;
L164D:
    /* 164D bbs $28.0,$1655 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x28)) >> 0 & 1)) { spc_cyc(4); goto L1655; }
    spc_cyc(2);
L1650:
    /* 1650 mov a,#$02 */
    spc.a = spc_nz(0x02);
    spc_cyc(2);
    /* 1652 mov.w $0551,a */
    spc_st_cyc(0x0551, spc.a, 4, 1);
L1655:
    /* 1655 ret */
    spc_ret_cyc(4, 1);
    return;
L1656:
    /* 1656 bbs $28.0,$1662 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x28)) >> 0 & 1)) { spc_cyc(4); goto L1662; }
    spc_cyc(2);
    /* 1659 bbs $28.1,$166F */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x28)) >> 1 & 1)) { spc_cyc(4); goto L166F; }
    spc_cyc(2);
    /* 165C mov a,#$03 */
    spc.a = spc_nz(0x03);
    spc_cyc(2);
    /* 165E mov.w $0551,a */
    spc_st_cyc(0x0551, spc.a, 4, 1);
    /* 1661 ret */
    spc_ret_cyc(4, 1);
    return;
L1662:
    /* 1662 mov.w a,$0550 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0550));
    /* 1665 cmp a,#$3C */
    spc_cmp(spc.a, 0x3C);
    spc_cyc(2);
    /* 1667 beq $1655 */
    if (spc.z) { spc_cyc(4); goto L1655; }
    spc_cyc(2);
    /* 1669 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 166B mov.w $0551,a */
    spc_st_cyc(0x0551, spc.a, 4, 1);
    /* 166E ret */
    spc_ret_cyc(4, 1);
    return;
L166F:
    /* 166F mov.w a,$0550 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0550));
    /* 1672 cmp a,#$1E */
    spc_cmp(spc.a, 0x1E);
    spc_cyc(2);
    /* 1674 beq $1655 */
    if (spc.z) { spc_cyc(4); goto L1655; }
    spc_cyc(2);
    /* 1676 mov a,#$03 */
    spc.a = spc_nz(0x03);
    spc_cyc(2);
    /* 1678 mov.w $0551,a */
    spc_st_cyc(0x0551, spc.a, 4, 1);
    /* 167B ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D33B SpcPort1Clamp
 * WHAT IT DOES: Outside song 1, a port 1 command of 7 or more is ignored ($01 cleared).
 * HOW: SBC and BMI.
 * Entry: none. Exit: flags.
 * @spc 16B1
 * @spcuses 16B1-16C1
 */
void abi_SpcPort1Clamp(void)
{
    /* 16B1 cmp $1C,#$01 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1C)), 0x01);
    spc_cyc(1);
    /* 16B4 beq $16C0 */
    if (spc.z) { spc_cyc(4); goto L16C0; }
    spc_cyc(2);
    /* 16B6 mov.b a,$01 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x01)));
    /* 16B8 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 16B9 sbc a,#$07 */
    spc.a = spc_sbc(spc.a, 0x07);
    spc_cyc(2);
    /* 16BB bmi $16C0 */
    if (spc.n) { spc_cyc(4); goto L16C0; }
    spc_cyc(2);
    /* 16BD mov $01,#$00 */
    spc_st_cyc(spc_dp(0x01), 0x00, 4, 1);
L16C0:
    /* 16C0 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D34B SpcCommandFilter
 * WHAT IT DOES: Outside song 1, a port command $1D not in the list at $16D8 is dropped.
 * HOW: A zero-ended list scan.
 * Entry: none. Exit: X = list index.
 * @spc 16C1
 * @spcuses 16C1-16D8
 */
void abi_SpcCommandFilter(void)
{
    /* 16C1 cmp $1C,#$01 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1C)), 0x01);
    spc_cyc(1);
    /* 16C4 beq $16D7 */
    if (spc.z) { spc_cyc(4); goto L16D7; }
    spc_cyc(2);
    /* 16C6 mov x,#$00 */
    spc.x = spc_nz(0x00);
    spc_cyc(2);
L16C8:
    /* 16C8 mov.w a,$16D8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x16D8 + spc.x)));
    /* 16CB beq $16D4 */
    if (spc.z) { spc_cyc(4); goto L16D4; }
    spc_cyc(2);
    /* 16CD cmp.b a,$1D */
    spc_cyc(3);
    spc_cmp(spc.a, spc_rd(spc_dp(0x1D)));
    /* 16CF beq $16D7 */
    if (spc.z) { spc_cyc(4); goto L16D7; }
    spc_cyc(2);
    /* 16D1 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 16D2 bra $16C8 */
    spc_cyc(4);
    goto L16C8;
L16D4:
    /* 16D4 mov $1D,#$00 */
    spc_st_cyc(spc_dp(0x1D), 0x00, 4, 1);
L16D7:
    /* 16D7 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D37B SpcPitchRampB
 * WHAT IT DOES: Bit 2 of $28 ramps the offset $054D/$054F up (SpcPitchRampBUp), bit 3 down (SpcPitchRampBDown).
 * HOW: BBC tests.
 * Entry: none. Exit: as the ramp leaves it.
 * @spc 16F1
 * @spcuses 16F1-174B
 */
void abi_SpcPitchRampB(void)
{
    /* 16F1 bbc $28.2,$16F7 */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x28)) >> 2 & 1)) { spc_cyc(4); goto L16F7; }
    spc_cyc(2);
    /* 16F4 call $16FE */
    spc_call_cyc(abi_SpcPitchRampBUp, 0x16F7, 5, 1, 2);
L16F7:
    /* 16F7 bbc $28.3,$16FD */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x28)) >> 3 & 1)) { spc_cyc(4); goto L16FD; }
    spc_cyc(2);
    /* 16FA call $171F */
    spc_call_cyc(abi_SpcPitchRampBDown, 0x16FD, 5, 1, 2);
L16FD:
    /* 16FD ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D388 SpcPitchRampBUp
 * WHAT IT DOES: Up to $14 steps (count $0547), adds $20 to the pitch offset $054D/$054F.
 * HOW: ADDW through $20/$21.
 * Entry: none. Exit: YA = the offset.
 * @spc 16FE
 * @spcuses 16FE-171F
 */
void abi_SpcPitchRampBUp(void)
{
    /* 16FE mov.w a,$0547 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0547));
    /* 1701 cmp a,#$14 */
    spc_cmp(spc.a, 0x14);
    spc_cyc(2);
    /* 1703 beq $171E */
    if (spc.z) { spc_cyc(4); goto L171E; }
    spc_cyc(2);
    /* 1705 inc.w $0547 */
    spc_rmw_cyc(0x0547, spc_inc, 4, 1);
    /* 1708 mov.w a,$054D */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x054D));
    /* 170B mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 170D mov.w a,$054F */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x054F));
    /* 1710 mov.b $21,a */
    spc_st_cyc(spc_dp(0x21), spc.a, 3, 1);
    /* 1712 mov a,#$20 */
    spc.a = spc_nz(0x20);
    spc_cyc(2);
    /* 1714 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 1716 addw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 1718 mov.w $054D,a */
    spc_st_cyc(0x054D, spc.a, 4, 1);
    /* 171B mov.w $054F,y */
    spc_st_cyc(0x054F, spc.y, 4, 1);
L171E:
    /* 171E ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D3A9 SpcPitchRampBDown
 * WHAT IT DOES: Counts $0547 down, taking $20 from the offset; at 0 clears bits 2 and 3 of $28 and the offset.
 * HOW: SUBW through $20/$21.
 * Entry: none. Exit: A = 0 at the end.
 * @spc 171F
 * @spcuses 171F-174B
 */
void abi_SpcPitchRampBDown(void)
{
    /* 171F mov.w a,$0547 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0547));
    /* 1722 cmp a,#$00 */
    spc_cmp(spc.a, 0x00);
    spc_cyc(2);
    /* 1724 beq $173E */
    if (spc.z) { spc_cyc(4); goto L173E; }
    spc_cyc(2);
    /* 1726 dec.w $0547 */
    spc_rmw_cyc(0x0547, spc_dec, 4, 1);
    /* 1729 mov.w a,$054D */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x054D));
    /* 172C mov.w y,$054F */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x054F));
    /* 172F mov $20,#$20 */
    spc_st_cyc(spc_dp(0x20), 0x20, 4, 1);
    /* 1732 mov $21,#$00 */
    spc_st_cyc(spc_dp(0x21), 0x00, 4, 1);
    /* 1735 subw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 1737 mov.w $054D,a */
    spc_st_cyc(0x054D, spc.a, 4, 1);
    /* 173A mov.w $054F,y */
    spc_st_cyc(0x054F, spc.y, 4, 1);
    /* 173D ret */
    spc_ret_cyc(4, 1);
    return;
L173E:
    /* 173E clr1 $28.2 */
    spc_bit_set_cyc(0x28, 2, 0, 3, 1);
    /* 1740 clr1 $28.3 */
    spc_bit_set_cyc(0x28, 3, 0, 3, 1);
    /* 1742 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1744 mov.w $054D,a */
    spc_st_cyc(0x054D, spc.a, 4, 1);
    /* 1747 mov.w $054F,a */
    spc_st_cyc(0x054F, spc.a, 4, 1);
    /* 174A ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D3D5 SpcPitchRampA
 * WHAT IT DOES: Bit 2 of $29 ramps the offset $0549/$054B up (SpcPitchRampAUp), bit 3 down (SpcPitchRampADown).
 * HOW: BBC tests.
 * Entry: none. Exit: as the ramp leaves it.
 * @spc 174B
 * @spcuses 174B-17A5
 */
void abi_SpcPitchRampA(void)
{
    /* 174B bbc $29.2,$1751 */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x29)) >> 2 & 1)) { spc_cyc(4); goto L1751; }
    spc_cyc(2);
    /* 174E call $1758 */
    spc_call_cyc(abi_SpcPitchRampAUp, 0x1751, 5, 1, 2);
L1751:
    /* 1751 bbc $29.3,$1757 */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x29)) >> 3 & 1)) { spc_cyc(4); goto L1757; }
    spc_cyc(2);
    /* 1754 call $1779 */
    spc_call_cyc(abi_SpcPitchRampADown, 0x1757, 5, 1, 2);
L1757:
    /* 1757 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D3E2 SpcPitchRampAUp
 * WHAT IT DOES: Up to $19 steps (count $0543), adds $3C to the offset $0549/$054B.
 * HOW: ADDW through $20/$21.
 * Entry: none. Exit: YA = the offset.
 * @spc 1758
 * @spcuses 1758-1779
 */
void abi_SpcPitchRampAUp(void)
{
    /* 1758 mov.w a,$0543 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0543));
    /* 175B cmp a,#$19 */
    spc_cmp(spc.a, 0x19);
    spc_cyc(2);
    /* 175D beq $1778 */
    if (spc.z) { spc_cyc(4); goto L1778; }
    spc_cyc(2);
    /* 175F inc.w $0543 */
    spc_rmw_cyc(0x0543, spc_inc, 4, 1);
    /* 1762 mov.w a,$0549 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0549));
    /* 1765 mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 1767 mov.w a,$054B */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x054B));
    /* 176A mov.b $21,a */
    spc_st_cyc(spc_dp(0x21), spc.a, 3, 1);
    /* 176C mov a,#$3C */
    spc.a = spc_nz(0x3C);
    spc_cyc(2);
    /* 176E mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 1770 addw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 1772 mov.w $0549,a */
    spc_st_cyc(0x0549, spc.a, 4, 1);
    /* 1775 mov.w $054B,y */
    spc_st_cyc(0x054B, spc.y, 4, 1);
L1778:
    /* 1778 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D403 SpcPitchRampADown
 * WHAT IT DOES: Counts $0543 down, taking $3C from the offset; at 0 clears bits 2 and 3 of $29 and the offset.
 * HOW: SUBW through $20/$21.
 * Entry: none. Exit: A = 0 at the end.
 * @spc 1779
 * @spcuses 1779-17A5
 */
void abi_SpcPitchRampADown(void)
{
    /* 1779 mov.w a,$0543 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0543));
    /* 177C cmp a,#$00 */
    spc_cmp(spc.a, 0x00);
    spc_cyc(2);
    /* 177E beq $1798 */
    if (spc.z) { spc_cyc(4); goto L1798; }
    spc_cyc(2);
    /* 1780 dec.w $0543 */
    spc_rmw_cyc(0x0543, spc_dec, 4, 1);
    /* 1783 mov.w a,$0549 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0549));
    /* 1786 mov.w y,$054B */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x054B));
    /* 1789 mov $20,#$3C */
    spc_st_cyc(spc_dp(0x20), 0x3C, 4, 1);
    /* 178C mov $21,#$00 */
    spc_st_cyc(spc_dp(0x21), 0x00, 4, 1);
    /* 178F subw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 1791 mov.w $0549,a */
    spc_st_cyc(0x0549, spc.a, 4, 1);
    /* 1794 mov.w $054B,y */
    spc_st_cyc(0x054B, spc.y, 4, 1);
    /* 1797 ret */
    spc_ret_cyc(4, 1);
    return;
L1798:
    /* 1798 clr1 $29.2 */
    spc_bit_set_cyc(0x29, 2, 0, 3, 1);
    /* 179A clr1 $29.3 */
    spc_bit_set_cyc(0x29, 3, 0, 3, 1);
    /* 179C mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 179E mov.w $0549,a */
    spc_st_cyc(0x0549, spc.a, 4, 1);
    /* 17A1 mov.w $054B,a */
    spc_st_cyc(0x054B, spc.a, 4, 1);
    /* 17A4 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D42F SpcPortCommand
 * WHAT IT DOES: The port 0 command $00 (bit 7 dropped): 6 and up is a sound command, $1D = value - 5; 1-5 (unless stopped, bit 0 of $29) is a song request, which also clears the tempo bend and fade state and flags 0, 1, 6 of $29 and 2, 3 of $2C; 0 leaves $1D = 0.
 * HOW: SBC #6 and BMI.
 * Entry: none. Exit: A as stored.
 * @spc 17A5
 * @spcuses 17A5-17DF
 */
void abi_SpcPortCommand(void)
{
    /* 17A5 mov $1D,#$00 */
    spc_st_cyc(spc_dp(0x1D), 0x00, 4, 1);
    /* 17A8 mov.b a,$00 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x00)));
    /* 17AA and a,#$7F */
    spc.a = spc_and(spc.a, 0x7F);
    spc_cyc(2);
    /* 17AC beq $17B7 */
    if (spc.z) { spc_cyc(4); goto L17B7; }
    spc_cyc(2);
    /* 17AE setc */
    spc.c = 1;
    spc_cyc(2);
    /* 17AF sbc a,#$06 */
    spc.a = spc_sbc(spc.a, 0x06);
    spc_cyc(2);
    /* 17B1 bmi $17BA */
    if (spc.n) { spc_cyc(4); goto L17BA; }
    spc_cyc(2);
    /* 17B3 inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
    /* 17B4 mov.b $1D,a */
    spc_st_cyc(spc_dp(0x1D), spc.a, 3, 1);
    /* 17B6 ret */
    spc_ret_cyc(4, 1);
    return;
L17B7:
    /* 17B7 mov.b $1D,a */
    spc_st_cyc(spc_dp(0x1D), spc.a, 3, 1);
    /* 17B9 ret */
    spc_ret_cyc(4, 1);
    return;
L17BA:
    /* 17BA bbs $29.0,$17DE */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x29)) >> 0 & 1)) { spc_cyc(4); goto L17DE; }
    spc_cyc(2);
    /* 17BD mov.b a,$00 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x00)));
    /* 17BF mov.b $1B,a */
    spc_st_cyc(spc_dp(0x1B), spc.a, 3, 1);
    /* 17C1 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    spc_cyc(2);
    /* 17C3 bne $17C5 */
    if (!spc.z) { spc_cyc(4); goto L17C5; }
    spc_cyc(2);
L17C5:
    /* 17C5 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 17C7 mov.b $2E,a */
    spc_st_cyc(spc_dp(0x2E), spc.a, 3, 1);
    /* 17C9 mov.b $2F,a */
    spc_st_cyc(spc_dp(0x2F), spc.a, 3, 1);
    /* 17CB mov.w $0553,a */
    spc_st_cyc(0x0553, spc.a, 4, 1);
    /* 17CE mov.w $0554,a */
    spc_st_cyc(0x0554, spc.a, 4, 1);
    /* 17D1 mov.w $0555,a */
    spc_st_cyc(0x0555, spc.a, 4, 1);
    /* 17D4 clr1 $29.0 */
    spc_bit_set_cyc(0x29, 0, 0, 3, 1);
    /* 17D6 clr1 $29.1 */
    spc_bit_set_cyc(0x29, 1, 0, 3, 1);
    /* 17D8 clr1 $29.6 */
    spc_bit_set_cyc(0x29, 6, 0, 3, 1);
    /* 17DA clr1 $2C.2 */
    spc_bit_set_cyc(0x2C, 2, 0, 3, 1);
    /* 17DC clr1 $2C.3 */
    spc_bit_set_cyc(0x2C, 3, 0, 3, 1);
L17DE:
    /* 17DE ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D469 SpcPort1Command
 * WHAT IT DOES: A new port 1 command ($01 differing from the last, $09; below $3E) runs its handler from the table at $1804: the handlers set flags in $28, $29, $2A and $2D, start or stop sounds and loops, ramps, the echo transition, pause, resume and fade the music.
 * HOW: JMP ($1804+X); the handlers end in RETs or tail jumps.
 * Entry: none. Exit: as the handler leaves it.
 * @spc 17DF
 * @spcuses 09E8-09EF,0A3B-0A3C,0A58-0A6D,1424-1436,1798-17A5,17DE-1AB0,1AE9-1AF6,1AFF-1B2C,1C84-1C8A,1C9E-1CCD,1D53-1D69,1E0F-1E28,1E93-1E9D,219C-21C8
 */
void abi_SpcPort1Command(void)
{
    /* 17DF mov.b a,$01 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x01)));
    /* 17E1 cmp.b a,$09 */
    spc_cyc(3);
    spc_cmp(spc.a, spc_rd(spc_dp(0x09)));
    /* 17E3 bne $17EA */
    if (!spc.z) { spc_cyc(4); goto L17EA; }
    spc_cyc(2);
    /* 17E5 mov $20,#$00 */
    spc_st_cyc(spc_dp(0x20), 0x00, 4, 1);
    /* 17E8 bra $17ED */
    spc_cyc(4);
    goto L17ED;
L17EA:
    /* 17EA mov $20,$01 */
    spc_cyc(3);
    spc_wr_cyc(spc_dp(0x20), spc_rd(spc_dp(0x01)), 2);
L17ED:
    /* 17ED mov $09,$01 */
    spc_cyc(3);
    spc_wr_cyc(spc_dp(0x09), spc_rd(spc_dp(0x01)), 2);
    /* 17F0 mov.b a,$20 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 17F2 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 17F3 sbc a,#$3E */
    spc.a = spc_sbc(spc.a, 0x3E);
    spc_cyc(2);
    /* 17F5 bmi $17FA */
    if (spc.n) { spc_cyc(4); goto L17FA; }
    spc_cyc(2);
    /* 17F7 mov $20,#$00 */
    spc_st_cyc(spc_dp(0x20), 0x00, 4, 1);
L17FA:
    /* 17FA mov.b a,$20 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 17FC beq $17DE */
    if (spc.z) { spc_cyc(4); goto L17DE; }
    spc_cyc(2);
    /* 17FE dec a */
    spc.a = spc_dec(spc.a);
    spc_cyc(2);
    /* 17FF asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 1800 mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 1801 jmp ($1804+x) */
    spc_cyc(5);
    { uint16_t t = spc_rd((uint16_t)(0x1804 + spc.x)); t |= (spc_cyc(1), spc_rd((uint16_t)(0x1804 + spc.x + 1))) << 8;
      switch (t) {
      case 0x187E: goto L187E;
      case 0x1887: goto L1887;
      case 0x1890: goto L1890;
      case 0x1899: goto L1899;
      case 0x18A2: goto L18A2;
      case 0x18AC: goto L18AC;
      case 0x18B9: goto L18B9;
      case 0x18D5: goto L18D5;
      case 0x18D6: goto L18D6;
      case 0x18E3: goto L18E3;
      case 0x18FF: goto L18FF;
      case 0x1908: { abi_SpcSfxStopIf(); return; }
      case 0x1915: goto L1915;
      case 0x1920: goto L1920;
      case 0x192B: goto L192B;
      case 0x1936: goto L1936;
      case 0x1951: { abi_SpcEchoModeA(); return; }
      case 0x1956: goto L1956;
      case 0x195B: goto L195B;
      case 0x1960: goto L1960;
      case 0x198A: goto L198A;
      case 0x19F9: goto L19F9;
      case 0x1AFF: goto L1AFF;
      case 0x1B09: goto L1B09;
      case 0x1B23: goto L1B23;
      } }
    return;
L187E:
    /* 187E bbs $2D.3,$18D5 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2D)) >> 3 & 1)) { spc_cyc(4); goto L18D5; }
    spc_cyc(2);
    /* 1881 set1 $2A.6 */
    spc_bit_set_cyc(0x2A, 6, 1, 3, 1);
    /* 1883 mov a,#$02 */
    spc.a = spc_nz(0x02);
    spc_cyc(2);
    /* 1885 bra $18A9 */
    spc_cyc(4);
    goto L18A9;
L1887:
    /* 1887 bbs $2D.6,$18D5 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2D)) >> 6 & 1)) { spc_cyc(4); goto L18D5; }
    spc_cyc(2);
    /* 188A set1 $2A.2 */
    spc_bit_set_cyc(0x2A, 2, 1, 3, 1);
    /* 188C mov a,#$25 */
    spc.a = spc_nz(0x25);
    spc_cyc(2);
    /* 188E bra $18A9 */
    spc_cyc(4);
    goto L18A9;
L1890:
    /* 1890 bbs $2D.6,$18D5 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2D)) >> 6 & 1)) { spc_cyc(4); goto L18D5; }
    spc_cyc(2);
    /* 1893 set1 $2A.4 */
    spc_bit_set_cyc(0x2A, 4, 1, 3, 1);
    /* 1895 mov a,#$15 */
    spc.a = spc_nz(0x15);
    spc_cyc(2);
    /* 1897 bra $18A9 */
    spc_cyc(4);
    goto L18A9;
L1899:
    /* 1899 bbs $2D.6,$18D5 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2D)) >> 6 & 1)) { spc_cyc(4); goto L18D5; }
    spc_cyc(2);
    /* 189C set1 $2A.0 */
    spc_bit_set_cyc(0x2A, 0, 1, 3, 1);
    /* 189E mov a,#$17 */
    spc.a = spc_nz(0x17);
    spc_cyc(2);
    /* 18A0 bra $18A9 */
    spc_cyc(4);
    goto L18A9;
L18A2:
    /* 18A2 bbs $2D.6,$18D5 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2D)) >> 6 & 1)) { spc_cyc(4); goto L18D5; }
    spc_cyc(2);
    /* 18A5 set1 $2A.7 */
    spc_bit_set_cyc(0x2A, 7, 1, 3, 1);
    /* 18A7 mov a,#$21 */
    spc.a = spc_nz(0x21);
    spc_cyc(2);
L18A9:
    /* 18A9 jmp $1424 */
    { spc_cyc(3); abi_SpcSoundQueuePush(); return; }
L18AC:
    /* 18AC bbs $29.2,$18D5 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x29)) >> 2 & 1)) { spc_cyc(4); goto L18D5; }
    spc_cyc(2);
    /* 18AF set1 $29.2 */
    spc_bit_set_cyc(0x29, 2, 1, 3, 1);
    /* 18B1 clr1 $29.3 */
    spc_bit_set_cyc(0x29, 3, 0, 3, 1);
    /* 18B3 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 18B5 mov y,#$02 */
    spc.y = spc_nz(0x02);
    spc_cyc(2);
    /* 18B7 bra $18CE */
    spc_cyc(4);
    goto L18CE;
L18B9:
    /* 18B9 bbc $29.2,$18D5 */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x29)) >> 2 & 1)) { spc_cyc(4); goto L18D5; }
    spc_cyc(2);
    /* 18BC clr1 $29.2 */
    spc_bit_set_cyc(0x29, 2, 0, 3, 1);
    /* 18BE set1 $29.3 */
    spc_bit_set_cyc(0x29, 3, 1, 3, 1);
    /* 18C0 mov.w a,$0549 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0549));
    /* 18C3 mov.w y,$054B */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x054B));
    /* 18C6 mov $20,#$00 */
    spc_st_cyc(spc_dp(0x20), 0x00, 4, 1);
    /* 18C9 mov $21,#$02 */
    spc_st_cyc(spc_dp(0x21), 0x02, 4, 1);
    /* 18CC subw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
L18CE:
    /* 18CE mov.w $0549,a */
    spc_st_cyc(0x0549, spc.a, 4, 1);
    /* 18D1 mov.w $054B,y */
    spc_st_cyc(0x054B, spc.y, 4, 1);
    /* 18D4 ret */
    spc_ret_cyc(4, 1);
    return;
L18D5:
    /* 18D5 ret */
    spc_ret_cyc(4, 1);
    return;
L18D6:
    /* 18D6 bbs $28.2,$18D5 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x28)) >> 2 & 1)) { spc_cyc(4); goto L18D5; }
    spc_cyc(2);
    /* 18D9 set1 $28.2 */
    spc_bit_set_cyc(0x28, 2, 1, 3, 1);
    /* 18DB clr1 $28.3 */
    spc_bit_set_cyc(0x28, 3, 0, 3, 1);
    /* 18DD mov a,#$40 */
    spc.a = spc_nz(0x40);
    spc_cyc(2);
    /* 18DF mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 18E1 bra $18F8 */
    spc_cyc(4);
    goto L18F8;
L18E3:
    /* 18E3 bbc $28.2,$18D5 */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x28)) >> 2 & 1)) { spc_cyc(4); goto L18D5; }
    spc_cyc(2);
    /* 18E6 clr1 $28.2 */
    spc_bit_set_cyc(0x28, 2, 0, 3, 1);
    /* 18E8 set1 $28.3 */
    spc_bit_set_cyc(0x28, 3, 1, 3, 1);
    /* 18EA mov.w a,$054D */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x054D));
    /* 18ED mov.w y,$054F */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x054F));
    /* 18F0 mov $20,#$40 */
    spc_st_cyc(spc_dp(0x20), 0x40, 4, 1);
    /* 18F3 mov $21,#$00 */
    spc_st_cyc(spc_dp(0x21), 0x00, 4, 1);
    /* 18F6 subw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
L18F8:
    /* 18F8 mov.w $054D,a */
    spc_st_cyc(0x054D, spc.a, 4, 1);
    /* 18FB mov.w $054F,y */
    spc_st_cyc(0x054F, spc.y, 4, 1);
    /* 18FE ret */
    spc_ret_cyc(4, 1);
    return;
L18FF:
    /* 18FF mov.b a,$20 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 1901 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 1902 sbc a,#$32 */
    spc.a = spc_sbc(spc.a, 0x32);
    spc_cyc(2);
    /* 1904 mov.w $0530,a */
    spc_st_cyc(0x0530, spc.a, 4, 1);
    /* 1907 ret */
    spc_ret_cyc(4, 1);
    return;
L1915:
    /* 1915 clr1 $2A.2 */
    spc_bit_set_cyc(0x2A, 2, 0, 3, 1);
    /* 1917 mov.w a,$0395 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0395));
    /* 191A cmp a,#$25 */
    spc_cmp(spc.a, 0x25);
    spc_cyc(2);
    /* 191C bne $194B */
    if (!spc.z) { spc_cyc(4); goto L194B; }
    spc_cyc(2);
    /* 191E bra $193F */
    spc_cyc(4);
    goto L193F;
L1920:
    /* 1920 clr1 $2A.4 */
    spc_bit_set_cyc(0x2A, 4, 0, 3, 1);
    /* 1922 mov.w a,$0395 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0395));
    /* 1925 cmp a,#$15 */
    spc_cmp(spc.a, 0x15);
    spc_cyc(2);
    /* 1927 bne $194B */
    if (!spc.z) { spc_cyc(4); goto L194B; }
    spc_cyc(2);
    /* 1929 bra $193F */
    spc_cyc(4);
    goto L193F;
L192B:
    /* 192B clr1 $2A.0 */
    spc_bit_set_cyc(0x2A, 0, 0, 3, 1);
    /* 192D mov.w a,$0395 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0395));
    /* 1930 cmp a,#$17 */
    spc_cmp(spc.a, 0x17);
    spc_cyc(2);
    /* 1932 bne $194B */
    if (!spc.z) { spc_cyc(4); goto L194B; }
    spc_cyc(2);
    /* 1934 bra $193F */
    spc_cyc(4);
    goto L193F;
L1936:
    /* 1936 clr1 $2A.7 */
    spc_bit_set_cyc(0x2A, 7, 0, 3, 1);
    /* 1938 mov.w a,$0395 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0395));
    /* 193B cmp a,#$21 */
    spc_cmp(spc.a, 0x21);
    spc_cyc(2);
    /* 193D bne $194B */
    if (!spc.z) { spc_cyc(4); goto L194B; }
    spc_cyc(2);
L193F:
    /* 193F mov x,#$05 */
    spc.x = spc_nz(0x05);
    spc_cyc(2);
    /* 1941 mov.w a,$0408+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 1944 beq $194B */
    if (spc.z) { spc_cyc(4); goto L194B; }
    spc_cyc(2);
    /* 1946 mov.b a,$20 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 1948 mov.w $03E0+x,a */
    spc_st_cyc((uint16_t)(0x03E0 + spc.x), spc.a, 5, 1);
L194B:
    /* 194B ret */
    spc_ret_cyc(4, 1);
    return;
L1956:
    /* 1956 clr1 $28.0 */
    spc_bit_set_cyc(0x28, 0, 0, 3, 1);
    /* 1958 clr1 $28.1 */
    spc_bit_set_cyc(0x28, 1, 0, 3, 1);
    /* 195A ret */
    spc_ret_cyc(4, 1);
    return;
L195B:
    /* 195B clr1 $28.0 */
    spc_bit_set_cyc(0x28, 0, 0, 3, 1);
    /* 195D set1 $28.1 */
    spc_bit_set_cyc(0x28, 1, 1, 3, 1);
L195F:
    /* 195F ret */
    spc_ret_cyc(4, 1);
    return;
L1960:
    /* 1960 bbc $29.0,$195F */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x29)) >> 0 & 1)) { spc_cyc(4); goto L195F; }
    spc_cyc(2);
    /* 1963 mov a,#$4C */
    spc.a = spc_nz(0x4C);
    spc_cyc(2);
    /* 1965 call $1424 */
    spc_call_cyc(abi_SpcSoundQueuePush, 0x1968, 5, 1, 2);
    /* 1968 clr1 $29.0 */
    spc_bit_set_cyc(0x29, 0, 0, 3, 1);
    /* 196A mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 196C mov.w $045F,a */
    spc_st_cyc(0x045F, spc.a, 4, 1);
    /* 196F mov.w $045E,a */
    spc_st_cyc(0x045E, spc.a, 4, 1);
    /* 1972 mov.w $045D,a */
    spc_st_cyc(0x045D, spc.a, 4, 1);
    /* 1975 mov.w $045C,a */
    spc_st_cyc(0x045C, spc.a, 4, 1);
    /* 1978 mov.w $04FF,a */
    spc_st_cyc(0x04FF, spc.a, 4, 1);
    /* 197B mov.w $04FE,a */
    spc_st_cyc(0x04FE, spc.a, 4, 1);
    /* 197E mov.w $04FD,a */
    spc_st_cyc(0x04FD, spc.a, 4, 1);
    /* 1981 mov.w $04FC,a */
    spc_st_cyc(0x04FC, spc.a, 4, 1);
    /* 1984 call $0A60 */
    spc_call_cyc(abi_SpcMusicPause, 0x1987, 5, 1, 2);
    /* 1987 jmp $1798 */
    spc_cyc(3);
    goto L1798;
L198A:
    /* 198A bbc $29.0,$195F */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x29)) >> 0 & 1)) { spc_cyc(4); goto L195F; }
    spc_cyc(2);
    /* 198D mov $2A,$2B */
    spc_cyc(3);
    spc_wr_cyc(spc_dp(0x2A), spc_rd(spc_dp(0x2B)), 2);
    /* 1990 bbc $29.7,$19A9 */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x29)) >> 7 & 1)) { spc_cyc(4); goto L19A9; }
    spc_cyc(2);
    /* 1993 clr1 $29.7 */
    spc_bit_set_cyc(0x29, 7, 0, 3, 1);
    /* 1995 mov.w a,$0574 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0574));
    /* 1998 mov.b $4A,a */
    spc_st_cyc(spc_dp(0x4A), spc.a, 3, 1);
    /* 199A mov.w a,$0575 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0575));
    /* 199D mov.b $4E,a */
    spc_st_cyc(spc_dp(0x4E), spc.a, 3, 1);
    /* 199F mov.w a,$0576 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0576));
    /* 19A2 mov.b $61,a */
    spc_st_cyc(spc_dp(0x61), spc.a, 3, 1);
    /* 19A4 mov.w a,$0577 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0577));
    /* 19A7 mov.b $63,a */
    spc_st_cyc(spc_dp(0x63), spc.a, 3, 1);
L19A9:
    /* 19A9 mov a,#$4B */
    spc.a = spc_nz(0x4B);
    spc_cyc(2);
    /* 19AB call $1424 */
    spc_call_cyc(abi_SpcSoundQueuePush, 0x19AE, 5, 1, 2);
    /* 19AE clr1 $29.0 */
    spc_bit_set_cyc(0x29, 0, 0, 3, 1);
    /* 19B0 clr1 $2D.6 */
    spc_bit_set_cyc(0x2D, 6, 0, 3, 1);
    /* 19B2 mov x,#$07 */
    spc.x = spc_nz(0x07);
    spc_cyc(2);
    /* 19B4 call $19CC */
    spc_call_cyc(abi_SpcSfxChannelResume, 0x19B7, 5, 1, 2);
    /* 19B7 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 19B8 call $19CC */
    spc_call_cyc(abi_SpcSfxChannelResume, 0x19BB, 5, 1, 2);
    /* 19BB dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 19BC call $19CC */
    spc_call_cyc(abi_SpcSfxChannelResume, 0x19BF, 5, 1, 2);
    /* 19BF dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 19C0 call $19CC */
    spc_call_cyc(abi_SpcSfxChannelResume, 0x19C3, 5, 1, 2);
    /* 19C3 mov x,#$01 */
    spc.x = spc_nz(0x01);
    spc_cyc(2);
    /* 19C5 call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x19C8, 5, 1, 2);
    /* 19C8 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 19C9 jmp $194C */
    { spc_cyc(3); abi_SpcSfxStop(); return; }
L19F9:
    /* 19F9 bbs $29.0,$1A67 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x29)) >> 0 & 1)) { spc_cyc(4); goto L1A67; }
    spc_cyc(2);
    /* 19FC mov $2B,$2A */
    spc_cyc(3);
    spc_wr_cyc(spc_dp(0x2B), spc_rd(spc_dp(0x2A)), 2);
    /* 19FF mov $2A,#$00 */
    spc_st_cyc(spc_dp(0x2A), 0x00, 4, 1);
    /* 1A02 mov.w a,$0551 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0551));
    /* 1A05 beq $1A2B */
    if (spc.z) { spc_cyc(4); goto L1A2B; }
    spc_cyc(2);
    /* 1A07 set1 $29.7 */
    spc_bit_set_cyc(0x29, 7, 1, 3, 1);
    /* 1A09 set1 $2D.6 */
    spc_bit_set_cyc(0x2D, 6, 1, 3, 1);
    /* 1A0B mov.b a,$4A */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x4A)));
    /* 1A0D mov.w $0574,a */
    spc_st_cyc(0x0574, spc.a, 4, 1);
    /* 1A10 mov.b a,$4E */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x4E)));
    /* 1A12 mov.w $0575,a */
    spc_st_cyc(0x0575, spc.a, 4, 1);
    /* 1A15 mov.b a,$61 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x61)));
    /* 1A17 mov.w $0576,a */
    spc_st_cyc(0x0576, spc.a, 4, 1);
    /* 1A1A mov.b a,$63 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x63)));
    /* 1A1C mov.w $0577,a */
    spc_st_cyc(0x0577, spc.a, 4, 1);
    /* 1A1F mov a,#$50 */
    spc.a = spc_nz(0x50);
    spc_cyc(2);
    /* 1A21 mov.b $4E,a */
    spc_st_cyc(spc_dp(0x4E), spc.a, 3, 1);
    /* 1A23 mov a,#$11 */
    spc.a = spc_nz(0x11);
    spc_cyc(2);
    /* 1A25 mov.b $61,a */
    spc_st_cyc(spc_dp(0x61), spc.a, 3, 1);
    /* 1A27 mov a,#$11 */
    spc.a = spc_nz(0x11);
    spc_cyc(2);
    /* 1A29 mov.b $63,a */
    spc_st_cyc(spc_dp(0x63), spc.a, 3, 1);
L1A2B:
    /* 1A2B set1 $29.0 */
    spc_bit_set_cyc(0x29, 0, 1, 3, 1);
    /* 1A2D mov a,#$4A */
    spc.a = spc_nz(0x4A);
    spc_cyc(2);
    /* 1A2F call $1424 */
    spc_call_cyc(abi_SpcSoundQueuePush, 0x1A32, 5, 1, 2);
    /* 1A32 mov x,#$07 */
    spc.x = spc_nz(0x07);
    spc_cyc(2);
    /* 1A34 call $1A54 */
    spc_call_cyc(abi_SpcSfxChannelPause, 0x1A37, 5, 1, 2);
    /* 1A37 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 1A38 call $1A54 */
    spc_call_cyc(abi_SpcSfxChannelPause, 0x1A3B, 5, 1, 2);
    /* 1A3B dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 1A3C call $1A54 */
    spc_call_cyc(abi_SpcSfxChannelPause, 0x1A3F, 5, 1, 2);
    /* 1A3F dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 1A40 call $1A54 */
    spc_call_cyc(abi_SpcSfxChannelPause, 0x1A43, 5, 1, 2);
    /* 1A43 mov x,#$07 */
    spc.x = spc_nz(0x07);
    spc_cyc(2);
    /* 1A45 call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x1A48, 5, 1, 2);
    /* 1A48 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 1A49 call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x1A4C, 5, 1, 2);
    /* 1A4C dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 1A4D call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x1A50, 5, 1, 2);
    /* 1A50 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 1A51 jmp $194C */
    { spc_cyc(3); abi_SpcSfxStop(); return; }
L1A67:
    /* 1A67 ret */
    spc_ret_cyc(4, 1);
    return;
L1AE9:
    /* 1AE9 call $1AEF */
    spc_call_cyc(abi_SpcKeyOffAll, 0x1AEC, 5, 1, 2);
    /* 1AEC jmp $0675 */
    { spc_cyc(3); spc_restart(0x0675, abi_SpcReload); return; }
L1AFF:
    /* 1AFF call $1A83 */
    spc_call_cyc(abi_SpcSfxStopAll, 0x1B02, 5, 1, 2);
    /* 1B02 mov.b a,$59 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x59)));
    /* 1B04 beq $1AE9 */
    if (spc.z) { spc_cyc(4); goto L1AE9; }
    spc_cyc(2);
    /* 1B06 set1 $2C.7 */
    spc_bit_set_cyc(0x2C, 7, 1, 3, 1);
    /* 1B08 ret */
    spc_ret_cyc(4, 1);
    return;
L1B09:
    /* 1B09 set1 $2D.0 */
    spc_bit_set_cyc(0x2D, 0, 1, 3, 1);
    /* 1B0B call $1A83 */
    spc_call_cyc(abi_SpcSfxStopAll, 0x1B0E, 5, 1, 2);
    /* 1B0E mov.b a,$59 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x59)));
    /* 1B10 beq $1B15 */
    if (spc.z) { spc_cyc(4); goto L1B15; }
    spc_cyc(2);
    /* 1B12 set1 $2D.0 */
    spc_bit_set_cyc(0x2D, 0, 1, 3, 1);
    /* 1B14 ret */
    spc_ret_cyc(4, 1);
    return;
L1B15:
    /* 1B15 call $1AEF */
    spc_call_cyc(abi_SpcKeyOffAll, 0x1B18, 5, 1, 2);
    /* 1B18 mov.w a,$00F1 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x00F1));
    /* 1B1B or a,#$80 */
    spc.a = spc_or(spc.a, 0x80);
    spc_cyc(2);
    /* 1B1D mov.w $00F1,a */
    spc_st_cyc(0x00F1, spc.a, 4, 1);
    /* 1B20 jmp $FFC0 */
    { spc_cyc(3); spc_boot_rom(); return; }
L1B23:
    /* 1B23 mov.b a,$20 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 1B25 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 1B26 sbc a,#$2A */
    spc.a = spc_sbc(spc.a, 0x2A);
    spc_cyc(2);
    /* 1B28 mov.w $0562,a */
    spc_st_cyc(0x0562, spc.a, 4, 1);
    /* 1B2B ret */
    spc_ret_cyc(4, 1);
    return;
L1798:
    /* 1798 clr1 $29.2 */
    spc_bit_set_cyc(0x29, 2, 0, 3, 1);
    /* 179A clr1 $29.3 */
    spc_bit_set_cyc(0x29, 3, 0, 3, 1);
    /* 179C mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 179E mov.w $0549,a */
    spc_st_cyc(0x0549, spc.a, 4, 1);
    /* 17A1 mov.w $054B,a */
    spc_st_cyc(0x054B, spc.a, 4, 1);
    /* 17A4 ret */
    spc_ret_cyc(4, 1);
    return;
L17DE:
    /* 17DE ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D592 SpcSfxStopIf
 * WHAT IT DOES: Clears bit 6 of $2A and, when channel 4's sound ($0394) is 2, requests channel 4's stop with the reason $20; the entries after it do the same for channel 5 and its sounds $25, $15, $17, $21.
 * HOW: The stop request at $03E0 + X when the channel is active ($0408 + X).
 * Entry: $20 = reason. Exit: A, X changed.
 * @spc 1908
 * @spcuses 1908-1915,1941-194C
 */
void abi_SpcSfxStopIf(void)
{
    /* 1908 clr1 $2A.6 */
    spc_bit_set_cyc(0x2A, 6, 0, 3, 1);
    /* 190A mov.w a,$0394 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0394));
    /* 190D cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    spc_cyc(2);
    /* 190F bne $194B */
    if (!spc.z) { spc_cyc(4); goto L194B; }
    spc_cyc(2);
    /* 1911 mov x,#$04 */
    spc.x = spc_nz(0x04);
    spc_cyc(2);
    /* 1913 bra $1941 */
    spc_cyc(4);
    goto L1941;
L1941:
    /* 1941 mov.w a,$0408+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 1944 beq $194B */
    if (spc.z) { spc_cyc(4); goto L194B; }
    spc_cyc(2);
    /* 1946 mov.b a,$20 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 1948 mov.w $03E0+x,a */
    spc_st_cyc((uint16_t)(0x03E0 + spc.x), spc.a, 5, 1);
L194B:
    /* 194B ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D5D6 SpcSfxStop
 * WHAT IT DOES: Requests the stop of channel X (reason 1) when it is active.
 * HOW: $20 = 1, then SpcSfxStopIf's request.
 * Entry: X = channel (0-7). Exit: A changed.
 * @spc 194C
 * @spcuses 1941-1951
 */
void abi_SpcSfxStop(void)
{
    /* 194C mov $20,#$01 */
    spc_st_cyc(spc_dp(0x20), 0x01, 4, 1);
    /* 194F bra $1941 */
    spc_cyc(4);
    goto L1941;
L1941:
    /* 1941 mov.w a,$0408+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 1944 beq $194B */
    if (spc.z) { spc_cyc(4); goto L194B; }
    spc_cyc(2);
    /* 1946 mov.b a,$20 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 1948 mov.w $03E0+x,a */
    spc_st_cyc((uint16_t)(0x03E0 + spc.x), spc.a, 5, 1);
L194B:
    /* 194B ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D5DB SpcEchoModeA
 * WHAT IT DOES: Port 1 handler: $28 bit 0 set, bit 1 cleared (the echo transition's first mode).
 * HOW: SET1/CLR1.
 * Entry: none. Exit: none changed but $28.
 * @spc 1951
 * @spcuses 1951-1956
 */
void abi_SpcEchoModeA(void)
{
    /* 1951 clr1 $28.1 */
    spc_bit_set_cyc(0x28, 1, 0, 3, 1);
    /* 1953 set1 $28.0 */
    spc_bit_set_cyc(0x28, 0, 1, 3, 1);
    /* 1955 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D656 SpcSfxChannelResume
 * WHAT IT DOES: Channel X after a pause: its sound number $03C8 back from $0460, and a sound saved at $0458 restarted (SpcSfxPointer, SpcSfxStart); without one the channel stops.
 * HOW: Two table copies and CALLs.
 * Entry: X = channel. Exit: X kept.
 * @spc 19CC
 * @spcuses 1941-1951,19CC-19F9,1C84-1C8A,1C9E-1CCD,1D53-1D69,1E0F-1E28,1E93-1E9D,219C-21C8
 */
void abi_SpcSfxChannelResume(void)
{
    /* 19CC mov.w a,$0460+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0460 + spc.x)));
    /* 19CF mov.w $03C8+x,a */
    spc_st_cyc((uint16_t)(0x03C8 + spc.x), spc.a, 5, 1);
    /* 19D2 mov.w a,$0458+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0458 + spc.x)));
    /* 19D5 beq $19E7 */
    if (spc.z) { spc_cyc(4); goto L19E7; }
    spc_cyc(2);
    /* 19D7 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 19D8 call $19EA */
    spc_call_cyc(abi_SpcSfxPointer, 0x19DB, 5, 1, 2);
    /* 19DB mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 19DD mov.w $0458+x,a */
    spc_st_cyc((uint16_t)(0x0458 + spc.x), spc.a, 5, 1);
    /* 19E0 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 19E1 push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 19E2 call $219C */
    spc_call_cyc(abi_SpcSfxStart, 0x19E5, 5, 1, 2);
    /* 19E5 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 19E6 ret */
    spc_ret_cyc(4, 1);
    return;
L19E7:
    /* 19E7 jmp $194C */
    { spc_cyc(3); abi_SpcSfxStop(); return; }
}

/*
 * @routine 19:D674 SpcSfxPointer
 * WHAT IT DOES: The data pointer of sound A from the table at $0702 into $24/$25.
 * HOW: (A - 1) * 2 as the index.
 * Entry: A = sound (1 and up). Exit: Y = index + 1.
 * @spc 19EA
 * @spcuses 19EA-19F9
 */
void abi_SpcSfxPointer(void)
{
    /* 19EA dec a */
    spc.a = spc_dec(spc.a);
    spc_cyc(2);
    /* 19EB asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 19EC mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 19ED mov.w a,$0702+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0702 + spc.y)));
    /* 19F0 mov.b $24,a */
    spc_st_cyc(spc_dp(0x24), spc.a, 3, 1);
    /* 19F2 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 19F3 mov.w a,$0702+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0702 + spc.y)));
    /* 19F6 mov.b $25,a */
    spc_st_cyc(spc_dp(0x25), spc.a, 3, 1);
    /* 19F8 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D6DE SpcSfxChannelPause
 * WHAT IT DOES: Channel X at a pause: a pending queued sound or a running looped one ($0480) is saved at $0458 with its sound number, to restart after.
 * HOW: Compares on $04F8, $0480.
 * Entry: X = channel. Exit: A changed.
 * @spc 1A54
 * @spcuses 1A54-1A83
 */
void abi_SpcSfxChannelPause(void)
{
    /* 1A54 mov.w a,$04F8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F8 + spc.x)));
    /* 1A57 beq $1A68 */
    if (spc.z) { spc_cyc(4); goto L1A68; }
    spc_cyc(2);
L1A59:
    /* 1A59 mov.w a,$04F8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F8 + spc.x)));
    /* 1A5C mov.w $0458+x,a */
    spc_st_cyc((uint16_t)(0x0458 + spc.x), spc.a, 5, 1);
    /* 1A5F mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1A61 mov.w $04F8+x,a */
    spc_st_cyc((uint16_t)(0x04F8 + spc.x), spc.a, 5, 1);
    /* 1A64 mov.w $0480+x,a */
    spc_st_cyc((uint16_t)(0x0480 + spc.x), spc.a, 5, 1);
L1A67:
    /* 1A67 ret */
    spc_ret_cyc(4, 1);
    return;
L1A68:
    /* 1A68 mov.w a,$0480+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0480 + spc.x)));
    /* 1A6B beq $1A67 */
    if (spc.z) { spc_cyc(4); goto L1A67; }
    spc_cyc(2);
    /* 1A6D cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    spc_cyc(2);
    /* 1A6F beq $1A59 */
    if (spc.z) { spc_cyc(4); goto L1A59; }
    spc_cyc(2);
    /* 1A71 mov.w a,$03C8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03C8 + spc.x)));
    /* 1A74 mov.w $0460+x,a */
    spc_st_cyc((uint16_t)(0x0460 + spc.x), spc.a, 5, 1);
    /* 1A77 mov.w a,$0390+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1A7A mov.w $0458+x,a */
    spc_st_cyc((uint16_t)(0x0458 + spc.x), spc.a, 5, 1);
    /* 1A7D mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1A7F mov.w $0480+x,a */
    spc_st_cyc((uint16_t)(0x0480 + spc.x), spc.a, 5, 1);
    /* 1A82 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D70D SpcSfxStopAll
 * WHAT IT DOES: Stops every sound effect channel with a fade, $2A cleared, and the music fade-out started (SpcFadeOutStart).
 * HOW: SpcSfxStopRequest for X = 7 down to 0.
 * Entry: none. Exit: X = $FF.
 * @spc 1A83
 * @spcuses 1A83-1AB0
 */
void abi_SpcSfxStopAll(void)
{
    /* 1A83 mov $2A,#$00 */
    spc_st_cyc(spc_dp(0x2A), 0x00, 4, 1);
    /* 1A86 call $1AAA */
    spc_call_cyc(abi_SpcFadeOutStart, 0x1A89, 5, 1, 2);
    /* 1A89 mov x,#$07 */
    spc.x = spc_nz(0x07);
    spc_cyc(2);
L1A8B:
    /* 1A8B call $1A92 */
    spc_call_cyc(abi_SpcSfxStopRequest, 0x1A8E, 5, 1, 2);
    /* 1A8E dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 1A8F bpl $1A8B */
    if (!spc.n) { spc_cyc(4); goto L1A8B; }
    spc_cyc(2);
    /* 1A91 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D71C SpcSfxStopRequest
 * WHAT IT DOES: Channel X's queued and looping state cleared ($04F8, $0480, $2A) and, if active, its fade-out started ($04C0, $04C8 = 1).
 * HOW: Stores.
 * Entry: X = channel. Exit: A = 0 or 1.
 * @spc 1A92
 * @spcuses 1A92-1AAA
 */
void abi_SpcSfxStopRequest(void)
{
    /* 1A92 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1A94 mov.w $04F8+x,a */
    spc_st_cyc((uint16_t)(0x04F8 + spc.x), spc.a, 5, 1);
    /* 1A97 mov.w $0480+x,a */
    spc_st_cyc((uint16_t)(0x0480 + spc.x), spc.a, 5, 1);
    /* 1A9A mov.b $2A,a */
    spc_st_cyc(spc_dp(0x2A), spc.a, 3, 1);
    /* 1A9C mov.w a,$0408+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 1A9F beq $1AA9 */
    if (spc.z) { spc_cyc(4); goto L1AA9; }
    spc_cyc(2);
    /* 1AA1 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 1AA3 mov.w $04C0+x,a */
    spc_st_cyc((uint16_t)(0x04C0 + spc.x), spc.a, 5, 1);
    /* 1AA6 mov.w $04C8+x,a */
    spc_st_cyc((uint16_t)(0x04C8 + spc.x), spc.a, 5, 1);
L1AA9:
    /* 1AA9 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D734 SpcFadeOutStart
 * WHAT IT DOES: Unless stopped (bit 0 of $29), starts the music fade-out (bit 6 of $29).
 * HOW: SET1.
 * Entry: none. Exit: none.
 * @spc 1AAA
 * @spcuses 1AAA-1AB0
 */
void abi_SpcFadeOutStart(void)
{
    /* 1AAA bbs $29.0,$1AAF */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x29)) >> 0 & 1)) { spc_cyc(4); goto L1AAF; }
    spc_cyc(2);
    /* 1AAD set1 $29.6 */
    spc_bit_set_cyc(0x29, 6, 1, 3, 1);
L1AAF:
    /* 1AAF ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D73A SpcMusicFade
 * WHAT IT DOES: The music fade: while bit 6 of $29 is set, the fade amount $2E/$2F grows by $2F0 a tick and the volume scale $0565 is the start level $0564 less it; at the bottom the fade ends and, by flags, the driver reloads (bit 7 of $2C: all keys off, SpcReload), returns to the IPL ROM (bit 0 of $2D) or pauses the music. Without a fade, the scale follows the master volume.
 * HOW: SUBW from $0564 in the high byte.
 * Entry: none. Exit: as the chosen path leaves it.
 * @spc 1AB0
 * @spcuses 09E8-09EF,0A3B-0A3C,0A58-0A6D,1AB0-1AFF,1B15-1B23
 */
void abi_SpcMusicFade(void)
{
    /* 1AB0 bbc $29.6,$1AF6 */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x29)) >> 6 & 1)) { spc_cyc(4); goto L1AF6; }
    spc_cyc(2);
    /* 1AB3 mov a,#$F0 */
    spc.a = spc_nz(0xF0);
    spc_cyc(2);
    /* 1AB5 mov y,#$02 */
    spc.y = spc_nz(0x02);
    spc_cyc(2);
    /* 1AB7 addw ya,$2E */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x2E)); v |= (spc_cyc(2), spc_rd(spc_dp(0x2F))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 1AB9 mov.b $2E,a */
    spc_st_cyc(spc_dp(0x2E), spc.a, 3, 1);
    /* 1ABB mov.b $2F,y */
    spc_st_cyc(spc_dp(0x2F), spc.y, 3, 1);
    /* 1ABD mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1ABF mov.w y,$0564 */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x0564));
    /* 1AC2 subw ya,$2E */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x2E)); v |= (spc_cyc(2), spc_rd(spc_dp(0x2F))) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 1AC4 mov.w $0565,y */
    spc_st_cyc(0x0565, spc.y, 4, 1);
    /* 1AC7 cmp y,#$00 */
    spc_cmp(spc.y, 0x00);
    spc_cyc(2);
    /* 1AC9 beq $1AD8 */
    if (spc.z) { spc_cyc(4); goto L1AD8; }
    spc_cyc(2);
    /* 1ACB cmp y,#$01 */
    spc_cmp(spc.y, 0x01);
    spc_cyc(2);
    /* 1ACD beq $1AD8 */
    if (spc.z) { spc_cyc(4); goto L1AD8; }
    spc_cyc(2);
    /* 1ACF cmp y,#$02 */
    spc_cmp(spc.y, 0x02);
    spc_cyc(2);
    /* 1AD1 beq $1AD8 */
    if (spc.z) { spc_cyc(4); goto L1AD8; }
    spc_cyc(2);
    /* 1AD3 cmp y,#$03 */
    spc_cmp(spc.y, 0x03);
    spc_cyc(2);
    /* 1AD5 beq $1AD8 */
    if (spc.z) { spc_cyc(4); goto L1AD8; }
    spc_cyc(2);
    /* 1AD7 ret */
    spc_ret_cyc(4, 1);
    return;
L1AD8:
    /* 1AD8 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1ADA mov.b $2E,a */
    spc_st_cyc(spc_dp(0x2E), spc.a, 3, 1);
    /* 1ADC mov.b $2F,a */
    spc_st_cyc(spc_dp(0x2F), spc.a, 3, 1);
    /* 1ADE clr1 $29.6 */
    spc_bit_set_cyc(0x29, 6, 0, 3, 1);
    /* 1AE0 bbs $2C.7,$1AE9 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2C)) >> 7 & 1)) { spc_cyc(4); goto L1AE9; }
    spc_cyc(2);
    /* 1AE3 bbs $2D.0,$1B15 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2D)) >> 0 & 1)) { spc_cyc(4); goto L1B15; }
    spc_cyc(2);
    /* 1AE6 jmp $0A60 */
    { spc_cyc(3); abi_SpcMusicPause(); return; }
L1AE9:
    /* 1AE9 call $1AEF */
    spc_call_cyc(abi_SpcKeyOffAll, 0x1AEC, 5, 1, 2);
    /* 1AEC jmp $0675 */
    { spc_cyc(3); spc_restart(0x0675, abi_SpcReload); return; }
L1AF6:
    /* 1AF6 mov.b a,$59 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x59)));
    /* 1AF8 mov.w $0564,a */
    spc_st_cyc(0x0564, spc.a, 4, 1);
    /* 1AFB mov.w $0565,a */
    spc_st_cyc(0x0565, spc.a, 4, 1);
    /* 1AFE ret */
    spc_ret_cyc(4, 1);
    return;
L1B15:
    /* 1B15 call $1AEF */
    spc_call_cyc(abi_SpcKeyOffAll, 0x1B18, 5, 1, 2);
    /* 1B18 mov.w a,$00F1 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x00F1));
    /* 1B1B or a,#$80 */
    spc.a = spc_or(spc.a, 0x80);
    spc_cyc(2);
    /* 1B1D mov.w $00F1,a */
    spc_st_cyc(0x00F1, spc.a, 4, 1);
    /* 1B20 jmp $FFC0 */
    { spc_cyc(3); spc_boot_rom(); return; }
}

/*
 * @routine 19:D779 SpcKeyOffAll
 * WHAT IT DOES: Keys off all eight voices.
 * HOW: DSP KOF ($5C) = $FF through SpcDspWrite.
 * Entry: none. Exit: A = $FF, Y = $5C.
 * @spc 1AEF
 * @spcuses 09E8-09EF,1AEF-1AF6
 */
void abi_SpcKeyOffAll(void)
{
    /* 1AEF mov a,#$FF */
    spc.a = spc_nz(0xFF);
    spc_cyc(2);
    /* 1AF1 mov y,#$5C */
    spc.y = spc_nz(0x5C);
    spc_cyc(2);
    /* 1AF3 jmp $09E8 */
    { spc_cyc(3); abi_SpcDspWrite(); return; }
}

/*
 * @routine 19:D7B6 SpcCommandChannelCheck
 * WHAT IT DOES: A sound command $1D outside the list at $26D6 is checked against the list at $26E4: when an earlier-listed sound already plays on its channel, the command is dropped; the command kept is also stored at $1F.
 * HOW: List scans; SpcSfxLookup, SpcSfxChannel.
 * Entry: none. Exit: X = channel.
 * @spc 1B2C
 * @spcuses 1B2C-1B7A,1C8A-1CA5
 */
void abi_SpcCommandChannelCheck(void)
{
    /* 1B2C mov.b a,$1D */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 1B2E beq $1B76 */
    if (spc.z) { spc_cyc(4); goto L1B76; }
    spc_cyc(2);
    /* 1B30 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
L1B32:
    /* 1B32 mov.w a,$26D6+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x26D6 + spc.y)));
    /* 1B35 beq $1B3E */
    if (spc.z) { spc_cyc(4); goto L1B3E; }
    spc_cyc(2);
    /* 1B37 cmp.b a,$1D */
    spc_cyc(3);
    spc_cmp(spc.a, spc_rd(spc_dp(0x1D)));
    /* 1B39 beq $1B79 */
    if (spc.z) { spc_cyc(4); goto L1B79; }
    spc_cyc(2);
    /* 1B3B inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 1B3C bra $1B32 */
    spc_cyc(4);
    goto L1B32;
L1B3E:
    /* 1B3E mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
L1B40:
    /* 1B40 mov.w a,$26E4+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x26E4 + spc.y)));
    /* 1B43 beq $1B79 */
    if (spc.z) { spc_cyc(4); goto L1B79; }
    spc_cyc(2);
    /* 1B45 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 1B46 mov.b a,$1D */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 1B48 push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 1B49 call $1C8A */
    spc_call_cyc(abi_SpcSfxLookup, 0x1B4C, 5, 1, 2);
    /* 1B4C call $1C9E */
    spc_call_cyc(abi_SpcSfxChannel, 0x1B4F, 5, 1, 2);
    /* 1B4F pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 1B50 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 1B51 cmp.w a,$0390+x */
    spc_cyc(5);
    spc_cmp(spc.a, spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1B54 beq $1B59 */
    if (spc.z) { spc_cyc(4); goto L1B59; }
    spc_cyc(2);
L1B56:
    /* 1B56 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 1B57 bra $1B40 */
    spc_cyc(4);
    goto L1B40;
L1B59:
    /* 1B59 mov x,#$00 */
    spc.x = spc_nz(0x00);
    spc_cyc(2);
L1B5B:
    /* 1B5B mov.w a,$26E4+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x26E4 + spc.x)));
    /* 1B5E beq $1B6F */
    if (spc.z) { spc_cyc(4); goto L1B6F; }
    spc_cyc(2);
    /* 1B60 cmp.b a,$1D */
    spc_cyc(3);
    spc_cmp(spc.a, spc_rd(spc_dp(0x1D)));
    /* 1B62 beq $1B67 */
    if (spc.z) { spc_cyc(4); goto L1B67; }
    spc_cyc(2);
    /* 1B64 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 1B65 bra $1B5B */
    spc_cyc(4);
    goto L1B5B;
L1B67:
    /* 1B67 mov.b $20,x */
    spc_st_cyc(spc_dp(0x20), spc.x, 3, 1);
    /* 1B69 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 1B6A setc */
    spc.c = 1;
    spc_cyc(2);
    /* 1B6B sbc.b a,$20 */
    spc_cyc(3);
    spc.a = spc_sbc(spc.a, spc_rd(spc_dp(0x20)));
    /* 1B6D bpl $1B56 */
    if (!spc.n) { spc_cyc(4); goto L1B56; }
    spc_cyc(2);
L1B6F:
    /* 1B6F mov $1F,$1D */
    spc_cyc(3);
    spc_wr_cyc(spc_dp(0x1F), spc_rd(spc_dp(0x1D)), 2);
    /* 1B72 mov $1D,#$00 */
    spc_st_cyc(spc_dp(0x1D), 0x00, 4, 1);
    /* 1B75 ret */
    spc_ret_cyc(4, 1);
    return;
L1B76:
    /* 1B76 mov $1F,#$00 */
    spc_st_cyc(spc_dp(0x1F), 0x00, 4, 1);
L1B79:
    /* 1B79 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D804 SpcCommandBlock
 * WHAT IT DOES: A sound command in the list at $1B8C is dropped.
 * HOW: A zero-ended list scan.
 * Entry: none. Exit: X = index.
 * @spc 1B7A
 * @spcuses 1B7A-1B8C
 */
void abi_SpcCommandBlock(void)
{
    /* 1B7A mov x,#$00 */
    spc.x = spc_nz(0x00);
    spc_cyc(2);
L1B7C:
    /* 1B7C mov.w a,$1B8C+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x1B8C + spc.x)));
    /* 1B7F beq $1B8B */
    if (spc.z) { spc_cyc(4); goto L1B8B; }
    spc_cyc(2);
    /* 1B81 cmp.b a,$1D */
    spc_cyc(3);
    spc_cmp(spc.a, spc_rd(spc_dp(0x1D)));
    /* 1B83 beq $1B88 */
    if (spc.z) { spc_cyc(4); goto L1B88; }
    spc_cyc(2);
    /* 1B85 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 1B86 bra $1B7C */
    spc_cyc(4);
    goto L1B7C;
L1B88:
    /* 1B88 mov $1D,#$00 */
    spc_st_cyc(spc_dp(0x1D), 0x00, 4, 1);
L1B8B:
    /* 1B8B ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D81C SpcSfxHolds
 * WHAT IT DOES: The channels' hold counters $0468 count up; a new command whose channel is held ($0470) is dropped, and a command in the list at $1C04 starts its channel's hold; a hold ends when its count reaches the list's time.
 * HOW: Pairs (sound, time) in the list at $1C04.
 * Entry: none. Exit: A, X, Y changed.
 * @spc 1B92
 * @spcuses 19EA-19F9,1B92-1C04
 */
void abi_SpcSfxHolds(void)
{
    /* 1B92 mov x,#$00 */
    spc.x = spc_nz(0x00);
    spc_cyc(2);
L1B94:
    /* 1B94 mov.w a,$0468+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0468 + spc.x)));
    /* 1B97 beq $1B9D */
    if (spc.z) { spc_cyc(4); goto L1B9D; }
    spc_cyc(2);
    /* 1B99 inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
    /* 1B9A mov.w $0468+x,a */
    spc_st_cyc((uint16_t)(0x0468 + spc.x), spc.a, 5, 1);
L1B9D:
    /* 1B9D inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 1B9E cmp x,#$08 */
    spc_cmp(spc.x, 0x08);
    spc_cyc(2);
    /* 1BA0 bne $1B94 */
    if (!spc.z) { spc_cyc(4); goto L1B94; }
    spc_cyc(2);
    /* 1BA2 mov.b a,$1D */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 1BA4 beq $1BD3 */
    if (spc.z) { spc_cyc(4); goto L1BD3; }
    spc_cyc(2);
    /* 1BA6 call $19EA */
    spc_call_cyc(abi_SpcSfxPointer, 0x1BA9, 5, 1, 2);
    /* 1BA9 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 1BAB mov a,($24)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x24)); p |= (spc_cyc(1), spc_rd(spc_dp(0x25))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 1BAD and a,#$07 */
    spc.a = spc_and(spc.a, 0x07);
    spc_cyc(2);
    /* 1BAF mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 1BB0 mov $26,$1D */
    spc_cyc(3);
    spc_wr_cyc(spc_dp(0x26), spc_rd(spc_dp(0x1D)), 2);
    /* 1BB3 mov.w a,$0470+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0470 + spc.x)));
    /* 1BB6 beq $1BBB */
    if (spc.z) { spc_cyc(4); goto L1BBB; }
    spc_cyc(2);
    /* 1BB8 mov $1D,#$00 */
    spc_st_cyc(spc_dp(0x1D), 0x00, 4, 1);
L1BBB:
    /* 1BBB mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
L1BBD:
    /* 1BBD mov.w a,$1C04+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x1C04 + spc.y)));
    /* 1BC0 beq $1BD2 */
    if (spc.z) { spc_cyc(4); goto L1BD2; }
    spc_cyc(2);
    /* 1BC2 cmp.b a,$26 */
    spc_cyc(3);
    spc_cmp(spc.a, spc_rd(spc_dp(0x26)));
    /* 1BC4 beq $1BCA */
    if (spc.z) { spc_cyc(4); goto L1BCA; }
    spc_cyc(2);
    /* 1BC6 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 1BC7 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 1BC8 bra $1BBD */
    spc_cyc(4);
    goto L1BBD;
L1BCA:
    /* 1BCA mov.w $0470+x,a */
    spc_st_cyc((uint16_t)(0x0470 + spc.x), spc.a, 5, 1);
    /* 1BCD mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 1BCF mov.w $0468+x,a */
    spc_st_cyc((uint16_t)(0x0468 + spc.x), spc.a, 5, 1);
L1BD2:
    /* 1BD2 ret */
    spc_ret_cyc(4, 1);
    return;
L1BD3:
    /* 1BD3 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
L1BD5:
    /* 1BD5 mov.w a,$0470+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0470 + spc.y)));
    /* 1BD8 bne $1BE0 */
    if (!spc.z) { spc_cyc(4); goto L1BE0; }
    spc_cyc(2);
L1BDA:
    /* 1BDA inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 1BDB cmp y,#$08 */
    spc_cmp(spc.y, 0x08);
    spc_cyc(2);
    /* 1BDD bne $1BD5 */
    if (!spc.z) { spc_cyc(4); goto L1BD5; }
    spc_cyc(2);
    /* 1BDF ret */
    spc_ret_cyc(4, 1);
    return;
L1BE0:
    /* 1BE0 mov.b $26,a */
    spc_st_cyc(spc_dp(0x26), spc.a, 3, 1);
    /* 1BE2 mov x,#$00 */
    spc.x = spc_nz(0x00);
    spc_cyc(2);
L1BE4:
    /* 1BE4 mov.w a,$1C04+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x1C04 + spc.x)));
    /* 1BE7 beq $1BDA */
    if (spc.z) { spc_cyc(4); goto L1BDA; }
    spc_cyc(2);
    /* 1BE9 cmp.b a,$26 */
    spc_cyc(3);
    spc_cmp(spc.a, spc_rd(spc_dp(0x26)));
    /* 1BEB beq $1BF1 */
    if (spc.z) { spc_cyc(4); goto L1BF1; }
    spc_cyc(2);
    /* 1BED inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 1BEE inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 1BEF bra $1BE4 */
    spc_cyc(4);
    goto L1BE4;
L1BF1:
    /* 1BF1 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 1BF2 mov.w a,$1C04+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x1C04 + spc.x)));
    /* 1BF5 cmp.w a,$0468+y */
    spc_cyc(5);
    spc_cmp(spc.a, spc_rd((uint16_t)(0x0468 + spc.y)));
    /* 1BF8 bne $1BDA */
    if (!spc.z) { spc_cyc(4); goto L1BDA; }
    spc_cyc(2);
    /* 1BFA mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1BFC mov.w $0468+y,a */
    spc_st_cyc((uint16_t)(0x0468 + spc.y), spc.a, 5, 1);
    /* 1BFF mov.w $0470+y,a */
    spc_st_cyc((uint16_t)(0x0470 + spc.y), spc.a, 5, 1);
    /* 1C02 bra $1BDA */
    spc_cyc(4);
    goto L1BDA;
}

/*
 * @routine 19:D89E SpcSfxFrameHoldReset
 * WHAT IT DOES: SpcSfxFrame's branch for a command still set after the queue push: the hold index $057B cleared, then SpcSfxFrame from its queue test at $1C25. The command is cleared just before the test ($1C0C), so no code reaches these bytes; a direct call is their only way in.
 * HOW: MOV, BRA; then as SpcSfxFrame.
 * Entry: none. Exit: as SpcSfxFrame.
 * @spc 1C14
 * @spcuses 09E0-09EF,0A3B-0A3C,0A58-0A6D,0BB4-0DBC,0DCC-0DF4,0DFF-0E1B,0E97-0EE4,129D-13FA,1424-1436,1908-1915,1941-1951,1A92-1AAA,1AE9-1AF6,1B15-1B23,1C14-1C1B,1C25-1DC1,1DCE-23EB,23FB-241B,2441-26D6,3FCA-4000,CA75-CAA8,EDF0-EE20
 */
void abi_SpcSfxFrameHoldReset(void)
{
    /* 1C14 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1C16 mov.w $057B,a */
    spc_st_cyc(0x057B, spc.a, 4, 1);
    /* 1C19 bra $1C25 */
    spc_cyc(4);
    goto L1C25;
L1C25:
    /* 1C25 cmp $1D,#$00 */
    spc_cyc(4);
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x00);
    spc_cyc(1);
    /* 1C28 bne $1C54 */
    if (!spc.z) { spc_cyc(4); goto L1C54; }
    spc_cyc(2);
    /* 1C2A mov.w a,$057B */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x057B));
    /* 1C2D beq $1C54 */
    if (spc.z) { spc_cyc(4); goto L1C54; }
    spc_cyc(2);
    /* 1C2F cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 1C31 beq $1C54 */
    if (spc.z) { spc_cyc(4); goto L1C54; }
    spc_cyc(2);
    /* 1C33 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    spc_cyc(2);
    /* 1C35 beq $1C54 */
    if (spc.z) { spc_cyc(4); goto L1C54; }
    spc_cyc(2);
    /* 1C37 mov.w a,$0588 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0588));
    /* 1C3A beq $1C54 */
    if (spc.z) { spc_cyc(4); goto L1C54; }
    spc_cyc(2);
    /* 1C3C push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 1C3D mov x,#$01 */
    spc.x = spc_nz(0x01);
    spc_cyc(2);
L1C3F:
    /* 1C3F mov.w a,$0588+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0588 + spc.x)));
    /* 1C42 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* 1C43 mov.w $0588+x,a */
    spc_st_cyc((uint16_t)(0x0588 + spc.x), spc.a, 5, 1);
    /* 1C46 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 1C47 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 1C48 cmp x,#$08 */
    spc_cmp(spc.x, 0x08);
    spc_cyc(2);
    /* 1C4A bne $1C3F */
    if (!spc.z) { spc_cyc(4); goto L1C3F; }
    spc_cyc(2);
    /* 1C4C pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 1C4D mov x,#$00 */
    spc.x = spc_nz(0x00);
    spc_cyc(2);
    /* 1C4F mov.w $057B,x */
    spc_st_cyc(0x057B, spc.x, 4, 1);
    /* 1C52 bra $1C63 */
    spc_cyc(4);
    goto L1C63;
L1C54:
    /* 1C54 mov.b a,$1D */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 1C56 cmp.b a,$1E */
    spc_cyc(3);
    spc_cmp(spc.a, spc_rd(spc_dp(0x1E)));
    /* 1C58 bne $1C5D */
    if (!spc.z) { spc_cyc(4); goto L1C5D; }
    spc_cyc(2);
L1C5A:
    /* 1C5A jmp $1CCD */
    { spc_cyc(3); abi_SpcSfxChannelsFrame(); return; }
L1C5D:
    /* 1C5D mov.b $1E,a */
    spc_st_cyc(spc_dp(0x1E), spc.a, 3, 1);
    /* 1C5F mov.b a,$1D */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 1C61 beq $1C5A */
    if (spc.z) { spc_cyc(4); goto L1C5A; }
    spc_cyc(2);
L1C63:
    /* 1C63 mov.b $23,a */
    spc_st_cyc(spc_dp(0x23), spc.a, 3, 1);
    /* 1C65 call $1C8A */
    spc_call_cyc(abi_SpcSfxLookup, 0x1C68, 5, 1, 2);
    /* 1C68 call $1C9E */
    spc_call_cyc(abi_SpcSfxChannel, 0x1C6B, 5, 1, 2);
    /* 1C6B call $1CA5 */
    spc_call_cyc(abi_SpcSfxMode, 0x1C6E, 5, 1, 2);
    /* 1C6E call $1C84 */
    spc_call_cyc(abi_SpcSfxActivate, 0x1C71, 5, 1, 2);
    /* 1C71 call $1CED */
    spc_call_cyc(abi_SpcSfxChannelSave, 0x1C74, 5, 1, 2);
    /* 1C74 mov.b a,$23 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x23)));
    /* 1C76 mov.w $0410+x,a */
    spc_st_cyc((uint16_t)(0x0410 + spc.x), spc.a, 5, 1);
    /* 1C79 push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 1C7A call $1CCD */
    spc_call_cyc(abi_SpcSfxChannelsFrame, 0x1C7D, 5, 1, 2);
    /* 1C7D pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 1C7E mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1C80 mov.w $0410+x,a */
    spc_st_cyc((uint16_t)(0x0410 + spc.x), spc.a, 5, 1);
    /* 1C83 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D90E SpcSfxActivate
 * WHAT IT DOES: Marks channel X active ($0408 = 1).
 * HOW: Store.
 * Entry: X = channel. Exit: A = 1.
 * @spc 1C84
 * @spcuses 1C84-1C8A
 */
void abi_SpcSfxActivate(void)
{
    /* 1C84 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 1C86 mov.w $0408+x,a */
    spc_st_cyc((uint16_t)(0x0408 + spc.x), spc.a, 5, 1);
    /* 1C89 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D914 SpcSfxLookup
 * WHAT IT DOES: The data of sound A: its pointer from $0702 into $20/$21 and its first byte (the channel and mode) in X.
 * HOW: (A - 1) * 2 as the index; MOV A,($20)+Y.
 * Entry: A = sound. Exit: X = header byte, Y = 0.
 * @spc 1C8A
 * @spcuses 1C8A-1C9E
 */
void abi_SpcSfxLookup(void)
{
    /* 1C8A dec a */
    spc.a = spc_dec(spc.a);
    spc_cyc(2);
    /* 1C8B asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 1C8C mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 1C8D mov.w a,$0702+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0702 + spc.y)));
    /* 1C90 mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 1C92 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 1C93 mov.w a,$0702+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0702 + spc.y)));
    /* 1C96 mov.b $21,a */
    spc_st_cyc(spc_dp(0x21), spc.a, 3, 1);
    /* 1C98 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 1C9A mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 1C9C mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 1C9D ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D928 SpcSfxChannel
 * WHAT IT DOES: Keeps the header byte X at $26 and leaves its channel (low four bits) in X.
 * HOW: AND #$0F.
 * Entry: X = header byte. Exit: X = channel.
 * @spc 1C9E
 * @spcuses 1C9E-1CA5
 */
void abi_SpcSfxChannel(void)
{
    /* 1C9E mov.b $26,x */
    spc_st_cyc(spc_dp(0x26), spc.x, 3, 1);
    /* 1CA0 mov a,x */
    spc.a = spc_nz(spc.x);
    spc_cyc(2);
    /* 1CA1 and a,#$0F */
    spc.a = spc_and(spc.a, 0x0F);
    spc_cyc(2);
    /* 1CA3 mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 1CA4 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D92F SpcSfxMode
 * WHAT IT DOES: The header's mode bits for channel X: bit 7 marks a looping sound ($0480 = 1, or 2 with bit 5, which also drops a queued one); otherwise a looping channel saves its sound to restart ($04F8).
 * HOW: BBC tests on $26.
 * Entry: X = channel, $26 = header. Exit: A changed.
 * @spc 1CA5
 * @spcuses 1CA5-1CCD
 */
void abi_SpcSfxMode(void)
{
    /* 1CA5 bbc $26.7,$1CBA */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x26)) >> 7 & 1)) { spc_cyc(4); goto L1CBA; }
    spc_cyc(2);
    /* 1CA8 bbc $26.5,$1CB4 */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x26)) >> 5 & 1)) { spc_cyc(4); goto L1CB4; }
    spc_cyc(2);
    /* 1CAB mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1CAD mov.w $04F8+x,a */
    spc_st_cyc((uint16_t)(0x04F8 + spc.x), spc.a, 5, 1);
    /* 1CB0 mov a,#$02 */
    spc.a = spc_nz(0x02);
    spc_cyc(2);
    /* 1CB2 bra $1CB6 */
    spc_cyc(4);
    goto L1CB6;
L1CB4:
    /* 1CB4 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
L1CB6:
    /* 1CB6 mov.w $0480+x,a */
    spc_st_cyc((uint16_t)(0x0480 + spc.x), spc.a, 5, 1);
    /* 1CB9 ret */
    spc_ret_cyc(4, 1);
    return;
L1CBA:
    /* 1CBA mov.w a,$0480+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0480 + spc.x)));
    /* 1CBD cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 1CBF bne $1CCC */
    if (!spc.z) { spc_cyc(4); goto L1CCC; }
    spc_cyc(2);
    /* 1CC1 mov.w a,$04F8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F8 + spc.x)));
    /* 1CC4 bne $1CCC */
    if (!spc.z) { spc_cyc(4); goto L1CCC; }
    spc_cyc(2);
    /* 1CC6 mov.w a,$0390+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1CC9 mov.w $04F8+x,a */
    spc_st_cyc((uint16_t)(0x04F8 + spc.x), spc.a, 5, 1);
L1CCC:
    /* 1CCC ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D957 SpcSfxChannelsFrame
 * WHAT IT DOES: Runs every active sound effect channel's tick (SpcSfxChannelLoad, SpcSfxChannelStep).
 * HOW: X from 0 to 7.
 * Entry: none. Exit: X = 8.
 * @spc 1CCD
 * @spcuses 09E0-09EF,0BC1-0C17,0E97-0EE4,1424-1436,1941-1951,1AE9-1AF6,1B15-1B23,1C84-1C8A,1C9E-1DC1,1DCE-23EB,23FB-2419,2441-26D6,CA75-CAA8,EDF0-EE20
 */
void abi_SpcSfxChannelsFrame(void)
{
    /* 1CCD mov x,#$00 */
    spc.x = spc_nz(0x00);
    spc_cyc(2);
L1CCF:
    /* 1CCF mov.w a,$0408+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 1CD2 beq $1CDA */
    if (spc.z) { spc_cyc(4); goto L1CDA; }
    spc_cyc(2);
    /* 1CD4 call $1CE2 */
    spc_call_cyc(abi_SpcSfxChannelLoad, 0x1CD7, 5, 1, 2);
    /* 1CD7 call $1CF8 */
    spc_call_cyc(abi_SpcSfxChannelStep, 0x1CDA, 5, 1, 2);
L1CDA:
    /* 1CDA inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 1CDB cmp x,#$08 */
    spc_cmp(spc.x, 0x08);
    spc_cyc(2);
    /* 1CDD beq $1CE1 */
    if (spc.z) { spc_cyc(4); goto L1CE1; }
    spc_cyc(2);
    /* 1CDF bra $1CCF */
    spc_cyc(4);
    goto L1CCF;
L1CE1:
    /* 1CE1 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D96C SpcSfxChannelLoad
 * WHAT IT DOES: Channel X's data pointer ($0488/$0490) into $20/$21.
 * HOW: Two copies.
 * Entry: X = channel. Exit: A = the pointer's high byte.
 * @spc 1CE2
 * @spcuses 1CE2-1CED
 */
void abi_SpcSfxChannelLoad(void)
{
    /* 1CE2 mov.w a,$0488+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0488 + spc.x)));
    /* 1CE5 mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 1CE7 mov.w a,$0490+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0490 + spc.x)));
    /* 1CEA mov.b $21,a */
    spc_st_cyc(spc_dp(0x21), spc.a, 3, 1);
    /* 1CEC ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D977 SpcSfxChannelSave
 * WHAT IT DOES: $20/$21 back as channel X's data pointer.
 * HOW: Two copies.
 * Entry: X = channel. Exit: A = the high byte.
 * @spc 1CED
 * @spcuses 1CED-1CF8
 */
void abi_SpcSfxChannelSave(void)
{
    /* 1CED mov.b a,$20 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 1CEF mov.w $0488+x,a */
    spc_st_cyc((uint16_t)(0x0488 + spc.x), spc.a, 5, 1);
    /* 1CF2 mov.b a,$21 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x21)));
    /* 1CF4 mov.w $0490+x,a */
    spc_st_cyc((uint16_t)(0x0490 + spc.x), spc.a, 5, 1);
    /* 1CF7 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:D982 SpcSfxChannelStep
 * WHAT IT DOES: Channel X's state ($03F8: 0 start, 1 started, 2 and 3 playing, 4 stopping): a new sound ($0410) starts with its voice taken from the music and keyed off; playing, it is keyed on, read (SpcSfxRead), its special handler run (SpcSfxSpecial) and its DSP registers written; stop requests ($03E0) by reason, the end of a non-looping sound, or a fade's end release the voice (SpcSfxRelease) and clear the channel.
 * HOW: Compare chains on the state and the request; tail jumps into the shared code.
 * Entry: X = channel, $20/$21 its pointer. Exit: A, Y changed.
 * @spc 1CF8
 * @spcuses 09E0-09EF,0BC1-0C17,0E97-0EE4,1424-1436,1941-1951,1AE9-1AF6,1B15-1B23,1C84-1C8A,1C9E-1CCD,1CE2-1DC1,1DCE-23EB,23FB-2419,2441-26D6,CA75-CAA8,EDF0-EE20
 */
void abi_SpcSfxChannelStep(void)
{
    /* 1CF8 mov.w a,$03F8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03F8 + spc.x)));
    /* 1CFB beq $1D50 */
    if (spc.z) { spc_cyc(4); goto L1D50; }
    spc_cyc(2);
    /* 1CFD cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 1CFF beq $1D1C */
    if (spc.z) { spc_cyc(4); goto L1D1C; }
    spc_cyc(2);
    /* 1D01 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    spc_cyc(2);
    /* 1D03 beq $1D31 */
    if (spc.z) { spc_cyc(4); goto L1D31; }
    spc_cyc(2);
    /* 1D05 cmp a,#$03 */
    spc_cmp(spc.a, 0x03);
    spc_cyc(2);
    /* 1D07 beq $1D36 */
    if (spc.z) { spc_cyc(4); goto L1D36; }
    spc_cyc(2);
    /* 1D09 mov.w a,$0410+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0410 + spc.x)));
    /* 1D0C bne $1D53 */
    if (!spc.z) { spc_cyc(4); goto L1D53; }
    spc_cyc(2);
    /* 1D0E mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1D10 mov.w $0390+x,a */
    spc_st_cyc((uint16_t)(0x0390 + spc.x), spc.a, 5, 1);
    /* 1D13 mov.w $03F8+x,a */
    spc_st_cyc((uint16_t)(0x03F8 + spc.x), spc.a, 5, 1);
    /* 1D16 mov.w $0408+x,a */
    spc_st_cyc((uint16_t)(0x0408 + spc.x), spc.a, 5, 1);
    /* 1D19 jmp $1E31 */
    { spc_cyc(3); abi_SpcSfxRelease(); return; }
L1D1C:
    /* 1D1C mov.w a,$0410+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0410 + spc.x)));
    /* 1D1F bne $1D53 */
    if (!spc.z) { spc_cyc(4); goto L1D53; }
    spc_cyc(2);
    /* 1D21 mov a,#$02 */
    spc.a = spc_nz(0x02);
    spc_cyc(2);
    /* 1D23 mov.w $03F8+x,a */
    spc_st_cyc((uint16_t)(0x03F8 + spc.x), spc.a, 5, 1);
    /* 1D26 call $1E89 */
    spc_call_cyc(abi_SpcKeyOnVoice, 0x1D29, 5, 1, 2);
    /* 1D29 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1D2B mov.w $03E8+x,a */
    spc_st_cyc((uint16_t)(0x03E8 + spc.x), spc.a, 5, 1);
L1D2E:
    /* 1D2E jmp $1E9D */
    spc_cyc(3);
    goto L1E9D;
L1D31:
    /* 1D31 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1D33 mov.w $03E8+x,a */
    spc_st_cyc((uint16_t)(0x03E8 + spc.x), spc.a, 5, 1);
L1D36:
    /* 1D36 mov.w a,$0410+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0410 + spc.x)));
    /* 1D39 bne $1D53 */
    if (!spc.z) { spc_cyc(4); goto L1D53; }
    spc_cyc(2);
    /* 1D3B mov a,#$03 */
    spc.a = spc_nz(0x03);
    spc_cyc(2);
    /* 1D3D mov.w $03F8+x,a */
    spc_st_cyc((uint16_t)(0x03F8 + spc.x), spc.a, 5, 1);
    /* 1D40 mov.w a,$03E8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03E8 + spc.x)));
    /* 1D43 beq $1D6C */
    if (spc.z) { spc_cyc(4); goto L1D6C; }
    spc_cyc(2);
    /* 1D45 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 1D47 beq $1D1C */
    if (spc.z) { spc_cyc(4); goto L1D1C; }
    spc_cyc(2);
    /* 1D49 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1D4B mov.w $03F0+x,a */
    spc_st_cyc((uint16_t)(0x03F0 + spc.x), spc.a, 5, 1);
    /* 1D4E bra $1D1C */
    spc_cyc(4);
    goto L1D1C;
L1D50:
    /* 1D50 mov.w a,$0410+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0410 + spc.x)));
L1D53:
    /* 1D53 mov.w $0390+x,a */
    spc_st_cyc((uint16_t)(0x0390 + spc.x), spc.a, 5, 1);
    /* 1D56 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 1D58 mov.w $03F8+x,a */
    spc_st_cyc((uint16_t)(0x03F8 + spc.x), spc.a, 5, 1);
    /* 1D5B mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1D5D mov.w $03F0+x,a */
    spc_st_cyc((uint16_t)(0x03F0 + spc.x), spc.a, 5, 1);
    /* 1D60 mov.w $03E0+x,a */
    spc_st_cyc((uint16_t)(0x03E0 + spc.x), spc.a, 5, 1);
    { abi_SpcSfxTakeVoice(); return; }
L1D69:
    /* 1D69 jmp $1DE8 */
    spc_cyc(3);
    goto L1DE8;
L1D6C:
    /* 1D6C mov.w a,$03E0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03E0 + spc.x)));
    /* 1D6F beq $1D2E */
    if (spc.z) { spc_cyc(4); goto L1D2E; }
    spc_cyc(2);
    /* 1D71 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 1D73 beq $1D69 */
    if (spc.z) { spc_cyc(4); goto L1D69; }
    spc_cyc(2);
    /* 1D75 cmp a,#$07 */
    spc_cmp(spc.a, 0x07);
    spc_cyc(2);
    /* 1D77 beq $1DCE */
    if (spc.z) { spc_cyc(4); goto L1DCE; }
    spc_cyc(2);
    /* 1D79 cmp a,#$08 */
    spc_cmp(spc.a, 0x08);
    spc_cyc(2);
    /* 1D7B beq $1D94 */
    if (spc.z) { spc_cyc(4); goto L1D94; }
    spc_cyc(2);
    /* 1D7D cmp a,#$0A */
    spc_cmp(spc.a, 0x0A);
    spc_cyc(2);
    /* 1D7F beq $1D9D */
    if (spc.z) { spc_cyc(4); goto L1D9D; }
    spc_cyc(2);
    /* 1D81 cmp a,#$0B */
    spc_cmp(spc.a, 0x0B);
    spc_cyc(2);
    /* 1D83 beq $1DA6 */
    if (spc.z) { spc_cyc(4); goto L1DA6; }
    spc_cyc(2);
    /* 1D85 cmp a,#$0C */
    spc_cmp(spc.a, 0x0C);
    spc_cyc(2);
    /* 1D87 beq $1DAF */
    if (spc.z) { spc_cyc(4); goto L1DAF; }
    spc_cyc(2);
    /* 1D89 cmp a,#$0E */
    spc_cmp(spc.a, 0x0E);
    spc_cyc(2);
    /* 1D8B beq $1DB8 */
    if (spc.z) { spc_cyc(4); goto L1DB8; }
    spc_cyc(2);
L1D8D:
    /* 1D8D mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1D8F mov.w $03E0+x,a */
    spc_st_cyc((uint16_t)(0x03E0 + spc.x), spc.a, 5, 1);
    /* 1D92 bra $1D2E */
    spc_cyc(4);
    goto L1D2E;
L1D94:
    /* 1D94 mov.w a,$0390+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1D97 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    spc_cyc(2);
    /* 1D99 beq $1DCE */
    if (spc.z) { spc_cyc(4); goto L1DCE; }
    spc_cyc(2);
    /* 1D9B bra $1D8D */
    spc_cyc(4);
    goto L1D8D;
L1D9D:
    /* 1D9D mov.w a,$0390+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1DA0 cmp a,#$25 */
    spc_cmp(spc.a, 0x25);
    spc_cyc(2);
    /* 1DA2 beq $1DCE */
    if (spc.z) { spc_cyc(4); goto L1DCE; }
    spc_cyc(2);
    /* 1DA4 bra $1D8D */
    spc_cyc(4);
    goto L1D8D;
L1DA6:
    /* 1DA6 mov.w a,$0390+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1DA9 cmp a,#$15 */
    spc_cmp(spc.a, 0x15);
    spc_cyc(2);
    /* 1DAB beq $1DCE */
    if (spc.z) { spc_cyc(4); goto L1DCE; }
    spc_cyc(2);
    /* 1DAD bra $1D8D */
    spc_cyc(4);
    goto L1D8D;
L1DAF:
    /* 1DAF mov.w a,$0390+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1DB2 cmp a,#$17 */
    spc_cmp(spc.a, 0x17);
    spc_cyc(2);
    /* 1DB4 beq $1DCE */
    if (spc.z) { spc_cyc(4); goto L1DCE; }
    spc_cyc(2);
    /* 1DB6 bra $1D8D */
    spc_cyc(4);
    goto L1D8D;
L1DB8:
    /* 1DB8 mov.w a,$0390+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1DBB cmp a,#$21 */
    spc_cmp(spc.a, 0x21);
    spc_cyc(2);
    /* 1DBD beq $1DCE */
    if (spc.z) { spc_cyc(4); goto L1DCE; }
    spc_cyc(2);
    /* 1DBF bra $1D8D */
    spc_cyc(4);
    goto L1D8D;
L1DCE:
    /* 1DCE mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1DD0 mov.w $03E0+x,a */
    spc_st_cyc((uint16_t)(0x03E0 + spc.x), spc.a, 5, 1);
    /* 1DD3 mov.w a,$04F8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F8 + spc.x)));
    /* 1DD6 beq $1DE0 */
    if (spc.z) { spc_cyc(4); goto L1DE0; }
    spc_cyc(2);
    /* 1DD8 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1DDA mov.w $04F8+x,a */
    spc_st_cyc((uint16_t)(0x04F8 + spc.x), spc.a, 5, 1);
    /* 1DDD jmp $1D2E */
    spc_cyc(3);
    goto L1D2E;
L1DE0:
    /* 1DE0 mov.w a,$0480+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0480 + spc.x)));
    /* 1DE3 bne $1DF5 */
    if (!spc.z) { spc_cyc(4); goto L1DF5; }
    spc_cyc(2);
    /* 1DE5 jmp $1D2E */
    spc_cyc(3);
    goto L1D2E;
L1DE8:
    /* 1DE8 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1DEA mov.w $03E0+x,a */
    spc_st_cyc((uint16_t)(0x03E0 + spc.x), spc.a, 5, 1);
    /* 1DED mov.w a,$04F8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F8 + spc.x)));
    /* 1DF0 beq $1DF5 */
    if (spc.z) { spc_cyc(4); goto L1DF5; }
    spc_cyc(2);
    /* 1DF2 call $1424 */
    spc_call_cyc(abi_SpcSoundQueuePush, 0x1DF5, 5, 1, 2);
L1DF5:
    /* 1DF5 mov a,#$04 */
    spc.a = spc_nz(0x04);
    spc_cyc(2);
    /* 1DF7 mov.w $03F8+x,a */
    spc_st_cyc((uint16_t)(0x03F8 + spc.x), spc.a, 5, 1);
    /* 1DFA call $1E93 */
    spc_call_cyc(abi_SpcKeyOffVoice, 0x1DFD, 5, 1, 2);
    /* 1DFD mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1DFF mov.w $0480+x,a */
    spc_st_cyc((uint16_t)(0x0480 + spc.x), spc.a, 5, 1);
    /* 1E02 mov.w $04F8+x,a */
    spc_st_cyc((uint16_t)(0x04F8 + spc.x), spc.a, 5, 1);
    /* 1E05 mov.w $03F0+x,a */
    spc_st_cyc((uint16_t)(0x03F0 + spc.x), spc.a, 5, 1);
    /* 1E08 mov.w $03C8+x,a */
    spc_st_cyc((uint16_t)(0x03C8 + spc.x), spc.a, 5, 1);
    /* 1E0B mov.w $04F0+x,a */
    spc_st_cyc((uint16_t)(0x04F0 + spc.x), spc.a, 5, 1);
    /* 1E0E ret */
    spc_ret_cyc(4, 1);
    return;
L1E9D:
    /* 1E9D call $1EA6 */
    spc_call_cyc(abi_SpcSfxRead, 0x1EA0, 5, 1, 2);
    /* 1EA0 call $23CF */
    spc_call_cyc(abi_SpcSfxSpecial, 0x1EA3, 5, 1, 2);
    /* 1EA3 jmp $22D3 */
    spc_cyc(3);
    goto L22D3;
L22D3:
    /* 22D3 mov a,x */
    spc.a = spc_nz(spc.x);
    spc_cyc(2);
    /* 22D4 mov y,#$10 */
    spc.y = spc_nz(0x10);
    spc_cyc(2);
    /* 22D6 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 22D7 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 22D8 push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 22D9 mov.w a,$0408+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 22DC beq $22E8 */
    if (spc.z) { spc_cyc(4); goto L22E8; }
    spc_cyc(2);
    /* 22DE mov.w a,$04C0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04C0 + spc.x)));
    /* 22E1 beq $22E8 */
    if (spc.z) { spc_cyc(4); goto L22E8; }
    spc_cyc(2);
    /* 22E3 call $2336 */
    spc_call_cyc(abi_SpcSfxFadeVolume, 0x22E6, 5, 1, 2);
    /* 22E6 bra $22EB */
    spc_cyc(4);
    goto L22EB;
L22E8:
    /* 22E8 mov.w a,$0398+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0398 + spc.x)));
L22EB:
    /* 22EB call $23B2 */
    spc_call_cyc(abi_SpcSfxVolumeAdjust, 0x22EE, 5, 1, 2);
    /* 22EE pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 22EF call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x22F2, 5, 1, 2);
    /* 22F2 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 22F3 push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 22F4 mov.w a,$0408+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 22F7 beq $2309 */
    if (spc.z) { spc_cyc(4); goto L2309; }
    spc_cyc(2);
    /* 22F9 mov.w a,$04C8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04C8 + spc.x)));
    /* 22FC beq $2309 */
    if (spc.z) { spc_cyc(4); goto L2309; }
    spc_cyc(2);
    /* 22FE call $CA96 */
    spc_call_cyc(abi_SpcIndexRight, 0x2301, 5, 1, 2);
    /* 2301 call $2336 */
    spc_call_cyc(abi_SpcSfxFadeVolume, 0x2304, 5, 1, 2);
    /* 2304 call $CA9F */
    spc_call_cyc(abi_SpcIndexLeft, 0x2307, 5, 1, 2);
    /* 2307 bra $230C */
    spc_cyc(4);
    goto L230C;
L2309:
    /* 2309 mov.w a,$03A0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03A0 + spc.x)));
L230C:
    /* 230C call $23B2 */
    spc_call_cyc(abi_SpcSfxVolumeAdjust, 0x230F, 5, 1, 2);
    /* 230F pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 2310 call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x2313, 5, 1, 2);
    /* 2313 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 2314 mov.w a,$03B8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03B8 + spc.x)));
    /* 2317 call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x231A, 5, 1, 2);
    /* 231A inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 231B mov.w a,$03C0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03C0 + spc.x)));
    /* 231E call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x2321, 5, 1, 2);
    /* 2321 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 2322 mov.w a,$03C8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03C8 + spc.x)));
    /* 2325 call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x2328, 5, 1, 2);
    /* 2328 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 2329 mov.w a,$03D0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03D0 + spc.x)));
    /* 232C call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x232F, 5, 1, 2);
    /* 232F inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 2330 mov.w a,$03D8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03D8 + spc.x)));
    /* 2333 jmp $09E8 */
    { spc_cyc(3); abi_SpcDspWrite(); return; }
}

/*
 * @routine 19:D9ED SpcSfxTakeVoice
 * WHAT IT DOES: Takes voice X from the music (SpcSfxMarkVoice) and keys it off.
 * HOW: Two CALLs.
 * Entry: X = channel. Exit: as SpcKeyOffVoice.
 * @spc 1D63
 * @spcuses 1D63-1D69,1E0F-1E28,1E93-1E9D
 */
void abi_SpcSfxTakeVoice(void)
{
    /* 1D63 call $1E0F */
    spc_call_cyc(abi_SpcSfxMarkVoice, 0x1D66, 5, 1, 2);
    /* 1D66 jmp $1E93 */
    { spc_cyc(3); abi_SpcKeyOffVoice(); return; }
}

/*
 * @routine 19:DA4B SpcSfxStopMatchOld
 * WHAT IT DOES: A stop-request match for the commands $05 and $08 that no case of SpcSfxChannelStep's stop check dispatches to: channel X's command $0390 of 5 or 8 takes the stop ($1DE8), any other refuses it ($1D8D); then as SpcSfxChannelStep. It follows an unconditional BRA and nothing jumps to it; a direct call is its only way in.
 * HOW: CMP, BEQ; then as SpcSfxChannelStep.
 * Entry: X = channel. Exit: as SpcSfxChannelStep.
 * @spc 1DC1
 * @spcuses 09E0-09EF,0A3B-0A3C,0A58-0A6D,0BB4-0DBC,0DCC-0DF4,0DFF-0E1B,0E97-0EE4,129D-13FA,1424-1436,1908-1915,1941-1951,1A92-1AAA,1AE9-1AF6,1B15-1B23,1C84-1C8A,1C9E-1CCD,1CE2-1CF8,1D2E-1D31,1D53-1D69,1D8D-1D94,1DC1-1DCE,1DE8-1E89,1E93-23EB,23FB-241B,2441-26D6,3FCA-4000,CA75-CAA8,EDF0-EE20
 */
void abi_SpcSfxStopMatchOld(void)
{
    /* 1DC1 mov.w a,$0390+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1DC4 cmp a,#$05 */
    spc_cmp(spc.a, 0x05);
    spc_cyc(2);
    /* 1DC6 beq $1DE8 */
    if (spc.z) { spc_cyc(4); goto L1DE8; }
    spc_cyc(2);
    /* 1DC8 cmp a,#$08 */
    spc_cmp(spc.a, 0x08);
    spc_cyc(2);
    /* 1DCA beq $1DE8 */
    if (spc.z) { spc_cyc(4); goto L1DE8; }
    spc_cyc(2);
    /* 1DCC bra $1D8D */
    spc_cyc(4);
    goto L1D8D;
L1DE8:
    /* 1DE8 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1DEA mov.w $03E0+x,a */
    spc_st_cyc((uint16_t)(0x03E0 + spc.x), spc.a, 5, 1);
    /* 1DED mov.w a,$04F8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F8 + spc.x)));
    /* 1DF0 beq $1DF5 */
    if (spc.z) { spc_cyc(4); goto L1DF5; }
    spc_cyc(2);
    /* 1DF2 call $1424 */
    spc_call_cyc(abi_SpcSoundQueuePush, 0x1DF5, 5, 1, 2);
L1DF5:
    /* 1DF5 mov a,#$04 */
    spc.a = spc_nz(0x04);
    spc_cyc(2);
    /* 1DF7 mov.w $03F8+x,a */
    spc_st_cyc((uint16_t)(0x03F8 + spc.x), spc.a, 5, 1);
    /* 1DFA call $1E93 */
    spc_call_cyc(abi_SpcKeyOffVoice, 0x1DFD, 5, 1, 2);
    /* 1DFD mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1DFF mov.w $0480+x,a */
    spc_st_cyc((uint16_t)(0x0480 + spc.x), spc.a, 5, 1);
    /* 1E02 mov.w $04F8+x,a */
    spc_st_cyc((uint16_t)(0x04F8 + spc.x), spc.a, 5, 1);
    /* 1E05 mov.w $03F0+x,a */
    spc_st_cyc((uint16_t)(0x03F0 + spc.x), spc.a, 5, 1);
    /* 1E08 mov.w $03C8+x,a */
    spc_st_cyc((uint16_t)(0x03C8 + spc.x), spc.a, 5, 1);
    /* 1E0B mov.w $04F0+x,a */
    spc_st_cyc((uint16_t)(0x04F0 + spc.x), spc.a, 5, 1);
    /* 1E0E ret */
    spc_ret_cyc(4, 1);
    return;
L1E9D:
    /* 1E9D call $1EA6 */
    spc_call_cyc(abi_SpcSfxRead, 0x1EA0, 5, 1, 2);
    /* 1EA0 call $23CF */
    spc_call_cyc(abi_SpcSfxSpecial, 0x1EA3, 5, 1, 2);
    /* 1EA3 jmp $22D3 */
    spc_cyc(3);
    goto L22D3;
L22D3:
    /* 22D3 mov a,x */
    spc.a = spc_nz(spc.x);
    spc_cyc(2);
    /* 22D4 mov y,#$10 */
    spc.y = spc_nz(0x10);
    spc_cyc(2);
    /* 22D6 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 22D7 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 22D8 push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 22D9 mov.w a,$0408+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 22DC beq $22E8 */
    if (spc.z) { spc_cyc(4); goto L22E8; }
    spc_cyc(2);
    /* 22DE mov.w a,$04C0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04C0 + spc.x)));
    /* 22E1 beq $22E8 */
    if (spc.z) { spc_cyc(4); goto L22E8; }
    spc_cyc(2);
    /* 22E3 call $2336 */
    spc_call_cyc(abi_SpcSfxFadeVolume, 0x22E6, 5, 1, 2);
    /* 22E6 bra $22EB */
    spc_cyc(4);
    goto L22EB;
L22E8:
    /* 22E8 mov.w a,$0398+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0398 + spc.x)));
L22EB:
    /* 22EB call $23B2 */
    spc_call_cyc(abi_SpcSfxVolumeAdjust, 0x22EE, 5, 1, 2);
    /* 22EE pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 22EF call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x22F2, 5, 1, 2);
    /* 22F2 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 22F3 push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 22F4 mov.w a,$0408+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 22F7 beq $2309 */
    if (spc.z) { spc_cyc(4); goto L2309; }
    spc_cyc(2);
    /* 22F9 mov.w a,$04C8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04C8 + spc.x)));
    /* 22FC beq $2309 */
    if (spc.z) { spc_cyc(4); goto L2309; }
    spc_cyc(2);
    /* 22FE call $CA96 */
    spc_call_cyc(abi_SpcIndexRight, 0x2301, 5, 1, 2);
    /* 2301 call $2336 */
    spc_call_cyc(abi_SpcSfxFadeVolume, 0x2304, 5, 1, 2);
    /* 2304 call $CA9F */
    spc_call_cyc(abi_SpcIndexLeft, 0x2307, 5, 1, 2);
    /* 2307 bra $230C */
    spc_cyc(4);
    goto L230C;
L2309:
    /* 2309 mov.w a,$03A0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03A0 + spc.x)));
L230C:
    /* 230C call $23B2 */
    spc_call_cyc(abi_SpcSfxVolumeAdjust, 0x230F, 5, 1, 2);
    /* 230F pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 2310 call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x2313, 5, 1, 2);
    /* 2313 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 2314 mov.w a,$03B8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03B8 + spc.x)));
    /* 2317 call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x231A, 5, 1, 2);
    /* 231A inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 231B mov.w a,$03C0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03C0 + spc.x)));
    /* 231E call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x2321, 5, 1, 2);
    /* 2321 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 2322 mov.w a,$03C8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03C8 + spc.x)));
    /* 2325 call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x2328, 5, 1, 2);
    /* 2328 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 2329 mov.w a,$03D0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03D0 + spc.x)));
    /* 232C call $09E8 */
    spc_call_cyc(abi_SpcDspWrite, 0x232F, 5, 1, 2);
    /* 232F inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 2330 mov.w a,$03D8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03D8 + spc.x)));
    /* 2333 jmp $09E8 */
    { spc_cyc(3); abi_SpcDspWrite(); return; }
L1D2E:
    /* 1D2E jmp $1E9D */
    spc_cyc(3);
    goto L1E9D;
L1D8D:
    /* 1D8D mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1D8F mov.w $03E0+x,a */
    spc_st_cyc((uint16_t)(0x03E0 + spc.x), spc.a, 5, 1);
    /* 1D92 bra $1D2E */
    spc_cyc(4);
    goto L1D2E;
}

/*
 * @routine 19:DA99 SpcSfxMarkVoice
 * WHAT IT DOES: Marks voice X as a sound effect's ($1A).
 * HOW: SpcVoiceMask, OR.
 * Entry: X = voice. Exit: A = $1A.
 * @spc 1E0F
 * @spcuses 1E0F-1E28
 */
void abi_SpcSfxMarkVoice(void)
{
    /* 1E0F call $1E19 */
    spc_call_cyc(abi_SpcVoiceMask, 0x1E12, 5, 1, 2);
    /* 1E12 mov.b a,$1A */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1A)));
    /* 1E14 or.b a,$22 */
    spc_cyc(3);
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x22)));
    /* 1E16 mov.b $1A,a */
    spc_st_cyc(spc_dp(0x1A), spc.a, 3, 1);
    /* 1E18 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:DAA3 SpcVoiceMask
 * WHAT IT DOES: $22 = the bit of voice X (1 << X).
 * HOW: ROL of a carry X + 1 times.
 * Entry: X = voice. Exit: A = $22.
 * @spc 1E19
 * @spcuses 1E19-1E28
 */
void abi_SpcVoiceMask(void)
{
    /* 1E19 mov.b $22,x */
    spc_st_cyc(spc_dp(0x22), spc.x, 3, 1);
    /* 1E1B inc.b $22 */
    spc_rmw_cyc(spc_dp(0x22), spc_inc, 3, 1);
    /* 1E1D mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1E1F setc */
    spc.c = 1;
    spc_cyc(2);
L1E20:
    /* 1E20 rol a */
    spc.a = spc_rol(spc.a);
    spc_cyc(2);
    /* 1E21 dec.b $22 */
    spc_rmw_cyc(spc_dp(0x22), spc_dec, 3, 1);
    /* 1E23 bne $1E20 */
    if (!spc.z) { spc_cyc(4); goto L1E20; }
    spc_cyc(2);
    /* 1E25 mov.b $22,a */
    spc_st_cyc(spc_dp(0x22), spc.a, 3, 1);
    /* 1E27 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:DAB2 SpcVoiceMaskNot
 * WHAT IT DOES: $22 = every bit but voice X's.
 * HOW: ROL of $FF with a clear carry.
 * Entry: X = voice. Exit: A = $22.
 * @spc 1E28
 * @spcuses 1E20-1E31
 */
void abi_SpcVoiceMaskNot(void)
{
    /* 1E28 mov.b $22,x */
    spc_st_cyc(spc_dp(0x22), spc.x, 3, 1);
    /* 1E2A inc.b $22 */
    spc_rmw_cyc(spc_dp(0x22), spc_inc, 3, 1);
    /* 1E2C mov a,#$FF */
    spc.a = spc_nz(0xFF);
    spc_cyc(2);
    /* 1E2E clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 1E2F bra $1E20 */
    spc_cyc(4);
    goto L1E20;
L1E20:
    /* 1E20 rol a */
    spc.a = spc_rol(spc.a);
    spc_cyc(2);
    /* 1E21 dec.b $22 */
    spc_rmw_cyc(spc_dp(0x22), spc_dec, 3, 1);
    /* 1E23 bne $1E20 */
    if (!spc.z) { spc_cyc(4); goto L1E20; }
    spc_cyc(2);
    /* 1E25 mov.b $22,a */
    spc_st_cyc(spc_dp(0x22), spc.a, 3, 1);
    /* 1E27 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:DABB SpcSfxRelease
 * WHAT IT DOES: Gives voice X back to the music: its sound effect bit cleared, the music instrument reloaded (SpcLoadInstrument), its echo bit from the music's echo voices $4F, and its volume rewritten (SpcVoiceVolumeWrite).
 * HOW: Masks through $22; X doubled for the music's arrays.
 * Entry: X = voice. Exit: X kept.
 * @spc 1E31
 * @spcuses 09E0-09EF,0BC1-0C17,0E97-0EE4,1E19-1E89
 */
void abi_SpcSfxRelease(void)
{
    /* 1E31 call $1E28 */
    spc_call_cyc(abi_SpcVoiceMaskNot, 0x1E34, 5, 1, 2);
    /* 1E34 mov.b a,$1A */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1A)));
    /* 1E36 and.b a,$22 */
    spc_cyc(3);
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0x22)));
    /* 1E38 mov.b $1A,a */
    spc_st_cyc(spc_dp(0x1A), spc.a, 3, 1);
    /* 1E3A push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 1E3B clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 1E3C mov a,x */
    spc.a = spc_nz(spc.x);
    spc_cyc(2);
    /* 1E3D asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 1E3E mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 1E3F mov.w a,$0211+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0211 + spc.x)));
    /* 1E42 call $0BC1 */
    spc_call_cyc(abi_SpcLoadInstrument, 0x1E45, 5, 1, 2);
    /* 1E45 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 1E46 mov.b $20,x */
    spc_st_cyc(spc_dp(0x20), spc.x, 3, 1);
    /* 1E48 inc.b $20 */
    spc_rmw_cyc(spc_dp(0x20), spc_inc, 3, 1);
    /* 1E4A mov.b a,$4F */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x4F)));
L1E4C:
    /* 1E4C lsr a */
    spc.a = spc_lsr(spc.a);
    spc_cyc(2);
    /* 1E4D dec.b $20 */
    spc_rmw_cyc(spc_dp(0x20), spc_dec, 3, 1);
    /* 1E4F bne $1E4C */
    if (!spc.z) { spc_cyc(4); goto L1E4C; }
    spc_cyc(2);
    /* 1E51 bcc $1E5E */
    if (!spc.c) { spc_cyc(4); goto L1E5E; }
    spc_cyc(2);
    /* 1E53 call $1E19 */
    spc_call_cyc(abi_SpcVoiceMask, 0x1E56, 5, 1, 2);
    /* 1E56 mov.b a,$4A */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x4A)));
    /* 1E58 or.b a,$22 */
    spc_cyc(3);
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x22)));
    /* 1E5A mov.b $4A,a */
    spc_st_cyc(spc_dp(0x4A), spc.a, 3, 1);
    /* 1E5C bra $1E67 */
    spc_cyc(4);
    goto L1E67;
L1E5E:
    /* 1E5E call $1E28 */
    spc_call_cyc(abi_SpcVoiceMaskNot, 0x1E61, 5, 1, 2);
    /* 1E61 mov.b a,$4A */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x4A)));
    /* 1E63 and.b a,$22 */
    spc_cyc(3);
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0x22)));
    /* 1E65 mov.b $4A,a */
    spc_st_cyc(spc_dp(0x4A), spc.a, 3, 1);
L1E67:
    /* 1E67 call $1E28 */
    spc_call_cyc(abi_SpcVoiceMaskNot, 0x1E6A, 5, 1, 2);
    /* 1E6A mov.b a,$1A */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x1A)));
    /* 1E6C and.b a,$22 */
    spc_cyc(3);
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0x22)));
    /* 1E6E mov.b $1A,a */
    spc_st_cyc(spc_dp(0x1A), spc.a, 3, 1);
    /* 1E70 call $1E19 */
    spc_call_cyc(abi_SpcVoiceMask, 0x1E73, 5, 1, 2);
    /* 1E73 mov.b a,$47 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x47)));
    /* 1E75 or.b a,$22 */
    spc_cyc(3);
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x22)));
    /* 1E77 mov.b $47,a */
    spc_st_cyc(spc_dp(0x47), spc.a, 3, 1);
    /* 1E79 mov.b a,$5E */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x5E)));
    /* 1E7B or.b a,$20 */
    spc_cyc(3);
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x20)));
    /* 1E7D mov.b $5E,a */
    spc_st_cyc(spc_dp(0x5E), spc.a, 3, 1);
    /* 1E7F push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 1E80 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 1E81 mov a,x */
    spc.a = spc_nz(spc.x);
    spc_cyc(2);
    /* 1E82 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 1E83 mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 1E84 call $0E97 */
    spc_call_cyc(abi_SpcVoiceVolumeWrite, 0x1E87, 5, 1, 2);
    /* 1E87 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 1E88 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:DB13 SpcKeyOnVoice
 * WHAT IT DOES: Marks voice X for key-on ($45).
 * HOW: SpcVoiceMask, OR.
 * Entry: X = voice. Exit: A = $45.
 * @spc 1E89
 * @spcuses 1E19-1E28,1E89-1E93
 */
void abi_SpcKeyOnVoice(void)
{
    /* 1E89 call $1E19 */
    spc_call_cyc(abi_SpcVoiceMask, 0x1E8C, 5, 1, 2);
    /* 1E8C mov.b a,$45 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x45)));
    /* 1E8E or.b a,$22 */
    spc_cyc(3);
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x22)));
    /* 1E90 mov.b $45,a */
    spc_st_cyc(spc_dp(0x45), spc.a, 3, 1);
    /* 1E92 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:DB1D SpcKeyOffVoice
 * WHAT IT DOES: Marks voice X for key-off ($46).
 * HOW: SpcVoiceMask, OR.
 * Entry: X = voice. Exit: A = $46.
 * @spc 1E93
 * @spcuses 1E19-1E28,1E93-1E9D
 */
void abi_SpcKeyOffVoice(void)
{
    /* 1E93 call $1E19 */
    spc_call_cyc(abi_SpcVoiceMask, 0x1E96, 5, 1, 2);
    /* 1E96 mov.b a,$46 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x46)));
    /* 1E98 or.b a,$22 */
    spc_cyc(3);
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x22)));
    /* 1E9A mov.b $46,a */
    spc_st_cyc(spc_dp(0x46), spc.a, 3, 1);
    /* 1E9C ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:DB30 SpcSfxRead
 * WHAT IT DOES: Channel X's sound data for this tick: its age $03F0 counts (a set lifetime $0448 stops it); on the first tick its pitch, volume, slide and envelope state is reset (ADSR $8F/$E0, echo off); while a note's length $0418 runs, the slides step (SpcSfxPitchSlide, volume slides); then bytes are read from ($20)+Y: note numbers set the pitch (table $CAA8, by octave), $00-$7F lengths, and commands $E0 and up run their handler from the table at $1F59 (volume, pan, pitch and volume slides, echo on/off, ADSR, sample, lifetime, jumps, end).
 * HOW: JMP ($1F59+X) with X pushed; handlers return into the read loop.
 * Entry: X = channel. Exit: as the last handler leaves it.
 * @spc 1EA6
 * @spcuses 1941-1951,1C84-1C8A,1C9E-1CCD,1CE2-1CF8,1D53-1D69,1E0F-1E31,1E93-1E9D,1EA6-22D3,CA75-CA96,EDF0-EE20
 */
void abi_SpcSfxRead(void)
{
    /* 1EA6 mov.w a,$0390+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1EA9 beq $1EC9 */
    if (spc.z) { spc_cyc(4); goto L1EC9; }
    spc_cyc(2);
    /* 1EAB mov.w a,$0448+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0448 + spc.x)));
    /* 1EAE beq $1EBC */
    if (spc.z) { spc_cyc(4); goto L1EBC; }
    spc_cyc(2);
    /* 1EB0 mov.w a,$03F0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03F0 + spc.x)));
    /* 1EB3 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 1EB4 sbc.w a,$0448+x */
    spc_cyc(5);
    spc.a = spc_sbc(spc.a, spc_rd((uint16_t)(0x0448 + spc.x)));
    /* 1EB7 bmi $1EBC */
    if (spc.n) { spc_cyc(4); goto L1EBC; }
    spc_cyc(2);
    /* 1EB9 call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x1EBC, 5, 1, 2);
L1EBC:
    /* 1EBC mov.w a,$03F0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03F0 + spc.x)));
    /* 1EBF cmp a,#$FF */
    spc_cmp(spc.a, 0xFF);
    spc_cyc(2);
    /* 1EC1 beq $1EC7 */
    if (spc.z) { spc_cyc(4); goto L1EC7; }
    spc_cyc(2);
    /* 1EC3 inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
    /* 1EC4 mov.w $03F0+x,a */
    spc_st_cyc((uint16_t)(0x03F0 + spc.x), spc.a, 5, 1);
L1EC7:
    /* 1EC7 bra $1ECF */
    spc_cyc(4);
    goto L1ECF;
L1EC9:
    /* 1EC9 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1ECB mov.w $03F0+x,a */
    spc_st_cyc((uint16_t)(0x03F0 + spc.x), spc.a, 5, 1);
    /* 1ECE ret */
    spc_ret_cyc(4, 1);
    return;
L1ECF:
    /* 1ECF call $1CE2 */
    spc_call_cyc(abi_SpcSfxChannelLoad, 0x1ED2, 5, 1, 2);
    /* 1ED2 mov.w a,$03F0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03F0 + spc.x)));
    /* 1ED5 beq $1EDD */
    if (spc.z) { spc_cyc(4); goto L1EDD; }
    spc_cyc(2);
    /* 1ED7 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 1ED9 beq $1EDE */
    if (spc.z) { spc_cyc(4); goto L1EDE; }
    spc_cyc(2);
    /* 1EDB bra $1F30 */
    spc_cyc(4);
    goto L1F30;
L1EDD:
    /* 1EDD ret */
    spc_ret_cyc(4, 1);
    return;
L1EDE:
    /* 1EDE mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1EE0 mov.w $0400+x,a */
    spc_st_cyc((uint16_t)(0x0400 + spc.x), spc.a, 5, 1);
    /* 1EE3 mov.w $0438+x,a */
    spc_st_cyc((uint16_t)(0x0438 + spc.x), spc.a, 5, 1);
    /* 1EE6 mov.w $0520+x,a */
    spc_st_cyc((uint16_t)(0x0520 + spc.x), spc.a, 5, 1);
    /* 1EE9 mov.w $0478+x,a */
    spc_st_cyc((uint16_t)(0x0478 + spc.x), spc.a, 5, 1);
    /* 1EEC mov.w $0448+x,a */
    spc_st_cyc((uint16_t)(0x0448 + spc.x), spc.a, 5, 1);
    /* 1EEF mov.w $0440+x,a */
    spc_st_cyc((uint16_t)(0x0440 + spc.x), spc.a, 5, 1);
    /* 1EF2 mov.w $03A8+x,a */
    spc_st_cyc((uint16_t)(0x03A8 + spc.x), spc.a, 5, 1);
    /* 1EF5 mov.w $03B0+x,a */
    spc_st_cyc((uint16_t)(0x03B0 + spc.x), spc.a, 5, 1);
    /* 1EF8 mov.w $04C0+x,a */
    spc_st_cyc((uint16_t)(0x04C0 + spc.x), spc.a, 5, 1);
    /* 1EFB mov.w $04C8+x,a */
    spc_st_cyc((uint16_t)(0x04C8 + spc.x), spc.a, 5, 1);
    /* 1EFE mov.w $04F0+x,a */
    spc_st_cyc((uint16_t)(0x04F0 + spc.x), spc.a, 5, 1);
    /* 1F01 mov.w $0398+x,a */
    spc_st_cyc((uint16_t)(0x0398 + spc.x), spc.a, 5, 1);
    /* 1F04 mov.w $04A8+x,a */
    spc_st_cyc((uint16_t)(0x04A8 + spc.x), spc.a, 5, 1);
    /* 1F07 mov.w $03A0+x,a */
    spc_st_cyc((uint16_t)(0x03A0 + spc.x), spc.a, 5, 1);
    /* 1F0A mov.w $04B0+x,a */
    spc_st_cyc((uint16_t)(0x04B0 + spc.x), spc.a, 5, 1);
    /* 1F0D mov.w $03B8+x,a */
    spc_st_cyc((uint16_t)(0x03B8 + spc.x), spc.a, 5, 1);
    /* 1F10 mov.w $0498+x,a */
    spc_st_cyc((uint16_t)(0x0498 + spc.x), spc.a, 5, 1);
    /* 1F13 mov.w $03C0+x,a */
    spc_st_cyc((uint16_t)(0x03C0 + spc.x), spc.a, 5, 1);
    /* 1F16 mov.w $04A0+x,a */
    spc_st_cyc((uint16_t)(0x04A0 + spc.x), spc.a, 5, 1);
    /* 1F19 mov a,#$8F */
    spc.a = spc_nz(0x8F);
    spc_cyc(2);
    /* 1F1B mov.w $03D0+x,a */
    spc_st_cyc((uint16_t)(0x03D0 + spc.x), spc.a, 5, 1);
    /* 1F1E mov a,#$E0 */
    spc.a = spc_nz(0xE0);
    spc_cyc(2);
    /* 1F20 mov.w $03D8+x,a */
    spc_st_cyc((uint16_t)(0x03D8 + spc.x), spc.a, 5, 1);
    /* 1F23 call $1E28 */
    spc_call_cyc(abi_SpcVoiceMaskNot, 0x1F26, 5, 1, 2);
    /* 1F26 mov.b a,$4A */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x4A)));
    /* 1F28 and.b a,$22 */
    spc_cyc(3);
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0x22)));
    /* 1F2A mov.b $4A,a */
    spc_st_cyc(spc_dp(0x4A), spc.a, 3, 1);
    /* 1F2C mov y,#$01 */
    spc.y = spc_nz(0x01);
    spc_cyc(2);
    /* 1F2E bra $1F48 */
    spc_cyc(4);
    goto L1F48;
L1F30:
    /* 1F30 mov.w a,$0418+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0418 + spc.x)));
    /* 1F33 dec a */
    spc.a = spc_dec(spc.a);
    spc_cyc(2);
    /* 1F34 mov.w $0418+x,a */
    spc_st_cyc((uint16_t)(0x0418 + spc.x), spc.a, 5, 1);
    /* 1F37 beq $1F43 */
    if (spc.z) { spc_cyc(4); goto L1F43; }
    spc_cyc(2);
    /* 1F39 cmp a,#$FF */
    spc_cmp(spc.a, 0xFF);
    spc_cyc(2);
    /* 1F3B beq $1F43 */
    if (spc.z) { spc_cyc(4); goto L1F43; }
    spc_cyc(2);
    /* 1F3D call $224A */
    spc_call_cyc(abi_SpcSfxPitchSlide, 0x1F40, 5, 1, 2);
    /* 1F40 jmp $2286 */
    spc_cyc(3);
    goto L2286;
L1F43:
    /* 1F43 mov.w a,$0400+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0400 + spc.x)));
    /* 1F46 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
L1F47:
    /* 1F47 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
L1F48:
    /* 1F48 mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 1F4A beq $1F47 */
    if (spc.z) { spc_cyc(4); goto L1F47; }
    spc_cyc(2);
    /* 1F4C bpl $1F9B */
    if (!spc.n) { spc_cyc(4); goto L1F9B; }
    spc_cyc(2);
    /* 1F4E setc */
    spc.c = 1;
    spc_cyc(2);
    /* 1F4F sbc a,#$E0 */
    spc.a = spc_sbc(spc.a, 0xE0);
    spc_cyc(2);
    /* 1F51 bmi $1FCC */
    if (spc.n) { spc_cyc(4); goto L1FCC; }
    spc_cyc(2);
    /* 1F53 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 1F54 push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 1F55 mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 1F56 jmp ($1F59+x) */
    spc_cyc(5);
    { uint16_t t = spc_rd((uint16_t)(0x1F59 + spc.x)); t |= (spc_cyc(1), spc_rd((uint16_t)(0x1F59 + spc.x + 1))) << 8;
      switch (t) {
      case 0x1F99: goto L1F99;
      case 0x2053: goto L2053;
      case 0x2057: goto L2057;
      case 0x205F: goto L205F;
      case 0x209E: goto L209E;
      case 0x20A0: goto L20A0;
      case 0x20EB: goto L20EB;
      case 0x20FA: goto L20FA;
      case 0x2114: goto L2114;
      case 0x2128: goto L2128;
      case 0x213C: goto L213C;
      case 0x2150: goto L2150;
      case 0x215F: goto L215F;
      case 0x216B: goto L216B;
      case 0x21C8: goto L21C8;
      case 0x21D2: goto L21D2;
      case 0x21F3: goto L21F3;
      case 0x21FC: goto L21FC;
      case 0x2204: goto L2204;
      case 0x220C: goto L220C;
      case 0x221E: goto L221E;
      case 0x2227: goto L2227;
      case 0x2232: goto L2232;
      case 0x223B: goto L223B;
      } }
    return;
L1F99:
    /* 1F99 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 1F9A ret */
    spc_ret_cyc(4, 1);
    return;
L1F9B:
    /* 1F9B push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 1F9C mov.w a,$0478+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0478 + spc.x)));
    /* 1F9F beq $1FA7 */
    if (spc.z) { spc_cyc(4); goto L1FA7; }
    spc_cyc(2);
    /* 1FA1 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 1FA3 beq $1FAA */
    if (spc.z) { spc_cyc(4); goto L1FAA; }
    spc_cyc(2);
    /* 1FA5 bra $1FAF */
    spc_cyc(4);
    goto L1FAF;
L1FA7:
    /* 1FA7 call $1E93 */
    spc_call_cyc(abi_SpcKeyOffVoice, 0x1FAA, 5, 1, 2);
L1FAA:
    /* 1FAA mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 1FAC mov.w $03E8+x,a */
    spc_st_cyc((uint16_t)(0x03E8 + spc.x), spc.a, 5, 1);
L1FAF:
    /* 1FAF mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1FB1 mov.w $0478+x,a */
    spc_st_cyc((uint16_t)(0x0478 + spc.x), spc.a, 5, 1);
    /* 1FB4 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 1FB5 mov.w $0418+x,a */
    spc_st_cyc((uint16_t)(0x0418 + spc.x), spc.a, 5, 1);
    /* 1FB8 cmp a,#$7F */
    spc_cmp(spc.a, 0x7F);
    spc_cyc(2);
    /* 1FBA bne $1FC0 */
    if (!spc.z) { spc_cyc(4); goto L1FC0; }
    spc_cyc(2);
    /* 1FBC mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 1FBE bra $1FC2 */
    spc_cyc(4);
    goto L1FC2;
L1FC0:
    /* 1FC0 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
L1FC2:
    /* 1FC2 mov.w $0420+x,a */
    spc_st_cyc((uint16_t)(0x0420 + spc.x), spc.a, 5, 1);
    /* 1FC5 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 1FC6 mov.w $0400+x,a */
    spc_st_cyc((uint16_t)(0x0400 + spc.x), spc.a, 5, 1);
    /* 1FC9 jmp $1CED */
    { spc_cyc(3); abi_SpcSfxChannelSave(); return; }
L1FCC:
    /* 1FCC push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 1FCD push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 1FCE clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 1FCF adc a,#$60 */
    spc.a = spc_adc(spc.a, 0x60);
    spc_cyc(2);
    /* 1FD1 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 1FD3 mov x,#$0C */
    spc.x = spc_nz(0x0C);
    spc_cyc(2);
    /* 1FD5 div ya,x */
    spc_div();
    spc_cyc(12);
    /* 1FD6 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 1FD7 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 1FD8 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 1FD9 mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 1FDA mov.w a,$CAA8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0xCAA8 + spc.x)));
    /* 1FDD mov.b $22,a */
    spc_st_cyc(spc_dp(0x22), spc.a, 3, 1);
    /* 1FDF mov.w a,$CAA9+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0xCAA9 + spc.x)));
    /* 1FE2 mov.b $23,a */
    spc_st_cyc(spc_dp(0x23), spc.a, 3, 1);
    /* 1FE4 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 1FE5 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 1FE6 sbc a,#$07 */
    spc.a = spc_sbc(spc.a, 0x07);
    spc_cyc(2);
    /* 1FE8 mov.b $24,a */
    spc_st_cyc(spc_dp(0x24), spc.a, 3, 1);
    /* 1FEA beq $1FFE */
    if (spc.z) { spc_cyc(4); goto L1FFE; }
    spc_cyc(2);
L1FEC:
    /* 1FEC mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 1FEE mov.b a,$23 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x23)));
    /* 1FF0 mov x,#$02 */
    spc.x = spc_nz(0x02);
    spc_cyc(2);
    /* 1FF2 div ya,x */
    spc_div();
    spc_cyc(12);
    /* 1FF3 mov.b $23,a */
    spc_st_cyc(spc_dp(0x23), spc.a, 3, 1);
    /* 1FF5 mov.b a,$22 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 1FF7 div ya,x */
    spc_div();
    spc_cyc(12);
    /* 1FF8 mov.b $22,a */
    spc_st_cyc(spc_dp(0x22), spc.a, 3, 1);
    /* 1FFA inc.b $24 */
    spc_rmw_cyc(spc_dp(0x24), spc_inc, 3, 1);
    /* 1FFC bne $1FEC */
    if (!spc.z) { spc_cyc(4); goto L1FEC; }
    spc_cyc(2);
L1FFE:
    /* 1FFE pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 1FFF mov.b a,$22 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 2001 mov.w $03B8+x,a */
    spc_st_cyc((uint16_t)(0x03B8 + spc.x), spc.a, 5, 1);
    /* 2004 mov.w $0498+x,a */
    spc_st_cyc((uint16_t)(0x0498 + spc.x), spc.a, 5, 1);
    /* 2007 mov.b a,$23 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x23)));
    /* 2009 mov.w $03C0+x,a */
    spc_st_cyc((uint16_t)(0x03C0 + spc.x), spc.a, 5, 1);
    /* 200C mov.w $04A0+x,a */
    spc_st_cyc((uint16_t)(0x04A0 + spc.x), spc.a, 5, 1);
    /* 200F mov.w a,$0440+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0440 + spc.x)));
    /* 2012 beq $204F */
    if (spc.z) { spc_cyc(4); goto L204F; }
    spc_cyc(2);
    /* 2014 push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 2015 mov.b $24,a */
    spc_st_cyc(spc_dp(0x24), spc.a, 3, 1);
    /* 2017 mov.b a,$23 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x23)));
    /* 2019 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 201A sbc a,#$10 */
    spc.a = spc_sbc(spc.a, 0x10);
    spc_cyc(2);
    /* 201C bmi $2030 */
    if (spc.n) { spc_cyc(4); goto L2030; }
    spc_cyc(2);
    /* 201E mov.b a,$22 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 2020 mov.b y,$23 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x23)));
    /* 2022 mov x,#$43 */
    spc.x = spc_nz(0x43);
    spc_cyc(2);
    /* 2024 div ya,x */
    spc_div();
    spc_cyc(12);
    /* 2025 mov y,#$04 */
    spc.y = spc_nz(0x04);
    spc_cyc(2);
    /* 2027 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 2028 mov x,#$FC */
    spc.x = spc_nz(0xFC);
    spc_cyc(2);
    /* 202A div ya,x */
    spc_div();
    spc_cyc(12);
    /* 202B mov.b y,$24 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x24)));
    /* 202D mul ya */
    spc_mul();
    spc_cyc(9);
    /* 202E bra $2045 */
    spc_cyc(4);
    goto L2045;
L2030:
    /* 2030 mov.b a,$22 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 2032 mov.b y,$23 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x23)));
    /* 2034 mov x,#$11 */
    spc.x = spc_nz(0x11);
    spc_cyc(2);
    /* 2036 div ya,x */
    spc_div();
    spc_cyc(12);
    /* 2037 mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 2038 mov a,#$FF */
    spc.a = spc_nz(0xFF);
    spc_cyc(2);
    /* 203A mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 203C div ya,x */
    spc_div();
    spc_cyc(12);
    /* 203D mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 203E mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 2040 mov.b a,$24 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x24)));
    /* 2042 div ya,x */
    spc_div();
    spc_cyc(12);
    /* 2043 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
L2045:
    /* 2045 addw ya,$22 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x22)); v |= (spc_cyc(2), spc_rd(spc_dp(0x23))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 2047 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 2048 mov.w $03B8+x,a */
    spc_st_cyc((uint16_t)(0x03B8 + spc.x), spc.a, 5, 1);
    /* 204B mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 204C mov.w $03C0+x,a */
    spc_st_cyc((uint16_t)(0x03C0 + spc.x), spc.a, 5, 1);
L204F:
    /* 204F pop y */
    spc_cyc(4);
    spc.y = spc_pull();
L2050:
    /* 2050 jmp $1F47 */
    spc_cyc(3);
    goto L1F47;
L2053:
    /* 2053 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 2055 bra $2059 */
    spc_cyc(4);
    goto L2059;
L2057:
    /* 2057 mov a,#$02 */
    spc.a = spc_nz(0x02);
    spc_cyc(2);
L2059:
    /* 2059 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 205A mov.w $0478+x,a */
    spc_st_cyc((uint16_t)(0x0478 + spc.x), spc.a, 5, 1);
L205D:
    /* 205D bra $2050 */
    spc_cyc(4);
    goto L2050;
L205F:
    /* 205F pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 2060 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 2061 mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2063 mov.w $03C8+x,a */
    spc_st_cyc((uint16_t)(0x03C8 + spc.x), spc.a, 5, 1);
    /* 2066 bra $205D */
    spc_cyc(4);
    goto L205D;
L209E:
    /* 209E set1 $2C.6 */
    spc_bit_set_cyc(0x2C, 6, 1, 3, 1);
L20A0:
    /* 20A0 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 20A1 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 20A2 mov.w a,$04F0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F0 + spc.x)));
    /* 20A5 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 20A7 beq $20AD */
    if (spc.z) { spc_cyc(4); goto L20AD; }
    spc_cyc(2);
    /* 20A9 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    spc_cyc(2);
    /* 20AB beq $20CA */
    if (spc.z) { spc_cyc(4); goto L20CA; }
    spc_cyc(2);
L20AD:
    /* 20AD mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 20AF call $2068 */
    spc_call_cyc(abi_SpcSfxEngineScale, 0x20B2, 5, 1, 2);
    /* 20B2 mov.w $0398+x,a */
    spc_st_cyc((uint16_t)(0x0398 + spc.x), spc.a, 5, 1);
    /* 20B5 mov.w $04A8+x,a */
    spc_st_cyc((uint16_t)(0x04A8 + spc.x), spc.a, 5, 1);
    /* 20B8 bbs $2C.6,$20BC */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2C)) >> 6 & 1)) { spc_cyc(4); goto L20BC; }
    spc_cyc(2);
    /* 20BB inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
L20BC:
    /* 20BC mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 20BE call $2068 */
    spc_call_cyc(abi_SpcSfxEngineScale, 0x20C1, 5, 1, 2);
    /* 20C1 mov.w $03A0+x,a */
    spc_st_cyc((uint16_t)(0x03A0 + spc.x), spc.a, 5, 1);
    /* 20C4 mov.w $04B0+x,a */
    spc_st_cyc((uint16_t)(0x04B0 + spc.x), spc.a, 5, 1);
    /* 20C7 jmp $20DE */
    spc_cyc(3);
    goto L20DE;
L20CA:
    /* 20CA mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 20CC mov.w $0398+x,a */
    spc_st_cyc((uint16_t)(0x0398 + spc.x), spc.a, 5, 1);
    /* 20CF mov.w $04A8+x,a */
    spc_st_cyc((uint16_t)(0x04A8 + spc.x), spc.a, 5, 1);
    /* 20D2 bbs $2C.6,$20D6 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2C)) >> 6 & 1)) { spc_cyc(4); goto L20D6; }
    spc_cyc(2);
    /* 20D5 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
L20D6:
    /* 20D6 mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 20D8 mov.w $03A0+x,a */
    spc_st_cyc((uint16_t)(0x03A0 + spc.x), spc.a, 5, 1);
    /* 20DB mov.w $04B0+x,a */
    spc_st_cyc((uint16_t)(0x04B0 + spc.x), spc.a, 5, 1);
L20DE:
    /* 20DE mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 20E0 mov.w $03A8+x,a */
    spc_st_cyc((uint16_t)(0x03A8 + spc.x), spc.a, 5, 1);
    /* 20E3 mov.w $03B0+x,a */
    spc_st_cyc((uint16_t)(0x03B0 + spc.x), spc.a, 5, 1);
    /* 20E6 clr1 $2C.6 */
    spc_bit_set_cyc(0x2C, 6, 0, 3, 1);
L20E8:
    /* 20E8 jmp $1F47 */
    spc_cyc(3);
    goto L1F47;
L20EB:
    /* 20EB pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 20EC inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 20ED mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 20EF and a,#$0F */
    spc.a = spc_and(spc.a, 0x0F);
    spc_cyc(2);
    /* 20F1 mov.b $22,a */
    spc_st_cyc(spc_dp(0x22), spc.a, 3, 1);
    /* 20F3 mov.w a,$03D0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03D0 + spc.x)));
    /* 20F6 and a,#$F0 */
    spc.a = spc_and(spc.a, 0xF0);
    spc_cyc(2);
    /* 20F8 bra $210B */
    spc_cyc(4);
    goto L210B;
L20FA:
    /* 20FA pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 20FB inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 20FC mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 20FE and a,#$07 */
    spc.a = spc_and(spc.a, 0x07);
    spc_cyc(2);
    /* 2100 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 2101 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 2102 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 2103 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 2104 mov.b $22,a */
    spc_st_cyc(spc_dp(0x22), spc.a, 3, 1);
    /* 2106 mov.w a,$03D0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03D0 + spc.x)));
    /* 2109 and a,#$8F */
    spc.a = spc_and(spc.a, 0x8F);
    spc_cyc(2);
L210B:
    /* 210B or.b a,$22 */
    spc_cyc(3);
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x22)));
    /* 210D or a,#$80 */
    spc.a = spc_or(spc.a, 0x80);
    spc_cyc(2);
    /* 210F mov.w $03D0+x,a */
    spc_st_cyc((uint16_t)(0x03D0 + spc.x), spc.a, 5, 1);
L2112:
    /* 2112 bra $20E8 */
    spc_cyc(4);
    goto L20E8;
L2114:
    /* 2114 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 2115 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 2116 mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2118 and a,#$07 */
    spc.a = spc_and(spc.a, 0x07);
    spc_cyc(2);
    /* 211A asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 211B asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 211C asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 211D asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 211E asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 211F mov.b $22,a */
    spc_st_cyc(spc_dp(0x22), spc.a, 3, 1);
    /* 2121 mov.w a,$03D8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03D8 + spc.x)));
    /* 2124 and a,#$1F */
    spc.a = spc_and(spc.a, 0x1F);
    spc_cyc(2);
    /* 2126 bra $2135 */
    spc_cyc(4);
    goto L2135;
L2128:
    /* 2128 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 2129 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 212A mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 212C and a,#$1F */
    spc.a = spc_and(spc.a, 0x1F);
    spc_cyc(2);
    /* 212E mov.b $22,a */
    spc_st_cyc(spc_dp(0x22), spc.a, 3, 1);
    /* 2130 mov.w a,$03D8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03D8 + spc.x)));
    /* 2133 and a,#$E0 */
    spc.a = spc_and(spc.a, 0xE0);
    spc_cyc(2);
L2135:
    /* 2135 or.b a,$22 */
    spc_cyc(3);
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x22)));
    /* 2137 mov.w $03D8+x,a */
    spc_st_cyc((uint16_t)(0x03D8 + spc.x), spc.a, 5, 1);
L213A:
    /* 213A bra $2112 */
    spc_cyc(4);
    goto L2112;
L213C:
    /* 213C pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 213D inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 213E mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2140 mov.w $0428+x,a */
    spc_st_cyc((uint16_t)(0x0428 + spc.x), spc.a, 5, 1);
    /* 2143 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 2144 mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2146 mov.w $0430+x,a */
    spc_st_cyc((uint16_t)(0x0430 + spc.x), spc.a, 5, 1);
    /* 2149 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 214B mov.w $0438+x,a */
    spc_st_cyc((uint16_t)(0x0438 + spc.x), spc.a, 5, 1);
L214E:
    /* 214E bra $213A */
    spc_cyc(4);
    goto L213A;
L2150:
    /* 2150 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 2151 bbs $28.0,$2169 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x28)) >> 0 & 1)) { spc_cyc(4); goto L2169; }
    spc_cyc(2);
    /* 2154 call $1E19 */
    spc_call_cyc(abi_SpcVoiceMask, 0x2157, 5, 1, 2);
    /* 2157 mov.b a,$4A */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x4A)));
    /* 2159 or.b a,$22 */
    spc_cyc(3);
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x22)));
    /* 215B mov.b $4A,a */
    spc_st_cyc(spc_dp(0x4A), spc.a, 3, 1);
    /* 215D bra $2169 */
    spc_cyc(4);
    goto L2169;
L215F:
    /* 215F pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 2160 call $1E28 */
    spc_call_cyc(abi_SpcVoiceMaskNot, 0x2163, 5, 1, 2);
    /* 2163 mov.b a,$4A */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x4A)));
    /* 2165 and.b a,$22 */
    spc_cyc(3);
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0x22)));
    /* 2167 mov.b $4A,a */
    spc_st_cyc(spc_dp(0x4A), spc.a, 3, 1);
L2169:
    /* 2169 bra $214E */
    spc_cyc(4);
    goto L214E;
L216B:
    /* 216B inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 216C mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 216E mov.b $24,a */
    spc_st_cyc(spc_dp(0x24), spc.a, 3, 1);
    /* 2170 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 2171 mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2173 mov.b $25,a */
    spc_st_cyc(spc_dp(0x25), spc.a, 3, 1);
    /* 2175 push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 2176 mov x,#$BC */
    spc.x = spc_nz(0xBC);
    spc_cyc(2);
L2178:
    /* 2178 mov.w a,$0702+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0702 + spc.x)));
    /* 217B cmp.b a,$24 */
    spc_cyc(3);
    spc_cmp(spc.a, spc_rd(spc_dp(0x24)));
    /* 217D beq $2183 */
    if (spc.z) { spc_cyc(4); goto L2183; }
    spc_cyc(2);
    /* 217F inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 2180 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 2181 bra $2178 */
    spc_cyc(4);
    goto L2178;
L2183:
    /* 2183 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 2184 mov.w a,$0702+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0702 + spc.x)));
    /* 2187 cmp.b a,$25 */
    spc_cyc(3);
    spc_cmp(spc.a, spc_rd(spc_dp(0x25)));
    /* 2189 beq $2192 */
    if (spc.z) { spc_cyc(4); goto L2192; }
    spc_cyc(2);
    /* 218B inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 218C cmp x,#$00 */
    spc_cmp(spc.x, 0x00);
    spc_cyc(2);
    /* 218E beq $2198 */
    if (spc.z) { spc_cyc(4); goto L2198; }
    spc_cyc(2);
    /* 2190 bra $2178 */
    spc_cyc(4);
    goto L2178;
L2192:
    /* 2192 mov a,x */
    spc.a = spc_nz(spc.x);
    spc_cyc(2);
    /* 2193 lsr a */
    spc.a = spc_lsr(spc.a);
    spc_cyc(2);
    /* 2194 inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
    /* 2195 call $219C */
    spc_call_cyc(abi_SpcSfxStart, 0x2198, 5, 1, 2);
L2198:
    /* 2198 pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 2199 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 219A bra $2169 */
    spc_cyc(4);
    goto L2169;
L21C8:
    /* 21C8 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 21C9 mov.w a,$0420+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0420 + spc.x)));
    /* 21CC bne $21D1 */
    if (!spc.z) { spc_cyc(4); goto L21D1; }
    spc_cyc(2);
    /* 21CE call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x21D1, 5, 1, 2);
L21D1:
    /* 21D1 ret */
    spc_ret_cyc(4, 1);
    return;
L21D2:
    /* 21D2 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 21D3 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 21D4 mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 21D6 mov.w $0500+x,a */
    spc_st_cyc((uint16_t)(0x0500 + spc.x), spc.a, 5, 1);
    /* 21D9 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 21DA mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 21DC mov.w $0508+x,a */
    spc_st_cyc((uint16_t)(0x0508 + spc.x), spc.a, 5, 1);
    /* 21DF inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 21E0 mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 21E2 mov.w $0510+x,a */
    spc_st_cyc((uint16_t)(0x0510 + spc.x), spc.a, 5, 1);
    /* 21E5 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 21E6 mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 21E8 mov.w $0518+x,a */
    spc_st_cyc((uint16_t)(0x0518 + spc.x), spc.a, 5, 1);
    /* 21EB mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 21ED mov.w $0520+x,a */
    spc_st_cyc((uint16_t)(0x0520 + spc.x), spc.a, 5, 1);
L21F0:
    /* 21F0 jmp $1F47 */
    spc_cyc(3);
    goto L1F47;
L21F3:
    /* 21F3 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 21F4 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 21F5 mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 21F7 mov.w $0440+x,a */
    spc_st_cyc((uint16_t)(0x0440 + spc.x), spc.a, 5, 1);
L21FA:
    /* 21FA bra $21F0 */
    spc_cyc(4);
    goto L21F0;
L21FC:
    /* 21FC pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 21FD mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 21FF mov.w $0438+x,a */
    spc_st_cyc((uint16_t)(0x0438 + spc.x), spc.a, 5, 1);
L2202:
    /* 2202 bra $21FA */
    spc_cyc(4);
    goto L21FA;
L2204:
    /* 2204 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 2205 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 2207 mov.w $0520+x,a */
    spc_st_cyc((uint16_t)(0x0520 + spc.x), spc.a, 5, 1);
L220A:
    /* 220A bra $2202 */
    spc_cyc(4);
    goto L2202;
L220C:
    /* 220C pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 220D inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 220E mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2210 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 2211 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 2212 mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2214 mov.b $21,a */
    spc_st_cyc(spc_dp(0x21), spc.a, 3, 1);
    /* 2216 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 2217 mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 2219 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 221B jmp $1F48 */
    spc_cyc(3);
    goto L1F48;
L221E:
    /* 221E pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 221F inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 2220 mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2222 mov.w $0448+x,a */
    spc_st_cyc((uint16_t)(0x0448 + spc.x), spc.a, 5, 1);
L2225:
    /* 2225 bra $220A */
    spc_cyc(4);
    goto L220A;
L2227:
    /* 2227 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 2228 call $1E93 */
    spc_call_cyc(abi_SpcKeyOffVoice, 0x222B, 5, 1, 2);
    /* 222B mov a,#$02 */
    spc.a = spc_nz(0x02);
    spc_cyc(2);
    /* 222D mov.w $0478+x,a */
    spc_st_cyc((uint16_t)(0x0478 + spc.x), spc.a, 5, 1);
L2230:
    /* 2230 bra $2225 */
    spc_cyc(4);
    goto L2225;
L2232:
    /* 2232 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 2233 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 2234 mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2236 mov.w $04F0+x,a */
    spc_st_cyc((uint16_t)(0x04F0 + spc.x), spc.a, 5, 1);
L2239:
    /* 2239 bra $2230 */
    spc_cyc(4);
    goto L2230;
L223B:
    /* 223B pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 223C inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 223D mov a,($20)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= (spc_cyc(1), spc_rd(spc_dp(0x21))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 223F and a,#$FE */
    spc.a = spc_and(spc.a, 0xFE);
    spc_cyc(2);
    /* 2241 push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 2242 push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 2243 call $EDF0 */
    spc_call_cyc(abi_SpcNoiseSampleInit, 0x2246, 5, 1, 2);
    /* 2246 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 2247 pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 2248 bra $2239 */
    spc_cyc(4);
    goto L2239;
L2286:
    /* 2286 mov.w a,$0520+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0520 + spc.x)));
    /* 2289 beq $22AF */
    if (spc.z) { spc_cyc(4); goto L22AF; }
    spc_cyc(2);
    /* 228B mov.w a,$0500+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0500 + spc.x)));
    /* 228E mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 2290 mov.w a,$0508+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0508 + spc.x)));
    /* 2293 mov.b $21,a */
    spc_st_cyc(spc_dp(0x21), spc.a, 3, 1);
    /* 2295 call $22B0 */
    spc_call_cyc(abi_SpcSfxVolumeSlide, 0x2298, 5, 1, 2);
    /* 2298 mov.w a,$0510+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0510 + spc.x)));
    /* 229B mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 229D mov.w a,$0518+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0518 + spc.x)));
    /* 22A0 mov.b $21,a */
    spc_st_cyc(spc_dp(0x21), spc.a, 3, 1);
    /* 22A2 mov a,x */
    spc.a = spc_nz(spc.x);
    spc_cyc(2);
    /* 22A3 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 22A4 adc a,#$08 */
    spc.a = spc_adc(spc.a, 0x08);
    spc_cyc(2);
    /* 22A6 mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 22A7 call $22B0 */
    spc_call_cyc(abi_SpcSfxVolumeSlide, 0x22AA, 5, 1, 2);
    /* 22AA mov a,x */
    spc.a = spc_nz(spc.x);
    spc_cyc(2);
    /* 22AB setc */
    spc.c = 1;
    spc_cyc(2);
    /* 22AC sbc a,#$08 */
    spc.a = spc_sbc(spc.a, 0x08);
    spc_cyc(2);
    /* 22AE mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
L22AF:
    /* 22AF ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:DB67 SpcSfxReadNoAge
 * WHAT IT DOES: SpcSfxRead's return for a channel whose age $03F0 is 0, which never happens there (SpcSfxRead runs only on channels that have started); a direct call is its only way in.
 * HOW: RET.
 * Entry: none. Exit: nothing changed.
 * @spc 1EDD
 * @spcuses 1EDD-1EDE
 */
void abi_SpcSfxReadNoAge(void)
{
    /* 1EDD ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:DCF2 SpcSfxEngineScale
 * WHAT IT DOES: For a channel in engine mode ($04F0 = 4), scales the volume A by the engine speed from port 3 ($03 * 3, / 2, + $80); otherwise A is kept.
 * HOW: SpcDiv16, SpcMul16; clamped to $7F.
 * Entry: A = volume, X = channel. Exit: A = volume; X, Y kept.
 * @spc 2068
 * @spcuses 2068-209E,CA75-CA96
 */
void abi_SpcSfxEngineScale(void)
{
    /* 2068 push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 2069 push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 206A mov.b $22,a */
    spc_st_cyc(spc_dp(0x22), spc.a, 3, 1);
    /* 206C mov.w a,$04F0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F0 + spc.x)));
    /* 206F cmp a,#$04 */
    spc_cmp(spc.a, 0x04);
    spc_cyc(2);
    /* 2071 bne $209A */
    if (!spc.z) { spc_cyc(4); goto L209A; }
    spc_cyc(2);
    /* 2073 mov.b a,$03 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x03)));
    /* 2075 bne $2078 */
    if (!spc.z) { spc_cyc(4); goto L2078; }
    spc_cyc(2);
    /* 2077 inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
L2078:
    /* 2078 mov y,#$03 */
    spc.y = spc_nz(0x03);
    spc_cyc(2);
    /* 207A mul ya */
    spc_mul();
    spc_cyc(9);
    /* 207B mov x,#$02 */
    spc.x = spc_nz(0x02);
    spc_cyc(2);
    /* 207D call $CA89 */
    spc_call_cyc(abi_SpcDiv16, 0x2080, 5, 1, 2);
    /* 2080 mov.b $24,a */
    spc_st_cyc(spc_dp(0x24), spc.a, 3, 1);
    /* 2082 mov.b $25,y */
    spc_st_cyc(spc_dp(0x25), spc.y, 3, 1);
    /* 2084 mov a,#$80 */
    spc.a = spc_nz(0x80);
    spc_cyc(2);
    /* 2086 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 2088 addw ya,$24 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x24)); v |= (spc_cyc(2), spc_rd(spc_dp(0x25))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 208A push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 208B pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 208C mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 208D mov.b a,$22 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 208F call $CA75 */
    spc_call_cyc(abi_SpcMul16, 0x2092, 5, 1, 2);
    /* 2092 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 2093 bpl $2097 */
    if (!spc.n) { spc_cyc(4); goto L2097; }
    spc_cyc(2);
    /* 2095 mov a,#$7F */
    spc.a = spc_nz(0x7F);
    spc_cyc(2);
L2097:
    /* 2097 pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 2098 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 2099 ret */
    spc_ret_cyc(4, 1);
    return;
L209A:
    /* 209A mov.b a,$22 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 209C bra $2097 */
    spc_cyc(4);
    goto L2097;
}

/*
 * @routine 19:DE26 SpcSfxStart
 * WHAT IT DOES: Starts sound A (its data at $24/$25) on its channel unless that channel is in mode 2 or 3: mode bits, data pointer, active, then the start state (SpcSfxChannelStep's code).
 * HOW: SpcSfxChannel, SpcSfxMode, SpcSfxActivate.
 * Entry: A = sound, $24/$25 = data. Exit: as SpcSfxChannelStep.
 * @spc 219C
 * @spcuses 1C84-1C8A,1C9E-1CCD,1D53-1D69,1E0F-1E28,1E93-1E9D,219C-21C8
 */
void abi_SpcSfxStart(void)
{
    /* 219C mov.b $23,a */
    spc_st_cyc(spc_dp(0x23), spc.a, 3, 1);
    /* 219E push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 219F mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 21A1 mov a,($24)+y */
    spc_cyc(3);
    { uint16_t p = spc_rd(spc_dp(0x24)); p |= (spc_cyc(1), spc_rd(spc_dp(0x25))) << 8;
      spc_cyc(2);
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 21A3 mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 21A4 mov.w a,$04F0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F0 + spc.x)));
    /* 21A7 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    spc_cyc(2);
    /* 21A9 beq $21C6 */
    if (spc.z) { spc_cyc(4); goto L21C6; }
    spc_cyc(2);
    /* 21AB cmp a,#$03 */
    spc_cmp(spc.a, 0x03);
    spc_cyc(2);
    /* 21AD beq $21C6 */
    if (spc.z) { spc_cyc(4); goto L21C6; }
    spc_cyc(2);
    /* 21AF call $1C9E */
    spc_call_cyc(abi_SpcSfxChannel, 0x21B2, 5, 1, 2);
    /* 21B2 call $1CA5 */
    spc_call_cyc(abi_SpcSfxMode, 0x21B5, 5, 1, 2);
    /* 21B5 mov.b a,$24 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x24)));
    /* 21B7 mov.w $0488+x,a */
    spc_st_cyc((uint16_t)(0x0488 + spc.x), spc.a, 5, 1);
    /* 21BA mov.b a,$25 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x25)));
    /* 21BC mov.w $0490+x,a */
    spc_st_cyc((uint16_t)(0x0490 + spc.x), spc.a, 5, 1);
    /* 21BF call $1C84 */
    spc_call_cyc(abi_SpcSfxActivate, 0x21C2, 5, 1, 2);
    /* 21C2 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 21C3 jmp $1D53 */
    spc_cyc(3);
    goto L1D53;
L21C6:
    /* 21C6 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 21C7 ret */
    spc_ret_cyc(4, 1);
    return;
L1D53:
    /* 1D53 mov.w $0390+x,a */
    spc_st_cyc((uint16_t)(0x0390 + spc.x), spc.a, 5, 1);
    /* 1D56 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 1D58 mov.w $03F8+x,a */
    spc_st_cyc((uint16_t)(0x03F8 + spc.x), spc.a, 5, 1);
    /* 1D5B mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 1D5D mov.w $03F0+x,a */
    spc_st_cyc((uint16_t)(0x03F0 + spc.x), spc.a, 5, 1);
    /* 1D60 mov.w $03E0+x,a */
    spc_st_cyc((uint16_t)(0x03E0 + spc.x), spc.a, 5, 1);
    { abi_SpcSfxTakeVoice(); return; }
}

/*
 * @routine 19:DED4 SpcSfxPitchSlide
 * WHAT IT DOES: Channel X's pitch slide ($0438 on): the delta $0428/$0430 added to the pitch $0498/$04A0, clamped to 0-$3FFF, and copied to the DSP pitch $03B8/$03C0.
 * HOW: ADDW; sign and range tests.
 * Entry: X = channel. Exit: A, Y changed.
 * @spc 224A
 * @spcuses 224A-2286
 */
void abi_SpcSfxPitchSlide(void)
{
    /* 224A mov.w a,$0438+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0438 + spc.x)));
    /* 224D beq $227A */
    if (spc.z) { spc_cyc(4); goto L227A; }
    spc_cyc(2);
    /* 224F mov.w a,$0428+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0428 + spc.x)));
    /* 2252 mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 2254 mov.w a,$0430+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0430 + spc.x)));
    /* 2257 mov.b $21,a */
    spc_st_cyc(spc_dp(0x21), spc.a, 3, 1);
    /* 2259 mov.w a,$04A0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04A0 + spc.x)));
    /* 225C mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 225D mov.w a,$0498+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0498 + spc.x)));
    /* 2260 addw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 2262 mov.w $0498+x,a */
    spc_st_cyc((uint16_t)(0x0498 + spc.x), spc.a, 5, 1);
    /* 2265 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 2266 mov.w $04A0+x,a */
    spc_st_cyc((uint16_t)(0x04A0 + spc.x), spc.a, 5, 1);
    /* 2269 bmi $227B */
    if (spc.n) { spc_cyc(4); goto L227B; }
    spc_cyc(2);
    /* 226B setc */
    spc.c = 1;
    spc_cyc(2);
    /* 226C sbc a,#$40 */
    spc.a = spc_sbc(spc.a, 0x40);
    spc_cyc(2);
    /* 226E bpl $2280 */
    if (!spc.n) { spc_cyc(4); goto L2280; }
    spc_cyc(2);
    /* 2270 mov.w a,$0498+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0498 + spc.x)));
L2273:
    /* 2273 mov.w $03B8+x,a */
    spc_st_cyc((uint16_t)(0x03B8 + spc.x), spc.a, 5, 1);
    /* 2276 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 2277 mov.w $03C0+x,a */
    spc_st_cyc((uint16_t)(0x03C0 + spc.x), spc.a, 5, 1);
L227A:
    /* 227A ret */
    spc_ret_cyc(4, 1);
    return;
L227B:
    /* 227B mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 227D mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 227E bra $2273 */
    spc_cyc(4);
    goto L2273;
L2280:
    /* 2280 mov a,#$FF */
    spc.a = spc_nz(0xFF);
    spc_cyc(2);
    /* 2282 mov y,#$3F */
    spc.y = spc_nz(0x3F);
    spc_cyc(2);
    /* 2284 bra $2273 */
    spc_cyc(4);
    goto L2273;
}

/*
 * @routine 19:DF3A SpcSfxVolumeSlide
 * WHAT IT DOES: One side of channel X's volume slide: the delta $20/$21 added to the volume $03A8/$04A8, clamped, and the volume byte to $0398.
 * HOW: ADDW; sign tests.
 * Entry: X = channel (plus 8 for the right side). Exit: A = the volume.
 * @spc 22B0
 * @spcuses 22B0-22D3
 */
void abi_SpcSfxVolumeSlide(void)
{
    /* 22B0 mov.w a,$04A8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04A8 + spc.x)));
    /* 22B3 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 22B4 mov.w a,$03A8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03A8 + spc.x)));
    /* 22B7 addw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 22B9 mov.w $03A8+x,a */
    spc_st_cyc((uint16_t)(0x03A8 + spc.x), spc.a, 5, 1);
    /* 22BC mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 22BD mov.w $04A8+x,a */
    spc_st_cyc((uint16_t)(0x04A8 + spc.x), spc.a, 5, 1);
    /* 22C0 bmi $22C4 */
    if (spc.n) { spc_cyc(4); goto L22C4; }
    spc_cyc(2);
    /* 22C2 bra $22CF */
    spc_cyc(4);
    goto L22CF;
L22C4:
    /* 22C4 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 22C5 sbc a,#$C0 */
    spc.a = spc_sbc(spc.a, 0xC0);
    spc_cyc(2);
    /* 22C7 bmi $22CD */
    if (spc.n) { spc_cyc(4); goto L22CD; }
    spc_cyc(2);
    /* 22C9 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 22CB bra $22CF */
    spc_cyc(4);
    goto L22CF;
L22CD:
    /* 22CD mov a,#$7F */
    spc.a = spc_nz(0x7F);
    spc_cyc(2);
L22CF:
    /* 22CF mov.w $0398+x,a */
    spc_st_cyc((uint16_t)(0x0398 + spc.x), spc.a, 5, 1);
    /* 22D2 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:DFC0 SpcSfxFadeVolume
 * WHAT IT DOES: The volume of channel X while it fades out (count A, $04C0): the start volume and per-step drop taken at count 1, the volume interpolated down over $3C steps; at the end the channel stops (or the driver reloads or leaves, by flags).
 * HOW: SpcMul16, SpcDiv16 by $3C; SUBW.
 * Entry: A = count, X = channel. Exit: A = volume.
 * @spc 2336
 * @spcuses 09E8-09EF,1941-1951,1AE9-1AF6,1B15-1B23,2336-23B2,CA75-CAA8
 */
void abi_SpcSfxFadeVolume(void)
{
    /* 2336 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 2338 beq $2348 */
    if (spc.z) { spc_cyc(4); goto L2348; }
    spc_cyc(2);
    /* 233A cmp a,#$3C */
    spc_cmp(spc.a, 0x3C);
    spc_cyc(2);
    /* 233C beq $2385 */
    if (spc.z) { spc_cyc(4); goto L2385; }
    spc_cyc(2);
    /* 233E cmp a,#$FF */
    spc_cmp(spc.a, 0xFF);
    spc_cyc(2);
    /* 2340 beq $23AD */
    if (spc.z) { spc_cyc(4); goto L23AD; }
    spc_cyc(2);
    /* 2342 inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
    /* 2343 mov.w $04C0+x,a */
    spc_st_cyc((uint16_t)(0x04C0 + spc.x), spc.a, 5, 1);
    /* 2346 bra $2361 */
    spc_cyc(4);
    goto L2361;
L2348:
    /* 2348 inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
    /* 2349 mov.w $04C0+x,a */
    spc_st_cyc((uint16_t)(0x04C0 + spc.x), spc.a, 5, 1);
    /* 234C mov.w a,$0398+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0398 + spc.x)));
    /* 234F mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 2350 mov.w a,$03A8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03A8 + spc.x)));
    /* 2353 push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 2354 mov x,#$3C */
    spc.x = spc_nz(0x3C);
    spc_cyc(2);
    /* 2356 call $CA89 */
    spc_call_cyc(abi_SpcDiv16, 0x2359, 5, 1, 2);
    /* 2359 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 235A mov.w $04D0+x,a */
    spc_st_cyc((uint16_t)(0x04D0 + spc.x), spc.a, 5, 1);
    /* 235D mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 235E mov.w $04E0+x,a */
    spc_st_cyc((uint16_t)(0x04E0 + spc.x), spc.a, 5, 1);
L2361:
    /* 2361 mov.w a,$04C0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04C0 + spc.x)));
    /* 2364 dec a */
    spc.a = spc_dec(spc.a);
    spc_cyc(2);
    /* 2365 mov.b $22,a */
    spc_st_cyc(spc_dp(0x22), spc.a, 3, 1);
    /* 2367 mov.w a,$04D0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04D0 + spc.x)));
    /* 236A mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 236B mov.w a,$04E0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04E0 + spc.x)));
    /* 236E push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 236F mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 2370 mov.b a,$22 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 2372 call $CA75 */
    spc_call_cyc(abi_SpcMul16, 0x2375, 5, 1, 2);
    /* 2375 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 2376 mov.b $22,a */
    spc_st_cyc(spc_dp(0x22), spc.a, 3, 1);
    /* 2378 mov.b $23,y */
    spc_st_cyc(spc_dp(0x23), spc.y, 3, 1);
    /* 237A mov.w a,$0398+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0398 + spc.x)));
    /* 237D mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 237E mov.w a,$03A8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03A8 + spc.x)));
    /* 2381 subw ya,$22 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x22)); v |= (spc_cyc(2), spc_rd(spc_dp(0x23))) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 2383 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 2384 ret */
    spc_ret_cyc(4, 1);
    return;
L2385:
    /* 2385 bbs $2C.7,$238D */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2C)) >> 7 & 1)) { spc_cyc(4); goto L238D; }
    spc_cyc(2);
    /* 2388 bbs $2D.0,$2390 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x2D)) >> 0 & 1)) { spc_cyc(4); goto L2390; }
    spc_cyc(2);
    /* 238B bra $2393 */
    spc_cyc(4);
    goto L2393;
L238D:
    /* 238D jmp $1AE9 */
    spc_cyc(3);
    goto L1AE9;
L2390:
    /* 2390 jmp $1B15 */
    spc_cyc(3);
    goto L1B15;
L2393:
    /* 2393 mov.b $22,x */
    spc_st_cyc(spc_dp(0x22), spc.x, 3, 1);
    /* 2395 bbs $22.3,$23A2 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x22)) >> 3 & 1)) { spc_cyc(4); goto L23A2; }
    spc_cyc(2);
    /* 2398 call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x239B, 5, 1, 2);
L239B:
    /* 239B mov a,#$FF */
    spc.a = spc_nz(0xFF);
    spc_cyc(2);
    /* 239D mov.w $04C0+x,a */
    spc_st_cyc((uint16_t)(0x04C0 + spc.x), spc.a, 5, 1);
    /* 23A0 inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
    /* 23A1 ret */
    spc_ret_cyc(4, 1);
    return;
L23A2:
    /* 23A2 call $CA9F */
    spc_call_cyc(abi_SpcIndexLeft, 0x23A5, 5, 1, 2);
    /* 23A5 call $194C */
    spc_call_cyc(abi_SpcSfxStop, 0x23A8, 5, 1, 2);
    /* 23A8 call $CA96 */
    spc_call_cyc(abi_SpcIndexRight, 0x23AB, 5, 1, 2);
    /* 23AB bra $239B */
    spc_cyc(4);
    goto L239B;
L23AD:
    /* 23AD inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
    /* 23AE mov.w $04C0+x,a */
    spc_st_cyc((uint16_t)(0x04C0 + spc.x), spc.a, 5, 1);
    /* 23B1 ret */
    spc_ret_cyc(4, 1);
    return;
L1AE9:
    /* 1AE9 call $1AEF */
    spc_call_cyc(abi_SpcKeyOffAll, 0x1AEC, 5, 1, 2);
    /* 1AEC jmp $0675 */
    { spc_cyc(3); spc_restart(0x0675, abi_SpcReload); return; }
L1B15:
    /* 1B15 call $1AEF */
    spc_call_cyc(abi_SpcKeyOffAll, 0x1B18, 5, 1, 2);
    /* 1B18 mov.w a,$00F1 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x00F1));
    /* 1B1B or a,#$80 */
    spc.a = spc_or(spc.a, 0x80);
    spc_cyc(2);
    /* 1B1D mov.w $00F1,a */
    spc_st_cyc(0x00F1, spc.a, 4, 1);
    /* 1B20 jmp $FFC0 */
    { spc_cyc(3); spc_boot_rom(); return; }
}

/*
 * @routine 19:E03C SpcSfxVolumeAdjust
 * WHAT IT DOES: For sound $25, adds $0530 * 4 to the volume A.
 * HOW: MUL, ADC.
 * Entry: A = volume, X = channel. Exit: A = volume; Y kept.
 * @spc 23B2
 * @spcuses 23B2-23CF
 */
void abi_SpcSfxVolumeAdjust(void)
{
    /* 23B2 push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 23B3 mov.b $22,a */
    spc_st_cyc(spc_dp(0x22), spc.a, 3, 1);
    /* 23B5 cmp a,#$00 */
    spc_cmp(spc.a, 0x00);
    spc_cyc(2);
    /* 23B7 beq $23C9 */
    if (spc.z) { spc_cyc(4); goto L23C9; }
    spc_cyc(2);
    /* 23B9 mov.w a,$0390+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 23BC cmp a,#$25 */
    spc_cmp(spc.a, 0x25);
    spc_cyc(2);
    /* 23BE bne $23CB */
    if (!spc.z) { spc_cyc(4); goto L23CB; }
    spc_cyc(2);
    /* 23C0 mov.w a,$0530 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0530));
    /* 23C3 mov y,#$04 */
    spc.y = spc_nz(0x04);
    spc_cyc(2);
    /* 23C5 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 23C6 clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 23C7 adc.b a,$22 */
    spc_cyc(3);
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0x22)));
L23C9:
    /* 23C9 pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 23CA ret */
    spc_ret_cyc(4, 1);
    return;
L23CB:
    /* 23CB mov.b a,$22 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 23CD bra $23C9 */
    spc_cyc(4);
    goto L23C9;
}

/*
 * @routine 19:E059 SpcSfxSpecial
 * WHAT IT DOES: When channel X plays a sound in the list at $23EB, runs its handler from the table at $23FB: the engine sounds' pitch and volume from the car's speed on port 3 (SpcEnginePitch and its variants), and rev timing.
 * HOW: JMP ($23FB+X) with X pushed.
 * Entry: X = channel. Exit: as the handler leaves it.
 * @spc 23CF
 * @spcuses 23CF-23EB,23FB-2419,2441-26D6
 */
void abi_SpcSfxSpecial(void)
{
    /* 23CF mov.w a,$0390+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 23D2 mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 23D4 push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 23D5 mov x,#$00 */
    spc.x = spc_nz(0x00);
    spc_cyc(2);
L23D7:
    /* 23D7 mov.w a,$23EB+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x23EB + spc.x)));
    /* 23DA beq $23E3 */
    if (spc.z) { spc_cyc(4); goto L23E3; }
    spc_cyc(2);
    /* 23DC cmp.b a,$20 */
    spc_cyc(3);
    spc_cmp(spc.a, spc_rd(spc_dp(0x20)));
    /* 23DE beq $23E5 */
    if (spc.z) { spc_cyc(4); goto L23E5; }
    spc_cyc(2);
    /* 23E0 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* 23E1 bra $23D7 */
    spc_cyc(4);
    goto L23D7;
L23E3:
    /* 23E3 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 23E4 ret */
    spc_ret_cyc(4, 1);
    return;
L23E5:
    /* 23E5 mov a,x */
    spc.a = spc_nz(spc.x);
    spc_cyc(2);
    /* 23E6 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 23E7 mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 23E8 jmp ($23FB+x) */
    spc_cyc(5);
    { uint16_t t = spc_rd((uint16_t)(0x23FB + spc.x)); t |= (spc_cyc(1), spc_rd((uint16_t)(0x23FB + spc.x + 1))) << 8;
      switch (t) {
      case 0x2441: goto L2441;
      case 0x25E2: goto L25E2;
      case 0x261F: goto L261F;
      case 0x262A: goto L262A;
      case 0x265C: goto L265C;
      } }
    return;
L2441:
    /* 2441 lsr a */
    spc.a = spc_lsr(spc.a);
    spc_cyc(2);
    /* 2442 mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* 2443 mov.w a,$2419+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x2419 + spc.x)));
    /* 2446 mov.b $22,a */
    spc_st_cyc(spc_dp(0x22), spc.a, 3, 1);
    /* 2448 mov.w a,$2423+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x2423 + spc.x)));
    /* 244B mov.b $23,a */
    spc_st_cyc(spc_dp(0x23), spc.a, 3, 1);
    /* 244D mov.w a,$242D+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x242D + spc.x)));
    /* 2450 mov.b $24,a */
    spc_st_cyc(spc_dp(0x24), spc.a, 3, 1);
    /* 2452 mov.w a,$2437+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x2437 + spc.x)));
    /* 2455 mov.b $25,a */
    spc_st_cyc(spc_dp(0x25), spc.a, 3, 1);
    /* 2457 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 2458 call $25A0 */
    spc_call_cyc(abi_SpcEngineVolume, 0x245B, 5, 1, 2);
    /* 245B push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 245C cmp $0B,$03 */
    spc_cyc(3);
    { uint8_t r = spc_rd(spc_dp(0x03)); spc_cmp((spc_cyc(2), spc_rd(spc_dp(0x0B))), r); }
    spc_cyc(1);
    /* 245F beq $24A9 */
    if (spc.z) { spc_cyc(4); goto L24A9; }
    spc_cyc(2);
    /* 2461 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 2463 mov.w $0539,a */
    spc_st_cyc(0x0539, spc.a, 4, 1);
    /* 2466 mov.b a,$03 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x03)));
    /* 2468 mov.w $053B,a */
    spc_st_cyc(0x053B, spc.a, 4, 1);
    /* 246B setc */
    spc.c = 1;
    spc_cyc(2);
    /* 246C sbc.b a,$0B */
    spc_cyc(3);
    spc.a = spc_sbc(spc.a, spc_rd(spc_dp(0x0B)));
    /* 246E bmi $249E */
    if (spc.n) { spc_cyc(4); goto L249E; }
    spc_cyc(2);
    /* 2470 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 2471 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 2473 mov.w $053D,a */
    spc_st_cyc(0x053D, spc.a, 4, 1);
L2476:
    /* 2476 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 2477 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 2478 mov.w x,$0533 */
    spc_cyc(4);
    spc.x = spc_nz(spc_rd(0x0533));
    /* 247B bne $247F */
    if (!spc.z) { spc_cyc(4); goto L247F; }
    spc_cyc(2);
    /* 247D mov x,#$01 */
    spc.x = spc_nz(0x01);
    spc_cyc(2);
L247F:
    /* 247F mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 2481 div ya,x */
    spc_div();
    spc_cyc(12);
    /* 2482 bvs $248E */
    if (spc.v) { spc_cyc(4); goto L248E; }
    spc_cyc(2);
    /* 2484 mov.w $0535,a */
    spc_st_cyc(0x0535, spc.a, 4, 1);
    /* 2487 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 2489 mov.w $0537,a */
    spc_st_cyc(0x0537, spc.a, 4, 1);
    /* 248C bra $2496 */
    spc_cyc(4);
    goto L2496;
L248E:
    /* 248E mov.w $0535,a */
    spc_st_cyc(0x0535, spc.a, 4, 1);
    /* 2491 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 2493 mov.w $0537,a */
    spc_st_cyc(0x0537, spc.a, 4, 1);
L2496:
    /* 2496 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 2498 mov.w $0533,a */
    spc_st_cyc(0x0533, spc.a, 4, 1);
    /* 249B jmp $254F */
    spc_cyc(3);
    goto L254F;
L249E:
    /* 249E eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    spc_cyc(2);
    /* 24A0 inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
    /* 24A1 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 24A2 mov a,#$02 */
    spc.a = spc_nz(0x02);
    spc_cyc(2);
    /* 24A4 mov.w $053D,a */
    spc_st_cyc(0x053D, spc.a, 4, 1);
    /* 24A7 bra $2476 */
    spc_cyc(4);
    goto L2476;
L24A9:
    /* 24A9 mov.b a,$03 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x03)));
    /* 24AB bne $24B7 */
    if (!spc.z) { spc_cyc(4); goto L24B7; }
    spc_cyc(2);
    /* 24AD mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 24AF mov.w $0535,a */
    spc_st_cyc(0x0535, spc.a, 4, 1);
    /* 24B2 mov.w $0536,a */
    spc_st_cyc(0x0536, spc.a, 4, 1);
    /* 24B5 bra $24DC */
    spc_cyc(4);
    goto L24DC;
L24B7:
    /* 24B7 mov.b a,$03 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x03)));
    /* 24B9 bpl $24DC */
    if (!spc.n) { spc_cyc(4); goto L24DC; }
    spc_cyc(2);
    /* 24BB setc */
    spc.c = 1;
    spc_cyc(2);
    /* 24BC sbc a,#$F0 */
    spc.a = spc_sbc(spc.a, 0xF0);
    spc_cyc(2);
    /* 24BE bmi $24DC */
    if (spc.n) { spc_cyc(4); goto L24DC; }
    spc_cyc(2);
    /* 24C0 mov.w a,$056D */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x056D));
    /* 24C3 mov.b $22,a */
    spc_st_cyc(spc_dp(0x22), spc.a, 3, 1);
    /* 24C5 mov.w a,$056F */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x056F));
    /* 24C8 mov.b $23,a */
    spc_st_cyc(spc_dp(0x23), spc.a, 3, 1);
    /* 24CA incw $22 */
    spc_incw_cyc(0x22, +1, 3, 1, 1, 1);
    /* 24CC incw $22 */
    spc_incw_cyc(0x22, +1, 3, 1, 1, 1);
    /* 24CE incw $22 */
    spc_incw_cyc(0x22, +1, 3, 1, 1, 1);
    /* 24D0 mov.b a,$22 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 24D2 mov.w $056D,a */
    spc_st_cyc(0x056D, spc.a, 4, 1);
    /* 24D5 mov.b a,$23 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x23)));
    /* 24D7 mov.w $056F,a */
    spc_st_cyc(0x056F, spc.a, 4, 1);
    /* 24DA bra $24E4 */
    spc_cyc(4);
    goto L24E4;
L24DC:
    /* 24DC mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 24DE mov.w $056D,a */
    spc_st_cyc(0x056D, spc.a, 4, 1);
    /* 24E1 mov.w $056F,a */
    spc_st_cyc(0x056F, spc.a, 4, 1);
L24E4:
    /* 24E4 mov.w a,$0533 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0533));
    /* 24E7 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 24E8 sbc a,#$0A */
    spc.a = spc_sbc(spc.a, 0x0A);
    spc_cyc(2);
    /* 24EA bmi $2506 */
    if (spc.n) { spc_cyc(4); goto L2506; }
    spc_cyc(2);
    /* 24EC mov $22,#$01 */
    spc_st_cyc(spc_dp(0x22), 0x01, 4, 1);
    /* 24EF mov $23,#$00 */
    spc_st_cyc(spc_dp(0x23), 0x00, 4, 1);
    /* 24F2 mov.w y,$0537 */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x0537));
    /* 24F5 mov.w a,$0535 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0535));
    /* 24F8 subw ya,$22 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x22)); v |= (spc_cyc(2), spc_rd(spc_dp(0x23))) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 24FA bpl $2500 */
    if (!spc.n) { spc_cyc(4); goto L2500; }
    spc_cyc(2);
    /* 24FC mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 24FE mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
L2500:
    /* 2500 mov.w $0535,a */
    spc_st_cyc(0x0535, spc.a, 4, 1);
    /* 2503 mov.w $0537,y */
    spc_st_cyc(0x0537, spc.y, 4, 1);
L2506:
    /* 2506 mov.w a,$0533 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0533));
    /* 2509 cmp a,#$FF */
    spc_cmp(spc.a, 0xFF);
    spc_cyc(2);
    /* 250B beq $2514 */
    if (spc.z) { spc_cyc(4); goto L2514; }
    spc_cyc(2);
    /* 250D mov.w a,$0533 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0533));
    /* 2510 inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
    /* 2511 mov.w $0533,a */
    spc_st_cyc(0x0533, spc.a, 4, 1);
L2514:
    /* 2514 mov.w a,$0533 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0533));
    /* 2517 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    spc_cyc(2);
    /* 2519 beq $2548 */
    if (spc.z) { spc_cyc(4); goto L2548; }
    spc_cyc(2);
    /* 251B mov.w y,$053B */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x053B));
    /* 251E mov.w a,$0539 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0539));
L2521:
    /* 2521 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 2522 mov.w a,$0535 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0535));
    /* 2525 mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 2527 mov.w a,$0537 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0537));
    /* 252A mov.b $21,a */
    spc_st_cyc(spc_dp(0x21), spc.a, 3, 1);
    /* 252C mov.w a,$053D */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x053D));
    /* 252F cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    spc_cyc(2);
    /* 2531 beq $2541 */
    if (spc.z) { spc_cyc(4); goto L2541; }
    spc_cyc(2);
    /* 2533 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 2534 addw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 2536 bcs $254F */
    if (spc.c) { spc_cyc(4); goto L254F; }
    spc_cyc(2);
L2538:
    /* 2538 mov.w $0539,a */
    spc_st_cyc(0x0539, spc.a, 4, 1);
    /* 253B mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 253C mov.w $053B,a */
    spc_st_cyc(0x053B, spc.a, 4, 1);
    /* 253F bra $254F */
    spc_cyc(4);
    goto L254F;
L2541:
    /* 2541 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 2542 subw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 2544 bcc $254F */
    if (!spc.c) { spc_cyc(4); goto L254F; }
    spc_cyc(2);
    /* 2546 bra $2538 */
    spc_cyc(4);
    goto L2538;
L2548:
    /* 2548 mov.b a,$03 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x03)));
    /* 254A mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 254B mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* 254D bra $2521 */
    spc_cyc(4);
    goto L2521;
L254F:
    /* 254F mov $20,#$00 */
    spc_st_cyc(spc_dp(0x20), 0x00, 4, 1);
    /* 2552 mov $21,$24 */
    spc_cyc(3);
    spc_wr_cyc(spc_dp(0x21), spc_rd(spc_dp(0x24)), 2);
    /* 2555 mov.b y,$25 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x25)));
    /* 2557 mov.w a,$053B */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x053B));
    /* 255A mul ya */
    spc_mul();
    spc_cyc(9);
    /* 255B mov $20,#$00 */
    spc_st_cyc(spc_dp(0x20), 0x00, 4, 1);
    /* 255E addw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 2560 mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 2562 mov.b $21,y */
    spc_st_cyc(spc_dp(0x21), spc.y, 3, 1);
    /* 2564 mov.w a,$0549 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0549));
    /* 2567 mov.w y,$054B */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x054B));
    /* 256A addw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 256C mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 256E mov.b $21,y */
    spc_st_cyc(spc_dp(0x21), spc.y, 3, 1);
    /* 2570 mov.w a,$054D */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x054D));
    /* 2573 mov.w y,$054F */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x054F));
    /* 2576 addw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 2578 mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 257A mov.b $21,y */
    spc_st_cyc(spc_dp(0x21), spc.y, 3, 1);
    /* 257C mov.w a,$056D */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x056D));
    /* 257F mov.w y,$056F */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x056F));
    /* 2582 addw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 2584 mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 2586 mov.b $21,y */
    spc_st_cyc(spc_dp(0x21), spc.y, 3, 1);
    /* 2588 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 2589 sbc $21,#$3F */
    spc_cyc(4);
    spc_wr_cyc(spc_dp(0x21), spc_sbc(spc_rd(spc_dp(0x21)), 0x3F), 1);
    /* 258C bmi $2591 */
    if (spc.n) { spc_cyc(4); goto L2591; }
    spc_cyc(2);
    /* 258E mov $21,#$3F */
    spc_st_cyc(spc_dp(0x21), 0x3F, 4, 1);
L2591:
    /* 2591 mov $0B,$03 */
    spc_cyc(3);
    spc_wr_cyc(spc_dp(0x0B), spc_rd(spc_dp(0x03)), 2);
    /* 2594 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 2595 mov.b a,$20 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 2597 mov.w $03B8+x,a */
    spc_st_cyc((uint16_t)(0x03B8 + spc.x), spc.a, 5, 1);
    /* 259A mov.b a,$21 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x21)));
    /* 259C mov.w $03C0+x,a */
    spc_st_cyc((uint16_t)(0x03C0 + spc.x), spc.a, 5, 1);
    /* 259F ret */
    spc_ret_cyc(4, 1);
    return;
L25E2:
    /* 25E2 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 25E3 mov $22,#$28 */
    spc_st_cyc(spc_dp(0x22), 0x28, 4, 1);
    /* 25E6 mov $23,#$03 */
    spc_st_cyc(spc_dp(0x23), 0x03, 4, 1);
    /* 25E9 mov $24,#$02 */
    spc_st_cyc(spc_dp(0x24), 0x02, 4, 1);
    /* 25EC call $2634 */
    spc_call_cyc(abi_SpcEngineRevTimer, 0x25EF, 5, 1, 2);
    /* 25EF mov.w a,$03E8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03E8 + spc.x)));
    /* 25F2 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    spc_cyc(2);
    /* 25F4 beq $25FB */
    if (spc.z) { spc_cyc(4); goto L25FB; }
    spc_cyc(2);
L25F6:
    /* 25F6 bbs $28.6,$2614 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x28)) >> 6 & 1)) { spc_cyc(4); goto L2614; }
    spc_cyc(2);
    /* 25F9 bra $2609 */
    spc_cyc(4);
    goto L2609;
L25FB:
    /* 25FB call $2600 */
    spc_call_cyc(abi_SpcEngineToggle, 0x25FE, 5, 1, 2);
    /* 25FE bra $25F6 */
    spc_cyc(4);
    goto L25F6;
L2609:
    /* 2609 mov a,#$80 */
    spc.a = spc_nz(0x80);
    spc_cyc(2);
    /* 260B mov.w $03B8+x,a */
    spc_st_cyc((uint16_t)(0x03B8 + spc.x), spc.a, 5, 1);
    /* 260E mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 2610 mov.w $03C0+x,a */
    spc_st_cyc((uint16_t)(0x03C0 + spc.x), spc.a, 5, 1);
    /* 2613 ret */
    spc_ret_cyc(4, 1);
    return;
L2614:
    /* 2614 mov a,#$10 */
    spc.a = spc_nz(0x10);
    spc_cyc(2);
    /* 2616 mov.w $03B8+x,a */
    spc_st_cyc((uint16_t)(0x03B8 + spc.x), spc.a, 5, 1);
    /* 2619 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 261B mov.w $03C0+x,a */
    spc_st_cyc((uint16_t)(0x03C0 + spc.x), spc.a, 5, 1);
    /* 261E ret */
    spc_ret_cyc(4, 1);
    return;
L261F:
    /* 261F mov $22,#$1E */
    spc_st_cyc(spc_dp(0x22), 0x1E, 4, 1);
    /* 2622 mov $23,#$03 */
    spc_st_cyc(spc_dp(0x23), 0x03, 4, 1);
    /* 2625 mov $24,#$01 */
    spc_st_cyc(spc_dp(0x24), 0x01, 4, 1);
    /* 2628 bra $2633 */
    spc_cyc(4);
    goto L2633;
L262A:
    /* 262A mov $22,#$14 */
    spc_st_cyc(spc_dp(0x22), 0x14, 4, 1);
    /* 262D mov $23,#$0A */
    spc_st_cyc(spc_dp(0x23), 0x0A, 4, 1);
    /* 2630 mov $24,#$02 */
    spc_st_cyc(spc_dp(0x24), 0x02, 4, 1);
L2633:
    /* 2633 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    { abi_SpcEngineRevTimer(); return; }
L265C:
    /* 265C pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 265D mov.b a,$02 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x02)));
    /* 265F and a,#$07 */
    spc.a = spc_and(spc.a, 0x07);
    spc_cyc(2);
    /* 2661 bbs $02.3,$2666 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x02)) >> 3 & 1)) { spc_cyc(4); goto L2666; }
    spc_cyc(2);
    /* 2664 eor a,#$07 */
    spc.a = spc_eor(spc.a, 0x07);
    spc_cyc(2);
L2666:
    /* 2666 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 2667 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 2668 mov.w a,$EE20+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0xEE20 + spc.y)));
    /* 266B mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 266D inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 266E mov.w a,$EE20+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0xEE20 + spc.y)));
    /* 2671 mov.b $21,a */
    spc_st_cyc(spc_dp(0x21), spc.a, 3, 1);
    /* 2673 mov.w a,$04A0+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x04A0 + spc.x)));
    /* 2676 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 2677 mov.w a,$0498+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0498 + spc.x)));
    /* 267A bbs $02.3,$2681 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x02)) >> 3 & 1)) { spc_cyc(4); goto L2681; }
    spc_cyc(2);
    /* 267D subw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 267F bra $2683 */
    spc_cyc(4);
    goto L2683;
L2681:
    /* 2681 addw ya,$20 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= (spc_cyc(2), spc_rd(spc_dp(0x21))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
L2683:
    /* 2683 mov.w $03B8+x,a */
    spc_st_cyc((uint16_t)(0x03B8 + spc.x), spc.a, 5, 1);
    /* 2686 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 2687 mov.w $03C0+x,a */
    spc_st_cyc((uint16_t)(0x03C0 + spc.x), spc.a, 5, 1);
    /* 268A mov.b a,$02 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x02)));
    /* 268C and a,#$F0 */
    spc.a = spc_and(spc.a, 0xF0);
    spc_cyc(2);
    /* 268E xcn a */
    spc.a = spc_xcn(spc.a);
    spc_cyc(5);
    /* 268F bbs $02.7,$26A8 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x02)) >> 7 & 1)) { spc_cyc(4); goto L26A8; }
    spc_cyc(2);
    /* 2692 asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 2693 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 2694 mov.w a,$EE30+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0xEE30 + spc.y)));
    /* 2697 call $26C0 */
    spc_call_cyc(abi_SpcSfxPanScale, 0x269A, 5, 1, 2);
    /* 269A mov.w $0398+x,a */
    spc_st_cyc((uint16_t)(0x0398 + spc.x), spc.a, 5, 1);
    /* 269D inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 269E mov.w a,$EE30+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0xEE30 + spc.y)));
    /* 26A1 call $26C0 */
    spc_call_cyc(abi_SpcSfxPanScale, 0x26A4, 5, 1, 2);
    /* 26A4 mov.w $03A0+x,a */
    spc_st_cyc((uint16_t)(0x03A0 + spc.x), spc.a, 5, 1);
    /* 26A7 ret */
    spc_ret_cyc(4, 1);
    return;
L26A8:
    /* 26A8 eor a,#$0F */
    spc.a = spc_eor(spc.a, 0x0F);
    spc_cyc(2);
    /* 26AA asl a */
    spc.a = spc_asl(spc.a);
    spc_cyc(2);
    /* 26AB mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* 26AC mov.w a,$EE30+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0xEE30 + spc.y)));
    /* 26AF call $26C0 */
    spc_call_cyc(abi_SpcSfxPanScale, 0x26B2, 5, 1, 2);
    /* 26B2 mov.w $03A0+x,a */
    spc_st_cyc((uint16_t)(0x03A0 + spc.x), spc.a, 5, 1);
    /* 26B5 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 26B6 mov.w a,$EE30+y */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0xEE30 + spc.y)));
    /* 26B9 call $26C0 */
    spc_call_cyc(abi_SpcSfxPanScale, 0x26BC, 5, 1, 2);
    /* 26BC mov.w $0398+x,a */
    spc_st_cyc((uint16_t)(0x0398 + spc.x), spc.a, 5, 1);
    /* 26BF ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:E22A SpcEngineVolume
 * WHAT IT DOES: The engine sound's volume from port 3: (inverted speed) / $22 + $23, scaled by $0552 in echo mode A, plus 3 with bit 2 of $29, by $D0 unless bit 2 of $28 (SpcEngineDamp), by $056B, by $C0 with bit 4 of $2D; both sides' volume set.
 * HOW: DIV and MULs keeping high bytes.
 * Entry: X = channel, $22/$23 = curve. Exit: A = volume.
 * @spc 25A0
 * @spcuses 25A0-25E2
 */
void abi_SpcEngineVolume(void)
{
    /* 25A0 mov.b a,$03 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x03)));
    /* 25A2 eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    spc_cyc(2);
    /* 25A4 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 25A6 push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 25A7 mov.b x,$22 */
    spc_cyc(3);
    spc.x = spc_nz(spc_rd(spc_dp(0x22)));
    /* 25A9 div ya,x */
    spc_div();
    spc_cyc(12);
    /* 25AA mov.b $20,a */
    spc_st_cyc(spc_dp(0x20), spc.a, 3, 1);
    /* 25AC pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 25AD mov.b a,$23 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x23)));
    /* 25AF clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 25B0 adc.b a,$20 */
    spc_cyc(3);
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0x20)));
    /* 25B2 bbc $28.0,$25BB */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x28)) >> 0 & 1)) { spc_cyc(4); goto L25BB; }
    spc_cyc(2);
    /* 25B5 mov.w y,$0552 */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x0552));
    /* 25B8 dec y */
    spc.y = spc_dec(spc.y);
    spc_cyc(2);
    /* 25B9 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 25BA mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
L25BB:
    /* 25BB bbc $29.2,$25C0 */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x29)) >> 2 & 1)) { spc_cyc(4); goto L25C0; }
    spc_cyc(2);
    /* 25BE adc a,#$03 */
    spc.a = spc_adc(spc.a, 0x03);
    spc_cyc(2);
L25C0:
    /* 25C0 call $25D8 */
    spc_call_cyc(abi_SpcEngineDamp, 0x25C3, 5, 1, 2);
    /* 25C3 mov.w y,$056B */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x056B));
    /* 25C6 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 25C7 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 25C8 bbc $2D.4,$25D1 */
    spc_cyc(3);
    if (!(spc_rd(spc_dp(0x2D)) >> 4 & 1)) { spc_cyc(4); goto L25D1; }
    spc_cyc(2);
    /* 25CB push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 25CC mov y,#$C0 */
    spc.y = spc_nz(0xC0);
    spc_cyc(2);
    /* 25CE mul ya */
    spc_mul();
    spc_cyc(9);
    /* 25CF mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 25D0 pop x */
    spc_cyc(4);
    spc.x = spc_pull();
L25D1:
    /* 25D1 mov.w $0398+x,a */
    spc_st_cyc((uint16_t)(0x0398 + spc.x), spc.a, 5, 1);
    /* 25D4 mov.w $03A0+x,a */
    spc_st_cyc((uint16_t)(0x03A0 + spc.x), spc.a, 5, 1);
    /* 25D7 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:E262 SpcEngineDamp
 * WHAT IT DOES: Unless bit 2 of $28 is set, A = A * $D0 / 256.
 * HOW: MUL.
 * Entry: A. Exit: A; Y kept.
 * @spc 25D8
 * @spcuses 25D8-25E2
 */
void abi_SpcEngineDamp(void)
{
    /* 25D8 bbs $28.2,$25E1 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x28)) >> 2 & 1)) { spc_cyc(4); goto L25E1; }
    spc_cyc(2);
    /* 25DB push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 25DC mov y,#$D0 */
    spc.y = spc_nz(0xD0);
    spc_cyc(2);
    /* 25DE mul ya */
    spc_mul();
    spc_cyc(9);
    /* 25DF mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* 25E0 pop y */
    spc_cyc(4);
    spc.y = spc_pull();
L25E1:
    /* 25E1 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:E28A SpcEngineToggle
 * WHAT IT DOES: Toggles bit 6 of $28.
 * HOW: BBS, SET1/CLR1.
 * Entry: none. Exit: none.
 * @spc 2600
 * @spcuses 2600-2609
 */
void abi_SpcEngineToggle(void)
{
    /* 2600 bbs $28.6,$2606 */
    spc_cyc(3);
    if ((spc_rd(spc_dp(0x28)) >> 6 & 1)) { spc_cyc(4); goto L2606; }
    spc_cyc(2);
    /* 2603 set1 $28.6 */
    spc_bit_set_cyc(0x28, 6, 1, 3, 1);
    /* 2605 ret */
    spc_ret_cyc(4, 1);
    return;
L2606:
    /* 2606 clr1 $28.6 */
    spc_bit_set_cyc(0x28, 6, 0, 3, 1);
    /* 2608 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:E2BE SpcEngineRevTimer
 * WHAT IT DOES: The rev pulse: the counter $0545 counts up to (inverted speed) / $22 + $23 and then restarts with the channel's mode $03E8 = $24.
 * HOW: DIV, compare.
 * Entry: X = channel, $22-$24 = curve. Exit: A changed.
 * @spc 2634
 * @spcuses 2634-265C
 */
void abi_SpcEngineRevTimer(void)
{
    /* 2634 mov.b a,$03 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x03)));
    /* 2636 eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    spc_cyc(2);
    /* 2638 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* 263A push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 263B mov.b x,$22 */
    spc_cyc(3);
    spc.x = spc_nz(spc_rd(spc_dp(0x22)));
    /* 263D div ya,x */
    spc_div();
    spc_cyc(12);
    /* 263E pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 263F clrc */
    spc.c = 0;
    spc_cyc(2);
    /* 2640 adc.b a,$23 */
    spc_cyc(3);
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0x23)));
    /* 2642 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* 2643 mov.w a,$0545 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0545));
    /* 2646 inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
    /* 2647 mov.w $0545,a */
    spc_st_cyc(0x0545, spc.a, 4, 1);
    /* 264A pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* 264B setc */
    spc.c = 1;
    spc_cyc(2);
    /* 264C sbc.w a,$0545 */
    spc_cyc(4);
    spc.a = spc_sbc(spc.a, spc_rd(0x0545));
    /* 264F bpl $265B */
    if (!spc.n) { spc_cyc(4); goto L265B; }
    spc_cyc(2);
    /* 2651 mov.b a,$24 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x24)));
    /* 2653 mov.w $03E8+x,a */
    spc_st_cyc((uint16_t)(0x03E8 + spc.x), spc.a, 5, 1);
    /* 2656 mov a,#$01 */
    spc.a = spc_nz(0x01);
    spc_cyc(2);
    /* 2658 mov.w $0545,a */
    spc_st_cyc(0x0545, spc.a, 4, 1);
L265B:
    /* 265B ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:E34A SpcSfxPanScale
 * WHAT IT DOES: A pan table value A scaled by $0562 + 1 and divided by $15, less the low four bits of port 2.
 * HOW: MUL, DIV.
 * Entry: A = table value. Exit: A; X, Y kept.
 * @spc 26C0
 * @spcuses 26C0-26D6
 */
void abi_SpcSfxPanScale(void)
{
    /* 26C0 push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* 26C1 push x */
    spc_cyc(3);
    spc_push(spc.x);
    spc_cyc(1);
    /* 26C2 mov.w y,$0562 */
    spc_cyc(4);
    spc.y = spc_nz(spc_rd(0x0562));
    /* 26C5 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* 26C6 mul ya */
    spc_mul();
    spc_cyc(9);
    /* 26C7 mov x,#$15 */
    spc.x = spc_nz(0x15);
    spc_cyc(2);
    /* 26C9 div ya,x */
    spc_div();
    spc_cyc(12);
    /* 26CA pop x */
    spc_cyc(4);
    spc.x = spc_pull();
    /* 26CB pop y */
    spc_cyc(4);
    spc.y = spc_pull();
    /* 26CC mov $22,$02 */
    spc_cyc(3);
    spc_wr_cyc(spc_dp(0x22), spc_rd(spc_dp(0x02)), 2);
    /* 26CF and $22,#$0F */
    spc_cyc(4);
    spc_wr_cyc(spc_dp(0x22), spc_and(spc_rd(spc_dp(0x22)), 0x0F), 1);
    /* 26D2 setc */
    spc.c = 1;
    spc_cyc(2);
    /* 26D3 sbc.b a,$22 */
    spc_cyc(3);
    spc.a = spc_sbc(spc.a, spc_rd(spc_dp(0x22)));
    /* 26D5 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:F1C3 SpcMul16
 * WHAT IT DOES: A times the 16-bit value X:Y (X the high byte): YA = the result's middle 16 bits plus A * Y, as the sound code's fixed-point multiply.
 * HOW: Two MULs and ADDW through $26/$27.
 * Entry: A, X, Y. Exit: YA, X.
 * @spc CA75
 * @spcuses CA75-CA89
 */
void abi_SpcMul16(void)
{
    /* CA75 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* CA76 mul ya */
    spc_mul();
    spc_cyc(9);
    /* CA77 mov.b $26,a */
    spc_st_cyc(spc_dp(0x26), spc.a, 3, 1);
    /* CA79 mov.b $27,y */
    spc_st_cyc(spc_dp(0x27), spc.y, 3, 1);
    /* CA7B mov a,x */
    spc.a = spc_nz(spc.x);
    spc_cyc(2);
    /* CA7C mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* CA7D pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* CA7E mul ya */
    spc_mul();
    spc_cyc(9);
    /* CA7F push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* CA80 mov a,y */
    spc.a = spc_nz(spc.y);
    spc_cyc(2);
    /* CA81 mov x,a */
    spc.x = spc_nz(spc.a);
    spc_cyc(2);
    /* CA82 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* CA83 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* CA84 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* CA86 addw ya,$26 */
    spc_cyc(3);
    { uint16_t v = spc_rd(spc_dp(0x26)); v |= (spc_cyc(2), spc_rd(spc_dp(0x27))) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* CA88 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:F1D7 SpcDiv16
 * WHAT IT DOES: The 16-bit value Y:A divided by X: YA = the quotient.
 * HOW: Two DIVs, the high quotient through $27.
 * Entry: Y:A, X. Exit: YA = quotient.
 * @spc CA89
 * @spcuses CA89-CA96
 */
void abi_SpcDiv16(void)
{
    /* CA89 push a */
    spc_cyc(3);
    spc_push(spc.a);
    spc_cyc(1);
    /* CA8A push y */
    spc_cyc(3);
    spc_push(spc.y);
    spc_cyc(1);
    /* CA8B mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* CA8D pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* CA8E div ya,x */
    spc_div();
    spc_cyc(12);
    /* CA8F mov.b $27,a */
    spc_st_cyc(spc_dp(0x27), spc.a, 3, 1);
    /* CA91 pop a */
    spc_cyc(4);
    spc.a = spc_pull();
    /* CA92 div ya,x */
    spc_div();
    spc_cyc(12);
    /* CA93 mov.b y,$27 */
    spc_cyc(3);
    spc.y = spc_nz(spc_rd(spc_dp(0x27)));
    /* CA95 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:F1E4 SpcIndexRight
 * WHAT IT DOES: X + 8 (a channel's right-side index).
 * HOW: Eight INC X.
 * Entry: X. Exit: X + 8.
 * @spc CA96
 * @spcuses CA96-CA9F
 */
void abi_SpcIndexRight(void)
{
    /* CA96 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* CA97 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* CA98 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* CA99 inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* CA9A inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* CA9B inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* CA9C inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* CA9D inc x */
    spc.x = spc_inc(spc.x);
    spc_cyc(2);
    /* CA9E ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:F1ED SpcIndexLeft
 * WHAT IT DOES: X - 8.
 * HOW: Eight DEC X.
 * Entry: X. Exit: X - 8.
 * @spc CA9F
 * @spcuses CA9F-CAA8
 */
void abi_SpcIndexLeft(void)
{
    /* CA9F dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* CAA0 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* CAA1 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* CAA2 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* CAA3 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* CAA4 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* CAA5 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* CAA6 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* CAA7 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:F20E SpcNoiseSample
 * WHAT IT DOES: While a channel plays sound $23, rewrites part of the sample at $FF00 each tick with noise from the LFSR $18/$19: seven blocks of eight bytes, the block chosen by the counter $0563.
 * HOW: MOV1/EOR1 of two LFSR bits, ROL; bytes ORed with $11.
 * Entry: none. Exit: X, Y changed.
 * @spc CAC0
 * @spcuses CAC0-CB04
 */
void abi_SpcNoiseSample(void)
{
    /* CAC0 mov x,#$07 */
    spc.x = spc_nz(0x07);
    spc_cyc(2);
LCAC2:
    /* CAC2 mov.w a,$03C8+x */
    spc_cyc(5);
    spc.a = spc_nz(spc_rd((uint16_t)(0x03C8 + spc.x)));
    /* CAC5 cmp a,#$23 */
    spc_cmp(spc.a, 0x23);
    spc_cyc(2);
    /* CAC7 beq $CACD */
    if (spc.z) { spc_cyc(4); goto LCACD; }
    spc_cyc(2);
    /* CAC9 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* CACA bne $CAC2 */
    if (!spc.z) { spc_cyc(4); goto LCAC2; }
    spc_cyc(2);
    /* CACC ret */
    spc_ret_cyc(4, 1);
    return;
LCACD:
    /* CACD dec.w $0563 */
    spc_rmw_cyc(0x0563, spc_dec, 4, 1);
    /* CAD0 mov.w a,$0563 */
    spc_cyc(4);
    spc.a = spc_nz(spc_rd(0x0563));
    /* CAD3 and a,#$03 */
    spc.a = spc_and(spc.a, 0x03);
    spc_cyc(2);
    /* CAD5 mov y,#$3F */
    spc.y = spc_nz(0x3F);
    spc_cyc(2);
    /* CAD7 mul ya */
    spc_mul();
    spc_cyc(9);
    /* CAD8 mov y,a */
    spc.y = spc_nz(spc.a);
    spc_cyc(2);
    /* CAD9 mov $20,#$07 */
    spc_st_cyc(spc_dp(0x20), 0x07, 4, 1);
LCADC:
    /* CADC inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* CADD mov x,#$04 */
    spc.x = spc_nz(0x04);
    spc_cyc(2);
LCADF:
    /* CADF mov1 c,$0019.6 */
    spc_cyc(4);
    spc.c = spc_rd(0x0019) >> 6 & 1;
    /* CAE2 eor1 c,$0019.5 */
    spc_cyc(4);
    spc.c ^= spc_rd(0x0019) >> 5 & 1;
    spc_cyc(1);
    /* CAE5 rol.b $18 */
    spc_rmw_cyc(spc_dp(0x18), spc_rol, 3, 1);
    /* CAE7 rol.b $19 */
    spc_rmw_cyc(spc_dp(0x19), spc_rol, 3, 1);
    /* CAE9 mov.b a,$19 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x19)));
    /* CAEB and a,#$D0 */
    spc.a = spc_and(spc.a, 0xD0);
    spc_cyc(2);
    /* CAED or a,#$11 */
    spc.a = spc_or(spc.a, 0x11);
    spc_cyc(2);
    /* CAEF mov.w $FF00+y,a */
    spc_st_cyc((uint16_t)(0xFF00 + spc.y), spc.a, 5, 1);
    /* CAF2 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* CAF3 mov.b a,$18 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x18)));
    /* CAF5 and a,#$FF */
    spc.a = spc_and(spc.a, 0xFF);
    spc_cyc(2);
    /* CAF7 or a,#$11 */
    spc.a = spc_or(spc.a, 0x11);
    spc_cyc(2);
    /* CAF9 mov.w $FF00+y,a */
    spc_st_cyc((uint16_t)(0xFF00 + spc.y), spc.a, 5, 1);
    /* CAFC inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* CAFD dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* CAFE bne $CADF */
    if (!spc.z) { spc_cyc(4); goto LCADF; }
    spc_cyc(2);
    /* CB00 dbnz $20,$CADC */
    spc_cyc(3);
    { uint8_t v = (uint8_t)(spc_rd(spc_dp(0x20)) - 1); spc_cyc(1); spc_wr(spc_dp(0x20), v); if (v) { spc_cyc(3); goto LCADC; } }
    spc_cyc(1);
    /* CB03 ret */
    spc_ret_cyc(4, 1);
    return;
}

/*
 * @routine 19:F40D SpcNoiseSampleInit
 * WHAT IT DOES: Builds the sample at $FF00: every ninth byte (the BRR block headers) set to A for $1B blocks, the last A + 1 (the end flag); the sample directory's entry at $3F8C-$3F8F points start and loop at $FF00; the LFSR made nonzero.
 * HOW: Y stepping by 9.
 * Entry: A = header byte. Exit: A, Y changed.
 * @spc EDF0
 * @spcuses EDF0-EE20
 */
void abi_SpcNoiseSampleInit(void)
{
    /* EDF0 mov y,#$00 */
    spc.y = spc_nz(0x00);
    spc_cyc(2);
    /* EDF2 mov x,#$1B */
    spc.x = spc_nz(0x1B);
    spc_cyc(2);
LEDF4:
    /* EDF4 mov.w $FF00+y,a */
    spc_st_cyc((uint16_t)(0xFF00 + spc.y), spc.a, 5, 1);
    /* EDF7 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* EDF8 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* EDF9 inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* EDFA inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* EDFB inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* EDFC inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* EDFD inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* EDFE inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* EDFF inc y */
    spc.y = spc_inc(spc.y);
    spc_cyc(2);
    /* EE00 dec x */
    spc.x = spc_dec(spc.x);
    spc_cyc(2);
    /* EE01 bne $EDF4 */
    if (!spc.z) { spc_cyc(4); goto LEDF4; }
    spc_cyc(2);
    /* EE03 inc a */
    spc.a = spc_inc(spc.a);
    spc_cyc(2);
    /* EE04 mov.w $FF00+y,a */
    spc_st_cyc((uint16_t)(0xFF00 + spc.y), spc.a, 5, 1);
    /* EE07 mov y,#$FF */
    spc.y = spc_nz(0xFF);
    spc_cyc(2);
    /* EE09 mov a,#$00 */
    spc.a = spc_nz(0x00);
    spc_cyc(2);
    /* EE0B mov.w $3F8C,a */
    spc_st_cyc(0x3F8C, spc.a, 4, 1);
    /* EE0E mov.w $3F8D,y */
    spc_st_cyc(0x3F8D, spc.y, 4, 1);
    /* EE11 mov.w $3F8E,a */
    spc_st_cyc(0x3F8E, spc.a, 4, 1);
    /* EE14 mov.w $3F8F,y */
    spc_st_cyc(0x3F8F, spc.y, 4, 1);
    /* EE17 mov.b a,$18 */
    spc_cyc(3);
    spc.a = spc_nz(spc_rd(spc_dp(0x18)));
    /* EE19 or.b a,$19 */
    spc_cyc(3);
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x19)));
    /* EE1B bne $EE1F */
    if (!spc.z) { spc_cyc(4); goto LEE1F; }
    spc_cyc(2);
    /* EE1D inc.b $18 */
    spc_rmw_cyc(spc_dp(0x18), spc_inc, 3, 1);
LEE1F:
    /* EE1F ret */
    spc_ret_cyc(4, 1);
    return;
}
