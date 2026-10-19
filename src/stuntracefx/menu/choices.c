/*
 * choices.c: moving the menu's choices with the joypads. The frame code
 * appends each frame's newly pressed buttons to two word lists (player 1
 * at $1195, size $1191; player 2 at $11B5, size $1193); the menu works
 * through them here, left and right stepping a choice ($18CB for player 1,
 * $18C9 for player 2) through the tables at $0B:C81F (left) and $0B:C827
 * (right). Choice 3 is offered only when the saved progress byte at
 * $70:FC0B has bit 2 set (for player 1 also when the mode $18C3 is 3).
 */
#include "machine.h"

void SoundPort0QueuePush(uint8_t command);
void MenuSet1A66(uint8_t bank, uint16_t i);
uint16_t MenuSetPairAnimations(uint8_t bank, uint16_t pair, uint16_t *x, uint16_t *y, uint8_t *p);
uint16_t MenuChoiceRepeat(uint16_t *x, uint8_t *p);

#define CHOICE_LEFT 0x0BC81Fu
#define CHOICE_RIGHT 0x0BC827u
#define PRESS 0x118Fu            /* the press being worked on */
#define CHOICE_P1 0x0018CBu
#define CHOICE_P2 0x0018C9u
#define SAVE_PROGRESS 0x70FC0Bu
#define MENU_MODE 0x0018C3u

static uint32_t dbw(uint8_t bank, uint16_t a)
{
    return (uint32_t)bank << 16 | a;
}

/* MenuSet1A66 and MenuSetPairAnimations for choice c, with the registers
   MenuSet1A66 leaves (X = c * 2, Y = $1A64) before the second call. */
static uint16_t show_choice(uint8_t bank, uint16_t c, uint16_t *x, uint16_t *y, uint8_t *p)
{
    MenuSet1A66(bank, c);
    *x = (uint16_t)(c << 1);
    *y = 0x1A64;
    return MenuSetPairAnimations(bank, c, x, y, p);
}

/* One direction for player 1: sound $45, the choice through `table`, then
   choice 3 is replaced by `instead` unless it is allowed. */
static uint16_t step_p1(uint8_t bank, uint32_t table, uint16_t instead, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t c, s;

    SoundPort0QueuePush(0x45);
    cyc(54);
    c = rd16(CHOICE_P1);
    cyc(174);
    c = rd16(table + (uint16_t)(c << 1));
    wr16(CHOICE_P1, c);
    cyc(32);
    s = rd16(SAVE_PROGRESS) & 0x00FF;
    if (!s || (!(s & 0x0004) && rd16(cyc_then(136, MENU_MODE)) != 0x0003)) {
        cyc(102);
        c = rd16(CHOICE_P1);
        if (c == 0x0003)
            { cyc(96); wr16(CHOICE_P1, instead); } else { cyc(46); }
    } else { cyc(46); }
    cyc(32);
    c = rd16(CHOICE_P1);
    return show_choice(bank, c, x, y, p);
}

/*
 * @routine 0B:C676 MenuChoiceStepP1
 * WHAT IT DOES: Applies one press of player 1 ($118F) to the choice at
 * $18CB: left steps it through $0B:C81F, right through $0B:C827, each with
 * sound $45; a choice of 3 that is not allowed becomes 2 (left) or 1
 * (right). The new choice is shown (MenuSet1A66, MenuSetPairAnimations).
 * Left is handled before right. The mode test is not reached in the
 * traces so far.
 * HOW: 16-bit tests of the press; the choice and its tables are read with
 * long addressing; X is pushed around the table read.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: right not pressed: A = 0; Z
 * set, N clear; X, Y, C as left left them (as MenuSetPairAnimations, or
 * unchanged). Right pressed: as MenuSetPairAnimations.
 */
uint16_t MenuChoiceStepP1(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t a = rd16(cyc_then(24, dbw(bank, PRESS))) & 0x0200;

    if (a)
        { cyc(140); step_p1(bank, CHOICE_LEFT, 0x0002, x, y, p); } else { cyc(4); }
    a = rd16(cyc_then(66, dbw(bank, PRESS))) & 0x0100;
    if (!a) {
        cyc(4);
        *p = nz16(*p, a);
        return a;
    } else { cyc(140); }
    return step_p1(bank, CHOICE_RIGHT, 0x0001, x, y, p);
}

void abi_MenuChoiceStepP1(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuChoiceStepP1(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
}

/*
 * @routine 0B:C64C MenuChoicePressesP1
 * WHAT IT DOES: Works through player 1's presses queued this frame, last
 * first: each is stored at $118F, ORed into $118B, and applied to the
 * choice (MenuChoiceStepP1); then empties the list. $118B is cleared
 * first.
 * HOW: The list offset is pushed around each step and pulled back into A.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: list empty: A = -2; N set,
 * Z clear. Otherwise A = 0; Z set, N clear; X, Y, C from the last step.
 */
uint16_t MenuChoicePressesP1(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t i, v;

    wr16(dbw(bank, 0x118B), cyc_then(24, 0x0000));
    cyc(32);
    i = (uint16_t)(rd16(0x001191) - 2);
    if (i & 0x8000) {
        *p = nz16(*p, i);
        return i;
    }
    do {
        *x = i;
        cyc(216);
        v = rd16(0x001195 + i);
        wr16(dbw(bank, PRESS), cyc_then(24, v));
        v |= rd16(cyc_then(24, dbw(bank, 0x118B)));
        wr16(dbw(bank, 0x118B), cyc_then(24, v));
        cyc(76);
        MenuChoiceStepP1(bank, x, y, p);
        i = (uint16_t)(i - 2);
    } while (!(i & 0x8000));
    cyc(220);
    wr16(0x001191, 0x0000);
    *p = nz16(*p, 0);
    return 0;
}

void abi_MenuChoicePressesP1(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuChoicePressesP1(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
}

/* One direction for player 2: as step_p1 on $18C9, with the repeat value
   refreshed (MenuChoiceRepeat) before the choice is shown. */
static uint16_t step_p2(uint8_t bank, uint32_t table, uint16_t instead, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t c;

    SoundPort0QueuePush(0x45);
    cyc(54);
    c = rd16(CHOICE_P2);
    cyc(174);
    c = rd16(table + (uint16_t)(c << 1));
    wr16(CHOICE_P2, c);
    cyc(32);
    if (!(rd16(SAVE_PROGRESS) & 0x0004)) {
        cyc(72);
        c = rd16(CHOICE_P2);
        if (c == 0x0003)
            { cyc(96); wr16(CHOICE_P2, instead); } else { cyc(46); }
    }
    cyc(32);
    c = rd16(CHOICE_P2);
    cyc(30);
    MenuChoiceRepeat(x, p);
    cyc(4);
    return show_choice(bank, c, x, y, p);
}

/*
 * @routine 0B:C78A MenuChoiceStepP2
 * WHAT IT DOES: As MenuChoiceStepP1 for player 2's choice at $18C9; choice
 * 3 is allowed only by bit 2 of $70:FC0B, and the repeat value $18D3 is
 * refreshed (MenuChoiceRepeat) before the choice is shown. Not reached in
 * the traces so far, except with neither left nor right pressed.
 * HOW: As MenuChoiceStepP1; the choice is pushed around MenuChoiceRepeat.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: as MenuChoiceStepP1.
 */
uint16_t MenuChoiceStepP2(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t a = rd16(cyc_then(100, dbw(bank, PRESS))) & 0x0200;

    if (a)
        { cyc(140); step_p2(bank, CHOICE_LEFT, 0x0002, x, y, p); } else { cyc(4); }
    a = rd16(cyc_then(66, dbw(bank, PRESS))) & 0x0100;
    if (!a) {
        cyc(4);
        *p = nz16(*p, a);
        return a;
    } else { cyc(140); }
    return step_p2(bank, CHOICE_RIGHT, 0x0001, x, y, p);
}

void abi_MenuChoiceStepP2(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuChoiceStepP2(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
}

/*
 * @routine 0B:C760 MenuChoicePressesP2
 * WHAT IT DOES: As MenuChoicePressesP1 for player 2: the presses at $11B5
 * (size $1193), ORed into $118D, applied by MenuChoiceStepP2.
 * HOW: As MenuChoicePressesP1.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: as MenuChoicePressesP1.
 */
uint16_t MenuChoicePressesP2(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t i, v;

    wr16(dbw(bank, 0x118D), cyc_then(24, 0x0000));
    cyc(32);
    i = (uint16_t)(rd16(0x001193) - 2);
    if (i & 0x8000) {
        *p = nz16(*p, i);
        return i;
    }
    do {
        *x = i;
        cyc(216);
        v = rd16(0x0011B5 + i);
        wr16(dbw(bank, PRESS), cyc_then(24, v));
        v |= rd16(cyc_then(24, dbw(bank, 0x118D)));
        wr16(dbw(bank, 0x118D), cyc_then(24, v));
        MenuChoiceStepP2(bank, x, y, p);
        i = (uint16_t)(i - 2);
    } while (!(i & 0x8000));
    cyc(220);
    wr16(0x001193, 0x0000);
    *p = nz16(*p, 0);
    return 0;
}

void abi_MenuChoicePressesP2(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuChoicePressesP2(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
}

int MenuActorFind(uint8_t bank, uint16_t kind, uint16_t *x, uint16_t *y, uint8_t *p);
uint16_t MenuActorsSet10(uint8_t bank, uint16_t kind, uint16_t value, uint8_t *p);
uint16_t MenuChoiceAnimations(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p);
uint16_t MenuFixedColourUp(uint8_t bank, uint16_t x, uint8_t *p);
void SelectionTables18CBLong(uint8_t bank);

#define SETTING_ROW 0x0018EBu    /* the settings row being changed, 0-2 */
#define SETTINGS 0x0018CDu       /* the three settings, 0-2 each */
#define CURRENT 0x11F9u          /* the object being run */

/* Compare as CMP does: C when a >= b; N, Z from a - b. */
static uint8_t cmp16(uint8_t p, uint16_t a, uint16_t b)
{
    return (uint8_t)((nz16(p, (uint16_t)(a - b)) & ~P_C) | (a >= b ? (cyc(46), P_C) : (cyc(46), 0)));
}

/* Starts animation v on record rec unless +$14 already holds it (CMP of
   the field with v; on a change +$16 and +$18 restart from 0). Returns A:
   the field, or v. */
static uint16_t start_anim(uint16_t rec, uint16_t v, uint8_t *p)
{
    cyc(22);
    uint16_t a = rd16((uint16_t)(rec + 0x14));

    *p = cmp16(*p, a, v);
    if (a == v)
        return a;
    cyc(40);
    wr16((uint16_t)(rec + 0x14), v);
    cyc(22);
    wr16((uint16_t)(rec + 0x16), 0x0000);
    cyc(22);
    wr16((uint16_t)(rec + 0x18), 0x0000);
    *p = nz16(*p, v);
    return v;
}

/* MenuActorsSet10 with the registers its wrapper leaves: Y = $FFFE, N, Z
   from X. */
static uint16_t set10(uint8_t bank, uint16_t kind, uint16_t value, uint16_t *x, uint16_t *y, uint8_t *p)
{
    cyc(38);
    uint16_t a = MenuActorsSet10(bank, kind, value, p);

    *y = 0xFFFE;
    *p = nz16(*p, *x);
    return a;
}

/* SoundPort0QueuePush: N, Z come back from the data bank. */
static void sound0(uint8_t bank, uint8_t command, uint8_t *p)
{
    cyc(100);
    SoundPort0QueuePush(command);
    *p = nz8(*p, bank);
}

/* The menu choice for a saved 2-bit value, a word per value. */
#define CHOICE_FROM_SAVE 0x0BB9BAu

static uint16_t choice_from_save(uint32_t byte, unsigned shift, uint16_t *x)
{
    cyc(32);
    uint16_t v = (uint16_t)((rd16(byte) & (0x0003u << shift)) >> shift);

    *x = (uint16_t)(v << 1);
    return rd16(CHOICE_FROM_SAVE + *x);
}

/*
 * @routine 0B:B9C2 MenuChoicesFromSave
 * WHAT IT DOES: Restores the menu's choices from the saved settings in
 * Super FX RAM for the menu A: menu 0 sets $18CB from bits 0-1 of
 * $70:FC0C, menu 1 from bits 4-5 of it, menu 2 sets $18CB and $18C9 from
 * bits 0-1 and 4-5 of $70:FC0E; any other menu sets $18CB from bits 0-1 of
 * $70:FC0D, or to 1 when the progress byte $70:FC0B is 0. Each value is
 * mapped through the word table at $0B:B9BA. Menus other than 0-2 are not
 * reached in the traces so far.
 * HOW: 16-bit long reads; P is saved and restored.
 * Entry: A = the menu (16 bits are compared). Exit: A = the last value
 * stored; X = the last saved value * 2, unchanged for the default of 1; P
 * unchanged.
 */
uint16_t MenuChoicesFromSave(uint16_t menu, uint16_t *x)
{
    uint16_t v;

    switch (menu) {
    case 0:
        cyc(136);
        v = choice_from_save(0x70FC0C, 0, x);
        cyc(132);
        wr16(CHOICE_P1, v);
        cyc(22);
        return v;
    case 1:
        cyc(176);
        v = choice_from_save(0x70FC0C, 4, x);
        cyc(188);
        wr16(CHOICE_P1, v);
        cyc(22);
        return v;
    case 2:
        cyc(216);
        v = choice_from_save(0x70FC0E, 0, x);
        cyc(132);
        wr16(CHOICE_P1, v);
        v = choice_from_save(0x70FC0E, 4, x);
        cyc(188);
        wr16(CHOICE_P2, v);
        return v;
    }
    cyc(196);
    if (!(rd16(SAVE_PROGRESS) & 0x00FF)) {
        v = 0x0001;
        cyc(126);
        wr16(CHOICE_P1, v);
        return v;
    } else { cyc(64); }
    v = choice_from_save(0x70FC0D, 0, x);
    cyc(132);
    wr16(CHOICE_P1, v);
    return v;
}

void abi_MenuChoicesFromSave(void)
{
    cpu.a = MenuChoicesFromSave(cpu.a, &cpu.x);
    if (cpu.p & P_X)
        cpu.x &= 0x00FF; else { cyc(92); }
}

/*
 * @routine 0B:C1C8 MenuSettingsRowMarker
 * WHAT IT DOES: Moves the settings row marker (the kind $0D object) to the
 * row $18EB: its height +$02 = row * 32 + $34, and its animation $64, $5A
 * or $6E for rows 0, 1 and 2. Rows 0 and 1 are not reached in the traces
 * so far.
 * HOW: MenuActorFind for kind $0D; the animation is started only when +$14
 * does not already hold it.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the height; X = $11F9; Y
 * from the search; N, Z from X; C, V from the add.
 */
uint16_t MenuSettingsRowMarker(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    cyc(138);
    uint16_t row, a;

    MenuActorFind(bank, 0x000D, x, y, p);
    cyc(32);
    row = rd16(SETTING_ROW);
    if (row == 0)
        { cyc(16); start_anim(*x, 0x0064, p); }
    else if (row == 1)
        { cyc(62); start_anim(*x, 0x005A, p); }
    else if (row == 2)
        { cyc(108); start_anim(*x, 0x006E, p); }
    cyc(54);
    row = rd16(SETTING_ROW);
    a = (uint16_t)(row << 5);
    {
        unsigned s = (unsigned)a + 0x34;
        uint16_t r = (uint16_t)s;

        *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0) | ((~(a ^ 0x34) & (a ^ r) & 0x8000) ? P_V : 0));
        a = r;
    }
    cyc(130);
    wr16((uint16_t)(*x + 0x02), a);
    *x = rd16(cyc_then(24, dbw(bank, CURRENT)));
    *p = nz16(*p, *x);
    return a;
}

void abi_MenuSettingsRowMarker(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuSettingsRowMarker(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
}

/*
 * @routine 0B:C0E7 MenuSettingsShow
 * WHAT IT DOES: Shows the settings: derives the values that follow from
 * the choices (SelectionTables18CB), starts each choice's animation
 * (MenuChoiceAnimations), and gives the kind 3 object animation $36, $3C
 * or $30 for the value 0, 1 or 2 of the setting in the current row
 * ($18CD[$18EB]). Value 0 is not reached in the traces so far.
 * HOW: The value is kept at $11DF and the kind listed for the row at
 * $0B:C15E at $11E3 (unused here); MenuActorFind for kind 3.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the last value loaded;
 * X = $11F9; Y from the last search; N, Z from X; C from the last compare;
 * V from MenuChoiceAnimations.
 */
uint16_t MenuSettingsShow(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t i, v, a;

    SelectionTables18CBLong(bank);
    MenuChoiceAnimations(bank, x, y, p);
    cyc(74);
    i = (uint16_t)(rd16(SETTING_ROW) << 1);
    cyc(60);
    v = rd16(SETTINGS + i);
    wr16(dbw(bank, 0x11DF), cyc_then(24, v));
    wr16(dbw(bank, 0x11E3), rd16(cyc_then(72, 0x0BC15E + i)));
    a = rd16(cyc_then(24, dbw(bank, 0x11DF)));
    cyc(64);
    *p = cmp16(*p, a, 0x0000);
    if (a == 0) {
        MenuActorFind(bank, 0x0003, x, y, p);
        a = start_anim(*x, 0x0036, p);
    } else {
        *p = cmp16(*p, a, 0x0001);
        if (a == 1) {
            MenuActorFind(bank, 0x0003, x, y, p);
            a = start_anim(*x, 0x003C, p);
        } else {
            *p = cmp16(*p, a, 0x0002);
            if (a == 2) {
                MenuActorFind(bank, 0x0003, x, y, p);
                a = start_anim(*x, 0x0030, p);
            }
        }
    }
    *x = rd16(cyc_then(46, dbw(bank, CURRENT)));
    *p = nz16(*p, *x);
    return a;
}

void abi_MenuSettingsShow(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuSettingsShow(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
}

/*
 * @routine 0B:C048 MenuSettingsInput
 * WHAT IT DOES: Applies player 1's presses ($118B) on the settings screen:
 * left and right move the row $18EB (0-2) with sound $46 and move the row
 * marker (MenuSettingsRowMarker); up and down step the row's setting
 * $18CD[row] through 0-2, wrapping, with sound $45. Then shows the
 * settings (MenuSettingsShow, MenuChoiceAnimations). Left and up are not
 * reached in the traces so far.
 * HOW: Four 16-bit tests of the presses, in the order left ($0200), right
 * ($0100), up ($0800), down ($0400).
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: as MenuChoiceAnimations.
 */
uint16_t MenuSettingsInput(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t v, i;

    if (rd16(cyc_then(158, dbw(bank, 0x118B))) & 0x0200) {
        cyc(72);
        v = rd16(SETTING_ROW);
        if (v) {
            cyc(62);
            wr16(SETTING_ROW, (uint16_t)(v - 1));
            sound0(bank, 0x46, p);
            MenuSettingsRowMarker(bank, x, y, p);
        }
    } else { cyc(4); }
    if (rd16(cyc_then(66, dbw(bank, 0x118B))) & 0x0100) {
        cyc(72);
        v = rd16(SETTING_ROW);
        if (v != 0x0002) {
            cyc(86);
            wr16(SETTING_ROW, (uint16_t)(v + 1));
            sound0(bank, 0x46, p);
            MenuSettingsRowMarker(bank, x, y, p);
        } else { cyc(4); }
    } else { cyc(4); }
    if (rd16(cyc_then(66, dbw(bank, 0x118B))) & 0x0800) {
        cyc(40);
        sound0(bank, 0x45, p);
        cyc(54);
        i = (uint16_t)(rd16(SETTING_ROW) << 1);
        *x = i;
        cyc(60);
        v = (uint16_t)(rd16(SETTINGS + i) - 1);
        if (v == 0xFFFF)
            { cyc(18); v = 0x0002; }
        cyc(92);
        wr16(SETTINGS + i, v);
    } else { cyc(46); }
    if (rd16(cyc_then(24, dbw(bank, 0x118B))) & 0x0400) {
        cyc(40);
        sound0(bank, 0x45, p);
        cyc(54);
        i = (uint16_t)(rd16(SETTING_ROW) << 1);
        *x = i;
        cyc(60);
        v = (uint16_t)(rd16(SETTINGS + i) + 1);
        if (v == 0x0003)
            { cyc(18); v = 0x0000; }
        cyc(92);
        wr16(SETTINGS + i, v);
    } else { cyc(46); }
    *x = rd16(cyc_then(24, dbw(bank, CURRENT)));
    MenuSettingsShow(bank, x, y, p);
    return MenuChoiceAnimations(bank, x, y, p);
}

void abi_MenuSettingsInput(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuSettingsInput(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
}

/* The end of both settings handlers: $18D3 = 0, then the choice $18CB is
   shown (MenuSet1A66, MenuSetPairAnimations). */
static uint16_t settings_leave(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t c;

    cyc(56);
    wr16(0x0018D3, 0x0000);
    cyc(32);
    c = rd16(CHOICE_P1);
    return cyc_then(42, show_choice(bank, c, x, y, p));
}

/*
 * @routine 0B:BEEF MenuSettingsHandler
 * WHAT IT DOES: The handler of the settings screen: clears the fast
 * animation flag $18FD, raises the fixed colour (MenuFixedColourUp) and
 * applies the presses (MenuSettingsInput). When B, Y, Select or Start
 * ($F000) was pressed it leaves: kinds 4 and 5 get +$10 = 0 (sound $46
 * between them), kinds 6 and 7 animations $AE and $B8, kinds $0A-$0E +$10
 * = $FFFF; player 1's press list is emptied; the object moves on to
 * handler $BA4E with +$0A = 0 and +$1E = 1; kind 4 gets animation $8A, and
 * the choice $18CB is shown again with $18D3 = 0.
 * HOW: MenuActorsSet10 and MenuActorFind; X is pushed around the two
 * searches. The kind 4 animation change is not reached in the traces so
 * far.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: staying: A = 0;
 * Y = $118B; Z set, N clear; X, C, V as MenuSettingsInput. Leaving: as
 * MenuSetPairAnimations.
 */
uint16_t MenuSettingsHandler(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t a, keep;

    cyc(56);
    wr16(0x0018FD, 0x0000);
    MenuFixedColourUp(bank, *x, p);
    MenuSettingsInput(bank, x, y, p);
    *y = rd16(cyc_then(108, dbw(bank, 0x118B)));
    a = *y & 0xF000;
    if (!a) {
        cyc(54);
        *p = nz16(*p, a);
        return a;
    } else { cyc(60); }
    set10(bank, 0x0004, 0x0000, x, y, p);
    sound0(bank, 0x46, p);
    cyc(22);
    set10(bank, 0x0005, 0x0000, x, y, p);
    keep = *x;
    cyc(100);
    MenuActorFind(bank, 0x0006, x, y, p);
    start_anim(*x, 0x00AE, p);
    cyc(70);
    MenuActorFind(bank, 0x0007, x, y, p);
    start_anim(*x, 0x00B8, p);
    *x = keep;
    cyc(36);
    set10(bank, 0x000A, 0xFFFF, x, y, p);
    set10(bank, 0x000B, 0xFFFF, x, y, p);
    set10(bank, 0x000C, 0xFFFF, x, y, p);
    set10(bank, 0x000D, 0xFFFF, x, y, p);
    set10(bank, 0x000E, 0xFFFF, x, y, p);
    cyc(56);
    wr16(0x001191, 0x0000);
    cyc(46);
    wr16((uint16_t)(*x + 0x08), 0xBA4E);
    cyc(22);
    wr16((uint16_t)(*x + 0x0A), 0x0000);
    cyc(46);
    wr16((uint16_t)(*x + 0x1E), 0x0001);
    cyc(70);
    MenuActorFind(bank, 0x0004, x, y, p);
    start_anim(*x, 0x008A, p);
    return settings_leave(bank, x, y, p);
}

void abi_MenuSettingsHandler(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuSettingsHandler(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
}

/*
 * @routine 0B:BFB8 MenuSettingsHandler2
 * WHAT IT DOES: As MenuSettingsHandler for the second settings screen,
 * without clearing $18FD: when B, Y, Select or Start was pressed, kind 4
 * gets +$10 = 0 (then sound $46), $190B and $190D are cleared, kinds 8
 * and 9 get +$10 = 0 and kinds $0A-$0E $FFFF; the object moves on to
 * handler $C225 with +$1E = 1, player 1's press list is emptied and the
 * choice $18CB is shown again with $18D3 = 0.
 * HOW: MenuActorsSet10 for each kind.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: as
 * MenuSettingsHandler.
 */
uint16_t MenuSettingsHandler2(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t a;

    MenuFixedColourUp(bank, *x, p);
    MenuSettingsInput(bank, x, y, p);
    *y = rd16(cyc_then(108, dbw(bank, 0x118B)));
    a = *y & 0xF000;
    if (!a) {
        cyc(54);
        *p = nz16(*p, a);
        return a;
    } else { cyc(60); }
    set10(bank, 0x0004, 0x0000, x, y, p);
    sound0(bank, 0x46, p);
    cyc(78);
    wr16(0x00190B, 0x0000);
    cyc(32);
    wr16(0x00190D, 0x0000);
    set10(bank, 0x0008, 0x0000, x, y, p);
    set10(bank, 0x0009, 0x0000, x, y, p);
    set10(bank, 0x000A, 0xFFFF, x, y, p);
    set10(bank, 0x000B, 0xFFFF, x, y, p);
    set10(bank, 0x000C, 0xFFFF, x, y, p);
    set10(bank, 0x000D, 0xFFFF, x, y, p);
    set10(bank, 0x000E, 0xFFFF, x, y, p);
    cyc(46);
    wr16((uint16_t)(*x + 0x08), 0xC225);
    cyc(46);
    wr16((uint16_t)(*x + 0x1E), 0x0001);
    cyc(56);
    wr16(0x001191, 0x0000);
    return settings_leave(bank, x, y, p);
}

void abi_MenuSettingsHandler2(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuSettingsHandler2(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
}

uint16_t MenuChoiceUpDown(uint8_t bank);
uint16_t MenuActorFree(uint8_t bank, uint16_t rec, uint16_t slot);
uint16_t MenuActorSpawn(uint8_t bank, uint16_t list, uint16_t *a, uint16_t *y, uint8_t *p);

/*
 * @routine 0B:B952 MenuModeSelect
 * WHAT IT DOES: The handler of the mode menu: moves the selection $18C3
 * (MenuChoiceUpDown); on A or Start ($9000) plays sound $46 and confirms.
 * Mode 3 sets both choices to 1, and with no saved progress ($70:FC0B)
 * leaves the menu at once ($11EB = 1) and frees this object. Otherwise the
 * object moves on to handler $C5EF (+$0A = 0), spawn list $270 is made
 * (MenuActorSpawn), the choices are restored from the save for the mode
 * (MenuChoicesFromSave), $18D3 = 0 and the choice $18CB is shown.
 * HOW: MenuActorFree with Y = $11FB, this object's list slot.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: no press: A =
 * 0; Y = $118B; Z set, N clear. Left: A = the list entry moved; X = $FFFF;
 * Y = $11FB; N set, Z clear. Otherwise: A, Y, C as MenuSetPairAnimations;
 * X = $11F9; N, Z from X.
 */
uint16_t MenuModeSelect(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t a, mode, c;

    a = MenuChoiceUpDown(bank);
    *p = (uint8_t)(nz16(*p, a) & ~P_M);
    *y = rd16(cyc_then(66, dbw(bank, 0x118B)));  /* MenuChoiceUpDown leaves it in Y */
    a = *y & 0x9000;
    if (!a) {
        cyc(46);
        *p = nz16(*p, a);
        return a;
    } else { cyc(40); }
    sound0(bank, 0x46, p);
    cyc(54);
    mode = rd16(MENU_MODE);
    if (mode == 0x0003) {
        cyc(96);
        wr16(CHOICE_P1, 0x0001);
        cyc(32);
        wr16(CHOICE_P2, 0x0001);
        cyc(32);
        if (!(rd16(SAVE_PROGRESS) & 0x00FF)) {
            wr16(dbw(bank, 0x11EB), cyc_then(88, 0x0001));
            *y = rd16(cyc_then(24, dbw(bank, 0x11FB)));
            cyc(46);
            a = MenuActorFree(bank, *x, *y);
            *x = 0xFFFF;
            *p = (uint8_t)((*p & ~P_Z) | P_N);
            return a;
        }
    }
    cyc(92);
    wr16((uint16_t)(*x + 0x08), 0xC5EF);
    cyc(22);
    wr16((uint16_t)(*x + 0x0A), 0x0000);
    cyc(70);
    *x = MenuActorSpawn(bank, 0x0270, &a, y, p);
    cyc(74);
    MenuChoicesFromSave(rd16(MENU_MODE), x);
    cyc(126);
    wr16(0x0018D3, 0x0000);
    cyc(32);
    c = rd16(CHOICE_P1);
    a = show_choice(bank, c, x, y, p);
    *x = rd16(cyc_then(66, dbw(bank, CURRENT)));
    *p = nz16(*p, *x);
    return a;
}

void abi_MenuModeSelect(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuModeSelect(cpu.db, &cpu.x, &cpu.y, &p);
    cpu.p = p;
}
