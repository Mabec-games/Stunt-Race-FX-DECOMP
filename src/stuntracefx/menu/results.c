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
    wr16(fld(off), v);
}

/* DEC of a field (16-bit): N, Z from the result. */
static uint16_t dec_fld(uint16_t off)
{
    uint16_t v = (uint16_t)(rd16(fld(off)) - 1);

    wr16(fld(off), v);
    cpu.p = nz16(cpu.p, v);
    return v;
}

/* SEP #$20, LDA #value, (SEP #$20, REP #$10,) PHX, LDX #reg, JSL
   FrameByteWritePush, PLX: a byte for register reg on the frame's write
   list; X and its N, Z back. */
static void push_reg_byte(uint8_t value, uint16_t reg)
{
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
    abi_SoundPort0QueuePush();
    op_rep(P_M);
}

/* LDA.L long, AND #$FF: a byte of bank 0 as a word. */
static uint16_t byte_of(uint32_t addr)
{
    op_lda16(addr);
    op_and16(0x00FF);
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
        return;
    set_field(0x08, 0xD8B5);
    wr16(fld(0x0A), 0x0000);
}

/* The rise shared by ResultsRise and ResultsRiseAgain: +$0A up by 4 to at
   most $66, kept at $1921, the scroll $11EF = $66 - it; 1 when it is $66. */
static int rise(void)
{
    op_rep(P_M | P_X);
    op_lda16(fld(0x0A));
    op_add16(0x0004);
    op_cmp16(0x0066);
    if (cpu.p & P_C)
        op_lda16_imm(0x0066);
    wr16(fld(0x0A), cpu.a);
    wr16(0x001921, cpu.a);
    {
        uint16_t kept = cpu.a;

        op_lda16_imm(0x0066);
        op_sub16(rd16(fld(0x0A)));
        wr16(abs_db(0x11EF), cpu.a);
        cpu.a = kept;
        cpu.p = nz16(cpu.p, kept);
    }
    op_cmp16(0x0066);
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
        return;
    set_field(0x08, 0xD8FA);
    set_field(0x0A, 0x0000);
    push_reg_byte(0xE8, 0x420C);
    op_rep(P_M);
}

/* The window opening shared by ResultsOpenTop and ResultsOpenBottom: +$0A
   up by 8; below $100 its low byte goes to the window line at `line`;
   1 at $100. */
static int open_window(uint32_t line)
{
    op_lda16(fld(0x0A));
    op_add16(0x0008);
    wr16(fld(0x0A), cpu.a);
    op_cmp16(0x0100);
    if (cpu.p & P_Z)
        return 1;
    wr8(line, (uint8_t)cpu.a);
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
        return;
    set_field(0x08, 0xD91B);
    set_field(0x0A, 0x0000);
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
        return;
    set_field(0x08, 0xDA8A);
    set_field(0x0A, 0x0000);
    push_reg_byte(0xE0, 0x420C);
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    cpu.p = nz8(cpu.p, 0x00);
    push_reg_byte(0x00, 0x212E);
    op_rep(P_M);
    byte_of(0x000FAC);
    op_cmp16(0x0000);
    if (cpu.p & P_Z)
        return;
    byte_of(0x000DD6);
    op_cmp16(0x0008);
    if (!(cpu.p & P_Z))
        return;
    op_lda16_imm(0x0018);
    abi_MenuActorFind();
    set_field(0x08, 0xDA33);
    wr16(fld(0x0A), 0x0000);
    wr16(fld(0x22), 0x0000);
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
    op_cmp16(rd16(fld(0x22)));
    if (cpu.p & P_Z) {
        byte_of(0x000DD6);
        op_cmp16(0x0008);
        if (cpu.p & P_Z) {
            abi_MenuActorCopyGroup();
            cpu.x = rd16(abs_db(0x11F9));
            cpu.p = nz16(cpu.p, cpu.x);
            set_field(0x08, 0xDA33);
            wr16(fld(0x22), 0x0000);
            wr16(fld(0x0A), 0x0000);
            return;
        }
    }
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

    byte_of(0x000D4A);
    wr16(abs_db(0x11DF), cpu.a);
    op_asl16();
    op_asl16();
    op_asl16();
    op_add16(rd16(abs_db(0x11DF)));
    wr16(abs_db(0x11E1), cpu.a);
    op_lda16(fld(0x22));
    v = (uint16_t)(cpu.a - 1);
    cpu.a = v;
    cpu.p = nz16(cpu.p, v);
    wr16(abs_db(0x11DF), v);
    op_asl16();
    op_add16(rd16(abs_db(0x11DF)));
    op_add16(rd16(abs_db(0x11E1)));
    op_tax16();
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
    op_cmp16(rd16(fld(0x22)));
    table = (cpu.p & P_Z) ? 0x0BDA17 : 0x0BDA0F;
    abi_ResultsCarIndex();
    op_lda16(0x70FC6F + (uint32_t)cpu.x);
    op_and16(0x0003);
    op_asl16();
    op_tax16();
    op_lda16(table + (uint32_t)cpu.x);
    cpu.x = rd16(abs_db(0x11F9));
    cpu.p = nz16(cpu.p, cpu.x);
    wr16(fld(0x14), cpu.a);
    abi_MenuWaitThenAnimate();
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

    op_lda16(fld(0x12));
    op_tax16();
    op_lda16(0x7F1356 + (uint32_t)cpu.x);
    op_tay16();
    cpu.x = (uint16_t)(cpu.x + 2);
    do {
        op_lda16(0x7F135A + (uint32_t)cpu.x);
        op_and16(0xF1FF);
        cpu.a |= rd16(abs_db(0x11DF));
        cpu.p = nz16(cpu.p, cpu.a);
        wr16(0x7F135A + (uint32_t)cpu.x, cpu.a);
        op_txa16();
        op_add16(0x0008);
        op_tax16();
        cpu.y = (uint16_t)(cpu.y - 1);
        cpu.p = nz16(cpu.p, cpu.y);
    } while (!(cpu.y & 0x8000));
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
        set_field(0x0A, 0x0005);
        op_lda16(fld(0x22));
        cpu.a = (uint16_t)(cpu.a + 1);
        op_and16(0x0001);
        wr16(fld(0x22), cpu.a);
    }
    op_lda16(fld(0x22));
    op_asl16();
    op_tax16();
    op_lda16(0x0BDA86 + (uint32_t)cpu.x);
    wr16(abs_db(0x11DF), cpu.a);
    cpu.x = rd16(abs_db(0x11F9));
    cpu.p = nz16(cpu.p, cpu.x);
    op_lda16(fld(0x0A));
    wr16(0x00190F, cpu.a);
    op_lda16(fld(0x22));
    wr16(0x001911, cpu.a);
    abi_ResultsGroupPalette();
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
    abi_MenuActorCopyGroup();
    op_lda16(0x00190F);
    wr16(fld(0x0A), cpu.a);
    op_lda16(0x001911);
    wr16(fld(0x22), cpu.a);
    set_field(0x08, 0xDA33);
    abi_ResultsLeaderBlink();
}

/* Y = the A, B or Start presses ($118B AND $9000), with N, Z (TYA, AND). */
static uint16_t pressed(void)
{
    cpu.y = rd16(abs_db(0x118B));
    cpu.a = cpu.y;
    op_and16(0x9000);
    return cpu.a;
}

/* LDA #1, STA $11EB, LDY $11FB, JSR MenuActorFree, then the sound: the
   screen ends and the object is freed. */
static void end_screen(void)
{
    op_lda16_imm(0x0001);
    wr16(abs_db(0x11EB), 0x0001);
    cpu.y = rd16(abs_db(0x11FB));
    cpu.p = nz16(cpu.p, cpu.y);
    abi_MenuActorFree();
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
        return;
    byte_of(0x000DD6);
    op_cmp16(0x0008);
    if (!(cpu.p & P_Z)) {
        end_screen();
        return;
    }
    op_lda16(abs_db(0x0FAC));
    op_and16(0x00FF);
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
        return;
    set_field(0x08, 0xDC9F);
    set_field(0x0A, 0x0000);
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
        return;
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
        abi_MenuWaitThenAnimate();
        return;
    }
    cpu.x = rd16(abs_db(0x11F9));
    cpu.p = nz16(cpu.p, cpu.x);
    set_field(0x10, 0xFFFF);
    set_field(0x08, 0xDF7B);
}
