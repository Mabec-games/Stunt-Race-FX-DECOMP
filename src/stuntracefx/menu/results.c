/*
 * results.c: the objects of the screens shown after a championship race
 * (the standings), run once a frame through their +$08 like the other menu
 * objects (handlers.c): X = the object, M 16-bit, X 16-bit, D = $0000, so
 * the object's fields are direct page offsets indexed by X. Fields used
 * here: +$08 (the handler), +$0A (a count), +$10, +$12 (the sprite group),
 * +$14 (the animation) and +$22 (a step). $0DD6 is the race phase (8 once
 * the championship is over) and $0FAC the leader's place.
 */
#include "machine.h"
#include "cpu_ops.h"

void abi_MenuWaitThenAnimate(void);
void abi_MenuActorCopyGroup(void);
void abi_MenuActorFind(void);
void abi_MenuActorFree(void);
void abi_FrameByteWritePush(void);
void abi_SoundPort0QueuePush(void);
void abi_MenuActorNoStep(void);

/* A field of object X: direct page (D = 0) + X, bank 0. */
static uint16_t fld(uint16_t off)
{
    return (uint16_t)(cpu.x + off);
}

/* An absolute address through the data bank. */
static uint32_t abs_db(uint16_t a)
{
    return (uint32_t)cpu.db << 16 | a;
}

/* LDA #v; STA field. */
static void set_field(uint16_t off, uint16_t v)
{
    op_lda16_imm(v);
    wr16(fld(off), cyc_then(8, v));
}

/* DEC of a field (16-bit): N, Z from the result. */
static uint16_t dec_fld(uint16_t off)
{
    uint16_t v = (uint16_t)(rd16(cyc_then(22, fld(off))) - 1);

    wr16(fld(off), cyc_then(14, v));
    cpu.p = nz16(cpu.p, v);
    return v;
}

/* SEP #$20, LDA #value, (SEP #$20, REP #$10,) PHX, LDX #reg, JSL
   FrameByteWritePush, PLX: a byte for register reg on the frame's write
   list; X and its N, Z back. */
static void push_reg_byte(uint8_t value, uint16_t reg)
{
    cyc(176);
    uint16_t x = cpu.x;

    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | value);
    cpu.p = nz8(cpu.p, value);
    op_rep(P_X);
    cpu.x = reg;
    abi_FrameByteWritePush();
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
}

/* SEP #$20, LDA #$46, JSL SoundPort0QueuePush, REP #$20: the menu's
   select sound. */
static void sound_46(void)
{
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x46);
    cpu.p = nz8(cpu.p, 0x46);
    (cyc(100), abi_SoundPort0QueuePush());
    op_rep(P_M);
    cyc(42);
}

/* LDA.L long, AND #$FF: a byte of bank 0 as a word. */
static uint16_t byte_of(uint32_t addr)
{
    cyc(32);
    op_lda16(addr);
    op_and16(0x00FF);
    cyc(4);
    return cpu.a;
}

/*
 * @routine 0B:D8A9 ResultsWaitThenRise
 * WHAT IT DOES: Counts object X's +$0A down; when it runs out, the object
 * goes on to ResultsRise (+$08 = $D8B5) with +$0A = 0.
 * HOW: DEC and the sign.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: counting: N,
 * Z from the count; else A = $D8B5, N set, Z clear.
 */
void abi_ResultsWaitThenRise(void)
{
    if (!(dec_fld(0x0A) & 0x8000))
        { cyc(64); return; } else { cyc(16); }
    set_field(0x08, 0xD8B5);
    wr16(fld(0x0A), cyc_then(22, 0x0000));
    cyc(42);
}

/* The rise shared by ResultsRise and ResultsRiseAgain: +$0A up by 4 to at
   most $66, kept at $1921, the scroll $11EF = $66 - it; 1 when it is $66. */
static int rise(void)
{
    op_rep(P_M | P_X);
    op_lda16(cyc_then(22, fld(0x0A)));
    op_add16(0x0004);
    op_cmp16(0x0066);
    if (cpu.p & P_C)
        op_lda16_imm(0x0066); else { cyc(20); }
    wr16(fld(0x0A), cyc_then(86, cpu.a));
    cyc(32);
    wr16(0x001921, cpu.a);
    {
        uint16_t kept = cpu.a;

        op_lda16_imm(0x0066);
        op_sub16(rd16(cyc_then(52, fld(0x0A))));
        wr16(abs_db(0x11EF), cyc_then(18, cpu.a));
        cpu.a = kept;
        cpu.p = nz16(cpu.p, kept);
    }
    op_cmp16(0x0066);
    cyc(4);
    return (cpu.p & P_Z) != 0;
}

/*
 * @routine 0B:D8B5 ResultsRise
 * WHAT IT DOES: Raises the standings panel: +$0A goes up by 4 to $66 (kept
 * at $1921), the scroll $11EF is $66 less it; at $66 the object goes on to
 * ResultsOpenTop (+$08 = $D8FA, +$0A = 0) and HDMA channels 3, 5, 6 and 7
 * are asked for ($E8 for $420C on the frame's write list).
 * HOW: The add is held at $66 with CMP and the carry; A is kept on the
 * stack around the scroll.
 * Entry: X = the object, D = $0000. Exit: M 16-bit, X 16-bit; rising: A =
 * +$0A, N, Z, C from the compare with $66; done: X = the object, N, Z from
 * it, A low byte $E8.
 */
void abi_ResultsRise(void)
{
    if (!rise())
        { cyc(114); return; } else { cyc(78); }
    set_field(0x08, 0xD8FA);
    set_field(0x0A, 0x0000);
    push_reg_byte(0xE8, 0x420C);
    op_rep(P_M);
    cyc(186);
}

/* The window opening shared by ResultsOpenTop and ResultsOpenBottom: +$0A
   up by 8; below $100 its low byte goes to the window line at `line`;
   1 at $100. */
static int open_window(uint32_t line)
{
    op_lda16(cyc_then(22, fld(0x0A)));
    op_add16(0x0008);
    wr16(fld(0x0A), cyc_then(60, cpu.a));
    op_cmp16(0x0100);
    if (cpu.p & P_Z)
        { cyc(46); return 1; }
    cyc(94);
    wr8(line, (uint8_t)cpu.a);
    cyc(42);
    return 0;
}

/*
 * @routine 0B:D8FA ResultsOpenTop
 * WHAT IT DOES: Opens the window's top band: +$0A up by 8, its low byte to
 * $7F:130D; at $100 the object goes on to ResultsOpenBottom (+$08 = $D91B,
 * +$0A = 0).
 * HOW: An 8-bit long store between SEP and REP.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: M 16-bit; A
 * = +$0A (0 when done); N, Z, C from the compare with $100, or from 0.
 */
void abi_ResultsOpenTop(void)
{
    if (!open_window(0x7F130D))
        { cyc(22); return; }
    set_field(0x08, 0xD91B);
    set_field(0x0A, 0x0000);
    cyc(42);
}

/*
 * @routine 0B:D91B ResultsOpenBottom
 * WHAT IT DOES: As ResultsOpenTop for the bottom band ($7F:130F); at $100
 * the object goes on to ResultsWaitButton (+$08 = $DA8A, +$0A = 0), HDMA
 * channels 5, 6 and 7 are asked for ($E0 for $420C) with the window mask
 * $212E cleared, and, when the leader's place $0FAC is not 0 and the
 * championship is over ($0DD6 = 8), the kind $18 object (MenuActorFind)
 * starts ResultsLeaderBlink (+$08 = $DA33, +$0A = +$22 = 0).
 * HOW: Two bytes on the frame's write list; the tests through 8-bit values
 * read as words and masked.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: M 16-bit;
 * opening: as ResultsOpenTop; done: A, X and the flags from the last step
 * (X = the kind $18 object when it was started).
 */
void abi_ResultsOpenBottom(void)
{
    if (!open_window(0x7F130F))
        { cyc(22); return; }
    set_field(0x08, 0xDA8A);
    set_field(0x0A, 0x0000);
    push_reg_byte(0xE0, 0x420C);
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    cpu.p = nz8(cpu.p, 0x00);
    cyc(122);
    push_reg_byte(0x00, 0x212E);
    op_rep(P_M);
    cyc(144);
    byte_of(0x000FAC);
    op_cmp16(0x0000);
    if (cpu.p & P_Z)
        return; else { cyc(58); }
    byte_of(0x000DD6);
    op_cmp16(0x0008);
    if (!(cpu.p & P_Z))
        return; else { cyc(90); }
    op_lda16_imm(0x0018);
    abi_MenuActorFind();
    set_field(0x08, 0xDA33);
    wr16(fld(0x0A), cyc_then(22, 0x0000));
    wr16(fld(0x22), cyc_then(22, 0x0000));
    cyc(42);
}

/*
 * @routine 0B:D985 ResultsPlaceWait
 * WHAT IT DOES: The place marker for place +$22: when the leader's place
 * $0FAC is it and the championship is over ($0DD6 = 8), copies its sprite
 * group (MenuActorCopyGroup) and starts ResultsLeaderBlink (+$08 = $DA33,
 * +$22 = +$0A = 0); otherwise MenuWaitThenAnimate.
 * HOW: Byte compares through words masked with $FF; X reloaded from $11F9
 * after the copy.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: started: X =
 * $11F9; A = $DA33; N, Z from it; otherwise as MenuWaitThenAnimate.
 */
void abi_ResultsPlaceWait(void)
{
    byte_of(0x000FAC);
    op_cmp16(rd16(cyc_then(40, fld(0x22))));
    if (cpu.p & P_Z) {
        cyc(16);
        byte_of(0x000DD6);
        op_cmp16(0x0008);
        if (cpu.p & P_Z) {
            cyc(104);
            abi_MenuActorCopyGroup();
            cpu.x = rd16(cyc_then(24, abs_db(0x11F9)));
            cpu.p = nz16(cpu.p, cpu.x);
            set_field(0x08, 0xDA33);
            wr16(fld(0x22), cyc_then(22, 0x0000));
            wr16(fld(0x0A), cyc_then(22, 0x0000));
            cyc(42);
            return;
        } else { cyc(42); }            /* CMP; the BNE taken is in the 46 below */
    }
    cyc(46);
    abi_MenuWaitThenAnimate();
}

/*
 * @routine 0B:D9EA ResultsCarIndex
 * WHAT IT DOES: X = the index of the car for place +$22 - 1 in the course
 * $0D4A's results: course * 9 + (place - 1) * 3.
 * HOW: Shifts and adds; $11DF and $11E1 hold the parts.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: X = A = the
 * index; N, Z, C, V from the last add.
 */
void abi_ResultsCarIndex(void)
{
    uint16_t v;

    cyc(0);
    byte_of(0x000D4A);
    wr16(abs_db(0x11DF), cyc_then(42, cpu.a));
    op_asl16();
    op_asl16();
    op_asl16();
    op_add16(rd16(cyc_then(80, abs_db(0x11DF))));
    wr16(abs_db(0x11E1), cyc_then(24, cpu.a));
    op_lda16(cyc_then(22, fld(0x22)));
    v = (uint16_t)(cpu.a - 1);
    cpu.a = v;
    cpu.p = nz16(cpu.p, v);
    wr16(abs_db(0x11DF), cyc_then(38, v));
    op_asl16();
    op_add16(rd16(cyc_then(52, abs_db(0x11DF))));
    op_add16(rd16(cyc_then(38, abs_db(0x11E1))));
    op_tax16();
    cyc(26);
}

/*
 * @routine 0B:D9AF ResultsPlaceCar
 * WHAT IT DOES: The car picture for place +$22: the car's kind (the low two
 * bits of the byte at $70:FC6F + ResultsCarIndex) picks its animation from
 * the table at $0B:DA17 for the leader's place ($0FAC) or $0B:DA0F for the
 * others, stored at +$14; then MenuWaitThenAnimate.
 * HOW: The table read through X, then X reloaded from $11F9.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as
 * MenuWaitThenAnimate.
 */
void abi_ResultsPlaceCar(void)
{
    uint32_t table;

    byte_of(0x000FAC);
    op_cmp16(rd16(cyc_then(40, fld(0x22))));
    table = (cpu.p & P_Z) ? (cyc(12), 0x0BDA17) : (cyc(18), 0x0BDA0F);
    (cyc(50), abi_ResultsCarIndex());
    cyc(32);
    op_lda16(0x70FC6F + (uint32_t)cpu.x);
    op_and16(0x0003);
    op_asl16();
    op_tax16();
    cyc(92);
    op_lda16(table + (uint32_t)cpu.x);
    cpu.x = rd16(abs_db(0x11F9));
    cpu.p = nz16(cpu.p, cpu.x);
    wr16(fld(0x14), cyc_then(22, cpu.a));
    (cyc(24), abi_MenuWaitThenAnimate());
}

/*
 * @routine 0B:DA62 ResultsGroupPalette
 * WHAT IT DOES: Gives every sprite of object X's group the palette bits
 * $11DF: each attribute word (the group's sprite count at $7F:1356 + +$12,
 * its entries from $7F:135A) has bits 9-11 replaced.
 * HOW: AND #$F1FF, ORA $11DF per entry, 8 bytes apart; X kept on the stack.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: X as on
 * entry, N, Z from it (PLX); Y = $FFFF; A = the last entry offset + 8;
 * C, V from the last add.
 */
void abi_ResultsGroupPalette(void)
{
    uint16_t obj = cpu.x;

    op_lda16(cyc_then(52, fld(0x12)));
    op_tax16();
    cyc(16);
    op_lda16(0x7F1356 + (uint32_t)cpu.x);
    op_tay16();
    cpu.x = (uint16_t)(cpu.x + 2);
    do {
        cyc(74);
        op_lda16(0x7F135A + (uint32_t)cpu.x);
        op_and16(0xF1FF);
        cpu.a |= rd16(cyc_then(46, abs_db(0x11DF)));
        cpu.p = nz16(cpu.p, cpu.a);
        cyc(32);
        wr16(0x7F135A + (uint32_t)cpu.x, cpu.a);
        op_txa16();
        op_add16(0x0008);
        op_tax16();
        cpu.y = (uint16_t)(cpu.y - 1);
        cpu.p = nz16(cpu.p, cpu.y);
    } while (!(cpu.y & 0x8000));
    cyc(114);
    cpu.x = obj;
    cpu.p = nz16(cpu.p, obj);
}

/*
 * @routine 0B:DA33 ResultsLeaderBlink
 * WHAT IT DOES: The leader's place marker blinks: every sixth frame (+$0A
 * run down from 5) its step +$22 flips between 0 and 1, which picks the
 * palette bits $200 or $600 (table $0B:DA86) for its group
 * (ResultsGroupPalette); the count and step are kept at $190F and $1911.
 * HOW: The palette word through $11DF; X reloaded from $11F9.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as
 * ResultsGroupPalette.
 */
void abi_ResultsLeaderBlink(void)
{
    if (dec_fld(0x0A) & 0x8000) {
        cyc(16);
        set_field(0x0A, 0x0005);
        op_lda16(cyc_then(22, fld(0x22)));
        cpu.a = (uint16_t)(cpu.a + 1);
        op_and16(0x0001);
        wr16(fld(0x22), cyc_then(58, cpu.a));
    } else { cyc(22); }
    op_lda16(cyc_then(22, fld(0x22)));
    op_asl16();
    op_tax16();
    cyc(70);
    op_lda16(0x0BDA86 + (uint32_t)cpu.x);
    wr16(abs_db(0x11DF), cpu.a);
    cpu.x = rd16(cyc_then(24, abs_db(0x11F9)));
    cpu.p = nz16(cpu.p, cpu.x);
    op_lda16(cyc_then(22, fld(0x0A)));
    cyc(32);
    wr16(0x00190F, cpu.a);
    op_lda16(cyc_then(22, fld(0x22)));
    cyc(32);
    wr16(0x001911, cpu.a);
    (cyc(46), abi_ResultsGroupPalette());
    cyc(42);
}

/*
 * @routine 0B:DA1F ResultsLeaderBlinkResume
 * WHAT IT DOES: Starts the leader's place marker blinking where it was:
 * copies its sprite group (MenuActorCopyGroup), takes the count and step
 * back from $190F and $1911, makes ResultsLeaderBlink its handler and runs
 * it.
 * HOW: Falls into ResultsLeaderBlink.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as
 * ResultsLeaderBlink.
 */
void abi_ResultsLeaderBlinkResume(void)
{
    cyc(46);
    abi_MenuActorCopyGroup();
    cyc(32);
    op_lda16(0x00190F);
    wr16(fld(0x0A), cyc_then(22, cpu.a));
    cyc(32);
    op_lda16(0x001911);
    wr16(fld(0x22), cyc_then(22, cpu.a));
    set_field(0x08, 0xDA33);
    abi_ResultsLeaderBlink();
}

/* Y = the A, B or Start presses ($118B AND $9000), with N, Z (TYA, AND). */
static uint16_t pressed(void)
{
    cpu.y = rd16(cyc_then(24, abs_db(0x118B)));
    cpu.a = cpu.y;
    op_and16(0x9000);
    return cpu.a;
}

/* LDA #1, STA $11EB, LDY $11FB, JSR MenuActorFree, then the sound: the
   screen ends and the object is freed. */
static void end_screen(void)
{
    op_lda16_imm(0x0001);
    wr16(abs_db(0x11EB), cyc_then(62, 0x0001));
    cpu.y = rd16(cyc_then(24, abs_db(0x11FB)));
    cpu.p = nz16(cpu.p, cpu.y);
    (cyc(46), abi_MenuActorFree());
    sound_46();
}

/*
 * @routine 0B:DA8A ResultsWaitButton
 * WHAT IT DOES: Waits for A, B or Start: then, when the championship is
 * over ($0DD6 = 8), the panel goes down again (ResultsLower: +$08 = $DAD1,
 * +$0A = $66), otherwise the screen ends ($11EB = 1, the object freed,
 * reached by a unit test, unit_menus); sound
 * $46 either way.
 * HOW: The leader's place $0FAC is read and masked but not used.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: no press: Y
 * = the presses, A = 0, Z set; else M 16-bit, A low byte $46 and the
 * flags as SoundPort0QueuePush leaves them.
 */
void abi_ResultsWaitButton(void)
{
    if (!pressed())
        { cyc(100); return; } else { cyc(52); }
    byte_of(0x000DD6);
    op_cmp16(0x0008);
    if (!(cpu.p & P_Z)) {
        cyc(12);
        end_screen();
        return;
    }
    op_lda16(cyc_then(82, abs_db(0x0FAC)));
    op_and16(0x00FF);
    cyc(22);
    set_field(0x08, 0xDAD1);
    set_field(0x0A, 0x0066);
    sound_46();
}

/*
 * @routine 0B:DC6D ResultsRiseAgain
 * WHAT IT DOES: As ResultsRise, then goes on to ResultsWaitEnd (+$08 =
 * $DC9F, +$0A = 0) with no HDMA change.
 * HOW: As ResultsRise.
 * Entry: X = the object, D = $0000. Exit: M 16-bit, X 16-bit; rising: as
 * ResultsRise; done: A = 0, Z set, N clear.
 */
void abi_ResultsRiseAgain(void)
{
    if (!rise())
        { cyc(114); return; } else { cyc(78); }
    set_field(0x08, 0xDC9F);
    set_field(0x0A, 0x0000);
    cyc(42);
}

/*
 * @routine 0B:DC9F ResultsWaitEnd
 * WHAT IT DOES: Waits for A, B or Start, then ends the screen ($11EB = 1),
 * frees the object (MenuActorFree) and plays sound $46.
 * HOW: As ResultsWaitButton's other case.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as
 * ResultsWaitButton.
 */
void abi_ResultsWaitEnd(void)
{
    if (!pressed())
        { cyc(100); return; }
    end_screen();
}

/*
 * @routine 0B:DD01 ResultsOverOrHide
 * WHAT IT DOES: When the championship is over ($0DD6 = 8),
 * MenuWaitThenAnimate; otherwise the object is hidden (+$10 = $FFFF) and
 * does nothing more (+$08 = MenuActorNoStep; reached by a unit test,
 * unit_menus).
 * HOW: The other case is the code after the JMP ($0B:DD10), reloading X
 * from $11F9.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as
 * MenuWaitThenAnimate; hidden: X = $11F9, A = $DF7B, N set, Z clear.
 */
void abi_ResultsOverOrHide(void)
{
    byte_of(0x000DD6);
    op_cmp16(0x0008);
    if (cpu.p & P_Z) {
        cyc(82);
        abi_MenuWaitThenAnimate();
        return;
    }
    cpu.x = rd16(cyc_then(88, abs_db(0x11F9)));
    cpu.p = nz16(cpu.p, cpu.x);
    set_field(0x10, 0xFFFF);
    set_field(0x08, 0xDF7B);
    cyc(42);
}

void abi_MenuActorsSet10(void);
void abi_MenuActorAnimateFrame(void);

/*
 * @routine 0B:DAD1 ResultsLower
 * WHAT IT DOES: Lowers the standings panel: +$0A goes down by 4 to 0 (kept
 * at $1921), the scroll $11EF is $66 less it; at 0 the object goes on to
 * ResultsRiseAgain (+$08 = $DC6D, +$0A = 0), $1925 = $5F, the objects of
 * kinds $14-$17 are hidden and those of kinds $1A-$25 shown
 * (MenuActorsSet10 with $FFFF and 0), and for the leader's place $0FAC
 * (1-3; places 2 and 3 reached by unit tests,
 * unit_menus) the kind $18 object gets animation $262, $268 or $26E,
 * +$02 = $88 and ResultsLeaderBlinkResume, the kind $19 object animation
 * $250, $256 or $25C, +$02 = $58 and MenuWaitThenAnimate.
 * HOW: An animation is restarted (+$16 = +$18 = 0) only when it changes.
 * The place tests are byte compares through words masked with $FF.
 * Entry: X = the object, D = $0000. Exit: M 16-bit, X 16-bit; lowering: A
 * = +$0A, N, Z, C from the compare with 0; done: X = the kind $19 object
 * (or as MenuActorFind left it), A and the flags from the last step.
 */
static void place_object(uint16_t kind, uint16_t anim0, uint16_t pos, uint16_t handler)
{
    uint16_t place, anim;

    op_lda16_imm(kind);
    abi_MenuActorFind();
    place = byte_of(0x000FAC);
    if (!place)
        return;
    op_cmp16(0x0001);
    anim = anim0;
    if (!(cpu.p & P_Z)) {
        cyc(42);
        op_cmp16(0x0002);
        anim = (cpu.p & P_Z) ? (uint16_t)(anim0 + 6) : (cyc(6), (uint16_t)(anim0 + 12));
    }
    op_lda16(cyc_then(place == 1 ? 96 : 100, fld(0x14)));
    op_cmp16(anim);
    if (!(cpu.p & P_Z)) {
        cyc(40);
        set_field(0x14, anim);
        wr16(fld(0x16), cyc_then(22, 0x0000));
        wr16(fld(0x18), cyc_then(22, 0x0000));
    }
    set_field(0x02, pos);
    set_field(0x08, handler);
    if (place <= 2)
        cyc(22);   /* BRA past the next place's code */
}

void abi_ResultsLower(void)
{
    static const uint16_t kinds[16] = {0x14, 0x15, 0x16, 0x17, 0x1A, 0x1B, 0x1C, 0x1D,
                                       0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25};
    unsigned i;

    op_rep(P_M | P_X);
    op_lda16(cyc_then(22, fld(0x0A)));
    op_sub16(0x0004);
    if (cpu.p & P_N)
        op_lda16_imm(0x0000); else { cyc(20); }
    wr16(fld(0x0A), cyc_then(56, cpu.a));
    cyc(32);
    wr16(0x001921, cpu.a);
    {
        uint16_t kept = cpu.a;

        op_lda16_imm(0x0066);
        op_sub16(rd16(cyc_then(52, fld(0x0A))));
        wr16(abs_db(0x11EF), cyc_then(18, cpu.a));
        cpu.a = kept;
        cpu.p = nz16(cpu.p, kept);
    }
    op_cmp16(0x0000);
    if (!(cpu.p & P_Z))
        { cyc(118); return; } else { cyc(82); }
    set_field(0x08, 0xDC6D);
    set_field(0x0A, 0x0000);
    op_lda16_imm(0x005F);
    cyc(18);
    wr16(0x001925, 0x005F);
    for (i = 0; cyc_loop(56, i < 16); i++) {
        op_lda16_imm(kinds[i]);
        cpu.y = i < 4 ? 0xFFFF : 0x0000;
        cpu.p = nz16(cpu.p, cpu.y);
        abi_MenuActorsSet10();
    }
    cyc(32);
    place_object(0x0018, 0x0262, 0x0088, 0xDA1F);
    cyc(32);
    place_object(0x0019, 0x0250, 0x0058, 0xD5EC);
    cyc(42);
}

/* The sprite tile at $11E3 into the next entry of the group, $11E1 or
   $11E5 (`at`) stepping by 8: the attribute word's low 9 bits replaced. */
static void tile_at(uint16_t at)
{
    cpu.x = rd16(cyc_then(24, abs_db(at)));
    cpu.p = nz16(cpu.p, cpu.x);
    cyc(32);
    op_lda16(0x7F135A + (uint32_t)cpu.x);
    op_and16(0xFE00);
    cpu.a |= rd16(cyc_then(46, abs_db(0x11E3)));
    cpu.p = nz16(cpu.p, cpu.a);
    cyc(32);
    wr16(0x7F135A + (uint32_t)cpu.x, cpu.a);
    op_lda16(cyc_then(24, abs_db(at)));
    op_add16(0x0008);
    wr16(abs_db(at), cyc_then(62, cpu.a));
}

/* A character two sprites high: tile $11E3 and the one $10 on, through
   the entries at `at`. */
static void tall_tile(uint16_t at)
{
    tile_at(at);
    op_lda16(cyc_then(24, abs_db(0x11E3)));
    op_add16(0x0010);
    wr16(abs_db(0x11E3), cyc_then(62, cpu.a));
    tile_at(at);
    cyc(78);
}

/*
 * @routine 0B:DD87 ResultsLetterTiles
 * WHAT IT DOES: Puts letter A (0-31) in the next two sprites of the group
 * at $11E5: its tile from the table at $0B:DDD3 and the tile $10 on below
 * it.
 * HOW: The tile through $11E3; X kept on the stack.
 * Entry: M 16-bit, X 16-bit; A = the letter. Exit: X as on entry, N, Z
 * from it; A = the new $11E5; C, V from its add.
 */
void abi_ResultsLetterTiles(void)
{
    uint16_t x = cpu.x;

    op_asl16();
    op_tax16();
    cyc(100);
    op_lda16(0x0BDDD3 + (uint32_t)cpu.x);
    wr16(abs_db(0x11E3), cpu.a);
    tall_tile(0x11E5);
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
}

/*
 * @routine 0B:DE83 ResultsDigitTiles
 * WHAT IT DOES: Puts digit A (0-9; 15 for a blank, tile $94) in the next
 * two sprites of the group at $11E1: tile A + $50 and the one $10 on.
 * HOW: As ResultsLetterTiles with the tile computed.
 * Entry: M 16-bit, X 16-bit; A = the digit. Exit: X as on entry, N, Z
 * from it; A = the new $11E1; C, V from its add.
 */
void abi_ResultsDigitTiles(void)
{
    uint16_t x = cpu.x;

    op_cmp16(0x000F);
    if (cpu.p & P_Z)
        op_lda16_imm(0x0044); else { cyc(20); }
    op_add16(0x0050);
    wr16(abs_db(0x11E3), cyc_then(118, cpu.a));
    tall_tile(0x11E1);
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
}

/*
 * @routine 0B:DE69 ResultsBcdPair
 * WHAT IT DOES: Puts the two decimal digits of the byte A (high, then low)
 * in the group at $11E1 (ResultsDigitTiles).
 * HOW: The byte kept at $11E5.
 * Entry: M 16-bit, X 16-bit; A low byte = the digits. Exit: as
 * ResultsDigitTiles.
 */
void abi_ResultsBcdPair(void)
{
    op_and16(0x00FF);
    wr16(abs_db(0x11E5), cyc_then(46, cpu.a));
    op_lsr16();
    op_lsr16();
    op_lsr16();
    op_lsr16();
    op_and16(0x000F);
    (cyc(124), abi_ResultsDigitTiles());
    op_lda16(cyc_then(24, abs_db(0x11E5)));
    op_and16(0x000F);
    (cyc(68), abi_ResultsDigitTiles());
    cyc(42);
}

/*
 * @routine 0B:DCBF ResultsTimeDigits
 * WHAT IT DOES: When the championship is over ($0DD6 = 8), shows the time
 * $0EEF, $0EEE, $0EED (two digits each) on object X's sprite group and
 * stops it (+$08 = MenuActorNoStep); otherwise hides it (+$10 = $FFFF;
 * reached by unit tests, unit_menus).
 * HOW: MenuActorCopyGroup, then the digits from the group's first entry
 * ($11E1); X reloaded from $11F9.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: X = $11F9; A
 * = $DF7B; N set, Z clear.
 */
void abi_ResultsTimeDigits(void)
{
    byte_of(0x000DD6);
    op_cmp16(0x0008);
    if (cpu.p & P_Z) {
        cyc(104);
        abi_MenuActorCopyGroup();
        op_lda16(cyc_then(22, fld(0x12)));
        cpu.a = (uint16_t)(cpu.a + 2);
        cpu.p = nz16(cpu.p, cpu.a);
        wr16(abs_db(0x11E1), cyc_then(52, cpu.a));
        cyc(32);
        op_lda16(0x000EEF);
        (cyc(46), abi_ResultsBcdPair());
        cyc(32);
        op_lda16(0x000EEE);
        (cyc(46), abi_ResultsBcdPair());
        cyc(32);
        op_lda16(0x000EED);
        (cyc(46), abi_ResultsBcdPair());
        cpu.x = rd16(cyc_then(24, abs_db(0x11F9)));
        cpu.p = nz16(cpu.p, cpu.x);
    } else {
        cpu.x = rd16(cyc_then(88, abs_db(0x11F9)));
        cpu.p = nz16(cpu.p, cpu.x);
        set_field(0x10, 0xFFFF);
    }
    set_field(0x08, 0xDF7B);
    cyc(42);
}

/* The leader's place test of ResultsInitials and ResultsLapTimes: when
   $0FAC is the object's field `field` and the championship is over, the
   object starts ResultsLeaderBlink; otherwise it stops. */
static void leader_or_stop(uint16_t field)
{
    cpu.x = rd16(cyc_then(24, abs_db(0x11F9)));
    cpu.p = nz16(cpu.p, cpu.x);
    byte_of(0x000FAC);
    op_cmp16(rd16(cyc_then(40, fld(field))));
    if (cpu.p & P_Z) {
        cyc(16);
        byte_of(0x000DD6);
        op_cmp16(0x0008);
        if (cpu.p & P_Z) {
            cpu.x = rd16(cyc_then(82, abs_db(0x11F9)));
            cpu.p = nz16(cpu.p, cpu.x);
            set_field(0x08, 0xDA33);
            wr16(fld(0x22), cyc_then(22, 0x0000));
            wr16(fld(0x0A), cyc_then(22, 0x0000));
            cyc(42);
            return;
        } else { cyc(46); }
    } else { cyc(4); }
    cyc(18);
    set_field(0x08, 0xDF7B);
    cyc(42);
}

/*
 * @routine 0B:DD1E ResultsInitials
 * WHAT IT DOES: Shows the initials of the car for place +$22 (three 5-bit
 * letters in the word at $70:FC6D + ResultsCarIndex) on object X's sprite
 * group, then starts ResultsLeaderBlink when it is the leader's place and
 * the championship is over, otherwise stops (MenuActorNoStep).
 * HOW: MenuActorCopyGroup; the word shifted right 5 at a time through
 * $11DF; ResultsLetterTiles from the group's first entry ($11E5).
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: X = $11F9;
 * A and the flags from the last step.
 */
void abi_ResultsInitials(void)
{
    cyc(46);
    unsigned i;

    abi_MenuActorCopyGroup();
    op_lda16(cyc_then(22, fld(0x12)));
    cpu.a = (uint16_t)(cpu.a + 2);
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(abs_db(0x11E5), cyc_then(52, cpu.a));
    (cyc(46), abi_ResultsCarIndex());
    cyc(32);
    op_lda16(0x70FC6D + (uint32_t)cpu.x);
    wr16(abs_db(0x11DF), cyc_then(24, cpu.a));
    for (i = 0; i < 3; i++) {
        if (i) {
            op_lda16(cyc_then(24, abs_db(0x11DF)));
            op_lsr16();
            op_lsr16();
            op_lsr16();
            op_lsr16();
            op_lsr16();
            wr16(abs_db(0x11DF), cyc_then(94, cpu.a));
        }
        cyc(68);
        op_and16(0x001F);
        abi_ResultsLetterTiles();
    }
    leader_or_stop(0x22);
}

/*
 * @routine 0B:DF61 ResultsBlankLetters
 * WHAT IT DOES: Puts three blank letters (26) in object X's sprite group.
 * HOW: ResultsLetterTiles from the group's first entry ($11E5).
 * Entry: M 16-bit, X 16-bit; X = the object. Exit: as ResultsLetterTiles.
 */
void abi_ResultsBlankLetters(void)
{
    unsigned i;

    op_lda16(cyc_then(22, fld(0x12)));
    cpu.a = (uint16_t)(cpu.a + 2);
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(abs_db(0x11E5), cyc_then(52, cpu.a));
    for (i = 0; cyc_loop(32, i < 3); i++) {
        op_lda16_imm(0x001A);
        abi_ResultsLetterTiles();
    }
    cyc(42);
}

/*
 * @routine 0B:DED5 ResultsNoTime
 * WHAT IT DOES: For a place with no time: by the current object's kind
 * ($20, $21 or another; $20 reached by a unit test, unit_menus) blanks
 * the initials of the kind $23, $24 or $25 object (ResultsBlankLetters)
 * and hides the kind $1D, $1E or $1F object (+$10 = $FFFF, +$02 = $200,
 * MenuActorNoStep).
 * HOW: $11DF and $11E1 kept on the stack.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: X = the hidden object; A =
 * $11DF; N, Z from it.
 */
void abi_ResultsNoTime(void)
{
    uint16_t k11df = rd16(cyc_then(24, abs_db(0x11DF))), k11e1, letters, other;

    cpu.a = k11df;
    cpu.p = nz16(cpu.p, k11df);
    k11e1 = rd16(cyc_then(54, abs_db(0x11E1)));
    cpu.x = rd16(cyc_then(54, abs_db(0x11F9)));
    cpu.p = nz16(cpu.p, cpu.x);
    op_lda16(cyc_then(22, fld(0x1A)));
    op_cmp16(0x0020);
    if (cpu.p & P_Z) {
        letters = 0x0023;
        other = 0x001D;
        cyc(78);
    } else {
        cyc(30);
        op_cmp16(0x0021);
        letters = (cpu.p & P_Z) ? (cyc(6), 0x0024) : 0x0025;
        other = (cpu.p & P_Z) ? 0x001E : 0x001F;
        cyc(82);
    }
    op_lda16_imm(letters);
    abi_MenuActorFind();
    (cyc(46), abi_ResultsBlankLetters());
    op_lda16_imm(other);
    (cyc(32), abi_MenuActorFind());
    set_field(0x10, 0xFFFF);
    set_field(0x02, 0x0200);
    set_field(0x08, 0xDF7B);
    wr16(abs_db(0x11E1), cyc_then(60, k11e1));
    op_lda16_imm(k11df);
    wr16(abs_db(0x11DF), cyc_then(22, k11df));
    cyc(42);
}

/*
 * @routine 0B:DE0B ResultsLapTimes
 * WHAT IT DOES: Shows the time of place +$22 (the bytes at $0F78, $0F77 and
 * $0F76 + it, two digits each) on object X's sprite group; a place with no
 * time ($FF) first blanks its line (ResultsNoTime). Then starts
 * ResultsLeaderBlink when +$20 is the leader's place and the championship
 * is over, otherwise stops (MenuActorNoStep).
 * HOW: MenuActorCopyGroup; ResultsBcdPair from the group's first entry
 * ($11E1); the place and its first byte kept on the stack.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: X = $11F9;
 * A and the flags from the last step.
 */
void abi_ResultsLapTimes(void)
{
    cyc(46);
    uint16_t place, first;

    abi_MenuActorCopyGroup();
    op_lda16(cyc_then(22, fld(0x12)));
    cpu.a = (uint16_t)(cpu.a + 2);
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(abs_db(0x11E1), cyc_then(52, cpu.a));
    op_lda16(cyc_then(22, fld(0x22)));
    op_tax16();
    place = cpu.x;
    cyc(16);
    op_lda16(0x000F78 + (uint32_t)place);
    first = cpu.a;
    op_and16(0x00FF);
    op_cmp16(0x00FF);
    if (cpu.p & P_Z)
        { cyc(168); abi_ResultsNoTime(); } else { cyc(128); }
    cyc(118);
    cpu.x = place;
    cpu.a = first;
    cpu.p = nz16(cpu.p, first);
    abi_ResultsBcdPair();
    cyc(32);
    op_lda16(0x000F77 + (uint32_t)cpu.x);
    (cyc(46), abi_ResultsBcdPair());
    cyc(32);
    op_lda16(0x000F76 + (uint32_t)cpu.x);
    (cyc(46), abi_ResultsBcdPair());
    cyc(0);
    leader_or_stop(0x20);
}

/*
 * @routine 0B:E12E ResultsPointsTile
 * WHAT IT DOES: Puts tile A in the next sprite of the group at $11E1.
 * HOW: The tile through $11E3; X kept on the stack.
 * Entry: M 16-bit, X 16-bit; A = the tile. Exit: X as on entry, N, Z from
 * it; A = the new $11E1; C, V from its add.
 */
void abi_ResultsPointsTile(void)
{
    uint16_t x = cpu.x;

    wr16(abs_db(0x11E3), cyc_then(54, cpu.a));
    tile_at(0x11E1);
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
    cyc(78);
}

/*
 * @routine 0B:E114 ResultsPointsPair
 * WHAT IT DOES: Puts the two decimal digits of the byte A (high, then low)
 * in the group at $11E1 as single tiles 0-9 (ResultsPointsTile).
 * HOW: The byte kept at $11E5.
 * Entry: M 16-bit, X 16-bit; A low byte = the digits. Exit: as
 * ResultsPointsTile.
 */
void abi_ResultsPointsPair(void)
{
    op_and16(0x00FF);
    wr16(abs_db(0x11E5), cyc_then(46, cpu.a));
    op_lsr16();
    op_lsr16();
    op_lsr16();
    op_lsr16();
    op_and16(0x000F);
    (cyc(124), abi_ResultsPointsTile());
    op_lda16(cyc_then(24, abs_db(0x11E5)));
    op_and16(0x000F);
    (cyc(68), abi_ResultsPointsTile());
    cyc(42);
}

/*
 * @routine 0B:E0E0 ResultsPoints
 * WHAT IT DOES: Shows the points of car A (five digits: the low digit of
 * $0EFB + 3A, then the bytes $0EFA and $0EF9 + 3A) on object X's sprite
 * group.
 * HOW: MenuActorCopyGroup with A kept on the stack, then at $11DF; the
 * digits from the group's first entry ($11E1).
 * Entry: M 16-bit, X 16-bit, D = $0000; A = the car; X = the object. Exit:
 * X = $11F9, N, Z from it; A and C, V from the last step.
 */
void abi_ResultsPoints(void)
{
    cyc(76);
    uint16_t car = cpu.a, at;

    abi_MenuActorCopyGroup();
    cpu.a = car;
    cpu.p = nz16(cpu.p, car);
    wr16(abs_db(0x11DF), cyc_then(60, car));
    op_lda16(cyc_then(22, fld(0x12)));
    cpu.a = (uint16_t)(cpu.a + 2);
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(abs_db(0x11E1), cyc_then(52, cpu.a));
    op_lda16(cyc_then(24, abs_db(0x11DF)));
    op_asl16();
    op_add16(rd16(cyc_then(52, abs_db(0x11DF))));
    op_tax16();
    at = cpu.x;
    cyc(16);
    op_lda16(0x000EFB + (uint32_t)at);
    op_and16(0x000F);
    (cyc(68), abi_ResultsPointsTile());
    cyc(32);
    op_lda16(0x000EFA + (uint32_t)cpu.x);
    (cyc(46), abi_ResultsPointsPair());
    cyc(32);
    op_lda16(0x000EF9 + (uint32_t)cpu.x);
    (cyc(46), abi_ResultsPointsPair());
    cpu.x = rd16(cyc_then(24, abs_db(0x11F9)));
    cpu.p = nz16(cpu.p, cpu.x);
    cyc(42);
}

/*
 * @routine 0B:E14F ResultsPhaseAnim
 * WHAT IT DOES: Gives object X the animation for the race phase $0DD6: $1A4
 * for 6, $17C for $0A, $19A for $0C, $190 for $0E, $186 for $10; none for
 * the others. Reached by unit tests (unit_menus).
 * HOW: A compare chain; the animation is restarted (+$16 = +$18 = 0) only
 * when it changes.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: A = the
 * animation (or the phase), N, Z, C from the last compare.
 */
void abi_ResultsPhaseAnim(void)
{
    static const uint16_t phase[5] = {0x0006, 0x000A, 0x000C, 0x000E, 0x0010};
    static const uint16_t anim[5] = {0x01A4, 0x017C, 0x019A, 0x0190, 0x0186};
    unsigned i;

    byte_of(0x000DD6);
    for (i = 0; i < 5; i++) {
        cyc(40);
        op_cmp16(phase[i]);
        if (cpu.p & P_Z)
            break;
    }
    if (i == 5)
        { cyc(60); return; }
    op_lda16(cyc_then(46, fld(0x14)));
    op_cmp16(anim[i]);
    if (cpu.p & P_Z)
        { cyc(64); return; } else { cyc(40); }
    set_field(0x14, anim[i]);
    wr16(fld(0x16), cyc_then(22, 0x0000));
    wr16(fld(0x18), cyc_then(22, 0x0000));
    cyc(42);
}

/* ResultsPlace0-3: place n's points object: after its count +$0A runs
   out it shows (+$10 = 0, +$08 = its animate handler) and, when the
   championship is over or place n is past $0D48, shows car n's points
   (ResultsPoints) and stops; at $0D48 = n it animates by the phase
   (ResultsPhaseAnim); before it, it hides again. The $0D48 cases are
   reached by unit tests (unit_menus), except place 0's hiding: $0D48 is
   never below 0, so the BCC after CMP #0 never branches. */
static void place_points(uint16_t n, uint16_t handler)
{
    if (!(dec_fld(0x0A) & 0x8000))
        { cyc(64); return; }
    wr16(fld(0x10), cyc_then(38, 0x0000));
    set_field(0x08, handler);
    byte_of(0x000DD6);
    op_cmp16(0x0008);
    if (!(cpu.p & P_Z)) {
        cyc(58);
        byte_of(0x000D48);
        op_cmp16(n);
        if (cpu.p & P_Z) {
            cyc(110);
            abi_ResultsPhaseAnim();
            cyc(42);
            return;
        } else { cyc(cpu.p & P_C ? 50 : 80); }
        if (!(cpu.p & P_C)) {
            set_field(0x10, 0xFFFF);
            cyc(42);
            return;
        }
    } else { cyc(18); }
    cyc(78);
    op_lda16_imm(n);
    abi_ResultsPoints();
    cyc(0);
    set_field(0x08, 0xE0DF);
    cyc(42);
}

/*
 * @routine 0B:DFDB ResultsPlace0
 * WHAT IT DOES: Place 0's points object (see place_points in the C):
 * counts +$0A down, then shows, with ResultsPlace0Animate as its handler,
 * and shows car 0's points or animates by $0D48 (the hiding below it can
 * never run: the BCC after CMP #0 never branches).
 * HOW: As the other places, with 0.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: counting: N,
 * Z from the count; else as ResultsPoints, ResultsPhaseAnim or the hiding
 * store (A = $FFFF).
 */
void abi_ResultsPlace0(void)
{
    place_points(0x0000, 0xE018);
}

/*
 * @routine 0B:E006 ResultsPlace0Hide
 * WHAT IT DOES: ResultsPlace0's hiding store, which nothing reaches (the
 * BCC after CMP #0 before it never branches): +$10 = $FFFF.
 * HOW: A 16-bit direct page store indexed by the object.
 * Entry: M 16-bit, D = $0000; X = the object. Exit: A = $FFFF; N set, Z
 * clear.
 */
void abi_ResultsPlace0Hide(void)
{
    set_field(0x10, 0xFFFF);
    cyc(42);
}

/*
 * @routine 0B:E01C ResultsPlace1
 * WHAT IT DOES: As ResultsPlace0 for place 1 (handler ResultsPlace1Animate).
 * HOW: As ResultsPlace0, with 1.
 * Entry: as ResultsPlace0. Exit: as ResultsPlace0.
 */
void abi_ResultsPlace1(void)
{
    place_points(0x0001, 0xE059);
}

/*
 * @routine 0B:E05D ResultsPlace2
 * WHAT IT DOES: As ResultsPlace0 for place 2 (handler ResultsPlace2Animate).
 * HOW: As ResultsPlace0, with 2.
 * Entry: as ResultsPlace0. Exit: as ResultsPlace0.
 */
void abi_ResultsPlace2(void)
{
    place_points(0x0002, 0xE09A);
}

/*
 * @routine 0B:E09E ResultsPlace3
 * WHAT IT DOES: As ResultsPlace0 for place 3 (handler ResultsPlace3Animate).
 * HOW: As ResultsPlace0, with 3.
 * Entry: as ResultsPlace0. Exit: as ResultsPlace0.
 */
void abi_ResultsPlace3(void)
{
    place_points(0x0003, 0xE0DB);
}

/*
 * @routine 0B:E018 ResultsPlace0Animate
 * WHAT IT DOES: Place 0's points object's frame while animating:
 * MenuActorAnimateFrame.
 * HOW: JSR, RTS.
 * Entry: as MenuActorAnimateFrame. Exit: as MenuActorAnimateFrame.
 */
void abi_ResultsPlace0Animate(void)
{
    abi_MenuActorAnimateFrame();
    cyc(42);
}

/*
 * @routine 0B:E059 ResultsPlace1Animate
 * WHAT IT DOES: As ResultsPlace0Animate, for place 1.
 * HOW: JSR, RTS.
 * Entry: as MenuActorAnimateFrame. Exit: as MenuActorAnimateFrame.
 */
void abi_ResultsPlace1Animate(void)
{
    abi_MenuActorAnimateFrame();
    cyc(42);
}

/*
 * @routine 0B:E09A ResultsPlace2Animate
 * WHAT IT DOES: As ResultsPlace0Animate, for place 2.
 * HOW: JSR, RTS.
 * Entry: as MenuActorAnimateFrame. Exit: as MenuActorAnimateFrame.
 */
void abi_ResultsPlace2Animate(void)
{
    abi_MenuActorAnimateFrame();
    cyc(42);
}

/*
 * @routine 0B:E0DB ResultsPlace3Animate
 * WHAT IT DOES: As ResultsPlace0Animate, for place 3.
 * HOW: JSR, RTS.
 * Entry: as MenuActorAnimateFrame. Exit: as MenuActorAnimateFrame.
 */
void abi_ResultsPlace3Animate(void)
{
    abi_MenuActorAnimateFrame();
    cyc(42);
}
