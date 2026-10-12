/*
 * frame.c: the menu's frame handshake. The menu code sets bit 0 of $11D8
 * when it has finished a frame; the menu's vertical blank code clears $11D8
 * once it has applied the frame's updates.
 */
#include "machine.h"

/*
 * @routine 0B:E390 MenuWaitFrame
 * WHAT IT DOES: Marks the frame finished and waits for the vertical blank
 * code to take it.
 * HOW: Sets bit 0 of $11D8 (16-bit, through the data bank) and polls it
 * until it is zero.
 * Entry: any widths. Exit: A = 0; P unchanged.
 */
void MenuWaitFrame(uint8_t bank)
{
    uint32_t a = (uint32_t)bank << 16 | 0x11D8;

    wr16(a, (uint16_t)(rd16(a) | 0x0001));
    while (rd16(a))
        ;
}

void abi_MenuWaitFrame(void)
{
    MenuWaitFrame(cpu.db);
    cpu.a = 0x0000;
}

void MenuActorsRun(void);
uint16_t MenuOamBuild(uint16_t *x, uint16_t *y);

/*
 * @routine 0B:E2E1 MenuRunFrames
 * WHAT IT DOES: Runs menu frames until $11DA equals $11DB: each frame waits
 * for the vertical blank (MenuWaitFrame), runs the objects
 * (MenuActorsRun), builds the sprite table (MenuOamBuild) and asks for
 * every frame update (bits 0-4 of $11D7).
 * HOW: 8-bit A for the flags and the compare; P saved and restored.
 * Entry: any widths. Exit: A low byte = $11DA, high byte as MenuOamBuild
 * left it; X, Y as MenuOamBuild left them; P unchanged.
 */
void MenuRunFrames(void)
{
    uint32_t b;
    uint8_t p = cpu.p, a;

    cpu.p |= P_M;
    do {
        MenuWaitFrame(cpu.db);
        cpu.a = 0x0000;
        MenuActorsRun();
        cpu.a = MenuOamBuild(&cpu.x, &cpu.y);
        b = (uint32_t)cpu.db << 16;
        wr8(b | 0x11D7, (uint8_t)(rd8(b | 0x11D7) | 0x1F));
        a = rd8(b | 0x11DA);
    } while (a != rd8(b | 0x11DB));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = p;
}

void abi_MenuRunFrames(void)
{
    MenuRunFrames();
}
