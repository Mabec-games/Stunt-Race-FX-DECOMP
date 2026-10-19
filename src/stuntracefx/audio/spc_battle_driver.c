/*
 * spc_battle_driver.c: the battle sound driver (N-SPC), the SPC700 code
 * uploaded from the block list at $1A:F091 over the race driver's
 * $10B0-$260E, as C on the SPC700 model (spc.h): each access in the
 * original order. Calls to the race driver's routines below it are to
 * their C (spc_race_driver.c).
 */
#include "spc.h"
#include "spc_ops.h"

void abi_SpcReload(void);

void abi_SpcBSfxFrame(void);
void abi_SpcBPortRemap(void);
void abi_SpcBChannelGuards(void);
void abi_SpcBStaggeredChecks(void);
void abi_SpcBGameStateTick(void);
void abi_SpcBCommand4C(void);
void abi_SpcBSoundQueuePush(void);
void abi_SpcBCommandTimers(void);
void abi_SpcBTempoBend(void);
void abi_SpcBTempoBendStart(void);
void abi_SpcBPort1Clamp(void);
void abi_SpcBCommandFilter(void);
void abi_SpcBCommandBlock(void);
void abi_SpcBPitchRampB(void);
void abi_SpcBPitchRampB1Up(void);
void abi_SpcBPitchRampB2Up(void);
void abi_SpcBPitchRampB1Down(void);
void abi_SpcBPitchRampB2Down(void);
void abi_SpcBPitchRampA(void);
void abi_SpcBPitchRampA1Up(void);
void abi_SpcBPitchRampA1Down(void);
void abi_SpcBPitchRampA1End(void);
void abi_SpcBPitchRampA2Up(void);
void abi_SpcBPitchRampA2Down(void);
void abi_SpcBPortCommand(void);
void abi_SpcBPort1Command(void);
void abi_SpcBSfxStop(void);
void abi_SpcBSfxChannelResume(void);
void abi_SpcBSfxPointer(void);
void abi_SpcBSfxChannelPause(void);
void abi_SpcBSfxStopAll(void);
void abi_SpcBSfxStopRequest(void);
void abi_SpcBFadeOutStart(void);
void abi_SpcBMusicFade(void);
void abi_SpcBKeyOffAll(void);
void abi_SpcBCommandChannelCheck(void);
void abi_SpcBSfxActivate(void);
void abi_SpcBSfxLookup(void);
void abi_SpcBSfxChannel(void);
void abi_SpcBSfxMode(void);
void abi_SpcBSfxSide(void);
void abi_SpcBSfxChannelsFrame(void);
void abi_SpcBSfxChannelLoad(void);
void abi_SpcBSfxChannelSave(void);
void abi_SpcBSfxChannelStep(void);
void abi_SpcBSfxMarkVoice(void);
void abi_SpcBVoiceMask(void);
void abi_SpcBVoiceMaskNot(void);
void abi_SpcBKeyOnVoice(void);
void abi_SpcBKeyOffVoice(void);
void abi_SpcBSfxRead(void);
void abi_SpcBSfxEngineScale(void);
void abi_SpcBSfxStart(void);
void abi_SpcBSfxFadeVolume(void);
void abi_SpcBIndexRight(void);
void abi_SpcBIndexLeft(void);
void abi_SpcBMul16(void);
void abi_SpcBDiv16(void);
void abi_SpcBSfxVolumeAdjust(void);
void abi_SpcBSfxSpecial(void);
void abi_SpcBEngineVolume(void);
void abi_SpcBEngineDamp2(void);
void abi_SpcBEngineDamp(void);
void abi_SpcBNoiseSampleInit(void);
void abi_SpcBNoiseSample(void);
void abi_SpcDspWrite(void);
void abi_SpcLoadInstrument(void);
void abi_SpcMusicPause(void);
void abi_SpcVoiceVolumeWrite(void);
void abi_SpcBSfxQueueStart(void);
void abi_SpcBSfxStopMatchOld(void);
void abi_SpcBSfxReadNoAge(void);
void abi_SpcBSfxReadVolumeSlides(void);
void abi_SpcBSfxVolumeSlide(void);

/*
 * @routine 1A:F095 SpcBSfxFrame
 * WHAT IT DOES: The battle driver's sound tick, the race one's (SpcSfxFrame) with the battle's game rules: a new port 0 byte becomes the command $00 (else $00 = 0); ports 1-3 copied to $01-$03 and $05-$07; a stop request (bit 1 of $29) clears bits 0 and 1; then the command remap, music fade, port command, channel guards, command block, command channel check, tempo bend, staggered checks, game state rules, pitch ramps, command filter, command $4C, timers, noise sample, port 1 clamp and port 1 command, and the sound effect channels (through the queue start at $1A7F). The race driver's also has the echo transition and the held sounds.
 * HOW: A chain of CALLs, then a JMP to the queue start.
 * Entry: none (called from SpcReset's loop, in place of the race driver's routine at the same address). Exit: as the sound effect channels leave it.
 * @spc 10B0
 * @spcuses 09E0-09EF,0A3B-0A3C,0A58-0A6D,0BC1-0C17,0E97-0EE4,10B0-1251,1256-148F,14A6-14B8,14D1-1A7D,1A7F-1C60,1C6D-1DFD,1E00-217B,21C8-222B,2243-233A,2341-234D,235D-25FF
 */
void abi_SpcBSfxFrame(void)
{
    /* 10B0 mov.w a,$0558 */
    spc.a = spc_nz(spc_rd(0x0558));
    /* 10B3 cmp.b a,$04 */
    spc_cmp(spc.a, spc_rd(spc_dp(0x04)));
    /* 10B5 beq $10C0 */
    if (spc.z) goto L10C0;
    /* 10B7 mov.w a,$0558 */
    spc.a = spc_nz(spc_rd(0x0558));
    /* 10BA mov.b $04,a */
    spc_st(spc_dp(0x04), spc.a);
    /* 10BC mov.b $00,a */
    spc_st(spc_dp(0x00), spc.a);
    /* 10BE bra $10C3 */
    goto L10C3;
L10C0:
    /* 10C0 mov $00,#$00 */
    spc_st(spc_dp(0x00), 0x00);
L10C3:
    /* 10C3 mov.w a,$0559 */
    spc.a = spc_nz(spc_rd(0x0559));
    /* 10C6 mov.b $05,a */
    spc_st(spc_dp(0x05), spc.a);
    /* 10C8 mov.b $01,a */
    spc_st(spc_dp(0x01), spc.a);
    /* 10CA mov.w a,$055A */
    spc.a = spc_nz(spc_rd(0x055A));
    /* 10CD mov.b $06,a */
    spc_st(spc_dp(0x06), spc.a);
    /* 10CF mov.b $02,a */
    spc_st(spc_dp(0x02), spc.a);
    /* 10D1 mov.w a,$055B */
    spc.a = spc_nz(spc_rd(0x055B));
    /* 10D4 mov.b $07,a */
    spc_st(spc_dp(0x07), spc.a);
    /* 10D6 mov.b $03,a */
    spc_st(spc_dp(0x03), spc.a);
    /* 10D8 bbc $29.1,$10DF */
    if (!(spc_rd(spc_dp(0x29)) >> 1 & 1)) goto L10DF;
    /* 10DB clr1 $29.0 */
    spc_bit_set(0x29, 0, 0);
    /* 10DD clr1 $29.1 */
    spc_bit_set(0x29, 1, 0);
L10DF:
    /* 10DF call $1115 */
    spc_call(abi_SpcBPortRemap, 0x10E2);
    /* 10E2 call $19BC */
    spc_call(abi_SpcBMusicFade, 0x10E5);
    /* 10E5 call $1637 */
    spc_call(abi_SpcBPortCommand, 0x10E8);
    /* 10E8 call $111E */
    spc_call(abi_SpcBChannelGuards, 0x10EB);
    /* 10EB call $14A6 */
    spc_call(abi_SpcBCommandBlock, 0x10EE);
    /* 10EE call $1A2F */
    spc_call(abi_SpcBCommandChannelCheck, 0x10F1);
    /* 10F1 call $13C6 */
    spc_call(abi_SpcBTempoBend, 0x10F4);
    /* 10F4 call $1163 */
    spc_call(abi_SpcBStaggeredChecks, 0x10F7);
    /* 10F7 call $11ED */
    spc_call(abi_SpcBGameStateTick, 0x10FA);
    /* 10FA call $14D1 */
    spc_call(abi_SpcBPitchRampB, 0x10FD);
    /* 10FD call $1584 */
    spc_call(abi_SpcBPitchRampA, 0x1100);
    /* 1100 call $1477 */
    spc_call(abi_SpcBCommandFilter, 0x1103);
    /* 1103 call $12D2 */
    spc_call(abi_SpcBCommand4C, 0x1106);
    /* 1106 call $130C */
    spc_call(abi_SpcBCommandTimers, 0x1109);
    /* 1109 call $25B8 */
    spc_call(abi_SpcBNoiseSample, 0x110C);
    /* 110C call $1467 */
    spc_call(abi_SpcBPort1Clamp, 0x110F);
    /* 110F call $1671 */
    spc_call(abi_SpcBPort1Command, 0x1112);
    /* 1112 jmp $1A7F */
    goto L1A7F;
L1A7F:
    /* 1A7F cmp $1D,#$00 */
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x00);
    /* 1A82 beq $1A8B */
    if (spc.z) goto L1A8B;
    /* 1A84 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1A86 mov.w $057B,a */
    spc_st(0x057B, spc.a);
    /* 1A89 bra $1A95 */
    goto L1A95;
L1A8B:
    /* 1A8B mov.w a,$057B */
    spc.a = spc_nz(spc_rd(0x057B));
    /* 1A8E cmp a,#$FF */
    spc_cmp(spc.a, 0xFF);
    /* 1A90 beq $1A95 */
    if (spc.z) goto L1A95;
    /* 1A92 inc.w $057B */
    spc_rmw(0x057B, spc_inc);
L1A95:
    /* 1A95 cmp $1D,#$00 */
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x00);
    /* 1A98 bne $1ACA */
    if (!spc.z) goto L1ACA;
    /* 1A9A mov.w a,$057B */
    spc.a = spc_nz(spc_rd(0x057B));
    /* 1A9D beq $1ACA */
    if (spc.z) goto L1ACA;
    /* 1A9F cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    /* 1AA1 beq $1ACA */
    if (spc.z) goto L1ACA;
    /* 1AA3 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    /* 1AA5 beq $1ACA */
    if (spc.z) goto L1ACA;
    /* 1AA7 mov.w a,$0588 */
    spc.a = spc_nz(spc_rd(0x0588));
    /* 1AAA beq $1ACA */
    if (spc.z) goto L1ACA;
    /* 1AAC push a */
    spc_push(spc.a);
    /* 1AAD mov x,#$01 */
    spc.x = spc_nz(0x01);
L1AAF:
    /* 1AAF mov.w a,$0588+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0588 + spc.x)));
    /* 1AB2 dec x */
    spc.x = spc_dec(spc.x);
    /* 1AB3 mov.w $0588+x,a */
    spc_st((uint16_t)(0x0588 + spc.x), spc.a);
    /* 1AB6 inc x */
    spc.x = spc_inc(spc.x);
    /* 1AB7 inc x */
    spc.x = spc_inc(spc.x);
    /* 1AB8 cmp x,#$08 */
    spc_cmp(spc.x, 0x08);
    /* 1ABA bne $1AAF */
    if (!spc.z) goto L1AAF;
    /* 1ABC pop a */
    spc.a = spc_pull();
    /* 1ABD asl a */
    spc.a = spc_asl(spc.a);
    /* 1ABE bcc $1AC2 */
    if (!spc.c) goto L1AC2;
    /* 1AC0 set1 $00.7 */
    spc_bit_set(0x00, 7, 1);
L1AC2:
    /* 1AC2 lsr a */
    spc.a = spc_lsr(spc.a);
    /* 1AC3 mov x,#$00 */
    spc.x = spc_nz(0x00);
    /* 1AC5 mov.w $057B,x */
    spc_st(0x057B, spc.x);
    /* 1AC8 bra $1AD9 */
    goto L1AD9;
L1ACA:
    /* 1ACA mov.b a,$1D */
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 1ACC cmp.b a,$1E */
    spc_cmp(spc.a, spc_rd(spc_dp(0x1E)));
    /* 1ACE bne $1AD3 */
    if (!spc.z) goto L1AD3;
L1AD0:
    /* 1AD0 jmp $1B7E */
    { abi_SpcBSfxChannelsFrame(); return; }
L1AD3:
    /* 1AD3 mov.b $1E,a */
    spc_st(spc_dp(0x1E), spc.a);
    /* 1AD5 mov.b a,$1D */
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 1AD7 beq $1AD0 */
    if (spc.z) goto L1AD0;
L1AD9:
    /* 1AD9 mov.b $23,a */
    spc_st(spc_dp(0x23), spc.a);
    /* 1ADB call $1B03 */
    spc_call(abi_SpcBSfxLookup, 0x1ADE);
    /* 1ADE call $1B17 */
    spc_call(abi_SpcBSfxChannel, 0x1AE1);
    /* 1AE1 call $1B48 */
    spc_call(abi_SpcBSfxMode, 0x1AE4);
    /* 1AE4 call $1B71 */
    spc_call(abi_SpcBSfxSide, 0x1AE7);
    /* 1AE7 call $1AFD */
    spc_call(abi_SpcBSfxActivate, 0x1AEA);
    /* 1AEA call $1B9E */
    spc_call(abi_SpcBSfxChannelSave, 0x1AED);
    /* 1AED mov.b a,$23 */
    spc.a = spc_nz(spc_rd(spc_dp(0x23)));
    /* 1AEF mov.w $0410+x,a */
    spc_st((uint16_t)(0x0410 + spc.x), spc.a);
    /* 1AF2 push x */
    spc_push(spc.x);
    /* 1AF3 call $1B7E */
    spc_call(abi_SpcBSfxChannelsFrame, 0x1AF6);
    /* 1AF6 pop x */
    spc.x = spc_pull();
    /* 1AF7 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1AF9 mov.w $0410+x,a */
    spc_st((uint16_t)(0x0410 + spc.x), spc.a);
    /* 1AFC ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F0FA SpcBPortRemap
 * WHAT IT DOES: Port 0 command $88 becomes $15.
 * HOW: CMP and MOV on $00.
 * Entry: none. Exit: flags from the compare.
 * @spc 1115
 * @spcuses 1115-111E
 */
void abi_SpcBPortRemap(void)
{
    /* 1115 cmp $00,#$88 */
    spc_cmp(spc_rd(spc_dp(0x00)), 0x88);
    /* 1118 bne $111D */
    if (!spc.z) goto L111D;
    /* 111A mov $00,#$15 */
    spc_st(spc_dp(0x00), 0x15);
L111D:
    /* 111D ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F103 SpcBChannelGuards
 * WHAT IT DOES: With bit 6 of $2D, stops channel 5 unless it plays sound $43 (SpcBSfxStop). The code after it (from $112D) is SpcBStaggeredChecks' step 6: with bit 5 of $2D, channel 7 stops when its sound $0397 is $77, $78 or $79, and channel 6 when its sound $0396 is $77-$7A. The race driver checks other sounds (SpcChannelGuards).
 * HOW: Compare chains into SpcBSfxStop; the first check returns through SpcBPortRemap's RET.
 * Entry: none. Exit: as SpcBSfxStop or a RET.
 * @spc 111E
 * @spcuses 111D-112D,1861-1871
 */
void abi_SpcBChannelGuards(void)
{
    /* 111E bbc $2D.6,$111D */
    if (!(spc_rd(spc_dp(0x2D)) >> 6 & 1)) goto L111D;
    /* 1121 mov.w a,$0395 */
    spc.a = spc_nz(spc_rd(0x0395));
    /* 1124 cmp a,#$43 */
    spc_cmp(spc.a, 0x43);
    /* 1126 beq $111D */
    if (spc.z) goto L111D;
    /* 1128 mov x,#$05 */
    spc.x = spc_nz(0x05);
    /* 112A jmp $186C */
    { abi_SpcBSfxStop(); return; }
L111D:
    /* 111D ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F148 SpcBStaggeredChecks
 * WHAT IT DOES: Steps the counter $0581 through 0-$18 each tick and at 6, 9, $0C, $0F, $12, $15 and $18 does one check: step 6 the channel 6 and 7 guards (SpcBChannelGuards' second part); the others restart a looping sound by the bits of $2A (a stop through SpcBPort1Command's stop paths with $20 = $0C, $1C, $0A, $1A, $0B or $1B) or queue sound $17, $97 or $25; outside song 1, $2A is cleared. The race driver's (SpcStaggeredChecks) has other steps and sounds.
 * HOW: A compare chain on the counter; the wrap test is SBC #$19 with the carry the last compare left.
 * Entry: none. Exit: as the chosen path leaves it.
 * @spc 1163
 * @spcuses 112D-11ED,12FA-130C,181D-1871
 */
void abi_SpcBStaggeredChecks(void)
{
    /* 1163 inc.w $0581 */
    spc_rmw(0x0581, spc_inc);
    /* 1166 mov.w a,$0581 */
    spc.a = spc_nz(spc_rd(0x0581));
    /* 1169 cmp a,#$06 */
    spc_cmp(spc.a, 0x06);
    /* 116B beq $112D */
    if (spc.z) goto L112D;
    /* 116D cmp a,#$09 */
    spc_cmp(spc.a, 0x09);
    /* 116F beq $1197 */
    if (spc.z) goto L1197;
    /* 1171 cmp a,#$0C */
    spc_cmp(spc.a, 0x0C);
    /* 1173 beq $11A5 */
    if (spc.z) goto L11A5;
    /* 1175 cmp a,#$0F */
    spc_cmp(spc.a, 0x0F);
    /* 1177 beq $11B3 */
    if (spc.z) goto L11B3;
    /* 1179 cmp a,#$12 */
    spc_cmp(spc.a, 0x12);
    /* 117B beq $11C7 */
    if (spc.z) goto L11C7;
    /* 117D cmp a,#$15 */
    spc_cmp(spc.a, 0x15);
    /* 117F beq $11DB */
    if (spc.z) goto L11DB;
    /* 1181 cmp a,#$18 */
    spc_cmp(spc.a, 0x18);
    /* 1183 beq $11E4 */
    if (spc.z) goto L11E4;
    /* 1185 sbc a,#$19 */
    spc.a = spc_sbc(spc.a, 0x19);
    /* 1187 bmi $118E */
    if (spc.n) goto L118E;
    /* 1189 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 118B mov.w $0581,a */
    spc_st(0x0581, spc.a);
L118E:
    /* 118E cmp $1C,#$01 */
    spc_cmp(spc_rd(spc_dp(0x1C)), 0x01);
    /* 1191 beq $1196 */
    if (spc.z) goto L1196;
    /* 1193 mov $2A,#$00 */
    spc_st(spc_dp(0x2A), 0x00);
L1196:
    /* 1196 ret */
    spc_ret();
    return;
L1197:
    /* 1197 bbs $2A.0,$11A0 */
    if ((spc_rd(spc_dp(0x2A)) >> 0 & 1)) goto L11A0;
    /* 119A mov $20,#$0C */
    spc_st(spc_dp(0x20), 0x0C);
    /* 119D jmp $1833 */
    goto L1833;
L11A0:
    /* 11A0 mov a,#$17 */
    spc.a = spc_nz(0x17);
    /* 11A2 jmp $12FA */
    { abi_SpcBSoundQueuePush(); return; }
L11A5:
    /* 11A5 bbs $2A.1,$11AE */
    if ((spc_rd(spc_dp(0x2A)) >> 1 & 1)) goto L11AE;
    /* 11A8 mov $20,#$1C */
    spc_st(spc_dp(0x20), 0x1C);
    /* 11AB jmp $1856 */
    goto L1856;
L11AE:
    /* 11AE mov a,#$97 */
    spc.a = spc_nz(0x97);
    /* 11B0 jmp $12FA */
    { abi_SpcBSoundQueuePush(); return; }
L11B3:
    /* 11B3 bbs $2A.2,$11BC */
    if ((spc_rd(spc_dp(0x2A)) >> 2 & 1)) goto L11BC;
    /* 11B6 mov $20,#$0A */
    spc_st(spc_dp(0x20), 0x0A);
    /* 11B9 jmp $181D */
    goto L181D;
L11BC:
    /* 11BC bbs $2A.0,$118E */
    if ((spc_rd(spc_dp(0x2A)) >> 0 & 1)) goto L118E;
    /* 11BF bbs $2A.4,$118E */
    if ((spc_rd(spc_dp(0x2A)) >> 4 & 1)) goto L118E;
    /* 11C2 mov a,#$25 */
    spc.a = spc_nz(0x25);
    /* 11C4 jmp $12FA */
    { abi_SpcBSoundQueuePush(); return; }
L11C7:
    /* 11C7 bbs $2A.2,$11D0 */
    if ((spc_rd(spc_dp(0x2A)) >> 2 & 1)) goto L11D0;
    /* 11CA mov $20,#$1A */
    spc_st(spc_dp(0x20), 0x1A);
    /* 11CD jmp $1840 */
    goto L1840;
L11D0:
    /* 11D0 bbs $2A.1,$118E */
    if ((spc_rd(spc_dp(0x2A)) >> 1 & 1)) goto L118E;
    /* 11D3 bbs $2A.5,$118E */
    if ((spc_rd(spc_dp(0x2A)) >> 5 & 1)) goto L118E;
    /* 11D6 mov a,#$25 */
    spc.a = spc_nz(0x25);
    /* 11D8 jmp $12FA */
    { abi_SpcBSoundQueuePush(); return; }
L11DB:
    /* 11DB bbs $2A.4,$118E */
    if ((spc_rd(spc_dp(0x2A)) >> 4 & 1)) goto L118E;
    /* 11DE mov $20,#$0B */
    spc_st(spc_dp(0x20), 0x0B);
    /* 11E1 jmp $1828 */
    goto L1828;
L11E4:
    /* 11E4 bbs $2A.5,$118E */
    if ((spc_rd(spc_dp(0x2A)) >> 5 & 1)) goto L118E;
    /* 11E7 mov $20,#$1B */
    spc_st(spc_dp(0x20), 0x1B);
    /* 11EA jmp $184B */
    goto L184B;
L181D:
    /* 181D clr1 $2A.2 */
    spc_bit_set(0x2A, 2, 0);
    /* 181F mov.w a,$0395 */
    spc.a = spc_nz(spc_rd(0x0395));
    /* 1822 cmp a,#$25 */
    spc_cmp(spc.a, 0x25);
    /* 1824 bne $186B */
    if (!spc.z) goto L186B;
    /* 1826 bra $183C */
    goto L183C;
L1828:
    /* 1828 clr1 $2A.4 */
    spc_bit_set(0x2A, 4, 0);
    /* 182A mov.w a,$0395 */
    spc.a = spc_nz(spc_rd(0x0395));
    /* 182D cmp a,#$15 */
    spc_cmp(spc.a, 0x15);
    /* 182F bne $186B */
    if (!spc.z) goto L186B;
    /* 1831 bra $183C */
    goto L183C;
L1833:
    /* 1833 clr1 $2A.0 */
    spc_bit_set(0x2A, 0, 0);
    /* 1835 mov.w a,$0395 */
    spc.a = spc_nz(spc_rd(0x0395));
    /* 1838 cmp a,#$17 */
    spc_cmp(spc.a, 0x17);
    /* 183A bne $186B */
    if (!spc.z) goto L186B;
L183C:
    /* 183C mov x,#$05 */
    spc.x = spc_nz(0x05);
    /* 183E bra $1861 */
    goto L1861;
L1840:
    /* 1840 clr1 $2A.3 */
    spc_bit_set(0x2A, 3, 0);
    /* 1842 mov.w a,$0395 */
    spc.a = spc_nz(spc_rd(0x0395));
    /* 1845 cmp a,#$25 */
    spc_cmp(spc.a, 0x25);
    /* 1847 bne $186B */
    if (!spc.z) goto L186B;
    /* 1849 bra $185F */
    goto L185F;
L184B:
    /* 184B clr1 $2A.5 */
    spc_bit_set(0x2A, 5, 0);
    /* 184D mov.w a,$0395 */
    spc.a = spc_nz(spc_rd(0x0395));
    /* 1850 cmp a,#$15 */
    spc_cmp(spc.a, 0x15);
    /* 1852 bne $186B */
    if (!spc.z) goto L186B;
    /* 1854 bra $185F */
    goto L185F;
L1856:
    /* 1856 clr1 $2A.1 */
    spc_bit_set(0x2A, 1, 0);
    /* 1858 mov.w a,$0395 */
    spc.a = spc_nz(spc_rd(0x0395));
    /* 185B cmp a,#$17 */
    spc_cmp(spc.a, 0x17);
    /* 185D bne $186B */
    if (!spc.z) goto L186B;
L185F:
    /* 185F mov x,#$05 */
    spc.x = spc_nz(0x05);
L1861:
    /* 1861 mov.w a,$0408+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 1864 beq $186B */
    if (spc.z) goto L186B;
    /* 1866 mov.b a,$20 */
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 1868 mov.w $03E0+x,a */
    spc_st((uint16_t)(0x03E0 + spc.x), spc.a);
L186B:
    /* 186B ret */
    spc_ret();
    return;
L112D:
    /* 112D bbc $2D.5,$1156 */
    if (!(spc_rd(spc_dp(0x2D)) >> 5 & 1)) goto L1156;
    /* 1130 mov.w a,$0397 */
    spc.a = spc_nz(spc_rd(0x0397));
    /* 1133 cmp a,#$77 */
    spc_cmp(spc.a, 0x77);
    /* 1135 beq $1157 */
    if (spc.z) goto L1157;
    /* 1137 cmp a,#$78 */
    spc_cmp(spc.a, 0x78);
    /* 1139 beq $1157 */
    if (spc.z) goto L1157;
    /* 113B cmp a,#$79 */
    spc_cmp(spc.a, 0x79);
    /* 113D beq $1157 */
    if (spc.z) goto L1157;
    /* 113F cmp a,#$79 */
    spc_cmp(spc.a, 0x79);
    /* 1141 beq $1157 */
    if (spc.z) goto L1157;
L1143:
    /* 1143 mov.w a,$0396 */
    spc.a = spc_nz(spc_rd(0x0396));
    /* 1146 cmp a,#$77 */
    spc_cmp(spc.a, 0x77);
    /* 1148 beq $115E */
    if (spc.z) goto L115E;
    /* 114A cmp a,#$78 */
    spc_cmp(spc.a, 0x78);
    /* 114C beq $115E */
    if (spc.z) goto L115E;
    /* 114E cmp a,#$79 */
    spc_cmp(spc.a, 0x79);
    /* 1150 beq $115E */
    if (spc.z) goto L115E;
    /* 1152 cmp a,#$7A */
    spc_cmp(spc.a, 0x7A);
    /* 1154 beq $115E */
    if (spc.z) goto L115E;
L1156:
    /* 1156 ret */
    spc_ret();
    return;
L1157:
    /* 1157 mov x,#$07 */
    spc.x = spc_nz(0x07);
    /* 1159 call $186C */
    spc_call(abi_SpcBSfxStop, 0x115C);
    /* 115C bra $1143 */
    goto L1143;
L115E:
    /* 115E mov x,#$06 */
    spc.x = spc_nz(0x06);
    /* 1160 jmp $186C */
    { abi_SpcBSfxStop(); return; }
}

/*
 * @routine 1A:F1D2 SpcBGameStateTick
 * WHAT IT DOES: The battle's sound rules each tick: the counters $056A/$056B after song request 5; song request 3 $E6 ticks after request 2 ($0567); while $056A is under $80, port commands $11-$14 (the list at $1251) are dropped; command $32 clears $2A, sets bits 5 and 6 of $2D and stops channels 2, 1 and 0, commands $03 and $10 stop channels 5 and 4 and do the same to $2A and $2D; the timer $0579 sets bit 5 of $2D after $78 ticks; port 1 commands $09 and $0D are cleared while channel 5 plays sound $17 or $21; channel 4 stops when channel 7 plays $32 and channel 4 $38. The race driver's (SpcGameStateTick) also has the countdown, tempo and finish music rules.
 * HOW: Compare chains on $1D, $01, $0394-$0397; SpcBSfxStop.
 * Entry: none. Exit: A, X changed.
 * @spc 11ED
 * @spcuses 11ED-1251,1256-12D2,1861-1871
 */
void abi_SpcBGameStateTick(void)
{
    /* 11ED mov.w a,$056A */
    spc.a = spc_nz(spc_rd(0x056A));
    /* 11F0 cmp a,#$FF */
    spc_cmp(spc.a, 0xFF);
    /* 11F2 beq $11F7 */
    if (spc.z) goto L11F7;
    /* 11F4 inc.w $056A */
    spc_rmw(0x056A, spc_inc);
L11F7:
    /* 11F7 mov.b a,$1B */
    spc.a = spc_nz(spc_rd(spc_dp(0x1B)));
    /* 11F9 cmp a,#$05 */
    spc_cmp(spc.a, 0x05);
    /* 11FB beq $11FF */
    if (spc.z) goto L11FF;
    /* 11FD bra $1207 */
    goto L1207;
L11FF:
    /* 11FF mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1201 mov.w $056A,a */
    spc_st(0x056A, spc.a);
    /* 1204 mov.w $056B,a */
    spc_st(0x056B, spc.a);
L1207:
    /* 1207 mov.w a,$056A */
    spc.a = spc_nz(spc_rd(0x056A));
    /* 120A bmi $120E */
    if (spc.n) goto L120E;
    /* 120C bra $1218 */
    goto L1218;
L120E:
    /* 120E mov.w a,$056B */
    spc.a = spc_nz(spc_rd(0x056B));
    /* 1211 cmp a,#$FF */
    spc_cmp(spc.a, 0xFF);
    /* 1213 beq $1218 */
    if (spc.z) goto L1218;
    /* 1215 inc.w $056B */
    spc_rmw(0x056B, spc_inc);
L1218:
    /* 1218 cmp $1B,#$02 */
    spc_cmp(spc_rd(spc_dp(0x1B)), 0x02);
    /* 121B bne $1222 */
    if (!spc.z) goto L1222;
    /* 121D mov a,#$01 */
    spc.a = spc_nz(0x01);
    /* 121F mov.w $0567,a */
    spc_st(0x0567, spc.a);
L1222:
    /* 1222 mov.w a,$0567 */
    spc.a = spc_nz(spc_rd(0x0567));
    /* 1225 beq $1239 */
    if (spc.z) goto L1239;
    /* 1227 inc.w $0567 */
    spc_rmw(0x0567, spc_inc);
    /* 122A mov.w a,$0567 */
    spc.a = spc_nz(spc_rd(0x0567));
    /* 122D cmp a,#$E6 */
    spc_cmp(spc.a, 0xE6);
    /* 122F bne $1239 */
    if (!spc.z) goto L1239;
    /* 1231 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1233 mov.w $0567,a */
    spc_st(0x0567, spc.a);
    /* 1236 mov $1B,#$03 */
    spc_st(spc_dp(0x1B), 0x03);
L1239:
    /* 1239 mov.w a,$056A */
    spc.a = spc_nz(spc_rd(0x056A));
    /* 123C bmi $1256 */
    if (spc.n) goto L1256;
    /* 123E mov x,#$00 */
    spc.x = spc_nz(0x00);
L1240:
    /* 1240 mov.w a,$1251+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x1251 + spc.x)));
    /* 1243 beq $1256 */
    if (spc.z) goto L1256;
    /* 1245 cmp.b a,$1D */
    spc_cmp(spc.a, spc_rd(spc_dp(0x1D)));
    /* 1247 beq $124C */
    if (spc.z) goto L124C;
    /* 1249 inc x */
    spc.x = spc_inc(spc.x);
    /* 124A bra $1240 */
    goto L1240;
L124C:
    /* 124C mov $1D,#$00 */
    spc_st(spc_dp(0x1D), 0x00);
    /* 124F bra $1256 */
    goto L1256;
L1256:
    /* 1256 cmp $1D,#$32 */
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x32);
    /* 1259 bne $126F */
    if (!spc.z) goto L126F;
    /* 125B mov $2A,#$00 */
    spc_st(spc_dp(0x2A), 0x00);
    /* 125E set1 $2D.5 */
    spc_bit_set(0x2D, 5, 1);
    /* 1260 set1 $2D.6 */
    spc_bit_set(0x2D, 6, 1);
    /* 1262 mov x,#$02 */
    spc.x = spc_nz(0x02);
    /* 1264 call $186C */
    spc_call(abi_SpcBSfxStop, 0x1267);
    /* 1267 dec x */
    spc.x = spc_dec(spc.x);
    /* 1268 call $186C */
    spc_call(abi_SpcBSfxStop, 0x126B);
    /* 126B dec x */
    spc.x = spc_dec(spc.x);
    /* 126C call $186C */
    spc_call(abi_SpcBSfxStop, 0x126F);
L126F:
    /* 126F cmp $1D,#$03 */
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x03);
    /* 1272 beq $127B */
    if (spc.z) goto L127B;
    /* 1274 cmp $1D,#$10 */
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x10);
    /* 1277 beq $127B */
    if (spc.z) goto L127B;
    /* 1279 bra $128B */
    goto L128B;
L127B:
    /* 127B mov x,#$05 */
    spc.x = spc_nz(0x05);
    /* 127D call $186C */
    spc_call(abi_SpcBSfxStop, 0x1280);
    /* 1280 dec x */
    spc.x = spc_dec(spc.x);
    /* 1281 call $186C */
    spc_call(abi_SpcBSfxStop, 0x1284);
    /* 1284 mov $2A,#$00 */
    spc_st(spc_dp(0x2A), 0x00);
    /* 1287 set1 $2D.6 */
    spc_bit_set(0x2D, 6, 1);
    /* 1289 set1 $2D.5 */
    spc_bit_set(0x2D, 5, 1);
L128B:
    /* 128B mov.w a,$0579 */
    spc.a = spc_nz(spc_rd(0x0579));
    /* 128E beq $12A2 */
    if (spc.z) goto L12A2;
    /* 1290 inc.w $0579 */
    spc_rmw(0x0579, spc_inc);
    /* 1293 mov.w a,$0579 */
    spc.a = spc_nz(spc_rd(0x0579));
    /* 1296 setc */
    spc.c = 1;
    /* 1297 sbc a,#$78 */
    spc.a = spc_sbc(spc.a, 0x78);
    /* 1299 bmi $12A2 */
    if (spc.n) goto L12A2;
    /* 129B set1 $2D.5 */
    spc_bit_set(0x2D, 5, 1);
    /* 129D mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 129F mov.w $0579,a */
    spc_st(0x0579, spc.a);
L12A2:
    /* 12A2 mov.w a,$0395 */
    spc.a = spc_nz(spc_rd(0x0395));
    /* 12A5 cmp a,#$17 */
    spc_cmp(spc.a, 0x17);
    /* 12A7 beq $12AF */
    if (spc.z) goto L12AF;
    /* 12A9 cmp a,#$21 */
    spc_cmp(spc.a, 0x21);
    /* 12AB beq $12AF */
    if (spc.z) goto L12AF;
    /* 12AD bra $12BE */
    goto L12BE;
L12AF:
    /* 12AF cmp $01,#$09 */
    spc_cmp(spc_rd(spc_dp(0x01)), 0x09);
    /* 12B2 beq $12BB */
    if (spc.z) goto L12BB;
    /* 12B4 cmp $01,#$0D */
    spc_cmp(spc_rd(spc_dp(0x01)), 0x0D);
    /* 12B7 beq $12BB */
    if (spc.z) goto L12BB;
    /* 12B9 bra $12BE */
    goto L12BE;
L12BB:
    /* 12BB mov $01,#$00 */
    spc_st(spc_dp(0x01), 0x00);
L12BE:
    /* 12BE mov.w a,$0397 */
    spc.a = spc_nz(spc_rd(0x0397));
    /* 12C1 cmp a,#$32 */
    spc_cmp(spc.a, 0x32);
    /* 12C3 bne $12D1 */
    if (!spc.z) goto L12D1;
    /* 12C5 mov.w a,$0394 */
    spc.a = spc_nz(spc_rd(0x0394));
    /* 12C8 cmp a,#$38 */
    spc_cmp(spc.a, 0x38);
    /* 12CA bne $12D1 */
    if (!spc.z) goto L12D1;
    /* 12CC mov x,#$04 */
    spc.x = spc_nz(0x04);
    /* 12CE call $186C */
    spc_call(abi_SpcBSfxStop, 0x12D1);
L12D1:
    /* 12D1 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F2B7 SpcBCommand4C
 * WHAT IT DOES: Port command $4C: channels 7 and 6 stopped with their fades (SpcBSfxStopRequest), 5, 4 and 3 stopped, the timer $0579 started, $2A cleared, the music paused, bit 6 of $2D set. As the race driver's (SpcCommand4C) without its bit 3 of $2D.
 * HOW: CALLs with X counting down.
 * Entry: none. Exit: as SpcMusicPause.
 * @spc 12D2
 * @spcuses 0A3B-0A3C,0A58-0A6D,12D2-12FA,1861-1871,199E-19B6
 */
void abi_SpcBCommand4C(void)
{
    /* 12D2 cmp $1D,#$4C */
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x4C);
    /* 12D5 bne $12F9 */
    if (!spc.z) goto L12F9;
    /* 12D7 mov x,#$07 */
    spc.x = spc_nz(0x07);
    /* 12D9 call $199E */
    spc_call(abi_SpcBSfxStopRequest, 0x12DC);
    /* 12DC dec x */
    spc.x = spc_dec(spc.x);
    /* 12DD call $199E */
    spc_call(abi_SpcBSfxStopRequest, 0x12E0);
    /* 12E0 dec x */
    spc.x = spc_dec(spc.x);
    /* 12E1 call $186C */
    spc_call(abi_SpcBSfxStop, 0x12E4);
    /* 12E4 dec x */
    spc.x = spc_dec(spc.x);
    /* 12E5 call $186C */
    spc_call(abi_SpcBSfxStop, 0x12E8);
    /* 12E8 dec x */
    spc.x = spc_dec(spc.x);
    /* 12E9 call $186C */
    spc_call(abi_SpcBSfxStop, 0x12EC);
    /* 12EC mov a,#$01 */
    spc.a = spc_nz(0x01);
    /* 12EE mov.w $0579,a */
    spc_st(0x0579, spc.a);
    /* 12F1 mov $2A,#$00 */
    spc_st(spc_dp(0x2A), 0x00);
    /* 12F4 call $0A60 */
    spc_call(abi_SpcMusicPause, 0x12F7);
    /* 12F7 set1 $2D.6 */
    spc_bit_set(0x2D, 6, 1);
L12F9:
    /* 12F9 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F2DF SpcBSoundQueuePush
 * WHAT IT DOES: Appends sound A to the zero-ended queue at $0588 (the race driver's SpcSoundQueuePush).
 * HOW: Scans for the first zero byte; X kept on the stack.
 * Entry: A = sound. Exit: A, X kept.
 * @spc 12FA
 * @spcuses 12FA-130C
 */
void abi_SpcBSoundQueuePush(void)
{
    /* 12FA push x */
    spc_push(spc.x);
    /* 12FB push a */
    spc_push(spc.a);
    /* 12FC mov x,#$00 */
    spc.x = spc_nz(0x00);
L12FE:
    /* 12FE mov.w a,$0588+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0588 + spc.x)));
    /* 1301 beq $1306 */
    if (spc.z) goto L1306;
    /* 1303 inc x */
    spc.x = spc_inc(spc.x);
    /* 1304 bra $12FE */
    goto L12FE;
L1306:
    /* 1306 pop a */
    spc.a = spc_pull();
    /* 1307 mov.w $0588+x,a */
    spc_st((uint16_t)(0x0588 + spc.x), spc.a);
    /* 130A pop x */
    spc.x = spc_pull();
    /* 130B ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F2F1 SpcBCommandTimers
 * WHAT IT DOES: Unless stopped (bit 0 of $29): command $3E starts the count $0568 (bit 7 set with bit 7 of port 0), which at $2A queues sound $2F or $0F by channel 7's sound $0397 ($77/$79 or $78/$7A), or channel 6's $0396 when bit 7 is set, with bit 7 carried into the sound; commands $20 and $16 mark bits 4 and 5 of $2C, become command $1B or $27 by the track and start the count $055D, which at $11 queues $20 or $16. The race driver's (SpcCommandTimers) also has commands $37, $30 and $29.
 * HOW: Byte counters in RAM; ASL of $0568 for its bit 7; SpcBSoundQueuePush.
 * Entry: none. Exit: A, X changed.
 * @spc 130C
 * @spcuses 12FA-13C6
 */
void abi_SpcBCommandTimers(void)
{
    /* 130C bbs $29.0,$130B */
    if ((spc_rd(spc_dp(0x29)) >> 0 & 1)) goto L130B;
    /* 130F cmp $1D,#$3E */
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x3E);
    /* 1312 bne $1325 */
    if (!spc.z) goto L1325;
    /* 1314 mov a,#$01 */
    spc.a = spc_nz(0x01);
    /* 1316 mov.w $0568,a */
    spc_st(0x0568, spc.a);
    /* 1319 bbc $00.7,$1325 */
    if (!(spc_rd(spc_dp(0x00)) >> 7 & 1)) goto L1325;
    /* 131C mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 131E set1 $20.7 */
    spc_bit_set(0x20, 7, 1);
    /* 1320 mov.b a,$20 */
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 1322 mov.w $0568,a */
    spc_st(0x0568, spc.a);
L1325:
    /* 1325 mov.w a,$0568 */
    spc.a = spc_nz(spc_rd(0x0568));
    /* 1328 beq $132D */
    if (spc.z) goto L132D;
    /* 132A inc.w $0568 */
    spc_rmw(0x0568, spc_inc);
L132D:
    /* 132D mov.w a,$0568 */
    spc.a = spc_nz(spc_rd(0x0568));
    /* 1330 and a,#$7F */
    spc.a = spc_and(spc.a, 0x7F);
    /* 1332 cmp a,#$2A */
    spc_cmp(spc.a, 0x2A);
    /* 1334 bne $136B */
    if (!spc.z) goto L136B;
    /* 1336 mov.w a,$0568 */
    spc.a = spc_nz(spc_rd(0x0568));
    /* 1339 asl a */
    spc.a = spc_asl(spc.a);
    /* 133A bcs $1351 */
    if (spc.c) goto L1351;
    /* 133C mov.w a,$0397 */
    spc.a = spc_nz(spc_rd(0x0397));
L133F:
    /* 133F cmp a,#$77 */
    spc_cmp(spc.a, 0x77);
    /* 1341 beq $135A */
    if (spc.z) goto L135A;
    /* 1343 cmp a,#$78 */
    spc_cmp(spc.a, 0x78);
    /* 1345 beq $1356 */
    if (spc.z) goto L1356;
    /* 1347 cmp a,#$79 */
    spc_cmp(spc.a, 0x79);
    /* 1349 beq $135A */
    if (spc.z) goto L135A;
    /* 134B cmp a,#$7A */
    spc_cmp(spc.a, 0x7A);
    /* 134D beq $1356 */
    if (spc.z) goto L1356;
    /* 134F bra $1366 */
    goto L1366;
L1351:
    /* 1351 mov.w a,$0396 */
    spc.a = spc_nz(spc_rd(0x0396));
    /* 1354 bra $133F */
    goto L133F;
L1356:
    /* 1356 mov a,#$0F */
    spc.a = spc_nz(0x0F);
    /* 1358 bra $135C */
    goto L135C;
L135A:
    /* 135A mov a,#$2F */
    spc.a = spc_nz(0x2F);
L135C:
    /* 135C asl.w $0568 */
    spc_rmw(0x0568, spc_asl);
    /* 135F bcc $1363 */
    if (!spc.c) goto L1363;
    /* 1361 or a,#$80 */
    spc.a = spc_or(spc.a, 0x80);
L1363:
    /* 1363 call $12FA */
    spc_call(abi_SpcBSoundQueuePush, 0x1366);
L1366:
    /* 1366 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1368 mov.w $0568,a */
    spc_st(0x0568, spc.a);
L136B:
    /* 136B cmp $1D,#$20 */
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x20);
    /* 136E beq $1377 */
    if (spc.z) goto L1377;
    /* 1370 cmp $1D,#$16 */
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x16);
    /* 1373 beq $137B */
    if (spc.z) goto L137B;
    /* 1375 bra $1399 */
    goto L1399;
L1377:
    /* 1377 set1 $2C.4 */
    spc_bit_set(0x2C, 4, 1);
    /* 1379 bra $137D */
    goto L137D;
L137B:
    /* 137B set1 $2C.5 */
    spc_bit_set(0x2C, 5, 1);
L137D:
    /* 137D mov.w a,$0397 */
    spc.a = spc_nz(spc_rd(0x0397));
    /* 1380 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    /* 1382 beq $1391 */
    if (spc.z) goto L1391;
    /* 1384 cmp a,#$04 */
    spc_cmp(spc.a, 0x04);
    /* 1386 beq $138C */
    if (spc.z) goto L138C;
    /* 1388 cmp a,#$07 */
    spc_cmp(spc.a, 0x07);
    /* 138A beq $1391 */
    if (spc.z) goto L1391;
L138C:
    /* 138C mov $1D,#$1B */
    spc_st(spc_dp(0x1D), 0x1B);
    /* 138F bra $1394 */
    goto L1394;
L1391:
    /* 1391 mov $1D,#$27 */
    spc_st(spc_dp(0x1D), 0x27);
L1394:
    /* 1394 mov a,#$01 */
    spc.a = spc_nz(0x01);
    /* 1396 mov.w $055D,a */
    spc_st(0x055D, spc.a);
L1399:
    /* 1399 mov.w a,$055D */
    spc.a = spc_nz(spc_rd(0x055D));
    /* 139C beq $13A1 */
    if (spc.z) goto L13A1;
    /* 139E inc.w $055D */
    spc_rmw(0x055D, spc_inc);
L13A1:
    /* 13A1 mov.w a,$055D */
    spc.a = spc_nz(spc_rd(0x055D));
    /* 13A4 cmp a,#$11 */
    spc_cmp(spc.a, 0x11);
    /* 13A6 bne $13C5 */
    if (!spc.z) goto L13C5;
    /* 13A8 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 13AA mov.w $055D,a */
    spc_st(0x055D, spc.a);
    /* 13AD bbs $2C.4,$13B5 */
    if ((spc_rd(spc_dp(0x2C)) >> 4 & 1)) goto L13B5;
    /* 13B0 bbs $2C.5,$13BE */
    if ((spc_rd(spc_dp(0x2C)) >> 5 & 1)) goto L13BE;
    /* 13B3 bra $13C5 */
    goto L13C5;
L13B5:
    /* 13B5 clr1 $2C.4 */
    spc_bit_set(0x2C, 4, 0);
    /* 13B7 mov a,#$20 */
    spc.a = spc_nz(0x20);
    /* 13B9 call $12FA */
    spc_call(abi_SpcBSoundQueuePush, 0x13BC);
    /* 13BC bra $13C5 */
    goto L13C5;
L13BE:
    /* 13BE clr1 $2C.5 */
    spc_bit_set(0x2C, 5, 0);
    /* 13C0 mov a,#$16 */
    spc.a = spc_nz(0x16);
    /* 13C2 call $12FA */
    spc_call(abi_SpcBSoundQueuePush, 0x13C5);
L13C5:
    /* 13C5 ret */
    spc_ret();
    return;
L130B:
    /* 130B ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F3AB SpcBTempoBend
 * WHAT IT DOES: The tempo bend (SpcBTempoBendStart decides it), as the race driver's SpcTempoBend: while bit 2 of $2C is set, each tick lowers the transpose by the step count / $15 and the tempo by an accumulated amount from its saved values ($0566, $0578); bit 3 restores them and stops.
 * HOW: Word accumulator $0553/$0554, DIV by $15.
 * Entry: none. Exit: A = the tempo high byte.
 * @spc 13C6
 * @spcuses 13C6-1467
 */
void abi_SpcBTempoBend(void)
{
    /* 13C6 call $1444 */
    spc_call(abi_SpcBTempoBendStart, 0x13C9);
    /* 13C9 bbs $2C.2,$13D0 */
    if ((spc_rd(spc_dp(0x2C)) >> 2 & 1)) goto L13D0;
    /* 13CC bbs $2C.3,$13DB */
    if ((spc_rd(spc_dp(0x2C)) >> 3 & 1)) goto L13DB;
L13CF:
    /* 13CF ret */
    spc_ret();
    return;
L13D0:
    /* 13D0 mov.w a,$0555 */
    spc.a = spc_nz(spc_rd(0x0555));
    /* 13D3 beq $13F7 */
    if (spc.z) goto L13F7;
    /* 13D5 cmp a,#$FE */
    spc_cmp(spc.a, 0xFE);
    /* 13D7 beq $13CF */
    if (spc.z) goto L13CF;
    /* 13D9 bra $140F */
    goto L140F;
L13DB:
    /* 13DB clr1 $2C.2 */
    spc_bit_set(0x2C, 2, 0);
    /* 13DD clr1 $2C.3 */
    spc_bit_set(0x2C, 3, 0);
    /* 13DF mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 13E1 mov.w $0555,a */
    spc_st(0x0555, spc.a);
    /* 13E4 mov.w $0553,a */
    spc_st(0x0553, spc.a);
    /* 13E7 mov.w $0554,a */
    spc_st(0x0554, spc.a);
    /* 13EA mov.w $0569,a */
    spc_st(0x0569, spc.a);
    /* 13ED mov.w a,$0578 */
    spc.a = spc_nz(spc_rd(0x0578));
    /* 13F0 mov.b $50,a */
    spc_st(spc_dp(0x50), spc.a);
    /* 13F2 mov.w a,$0566 */
    spc.a = spc_nz(spc_rd(0x0566));
    /* 13F5 bra $1441 */
    goto L1441;
L13F7:
    /* 13F7 mov a,#$04 */
    spc.a = spc_nz(0x04);
    /* 13F9 mov.w $0554,a */
    spc_st(0x0554, spc.a);
    /* 13FC mov.b a,$53 */
    spc.a = spc_nz(spc_rd(spc_dp(0x53)));
    /* 13FE mov.w $0566,a */
    spc_st(0x0566, spc.a);
    /* 1401 mov.b a,$50 */
    spc.a = spc_nz(spc_rd(spc_dp(0x50)));
    /* 1403 mov.w $0578,a */
    spc_st(0x0578, spc.a);
    /* 1406 mov y,#$7F */
    spc.y = spc_nz(0x7F);
    /* 1408 mul ya */
    spc_mul();
    /* 1409 mov x,#$FE */
    spc.x = spc_nz(0xFE);
    /* 140B div ya,x */
    spc_div();
    /* 140C mov.w $0569,a */
    spc_st(0x0569, spc.a);
L140F:
    /* 140F inc.w $0555 */
    spc_rmw(0x0555, spc_inc);
    /* 1412 mov.w a,$0555 */
    spc.a = spc_nz(spc_rd(0x0555));
    /* 1415 mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 1417 mov x,#$15 */
    spc.x = spc_nz(0x15);
    /* 1419 div ya,x */
    spc_div();
    /* 141A mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 141C setc */
    spc.c = 1;
    /* 141D mov.w a,$0578 */
    spc.a = spc_nz(spc_rd(0x0578));
    /* 1420 sbc.b a,$20 */
    spc.a = spc_sbc(spc.a, spc_rd(spc_dp(0x20)));
    /* 1422 mov.b $50,a */
    spc_st(spc_dp(0x50), spc.a);
    /* 1424 mov.w a,$0569 */
    spc.a = spc_nz(spc_rd(0x0569));
    /* 1427 mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 1429 mov $21,#$00 */
    spc_st(spc_dp(0x21), 0x00);
    /* 142C mov.w a,$0553 */
    spc.a = spc_nz(spc_rd(0x0553));
    /* 142F mov.w y,$0554 */
    spc.y = spc_nz(spc_rd(0x0554));
    /* 1432 addw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 1434 mov.w $0553,a */
    spc_st(0x0553, spc.a);
    /* 1437 mov.w $0554,y */
    spc_st(0x0554, spc.y);
    /* 143A mov.w a,$0566 */
    spc.a = spc_nz(spc_rd(0x0566));
    /* 143D setc */
    spc.c = 1;
    /* 143E sbc.w a,$0554 */
    spc.a = spc_sbc(spc.a, spc_rd(0x0554));
L1441:
    /* 1441 mov.b $53,a */
    spc_st(spc_dp(0x53), spc.a);
    /* 1443 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F429 SpcBTempoBendStart
 * WHAT IT DOES: Command $2C during song 1 starts the tempo bend (bit 2 of $2C); commands $30 and $3C during a bend end it (bit 3). The race driver's SpcTempoBendStart.
 * HOW: SET1/CLR1 on $2C.
 * Entry: none. Exit: flags from the compares.
 * @spc 1444
 * @spcuses 1444-1467
 */
void abi_SpcBTempoBendStart(void)
{
    /* 1444 mov.b a,$1D */
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 1446 cmp a,#$2C */
    spc_cmp(spc.a, 0x2C);
    /* 1448 beq $1458 */
    if (spc.z) goto L1458;
    /* 144A mov.w x,$0555 */
    spc.x = spc_nz(spc_rd(0x0555));
    /* 144D beq $1461 */
    if (spc.z) goto L1461;
    /* 144F cmp a,#$30 */
    spc_cmp(spc.a, 0x30);
    /* 1451 beq $1462 */
    if (spc.z) goto L1462;
    /* 1453 cmp a,#$3C */
    spc_cmp(spc.a, 0x3C);
    /* 1455 beq $1462 */
    if (spc.z) goto L1462;
    /* 1457 ret */
    spc_ret();
    return;
L1458:
    /* 1458 cmp $1C,#$01 */
    spc_cmp(spc_rd(spc_dp(0x1C)), 0x01);
    /* 145B bne $1461 */
    if (!spc.z) goto L1461;
    /* 145D set1 $2C.2 */
    spc_bit_set(0x2C, 2, 1);
    /* 145F clr1 $2C.3 */
    spc_bit_set(0x2C, 3, 0);
L1461:
    /* 1461 ret */
    spc_ret();
    return;
L1462:
    /* 1462 clr1 $2C.2 */
    spc_bit_set(0x2C, 2, 0);
    /* 1464 set1 $2C.3 */
    spc_bit_set(0x2C, 3, 1);
    /* 1466 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F44C SpcBPort1Clamp
 * WHAT IT DOES: Outside song 1, a port 1 command of 7 or more is ignored ($01 cleared). The race driver's SpcPort1Clamp.
 * HOW: SBC and BMI.
 * Entry: none. Exit: flags.
 * @spc 1467
 * @spcuses 1467-1477
 */
void abi_SpcBPort1Clamp(void)
{
    /* 1467 cmp $1C,#$01 */
    spc_cmp(spc_rd(spc_dp(0x1C)), 0x01);
    /* 146A beq $1476 */
    if (spc.z) goto L1476;
    /* 146C mov.b a,$01 */
    spc.a = spc_nz(spc_rd(spc_dp(0x01)));
    /* 146E setc */
    spc.c = 1;
    /* 146F sbc a,#$07 */
    spc.a = spc_sbc(spc.a, 0x07);
    /* 1471 bmi $1476 */
    if (spc.n) goto L1476;
    /* 1473 mov $01,#$00 */
    spc_st(spc_dp(0x01), 0x00);
L1476:
    /* 1476 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F45C SpcBCommandFilter
 * WHAT IT DOES: Outside song 1, a port command $1D not in the list at $148F is dropped. As the race driver's SpcCommandFilter, with the battle's list.
 * HOW: A zero-ended list scan.
 * Entry: none. Exit: X = list index.
 * @spc 1477
 * @spcuses 1477-148F
 */
void abi_SpcBCommandFilter(void)
{
    /* 1477 mov.b a,$1C */
    spc.a = spc_nz(spc_rd(spc_dp(0x1C)));
    /* 1479 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    /* 147B beq $148E */
    if (spc.z) goto L148E;
    /* 147D mov x,#$00 */
    spc.x = spc_nz(0x00);
L147F:
    /* 147F mov.w a,$148F+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x148F + spc.x)));
    /* 1482 beq $148B */
    if (spc.z) goto L148B;
    /* 1484 cmp.b a,$1D */
    spc_cmp(spc.a, spc_rd(spc_dp(0x1D)));
    /* 1486 beq $148E */
    if (spc.z) goto L148E;
    /* 1488 inc x */
    spc.x = spc_inc(spc.x);
    /* 1489 bra $147F */
    goto L147F;
L148B:
    /* 148B mov $1D,#$00 */
    spc_st(spc_dp(0x1D), 0x00);
L148E:
    /* 148E ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F48B SpcBCommandBlock
 * WHAT IT DOES: A sound command in the list at $14B8 is dropped. As the race driver's SpcCommandBlock, with a longer list.
 * HOW: A zero-ended list scan.
 * Entry: none. Exit: X = index.
 * @spc 14A6
 * @spcuses 14A6-14B8
 */
void abi_SpcBCommandBlock(void)
{
    /* 14A6 mov x,#$00 */
    spc.x = spc_nz(0x00);
L14A8:
    /* 14A8 mov.w a,$14B8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x14B8 + spc.x)));
    /* 14AB beq $14B7 */
    if (spc.z) goto L14B7;
    /* 14AD cmp.b a,$1D */
    spc_cmp(spc.a, spc_rd(spc_dp(0x1D)));
    /* 14AF beq $14B4 */
    if (spc.z) goto L14B4;
    /* 14B1 inc x */
    spc.x = spc_inc(spc.x);
    /* 14B2 bra $14A8 */
    goto L14A8;
L14B4:
    /* 14B4 mov $1D,#$00 */
    spc_st(spc_dp(0x1D), 0x00);
L14B7:
    /* 14B7 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F4B6 SpcBPitchRampB
 * WHAT IT DOES: The second pair of pitch ramps: bit 2 of $28 ramps the offset $054D/$054F up (SpcBPitchRampB1Up), bit 3 down (SpcBPitchRampB1Down); bits 4 and 5 do the same to $054C/$054E (SpcBPitchRampB2Up, SpcBPitchRampB2Down). The race driver (SpcPitchRampB) has only the first.
 * HOW: BBC tests.
 * Entry: none. Exit: as the last ramp leaves it.
 * @spc 14D1
 * @spcuses 14D1-1584
 */
void abi_SpcBPitchRampB(void)
{
    /* 14D1 bbc $28.2,$14D7 */
    if (!(spc_rd(spc_dp(0x28)) >> 2 & 1)) goto L14D7;
    /* 14D4 call $14EA */
    spc_call(abi_SpcBPitchRampB1Up, 0x14D7);
L14D7:
    /* 14D7 bbc $28.3,$14DD */
    if (!(spc_rd(spc_dp(0x28)) >> 3 & 1)) goto L14DD;
    /* 14DA call $152C */
    spc_call(abi_SpcBPitchRampB1Down, 0x14DD);
L14DD:
    /* 14DD bbc $28.4,$14E3 */
    if (!(spc_rd(spc_dp(0x28)) >> 4 & 1)) goto L14E3;
    /* 14E0 call $150B */
    spc_call(abi_SpcBPitchRampB2Up, 0x14E3);
L14E3:
    /* 14E3 bbc $28.5,$14E9 */
    if (!(spc_rd(spc_dp(0x28)) >> 5 & 1)) goto L14E9;
    /* 14E6 call $1558 */
    spc_call(abi_SpcBPitchRampB2Down, 0x14E9);
L14E9:
    /* 14E9 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F4CF SpcBPitchRampB1Up
 * WHAT IT DOES: Up to $14 steps (count $0547), adds $20 to the pitch offset $054D/$054F. The race driver's SpcPitchRampBUp.
 * HOW: ADDW through $20/$21.
 * Entry: none. Exit: YA = the offset.
 * @spc 14EA
 * @spcuses 14EA-150B
 */
void abi_SpcBPitchRampB1Up(void)
{
    /* 14EA mov.w a,$0547 */
    spc.a = spc_nz(spc_rd(0x0547));
    /* 14ED cmp a,#$14 */
    spc_cmp(spc.a, 0x14);
    /* 14EF beq $150A */
    if (spc.z) goto L150A;
    /* 14F1 inc.w $0547 */
    spc_rmw(0x0547, spc_inc);
    /* 14F4 mov.w a,$054D */
    spc.a = spc_nz(spc_rd(0x054D));
    /* 14F7 mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 14F9 mov.w a,$054F */
    spc.a = spc_nz(spc_rd(0x054F));
    /* 14FC mov.b $21,a */
    spc_st(spc_dp(0x21), spc.a);
    /* 14FE mov a,#$20 */
    spc.a = spc_nz(0x20);
    /* 1500 mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 1502 addw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 1504 mov.w $054D,a */
    spc_st(0x054D, spc.a);
    /* 1507 mov.w $054F,y */
    spc_st(0x054F, spc.y);
L150A:
    /* 150A ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F4F0 SpcBPitchRampB2Up
 * WHAT IT DOES: Up to $14 steps (count $0546), adds $20 to the pitch offset $054C/$054E.
 * HOW: ADDW through $20/$21.
 * Entry: none. Exit: YA = the offset.
 * @spc 150B
 * @spcuses 150B-152C
 */
void abi_SpcBPitchRampB2Up(void)
{
    /* 150B mov.w a,$0546 */
    spc.a = spc_nz(spc_rd(0x0546));
    /* 150E cmp a,#$14 */
    spc_cmp(spc.a, 0x14);
    /* 1510 beq $152B */
    if (spc.z) goto L152B;
    /* 1512 inc.w $0546 */
    spc_rmw(0x0546, spc_inc);
    /* 1515 mov.w a,$054C */
    spc.a = spc_nz(spc_rd(0x054C));
    /* 1518 mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 151A mov.w a,$054E */
    spc.a = spc_nz(spc_rd(0x054E));
    /* 151D mov.b $21,a */
    spc_st(spc_dp(0x21), spc.a);
    /* 151F mov a,#$20 */
    spc.a = spc_nz(0x20);
    /* 1521 mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 1523 addw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 1525 mov.w $054C,a */
    spc_st(0x054C, spc.a);
    /* 1528 mov.w $054E,y */
    spc_st(0x054E, spc.y);
L152B:
    /* 152B ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F511 SpcBPitchRampB1Down
 * WHAT IT DOES: Counts $0547 down, taking $20 from the offset $054D/$054F; at 0 clears bits 2 and 3 of $28 and the offset. The race driver's SpcPitchRampBDown.
 * HOW: SUBW through $20/$21.
 * Entry: none. Exit: A = 0 at the end.
 * @spc 152C
 * @spcuses 152C-1558
 */
void abi_SpcBPitchRampB1Down(void)
{
    /* 152C mov.w a,$0547 */
    spc.a = spc_nz(spc_rd(0x0547));
    /* 152F cmp a,#$00 */
    spc_cmp(spc.a, 0x00);
    /* 1531 beq $154B */
    if (spc.z) goto L154B;
    /* 1533 dec.w $0547 */
    spc_rmw(0x0547, spc_dec);
    /* 1536 mov.w a,$054D */
    spc.a = spc_nz(spc_rd(0x054D));
    /* 1539 mov.w y,$054F */
    spc.y = spc_nz(spc_rd(0x054F));
    /* 153C mov $20,#$20 */
    spc_st(spc_dp(0x20), 0x20);
    /* 153F mov $21,#$00 */
    spc_st(spc_dp(0x21), 0x00);
    /* 1542 subw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 1544 mov.w $054D,a */
    spc_st(0x054D, spc.a);
    /* 1547 mov.w $054F,y */
    spc_st(0x054F, spc.y);
    /* 154A ret */
    spc_ret();
    return;
L154B:
    /* 154B clr1 $28.2 */
    spc_bit_set(0x28, 2, 0);
    /* 154D clr1 $28.3 */
    spc_bit_set(0x28, 3, 0);
    /* 154F mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1551 mov.w $054D,a */
    spc_st(0x054D, spc.a);
    /* 1554 mov.w $054F,a */
    spc_st(0x054F, spc.a);
    /* 1557 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F53D SpcBPitchRampB2Down
 * WHAT IT DOES: Counts $0546 down, taking $20 from the offset $054C/$054E; at 0 clears bits 4 and 5 of $28 and the offset.
 * HOW: SUBW through $20/$21.
 * Entry: none. Exit: A = 0 at the end.
 * @spc 1558
 * @spcuses 1558-1584
 */
void abi_SpcBPitchRampB2Down(void)
{
    /* 1558 mov.w a,$0546 */
    spc.a = spc_nz(spc_rd(0x0546));
    /* 155B cmp a,#$00 */
    spc_cmp(spc.a, 0x00);
    /* 155D beq $1577 */
    if (spc.z) goto L1577;
    /* 155F dec.w $0546 */
    spc_rmw(0x0546, spc_dec);
    /* 1562 mov.w a,$054C */
    spc.a = spc_nz(spc_rd(0x054C));
    /* 1565 mov.w y,$054E */
    spc.y = spc_nz(spc_rd(0x054E));
    /* 1568 mov $20,#$20 */
    spc_st(spc_dp(0x20), 0x20);
    /* 156B mov $21,#$00 */
    spc_st(spc_dp(0x21), 0x00);
    /* 156E subw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 1570 mov.w $054C,a */
    spc_st(0x054C, spc.a);
    /* 1573 mov.w $054E,y */
    spc_st(0x054E, spc.y);
    /* 1576 ret */
    spc_ret();
    return;
L1577:
    /* 1577 clr1 $28.4 */
    spc_bit_set(0x28, 4, 0);
    /* 1579 clr1 $28.5 */
    spc_bit_set(0x28, 5, 0);
    /* 157B mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 157D mov.w $054C,a */
    spc_st(0x054C, spc.a);
    /* 1580 mov.w $054E,a */
    spc_st(0x054E, spc.a);
    /* 1583 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F569 SpcBPitchRampA
 * WHAT IT DOES: The first pair of pitch ramps: bit 2 of $29 ramps the offset $0549/$054B up (SpcBPitchRampA1Up), bit 3 down (SpcBPitchRampA1Down); bits 4 and 5 do the same to $0548/$054A (SpcBPitchRampA2Up, SpcBPitchRampA2Down). The race driver (SpcPitchRampA) has only the first.
 * HOW: BBC tests.
 * Entry: none. Exit: as the last ramp leaves it.
 * @spc 1584
 * @spcuses 1584-1637
 */
void abi_SpcBPitchRampA(void)
{
    /* 1584 bbc $29.2,$158A */
    if (!(spc_rd(spc_dp(0x29)) >> 2 & 1)) goto L158A;
    /* 1587 call $159D */
    spc_call(abi_SpcBPitchRampA1Up, 0x158A);
L158A:
    /* 158A bbc $29.3,$1590 */
    if (!(spc_rd(spc_dp(0x29)) >> 3 & 1)) goto L1590;
    /* 158D call $15BE */
    spc_call(abi_SpcBPitchRampA1Down, 0x1590);
L1590:
    /* 1590 bbc $29.4,$1596 */
    if (!(spc_rd(spc_dp(0x29)) >> 4 & 1)) goto L1596;
    /* 1593 call $15EA */
    spc_call(abi_SpcBPitchRampA2Up, 0x1596);
L1596:
    /* 1596 bbc $29.5,$159C */
    if (!(spc_rd(spc_dp(0x29)) >> 5 & 1)) goto L159C;
    /* 1599 call $160B */
    spc_call(abi_SpcBPitchRampA2Down, 0x159C);
L159C:
    /* 159C ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F582 SpcBPitchRampA1Up
 * WHAT IT DOES: Up to $19 steps (count $0543), adds $3C to the offset $0549/$054B. The race driver's SpcPitchRampAUp.
 * HOW: ADDW through $20/$21.
 * Entry: none. Exit: YA = the offset.
 * @spc 159D
 * @spcuses 159D-15BE
 */
void abi_SpcBPitchRampA1Up(void)
{
    /* 159D mov.w a,$0543 */
    spc.a = spc_nz(spc_rd(0x0543));
    /* 15A0 cmp a,#$19 */
    spc_cmp(spc.a, 0x19);
    /* 15A2 beq $15BD */
    if (spc.z) goto L15BD;
    /* 15A4 inc.w $0543 */
    spc_rmw(0x0543, spc_inc);
    /* 15A7 mov.w a,$0549 */
    spc.a = spc_nz(spc_rd(0x0549));
    /* 15AA mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 15AC mov.w a,$054B */
    spc.a = spc_nz(spc_rd(0x054B));
    /* 15AF mov.b $21,a */
    spc_st(spc_dp(0x21), spc.a);
    /* 15B1 mov a,#$3C */
    spc.a = spc_nz(0x3C);
    /* 15B3 mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 15B5 addw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 15B7 mov.w $0549,a */
    spc_st(0x0549, spc.a);
    /* 15BA mov.w $054B,y */
    spc_st(0x054B, spc.y);
L15BD:
    /* 15BD ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F5A3 SpcBPitchRampA1Down
 * WHAT IT DOES: Counts $0543 down, taking $3C from the offset $0549/$054B; at 0 ends the ramp (SpcBPitchRampA1End). The race driver's SpcPitchRampADown.
 * HOW: SUBW through $20/$21; a branch into SpcBPitchRampA1End.
 * Entry: none. Exit: YA = the offset, or as SpcBPitchRampA1End.
 * @spc 15BE
 * @spcuses 15BE-15EA
 */
void abi_SpcBPitchRampA1Down(void)
{
    /* 15BE mov.w a,$0543 */
    spc.a = spc_nz(spc_rd(0x0543));
    /* 15C1 cmp a,#$00 */
    spc_cmp(spc.a, 0x00);
    /* 15C3 beq $15DD */
    if (spc.z) { abi_SpcBPitchRampA1End(); return; }
    /* 15C5 dec.w $0543 */
    spc_rmw(0x0543, spc_dec);
    /* 15C8 mov.w a,$0549 */
    spc.a = spc_nz(spc_rd(0x0549));
    /* 15CB mov.w y,$054B */
    spc.y = spc_nz(spc_rd(0x054B));
    /* 15CE mov $20,#$3C */
    spc_st(spc_dp(0x20), 0x3C);
    /* 15D1 mov $21,#$00 */
    spc_st(spc_dp(0x21), 0x00);
    /* 15D4 subw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 15D6 mov.w $0549,a */
    spc_st(0x0549, spc.a);
    /* 15D9 mov.w $054B,y */
    spc_st(0x054B, spc.y);
    /* 15DC ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F5C2 SpcBPitchRampA1End
 * WHAT IT DOES: Ends the first ramp: clears bits 2 and 3 of $29 and the offset $0549/$054B.
 * HOW: CLR1, then zero stores.
 * Entry: none. Exit: A = 0.
 * @spc 15DD
 * @spcuses 15DD-15EA
 */
void abi_SpcBPitchRampA1End(void)
{
    /* 15DD clr1 $29.2 */
    spc_bit_set(0x29, 2, 0);
    /* 15DF clr1 $29.3 */
    spc_bit_set(0x29, 3, 0);
    /* 15E1 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 15E3 mov.w $0549,a */
    spc_st(0x0549, spc.a);
    /* 15E6 mov.w $054B,a */
    spc_st(0x054B, spc.a);
    /* 15E9 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F5CF SpcBPitchRampA2Up
 * WHAT IT DOES: Up to $19 steps (count $0542), adds $3C to the offset $0548/$054A.
 * HOW: ADDW through $20/$21.
 * Entry: none. Exit: YA = the offset.
 * @spc 15EA
 * @spcuses 15EA-160B
 */
void abi_SpcBPitchRampA2Up(void)
{
    /* 15EA mov.w a,$0542 */
    spc.a = spc_nz(spc_rd(0x0542));
    /* 15ED cmp a,#$19 */
    spc_cmp(spc.a, 0x19);
    /* 15EF beq $160A */
    if (spc.z) goto L160A;
    /* 15F1 inc.w $0542 */
    spc_rmw(0x0542, spc_inc);
    /* 15F4 mov.w a,$0548 */
    spc.a = spc_nz(spc_rd(0x0548));
    /* 15F7 mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 15F9 mov.w a,$054A */
    spc.a = spc_nz(spc_rd(0x054A));
    /* 15FC mov.b $21,a */
    spc_st(spc_dp(0x21), spc.a);
    /* 15FE mov a,#$3C */
    spc.a = spc_nz(0x3C);
    /* 1600 mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 1602 addw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 1604 mov.w $0548,a */
    spc_st(0x0548, spc.a);
    /* 1607 mov.w $054A,y */
    spc_st(0x054A, spc.y);
L160A:
    /* 160A ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F5F0 SpcBPitchRampA2Down
 * WHAT IT DOES: Counts $0542 down, taking $3C from the offset $0548/$054A; at 0 clears bits 4 and 5 of $29 and the offset.
 * HOW: SUBW through $20/$21.
 * Entry: none. Exit: A = 0 at the end.
 * @spc 160B
 * @spcuses 160B-1637
 */
void abi_SpcBPitchRampA2Down(void)
{
    /* 160B mov.w a,$0542 */
    spc.a = spc_nz(spc_rd(0x0542));
    /* 160E cmp a,#$00 */
    spc_cmp(spc.a, 0x00);
    /* 1610 beq $162A */
    if (spc.z) goto L162A;
    /* 1612 dec.w $0542 */
    spc_rmw(0x0542, spc_dec);
    /* 1615 mov.w a,$0548 */
    spc.a = spc_nz(spc_rd(0x0548));
    /* 1618 mov.w y,$054A */
    spc.y = spc_nz(spc_rd(0x054A));
    /* 161B mov $20,#$3C */
    spc_st(spc_dp(0x20), 0x3C);
    /* 161E mov $21,#$00 */
    spc_st(spc_dp(0x21), 0x00);
    /* 1621 subw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 1623 mov.w $0548,a */
    spc_st(0x0548, spc.a);
    /* 1626 mov.w $054A,y */
    spc_st(0x054A, spc.y);
    /* 1629 ret */
    spc_ret();
    return;
L162A:
    /* 162A clr1 $29.4 */
    spc_bit_set(0x29, 4, 0);
    /* 162C clr1 $29.5 */
    spc_bit_set(0x29, 5, 0);
    /* 162E mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1630 mov.w $0548,a */
    spc_st(0x0548, spc.a);
    /* 1633 mov.w $054A,a */
    spc_st(0x054A, spc.a);
    /* 1636 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F61C SpcBPortCommand
 * WHAT IT DOES: The port 0 command $00 (bit 7 dropped), as the race driver's SpcPortCommand: 6 and up is a sound command, $1D = value - 5; 1-5 (unless stopped, bit 0 of $29) is a song request, which also clears the tempo bend and fade state and flags 0, 1, 6 of $29 and 2, 3 of $2C; 0 leaves $1D = 0.
 * HOW: SBC #6 and BMI.
 * Entry: none. Exit: A as stored.
 * @spc 1637
 * @spcuses 1637-1671
 */
void abi_SpcBPortCommand(void)
{
    /* 1637 mov $1D,#$00 */
    spc_st(spc_dp(0x1D), 0x00);
    /* 163A mov.b a,$00 */
    spc.a = spc_nz(spc_rd(spc_dp(0x00)));
    /* 163C and a,#$7F */
    spc.a = spc_and(spc.a, 0x7F);
    /* 163E beq $1649 */
    if (spc.z) goto L1649;
    /* 1640 setc */
    spc.c = 1;
    /* 1641 sbc a,#$06 */
    spc.a = spc_sbc(spc.a, 0x06);
    /* 1643 bmi $164C */
    if (spc.n) goto L164C;
    /* 1645 inc a */
    spc.a = spc_inc(spc.a);
    /* 1646 mov.b $1D,a */
    spc_st(spc_dp(0x1D), spc.a);
    /* 1648 ret */
    spc_ret();
    return;
L1649:
    /* 1649 mov.b $1D,a */
    spc_st(spc_dp(0x1D), spc.a);
    /* 164B ret */
    spc_ret();
    return;
L164C:
    /* 164C bbs $29.0,$1670 */
    if ((spc_rd(spc_dp(0x29)) >> 0 & 1)) goto L1670;
    /* 164F mov.b a,$00 */
    spc.a = spc_nz(spc_rd(spc_dp(0x00)));
    /* 1651 mov.b $1B,a */
    spc_st(spc_dp(0x1B), spc.a);
    /* 1653 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    /* 1655 bne $1657 */
    if (!spc.z) goto L1657;
L1657:
    /* 1657 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1659 mov.b $2E,a */
    spc_st(spc_dp(0x2E), spc.a);
    /* 165B mov.b $2F,a */
    spc_st(spc_dp(0x2F), spc.a);
    /* 165D mov.w $0553,a */
    spc_st(0x0553, spc.a);
    /* 1660 mov.w $0554,a */
    spc_st(0x0554, spc.a);
    /* 1663 mov.w $0555,a */
    spc_st(0x0555, spc.a);
    /* 1666 clr1 $29.0 */
    spc_bit_set(0x29, 0, 0);
    /* 1668 clr1 $29.1 */
    spc_bit_set(0x29, 1, 0);
    /* 166A clr1 $29.6 */
    spc_bit_set(0x29, 6, 0);
    /* 166C clr1 $2C.2 */
    spc_bit_set(0x2C, 2, 0);
    /* 166E clr1 $2C.3 */
    spc_bit_set(0x2C, 3, 0);
L1670:
    /* 1670 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F656 SpcBPort1Command
 * WHAT IT DOES: A new port 1 command ($01 differing from the last, $09; below $3E, as $20) runs its handler from the 61-entry table at $1696. The battle's handlers: 9, $0D, $12, $14, $18, $19 start a looping sound ($25, $15, $17, $A5, $95, $97; a bit of $2A) unless bit 6 of $2D is set; $0A-$0C and $1A-$1C end one (the bit of $2A cleared, and channel 5's stop requested with reason $20 while it plays $25, $15 or $17; SpcBStaggeredChecks enters these with its own $20); $23-$26 start the first ramps up or down ($0200 on $0549/$054B or $0548/$054A, bits 2-5 of $29), $3A-$3D the second ($0040 on $054D/$054F or $054C/$054E, bits 2-5 of $28); $32-$35 set $0530 and $36-$39 set $0531 to the command's offset (0-3); 5 and 6 (in SpcBKeyOffAll), $15 (in SpcBSfxPointer), $16 and $22 (in SpcBSfxStop) stop, pause and resume; the rest do nothing. The race driver's (SpcPort1Command) has other commands.
 * HOW: JMP ($1696+X) with X = (command - 1) * 2; the handlers end in RETs or tail jumps; the stop paths share the request at $1861 with SpcBSfxStop.
 * Entry: none. Exit: as the handler leaves it.
 * @spc 1671
 * @spcuses 0A3B-0A3C,0A58-0A6D,12FA-130C,15DD-15EA,162A-1637,1670-19BC,19F5-1A02,1A0B-1A2F,1AFD-1B03,1B17-1B7E,1C04-1C1A,1CCC-1CE5,1D50-1D5A,2087-20B6
 */
void abi_SpcBPort1Command(void)
{
    /* 1671 mov.b a,$01 */
    spc.a = spc_nz(spc_rd(spc_dp(0x01)));
    /* 1673 cmp.b a,$09 */
    spc_cmp(spc.a, spc_rd(spc_dp(0x09)));
    /* 1675 bne $167C */
    if (!spc.z) goto L167C;
    /* 1677 mov $20,#$00 */
    spc_st(spc_dp(0x20), 0x00);
    /* 167A bra $167F */
    goto L167F;
L167C:
    /* 167C mov $20,$01 */
    spc_wr(spc_dp(0x20), spc_rd(spc_dp(0x01)));
L167F:
    /* 167F mov $09,$01 */
    spc_wr(spc_dp(0x09), spc_rd(spc_dp(0x01)));
    /* 1682 mov.b a,$20 */
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 1684 setc */
    spc.c = 1;
    /* 1685 sbc a,#$3E */
    spc.a = spc_sbc(spc.a, 0x3E);
    /* 1687 bmi $168C */
    if (spc.n) goto L168C;
    /* 1689 mov $20,#$00 */
    spc_st(spc_dp(0x20), 0x00);
L168C:
    /* 168C mov.b a,$20 */
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 168E beq $1670 */
    if (spc.z) goto L1670;
    /* 1690 dec a */
    spc.a = spc_dec(spc.a);
    /* 1691 asl a */
    spc.a = spc_asl(spc.a);
    /* 1692 mov x,a */
    spc.x = spc_nz(spc.a);
    /* 1693 jmp ($1696+x) */
    { uint16_t t = spc_rd((uint16_t)(0x1696 + spc.x)); t |= spc_rd((uint16_t)(0x1696 + spc.x + 1)) << 8;
      switch (t) {
      case 0x1710: goto L1710;
      case 0x171B: goto L171B;
      case 0x1726: goto L1726;
      case 0x1731: goto L1731;
      case 0x173C: goto L173C;
      case 0x1747: goto L1747;
      case 0x1752: goto L1752;
      case 0x1764: goto L1764;
      case 0x1780: goto L1780;
      case 0x1792: goto L1792;
      case 0x17AE: goto L17AE;
      case 0x17AF: goto L17AF;
      case 0x17C1: goto L17C1;
      case 0x17D3: goto L17D3;
      case 0x17EF: goto L17EF;
      case 0x180B: goto L180B;
      case 0x1814: goto L1814;
      case 0x181D: goto L181D;
      case 0x1828: goto L1828;
      case 0x1833: goto L1833;
      case 0x1840: goto L1840;
      case 0x184B: goto L184B;
      case 0x1856: goto L1856;
      case 0x1871: goto L1871;
      case 0x18A6: goto L18A6;
      case 0x1917: goto L1917;
      case 0x1A0B: goto L1A0B;
      case 0x1A15: goto L1A15;
      } }
    return;
L1710:
    /* 1710 bbs $2D.6,$171A */
    if ((spc_rd(spc_dp(0x2D)) >> 6 & 1)) goto L171A;
    /* 1713 set1 $2A.2 */
    spc_bit_set(0x2A, 2, 1);
    /* 1715 mov a,#$25 */
    spc.a = spc_nz(0x25);
    /* 1717 call $12FA */
    spc_call(abi_SpcBSoundQueuePush, 0x171A);
L171A:
    /* 171A ret */
    spc_ret();
    return;
L171B:
    /* 171B bbs $2D.6,$1725 */
    if ((spc_rd(spc_dp(0x2D)) >> 6 & 1)) goto L1725;
    /* 171E set1 $2A.4 */
    spc_bit_set(0x2A, 4, 1);
    /* 1720 mov a,#$15 */
    spc.a = spc_nz(0x15);
    /* 1722 call $12FA */
    spc_call(abi_SpcBSoundQueuePush, 0x1725);
L1725:
    /* 1725 ret */
    spc_ret();
    return;
L1726:
    /* 1726 bbs $2D.6,$1730 */
    if ((spc_rd(spc_dp(0x2D)) >> 6 & 1)) goto L1730;
    /* 1729 set1 $2A.0 */
    spc_bit_set(0x2A, 0, 1);
    /* 172B mov a,#$17 */
    spc.a = spc_nz(0x17);
    /* 172D call $12FA */
    spc_call(abi_SpcBSoundQueuePush, 0x1730);
L1730:
    /* 1730 ret */
    spc_ret();
    return;
L1731:
    /* 1731 bbs $2D.6,$173B */
    if ((spc_rd(spc_dp(0x2D)) >> 6 & 1)) goto L173B;
    /* 1734 set1 $2A.3 */
    spc_bit_set(0x2A, 3, 1);
    /* 1736 mov a,#$A5 */
    spc.a = spc_nz(0xA5);
    /* 1738 call $12FA */
    spc_call(abi_SpcBSoundQueuePush, 0x173B);
L173B:
    /* 173B ret */
    spc_ret();
    return;
L173C:
    /* 173C bbs $2D.6,$1746 */
    if ((spc_rd(spc_dp(0x2D)) >> 6 & 1)) goto L1746;
    /* 173F set1 $2A.5 */
    spc_bit_set(0x2A, 5, 1);
    /* 1741 mov a,#$95 */
    spc.a = spc_nz(0x95);
    /* 1743 call $12FA */
    spc_call(abi_SpcBSoundQueuePush, 0x1746);
L1746:
    /* 1746 ret */
    spc_ret();
    return;
L1747:
    /* 1747 bbs $2D.6,$1751 */
    if ((spc_rd(spc_dp(0x2D)) >> 6 & 1)) goto L1751;
    /* 174A set1 $2A.1 */
    spc_bit_set(0x2A, 1, 1);
    /* 174C mov a,#$97 */
    spc.a = spc_nz(0x97);
    /* 174E call $12FA */
    spc_call(abi_SpcBSoundQueuePush, 0x1751);
L1751:
    /* 1751 ret */
    spc_ret();
    return;
L1752:
    /* 1752 bbs $29.2,$17AE */
    if ((spc_rd(spc_dp(0x29)) >> 2 & 1)) goto L17AE;
    /* 1755 set1 $29.2 */
    spc_bit_set(0x29, 2, 1);
    /* 1757 clr1 $29.3 */
    spc_bit_set(0x29, 3, 0);
    /* 1759 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 175B mov y,#$02 */
    spc.y = spc_nz(0x02);
    /* 175D mov.w $0549,a */
    spc_st(0x0549, spc.a);
    /* 1760 mov.w $054B,y */
    spc_st(0x054B, spc.y);
    /* 1763 ret */
    spc_ret();
    return;
L1764:
    /* 1764 bbc $29.2,$17AE */
    if (!(spc_rd(spc_dp(0x29)) >> 2 & 1)) goto L17AE;
    /* 1767 clr1 $29.2 */
    spc_bit_set(0x29, 2, 0);
    /* 1769 set1 $29.3 */
    spc_bit_set(0x29, 3, 1);
    /* 176B mov.w a,$0549 */
    spc.a = spc_nz(spc_rd(0x0549));
    /* 176E mov.w y,$054B */
    spc.y = spc_nz(spc_rd(0x054B));
    /* 1771 mov $20,#$00 */
    spc_st(spc_dp(0x20), 0x00);
    /* 1774 mov $21,#$02 */
    spc_st(spc_dp(0x21), 0x02);
    /* 1777 subw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 1779 mov.w $0549,a */
    spc_st(0x0549, spc.a);
    /* 177C mov.w $054B,y */
    spc_st(0x054B, spc.y);
    /* 177F ret */
    spc_ret();
    return;
L1780:
    /* 1780 bbs $29.4,$17AE */
    if ((spc_rd(spc_dp(0x29)) >> 4 & 1)) goto L17AE;
    /* 1783 set1 $29.4 */
    spc_bit_set(0x29, 4, 1);
    /* 1785 clr1 $29.5 */
    spc_bit_set(0x29, 5, 0);
    /* 1787 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1789 mov y,#$02 */
    spc.y = spc_nz(0x02);
    /* 178B mov.w $0548,a */
    spc_st(0x0548, spc.a);
    /* 178E mov.w $054A,y */
    spc_st(0x054A, spc.y);
    /* 1791 ret */
    spc_ret();
    return;
L1792:
    /* 1792 bbc $29.4,$17AE */
    if (!(spc_rd(spc_dp(0x29)) >> 4 & 1)) goto L17AE;
    /* 1795 clr1 $29.4 */
    spc_bit_set(0x29, 4, 0);
    /* 1797 set1 $29.5 */
    spc_bit_set(0x29, 5, 1);
    /* 1799 mov.w a,$0548 */
    spc.a = spc_nz(spc_rd(0x0548));
    /* 179C mov.w y,$054A */
    spc.y = spc_nz(spc_rd(0x054A));
    /* 179F mov $20,#$00 */
    spc_st(spc_dp(0x20), 0x00);
    /* 17A2 mov $21,#$02 */
    spc_st(spc_dp(0x21), 0x02);
    /* 17A5 subw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 17A7 mov.w $0548,a */
    spc_st(0x0548, spc.a);
    /* 17AA mov.w $054A,y */
    spc_st(0x054A, spc.y);
    /* 17AD ret */
    spc_ret();
    return;
L17AE:
    /* 17AE ret */
    spc_ret();
    return;
L17AF:
    /* 17AF bbs $28.2,$17AE */
    if ((spc_rd(spc_dp(0x28)) >> 2 & 1)) goto L17AE;
    /* 17B2 set1 $28.2 */
    spc_bit_set(0x28, 2, 1);
    /* 17B4 clr1 $28.3 */
    spc_bit_set(0x28, 3, 0);
    /* 17B6 mov a,#$40 */
    spc.a = spc_nz(0x40);
    /* 17B8 mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 17BA mov.w $054D,a */
    spc_st(0x054D, spc.a);
    /* 17BD mov.w $054F,y */
    spc_st(0x054F, spc.y);
    /* 17C0 ret */
    spc_ret();
    return;
L17C1:
    /* 17C1 bbs $28.4,$17AE */
    if ((spc_rd(spc_dp(0x28)) >> 4 & 1)) goto L17AE;
    /* 17C4 set1 $28.4 */
    spc_bit_set(0x28, 4, 1);
    /* 17C6 clr1 $28.5 */
    spc_bit_set(0x28, 5, 0);
    /* 17C8 mov a,#$40 */
    spc.a = spc_nz(0x40);
    /* 17CA mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 17CC mov.w $054C,a */
    spc_st(0x054C, spc.a);
    /* 17CF mov.w $054E,y */
    spc_st(0x054E, spc.y);
    /* 17D2 ret */
    spc_ret();
    return;
L17D3:
    /* 17D3 bbc $28.2,$17AE */
    if (!(spc_rd(spc_dp(0x28)) >> 2 & 1)) goto L17AE;
    /* 17D6 clr1 $28.2 */
    spc_bit_set(0x28, 2, 0);
    /* 17D8 set1 $28.3 */
    spc_bit_set(0x28, 3, 1);
    /* 17DA mov.w a,$054D */
    spc.a = spc_nz(spc_rd(0x054D));
    /* 17DD mov.w y,$054F */
    spc.y = spc_nz(spc_rd(0x054F));
    /* 17E0 mov $20,#$40 */
    spc_st(spc_dp(0x20), 0x40);
    /* 17E3 mov $21,#$00 */
    spc_st(spc_dp(0x21), 0x00);
    /* 17E6 subw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 17E8 mov.w $054D,a */
    spc_st(0x054D, spc.a);
    /* 17EB mov.w $054F,y */
    spc_st(0x054F, spc.y);
    /* 17EE ret */
    spc_ret();
    return;
L17EF:
    /* 17EF bbc $28.4,$17AE */
    if (!(spc_rd(spc_dp(0x28)) >> 4 & 1)) goto L17AE;
    /* 17F2 clr1 $28.4 */
    spc_bit_set(0x28, 4, 0);
    /* 17F4 set1 $28.5 */
    spc_bit_set(0x28, 5, 1);
    /* 17F6 mov.w a,$054C */
    spc.a = spc_nz(spc_rd(0x054C));
    /* 17F9 mov.w y,$054E */
    spc.y = spc_nz(spc_rd(0x054E));
    /* 17FC mov $20,#$40 */
    spc_st(spc_dp(0x20), 0x40);
    /* 17FF mov $21,#$00 */
    spc_st(spc_dp(0x21), 0x00);
    /* 1802 subw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 1804 mov.w $054C,a */
    spc_st(0x054C, spc.a);
    /* 1807 mov.w $054E,y */
    spc_st(0x054E, spc.y);
    /* 180A ret */
    spc_ret();
    return;
L180B:
    /* 180B mov.b a,$20 */
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 180D setc */
    spc.c = 1;
    /* 180E sbc a,#$32 */
    spc.a = spc_sbc(spc.a, 0x32);
    /* 1810 mov.w $0530,a */
    spc_st(0x0530, spc.a);
    /* 1813 ret */
    spc_ret();
    return;
L1814:
    /* 1814 mov.b a,$20 */
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 1816 setc */
    spc.c = 1;
    /* 1817 sbc a,#$36 */
    spc.a = spc_sbc(spc.a, 0x36);
    /* 1819 mov.w $0531,a */
    spc_st(0x0531, spc.a);
    /* 181C ret */
    spc_ret();
    return;
L181D:
    /* 181D clr1 $2A.2 */
    spc_bit_set(0x2A, 2, 0);
    /* 181F mov.w a,$0395 */
    spc.a = spc_nz(spc_rd(0x0395));
    /* 1822 cmp a,#$25 */
    spc_cmp(spc.a, 0x25);
    /* 1824 bne $186B */
    if (!spc.z) goto L186B;
    /* 1826 bra $183C */
    goto L183C;
L1828:
    /* 1828 clr1 $2A.4 */
    spc_bit_set(0x2A, 4, 0);
    /* 182A mov.w a,$0395 */
    spc.a = spc_nz(spc_rd(0x0395));
    /* 182D cmp a,#$15 */
    spc_cmp(spc.a, 0x15);
    /* 182F bne $186B */
    if (!spc.z) goto L186B;
    /* 1831 bra $183C */
    goto L183C;
L1833:
    /* 1833 clr1 $2A.0 */
    spc_bit_set(0x2A, 0, 0);
    /* 1835 mov.w a,$0395 */
    spc.a = spc_nz(spc_rd(0x0395));
    /* 1838 cmp a,#$17 */
    spc_cmp(spc.a, 0x17);
    /* 183A bne $186B */
    if (!spc.z) goto L186B;
L183C:
    /* 183C mov x,#$05 */
    spc.x = spc_nz(0x05);
    /* 183E bra $1861 */
    goto L1861;
L1840:
    /* 1840 clr1 $2A.3 */
    spc_bit_set(0x2A, 3, 0);
    /* 1842 mov.w a,$0395 */
    spc.a = spc_nz(spc_rd(0x0395));
    /* 1845 cmp a,#$25 */
    spc_cmp(spc.a, 0x25);
    /* 1847 bne $186B */
    if (!spc.z) goto L186B;
    /* 1849 bra $185F */
    goto L185F;
L184B:
    /* 184B clr1 $2A.5 */
    spc_bit_set(0x2A, 5, 0);
    /* 184D mov.w a,$0395 */
    spc.a = spc_nz(spc_rd(0x0395));
    /* 1850 cmp a,#$15 */
    spc_cmp(spc.a, 0x15);
    /* 1852 bne $186B */
    if (!spc.z) goto L186B;
    /* 1854 bra $185F */
    goto L185F;
L1856:
    /* 1856 clr1 $2A.1 */
    spc_bit_set(0x2A, 1, 0);
    /* 1858 mov.w a,$0395 */
    spc.a = spc_nz(spc_rd(0x0395));
    /* 185B cmp a,#$17 */
    spc_cmp(spc.a, 0x17);
    /* 185D bne $186B */
    if (!spc.z) goto L186B;
L185F:
    /* 185F mov x,#$05 */
    spc.x = spc_nz(0x05);
L1861:
    /* 1861 mov.w a,$0408+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 1864 beq $186B */
    if (spc.z) goto L186B;
    /* 1866 mov.b a,$20 */
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 1868 mov.w $03E0+x,a */
    spc_st((uint16_t)(0x03E0 + spc.x), spc.a);
L186B:
    /* 186B ret */
    spc_ret();
    return;
L1871:
    /* 1871 bbc $29.0,$186B */
    if (!(spc_rd(spc_dp(0x29)) >> 0 & 1)) goto L186B;
    /* 1874 mov a,#$4C */
    spc.a = spc_nz(0x4C);
    /* 1876 call $12FA */
    spc_call(abi_SpcBSoundQueuePush, 0x1879);
    /* 1879 clr1 $29.0 */
    spc_bit_set(0x29, 0, 0);
    /* 187B mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 187D mov.w $045F,a */
    spc_st(0x045F, spc.a);
    /* 1880 mov.w $045E,a */
    spc_st(0x045E, spc.a);
    /* 1883 mov.w $045D,a */
    spc_st(0x045D, spc.a);
    /* 1886 mov.w $045C,a */
    spc_st(0x045C, spc.a);
    /* 1889 mov.w $04FF,a */
    spc_st(0x04FF, spc.a);
    /* 188C mov.w $04FE,a */
    spc_st(0x04FE, spc.a);
    /* 188F mov.w $04FD,a */
    spc_st(0x04FD, spc.a);
    /* 1892 mov.w $04FC,a */
    spc_st(0x04FC, spc.a);
    /* 1895 clr1 $29.2 */
    spc_bit_set(0x29, 2, 0);
    /* 1897 clr1 $29.3 */
    spc_bit_set(0x29, 3, 0);
    /* 1899 clr1 $29.4 */
    spc_bit_set(0x29, 4, 0);
    /* 189B clr1 $29.5 */
    spc_bit_set(0x29, 5, 0);
    /* 189D call $0A60 */
    spc_call(abi_SpcMusicPause, 0x18A0);
    /* 18A0 call $15DD */
    spc_call(abi_SpcBPitchRampA1End, 0x18A3);
    /* 18A3 jmp $162A */
    goto L162A;
L18A6:
    /* 18A6 bbc $29.0,$186B */
    if (!(spc_rd(spc_dp(0x29)) >> 0 & 1)) goto L186B;
    /* 18A9 mov $2A,$2B */
    spc_wr(spc_dp(0x2A), spc_rd(spc_dp(0x2B)));
    /* 18AC mov a,#$4B */
    spc.a = spc_nz(0x4B);
    /* 18AE call $12FA */
    spc_call(abi_SpcBSoundQueuePush, 0x18B1);
    /* 18B1 clr1 $29.0 */
    spc_bit_set(0x29, 0, 0);
    /* 18B3 clr1 $2D.6 */
    spc_bit_set(0x2D, 6, 0);
    /* 18B5 mov x,#$07 */
    spc.x = spc_nz(0x07);
    /* 18B7 call $18D3 */
    spc_call(abi_SpcBSfxChannelResume, 0x18BA);
    /* 18BA mov x,#$06 */
    spc.x = spc_nz(0x06);
    /* 18BC call $18D3 */
    spc_call(abi_SpcBSfxChannelResume, 0x18BF);
    /* 18BF mov x,#$05 */
    spc.x = spc_nz(0x05);
    /* 18C1 call $18D3 */
    spc_call(abi_SpcBSfxChannelResume, 0x18C4);
    /* 18C4 mov x,#$04 */
    spc.x = spc_nz(0x04);
    /* 18C6 call $18D3 */
    spc_call(abi_SpcBSfxChannelResume, 0x18C9);
    /* 18C9 mov x,#$01 */
    spc.x = spc_nz(0x01);
    /* 18CB call $186C */
    spc_call(abi_SpcBSfxStop, 0x18CE);
    /* 18CE mov x,#$00 */
    spc.x = spc_nz(0x00);
    /* 18D0 jmp $186C */
    { abi_SpcBSfxStop(); return; }
L1917:
    /* 1917 bbs $29.0,$1956 */
    if ((spc_rd(spc_dp(0x29)) >> 0 & 1)) goto L1956;
    /* 191A mov $2B,$2A */
    spc_wr(spc_dp(0x2B), spc_rd(spc_dp(0x2A)));
    /* 191D mov $2A,#$00 */
    spc_st(spc_dp(0x2A), 0x00);
    /* 1920 set1 $29.0 */
    spc_bit_set(0x29, 0, 1);
    /* 1922 set1 $2D.6 */
    spc_bit_set(0x2D, 6, 1);
    /* 1924 mov a,#$4A */
    spc.a = spc_nz(0x4A);
    /* 1926 call $12FA */
    spc_call(abi_SpcBSoundQueuePush, 0x1929);
    /* 1929 mov x,#$07 */
    spc.x = spc_nz(0x07);
    /* 192B call $1951 */
    spc_call(abi_SpcBSfxChannelPause, 0x192E);
    /* 192E mov x,#$06 */
    spc.x = spc_nz(0x06);
    /* 1930 call $1951 */
    spc_call(abi_SpcBSfxChannelPause, 0x1933);
    /* 1933 mov x,#$05 */
    spc.x = spc_nz(0x05);
    /* 1935 call $1951 */
    spc_call(abi_SpcBSfxChannelPause, 0x1938);
    /* 1938 mov x,#$04 */
    spc.x = spc_nz(0x04);
    /* 193A call $1951 */
    spc_call(abi_SpcBSfxChannelPause, 0x193D);
    /* 193D mov x,#$07 */
    spc.x = spc_nz(0x07);
    /* 193F call $186C */
    spc_call(abi_SpcBSfxStop, 0x1942);
    /* 1942 mov x,#$06 */
    spc.x = spc_nz(0x06);
    /* 1944 call $186C */
    spc_call(abi_SpcBSfxStop, 0x1947);
    /* 1947 mov x,#$05 */
    spc.x = spc_nz(0x05);
    /* 1949 call $186C */
    spc_call(abi_SpcBSfxStop, 0x194C);
    /* 194C mov x,#$04 */
    spc.x = spc_nz(0x04);
    /* 194E jmp $186C */
    { abi_SpcBSfxStop(); return; }
L1956:
    /* 1956 mov.w a,$04F8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F8 + spc.x)));
    /* 1959 mov.w $0458+x,a */
    spc_st((uint16_t)(0x0458 + spc.x), spc.a);
    /* 195C mov.w a,$0528+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0528 + spc.x)));
    /* 195F mov.w $0450+x,a */
    spc_st((uint16_t)(0x0450 + spc.x), spc.a);
    /* 1962 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1964 mov.w $04F8+x,a */
    spc_st((uint16_t)(0x04F8 + spc.x), spc.a);
    /* 1967 mov.w $0480+x,a */
    spc_st((uint16_t)(0x0480 + spc.x), spc.a);
    /* 196A mov.w $0528+x,a */
    spc_st((uint16_t)(0x0528 + spc.x), spc.a);
    /* 196D ret */
    spc_ret();
    return;
L19F5:
    /* 19F5 call $19FB */
    spc_call(abi_SpcBKeyOffAll, 0x19F8);
    /* 19F8 jmp $0675 */
    { spc_restart(0x0675, abi_SpcReload); return; }
L1A0B:
    /* 1A0B call $198F */
    spc_call(abi_SpcBSfxStopAll, 0x1A0E);
    /* 1A0E mov.b a,$59 */
    spc.a = spc_nz(spc_rd(spc_dp(0x59)));
    /* 1A10 beq $19F5 */
    if (spc.z) goto L19F5;
    /* 1A12 set1 $2C.7 */
    spc_bit_set(0x2C, 7, 1);
    /* 1A14 ret */
    spc_ret();
    return;
L1A15:
    /* 1A15 set1 $2D.0 */
    spc_bit_set(0x2D, 0, 1);
    /* 1A17 call $198F */
    spc_call(abi_SpcBSfxStopAll, 0x1A1A);
    /* 1A1A mov.b a,$59 */
    spc.a = spc_nz(spc_rd(spc_dp(0x59)));
    /* 1A1C beq $1A21 */
    if (spc.z) goto L1A21;
    /* 1A1E set1 $2D.0 */
    spc_bit_set(0x2D, 0, 1);
    /* 1A20 ret */
    spc_ret();
    return;
L1A21:
    /* 1A21 call $19FB */
    spc_call(abi_SpcBKeyOffAll, 0x1A24);
    /* 1A24 mov.w a,$00F1 */
    spc.a = spc_nz(spc_rd(0x00F1));
    /* 1A27 or a,#$80 */
    spc.a = spc_or(spc.a, 0x80);
    /* 1A29 mov.w $00F1,a */
    spc_st(0x00F1, spc.a);
    /* 1A2C jmp $FFC0 */
    { spc_boot_rom(); return; }
L162A:
    /* 162A clr1 $29.4 */
    spc_bit_set(0x29, 4, 0);
    /* 162C clr1 $29.5 */
    spc_bit_set(0x29, 5, 0);
    /* 162E mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1630 mov.w $0548,a */
    spc_st(0x0548, spc.a);
    /* 1633 mov.w $054A,a */
    spc_st(0x054A, spc.a);
    /* 1636 ret */
    spc_ret();
    return;
L1670:
    /* 1670 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F851 SpcBSfxStop
 * WHAT IT DOES: Requests the stop of channel X (reason 1, $03E0+X) when it is active ($0408+X), as the race driver's SpcSfxStop. After it, two port 1 handlers: $22 (while stopped, bit 0 of $29: queues sound $4C, clears the stop, the saved sounds $045C-$045F and $04FC-$04FF and the ramp flags, pauses the music and ends both first ramps) and $16 (while stopped: $2A back from $2B, queues $4B, clears the stop and bit 6 of $2D, resumes channels 7-4 and stops channels 1 and 0).
 * HOW: $20 = 1, then the request in SpcBPort1Command at $1861; the handlers are CALL chains.
 * Entry: X = channel (0-7). Exit: A changed.
 * @spc 186C
 * @spcuses 1861-1871
 */
void abi_SpcBSfxStop(void)
{
    /* 186C mov $20,#$01 */
    spc_st(spc_dp(0x20), 0x01);
    /* 186F bra $1861 */
    goto L1861;
L1861:
    /* 1861 mov.w a,$0408+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 1864 beq $186B */
    if (spc.z) goto L186B;
    /* 1866 mov.b a,$20 */
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 1868 mov.w $03E0+x,a */
    spc_st((uint16_t)(0x03E0 + spc.x), spc.a);
L186B:
    /* 186B ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F8B8 SpcBSfxChannelResume
 * WHAT IT DOES: Channel X after a pause: by its saved mode $0450 (1 or 2, else nothing) bit 7 of $00 is cleared or set, the mode cleared, its sound number $03C8 back from $0460, and a sound saved at $0458 restarted (SpcBSfxPointer, SpcBSfxStart); without one the channel stops. The race driver's SpcSfxChannelResume has no mode.
 * HOW: Table copies and CALLs; the stop is SpcBSfxStop through the JMP at $1914.
 * Entry: X = channel. Exit: X kept.
 * @spc 18D3
 * @spcuses 1861-1871,18D3-1917,1AFD-1B03,1B17-1B7E,1C04-1C1A,1CCC-1CE5,1D50-1D5A,2087-20B6
 */
void abi_SpcBSfxChannelResume(void)
{
    /* 18D3 mov.w a,$0450+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0450 + spc.x)));
    /* 18D6 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    /* 18D8 beq $18DF */
    if (spc.z) goto L18DF;
    /* 18DA cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    /* 18DC beq $18E3 */
    if (spc.z) goto L18E3;
    /* 18DE ret */
    spc_ret();
    return;
L18DF:
    /* 18DF clr1 $00.7 */
    spc_bit_set(0x00, 7, 0);
    /* 18E1 bra $18E5 */
    goto L18E5;
L18E3:
    /* 18E3 set1 $00.7 */
    spc_bit_set(0x00, 7, 1);
L18E5:
    /* 18E5 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 18E7 mov.w $0450+x,a */
    spc_st((uint16_t)(0x0450 + spc.x), spc.a);
    /* 18EA mov.w a,$0460+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0460 + spc.x)));
    /* 18ED mov.w $03C8+x,a */
    spc_st((uint16_t)(0x03C8 + spc.x), spc.a);
    /* 18F0 mov.w a,$0458+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0458 + spc.x)));
    /* 18F3 beq $1914 */
    if (spc.z) goto L1914;
    /* 18F5 push a */
    spc_push(spc.a);
    /* 18F6 call $1905 */
    spc_call(abi_SpcBSfxPointer, 0x18F9);
    /* 18F9 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 18FB mov.w $0458+x,a */
    spc_st((uint16_t)(0x0458 + spc.x), spc.a);
    /* 18FE pop a */
    spc.a = spc_pull();
    /* 18FF push x */
    spc_push(spc.x);
    /* 1900 call $2087 */
    spc_call(abi_SpcBSfxStart, 0x1903);
    /* 1903 pop x */
    spc.x = spc_pull();
    /* 1904 ret */
    spc_ret();
    return;
L1914:
    /* 1914 jmp $186C */
    { abi_SpcBSfxStop(); return; }
}

/*
 * @routine 1A:F8EA SpcBSfxPointer
 * WHAT IT DOES: The data pointer of sound A from the table at $0702 into $24/$25 (the race driver's SpcSfxPointer). After it, the JMP to SpcBSfxStop that SpcBSfxChannelResume takes, and the port 1 handler $15, the pause: unless already stopped, $2A is saved at $2B and cleared, the stop (bit 0 of $29) and bit 6 of $2D set, sound $4A queued, channels 7-4 saved (SpcBSfxChannelPause) and stopped. When already stopped it branches into SpcBSfxChannelPause's save at $1956 with X = $28, the dispatch index (the race driver's branches to a RET).
 * HOW: (A - 1) * 2 as the index; the handler is a CALL chain.
 * Entry: A = sound (1 and up). Exit: Y = index + 1.
 * @spc 1905
 * @spcuses 1905-1914
 */
void abi_SpcBSfxPointer(void)
{
    /* 1905 dec a */
    spc.a = spc_dec(spc.a);
    /* 1906 asl a */
    spc.a = spc_asl(spc.a);
    /* 1907 mov y,a */
    spc.y = spc_nz(spc.a);
    /* 1908 mov.w a,$0702+y */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0702 + spc.y)));
    /* 190B mov.b $24,a */
    spc_st(spc_dp(0x24), spc.a);
    /* 190D inc y */
    spc.y = spc_inc(spc.y);
    /* 190E mov.w a,$0702+y */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0702 + spc.y)));
    /* 1911 mov.b $25,a */
    spc_st(spc_dp(0x25), spc.a);
    /* 1913 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F936 SpcBSfxChannelPause
 * WHAT IT DOES: Channel X at a pause: a pending queued sound ($04F8) or a running looped one ($0480, mode 1) is saved at $0458 to restart after, with its mode ($0528 or $04B8) at $0450 and, for a looped one, its sound number at $0460. The race driver's SpcSfxChannelPause has no mode.
 * HOW: Compares on $04F8, $0480.
 * Entry: X = channel. Exit: A changed.
 * @spc 1951
 * @spcuses 1951-198F
 */
void abi_SpcBSfxChannelPause(void)
{
    /* 1951 mov.w a,$04F8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F8 + spc.x)));
    /* 1954 beq $196E */
    if (spc.z) goto L196E;
L1956:
    /* 1956 mov.w a,$04F8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F8 + spc.x)));
    /* 1959 mov.w $0458+x,a */
    spc_st((uint16_t)(0x0458 + spc.x), spc.a);
    /* 195C mov.w a,$0528+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0528 + spc.x)));
    /* 195F mov.w $0450+x,a */
    spc_st((uint16_t)(0x0450 + spc.x), spc.a);
    /* 1962 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1964 mov.w $04F8+x,a */
    spc_st((uint16_t)(0x04F8 + spc.x), spc.a);
    /* 1967 mov.w $0480+x,a */
    spc_st((uint16_t)(0x0480 + spc.x), spc.a);
    /* 196A mov.w $0528+x,a */
    spc_st((uint16_t)(0x0528 + spc.x), spc.a);
    /* 196D ret */
    spc_ret();
    return;
L196E:
    /* 196E mov.w a,$0480+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0480 + spc.x)));
    /* 1971 beq $198E */
    if (spc.z) goto L198E;
    /* 1973 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    /* 1975 beq $1956 */
    if (spc.z) goto L1956;
    /* 1977 mov.w a,$04B8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04B8 + spc.x)));
    /* 197A mov.w $0450+x,a */
    spc_st((uint16_t)(0x0450 + spc.x), spc.a);
    /* 197D mov.w a,$03C8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03C8 + spc.x)));
    /* 1980 mov.w $0460+x,a */
    spc_st((uint16_t)(0x0460 + spc.x), spc.a);
    /* 1983 mov.w a,$0390+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1986 mov.w $0458+x,a */
    spc_st((uint16_t)(0x0458 + spc.x), spc.a);
    /* 1989 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 198B mov.w $0480+x,a */
    spc_st((uint16_t)(0x0480 + spc.x), spc.a);
L198E:
    /* 198E ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F974 SpcBSfxStopAll
 * WHAT IT DOES: Stops every sound effect channel with a fade, $2A cleared, and the music fade-out started (SpcBFadeOutStart). The race driver's SpcSfxStopAll.
 * HOW: SpcBSfxStopRequest for X = 7 down to 0.
 * Entry: none. Exit: X = $FF.
 * @spc 198F
 * @spcuses 198F-19BC
 */
void abi_SpcBSfxStopAll(void)
{
    /* 198F mov $2A,#$00 */
    spc_st(spc_dp(0x2A), 0x00);
    /* 1992 call $19B6 */
    spc_call(abi_SpcBFadeOutStart, 0x1995);
    /* 1995 mov x,#$07 */
    spc.x = spc_nz(0x07);
L1997:
    /* 1997 call $199E */
    spc_call(abi_SpcBSfxStopRequest, 0x199A);
    /* 199A dec x */
    spc.x = spc_dec(spc.x);
    /* 199B bpl $1997 */
    if (!spc.n) goto L1997;
    /* 199D ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F983 SpcBSfxStopRequest
 * WHAT IT DOES: Channel X's queued and looping state cleared ($04F8, $0480, $2A) and, if active, its fade-out started ($04C0, $04C8 = 1). The race driver's SpcSfxStopRequest.
 * HOW: Stores.
 * Entry: X = channel. Exit: A = 0 or 1.
 * @spc 199E
 * @spcuses 199E-19B6
 */
void abi_SpcBSfxStopRequest(void)
{
    /* 199E mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 19A0 mov.w $04F8+x,a */
    spc_st((uint16_t)(0x04F8 + spc.x), spc.a);
    /* 19A3 mov.w $0480+x,a */
    spc_st((uint16_t)(0x0480 + spc.x), spc.a);
    /* 19A6 mov.b $2A,a */
    spc_st(spc_dp(0x2A), spc.a);
    /* 19A8 mov.w a,$0408+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 19AB beq $19B5 */
    if (spc.z) goto L19B5;
    /* 19AD mov a,#$01 */
    spc.a = spc_nz(0x01);
    /* 19AF mov.w $04C0+x,a */
    spc_st((uint16_t)(0x04C0 + spc.x), spc.a);
    /* 19B2 mov.w $04C8+x,a */
    spc_st((uint16_t)(0x04C8 + spc.x), spc.a);
L19B5:
    /* 19B5 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F99B SpcBFadeOutStart
 * WHAT IT DOES: Unless stopped (bit 0 of $29), starts the music fade-out (bit 6 of $29). The race driver's SpcFadeOutStart.
 * HOW: SET1.
 * Entry: none. Exit: none.
 * @spc 19B6
 * @spcuses 19B6-19BC
 */
void abi_SpcBFadeOutStart(void)
{
    /* 19B6 bbs $29.0,$19BB */
    if ((spc_rd(spc_dp(0x29)) >> 0 & 1)) goto L19BB;
    /* 19B9 set1 $29.6 */
    spc_bit_set(0x29, 6, 1);
L19BB:
    /* 19BB ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:F9A1 SpcBMusicFade
 * WHAT IT DOES: The music fade, as the race driver's SpcMusicFade: while bit 6 of $29 is set, the fade amount $2E/$2F grows by $2F0 a tick and the volume scale $0565 is the start level $0564 less it; at the bottom the fade ends and, by flags, the driver reloads (bit 7 of $2C: all keys off, SpcReload), returns to the IPL ROM (bit 0 of $2D) or pauses the music. Without a fade, the scale follows the master volume.
 * HOW: SUBW from $0564 in the high byte; the no-fade path and the IPL ROM exit are in SpcBKeyOffAll.
 * Entry: none. Exit: as the chosen path leaves it.
 * @spc 19BC
 * @spcuses 19BC-1A0B,1A21-1A2F
 */
void abi_SpcBMusicFade(void)
{
    /* 19BC bbc $29.6,$1A02 */
    if (!(spc_rd(spc_dp(0x29)) >> 6 & 1)) goto L1A02;
    /* 19BF mov a,#$F0 */
    spc.a = spc_nz(0xF0);
    /* 19C1 mov y,#$02 */
    spc.y = spc_nz(0x02);
    /* 19C3 addw ya,$2E */
    { uint16_t v = spc_rd(spc_dp(0x2E)); v |= spc_rd(spc_dp(0x2F)) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 19C5 mov.b $2E,a */
    spc_st(spc_dp(0x2E), spc.a);
    /* 19C7 mov.b $2F,y */
    spc_st(spc_dp(0x2F), spc.y);
    /* 19C9 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 19CB mov.w y,$0564 */
    spc.y = spc_nz(spc_rd(0x0564));
    /* 19CE subw ya,$2E */
    { uint16_t v = spc_rd(spc_dp(0x2E)); v |= spc_rd(spc_dp(0x2F)) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 19D0 mov.w $0565,y */
    spc_st(0x0565, spc.y);
    /* 19D3 cmp y,#$00 */
    spc_cmp(spc.y, 0x00);
    /* 19D5 beq $19E4 */
    if (spc.z) goto L19E4;
    /* 19D7 cmp y,#$01 */
    spc_cmp(spc.y, 0x01);
    /* 19D9 beq $19E4 */
    if (spc.z) goto L19E4;
    /* 19DB cmp y,#$02 */
    spc_cmp(spc.y, 0x02);
    /* 19DD beq $19E4 */
    if (spc.z) goto L19E4;
    /* 19DF cmp y,#$03 */
    spc_cmp(spc.y, 0x03);
    /* 19E1 beq $19E4 */
    if (spc.z) goto L19E4;
    /* 19E3 ret */
    spc_ret();
    return;
L19E4:
    /* 19E4 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 19E6 mov.b $2E,a */
    spc_st(spc_dp(0x2E), spc.a);
    /* 19E8 mov.b $2F,a */
    spc_st(spc_dp(0x2F), spc.a);
    /* 19EA clr1 $29.6 */
    spc_bit_set(0x29, 6, 0);
    /* 19EC bbs $2C.7,$19F5 */
    if ((spc_rd(spc_dp(0x2C)) >> 7 & 1)) goto L19F5;
    /* 19EF bbs $2D.0,$1A21 */
    if ((spc_rd(spc_dp(0x2D)) >> 0 & 1)) goto L1A21;
    /* 19F2 jmp $0A60 */
    { abi_SpcMusicPause(); return; }
L19F5:
    /* 19F5 call $19FB */
    spc_call(abi_SpcBKeyOffAll, 0x19F8);
    /* 19F8 jmp $0675 */
    { spc_restart(0x0675, abi_SpcReload); return; }
L1A02:
    /* 1A02 mov.b a,$59 */
    spc.a = spc_nz(spc_rd(spc_dp(0x59)));
    /* 1A04 mov.w $0564,a */
    spc_st(0x0564, spc.a);
    /* 1A07 mov.w $0565,a */
    spc_st(0x0565, spc.a);
    /* 1A0A ret */
    spc_ret();
    return;
L1A21:
    /* 1A21 call $19FB */
    spc_call(abi_SpcBKeyOffAll, 0x1A24);
    /* 1A24 mov.w a,$00F1 */
    spc.a = spc_nz(spc_rd(0x00F1));
    /* 1A27 or a,#$80 */
    spc.a = spc_or(spc.a, 0x80);
    /* 1A29 mov.w $00F1,a */
    spc_st(0x00F1, spc.a);
    /* 1A2C jmp $FFC0 */
    { spc_boot_rom(); return; }
}

/*
 * @routine 1A:F9E0 SpcBKeyOffAll
 * WHAT IT DOES: Keys off all eight voices. After it, SpcBMusicFade's no-fade path (the scale and start level from the master volume) and two port 1 handlers: 5 stops every channel (SpcBSfxStopAll) and reloads the driver now when the master volume is 0, else at the fade's end (bit 7 of $2C); 6 does the same with a return to the IPL ROM (bit 0 of $2D; the IPL ROM switched in through $F1 and a JMP to $FFC0).
 * HOW: DSP KOF ($5C) = $FF through SpcDspWrite.
 * Entry: none. Exit: A = $FF, Y = $5C.
 * @spc 19FB
 * @spcuses 19FB-1A02
 */
void abi_SpcBKeyOffAll(void)
{
    /* 19FB mov a,#$FF */
    spc.a = spc_nz(0xFF);
    /* 19FD mov y,#$5C */
    spc.y = spc_nz(0x5C);
    /* 19FF jmp $09E8 */
    { abi_SpcDspWrite(); return; }
}

/*
 * @routine 1A:FA14 SpcBCommandChannelCheck
 * WHAT IT DOES: A sound command $1D outside the list at $25FF is checked against the list at $2608: when an earlier-listed sound already plays on its channel, the command is dropped; the command kept is also stored at $1F (as the race driver's SpcCommandChannelCheck, with the battle's lists). After it, the sound effect queue start that SpcBSfxFrame jumps to ($1A7F): $057B counts the ticks without a command; after three, the queue's head (SpcBSoundQueuePush) is taken, its bit 7 moved to bit 7 of $00, else a command differing from the last ($1E) is taken; the sound ($23) gets its channel, mode and side (SpcBSfxLookup, SpcBSfxChannel, SpcBSfxMode, SpcBSfxSide), is activated and its pointer saved, then the channels run (SpcBSfxChannelsFrame) with $0410 holding the new sound. The race driver queues every command first.
 * HOW: List scans; the queue is shifted down one byte.
 * Entry: none. Exit: X = channel, or as SpcBSfxChannelsFrame leaves it.
 * @spc 1A2F
 * @spcuses 1A2F-1A7D,1B03-1B48
 */
void abi_SpcBCommandChannelCheck(void)
{
    /* 1A2F mov.b a,$1D */
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 1A31 beq $1A79 */
    if (spc.z) goto L1A79;
    /* 1A33 mov y,#$00 */
    spc.y = spc_nz(0x00);
L1A35:
    /* 1A35 mov.w a,$25FF+y */
    spc.a = spc_nz(spc_rd((uint16_t)(0x25FF + spc.y)));
    /* 1A38 beq $1A41 */
    if (spc.z) goto L1A41;
    /* 1A3A cmp.b a,$1D */
    spc_cmp(spc.a, spc_rd(spc_dp(0x1D)));
    /* 1A3C beq $1A7C */
    if (spc.z) goto L1A7C;
    /* 1A3E inc y */
    spc.y = spc_inc(spc.y);
    /* 1A3F bra $1A35 */
    goto L1A35;
L1A41:
    /* 1A41 mov y,#$00 */
    spc.y = spc_nz(0x00);
L1A43:
    /* 1A43 mov.w a,$2608+y */
    spc.a = spc_nz(spc_rd((uint16_t)(0x2608 + spc.y)));
    /* 1A46 beq $1A7C */
    if (spc.z) goto L1A7C;
    /* 1A48 push a */
    spc_push(spc.a);
    /* 1A49 mov.b a,$1D */
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 1A4B push y */
    spc_push(spc.y);
    /* 1A4C call $1B03 */
    spc_call(abi_SpcBSfxLookup, 0x1A4F);
    /* 1A4F call $1B17 */
    spc_call(abi_SpcBSfxChannel, 0x1A52);
    /* 1A52 pop y */
    spc.y = spc_pull();
    /* 1A53 pop a */
    spc.a = spc_pull();
    /* 1A54 cmp.w a,$0390+x */
    spc_cmp(spc.a, spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1A57 beq $1A5C */
    if (spc.z) goto L1A5C;
L1A59:
    /* 1A59 inc y */
    spc.y = spc_inc(spc.y);
    /* 1A5A bra $1A43 */
    goto L1A43;
L1A5C:
    /* 1A5C mov x,#$00 */
    spc.x = spc_nz(0x00);
L1A5E:
    /* 1A5E mov.w a,$2608+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x2608 + spc.x)));
    /* 1A61 beq $1A72 */
    if (spc.z) goto L1A72;
    /* 1A63 cmp.b a,$1D */
    spc_cmp(spc.a, spc_rd(spc_dp(0x1D)));
    /* 1A65 beq $1A6A */
    if (spc.z) goto L1A6A;
    /* 1A67 inc x */
    spc.x = spc_inc(spc.x);
    /* 1A68 bra $1A5E */
    goto L1A5E;
L1A6A:
    /* 1A6A mov.b $20,x */
    spc_st(spc_dp(0x20), spc.x);
    /* 1A6C mov a,y */
    spc.a = spc_nz(spc.y);
    /* 1A6D setc */
    spc.c = 1;
    /* 1A6E sbc.b a,$20 */
    spc.a = spc_sbc(spc.a, spc_rd(spc_dp(0x20)));
    /* 1A70 bpl $1A59 */
    if (!spc.n) goto L1A59;
L1A72:
    /* 1A72 mov $1F,$1D */
    spc_wr(spc_dp(0x1F), spc_rd(spc_dp(0x1D)));
    /* 1A75 mov $1D,#$00 */
    spc_st(spc_dp(0x1D), 0x00);
    /* 1A78 ret */
    spc_ret();
    return;
L1A79:
    /* 1A79 mov $1F,#$00 */
    spc_st(spc_dp(0x1F), 0x00);
L1A7C:
    /* 1A7C ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FA64 SpcBSfxQueueStart
 * WHAT IT DOES: The sound effect queue start that SpcBSfxFrame jumps to: $057B counts the ticks without a command (cleared by one); after three, the queue's head (SpcBSoundQueuePush) is taken, its bit 7 moved to bit 7 of $00, else a command differing from the last ($1E) is taken; the sound ($23) gets its channel, mode and side (SpcBSfxLookup, SpcBSfxChannel, SpcBSfxMode, SpcBSfxSide), is activated and its pointer saved, then the channels run (SpcBSfxChannelsFrame) with $0410 holding the new sound.
 * HOW: The queue is shifted down one byte; compares against $1E.
 * Entry: none. Exit: as SpcBSfxChannelsFrame leaves it.
 * @spc 1A7F
 * @spcuses 09E0-09EF,0BC1-0C17,0E97-0EE4,12FA-130C,1861-1871,19F5-1A02,1A21-1A2F,1A7F-1C60,1C6D-1DFD,1E00-217B,21C8-222B,2243-233A,2341-234D,235D-25B8
 */
void abi_SpcBSfxQueueStart(void)
{
    /* 1A7F cmp $1D,#$00 */
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x00);
    /* 1A82 beq $1A8B */
    if (spc.z) goto L1A8B;
    /* 1A84 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1A86 mov.w $057B,a */
    spc_st(0x057B, spc.a);
    /* 1A89 bra $1A95 */
    goto L1A95;
L1A8B:
    /* 1A8B mov.w a,$057B */
    spc.a = spc_nz(spc_rd(0x057B));
    /* 1A8E cmp a,#$FF */
    spc_cmp(spc.a, 0xFF);
    /* 1A90 beq $1A95 */
    if (spc.z) goto L1A95;
    /* 1A92 inc.w $057B */
    spc_rmw(0x057B, spc_inc);
L1A95:
    /* 1A95 cmp $1D,#$00 */
    spc_cmp(spc_rd(spc_dp(0x1D)), 0x00);
    /* 1A98 bne $1ACA */
    if (!spc.z) goto L1ACA;
    /* 1A9A mov.w a,$057B */
    spc.a = spc_nz(spc_rd(0x057B));
    /* 1A9D beq $1ACA */
    if (spc.z) goto L1ACA;
    /* 1A9F cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    /* 1AA1 beq $1ACA */
    if (spc.z) goto L1ACA;
    /* 1AA3 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    /* 1AA5 beq $1ACA */
    if (spc.z) goto L1ACA;
    /* 1AA7 mov.w a,$0588 */
    spc.a = spc_nz(spc_rd(0x0588));
    /* 1AAA beq $1ACA */
    if (spc.z) goto L1ACA;
    /* 1AAC push a */
    spc_push(spc.a);
    /* 1AAD mov x,#$01 */
    spc.x = spc_nz(0x01);
L1AAF:
    /* 1AAF mov.w a,$0588+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0588 + spc.x)));
    /* 1AB2 dec x */
    spc.x = spc_dec(spc.x);
    /* 1AB3 mov.w $0588+x,a */
    spc_st((uint16_t)(0x0588 + spc.x), spc.a);
    /* 1AB6 inc x */
    spc.x = spc_inc(spc.x);
    /* 1AB7 inc x */
    spc.x = spc_inc(spc.x);
    /* 1AB8 cmp x,#$08 */
    spc_cmp(spc.x, 0x08);
    /* 1ABA bne $1AAF */
    if (!spc.z) goto L1AAF;
    /* 1ABC pop a */
    spc.a = spc_pull();
    /* 1ABD asl a */
    spc.a = spc_asl(spc.a);
    /* 1ABE bcc $1AC2 */
    if (!spc.c) goto L1AC2;
    /* 1AC0 set1 $00.7 */
    spc_bit_set(0x00, 7, 1);
L1AC2:
    /* 1AC2 lsr a */
    spc.a = spc_lsr(spc.a);
    /* 1AC3 mov x,#$00 */
    spc.x = spc_nz(0x00);
    /* 1AC5 mov.w $057B,x */
    spc_st(0x057B, spc.x);
    /* 1AC8 bra $1AD9 */
    goto L1AD9;
L1ACA:
    /* 1ACA mov.b a,$1D */
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 1ACC cmp.b a,$1E */
    spc_cmp(spc.a, spc_rd(spc_dp(0x1E)));
    /* 1ACE bne $1AD3 */
    if (!spc.z) goto L1AD3;
L1AD0:
    /* 1AD0 jmp $1B7E */
    { abi_SpcBSfxChannelsFrame(); return; }
L1AD3:
    /* 1AD3 mov.b $1E,a */
    spc_st(spc_dp(0x1E), spc.a);
    /* 1AD5 mov.b a,$1D */
    spc.a = spc_nz(spc_rd(spc_dp(0x1D)));
    /* 1AD7 beq $1AD0 */
    if (spc.z) goto L1AD0;
L1AD9:
    /* 1AD9 mov.b $23,a */
    spc_st(spc_dp(0x23), spc.a);
    /* 1ADB call $1B03 */
    spc_call(abi_SpcBSfxLookup, 0x1ADE);
    /* 1ADE call $1B17 */
    spc_call(abi_SpcBSfxChannel, 0x1AE1);
    /* 1AE1 call $1B48 */
    spc_call(abi_SpcBSfxMode, 0x1AE4);
    /* 1AE4 call $1B71 */
    spc_call(abi_SpcBSfxSide, 0x1AE7);
    /* 1AE7 call $1AFD */
    spc_call(abi_SpcBSfxActivate, 0x1AEA);
    /* 1AEA call $1B9E */
    spc_call(abi_SpcBSfxChannelSave, 0x1AED);
    /* 1AED mov.b a,$23 */
    spc.a = spc_nz(spc_rd(spc_dp(0x23)));
    /* 1AEF mov.w $0410+x,a */
    spc_st((uint16_t)(0x0410 + spc.x), spc.a);
    /* 1AF2 push x */
    spc_push(spc.x);
    /* 1AF3 call $1B7E */
    spc_call(abi_SpcBSfxChannelsFrame, 0x1AF6);
    /* 1AF6 pop x */
    spc.x = spc_pull();
    /* 1AF7 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1AF9 mov.w $0410+x,a */
    spc_st((uint16_t)(0x0410 + spc.x), spc.a);
    /* 1AFC ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FAE2 SpcBSfxActivate
 * WHAT IT DOES: Marks channel X active ($0408 = 1). The race driver's SpcSfxActivate.
 * HOW: Store.
 * Entry: X = channel. Exit: A = 1.
 * @spc 1AFD
 * @spcuses 1AFD-1B03
 */
void abi_SpcBSfxActivate(void)
{
    /* 1AFD mov a,#$01 */
    spc.a = spc_nz(0x01);
    /* 1AFF mov.w $0408+x,a */
    spc_st((uint16_t)(0x0408 + spc.x), spc.a);
    /* 1B02 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FAE8 SpcBSfxLookup
 * WHAT IT DOES: The data of sound A: its pointer from $0702 into $20/$21 and its first byte (the channel and mode) in X. The race driver's SpcSfxLookup.
 * HOW: (A - 1) * 2 as the index; MOV A,($20)+Y.
 * Entry: A = sound. Exit: X = header byte, Y = 0.
 * @spc 1B03
 * @spcuses 1B03-1B17
 */
void abi_SpcBSfxLookup(void)
{
    /* 1B03 dec a */
    spc.a = spc_dec(spc.a);
    /* 1B04 asl a */
    spc.a = spc_asl(spc.a);
    /* 1B05 mov y,a */
    spc.y = spc_nz(spc.a);
    /* 1B06 mov.w a,$0702+y */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0702 + spc.y)));
    /* 1B09 mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 1B0B inc y */
    spc.y = spc_inc(spc.y);
    /* 1B0C mov.w a,$0702+y */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0702 + spc.y)));
    /* 1B0F mov.b $21,a */
    spc_st(spc_dp(0x21), spc.a);
    /* 1B11 mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 1B13 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 1B15 mov x,a */
    spc.x = spc_nz(spc.a);
    /* 1B16 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FAFC SpcBSfxChannel
 * WHAT IT DOES: Keeps the header byte X at $26 and leaves its channel (low four bits) in X, one lower when the header's bit 6 and bit 7 of $00 are set; sounds $77-$7A ($23) set the channel's sound number $03C8 to $1C, or $20 with bit 7 of $00. The race driver's SpcSfxChannel only takes the channel.
 * HOW: AND #$0F; BBC tests; a compare chain on $23.
 * Entry: X = header byte, $23 = sound. Exit: X = channel.
 * @spc 1B17
 * @spcuses 1B17-1B48
 */
void abi_SpcBSfxChannel(void)
{
    /* 1B17 mov.b $26,x */
    spc_st(spc_dp(0x26), spc.x);
    /* 1B19 mov a,x */
    spc.a = spc_nz(spc.x);
    /* 1B1A and a,#$0F */
    spc.a = spc_and(spc.a, 0x0F);
    /* 1B1C mov x,a */
    spc.x = spc_nz(spc.a);
    /* 1B1D bbc $26.6,$1B24 */
    if (!(spc_rd(spc_dp(0x26)) >> 6 & 1)) goto L1B24;
    /* 1B20 bbc $00.7,$1B24 */
    if (!(spc_rd(spc_dp(0x00)) >> 7 & 1)) goto L1B24;
    /* 1B23 dec x */
    spc.x = spc_dec(spc.x);
L1B24:
    /* 1B24 cmp $23,#$79 */
    spc_cmp(spc_rd(spc_dp(0x23)), 0x79);
    /* 1B27 beq $1B39 */
    if (spc.z) goto L1B39;
    /* 1B29 cmp $23,#$78 */
    spc_cmp(spc_rd(spc_dp(0x23)), 0x78);
    /* 1B2C beq $1B39 */
    if (spc.z) goto L1B39;
    /* 1B2E cmp $23,#$77 */
    spc_cmp(spc_rd(spc_dp(0x23)), 0x77);
    /* 1B31 beq $1B39 */
    if (spc.z) goto L1B39;
    /* 1B33 cmp $23,#$7A */
    spc_cmp(spc_rd(spc_dp(0x23)), 0x7A);
    /* 1B36 beq $1B39 */
    if (spc.z) goto L1B39;
    /* 1B38 ret */
    spc_ret();
    return;
L1B39:
    /* 1B39 bbs $00.7,$1B42 */
    if ((spc_rd(spc_dp(0x00)) >> 7 & 1)) goto L1B42;
    /* 1B3C mov a,#$1C */
    spc.a = spc_nz(0x1C);
    /* 1B3E mov.w $03C8+x,a */
    spc_st((uint16_t)(0x03C8 + spc.x), spc.a);
    /* 1B41 ret */
    spc_ret();
    return;
L1B42:
    /* 1B42 mov a,#$20 */
    spc.a = spc_nz(0x20);
    /* 1B44 mov.w $03C8+x,a */
    spc_st((uint16_t)(0x03C8 + spc.x), spc.a);
    /* 1B47 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FB2D SpcBSfxMode
 * WHAT IT DOES: The header's mode bits for channel X: bit 7 marks a looping sound ($0480 = 1, or 2 with bit 5); otherwise a looping channel saves its sound to restart ($04F8) with its side ($04B8 to $0528). The race driver's SpcSfxMode without the side.
 * HOW: BBC tests on $26.
 * Entry: X = channel, $26 = header. Exit: A changed.
 * @spc 1B48
 * @spcuses 1B48-1B71
 */
void abi_SpcBSfxMode(void)
{
    /* 1B48 bbc $26.7,$1B58 */
    if (!(spc_rd(spc_dp(0x26)) >> 7 & 1)) goto L1B58;
    /* 1B4B bbc $26.5,$1B52 */
    if (!(spc_rd(spc_dp(0x26)) >> 5 & 1)) goto L1B52;
    /* 1B4E mov a,#$02 */
    spc.a = spc_nz(0x02);
    /* 1B50 bra $1B54 */
    goto L1B54;
L1B52:
    /* 1B52 mov a,#$01 */
    spc.a = spc_nz(0x01);
L1B54:
    /* 1B54 mov.w $0480+x,a */
    spc_st((uint16_t)(0x0480 + spc.x), spc.a);
    /* 1B57 ret */
    spc_ret();
    return;
L1B58:
    /* 1B58 mov.w a,$0480+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0480 + spc.x)));
    /* 1B5B cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    /* 1B5D bne $1B70 */
    if (!spc.z) goto L1B70;
    /* 1B5F mov.w a,$04F8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F8 + spc.x)));
    /* 1B62 bne $1B70 */
    if (!spc.z) goto L1B70;
    /* 1B64 mov.w a,$0390+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1B67 mov.w $04F8+x,a */
    spc_st((uint16_t)(0x04F8 + spc.x), spc.a);
    /* 1B6A mov.w a,$04B8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04B8 + spc.x)));
    /* 1B6D mov.w $0528+x,a */
    spc_st((uint16_t)(0x0528 + spc.x), spc.a);
L1B70:
    /* 1B70 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FB56 SpcBSfxSide
 * WHAT IT DOES: Channel X's side $04B8: 1, or 2 with bit 7 of $00.
 * HOW: BBS and a store.
 * Entry: X = channel. Exit: A = the side.
 * @spc 1B71
 * @spcuses 1B71-1B7E
 */
void abi_SpcBSfxSide(void)
{
    /* 1B71 bbs $00.7,$1B78 */
    if ((spc_rd(spc_dp(0x00)) >> 7 & 1)) goto L1B78;
    /* 1B74 mov a,#$01 */
    spc.a = spc_nz(0x01);
    /* 1B76 bra $1B7A */
    goto L1B7A;
L1B78:
    /* 1B78 mov a,#$02 */
    spc.a = spc_nz(0x02);
L1B7A:
    /* 1B7A mov.w $04B8+x,a */
    spc_st((uint16_t)(0x04B8 + spc.x), spc.a);
    /* 1B7D ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FB63 SpcBSfxChannelsFrame
 * WHAT IT DOES: Runs every active sound effect channel's tick (SpcBSfxChannelLoad, SpcBSfxChannelStep). The race driver's SpcSfxChannelsFrame.
 * HOW: X from 0 to 7.
 * Entry: none. Exit: X = 8.
 * @spc 1B7E
 * @spcuses 09E0-09EF,0BC1-0C17,0E97-0EE4,12FA-130C,1861-1871,19F5-1A02,1A21-1A2F,1AFD-1B03,1B17-1C60,1C6D-1DFD,1E00-217B,21C8-222B,2243-233A,2341-234D,235D-25B8
 */
void abi_SpcBSfxChannelsFrame(void)
{
    /* 1B7E mov x,#$00 */
    spc.x = spc_nz(0x00);
L1B80:
    /* 1B80 mov.w a,$0408+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 1B83 beq $1B8B */
    if (spc.z) goto L1B8B;
    /* 1B85 call $1B93 */
    spc_call(abi_SpcBSfxChannelLoad, 0x1B88);
    /* 1B88 call $1BA9 */
    spc_call(abi_SpcBSfxChannelStep, 0x1B8B);
L1B8B:
    /* 1B8B inc x */
    spc.x = spc_inc(spc.x);
    /* 1B8C cmp x,#$08 */
    spc_cmp(spc.x, 0x08);
    /* 1B8E beq $1B92 */
    if (spc.z) goto L1B92;
    /* 1B90 bra $1B80 */
    goto L1B80;
L1B92:
    /* 1B92 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FB78 SpcBSfxChannelLoad
 * WHAT IT DOES: Channel X's data pointer ($0488/$0490) into $20/$21. The race driver's SpcSfxChannelLoad.
 * HOW: Two copies.
 * Entry: X = channel. Exit: A = the pointer's high byte.
 * @spc 1B93
 * @spcuses 1B93-1B9E
 */
void abi_SpcBSfxChannelLoad(void)
{
    /* 1B93 mov.w a,$0488+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0488 + spc.x)));
    /* 1B96 mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 1B98 mov.w a,$0490+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0490 + spc.x)));
    /* 1B9B mov.b $21,a */
    spc_st(spc_dp(0x21), spc.a);
    /* 1B9D ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FB83 SpcBSfxChannelSave
 * WHAT IT DOES: $20/$21 back as channel X's data pointer. The race driver's SpcSfxChannelSave.
 * HOW: Two copies.
 * Entry: X = channel. Exit: A = the high byte.
 * @spc 1B9E
 * @spcuses 1B9E-1BA9
 */
void abi_SpcBSfxChannelSave(void)
{
    /* 1B9E mov.b a,$20 */
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 1BA0 mov.w $0488+x,a */
    spc_st((uint16_t)(0x0488 + spc.x), spc.a);
    /* 1BA3 mov.b a,$21 */
    spc.a = spc_nz(spc_rd(spc_dp(0x21)));
    /* 1BA5 mov.w $0490+x,a */
    spc_st((uint16_t)(0x0490 + spc.x), spc.a);
    /* 1BA8 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FB8E SpcBSfxChannelStep
 * WHAT IT DOES: Channel X's state ($03F8: 0 start, 1 started, 2 and 3 playing, 4 stopping), as the race driver's SpcSfxChannelStep: a new sound ($0410) starts with its voice taken from the music and keyed off; playing, it is keyed on, read (SpcBSfxRead), its special handler run (SpcBSfxSpecial) and its DSP registers written; stop requests ($03E0) release the voice (SpcBSfxRelease) and clear the channel: reason 1 queues a saved sound again with bit 7 set for side 1 and clear for side 2 ($0528), reasons $0A/$1A, $0B/$1B and $0C/$1C end the loops $25, $15 and $17. The battle's clear also resets the side ($04B8, $0528).
 * HOW: Compare chains on the state and the request; tail jumps into the shared code. The 13 bytes at $1C60 are code no path reaches.
 * Entry: X = channel, $20/$21 its pointer. Exit: A, Y changed.
 * @spc 1BA9
 * @spcuses 09E0-09EF,0BC1-0C17,0E97-0EE4,12FA-130C,1861-1871,19F5-1A02,1A21-1A2F,1AFD-1B03,1B17-1B7E,1B93-1C60,1C6D-1DFD,1E00-217B,21C8-222B,2243-233A,2341-234D,235D-25B8
 */
void abi_SpcBSfxChannelStep(void)
{
    /* 1BA9 mov.w a,$03F8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03F8 + spc.x)));
    /* 1BAC beq $1C01 */
    if (spc.z) goto L1C01;
    /* 1BAE cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    /* 1BB0 beq $1BCD */
    if (spc.z) goto L1BCD;
    /* 1BB2 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    /* 1BB4 beq $1BE2 */
    if (spc.z) goto L1BE2;
    /* 1BB6 cmp a,#$03 */
    spc_cmp(spc.a, 0x03);
    /* 1BB8 beq $1BE7 */
    if (spc.z) goto L1BE7;
    /* 1BBA mov.w a,$0410+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0410 + spc.x)));
    /* 1BBD bne $1C04 */
    if (!spc.z) goto L1C04;
    /* 1BBF mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1BC1 mov.w $0390+x,a */
    spc_st((uint16_t)(0x0390 + spc.x), spc.a);
    /* 1BC4 mov.w $03F8+x,a */
    spc_st((uint16_t)(0x03F8 + spc.x), spc.a);
    /* 1BC7 mov.w $0408+x,a */
    spc_st((uint16_t)(0x0408 + spc.x), spc.a);
    /* 1BCA jmp $1CEE */
    goto L1CEE;
L1BCD:
    /* 1BCD mov.w a,$0410+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0410 + spc.x)));
    /* 1BD0 bne $1C04 */
    if (!spc.z) goto L1C04;
    /* 1BD2 mov a,#$02 */
    spc.a = spc_nz(0x02);
    /* 1BD4 mov.w $03F8+x,a */
    spc_st((uint16_t)(0x03F8 + spc.x), spc.a);
    /* 1BD7 call $1D46 */
    spc_call(abi_SpcBKeyOnVoice, 0x1BDA);
    /* 1BDA mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1BDC mov.w $03E8+x,a */
    spc_st((uint16_t)(0x03E8 + spc.x), spc.a);
L1BDF:
    /* 1BDF jmp $1D5A */
    goto L1D5A;
L1BE2:
    /* 1BE2 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1BE4 mov.w $03E8+x,a */
    spc_st((uint16_t)(0x03E8 + spc.x), spc.a);
L1BE7:
    /* 1BE7 mov.w a,$0410+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0410 + spc.x)));
    /* 1BEA bne $1C04 */
    if (!spc.z) goto L1C04;
    /* 1BEC mov a,#$03 */
    spc.a = spc_nz(0x03);
    /* 1BEE mov.w $03F8+x,a */
    spc_st((uint16_t)(0x03F8 + spc.x), spc.a);
    /* 1BF1 mov.w a,$03E8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03E8 + spc.x)));
    /* 1BF4 beq $1C1D */
    if (spc.z) goto L1C1D;
    /* 1BF6 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    /* 1BF8 beq $1BCD */
    if (spc.z) goto L1BCD;
    /* 1BFA mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1BFC mov.w $03F0+x,a */
    spc_st((uint16_t)(0x03F0 + spc.x), spc.a);
    /* 1BFF bra $1BCD */
    goto L1BCD;
L1C01:
    /* 1C01 mov.w a,$0410+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0410 + spc.x)));
L1C04:
    /* 1C04 mov.w $0390+x,a */
    spc_st((uint16_t)(0x0390 + spc.x), spc.a);
    /* 1C07 mov a,#$01 */
    spc.a = spc_nz(0x01);
    /* 1C09 mov.w $03F8+x,a */
    spc_st((uint16_t)(0x03F8 + spc.x), spc.a);
    /* 1C0C mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1C0E mov.w $03F0+x,a */
    spc_st((uint16_t)(0x03F0 + spc.x), spc.a);
    /* 1C11 mov.w $03E0+x,a */
    spc_st((uint16_t)(0x03E0 + spc.x), spc.a);
    /* 1C14 call $1CCC */
    spc_call(abi_SpcBSfxMarkVoice, 0x1C17);
    /* 1C17 jmp $1D50 */
    { abi_SpcBKeyOffVoice(); return; }
L1C1A:
    /* 1C1A jmp $1C87 */
    goto L1C87;
L1C1D:
    /* 1C1D mov.w a,$03E0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03E0 + spc.x)));
    /* 1C20 beq $1BDF */
    if (spc.z) goto L1BDF;
    /* 1C22 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    /* 1C24 beq $1C1A */
    if (spc.z) goto L1C1A;
    /* 1C26 cmp a,#$0A */
    spc_cmp(spc.a, 0x0A);
    /* 1C28 beq $1C45 */
    if (spc.z) goto L1C45;
    /* 1C2A cmp a,#$1A */
    spc_cmp(spc.a, 0x1A);
    /* 1C2C beq $1C45 */
    if (spc.z) goto L1C45;
    /* 1C2E cmp a,#$0B */
    spc_cmp(spc.a, 0x0B);
    /* 1C30 beq $1C4E */
    if (spc.z) goto L1C4E;
    /* 1C32 cmp a,#$1B */
    spc_cmp(spc.a, 0x1B);
    /* 1C34 beq $1C4E */
    if (spc.z) goto L1C4E;
    /* 1C36 cmp a,#$0C */
    spc_cmp(spc.a, 0x0C);
    /* 1C38 beq $1C57 */
    if (spc.z) goto L1C57;
    /* 1C3A cmp a,#$1C */
    spc_cmp(spc.a, 0x1C);
    /* 1C3C beq $1C57 */
    if (spc.z) goto L1C57;
L1C3E:
    /* 1C3E mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1C40 mov.w $03E0+x,a */
    spc_st((uint16_t)(0x03E0 + spc.x), spc.a);
    /* 1C43 bra $1BDF */
    goto L1BDF;
L1C45:
    /* 1C45 mov.w a,$0390+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1C48 cmp a,#$25 */
    spc_cmp(spc.a, 0x25);
    /* 1C4A beq $1C6D */
    if (spc.z) goto L1C6D;
    /* 1C4C bra $1C3E */
    goto L1C3E;
L1C4E:
    /* 1C4E mov.w a,$0390+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1C51 cmp a,#$15 */
    spc_cmp(spc.a, 0x15);
    /* 1C53 beq $1C6D */
    if (spc.z) goto L1C6D;
    /* 1C55 bra $1C3E */
    goto L1C3E;
L1C57:
    /* 1C57 mov.w a,$0390+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1C5A cmp a,#$17 */
    spc_cmp(spc.a, 0x17);
    /* 1C5C beq $1C6D */
    if (spc.z) goto L1C6D;
    /* 1C5E bra $1C3E */
    goto L1C3E;
L1C6D:
    /* 1C6D mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1C6F mov.w $03E0+x,a */
    spc_st((uint16_t)(0x03E0 + spc.x), spc.a);
    /* 1C72 mov.w a,$04F8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F8 + spc.x)));
    /* 1C75 beq $1C7F */
    if (spc.z) goto L1C7F;
    /* 1C77 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1C79 mov.w $04F8+x,a */
    spc_st((uint16_t)(0x04F8 + spc.x), spc.a);
    /* 1C7C jmp $1BDF */
    goto L1BDF;
L1C7F:
    /* 1C7F mov.w a,$0480+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0480 + spc.x)));
    /* 1C82 bne $1CAC */
    if (!spc.z) goto L1CAC;
    /* 1C84 jmp $1BDF */
    goto L1BDF;
L1C87:
    /* 1C87 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1C89 mov.w $03E0+x,a */
    spc_st((uint16_t)(0x03E0 + spc.x), spc.a);
    /* 1C8C mov.w a,$04F8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F8 + spc.x)));
    /* 1C8F beq $1CAC */
    if (spc.z) goto L1CAC;
    /* 1C91 push a */
    spc_push(spc.a);
    /* 1C92 mov.w a,$0528+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0528 + spc.x)));
    /* 1C95 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    /* 1C97 beq $1CA0 */
    if (spc.z) goto L1CA0;
    /* 1C99 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    /* 1C9B beq $1CA5 */
    if (spc.z) goto L1CA5;
    /* 1C9D pop a */
    spc.a = spc_pull();
    /* 1C9E bra $1CAC */
    goto L1CAC;
L1CA0:
    /* 1CA0 pop a */
    spc.a = spc_pull();
    /* 1CA1 asl a */
    spc.a = spc_asl(spc.a);
    /* 1CA2 setc */
    spc.c = 1;
    /* 1CA3 bra $1CA8 */
    goto L1CA8;
L1CA5:
    /* 1CA5 pop a */
    spc.a = spc_pull();
    /* 1CA6 asl a */
    spc.a = spc_asl(spc.a);
    /* 1CA7 clrc */
    spc.c = 0;
L1CA8:
    /* 1CA8 ror a */
    spc.a = spc_ror(spc.a);
    /* 1CA9 call $12FA */
    spc_call(abi_SpcBSoundQueuePush, 0x1CAC);
L1CAC:
    /* 1CAC mov a,#$04 */
    spc.a = spc_nz(0x04);
    /* 1CAE mov.w $03F8+x,a */
    spc_st((uint16_t)(0x03F8 + spc.x), spc.a);
    /* 1CB1 call $1D50 */
    spc_call(abi_SpcBKeyOffVoice, 0x1CB4);
    /* 1CB4 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1CB6 mov.w $0480+x,a */
    spc_st((uint16_t)(0x0480 + spc.x), spc.a);
    /* 1CB9 mov.w $04F8+x,a */
    spc_st((uint16_t)(0x04F8 + spc.x), spc.a);
    /* 1CBC mov.w $03F0+x,a */
    spc_st((uint16_t)(0x03F0 + spc.x), spc.a);
    /* 1CBF mov.w $03C8+x,a */
    spc_st((uint16_t)(0x03C8 + spc.x), spc.a);
    /* 1CC2 mov.w $04B8+x,a */
    spc_st((uint16_t)(0x04B8 + spc.x), spc.a);
    /* 1CC5 mov.w $0528+x,a */
    spc_st((uint16_t)(0x0528 + spc.x), spc.a);
    /* 1CC8 mov.w $04F0+x,a */
    spc_st((uint16_t)(0x04F0 + spc.x), spc.a);
    /* 1CCB ret */
    spc_ret();
    return;
L1CEE:
    /* 1CEE call $1CE5 */
    spc_call(abi_SpcBVoiceMaskNot, 0x1CF1);
    /* 1CF1 mov.b a,$1A */
    spc.a = spc_nz(spc_rd(spc_dp(0x1A)));
    /* 1CF3 and.b a,$22 */
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0x22)));
    /* 1CF5 mov.b $1A,a */
    spc_st(spc_dp(0x1A), spc.a);
    /* 1CF7 push x */
    spc_push(spc.x);
    /* 1CF8 clrc */
    spc.c = 0;
    /* 1CF9 mov a,x */
    spc.a = spc_nz(spc.x);
    /* 1CFA asl a */
    spc.a = spc_asl(spc.a);
    /* 1CFB mov x,a */
    spc.x = spc_nz(spc.a);
    /* 1CFC mov.w a,$0211+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0211 + spc.x)));
    /* 1CFF call $0BC1 */
    spc_call(abi_SpcLoadInstrument, 0x1D02);
    /* 1D02 pop x */
    spc.x = spc_pull();
    /* 1D03 mov.b $20,x */
    spc_st(spc_dp(0x20), spc.x);
    /* 1D05 inc.b $20 */
    spc_rmw(spc_dp(0x20), spc_inc);
    /* 1D07 mov.b a,$4F */
    spc.a = spc_nz(spc_rd(spc_dp(0x4F)));
L1D09:
    /* 1D09 lsr a */
    spc.a = spc_lsr(spc.a);
    /* 1D0A dec.b $20 */
    spc_rmw(spc_dp(0x20), spc_dec);
    /* 1D0C bne $1D09 */
    if (!spc.z) goto L1D09;
    /* 1D0E bcc $1D1B */
    if (!spc.c) goto L1D1B;
    /* 1D10 call $1CD6 */
    spc_call(abi_SpcBVoiceMask, 0x1D13);
    /* 1D13 mov.b a,$4A */
    spc.a = spc_nz(spc_rd(spc_dp(0x4A)));
    /* 1D15 or.b a,$22 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x22)));
    /* 1D17 mov.b $4A,a */
    spc_st(spc_dp(0x4A), spc.a);
    /* 1D19 bra $1D24 */
    goto L1D24;
L1D1B:
    /* 1D1B call $1CE5 */
    spc_call(abi_SpcBVoiceMaskNot, 0x1D1E);
    /* 1D1E mov.b a,$4A */
    spc.a = spc_nz(spc_rd(spc_dp(0x4A)));
    /* 1D20 and.b a,$22 */
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0x22)));
    /* 1D22 mov.b $4A,a */
    spc_st(spc_dp(0x4A), spc.a);
L1D24:
    /* 1D24 call $1CE5 */
    spc_call(abi_SpcBVoiceMaskNot, 0x1D27);
    /* 1D27 mov.b a,$1A */
    spc.a = spc_nz(spc_rd(spc_dp(0x1A)));
    /* 1D29 and.b a,$22 */
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0x22)));
    /* 1D2B mov.b $1A,a */
    spc_st(spc_dp(0x1A), spc.a);
    /* 1D2D call $1CD6 */
    spc_call(abi_SpcBVoiceMask, 0x1D30);
    /* 1D30 mov.b a,$47 */
    spc.a = spc_nz(spc_rd(spc_dp(0x47)));
    /* 1D32 or.b a,$22 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x22)));
    /* 1D34 mov.b $47,a */
    spc_st(spc_dp(0x47), spc.a);
    /* 1D36 mov.b a,$5E */
    spc.a = spc_nz(spc_rd(spc_dp(0x5E)));
    /* 1D38 or.b a,$20 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x20)));
    /* 1D3A mov.b $5E,a */
    spc_st(spc_dp(0x5E), spc.a);
    /* 1D3C push x */
    spc_push(spc.x);
    /* 1D3D clrc */
    spc.c = 0;
    /* 1D3E mov a,x */
    spc.a = spc_nz(spc.x);
    /* 1D3F asl a */
    spc.a = spc_asl(spc.a);
    /* 1D40 mov x,a */
    spc.x = spc_nz(spc.a);
    /* 1D41 call $0E97 */
    spc_call(abi_SpcVoiceVolumeWrite, 0x1D44);
    /* 1D44 pop x */
    spc.x = spc_pull();
    /* 1D45 ret */
    spc_ret();
    return;
L1D5A:
    /* 1D5A call $1D63 */
    spc_call(abi_SpcBSfxRead, 0x1D5D);
    /* 1D5D call $231E */
    spc_call(abi_SpcBSfxSpecial, 0x1D60);
    /* 1D60 jmp $21C8 */
    goto L21C8;
L21C8:
    /* 21C8 mov a,x */
    spc.a = spc_nz(spc.x);
    /* 21C9 mov y,#$10 */
    spc.y = spc_nz(0x10);
    /* 21CB mul ya */
    spc_mul();
    /* 21CC mov y,a */
    spc.y = spc_nz(spc.a);
    /* 21CD push y */
    spc_push(spc.y);
    /* 21CE mov.w a,$0408+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 21D1 beq $21DD */
    if (spc.z) goto L21DD;
    /* 21D3 mov.w a,$04C0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04C0 + spc.x)));
    /* 21D6 beq $21DD */
    if (spc.z) goto L21DD;
    /* 21D8 call $2243 */
    spc_call(abi_SpcBSfxFadeVolume, 0x21DB);
    /* 21DB bra $21E0 */
    goto L21E0;
L21DD:
    /* 21DD mov.w a,$0398+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0398 + spc.x)));
L21E0:
    /* 21E0 call $22F2 */
    spc_call(abi_SpcBSfxVolumeAdjust, 0x21E3);
    /* 21E3 pop y */
    spc.y = spc_pull();
    /* 21E4 call $09E8 */
    spc_call(abi_SpcDspWrite, 0x21E7);
    /* 21E7 inc y */
    spc.y = spc_inc(spc.y);
    /* 21E8 push y */
    spc_push(spc.y);
    /* 21E9 mov.w a,$0408+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 21EC beq $21FE */
    if (spc.z) goto L21FE;
    /* 21EE mov.w a,$04C8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04C8 + spc.x)));
    /* 21F1 beq $21FE */
    if (spc.z) goto L21FE;
    /* 21F3 call $22BF */
    spc_call(abi_SpcBIndexRight, 0x21F6);
    /* 21F6 call $2243 */
    spc_call(abi_SpcBSfxFadeVolume, 0x21F9);
    /* 21F9 call $22C8 */
    spc_call(abi_SpcBIndexLeft, 0x21FC);
    /* 21FC bra $2201 */
    goto L2201;
L21FE:
    /* 21FE mov.w a,$03A0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03A0 + spc.x)));
L2201:
    /* 2201 call $22F2 */
    spc_call(abi_SpcBSfxVolumeAdjust, 0x2204);
    /* 2204 pop y */
    spc.y = spc_pull();
    /* 2205 call $09E8 */
    spc_call(abi_SpcDspWrite, 0x2208);
    /* 2208 inc y */
    spc.y = spc_inc(spc.y);
    /* 2209 mov.w a,$03B8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03B8 + spc.x)));
    /* 220C call $09E8 */
    spc_call(abi_SpcDspWrite, 0x220F);
    /* 220F inc y */
    spc.y = spc_inc(spc.y);
    /* 2210 mov.w a,$03C0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03C0 + spc.x)));
    /* 2213 call $09E8 */
    spc_call(abi_SpcDspWrite, 0x2216);
    /* 2216 inc y */
    spc.y = spc_inc(spc.y);
    /* 2217 mov.w a,$03C8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03C8 + spc.x)));
    /* 221A call $09E8 */
    spc_call(abi_SpcDspWrite, 0x221D);
    /* 221D inc y */
    spc.y = spc_inc(spc.y);
    /* 221E mov.w a,$03D0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03D0 + spc.x)));
    /* 2221 call $09E8 */
    spc_call(abi_SpcDspWrite, 0x2224);
    /* 2224 inc y */
    spc.y = spc_inc(spc.y);
    /* 2225 mov.w a,$03D8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03D8 + spc.x)));
    /* 2228 jmp $09E8 */
    { abi_SpcDspWrite(); return; }
}

/*
 * @routine 1A:FC45 SpcBSfxStopMatchOld
 * WHAT IT DOES: The race driver's stop-request match for the commands $05 and $08 (SpcSfxStopMatchOld), left in the battle block after SpcBSfxChannelStep's last case: channel X's command $0390 of 5 or 8 takes the stop ($1C87), any other refuses it ($1C3E); then as SpcBSfxChannelStep. It follows an unconditional BRA and nothing jumps to it; a direct call is its only way in.
 * HOW: CMP, BEQ; then as SpcBSfxChannelStep.
 * Entry: X = channel. Exit: as SpcBSfxChannelStep.
 * @spc 1C60
 * @spcuses 09E8-09EF,12FA-130C,1861-1871,19F5-1A02,1A21-1A2F,1AFD-1B03,1B17-1B7E,1B93-1BA9,1BDF-1BE2,1C04-1C1A,1C3E-1C45,1C60-1C6D,1C87-1CEE,1D50-1DFD,1E00-217B,21C8-222B,2243-233A,2341-234D,235D-25B8
 */
void abi_SpcBSfxStopMatchOld(void)
{
    /* 1C60 mov.w a,$0390+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1C63 cmp a,#$05 */
    spc_cmp(spc.a, 0x05);
    /* 1C65 beq $1C87 */
    if (spc.z) goto L1C87;
    /* 1C67 cmp a,#$08 */
    spc_cmp(spc.a, 0x08);
    /* 1C69 beq $1C87 */
    if (spc.z) goto L1C87;
    /* 1C6B bra $1C3E */
    goto L1C3E;
L1C87:
    /* 1C87 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1C89 mov.w $03E0+x,a */
    spc_st((uint16_t)(0x03E0 + spc.x), spc.a);
    /* 1C8C mov.w a,$04F8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F8 + spc.x)));
    /* 1C8F beq $1CAC */
    if (spc.z) goto L1CAC;
    /* 1C91 push a */
    spc_push(spc.a);
    /* 1C92 mov.w a,$0528+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0528 + spc.x)));
    /* 1C95 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    /* 1C97 beq $1CA0 */
    if (spc.z) goto L1CA0;
    /* 1C99 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    /* 1C9B beq $1CA5 */
    if (spc.z) goto L1CA5;
    /* 1C9D pop a */
    spc.a = spc_pull();
    /* 1C9E bra $1CAC */
    goto L1CAC;
L1CA0:
    /* 1CA0 pop a */
    spc.a = spc_pull();
    /* 1CA1 asl a */
    spc.a = spc_asl(spc.a);
    /* 1CA2 setc */
    spc.c = 1;
    /* 1CA3 bra $1CA8 */
    goto L1CA8;
L1CA5:
    /* 1CA5 pop a */
    spc.a = spc_pull();
    /* 1CA6 asl a */
    spc.a = spc_asl(spc.a);
    /* 1CA7 clrc */
    spc.c = 0;
L1CA8:
    /* 1CA8 ror a */
    spc.a = spc_ror(spc.a);
    /* 1CA9 call $12FA */
    spc_call(abi_SpcBSoundQueuePush, 0x1CAC);
L1CAC:
    /* 1CAC mov a,#$04 */
    spc.a = spc_nz(0x04);
    /* 1CAE mov.w $03F8+x,a */
    spc_st((uint16_t)(0x03F8 + spc.x), spc.a);
    /* 1CB1 call $1D50 */
    spc_call(abi_SpcBKeyOffVoice, 0x1CB4);
    /* 1CB4 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1CB6 mov.w $0480+x,a */
    spc_st((uint16_t)(0x0480 + spc.x), spc.a);
    /* 1CB9 mov.w $04F8+x,a */
    spc_st((uint16_t)(0x04F8 + spc.x), spc.a);
    /* 1CBC mov.w $03F0+x,a */
    spc_st((uint16_t)(0x03F0 + spc.x), spc.a);
    /* 1CBF mov.w $03C8+x,a */
    spc_st((uint16_t)(0x03C8 + spc.x), spc.a);
    /* 1CC2 mov.w $04B8+x,a */
    spc_st((uint16_t)(0x04B8 + spc.x), spc.a);
    /* 1CC5 mov.w $0528+x,a */
    spc_st((uint16_t)(0x0528 + spc.x), spc.a);
    /* 1CC8 mov.w $04F0+x,a */
    spc_st((uint16_t)(0x04F0 + spc.x), spc.a);
    /* 1CCB ret */
    spc_ret();
    return;
L1D5A:
    /* 1D5A call $1D63 */
    spc_call(abi_SpcBSfxRead, 0x1D5D);
    /* 1D5D call $231E */
    spc_call(abi_SpcBSfxSpecial, 0x1D60);
    /* 1D60 jmp $21C8 */
    goto L21C8;
L21C8:
    /* 21C8 mov a,x */
    spc.a = spc_nz(spc.x);
    /* 21C9 mov y,#$10 */
    spc.y = spc_nz(0x10);
    /* 21CB mul ya */
    spc_mul();
    /* 21CC mov y,a */
    spc.y = spc_nz(spc.a);
    /* 21CD push y */
    spc_push(spc.y);
    /* 21CE mov.w a,$0408+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 21D1 beq $21DD */
    if (spc.z) goto L21DD;
    /* 21D3 mov.w a,$04C0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04C0 + spc.x)));
    /* 21D6 beq $21DD */
    if (spc.z) goto L21DD;
    /* 21D8 call $2243 */
    spc_call(abi_SpcBSfxFadeVolume, 0x21DB);
    /* 21DB bra $21E0 */
    goto L21E0;
L21DD:
    /* 21DD mov.w a,$0398+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0398 + spc.x)));
L21E0:
    /* 21E0 call $22F2 */
    spc_call(abi_SpcBSfxVolumeAdjust, 0x21E3);
    /* 21E3 pop y */
    spc.y = spc_pull();
    /* 21E4 call $09E8 */
    spc_call(abi_SpcDspWrite, 0x21E7);
    /* 21E7 inc y */
    spc.y = spc_inc(spc.y);
    /* 21E8 push y */
    spc_push(spc.y);
    /* 21E9 mov.w a,$0408+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0408 + spc.x)));
    /* 21EC beq $21FE */
    if (spc.z) goto L21FE;
    /* 21EE mov.w a,$04C8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04C8 + spc.x)));
    /* 21F1 beq $21FE */
    if (spc.z) goto L21FE;
    /* 21F3 call $22BF */
    spc_call(abi_SpcBIndexRight, 0x21F6);
    /* 21F6 call $2243 */
    spc_call(abi_SpcBSfxFadeVolume, 0x21F9);
    /* 21F9 call $22C8 */
    spc_call(abi_SpcBIndexLeft, 0x21FC);
    /* 21FC bra $2201 */
    goto L2201;
L21FE:
    /* 21FE mov.w a,$03A0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03A0 + spc.x)));
L2201:
    /* 2201 call $22F2 */
    spc_call(abi_SpcBSfxVolumeAdjust, 0x2204);
    /* 2204 pop y */
    spc.y = spc_pull();
    /* 2205 call $09E8 */
    spc_call(abi_SpcDspWrite, 0x2208);
    /* 2208 inc y */
    spc.y = spc_inc(spc.y);
    /* 2209 mov.w a,$03B8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03B8 + spc.x)));
    /* 220C call $09E8 */
    spc_call(abi_SpcDspWrite, 0x220F);
    /* 220F inc y */
    spc.y = spc_inc(spc.y);
    /* 2210 mov.w a,$03C0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03C0 + spc.x)));
    /* 2213 call $09E8 */
    spc_call(abi_SpcDspWrite, 0x2216);
    /* 2216 inc y */
    spc.y = spc_inc(spc.y);
    /* 2217 mov.w a,$03C8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03C8 + spc.x)));
    /* 221A call $09E8 */
    spc_call(abi_SpcDspWrite, 0x221D);
    /* 221D inc y */
    spc.y = spc_inc(spc.y);
    /* 221E mov.w a,$03D0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03D0 + spc.x)));
    /* 2221 call $09E8 */
    spc_call(abi_SpcDspWrite, 0x2224);
    /* 2224 inc y */
    spc.y = spc_inc(spc.y);
    /* 2225 mov.w a,$03D8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03D8 + spc.x)));
    /* 2228 jmp $09E8 */
    { abi_SpcDspWrite(); return; }
L1BDF:
    /* 1BDF jmp $1D5A */
    goto L1D5A;
L1C3E:
    /* 1C3E mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1C40 mov.w $03E0+x,a */
    spc_st((uint16_t)(0x03E0 + spc.x), spc.a);
    /* 1C43 bra $1BDF */
    goto L1BDF;
}

/*
 * @routine 1A:FCB1 SpcBSfxMarkVoice
 * WHAT IT DOES: Marks voice X as a sound effect's ($1A). The race driver's SpcSfxMarkVoice.
 * HOW: SpcBVoiceMask, OR.
 * Entry: X = voice. Exit: A = $1A.
 * @spc 1CCC
 * @spcuses 1CCC-1CE5
 */
void abi_SpcBSfxMarkVoice(void)
{
    /* 1CCC call $1CD6 */
    spc_call(abi_SpcBVoiceMask, 0x1CCF);
    /* 1CCF mov.b a,$1A */
    spc.a = spc_nz(spc_rd(spc_dp(0x1A)));
    /* 1CD1 or.b a,$22 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x22)));
    /* 1CD3 mov.b $1A,a */
    spc_st(spc_dp(0x1A), spc.a);
    /* 1CD5 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FCBB SpcBVoiceMask
 * WHAT IT DOES: $22 = the bit of voice X (1 << X). The race driver's SpcVoiceMask.
 * HOW: ROL of a carry X + 1 times.
 * Entry: X = voice. Exit: A = $22.
 * @spc 1CD6
 * @spcuses 1CD6-1CE5
 */
void abi_SpcBVoiceMask(void)
{
    /* 1CD6 mov.b $22,x */
    spc_st(spc_dp(0x22), spc.x);
    /* 1CD8 inc.b $22 */
    spc_rmw(spc_dp(0x22), spc_inc);
    /* 1CDA mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1CDC setc */
    spc.c = 1;
L1CDD:
    /* 1CDD rol a */
    spc.a = spc_rol(spc.a);
    /* 1CDE dec.b $22 */
    spc_rmw(spc_dp(0x22), spc_dec);
    /* 1CE0 bne $1CDD */
    if (!spc.z) goto L1CDD;
    /* 1CE2 mov.b $22,a */
    spc_st(spc_dp(0x22), spc.a);
    /* 1CE4 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FCCA SpcBVoiceMaskNot
 * WHAT IT DOES: $22 = every bit but voice X's. After it (from $1CEE, where SpcBSfxChannelStep jumps), the release of voice X to the music, as the race driver's SpcSfxRelease: its sound effect bit cleared, the music instrument reloaded (SpcLoadInstrument), its echo bit from the music's echo voices $4F, and its volume rewritten (SpcVoiceVolumeWrite).
 * HOW: ROL of a clear carry through $FF, sharing SpcBVoiceMask's loop; masks through $22, X doubled for the music's arrays.
 * Entry: X = voice. Exit: A = $22; the release keeps X.
 * @spc 1CE5
 * @spcuses 1CDD-1CEE
 */
void abi_SpcBVoiceMaskNot(void)
{
    /* 1CE5 mov.b $22,x */
    spc_st(spc_dp(0x22), spc.x);
    /* 1CE7 inc.b $22 */
    spc_rmw(spc_dp(0x22), spc_inc);
    /* 1CE9 mov a,#$FF */
    spc.a = spc_nz(0xFF);
    /* 1CEB clrc */
    spc.c = 0;
    /* 1CEC bra $1CDD */
    goto L1CDD;
L1CDD:
    /* 1CDD rol a */
    spc.a = spc_rol(spc.a);
    /* 1CDE dec.b $22 */
    spc_rmw(spc_dp(0x22), spc_dec);
    /* 1CE0 bne $1CDD */
    if (!spc.z) goto L1CDD;
    /* 1CE2 mov.b $22,a */
    spc_st(spc_dp(0x22), spc.a);
    /* 1CE4 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FD2B SpcBKeyOnVoice
 * WHAT IT DOES: Marks voice X for key-on ($45). The race driver's SpcKeyOnVoice.
 * HOW: SpcBVoiceMask, OR.
 * Entry: X = voice. Exit: A = $45.
 * @spc 1D46
 * @spcuses 1CD6-1CE5,1D46-1D50
 */
void abi_SpcBKeyOnVoice(void)
{
    /* 1D46 call $1CD6 */
    spc_call(abi_SpcBVoiceMask, 0x1D49);
    /* 1D49 mov.b a,$45 */
    spc.a = spc_nz(spc_rd(spc_dp(0x45)));
    /* 1D4B or.b a,$22 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x22)));
    /* 1D4D mov.b $45,a */
    spc_st(spc_dp(0x45), spc.a);
    /* 1D4F ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FD35 SpcBKeyOffVoice
 * WHAT IT DOES: Marks voice X for key-off ($46). The race driver's SpcKeyOffVoice. After it, SpcBSfxChannelStep's playing path: SpcBSfxRead, SpcBSfxSpecial, then the DSP writes at $21C8.
 * HOW: SpcBVoiceMask, OR.
 * Entry: X = voice. Exit: A = $46.
 * @spc 1D50
 * @spcuses 1CD6-1CE5,1D50-1D5A
 */
void abi_SpcBKeyOffVoice(void)
{
    /* 1D50 call $1CD6 */
    spc_call(abi_SpcBVoiceMask, 0x1D53);
    /* 1D53 mov.b a,$46 */
    spc.a = spc_nz(spc_rd(spc_dp(0x46)));
    /* 1D55 or.b a,$22 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x22)));
    /* 1D57 mov.b $46,a */
    spc_st(spc_dp(0x46), spc.a);
    /* 1D59 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FD48 SpcBSfxRead
 * WHAT IT DOES: Channel X's sound data for this tick, as the race driver's SpcSfxRead: its age $03F0 counts (a set lifetime $0448 stops it); on the first tick its pitch, volume, slide and envelope state is reset (ADSR $8F/$E0, echo off); while a note's length $0418 runs, the slides step (from $213F); then bytes are read from ($20)+Y: note numbers set the pitch (the table at $222B, by octave, with the channel's detune $0440), $00-$7F lengths, and commands $E0 and up run their handler from the 32-entry table at $1E16 (volume, pan, pitch and volume slides, echo on/off, ADSR, sample, lifetime, sound jumps, end).
 * HOW: JMP ($1E16+X) with X pushed; handlers return into the read loop at $1E04. The JMP at $1DFD follows the JMP to the slides and nothing reaches it.
 * Entry: X = channel. Exit: as the last handler leaves it.
 * @spc 1D63
 * @spcuses 1861-1871,1AFD-1B03,1B17-1B7E,1B93-1BA9,1C04-1C1A,1CCC-1CEE,1D50-1D5A,1D63-1DFD,1E00-217B,22D1-22F2,2588-25B8
 */
void abi_SpcBSfxRead(void)
{
    /* 1D63 mov.w a,$0390+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 1D66 beq $1D86 */
    if (spc.z) goto L1D86;
    /* 1D68 mov.w a,$0448+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0448 + spc.x)));
    /* 1D6B beq $1D79 */
    if (spc.z) goto L1D79;
    /* 1D6D mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 1D6F mov.w a,$03F0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03F0 + spc.x)));
    /* 1D72 cmp.b a,$20 */
    spc_cmp(spc.a, spc_rd(spc_dp(0x20)));
    /* 1D74 bne $1D79 */
    if (!spc.z) goto L1D79;
    /* 1D76 call $186C */
    spc_call(abi_SpcBSfxStop, 0x1D79);
L1D79:
    /* 1D79 mov.w a,$03F0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03F0 + spc.x)));
    /* 1D7C cmp a,#$FF */
    spc_cmp(spc.a, 0xFF);
    /* 1D7E beq $1D84 */
    if (spc.z) goto L1D84;
    /* 1D80 inc a */
    spc.a = spc_inc(spc.a);
    /* 1D81 mov.w $03F0+x,a */
    spc_st((uint16_t)(0x03F0 + spc.x), spc.a);
L1D84:
    /* 1D84 bra $1D8C */
    goto L1D8C;
L1D86:
    /* 1D86 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1D88 mov.w $03F0+x,a */
    spc_st((uint16_t)(0x03F0 + spc.x), spc.a);
    /* 1D8B ret */
    spc_ret();
    return;
L1D8C:
    /* 1D8C call $1B93 */
    spc_call(abi_SpcBSfxChannelLoad, 0x1D8F);
    /* 1D8F mov.w a,$03F0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03F0 + spc.x)));
    /* 1D92 beq $1D9A */
    if (spc.z) goto L1D9A;
    /* 1D94 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    /* 1D96 beq $1D9B */
    if (spc.z) goto L1D9B;
    /* 1D98 bra $1DED */
    goto L1DED;
L1D9A:
    /* 1D9A ret */
    spc_ret();
    return;
L1D9B:
    /* 1D9B mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1D9D mov.w $0400+x,a */
    spc_st((uint16_t)(0x0400 + spc.x), spc.a);
    /* 1DA0 mov.w $0438+x,a */
    spc_st((uint16_t)(0x0438 + spc.x), spc.a);
    /* 1DA3 mov.w $0520+x,a */
    spc_st((uint16_t)(0x0520 + spc.x), spc.a);
    /* 1DA6 mov.w $0478+x,a */
    spc_st((uint16_t)(0x0478 + spc.x), spc.a);
    /* 1DA9 mov.w $0448+x,a */
    spc_st((uint16_t)(0x0448 + spc.x), spc.a);
    /* 1DAC mov.w $0440+x,a */
    spc_st((uint16_t)(0x0440 + spc.x), spc.a);
    /* 1DAF mov.w $03A8+x,a */
    spc_st((uint16_t)(0x03A8 + spc.x), spc.a);
    /* 1DB2 mov.w $03B0+x,a */
    spc_st((uint16_t)(0x03B0 + spc.x), spc.a);
    /* 1DB5 mov.w $03B8+x,a */
    spc_st((uint16_t)(0x03B8 + spc.x), spc.a);
    /* 1DB8 mov.w $0498+x,a */
    spc_st((uint16_t)(0x0498 + spc.x), spc.a);
    /* 1DBB mov.w $03C0+x,a */
    spc_st((uint16_t)(0x03C0 + spc.x), spc.a);
    /* 1DBE mov.w $04A0+x,a */
    spc_st((uint16_t)(0x04A0 + spc.x), spc.a);
    /* 1DC1 mov.w $04C0+x,a */
    spc_st((uint16_t)(0x04C0 + spc.x), spc.a);
    /* 1DC4 mov.w $04C8+x,a */
    spc_st((uint16_t)(0x04C8 + spc.x), spc.a);
    /* 1DC7 mov.w $04F0+x,a */
    spc_st((uint16_t)(0x04F0 + spc.x), spc.a);
    /* 1DCA mov.w $0398+x,a */
    spc_st((uint16_t)(0x0398 + spc.x), spc.a);
    /* 1DCD mov.w $04A8+x,a */
    spc_st((uint16_t)(0x04A8 + spc.x), spc.a);
    /* 1DD0 mov.w $03A0+x,a */
    spc_st((uint16_t)(0x03A0 + spc.x), spc.a);
    /* 1DD3 mov.w $04B0+x,a */
    spc_st((uint16_t)(0x04B0 + spc.x), spc.a);
    /* 1DD6 mov a,#$8F */
    spc.a = spc_nz(0x8F);
    /* 1DD8 mov.w $03D0+x,a */
    spc_st((uint16_t)(0x03D0 + spc.x), spc.a);
    /* 1DDB mov a,#$E0 */
    spc.a = spc_nz(0xE0);
    /* 1DDD mov.w $03D8+x,a */
    spc_st((uint16_t)(0x03D8 + spc.x), spc.a);
    /* 1DE0 call $1CE5 */
    spc_call(abi_SpcBVoiceMaskNot, 0x1DE3);
    /* 1DE3 mov.b a,$4A */
    spc.a = spc_nz(spc_rd(spc_dp(0x4A)));
    /* 1DE5 and.b a,$22 */
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0x22)));
    /* 1DE7 mov.b $4A,a */
    spc_st(spc_dp(0x4A), spc.a);
    /* 1DE9 mov y,#$01 */
    spc.y = spc_nz(0x01);
    /* 1DEB bra $1E05 */
    goto L1E05;
L1DED:
    /* 1DED mov.w a,$0418+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0418 + spc.x)));
    /* 1DF0 dec a */
    spc.a = spc_dec(spc.a);
    /* 1DF1 mov.w $0418+x,a */
    spc_st((uint16_t)(0x0418 + spc.x), spc.a);
    /* 1DF4 beq $1E00 */
    if (spc.z) goto L1E00;
    /* 1DF6 cmp a,#$FF */
    spc_cmp(spc.a, 0xFF);
    /* 1DF8 beq $1E00 */
    if (spc.z) goto L1E00;
    /* 1DFA jmp $213F */
    goto L213F;
L1E00:
    /* 1E00 mov.w a,$0400+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0400 + spc.x)));
    /* 1E03 mov y,a */
    spc.y = spc_nz(spc.a);
L1E04:
    /* 1E04 inc y */
    spc.y = spc_inc(spc.y);
L1E05:
    /* 1E05 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 1E07 beq $1E04 */
    if (spc.z) goto L1E04;
    /* 1E09 bpl $1E56 */
    if (!spc.n) goto L1E56;
    /* 1E0B setc */
    spc.c = 1;
    /* 1E0C sbc a,#$E0 */
    spc.a = spc_sbc(spc.a, 0xE0);
    /* 1E0E bmi $1E87 */
    if (spc.n) goto L1E87;
    /* 1E10 asl a */
    spc.a = spc_asl(spc.a);
    /* 1E11 push x */
    spc_push(spc.x);
    /* 1E12 mov x,a */
    spc.x = spc_nz(spc.a);
    /* 1E13 jmp ($1E16+x) */
    { uint16_t t = spc_rd((uint16_t)(0x1E16 + spc.x)); t |= spc_rd((uint16_t)(0x1E16 + spc.x + 1)) << 8;
      switch (t) {
      case 0x1F0E: goto L1F0E;
      case 0x1F12: goto L1F12;
      case 0x1F1B: goto L1F1B;
      case 0x1F68: goto L1F68;
      case 0x1F6A: goto L1F6A;
      case 0x1FD4: goto L1FD4;
      case 0x1FE3: goto L1FE3;
      case 0x1FFE: goto L1FFE;
      case 0x2012: goto L2012;
      case 0x2027: goto L2027;
      case 0x203C: goto L203C;
      case 0x2048: goto L2048;
      case 0x2055: goto L2055;
      case 0x20B6: goto L20B6;
      case 0x20C0: goto L20C0;
      case 0x20E1: goto L20E1;
      case 0x20EB: goto L20EB;
      case 0x20F4: goto L20F4;
      case 0x20FD: goto L20FD;
      case 0x210F: goto L210F;
      case 0x2119: goto L2119;
      case 0x2125: goto L2125;
      case 0x212F: goto L212F;
      } }
    return;
L1E56:
    /* 1E56 push a */
    spc_push(spc.a);
    /* 1E57 mov.w a,$0478+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0478 + spc.x)));
    /* 1E5A beq $1E62 */
    if (spc.z) goto L1E62;
    /* 1E5C cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    /* 1E5E beq $1E65 */
    if (spc.z) goto L1E65;
    /* 1E60 bra $1E6A */
    goto L1E6A;
L1E62:
    /* 1E62 call $1D50 */
    spc_call(abi_SpcBKeyOffVoice, 0x1E65);
L1E65:
    /* 1E65 mov a,#$01 */
    spc.a = spc_nz(0x01);
    /* 1E67 mov.w $03E8+x,a */
    spc_st((uint16_t)(0x03E8 + spc.x), spc.a);
L1E6A:
    /* 1E6A mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1E6C mov.w $0478+x,a */
    spc_st((uint16_t)(0x0478 + spc.x), spc.a);
    /* 1E6F pop a */
    spc.a = spc_pull();
    /* 1E70 mov.w $0418+x,a */
    spc_st((uint16_t)(0x0418 + spc.x), spc.a);
    /* 1E73 cmp a,#$7F */
    spc_cmp(spc.a, 0x7F);
    /* 1E75 bne $1E7B */
    if (!spc.z) goto L1E7B;
    /* 1E77 mov a,#$01 */
    spc.a = spc_nz(0x01);
    /* 1E79 bra $1E7D */
    goto L1E7D;
L1E7B:
    /* 1E7B mov a,#$00 */
    spc.a = spc_nz(0x00);
L1E7D:
    /* 1E7D mov.w $0420+x,a */
    spc_st((uint16_t)(0x0420 + spc.x), spc.a);
    /* 1E80 mov a,y */
    spc.a = spc_nz(spc.y);
    /* 1E81 mov.w $0400+x,a */
    spc_st((uint16_t)(0x0400 + spc.x), spc.a);
    /* 1E84 jmp $1B9E */
    { abi_SpcBSfxChannelSave(); return; }
L1E87:
    /* 1E87 push y */
    spc_push(spc.y);
    /* 1E88 push x */
    spc_push(spc.x);
    /* 1E89 clrc */
    spc.c = 0;
    /* 1E8A adc a,#$60 */
    spc.a = spc_adc(spc.a, 0x60);
    /* 1E8C mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 1E8E mov x,#$0C */
    spc.x = spc_nz(0x0C);
    /* 1E90 div ya,x */
    spc_div();
    /* 1E91 push a */
    spc_push(spc.a);
    /* 1E92 mov a,y */
    spc.a = spc_nz(spc.y);
    /* 1E93 asl a */
    spc.a = spc_asl(spc.a);
    /* 1E94 mov x,a */
    spc.x = spc_nz(spc.a);
    /* 1E95 mov.w a,$222B+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x222B + spc.x)));
    /* 1E98 mov.b $22,a */
    spc_st(spc_dp(0x22), spc.a);
    /* 1E9A mov.w a,$222C+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x222C + spc.x)));
    /* 1E9D mov.b $23,a */
    spc_st(spc_dp(0x23), spc.a);
    /* 1E9F pop a */
    spc.a = spc_pull();
    /* 1EA0 setc */
    spc.c = 1;
    /* 1EA1 sbc a,#$07 */
    spc.a = spc_sbc(spc.a, 0x07);
    /* 1EA3 mov.b $24,a */
    spc_st(spc_dp(0x24), spc.a);
    /* 1EA5 beq $1EB9 */
    if (spc.z) goto L1EB9;
L1EA7:
    /* 1EA7 mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 1EA9 mov.b a,$23 */
    spc.a = spc_nz(spc_rd(spc_dp(0x23)));
    /* 1EAB mov x,#$02 */
    spc.x = spc_nz(0x02);
    /* 1EAD div ya,x */
    spc_div();
    /* 1EAE mov.b $23,a */
    spc_st(spc_dp(0x23), spc.a);
    /* 1EB0 mov.b a,$22 */
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 1EB2 div ya,x */
    spc_div();
    /* 1EB3 mov.b $22,a */
    spc_st(spc_dp(0x22), spc.a);
    /* 1EB5 inc.b $24 */
    spc_rmw(spc_dp(0x24), spc_inc);
    /* 1EB7 bne $1EA7 */
    if (!spc.z) goto L1EA7;
L1EB9:
    /* 1EB9 pop x */
    spc.x = spc_pull();
    /* 1EBA mov.b a,$22 */
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 1EBC mov.w $03B8+x,a */
    spc_st((uint16_t)(0x03B8 + spc.x), spc.a);
    /* 1EBF mov.w $0498+x,a */
    spc_st((uint16_t)(0x0498 + spc.x), spc.a);
    /* 1EC2 mov.b a,$23 */
    spc.a = spc_nz(spc_rd(spc_dp(0x23)));
    /* 1EC4 mov.w $03C0+x,a */
    spc_st((uint16_t)(0x03C0 + spc.x), spc.a);
    /* 1EC7 mov.w $04A0+x,a */
    spc_st((uint16_t)(0x04A0 + spc.x), spc.a);
    /* 1ECA mov.w a,$0440+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0440 + spc.x)));
    /* 1ECD beq $1F0A */
    if (spc.z) goto L1F0A;
    /* 1ECF push x */
    spc_push(spc.x);
    /* 1ED0 mov.b $24,a */
    spc_st(spc_dp(0x24), spc.a);
    /* 1ED2 mov.b a,$23 */
    spc.a = spc_nz(spc_rd(spc_dp(0x23)));
    /* 1ED4 setc */
    spc.c = 1;
    /* 1ED5 sbc a,#$10 */
    spc.a = spc_sbc(spc.a, 0x10);
    /* 1ED7 bmi $1EEB */
    if (spc.n) goto L1EEB;
    /* 1ED9 mov.b a,$22 */
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 1EDB mov.b y,$23 */
    spc.y = spc_nz(spc_rd(spc_dp(0x23)));
    /* 1EDD mov x,#$43 */
    spc.x = spc_nz(0x43);
    /* 1EDF div ya,x */
    spc_div();
    /* 1EE0 mov y,#$04 */
    spc.y = spc_nz(0x04);
    /* 1EE2 mul ya */
    spc_mul();
    /* 1EE3 mov x,#$FC */
    spc.x = spc_nz(0xFC);
    /* 1EE5 div ya,x */
    spc_div();
    /* 1EE6 mov.b y,$24 */
    spc.y = spc_nz(spc_rd(spc_dp(0x24)));
    /* 1EE8 mul ya */
    spc_mul();
    /* 1EE9 bra $1F00 */
    goto L1F00;
L1EEB:
    /* 1EEB mov.b a,$22 */
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 1EED mov.b y,$23 */
    spc.y = spc_nz(spc_rd(spc_dp(0x23)));
    /* 1EEF mov x,#$11 */
    spc.x = spc_nz(0x11);
    /* 1EF1 div ya,x */
    spc_div();
    /* 1EF2 mov x,a */
    spc.x = spc_nz(spc.a);
    /* 1EF3 mov a,#$FF */
    spc.a = spc_nz(0xFF);
    /* 1EF5 mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 1EF7 div ya,x */
    spc_div();
    /* 1EF8 mov x,a */
    spc.x = spc_nz(spc.a);
    /* 1EF9 mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 1EFB mov.b a,$24 */
    spc.a = spc_nz(spc_rd(spc_dp(0x24)));
    /* 1EFD div ya,x */
    spc_div();
    /* 1EFE mov y,#$00 */
    spc.y = spc_nz(0x00);
L1F00:
    /* 1F00 addw ya,$22 */
    { uint16_t v = spc_rd(spc_dp(0x22)); v |= spc_rd(spc_dp(0x23)) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 1F02 pop x */
    spc.x = spc_pull();
    /* 1F03 mov.w $03B8+x,a */
    spc_st((uint16_t)(0x03B8 + spc.x), spc.a);
    /* 1F06 mov a,y */
    spc.a = spc_nz(spc.y);
    /* 1F07 mov.w $03C0+x,a */
    spc_st((uint16_t)(0x03C0 + spc.x), spc.a);
L1F0A:
    /* 1F0A pop y */
    spc.y = spc_pull();
    /* 1F0B jmp $1E04 */
    goto L1E04;
L1F0E:
    /* 1F0E mov a,#$01 */
    spc.a = spc_nz(0x01);
    /* 1F10 bra $1F14 */
    goto L1F14;
L1F12:
    /* 1F12 mov a,#$02 */
    spc.a = spc_nz(0x02);
L1F14:
    /* 1F14 pop x */
    spc.x = spc_pull();
    /* 1F15 mov.w $0478+x,a */
    spc_st((uint16_t)(0x0478 + spc.x), spc.a);
    /* 1F18 jmp $1E04 */
    goto L1E04;
L1F1B:
    /* 1F1B pop x */
    spc.x = spc_pull();
    /* 1F1C inc y */
    spc.y = spc_inc(spc.y);
    /* 1F1D mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 1F1F mov.w $03C8+x,a */
    spc_st((uint16_t)(0x03C8 + spc.x), spc.a);
    /* 1F22 jmp $1E04 */
    goto L1E04;
L1F68:
    /* 1F68 set1 $2C.6 */
    spc_bit_set(0x2C, 6, 1);
L1F6A:
    /* 1F6A pop x */
    spc.x = spc_pull();
    /* 1F6B inc y */
    spc.y = spc_inc(spc.y);
    /* 1F6C mov.w a,$04F0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F0 + spc.x)));
    /* 1F6F cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    /* 1F71 bne $1F90 */
    if (!spc.z) goto L1F90;
    /* 1F73 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 1F75 call $1F25 */
    spc_call(abi_SpcBSfxEngineScale, 0x1F78);
    /* 1F78 mov.w $0398+x,a */
    spc_st((uint16_t)(0x0398 + spc.x), spc.a);
    /* 1F7B mov.w $04A8+x,a */
    spc_st((uint16_t)(0x04A8 + spc.x), spc.a);
    /* 1F7E bbs $2C.6,$1F82 */
    if ((spc_rd(spc_dp(0x2C)) >> 6 & 1)) goto L1F82;
    /* 1F81 inc y */
    spc.y = spc_inc(spc.y);
L1F82:
    /* 1F82 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 1F84 call $1F25 */
    spc_call(abi_SpcBSfxEngineScale, 0x1F87);
    /* 1F87 mov.w $03A0+x,a */
    spc_st((uint16_t)(0x03A0 + spc.x), spc.a);
    /* 1F8A mov.w $04B0+x,a */
    spc_st((uint16_t)(0x04B0 + spc.x), spc.a);
    /* 1F8D jmp $1FC7 */
    goto L1FC7;
L1F90:
    /* 1F90 mov.w a,$04B8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04B8 + spc.x)));
    /* 1F93 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    /* 1F95 beq $1FB0 */
    if (spc.z) goto L1FB0;
    /* 1F97 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 1F99 call $1F25 */
    spc_call(abi_SpcBSfxEngineScale, 0x1F9C);
    /* 1F9C mov.w $0398+x,a */
    spc_st((uint16_t)(0x0398 + spc.x), spc.a);
    /* 1F9F mov.w $04A8+x,a */
    spc_st((uint16_t)(0x04A8 + spc.x), spc.a);
    /* 1FA2 bbs $2C.6,$1FA6 */
    if ((spc_rd(spc_dp(0x2C)) >> 6 & 1)) goto L1FA6;
    /* 1FA5 inc y */
    spc.y = spc_inc(spc.y);
L1FA6:
    /* 1FA6 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1FA8 mov.w $03A0+x,a */
    spc_st((uint16_t)(0x03A0 + spc.x), spc.a);
    /* 1FAB mov.w $04B0+x,a */
    spc_st((uint16_t)(0x04B0 + spc.x), spc.a);
    /* 1FAE bra $1FC7 */
    goto L1FC7;
L1FB0:
    /* 1FB0 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1FB2 mov.w $0398+x,a */
    spc_st((uint16_t)(0x0398 + spc.x), spc.a);
    /* 1FB5 mov.w $04A8+x,a */
    spc_st((uint16_t)(0x04A8 + spc.x), spc.a);
    /* 1FB8 bbs $2C.6,$1FBC */
    if ((spc_rd(spc_dp(0x2C)) >> 6 & 1)) goto L1FBC;
    /* 1FBB inc y */
    spc.y = spc_inc(spc.y);
L1FBC:
    /* 1FBC mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 1FBE call $1F25 */
    spc_call(abi_SpcBSfxEngineScale, 0x1FC1);
    /* 1FC1 mov.w $03A0+x,a */
    spc_st((uint16_t)(0x03A0 + spc.x), spc.a);
    /* 1FC4 mov.w $04B0+x,a */
    spc_st((uint16_t)(0x04B0 + spc.x), spc.a);
L1FC7:
    /* 1FC7 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1FC9 mov.w $03A8+x,a */
    spc_st((uint16_t)(0x03A8 + spc.x), spc.a);
    /* 1FCC mov.w $03B0+x,a */
    spc_st((uint16_t)(0x03B0 + spc.x), spc.a);
    /* 1FCF clr1 $2C.6 */
    spc_bit_set(0x2C, 6, 0);
    /* 1FD1 jmp $1E04 */
    goto L1E04;
L1FD4:
    /* 1FD4 pop x */
    spc.x = spc_pull();
    /* 1FD5 inc y */
    spc.y = spc_inc(spc.y);
    /* 1FD6 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 1FD8 and a,#$0F */
    spc.a = spc_and(spc.a, 0x0F);
    /* 1FDA mov.b $22,a */
    spc_st(spc_dp(0x22), spc.a);
    /* 1FDC mov.w a,$03D0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03D0 + spc.x)));
    /* 1FDF and a,#$F0 */
    spc.a = spc_and(spc.a, 0xF0);
    /* 1FE1 bra $1FF4 */
    goto L1FF4;
L1FE3:
    /* 1FE3 pop x */
    spc.x = spc_pull();
    /* 1FE4 inc y */
    spc.y = spc_inc(spc.y);
    /* 1FE5 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 1FE7 and a,#$07 */
    spc.a = spc_and(spc.a, 0x07);
    /* 1FE9 asl a */
    spc.a = spc_asl(spc.a);
    /* 1FEA asl a */
    spc.a = spc_asl(spc.a);
    /* 1FEB asl a */
    spc.a = spc_asl(spc.a);
    /* 1FEC asl a */
    spc.a = spc_asl(spc.a);
    /* 1FED mov.b $22,a */
    spc_st(spc_dp(0x22), spc.a);
    /* 1FEF mov.w a,$03D0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03D0 + spc.x)));
    /* 1FF2 and a,#$8F */
    spc.a = spc_and(spc.a, 0x8F);
L1FF4:
    /* 1FF4 or.b a,$22 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x22)));
    /* 1FF6 or a,#$80 */
    spc.a = spc_or(spc.a, 0x80);
    /* 1FF8 mov.w $03D0+x,a */
    spc_st((uint16_t)(0x03D0 + spc.x), spc.a);
    /* 1FFB jmp $1E04 */
    goto L1E04;
L1FFE:
    /* 1FFE pop x */
    spc.x = spc_pull();
    /* 1FFF inc y */
    spc.y = spc_inc(spc.y);
    /* 2000 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2002 and a,#$07 */
    spc.a = spc_and(spc.a, 0x07);
    /* 2004 asl a */
    spc.a = spc_asl(spc.a);
    /* 2005 asl a */
    spc.a = spc_asl(spc.a);
    /* 2006 asl a */
    spc.a = spc_asl(spc.a);
    /* 2007 asl a */
    spc.a = spc_asl(spc.a);
    /* 2008 asl a */
    spc.a = spc_asl(spc.a);
    /* 2009 mov.b $22,a */
    spc_st(spc_dp(0x22), spc.a);
    /* 200B mov.w a,$03D8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03D8 + spc.x)));
    /* 200E and a,#$1F */
    spc.a = spc_and(spc.a, 0x1F);
    /* 2010 bra $201F */
    goto L201F;
L2012:
    /* 2012 pop x */
    spc.x = spc_pull();
    /* 2013 inc y */
    spc.y = spc_inc(spc.y);
    /* 2014 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2016 and a,#$1F */
    spc.a = spc_and(spc.a, 0x1F);
    /* 2018 mov.b $22,a */
    spc_st(spc_dp(0x22), spc.a);
    /* 201A mov.w a,$03D8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03D8 + spc.x)));
    /* 201D and a,#$E0 */
    spc.a = spc_and(spc.a, 0xE0);
L201F:
    /* 201F or.b a,$22 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x22)));
    /* 2021 mov.w $03D8+x,a */
    spc_st((uint16_t)(0x03D8 + spc.x), spc.a);
    /* 2024 jmp $1E04 */
    goto L1E04;
L2027:
    /* 2027 pop x */
    spc.x = spc_pull();
    /* 2028 inc y */
    spc.y = spc_inc(spc.y);
    /* 2029 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 202B mov.w $0428+x,a */
    spc_st((uint16_t)(0x0428 + spc.x), spc.a);
    /* 202E inc y */
    spc.y = spc_inc(spc.y);
    /* 202F mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2031 mov.w $0430+x,a */
    spc_st((uint16_t)(0x0430 + spc.x), spc.a);
    /* 2034 mov a,#$01 */
    spc.a = spc_nz(0x01);
    /* 2036 mov.w $0438+x,a */
    spc_st((uint16_t)(0x0438 + spc.x), spc.a);
    /* 2039 jmp $1E04 */
    goto L1E04;
L203C:
    /* 203C pop x */
    spc.x = spc_pull();
    /* 203D call $1CD6 */
    spc_call(abi_SpcBVoiceMask, 0x2040);
    /* 2040 mov.b a,$4A */
    spc.a = spc_nz(spc_rd(spc_dp(0x4A)));
    /* 2042 or.b a,$22 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x22)));
    /* 2044 mov.b $4A,a */
    spc_st(spc_dp(0x4A), spc.a);
    /* 2046 bra $2052 */
    goto L2052;
L2048:
    /* 2048 pop x */
    spc.x = spc_pull();
    /* 2049 call $1CE5 */
    spc_call(abi_SpcBVoiceMaskNot, 0x204C);
    /* 204C mov.b a,$4A */
    spc.a = spc_nz(spc_rd(spc_dp(0x4A)));
    /* 204E and.b a,$22 */
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0x22)));
    /* 2050 mov.b $4A,a */
    spc_st(spc_dp(0x4A), spc.a);
L2052:
    /* 2052 jmp $1E04 */
    goto L1E04;
L2055:
    /* 2055 inc y */
    spc.y = spc_inc(spc.y);
    /* 2056 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2058 mov.b $24,a */
    spc_st(spc_dp(0x24), spc.a);
    /* 205A inc y */
    spc.y = spc_inc(spc.y);
    /* 205B mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 205D mov.b $25,a */
    spc_st(spc_dp(0x25), spc.a);
    /* 205F push y */
    spc_push(spc.y);
    /* 2060 mov x,#$BC */
    spc.x = spc_nz(0xBC);
L2062:
    /* 2062 mov.w a,$0702+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0702 + spc.x)));
    /* 2065 cmp.b a,$24 */
    spc_cmp(spc.a, spc_rd(spc_dp(0x24)));
    /* 2067 beq $206D */
    if (spc.z) goto L206D;
    /* 2069 inc x */
    spc.x = spc_inc(spc.x);
    /* 206A inc x */
    spc.x = spc_inc(spc.x);
    /* 206B bra $2062 */
    goto L2062;
L206D:
    /* 206D inc x */
    spc.x = spc_inc(spc.x);
    /* 206E mov.w a,$0702+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0702 + spc.x)));
    /* 2071 cmp.b a,$25 */
    spc_cmp(spc.a, spc_rd(spc_dp(0x25)));
    /* 2073 beq $207C */
    if (spc.z) goto L207C;
    /* 2075 inc x */
    spc.x = spc_inc(spc.x);
    /* 2076 cmp x,#$00 */
    spc_cmp(spc.x, 0x00);
    /* 2078 beq $2082 */
    if (spc.z) goto L2082;
    /* 207A bra $2062 */
    goto L2062;
L207C:
    /* 207C mov a,x */
    spc.a = spc_nz(spc.x);
    /* 207D lsr a */
    spc.a = spc_lsr(spc.a);
    /* 207E inc a */
    spc.a = spc_inc(spc.a);
    /* 207F call $2087 */
    spc_call(abi_SpcBSfxStart, 0x2082);
L2082:
    /* 2082 pop y */
    spc.y = spc_pull();
    /* 2083 pop x */
    spc.x = spc_pull();
    /* 2084 jmp $1E04 */
    goto L1E04;
L20B6:
    /* 20B6 pop x */
    spc.x = spc_pull();
    /* 20B7 mov.w a,$0420+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0420 + spc.x)));
    /* 20BA bne $20BF */
    if (!spc.z) goto L20BF;
    /* 20BC call $186C */
    spc_call(abi_SpcBSfxStop, 0x20BF);
L20BF:
    /* 20BF ret */
    spc_ret();
    return;
L20C0:
    /* 20C0 pop x */
    spc.x = spc_pull();
    /* 20C1 inc y */
    spc.y = spc_inc(spc.y);
    /* 20C2 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 20C4 mov.w $0500+x,a */
    spc_st((uint16_t)(0x0500 + spc.x), spc.a);
    /* 20C7 inc y */
    spc.y = spc_inc(spc.y);
    /* 20C8 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 20CA mov.w $0508+x,a */
    spc_st((uint16_t)(0x0508 + spc.x), spc.a);
    /* 20CD inc y */
    spc.y = spc_inc(spc.y);
    /* 20CE mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 20D0 mov.w $0510+x,a */
    spc_st((uint16_t)(0x0510 + spc.x), spc.a);
    /* 20D3 inc y */
    spc.y = spc_inc(spc.y);
    /* 20D4 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 20D6 mov.w $0518+x,a */
    spc_st((uint16_t)(0x0518 + spc.x), spc.a);
    /* 20D9 mov a,#$01 */
    spc.a = spc_nz(0x01);
    /* 20DB mov.w $0520+x,a */
    spc_st((uint16_t)(0x0520 + spc.x), spc.a);
    /* 20DE jmp $1E04 */
    goto L1E04;
L20E1:
    /* 20E1 pop x */
    spc.x = spc_pull();
    /* 20E2 inc y */
    spc.y = spc_inc(spc.y);
    /* 20E3 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 20E5 mov.w $0440+x,a */
    spc_st((uint16_t)(0x0440 + spc.x), spc.a);
    /* 20E8 jmp $1E04 */
    goto L1E04;
L20EB:
    /* 20EB pop x */
    spc.x = spc_pull();
    /* 20EC mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 20EE mov.w $0438+x,a */
    spc_st((uint16_t)(0x0438 + spc.x), spc.a);
    /* 20F1 jmp $1E04 */
    goto L1E04;
L20F4:
    /* 20F4 pop x */
    spc.x = spc_pull();
    /* 20F5 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 20F7 mov.w $0520+x,a */
    spc_st((uint16_t)(0x0520 + spc.x), spc.a);
    /* 20FA jmp $1E04 */
    goto L1E04;
L20FD:
    /* 20FD pop x */
    spc.x = spc_pull();
    /* 20FE inc y */
    spc.y = spc_inc(spc.y);
    /* 20FF mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2101 push a */
    spc_push(spc.a);
    /* 2102 inc y */
    spc.y = spc_inc(spc.y);
    /* 2103 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2105 mov.b $21,a */
    spc_st(spc_dp(0x21), spc.a);
    /* 2107 pop a */
    spc.a = spc_pull();
    /* 2108 mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 210A mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 210C jmp $1E05 */
    goto L1E05;
L210F:
    /* 210F pop x */
    spc.x = spc_pull();
    /* 2110 inc y */
    spc.y = spc_inc(spc.y);
    /* 2111 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2113 mov.w $0448+x,a */
    spc_st((uint16_t)(0x0448 + spc.x), spc.a);
    /* 2116 jmp $1E04 */
    goto L1E04;
L2119:
    /* 2119 pop x */
    spc.x = spc_pull();
    /* 211A call $1D50 */
    spc_call(abi_SpcBKeyOffVoice, 0x211D);
    /* 211D mov a,#$02 */
    spc.a = spc_nz(0x02);
    /* 211F mov.w $0478+x,a */
    spc_st((uint16_t)(0x0478 + spc.x), spc.a);
    /* 2122 jmp $1E04 */
    goto L1E04;
L2125:
    /* 2125 pop x */
    spc.x = spc_pull();
    /* 2126 inc y */
    spc.y = spc_inc(spc.y);
    /* 2127 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2129 mov.w $04F0+x,a */
    spc_st((uint16_t)(0x04F0 + spc.x), spc.a);
    /* 212C jmp $1E04 */
    goto L1E04;
L212F:
    /* 212F pop x */
    spc.x = spc_pull();
    /* 2130 inc y */
    spc.y = spc_inc(spc.y);
    /* 2131 mov a,($20)+y */
    { uint16_t p = spc_rd(spc_dp(0x20)); p |= spc_rd(spc_dp(0x21)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 2133 and a,#$FE */
    spc.a = spc_and(spc.a, 0xFE);
    /* 2135 push y */
    spc_push(spc.y);
    /* 2136 push x */
    spc_push(spc.x);
    /* 2137 call $2588 */
    spc_call(abi_SpcBNoiseSampleInit, 0x213A);
    /* 213A pop x */
    spc.x = spc_pull();
    /* 213B pop y */
    spc.y = spc_pull();
    /* 213C jmp $1E04 */
    goto L1E04;
L213F:
    /* 213F mov.w a,$0438+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0438 + spc.x)));
    /* 2142 beq $216F */
    if (spc.z) goto L216F;
    /* 2144 mov.w a,$0428+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0428 + spc.x)));
    /* 2147 mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 2149 mov.w a,$0430+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0430 + spc.x)));
    /* 214C mov.b $21,a */
    spc_st(spc_dp(0x21), spc.a);
    /* 214E mov.w a,$04A0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04A0 + spc.x)));
    /* 2151 mov y,a */
    spc.y = spc_nz(spc.a);
    /* 2152 mov.w a,$0498+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0498 + spc.x)));
    /* 2155 addw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 2157 mov.w $0498+x,a */
    spc_st((uint16_t)(0x0498 + spc.x), spc.a);
    /* 215A mov a,y */
    spc.a = spc_nz(spc.y);
    /* 215B mov.w $04A0+x,a */
    spc_st((uint16_t)(0x04A0 + spc.x), spc.a);
    /* 215E bmi $2170 */
    if (spc.n) goto L2170;
    /* 2160 setc */
    spc.c = 1;
    /* 2161 sbc a,#$40 */
    spc.a = spc_sbc(spc.a, 0x40);
    /* 2163 bpl $2175 */
    if (!spc.n) goto L2175;
    /* 2165 mov.w a,$0498+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0498 + spc.x)));
L2168:
    /* 2168 mov.w $03B8+x,a */
    spc_st((uint16_t)(0x03B8 + spc.x), spc.a);
    /* 216B mov a,y */
    spc.a = spc_nz(spc.y);
    /* 216C mov.w $03C0+x,a */
    spc_st((uint16_t)(0x03C0 + spc.x), spc.a);
L216F:
    /* 216F ret */
    spc_ret();
    return;
L2170:
    /* 2170 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 2172 mov y,a */
    spc.y = spc_nz(spc.a);
    /* 2173 bra $2168 */
    goto L2168;
L2175:
    /* 2175 mov a,#$FF */
    spc.a = spc_nz(0xFF);
    /* 2177 mov y,#$3F */
    spc.y = spc_nz(0x3F);
    /* 2179 bra $2168 */
    goto L2168;
}

/*
 * @routine 1A:FD7F SpcBSfxReadNoAge
 * WHAT IT DOES: SpcBSfxRead's return for a channel whose age is 0, which never happens there (as SpcSfxReadNoAge); a direct call is its only way in.
 * HOW: RET.
 * Entry: none. Exit: nothing changed.
 * @spc 1D9A
 * @spcuses 1D9A-1D9B
 */
void abi_SpcBSfxReadNoAge(void)
{
    /* 1D9A ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FDE2 SpcBSfxReadVolumeSlides
 * WHAT IT DOES: The race driver's volume slide step (in its SpcSfxRead), left in the battle block after an unconditional JMP: with channel X's slide on ($0520), the left delta $0500/$0508 and the right delta $0510/$0518 each go through SpcBSfxVolumeSlide, the right side at X + 8. Nothing jumps to it; a direct call is its only way in.
 * HOW: JMP to the step at $217B; two CALLs with X stepped by 8 and back.
 * Entry: X = channel. Exit: A = X; X unchanged.
 * @spc 1DFD
 * @spcuses 1DFD-1E00,217B-21C8
 */
void abi_SpcBSfxReadVolumeSlides(void)
{
    /* 1DFD jmp $217B */
    goto L217B;
L217B:
    /* 217B mov.w a,$0520+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0520 + spc.x)));
    /* 217E beq $21A4 */
    if (spc.z) goto L21A4;
    /* 2180 mov.w a,$0500+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0500 + spc.x)));
    /* 2183 mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 2185 mov.w a,$0508+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0508 + spc.x)));
    /* 2188 mov.b $21,a */
    spc_st(spc_dp(0x21), spc.a);
    /* 218A call $21A5 */
    spc_call(abi_SpcBSfxVolumeSlide, 0x218D);
    /* 218D mov.w a,$0510+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0510 + spc.x)));
    /* 2190 mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 2192 mov.w a,$0518+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0518 + spc.x)));
    /* 2195 mov.b $21,a */
    spc_st(spc_dp(0x21), spc.a);
    /* 2197 mov a,x */
    spc.a = spc_nz(spc.x);
    /* 2198 clrc */
    spc.c = 0;
    /* 2199 adc a,#$08 */
    spc.a = spc_adc(spc.a, 0x08);
    /* 219B mov x,a */
    spc.x = spc_nz(spc.a);
    /* 219C call $21A5 */
    spc_call(abi_SpcBSfxVolumeSlide, 0x219F);
    /* 219F mov a,x */
    spc.a = spc_nz(spc.x);
    /* 21A0 setc */
    spc.c = 1;
    /* 21A1 sbc a,#$08 */
    spc.a = spc_sbc(spc.a, 0x08);
    /* 21A3 mov x,a */
    spc.x = spc_nz(spc.a);
L21A4:
    /* 21A4 ret */
    spc_ret();
    return;
}

/*
 * @routine 1A:FF0A SpcBSfxEngineScale
 * WHAT IT DOES: For a channel in engine mode ($04F0 = 4), scales the volume A by the engine speed from port 3, or port 2 for side 2 ($02 or $03 * 3, / 2, + $80); otherwise A is kept. After it, SpcBSfxRead's command handlers: the volume command (in engine mode both sides; otherwise side 1's sounds play on the left only and side 2's on the right), the ADSR fields, the pitch slide, echo on and off, and a jump to another sound (the one whose data pointer is the operand, from sound $5F on, started through SpcBSfxStart). The race driver's SpcSfxEngineScale reads port 3 only.
 * HOW: SpcBDiv16, SpcBMul16; clamped to $7F; the handlers return to $1E04.
 * Entry: A = volume, X = channel. Exit: A = volume; X, Y kept.
 * @spc 1F25
 * @spcuses 1F25-1F68,22D1-22F2
 */
void abi_SpcBSfxEngineScale(void)
{
    /* 1F25 push x */
    spc_push(spc.x);
    /* 1F26 push y */
    spc_push(spc.y);
    /* 1F27 mov.b $22,a */
    spc_st(spc_dp(0x22), spc.a);
    /* 1F29 mov.w a,$04F0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F0 + spc.x)));
    /* 1F2C cmp a,#$04 */
    spc_cmp(spc.a, 0x04);
    /* 1F2E bne $1F64 */
    if (!spc.z) goto L1F64;
    /* 1F30 mov.w a,$04B8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04B8 + spc.x)));
    /* 1F33 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    /* 1F35 beq $1F3B */
    if (spc.z) goto L1F3B;
    /* 1F37 mov.b a,$03 */
    spc.a = spc_nz(spc_rd(spc_dp(0x03)));
    /* 1F39 bra $1F3D */
    goto L1F3D;
L1F3B:
    /* 1F3B mov.b a,$02 */
    spc.a = spc_nz(spc_rd(spc_dp(0x02)));
L1F3D:
    /* 1F3D cmp a,#$00 */
    spc_cmp(spc.a, 0x00);
    /* 1F3F bne $1F42 */
    if (!spc.z) goto L1F42;
    /* 1F41 inc a */
    spc.a = spc_inc(spc.a);
L1F42:
    /* 1F42 mov y,#$03 */
    spc.y = spc_nz(0x03);
    /* 1F44 mul ya */
    spc_mul();
    /* 1F45 mov x,#$02 */
    spc.x = spc_nz(0x02);
    /* 1F47 call $22E5 */
    spc_call(abi_SpcBDiv16, 0x1F4A);
    /* 1F4A mov.b $24,a */
    spc_st(spc_dp(0x24), spc.a);
    /* 1F4C mov.b $25,y */
    spc_st(spc_dp(0x25), spc.y);
    /* 1F4E mov a,#$80 */
    spc.a = spc_nz(0x80);
    /* 1F50 mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 1F52 addw ya,$24 */
    { uint16_t v = spc_rd(spc_dp(0x24)); v |= spc_rd(spc_dp(0x25)) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 1F54 push y */
    spc_push(spc.y);
    /* 1F55 pop x */
    spc.x = spc_pull();
    /* 1F56 mov y,a */
    spc.y = spc_nz(spc.a);
    /* 1F57 mov.b a,$22 */
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 1F59 call $22D1 */
    spc_call(abi_SpcBMul16, 0x1F5C);
    /* 1F5C mov a,y */
    spc.a = spc_nz(spc.y);
    /* 1F5D bpl $1F61 */
    if (!spc.n) goto L1F61;
    /* 1F5F mov a,#$7F */
    spc.a = spc_nz(0x7F);
L1F61:
    /* 1F61 pop y */
    spc.y = spc_pull();
    /* 1F62 pop x */
    spc.x = spc_pull();
    /* 1F63 ret */
    spc_ret();
    return;
L1F64:
    /* 1F64 mov.b a,$22 */
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 1F66 bra $1F61 */
    goto L1F61;
}

/*
 * @routine 1B:806C SpcBSfxStart
 * WHAT IT DOES: Starts sound A (its data at $24/$25) on its channel unless that channel is in mode 2 or 3: channel, mode bits and side, data pointer, active, then the start state (SpcBSfxChannelStep's code at $1C04). As the race driver's SpcSfxStart, with the side. After it, more of SpcBSfxRead's command handlers (the end, volume slide, detune, slide off, a jump in the data, lifetime, key-off, engine mode, noise), the slides (from $213F: the pitch slide, clamped to $3FFF), and from $21C8 the DSP writes of channel X's voice (volume left and right, by the fade and SpcBSfxVolumeAdjust, pitch, sample, ADSR), then the 12-entry pitch table at $222B. The bytes $217B-$21C7 are code no path reaches.
 * HOW: SpcBSfxChannel, SpcBSfxMode, SpcBSfxSide, SpcBSfxActivate; the DSP writes through SpcDspWrite with Y = voice * $10.
 * Entry: A = sound, $24/$25 = data. Exit: as SpcBSfxChannelStep.
 * @spc 2087
 * @spcuses 1AFD-1B03,1B17-1B7E,1C04-1C1A,1CCC-1CE5,1D50-1D5A,2087-20B6
 */
void abi_SpcBSfxStart(void)
{
    /* 2087 mov.b $23,a */
    spc_st(spc_dp(0x23), spc.a);
    /* 2089 push a */
    spc_push(spc.a);
    /* 208A mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 208C mov a,($24)+y */
    { uint16_t p = spc_rd(spc_dp(0x24)); p |= spc_rd(spc_dp(0x25)) << 8;
      spc.a = spc_nz(spc_rd((uint16_t)(p + spc.y))); }
    /* 208E mov x,a */
    spc.x = spc_nz(spc.a);
    /* 208F mov.w a,$04F0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04F0 + spc.x)));
    /* 2092 cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    /* 2094 beq $20B4 */
    if (spc.z) goto L20B4;
    /* 2096 cmp a,#$03 */
    spc_cmp(spc.a, 0x03);
    /* 2098 beq $20B4 */
    if (spc.z) goto L20B4;
    /* 209A call $1B17 */
    spc_call(abi_SpcBSfxChannel, 0x209D);
    /* 209D call $1B48 */
    spc_call(abi_SpcBSfxMode, 0x20A0);
    /* 20A0 call $1B71 */
    spc_call(abi_SpcBSfxSide, 0x20A3);
    /* 20A3 mov.b a,$24 */
    spc.a = spc_nz(spc_rd(spc_dp(0x24)));
    /* 20A5 mov.w $0488+x,a */
    spc_st((uint16_t)(0x0488 + spc.x), spc.a);
    /* 20A8 mov.b a,$25 */
    spc.a = spc_nz(spc_rd(spc_dp(0x25)));
    /* 20AA mov.w $0490+x,a */
    spc_st((uint16_t)(0x0490 + spc.x), spc.a);
    /* 20AD call $1AFD */
    spc_call(abi_SpcBSfxActivate, 0x20B0);
    /* 20B0 pop a */
    spc.a = spc_pull();
    /* 20B1 jmp $1C04 */
    goto L1C04;
L20B4:
    /* 20B4 pop a */
    spc.a = spc_pull();
    /* 20B5 ret */
    spc_ret();
    return;
L1C04:
    /* 1C04 mov.w $0390+x,a */
    spc_st((uint16_t)(0x0390 + spc.x), spc.a);
    /* 1C07 mov a,#$01 */
    spc.a = spc_nz(0x01);
    /* 1C09 mov.w $03F8+x,a */
    spc_st((uint16_t)(0x03F8 + spc.x), spc.a);
    /* 1C0C mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 1C0E mov.w $03F0+x,a */
    spc_st((uint16_t)(0x03F0 + spc.x), spc.a);
    /* 1C11 mov.w $03E0+x,a */
    spc_st((uint16_t)(0x03E0 + spc.x), spc.a);
    /* 1C14 call $1CCC */
    spc_call(abi_SpcBSfxMarkVoice, 0x1C17);
    /* 1C17 jmp $1D50 */
    { abi_SpcBKeyOffVoice(); return; }
}

/*
 * @routine 1B:818A SpcBSfxVolumeSlide
 * WHAT IT DOES: One side of channel X's volume slide, as the race driver's SpcSfxVolumeSlide: the delta $20/$21 added to the volume $03A8/$04A8, clamped, and the volume byte to $0398. Only SpcBSfxReadVolumeSlides, which nothing reaches, calls it.
 * HOW: ADDW; sign tests.
 * Entry: X = channel (plus 8 for the right side). Exit: A = the volume.
 * @spc 21A5
 * @spcuses 21A5-21C8
 */
void abi_SpcBSfxVolumeSlide(void)
{
    /* 21A5 mov.w a,$04A8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04A8 + spc.x)));
    /* 21A8 mov y,a */
    spc.y = spc_nz(spc.a);
    /* 21A9 mov.w a,$03A8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03A8 + spc.x)));
    /* 21AC addw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 21AE mov.w $03A8+x,a */
    spc_st((uint16_t)(0x03A8 + spc.x), spc.a);
    /* 21B1 mov a,y */
    spc.a = spc_nz(spc.y);
    /* 21B2 mov.w $04A8+x,a */
    spc_st((uint16_t)(0x04A8 + spc.x), spc.a);
    /* 21B5 bmi $21B9 */
    if (spc.n) goto L21B9;
    /* 21B7 bra $21C4 */
    goto L21C4;
L21B9:
    /* 21B9 setc */
    spc.c = 1;
    /* 21BA sbc a,#$C0 */
    spc.a = spc_sbc(spc.a, 0xC0);
    /* 21BC bmi $21C2 */
    if (spc.n) goto L21C2;
    /* 21BE mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 21C0 bra $21C4 */
    goto L21C4;
L21C2:
    /* 21C2 mov a,#$7F */
    spc.a = spc_nz(0x7F);
L21C4:
    /* 21C4 mov.w $0398+x,a */
    spc_st((uint16_t)(0x0398 + spc.x), spc.a);
    /* 21C7 ret */
    spc_ret();
    return;
}

/*
 * @routine 1B:8228 SpcBSfxFadeVolume
 * WHAT IT DOES: The volume of channel X while it fades out (count A, $04C0), as the race driver's SpcSfxFadeVolume: the start volume and per-step drop taken at count 1, the volume interpolated down over $3C steps; at the end the channel stops (both sides of a right-side index) or the driver reloads or leaves, by flags.
 * HOW: SpcBMul16, SpcBDiv16 by $3C; SUBW.
 * Entry: A = count, X = channel. Exit: A = volume.
 * @spc 2243
 * @spcuses 1861-1871,19F5-1A02,1A21-1A2F,2243-22F2
 */
void abi_SpcBSfxFadeVolume(void)
{
    /* 2243 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    /* 2245 beq $2255 */
    if (spc.z) goto L2255;
    /* 2247 cmp a,#$3C */
    spc_cmp(spc.a, 0x3C);
    /* 2249 beq $2292 */
    if (spc.z) goto L2292;
    /* 224B cmp a,#$FF */
    spc_cmp(spc.a, 0xFF);
    /* 224D beq $22BA */
    if (spc.z) goto L22BA;
    /* 224F inc a */
    spc.a = spc_inc(spc.a);
    /* 2250 mov.w $04C0+x,a */
    spc_st((uint16_t)(0x04C0 + spc.x), spc.a);
    /* 2253 bra $226E */
    goto L226E;
L2255:
    /* 2255 inc a */
    spc.a = spc_inc(spc.a);
    /* 2256 mov.w $04C0+x,a */
    spc_st((uint16_t)(0x04C0 + spc.x), spc.a);
    /* 2259 mov.w a,$0398+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0398 + spc.x)));
    /* 225C mov y,a */
    spc.y = spc_nz(spc.a);
    /* 225D mov.w a,$03A8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03A8 + spc.x)));
    /* 2260 push x */
    spc_push(spc.x);
    /* 2261 mov x,#$3C */
    spc.x = spc_nz(0x3C);
    /* 2263 call $22E5 */
    spc_call(abi_SpcBDiv16, 0x2266);
    /* 2266 pop x */
    spc.x = spc_pull();
    /* 2267 mov.w $04D0+x,a */
    spc_st((uint16_t)(0x04D0 + spc.x), spc.a);
    /* 226A mov a,y */
    spc.a = spc_nz(spc.y);
    /* 226B mov.w $04E0+x,a */
    spc_st((uint16_t)(0x04E0 + spc.x), spc.a);
L226E:
    /* 226E mov.w a,$04C0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04C0 + spc.x)));
    /* 2271 dec a */
    spc.a = spc_dec(spc.a);
    /* 2272 mov.b $22,a */
    spc_st(spc_dp(0x22), spc.a);
    /* 2274 mov.w a,$04D0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04D0 + spc.x)));
    /* 2277 mov y,a */
    spc.y = spc_nz(spc.a);
    /* 2278 mov.w a,$04E0+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x04E0 + spc.x)));
    /* 227B push x */
    spc_push(spc.x);
    /* 227C mov x,a */
    spc.x = spc_nz(spc.a);
    /* 227D mov.b a,$22 */
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 227F call $22D1 */
    spc_call(abi_SpcBMul16, 0x2282);
    /* 2282 pop x */
    spc.x = spc_pull();
    /* 2283 mov.b $22,a */
    spc_st(spc_dp(0x22), spc.a);
    /* 2285 mov.b $23,y */
    spc_st(spc_dp(0x23), spc.y);
    /* 2287 mov.w a,$0398+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0398 + spc.x)));
    /* 228A mov y,a */
    spc.y = spc_nz(spc.a);
    /* 228B mov.w a,$03A8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03A8 + spc.x)));
    /* 228E subw ya,$22 */
    { uint16_t v = spc_rd(spc_dp(0x22)); v |= spc_rd(spc_dp(0x23)) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 2290 mov a,y */
    spc.a = spc_nz(spc.y);
    /* 2291 ret */
    spc_ret();
    return;
L2292:
    /* 2292 bbs $2C.7,$229A */
    if ((spc_rd(spc_dp(0x2C)) >> 7 & 1)) goto L229A;
    /* 2295 bbs $2D.0,$229D */
    if ((spc_rd(spc_dp(0x2D)) >> 0 & 1)) goto L229D;
    /* 2298 bra $22A0 */
    goto L22A0;
L229A:
    /* 229A jmp $19F5 */
    goto L19F5;
L229D:
    /* 229D jmp $1A21 */
    goto L1A21;
L22A0:
    /* 22A0 mov.b $22,x */
    spc_st(spc_dp(0x22), spc.x);
    /* 22A2 bbs $22.3,$22AF */
    if ((spc_rd(spc_dp(0x22)) >> 3 & 1)) goto L22AF;
    /* 22A5 call $186C */
    spc_call(abi_SpcBSfxStop, 0x22A8);
L22A8:
    /* 22A8 mov a,#$FF */
    spc.a = spc_nz(0xFF);
    /* 22AA mov.w $04C0+x,a */
    spc_st((uint16_t)(0x04C0 + spc.x), spc.a);
    /* 22AD inc a */
    spc.a = spc_inc(spc.a);
    /* 22AE ret */
    spc_ret();
    return;
L22AF:
    /* 22AF call $22C8 */
    spc_call(abi_SpcBIndexLeft, 0x22B2);
    /* 22B2 call $186C */
    spc_call(abi_SpcBSfxStop, 0x22B5);
    /* 22B5 call $22BF */
    spc_call(abi_SpcBIndexRight, 0x22B8);
    /* 22B8 bra $22A8 */
    goto L22A8;
L22BA:
    /* 22BA inc a */
    spc.a = spc_inc(spc.a);
    /* 22BB mov.w $04C0+x,a */
    spc_st((uint16_t)(0x04C0 + spc.x), spc.a);
    /* 22BE ret */
    spc_ret();
    return;
L19F5:
    /* 19F5 call $19FB */
    spc_call(abi_SpcBKeyOffAll, 0x19F8);
    /* 19F8 jmp $0675 */
    { spc_restart(0x0675, abi_SpcReload); return; }
L1A21:
    /* 1A21 call $19FB */
    spc_call(abi_SpcBKeyOffAll, 0x1A24);
    /* 1A24 mov.w a,$00F1 */
    spc.a = spc_nz(spc_rd(0x00F1));
    /* 1A27 or a,#$80 */
    spc.a = spc_or(spc.a, 0x80);
    /* 1A29 mov.w $00F1,a */
    spc_st(0x00F1, spc.a);
    /* 1A2C jmp $FFC0 */
    { spc_boot_rom(); return; }
}

/*
 * @routine 1B:82A4 SpcBIndexRight
 * WHAT IT DOES: X + 8 (a channel's right-side index). The race driver's SpcIndexRight.
 * HOW: Eight INC X.
 * Entry: X. Exit: X + 8.
 * @spc 22BF
 * @spcuses 22BF-22C8
 */
void abi_SpcBIndexRight(void)
{
    /* 22BF inc x */
    spc.x = spc_inc(spc.x);
    /* 22C0 inc x */
    spc.x = spc_inc(spc.x);
    /* 22C1 inc x */
    spc.x = spc_inc(spc.x);
    /* 22C2 inc x */
    spc.x = spc_inc(spc.x);
    /* 22C3 inc x */
    spc.x = spc_inc(spc.x);
    /* 22C4 inc x */
    spc.x = spc_inc(spc.x);
    /* 22C5 inc x */
    spc.x = spc_inc(spc.x);
    /* 22C6 inc x */
    spc.x = spc_inc(spc.x);
    /* 22C7 ret */
    spc_ret();
    return;
}

/*
 * @routine 1B:82AD SpcBIndexLeft
 * WHAT IT DOES: X - 8. The race driver's SpcIndexLeft.
 * HOW: Eight DEC X.
 * Entry: X. Exit: X - 8.
 * @spc 22C8
 * @spcuses 22C8-22D1
 */
void abi_SpcBIndexLeft(void)
{
    /* 22C8 dec x */
    spc.x = spc_dec(spc.x);
    /* 22C9 dec x */
    spc.x = spc_dec(spc.x);
    /* 22CA dec x */
    spc.x = spc_dec(spc.x);
    /* 22CB dec x */
    spc.x = spc_dec(spc.x);
    /* 22CC dec x */
    spc.x = spc_dec(spc.x);
    /* 22CD dec x */
    spc.x = spc_dec(spc.x);
    /* 22CE dec x */
    spc.x = spc_dec(spc.x);
    /* 22CF dec x */
    spc.x = spc_dec(spc.x);
    /* 22D0 ret */
    spc_ret();
    return;
}

/*
 * @routine 1B:82B6 SpcBMul16
 * WHAT IT DOES: A times the 16-bit value X:Y (X the high byte): YA = the result's middle 16 bits plus A * Y, as the sound code's fixed-point multiply. The race driver's SpcMul16.
 * HOW: Two MULs and ADDW through $26/$27.
 * Entry: A, X, Y. Exit: YA, X.
 * @spc 22D1
 * @spcuses 22D1-22E5
 */
void abi_SpcBMul16(void)
{
    /* 22D1 push a */
    spc_push(spc.a);
    /* 22D2 mul ya */
    spc_mul();
    /* 22D3 mov.b $26,a */
    spc_st(spc_dp(0x26), spc.a);
    /* 22D5 mov.b $27,y */
    spc_st(spc_dp(0x27), spc.y);
    /* 22D7 mov a,x */
    spc.a = spc_nz(spc.x);
    /* 22D8 mov y,a */
    spc.y = spc_nz(spc.a);
    /* 22D9 pop a */
    spc.a = spc_pull();
    /* 22DA mul ya */
    spc_mul();
    /* 22DB push a */
    spc_push(spc.a);
    /* 22DC mov a,y */
    spc.a = spc_nz(spc.y);
    /* 22DD mov x,a */
    spc.x = spc_nz(spc.a);
    /* 22DE pop a */
    spc.a = spc_pull();
    /* 22DF mov y,a */
    spc.y = spc_nz(spc.a);
    /* 22E0 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 22E2 addw ya,$26 */
    { uint16_t v = spc_rd(spc_dp(0x26)); v |= spc_rd(spc_dp(0x27)) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 22E4 ret */
    spc_ret();
    return;
}

/*
 * @routine 1B:82CA SpcBDiv16
 * WHAT IT DOES: The 16-bit value Y:A divided by X: YA = the quotient. The race driver's SpcDiv16.
 * HOW: Two DIVs, the high quotient through $27.
 * Entry: Y:A, X. Exit: YA = quotient.
 * @spc 22E5
 * @spcuses 22E5-22F2
 */
void abi_SpcBDiv16(void)
{
    /* 22E5 push a */
    spc_push(spc.a);
    /* 22E6 push y */
    spc_push(spc.y);
    /* 22E7 mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 22E9 pop a */
    spc.a = spc_pull();
    /* 22EA div ya,x */
    spc_div();
    /* 22EB mov.b $27,a */
    spc_st(spc_dp(0x27), spc.a);
    /* 22ED pop a */
    spc.a = spc_pull();
    /* 22EE div ya,x */
    spc_div();
    /* 22EF mov.b y,$27 */
    spc.y = spc_nz(spc_rd(spc_dp(0x27)));
    /* 22F1 ret */
    spc_ret();
    return;
}

/*
 * @routine 1B:82D7 SpcBSfxVolumeAdjust
 * WHAT IT DOES: For sound $25, adds $0530 * 4 (channel 5) or $0531 * 4 (channel 4) to a nonzero volume A; the levels come from port 1 commands $32-$39. The race driver's SpcSfxVolumeAdjust has $0530 only.
 * HOW: MUL, ADC.
 * Entry: A = volume, X = channel. Exit: A = volume; Y kept.
 * @spc 22F2
 * @spcuses 22F2-231E
 */
void abi_SpcBSfxVolumeAdjust(void)
{
    /* 22F2 push y */
    spc_push(spc.y);
    /* 22F3 mov.b $22,a */
    spc_st(spc_dp(0x22), spc.a);
    /* 22F5 cmp a,#$00 */
    spc_cmp(spc.a, 0x00);
    /* 22F7 beq $2318 */
    if (spc.z) goto L2318;
    /* 22F9 mov.w a,$0390+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 22FC cmp a,#$25 */
    spc_cmp(spc.a, 0x25);
    /* 22FE bne $231A */
    if (!spc.z) goto L231A;
    /* 2300 cmp x,#$05 */
    spc_cmp(spc.x, 0x05);
    /* 2302 beq $230A */
    if (spc.z) goto L230A;
    /* 2304 cmp x,#$04 */
    spc_cmp(spc.x, 0x04);
    /* 2306 beq $230F */
    if (spc.z) goto L230F;
    /* 2308 bra $2318 */
    goto L2318;
L230A:
    /* 230A mov.w a,$0530 */
    spc.a = spc_nz(spc_rd(0x0530));
    /* 230D bra $2312 */
    goto L2312;
L230F:
    /* 230F mov.w a,$0531 */
    spc.a = spc_nz(spc_rd(0x0531));
L2312:
    /* 2312 mov y,#$04 */
    spc.y = spc_nz(0x04);
    /* 2314 mul ya */
    spc_mul();
    /* 2315 clrc */
    spc.c = 0;
    /* 2316 adc.b a,$22 */
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0x22)));
L2318:
    /* 2318 pop y */
    spc.y = spc_pull();
    /* 2319 ret */
    spc_ret();
    return;
L231A:
    /* 231A mov.b a,$22 */
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 231C bra $2318 */
    goto L2318;
}

/*
 * @routine 1B:8303 SpcBSfxSpecial
 * WHAT IT DOES: When channel X plays a sound in the list at $233A ($77-$7A, $17, $25), runs its handler from the 6-entry table at $2341: for $77-$7A the engine pitch (channel 7 from port 3, channel 6 from port 2, with the sound's curve from the tables at $234D-$235C): a change of speed starts a slew ($0532-$053C) toward the new pitch, full speed ($F0 and up) drifts it up ($056C/$056E), and the pitch ramps' offsets for that side are added, clamped to $3FFF, as the DSP pitch $03B8/$03C0; the volume follows (SpcBEngineVolume). $17 and $25 are the rev timing (in SpcBEngineDamp). As the race driver's SpcSfxSpecial with a channel per side.
 * HOW: JMP ($2341+X) with X pushed; Y = 1 for port 3, 0 for port 2.
 * Entry: X = channel. Exit: as the handler leaves it.
 * @spc 231E
 * @spcuses 231E-233A,2341-234D,235D-2588
 */
void abi_SpcBSfxSpecial(void)
{
    /* 231E mov.w a,$0390+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0390 + spc.x)));
    /* 2321 mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 2323 push x */
    spc_push(spc.x);
    /* 2324 mov x,#$00 */
    spc.x = spc_nz(0x00);
L2326:
    /* 2326 mov.w a,$233A+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x233A + spc.x)));
    /* 2329 beq $2332 */
    if (spc.z) goto L2332;
    /* 232B cmp.b a,$20 */
    spc_cmp(spc.a, spc_rd(spc_dp(0x20)));
    /* 232D beq $2334 */
    if (spc.z) goto L2334;
    /* 232F inc x */
    spc.x = spc_inc(spc.x);
    /* 2330 bra $2326 */
    goto L2326;
L2332:
    /* 2332 pop x */
    spc.x = spc_pull();
    /* 2333 ret */
    spc_ret();
    return;
L2334:
    /* 2334 mov a,x */
    spc.a = spc_nz(spc.x);
    /* 2335 asl a */
    spc.a = spc_asl(spc.a);
    /* 2336 mov x,a */
    spc.x = spc_nz(spc.a);
    /* 2337 jmp ($2341+x) */
    { uint16_t t = spc_rd((uint16_t)(0x2341 + spc.x)); t |= spc_rd((uint16_t)(0x2341 + spc.x + 1)) << 8;
      switch (t) {
      case 0x235D: goto L235D;
      case 0x253A: goto L253A;
      case 0x2545: goto L2545;
      } }
    return;
L235D:
    /* 235D lsr a */
    spc.a = spc_lsr(spc.a);
    /* 235E mov x,a */
    spc.x = spc_nz(spc.a);
    /* 235F mov.w a,$234D+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x234D + spc.x)));
    /* 2362 mov.b $22,a */
    spc_st(spc_dp(0x22), spc.a);
    /* 2364 mov.w a,$2351+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x2351 + spc.x)));
    /* 2367 mov.b $23,a */
    spc_st(spc_dp(0x23), spc.a);
    /* 2369 mov.w a,$2355+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x2355 + spc.x)));
    /* 236C mov.b $24,a */
    spc_st(spc_dp(0x24), spc.a);
    /* 236E mov.w a,$2359+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x2359 + spc.x)));
    /* 2371 mov.b $25,a */
    spc_st(spc_dp(0x25), spc.a);
    /* 2373 pop x */
    spc.x = spc_pull();
    /* 2374 cmp x,#$07 */
    spc_cmp(spc.x, 0x07);
    /* 2376 beq $237D */
    if (spc.z) goto L237D;
    /* 2378 cmp x,#$06 */
    spc_cmp(spc.x, 0x06);
    /* 237A beq $2381 */
    if (spc.z) goto L2381;
    /* 237C ret */
    spc_ret();
    return;
L237D:
    /* 237D mov y,#$01 */
    spc.y = spc_nz(0x01);
    /* 237F bra $2383 */
    goto L2383;
L2381:
    /* 2381 mov y,#$00 */
    spc.y = spc_nz(0x00);
L2383:
    /* 2383 call $24E2 */
    spc_call(abi_SpcBEngineVolume, 0x2386);
    /* 2386 push x */
    spc_push(spc.x);
    /* 2387 mov a,y */
    spc.a = spc_nz(spc.y);
    /* 2388 mov x,a */
    spc.x = spc_nz(spc.a);
    /* 2389 mov.w a,$0002+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0002 + spc.x)));
    /* 238C cmp.w a,$000A+x */
    spc_cmp(spc.a, spc_rd((uint16_t)(0x000A + spc.x)));
    /* 238F beq $23DF */
    if (spc.z) goto L23DF;
    /* 2391 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 2393 mov.w $0538+x,a */
    spc_st((uint16_t)(0x0538 + spc.x), spc.a);
    /* 2396 mov.w a,$0002+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0002 + spc.x)));
    /* 2399 mov.w $053A+x,a */
    spc_st((uint16_t)(0x053A + spc.x), spc.a);
    /* 239C setc */
    spc.c = 1;
    /* 239D sbc.w a,$000A+x */
    spc.a = spc_sbc(spc.a, spc_rd((uint16_t)(0x000A + spc.x)));
    /* 23A0 bmi $23D4 */
    if (spc.n) goto L23D4;
    /* 23A2 push a */
    spc_push(spc.a);
    /* 23A3 mov a,#$01 */
    spc.a = spc_nz(0x01);
    /* 23A5 mov.w $053C+x,a */
    spc_st((uint16_t)(0x053C + spc.x), spc.a);
L23A8:
    /* 23A8 pop a */
    spc.a = spc_pull();
    /* 23A9 mov y,a */
    spc.y = spc_nz(spc.a);
    /* 23AA mov.w a,$0532+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0532 + spc.x)));
    /* 23AD push x */
    spc_push(spc.x);
    /* 23AE mov x,a */
    spc.x = spc_nz(spc.a);
    /* 23AF bne $23B3 */
    if (!spc.z) goto L23B3;
    /* 23B1 mov x,#$01 */
    spc.x = spc_nz(0x01);
L23B3:
    /* 23B3 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 23B5 div ya,x */
    spc_div();
    /* 23B6 bvs $23C3 */
    if (spc.v) goto L23C3;
    /* 23B8 pop x */
    spc.x = spc_pull();
    /* 23B9 mov.w $0534+x,a */
    spc_st((uint16_t)(0x0534 + spc.x), spc.a);
    /* 23BC mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 23BE mov.w $0536+x,a */
    spc_st((uint16_t)(0x0536 + spc.x), spc.a);
    /* 23C1 bra $23CC */
    goto L23CC;
L23C3:
    /* 23C3 pop x */
    spc.x = spc_pull();
    /* 23C4 mov.w $0534+x,a */
    spc_st((uint16_t)(0x0534 + spc.x), spc.a);
    /* 23C7 mov a,#$01 */
    spc.a = spc_nz(0x01);
    /* 23C9 mov.w $0536+x,a */
    spc_st((uint16_t)(0x0536 + spc.x), spc.a);
L23CC:
    /* 23CC mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 23CE mov.w $0532+x,a */
    spc_st((uint16_t)(0x0532 + spc.x), spc.a);
    /* 23D1 jmp $248B */
    goto L248B;
L23D4:
    /* 23D4 eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    /* 23D6 inc a */
    spc.a = spc_inc(spc.a);
    /* 23D7 push a */
    spc_push(spc.a);
    /* 23D8 mov a,#$02 */
    spc.a = spc_nz(0x02);
    /* 23DA mov.w $053C+x,a */
    spc_st((uint16_t)(0x053C + spc.x), spc.a);
    /* 23DD bra $23A8 */
    goto L23A8;
L23DF:
    /* 23DF mov.w a,$0002+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0002 + spc.x)));
    /* 23E2 bne $23EE */
    if (!spc.z) goto L23EE;
    /* 23E4 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 23E6 mov.w $0534+x,a */
    spc_st((uint16_t)(0x0534 + spc.x), spc.a);
    /* 23E9 mov.w $0536+x,a */
    spc_st((uint16_t)(0x0536 + spc.x), spc.a);
    /* 23EC bra $2414 */
    goto L2414;
L23EE:
    /* 23EE mov.w a,$0002+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0002 + spc.x)));
    /* 23F1 bpl $2414 */
    if (!spc.n) goto L2414;
    /* 23F3 setc */
    spc.c = 1;
    /* 23F4 sbc a,#$F0 */
    spc.a = spc_sbc(spc.a, 0xF0);
    /* 23F6 bmi $2414 */
    if (spc.n) goto L2414;
    /* 23F8 mov.w a,$056C+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x056C + spc.x)));
    /* 23FB mov.b $22,a */
    spc_st(spc_dp(0x22), spc.a);
    /* 23FD mov.w a,$056E+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x056E + spc.x)));
    /* 2400 mov.b $23,a */
    spc_st(spc_dp(0x23), spc.a);
    /* 2402 incw $22 */
    spc_incw(0x22, +1);
    /* 2404 incw $22 */
    spc_incw(0x22, +1);
    /* 2406 incw $22 */
    spc_incw(0x22, +1);
    /* 2408 mov.b a,$22 */
    spc.a = spc_nz(spc_rd(spc_dp(0x22)));
    /* 240A mov.w $056C+x,a */
    spc_st((uint16_t)(0x056C + spc.x), spc.a);
    /* 240D mov.b a,$23 */
    spc.a = spc_nz(spc_rd(spc_dp(0x23)));
    /* 240F mov.w $056E+x,a */
    spc_st((uint16_t)(0x056E + spc.x), spc.a);
    /* 2412 bra $241C */
    goto L241C;
L2414:
    /* 2414 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 2416 mov.w $056C+x,a */
    spc_st((uint16_t)(0x056C + spc.x), spc.a);
    /* 2419 mov.w $056E+x,a */
    spc_st((uint16_t)(0x056E + spc.x), spc.a);
L241C:
    /* 241C mov.w a,$0532+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0532 + spc.x)));
    /* 241F setc */
    spc.c = 1;
    /* 2420 sbc a,#$0A */
    spc.a = spc_sbc(spc.a, 0x0A);
    /* 2422 bmi $2440 */
    if (spc.n) goto L2440;
    /* 2424 mov $22,#$01 */
    spc_st(spc_dp(0x22), 0x01);
    /* 2427 mov $23,#$00 */
    spc_st(spc_dp(0x23), 0x00);
    /* 242A mov.w a,$0536+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0536 + spc.x)));
    /* 242D mov y,a */
    spc.y = spc_nz(spc.a);
    /* 242E mov.w a,$0534+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0534 + spc.x)));
    /* 2431 subw ya,$22 */
    { uint16_t v = spc_rd(spc_dp(0x22)); v |= spc_rd(spc_dp(0x23)) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 2433 bpl $2439 */
    if (!spc.n) goto L2439;
    /* 2435 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 2437 mov y,#$00 */
    spc.y = spc_nz(0x00);
L2439:
    /* 2439 mov.w $0534+x,a */
    spc_st((uint16_t)(0x0534 + spc.x), spc.a);
    /* 243C mov a,y */
    spc.a = spc_nz(spc.y);
    /* 243D mov.w $0536+x,a */
    spc_st((uint16_t)(0x0536 + spc.x), spc.a);
L2440:
    /* 2440 mov.w a,$0532+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0532 + spc.x)));
    /* 2443 cmp a,#$FF */
    spc_cmp(spc.a, 0xFF);
    /* 2445 beq $244E */
    if (spc.z) goto L244E;
    /* 2447 mov.w a,$0532+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0532 + spc.x)));
    /* 244A inc a */
    spc.a = spc_inc(spc.a);
    /* 244B mov.w $0532+x,a */
    spc_st((uint16_t)(0x0532 + spc.x), spc.a);
L244E:
    /* 244E mov.w a,$0532+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0532 + spc.x)));
    /* 2451 cmp a,#$01 */
    spc_cmp(spc.a, 0x01);
    /* 2453 beq $2483 */
    if (spc.z) goto L2483;
    /* 2455 mov.w a,$053A+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x053A + spc.x)));
    /* 2458 mov y,a */
    spc.y = spc_nz(spc.a);
    /* 2459 mov.w a,$0538+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0538 + spc.x)));
L245C:
    /* 245C push a */
    spc_push(spc.a);
    /* 245D mov.w a,$0534+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0534 + spc.x)));
    /* 2460 mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 2462 mov.w a,$0536+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0536 + spc.x)));
    /* 2465 mov.b $21,a */
    spc_st(spc_dp(0x21), spc.a);
    /* 2467 mov.w a,$053C+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x053C + spc.x)));
    /* 246A cmp a,#$02 */
    spc_cmp(spc.a, 0x02);
    /* 246C beq $247C */
    if (spc.z) goto L247C;
    /* 246E pop a */
    spc.a = spc_pull();
    /* 246F addw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 2471 bcs $248B */
    if (spc.c) goto L248B;
L2473:
    /* 2473 mov.w $0538+x,a */
    spc_st((uint16_t)(0x0538 + spc.x), spc.a);
    /* 2476 mov a,y */
    spc.a = spc_nz(spc.y);
    /* 2477 mov.w $053A+x,a */
    spc_st((uint16_t)(0x053A + spc.x), spc.a);
    /* 247A bra $248B */
    goto L248B;
L247C:
    /* 247C pop a */
    spc.a = spc_pull();
    /* 247D subw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_subw(spc_ya(), v)); }
    /* 247F bcc $248B */
    if (!spc.c) goto L248B;
    /* 2481 bra $2473 */
    goto L2473;
L2483:
    /* 2483 mov.w a,$0002+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0002 + spc.x)));
    /* 2486 mov y,a */
    spc.y = spc_nz(spc.a);
    /* 2487 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 2489 bra $245C */
    goto L245C;
L248B:
    /* 248B mov $20,#$00 */
    spc_st(spc_dp(0x20), 0x00);
    /* 248E mov $21,$24 */
    spc_wr(spc_dp(0x21), spc_rd(spc_dp(0x24)));
    /* 2491 mov.b y,$25 */
    spc.y = spc_nz(spc_rd(spc_dp(0x25)));
    /* 2493 mov.w a,$053A+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x053A + spc.x)));
    /* 2496 mul ya */
    spc_mul();
    /* 2497 mov $20,#$00 */
    spc_st(spc_dp(0x20), 0x00);
    /* 249A addw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 249C mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 249E mov.b $21,y */
    spc_st(spc_dp(0x21), spc.y);
    /* 24A0 mov.w a,$054A+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x054A + spc.x)));
    /* 24A3 mov y,a */
    spc.y = spc_nz(spc.a);
    /* 24A4 mov.w a,$0548+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0548 + spc.x)));
    /* 24A7 addw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 24A9 mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 24AB mov.b $21,y */
    spc_st(spc_dp(0x21), spc.y);
    /* 24AD mov.w a,$054E+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x054E + spc.x)));
    /* 24B0 mov y,a */
    spc.y = spc_nz(spc.a);
    /* 24B1 mov.w a,$054C+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x054C + spc.x)));
    /* 24B4 addw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 24B6 mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 24B8 mov.b $21,y */
    spc_st(spc_dp(0x21), spc.y);
    /* 24BA mov.w a,$056E+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x056E + spc.x)));
    /* 24BD mov y,a */
    spc.y = spc_nz(spc.a);
    /* 24BE mov.w a,$056C+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x056C + spc.x)));
    /* 24C1 addw ya,$20 */
    { uint16_t v = spc_rd(spc_dp(0x20)); v |= spc_rd(spc_dp(0x21)) << 8; spc_set_ya(spc_addw(spc_ya(), v)); }
    /* 24C3 mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 24C5 mov.b $21,y */
    spc_st(spc_dp(0x21), spc.y);
    /* 24C7 setc */
    spc.c = 1;
    /* 24C8 sbc $21,#$3F */
    spc_wr(spc_dp(0x21), spc_sbc(spc_rd(spc_dp(0x21)), 0x3F));
    /* 24CB bmi $24D0 */
    if (spc.n) goto L24D0;
    /* 24CD mov $21,#$3F */
    spc_st(spc_dp(0x21), 0x3F);
L24D0:
    /* 24D0 mov.w a,$0002+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0002 + spc.x)));
    /* 24D3 mov.w $000A+x,a */
    spc_st((uint16_t)(0x000A + spc.x), spc.a);
    /* 24D6 pop x */
    spc.x = spc_pull();
    /* 24D7 mov.b a,$20 */
    spc.a = spc_nz(spc_rd(spc_dp(0x20)));
    /* 24D9 mov.w $03B8+x,a */
    spc_st((uint16_t)(0x03B8 + spc.x), spc.a);
    /* 24DC mov.b a,$21 */
    spc.a = spc_nz(spc_rd(spc_dp(0x21)));
    /* 24DE mov.w $03C0+x,a */
    spc_st((uint16_t)(0x03C0 + spc.x), spc.a);
    /* 24E1 ret */
    spc_ret();
    return;
L253A:
    /* 253A mov $22,#$1E */
    spc_st(spc_dp(0x22), 0x1E);
    /* 253D mov $23,#$03 */
    spc_st(spc_dp(0x23), 0x03);
    /* 2540 mov $24,#$01 */
    spc_st(spc_dp(0x24), 0x01);
    /* 2543 bra $254E */
    goto L254E;
L2545:
    /* 2545 mov $22,#$14 */
    spc_st(spc_dp(0x22), 0x14);
    /* 2548 mov $23,#$0A */
    spc_st(spc_dp(0x23), 0x0A);
    /* 254B mov $24,#$02 */
    spc_st(spc_dp(0x24), 0x02);
L254E:
    /* 254E pop x */
    spc.x = spc_pull();
    /* 254F cmp x,#$05 */
    spc_cmp(spc.x, 0x05);
    /* 2551 beq $2558 */
    if (spc.z) goto L2558;
    /* 2553 cmp x,#$04 */
    spc_cmp(spc.x, 0x04);
    /* 2555 beq $255C */
    if (spc.z) goto L255C;
    /* 2557 ret */
    spc_ret();
    return;
L2558:
    /* 2558 mov y,#$01 */
    spc.y = spc_nz(0x01);
    /* 255A bra $255E */
    goto L255E;
L255C:
    /* 255C mov y,#$00 */
    spc.y = spc_nz(0x00);
L255E:
    /* 255E mov.w a,$0002+y */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0002 + spc.y)));
    /* 2561 eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    /* 2563 push y */
    spc_push(spc.y);
    /* 2564 mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 2566 push x */
    spc_push(spc.x);
    /* 2567 mov.b x,$22 */
    spc.x = spc_nz(spc_rd(spc_dp(0x22)));
    /* 2569 div ya,x */
    spc_div();
    /* 256A pop x */
    spc.x = spc_pull();
    /* 256B pop y */
    spc.y = spc_pull();
    /* 256C clrc */
    spc.c = 0;
    /* 256D adc.b a,$23 */
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0x23)));
    /* 256F push a */
    spc_push(spc.a);
    /* 2570 mov.w a,$0544+y */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0544 + spc.y)));
    /* 2573 inc a */
    spc.a = spc_inc(spc.a);
    /* 2574 mov.w $0544+y,a */
    spc_st((uint16_t)(0x0544 + spc.y), spc.a);
    /* 2577 pop a */
    spc.a = spc_pull();
    /* 2578 setc */
    spc.c = 1;
    /* 2579 sbc.w a,$0544+y */
    spc.a = spc_sbc(spc.a, spc_rd((uint16_t)(0x0544 + spc.y)));
    /* 257C bpl $2587 */
    if (!spc.n) goto L2587;
    /* 257E mov.b a,$24 */
    spc.a = spc_nz(spc_rd(spc_dp(0x24)));
    /* 2580 mov.w $03E8+x,a */
    spc_st((uint16_t)(0x03E8 + spc.x), spc.a);
    /* 2583 dec a */
    spc.a = spc_dec(spc.a);
    /* 2584 mov.w $0544+y,a */
    spc_st((uint16_t)(0x0544 + spc.y), spc.a);
L2587:
    /* 2587 ret */
    spc_ret();
    return;
}

/*
 * @routine 1B:84C7 SpcBEngineVolume
 * WHAT IT DOES: The engine sound's volume from port 3 (Y = 1) or port 2 (Y = 0): (inverted speed) / $22 + $23, plus 3 with bit 2 (or 4) of $29, by $D0 unless bit 2 (or 4) of $28 (SpcBEngineDamp, SpcBEngineDamp2), by $056B; port 3's on the left only, port 2's on the right. The race driver's SpcEngineVolume sets both sides.
 * HOW: DIV and MULs keeping high bytes.
 * Entry: X = channel, Y = port - 2, $22/$23 = curve. Exit: A = volume.
 * @spc 24E2
 * @spcuses 24E2-253A
 */
void abi_SpcBEngineVolume(void)
{
    /* 24E2 mov.w a,$0002+y */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0002 + spc.y)));
    /* 24E5 eor a,#$FF */
    spc.a = spc_eor(spc.a, 0xFF);
    /* 24E7 push y */
    spc_push(spc.y);
    /* 24E8 mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 24EA push x */
    spc_push(spc.x);
    /* 24EB mov.b x,$22 */
    spc.x = spc_nz(spc_rd(spc_dp(0x22)));
    /* 24ED div ya,x */
    spc_div();
    /* 24EE mov.b $20,a */
    spc_st(spc_dp(0x20), spc.a);
    /* 24F0 pop x */
    spc.x = spc_pull();
    /* 24F1 pop y */
    spc.y = spc_pull();
    /* 24F2 mov.b a,$23 */
    spc.a = spc_nz(spc_rd(spc_dp(0x23)));
    /* 24F4 clrc */
    spc.c = 0;
    /* 24F5 adc.b a,$20 */
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0x20)));
    /* 24F7 cmp y,#$00 */
    spc_cmp(spc.y, 0x00);
    /* 24F9 beq $2513 */
    if (spc.z) goto L2513;
    /* 24FB bbc $29.2,$2500 */
    if (!(spc_rd(spc_dp(0x29)) >> 2 & 1)) goto L2500;
    /* 24FE adc a,#$03 */
    spc.a = spc_adc(spc.a, 0x03);
L2500:
    /* 2500 call $2530 */
    spc_call(abi_SpcBEngineDamp, 0x2503);
    /* 2503 push y */
    spc_push(spc.y);
    /* 2504 mov.w y,$056B */
    spc.y = spc_nz(spc_rd(0x056B));
    /* 2507 mul ya */
    spc_mul();
    /* 2508 mov a,y */
    spc.a = spc_nz(spc.y);
    /* 2509 pop y */
    spc.y = spc_pull();
    /* 250A mov.w $0398+x,a */
    spc_st((uint16_t)(0x0398 + spc.x), spc.a);
    /* 250D mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 250F mov.w $03A0+x,a */
    spc_st((uint16_t)(0x03A0 + spc.x), spc.a);
    /* 2512 ret */
    spc_ret();
    return;
L2513:
    /* 2513 bbc $29.4,$2518 */
    if (!(spc_rd(spc_dp(0x29)) >> 4 & 1)) goto L2518;
    /* 2516 adc a,#$03 */
    spc.a = spc_adc(spc.a, 0x03);
L2518:
    /* 2518 call $252B */
    spc_call(abi_SpcBEngineDamp2, 0x251B);
    /* 251B push y */
    spc_push(spc.y);
    /* 251C mov.w y,$056B */
    spc.y = spc_nz(spc_rd(0x056B));
    /* 251F mul ya */
    spc_mul();
    /* 2520 mov a,y */
    spc.a = spc_nz(spc.y);
    /* 2521 pop y */
    spc.y = spc_pull();
    /* 2522 mov.w $03A0+x,a */
    spc_st((uint16_t)(0x03A0 + spc.x), spc.a);
    /* 2525 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 2527 mov.w $0398+x,a */
    spc_st((uint16_t)(0x0398 + spc.x), spc.a);
    /* 252A ret */
    spc_ret();
    return;
}

/*
 * @routine 1B:8510 SpcBEngineDamp2
 * WHAT IT DOES: Unless bit 4 of $28 is set, A = A * $D0 / 256 (the port 2 side of SpcBEngineDamp).
 * HOW: Branches into SpcBEngineDamp's code.
 * Entry: A. Exit: A; Y kept.
 * @spc 252B
 * @spcuses 252B-2530,2533-253A
 */
void abi_SpcBEngineDamp2(void)
{
    /* 252B bbs $28.4,$2539 */
    if ((spc_rd(spc_dp(0x28)) >> 4 & 1)) goto L2539;
    /* 252E bra $2533 */
    goto L2533;
L2533:
    /* 2533 push y */
    spc_push(spc.y);
    /* 2534 mov y,#$D0 */
    spc.y = spc_nz(0xD0);
    /* 2536 mul ya */
    spc_mul();
    /* 2537 mov a,y */
    spc.a = spc_nz(spc.y);
    /* 2538 pop y */
    spc.y = spc_pull();
L2539:
    /* 2539 ret */
    spc_ret();
    return;
}

/*
 * @routine 1B:8515 SpcBEngineDamp
 * WHAT IT DOES: Unless bit 2 of $28 is set, A = A * $D0 / 256 (the race driver's SpcEngineDamp). After it, SpcBSfxSpecial's handlers for sounds $25 and $17, the rev timing: channel 5 by port 3, channel 4 by port 2, the counter $0544 counts up to (inverted speed) / $22 + $23 and restarts the sound's note ($03E8 = $24).
 * HOW: MUL; the handlers take their curve ($22-$24) and share the count.
 * Entry: A. Exit: A; Y kept.
 * @spc 2530
 * @spcuses 2530-253A
 */
void abi_SpcBEngineDamp(void)
{
    /* 2530 bbs $28.2,$2539 */
    if ((spc_rd(spc_dp(0x28)) >> 2 & 1)) goto L2539;
    /* 2533 push y */
    spc_push(spc.y);
    /* 2534 mov y,#$D0 */
    spc.y = spc_nz(0xD0);
    /* 2536 mul ya */
    spc_mul();
    /* 2537 mov a,y */
    spc.a = spc_nz(spc.y);
    /* 2538 pop y */
    spc.y = spc_pull();
L2539:
    /* 2539 ret */
    spc_ret();
    return;
}

/*
 * @routine 1B:856D SpcBNoiseSampleInit
 * WHAT IT DOES: Builds the sample at $FF00, as the race driver's SpcNoiseSampleInit: every ninth byte (the BRR block headers) set to A for $1B blocks, the last A + 1 (the end flag); the sample directory's entry at $3F8C-$3F8F points start and loop at $FF00; the LFSR made nonzero.
 * HOW: Y stepping by 9.
 * Entry: A = header byte. Exit: A, Y changed.
 * @spc 2588
 * @spcuses 2588-25B8
 */
void abi_SpcBNoiseSampleInit(void)
{
    /* 2588 mov y,#$00 */
    spc.y = spc_nz(0x00);
    /* 258A mov x,#$1B */
    spc.x = spc_nz(0x1B);
L258C:
    /* 258C mov.w $FF00+y,a */
    spc_st((uint16_t)(0xFF00 + spc.y), spc.a);
    /* 258F inc y */
    spc.y = spc_inc(spc.y);
    /* 2590 inc y */
    spc.y = spc_inc(spc.y);
    /* 2591 inc y */
    spc.y = spc_inc(spc.y);
    /* 2592 inc y */
    spc.y = spc_inc(spc.y);
    /* 2593 inc y */
    spc.y = spc_inc(spc.y);
    /* 2594 inc y */
    spc.y = spc_inc(spc.y);
    /* 2595 inc y */
    spc.y = spc_inc(spc.y);
    /* 2596 inc y */
    spc.y = spc_inc(spc.y);
    /* 2597 inc y */
    spc.y = spc_inc(spc.y);
    /* 2598 dec x */
    spc.x = spc_dec(spc.x);
    /* 2599 bne $258C */
    if (!spc.z) goto L258C;
    /* 259B inc a */
    spc.a = spc_inc(spc.a);
    /* 259C mov.w $FF00+y,a */
    spc_st((uint16_t)(0xFF00 + spc.y), spc.a);
    /* 259F mov y,#$FF */
    spc.y = spc_nz(0xFF);
    /* 25A1 mov a,#$00 */
    spc.a = spc_nz(0x00);
    /* 25A3 mov.w $3F8C,a */
    spc_st(0x3F8C, spc.a);
    /* 25A6 mov.w $3F8D,y */
    spc_st(0x3F8D, spc.y);
    /* 25A9 mov.w $3F8E,a */
    spc_st(0x3F8E, spc.a);
    /* 25AC mov.w $3F8F,y */
    spc_st(0x3F8F, spc.y);
    /* 25AF mov.b a,$18 */
    spc.a = spc_nz(spc_rd(spc_dp(0x18)));
    /* 25B1 or.b a,$19 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0x19)));
    /* 25B3 bne $25B7 */
    if (!spc.z) goto L25B7;
    /* 25B5 inc.b $18 */
    spc_rmw(spc_dp(0x18), spc_inc);
L25B7:
    /* 25B7 ret */
    spc_ret();
    return;
}

/*
 * @routine 1B:859D SpcBNoiseSample
 * WHAT IT DOES: While a channel 0-6 plays sound $23, rewrites part of the sample at $FF00 each tick with noise from the LFSR $18/$19: seven blocks of eight bytes, the block chosen by the counter $0563 (the race driver's SpcNoiseSample checks channels 7-1). After it, SpcBCommandChannelCheck's lists at $25FF and $2608.
 * HOW: MOV1/EOR1 of two LFSR bits, ROL; bytes ORed with $11.
 * Entry: none. Exit: X, Y changed.
 * @spc 25B8
 * @spcuses 25B7-25FF
 */
void abi_SpcBNoiseSample(void)
{
    /* 25B8 mov x,#$00 */
    spc.x = spc_nz(0x00);
L25BA:
    /* 25BA mov.w a,$03C8+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x03C8 + spc.x)));
    /* 25BD cmp a,#$23 */
    spc_cmp(spc.a, 0x23);
    /* 25BF beq $25C8 */
    if (spc.z) goto L25C8;
    /* 25C1 inc x */
    spc.x = spc_inc(spc.x);
    /* 25C2 cmp x,#$07 */
    spc_cmp(spc.x, 0x07);
    /* 25C4 beq $25B7 */
    if (spc.z) goto L25B7;
    /* 25C6 bra $25BA */
    goto L25BA;
L25C8:
    /* 25C8 dec.w $0563 */
    spc_rmw(0x0563, spc_dec);
    /* 25CB mov.w a,$0563 */
    spc.a = spc_nz(spc_rd(0x0563));
    /* 25CE and a,#$03 */
    spc.a = spc_and(spc.a, 0x03);
    /* 25D0 mov y,#$3F */
    spc.y = spc_nz(0x3F);
    /* 25D2 mul ya */
    spc_mul();
    /* 25D3 mov y,a */
    spc.y = spc_nz(spc.a);
    /* 25D4 mov $20,#$07 */
    spc_st(spc_dp(0x20), 0x07);
L25D7:
    /* 25D7 inc y */
    spc.y = spc_inc(spc.y);
    /* 25D8 mov x,#$04 */
    spc.x = spc_nz(0x04);
L25DA:
    /* 25DA mov1 c,$0019.6 */
    spc.c = spc_rd(0x0019) >> 6 & 1;
    /* 25DD eor1 c,$0019.5 */
    spc.c ^= spc_rd(0x0019) >> 5 & 1;
    /* 25E0 rol.b $18 */
    spc_rmw(spc_dp(0x18), spc_rol);
    /* 25E2 rol.b $19 */
    spc_rmw(spc_dp(0x19), spc_rol);
    /* 25E4 mov.b a,$19 */
    spc.a = spc_nz(spc_rd(spc_dp(0x19)));
    /* 25E6 and a,#$D0 */
    spc.a = spc_and(spc.a, 0xD0);
    /* 25E8 or a,#$11 */
    spc.a = spc_or(spc.a, 0x11);
    /* 25EA mov.w $FF00+y,a */
    spc_st((uint16_t)(0xFF00 + spc.y), spc.a);
    /* 25ED inc y */
    spc.y = spc_inc(spc.y);
    /* 25EE mov.b a,$18 */
    spc.a = spc_nz(spc_rd(spc_dp(0x18)));
    /* 25F0 and a,#$FF */
    spc.a = spc_and(spc.a, 0xFF);
    /* 25F2 or a,#$11 */
    spc.a = spc_or(spc.a, 0x11);
    /* 25F4 mov.w $FF00+y,a */
    spc_st((uint16_t)(0xFF00 + spc.y), spc.a);
    /* 25F7 inc y */
    spc.y = spc_inc(spc.y);
    /* 25F8 dec x */
    spc.x = spc_dec(spc.x);
    /* 25F9 bne $25DA */
    if (!spc.z) goto L25DA;
    /* 25FB dbnz $20,$25D7 */
    { uint8_t v = (uint8_t)(spc_rd(spc_dp(0x20)) - 1); spc_wr(spc_dp(0x20), v); if (v) goto L25D7; }
    /* 25FE ret */
    spc_ret();
    return;
L25B7:
    /* 25B7 ret */
    spc_ret();
    return;
}
