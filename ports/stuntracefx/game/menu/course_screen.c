/*
 * course_screen.c: setting up the menu screen that shows the chosen
 * course: its sprites, its picture drawn by the Super FX, and the colour
 * gradient behind it.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "carried.h"

uint8_t SoundPairFromTable(void);
void MenuOamFromList(uint8_t bank, uint16_t *x, uint16_t *y);
void OamDmaAll(void);
uint8_t GsuCF42To3000(uint16_t in68, uint16_t in6a);
uint16_t CoursePictureToVram(uint8_t bank);
uint16_t GradientSetup(uint8_t bank);

static uint32_t dbw(uint8_t bank, uint16_t a)
{
    return (uint32_t)bank << 16 | a;
}

/* The HUD's gauge and map positions at $0E09-$0E15 (hud/), in the order
   the code stores them. */
static const struct { uint16_t addr; uint8_t v; } course_params[] = {
    {0x0E0E, 0xB0}, {0x0E10, 0x1C}, {0x0E12, 0x18}, {0x0E14, 0x5A},
    {0x0E09, 0xB7}, {0x0E0C, 0x3C}, {0x0E0F, 0x10}, {0x0E11, 0x84},
    {0x0E13, 0x68}, {0x0E15, 0xC2}, {0x0E0A, 0x45}, {0x0E0D, 0xA4},
};

/*
 * @routine 0B:FA24 MenuCourseScreenSetup
 * WHAT IT DOES: Sets up the course screen: queues the two players' sounds
 * (SoundPairFromTable); hides every sprite (each word of the sprite table
 * copy $E0E0, of its high table $5555), then builds four sprite groups from
 * the lists at $0B:FCA2 (MenuOamFromList: sprite offsets 0, $200, $100,
 * $210 from list offsets 0, $154, $A8, $162) and copies the table to OAM;
 * has the Super FX draw the course picture (GsuCF42To3000, A = $D970, X =
 * $1D) and copies it to VRAM (CoursePictureToVram); sets OBSEL to 3,
 * $0D3F = 8, $0D7D = $0C and the HUD's gauge and map positions
 * $0E09-$0E15; then sets up the gradient (GradientSetup).
 * HOW: The lists are read with the data bank set to $0B; the stores to
 * OBSEL and work RAM go through the caller's data bank. P is saved and
 * restored.
 * Entry: D = $0000. Exit: A as GradientSetup; X = Y = $20; P unchanged.
 */
uint16_t MenuCourseScreenSetup(uint8_t bank)
{
    static const uint16_t groups[4][2] = {{0x0000, 0x0000}, {0x0200, 0x0154}, {0x0100, 0x00A8}, {0x0210, 0x0162}};
    uint16_t x, y;
    unsigned i;

    SoundPairFromTable();
    cyc(28);
    for (x = 0; x != 0x0200; x += 2)
        { wr16(dbw(bank, (uint16_t)(0x0375 + x)), cyc_then(72, 0xE0E0)); cyc(56); }
    cyc(0);
    for (x = 0; x != 0x0020; x += 2)
        { wr16(dbw(bank, (uint16_t)(0x0575 + x)), cyc_then(90, 0x5555)); cyc(38); }
    cyc(182);
    for (i = 0; cyc_loop(18, i < 4); i++) {
        cyc(56);
        x = groups[i][0];
        y = groups[i][1];
        MenuOamFromList(0x0B, &x, &y);
        cyc(20);
    }
    cyc(70);
    OamDmaAll();
    cyc(132);
    GsuCF42To3000(0xD970, 0x001D);
    cyc(62);
    CoursePictureToVram(bank);
    ppu_wr(dbw(bank, 0x2101), cyc_then(62, 0x03));
    wr8(dbw(bank, 0x0D3F), cyc_then(62, 0x08));
    wr8(dbw(bank, 0x0D7D), cyc_then(40, 0x0C));
    for (i = 0; i < sizeof course_params / sizeof course_params[0]; i++)
        wr8(dbw(bank, course_params[i].addr), cyc_then(40, course_params[i].v));
    uint16_t a = GradientSetup(bank);

    cyc(94);
    return a;
}

/* MenuCourseScreenSetup on carried registers. */
void MenuCourseScreenSetup_cr(Carried *cr)
{
    cyc(106);
    cr->a = MenuCourseScreenSetup(cr->db);
    cr->x = 0x0020;
    cr->y = 0x0020;
}

void abi_MenuCourseScreenSetup(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MenuCourseScreenSetup_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
