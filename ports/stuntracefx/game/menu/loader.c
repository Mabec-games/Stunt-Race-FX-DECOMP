/*
 * loader.c: the object that loads a menu screen's graphics, one step per
 * frame. Each step either has the Super FX draw a picture into $70:3000
 * (program $01:CF42) or uses what is already in Super FX RAM, queues a
 * VRAM transfer from Super FX RAM for the next vertical blank, and sets the
 * object's handler (+$08) to the next step:
 *
 *   D1F7 -> D234 -> D26F -> D2C9 -> D304 -> D333 -> D362 -> D391 -> D3D8
 *   -> D418 -> D447 -> D476 -> D4A5 (MenuSetExit)
 *
 * The object being run is the one at $11F9 (D = $0000).
 */
#include "machine.h"

uint8_t GsuCF42To3000(uint16_t in68, uint16_t in6a);
void VramDmaQueueAdd(void);
uint16_t MenuPictureDC8CGrid0(uint8_t bank);

/* Stage a transfer of `size` bytes from $70:src to VRAM word address
   `vram` and queue it; the staging area is filled in the original's
   order (size, bank, source, VRAM address). */
static void queue_from_gsu_ram(uint16_t size, uint16_t src, uint16_t vram)
{
    cyc(78);
    wr16(0x0016B1, size);
    cyc(56);
    wr16(0x0016B5, 0x0070);
    cyc(56);
    wr16(0x0016B3, src);
    cyc(56);
    wr16(0x0016B7, vram);
    (cyc(62), VramDmaQueueAdd());
}

/* Move the running object on to `next`; returns the object. */
static uint16_t next_step(uint8_t bank, uint16_t next)
{
    cyc(0);
    uint16_t x = rd16((uint32_t)bank << 16 | 0x11F9);

    cyc(46);
    wr16((uint16_t)(x + 0x08), next);
    return x;
}

/* The common exit: A = the next handler, X = the object; M 16-bit, X
   16-bit; N, Z from A. */
static void step_exit(uint16_t next)
{
    cpu.x = next_step(cpu.db, next);
    cpu.a = next;
    cpu.p = (uint8_t)(nz16(cpu.p, next) & ~(P_M | P_X));
    cyc(42);
}

/*
 * @routine 0B:D1F7 MenuLoadPictureCC78
 * WHAT IT DOES: Draws picture $CC78 (input $17) and queues $1000 bytes of it
 * to VRAM $3000; next step D234.
 * HOW: GsuCF42To3000, then queue_from_gsu_ram.
 * Entry: D = $0000. Exit: A = $D234; X = the object; M 16-bit, X 16-bit;
 * N set, Z clear.
 */
void MenuLoadPictureCC78(uint8_t bank)
{
    GsuCF42To3000(0xCC78, 0x0017);
    queue_from_gsu_ram(0x1000, 0x3000, 0x3000);
    next_step(bank, 0xD234);
}

void abi_MenuLoadPictureCC78(void)
{
    cyc(132);
    GsuCF42To3000(0xCC78, 0x0017);
    cyc(40);
    queue_from_gsu_ram(0x1000, 0x3000, 0x3000);
    cyc(68);
    step_exit(0xD234);
}

/*
 * @routine 0B:D234 MenuLoadPictureCE78
 * WHAT IT DOES: Draws picture $CE78 (input $17) and queues $1000 bytes of it
 * to VRAM $6800; next step D26F.
 * HOW: As MenuLoadPictureCC78.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: as MenuLoadPictureCC78, A =
 * $D26F.
 */
void MenuLoadPictureCE78(uint8_t bank)
{
    GsuCF42To3000(0xCE78, 0x0017);
    queue_from_gsu_ram(0x1000, 0x3000, 0x6800);
    next_step(bank, 0xD26F);
}

void abi_MenuLoadPictureCE78(void)
{
    cyc(110);
    GsuCF42To3000(0xCE78, 0x0017);
    cyc(40);
    queue_from_gsu_ram(0x1000, 0x3000, 0x6800);
    cyc(68);
    step_exit(0xD26F);
}

/*
 * @routine 0B:D26F MenuLoadPictureGrid
 * WHAT IT DOES: Draws picture $DC8C with the tile grid from tile 0 laid
 * over it (MenuPictureDC8CGrid0) and queues both copies, $800 bytes each,
 * to VRAM: $70:3000 to $6000 and $70:3800 to $6400; next step D2C9.
 * HOW: queue_from_gsu_ram twice.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: as MenuLoadPictureCC78, A =
 * $D2C9.
 */
void MenuLoadPictureGrid(uint8_t bank)
{
    MenuPictureDC8CGrid0(bank);
    queue_from_gsu_ram(0x0800, 0x3000, 0x6000);
    queue_from_gsu_ram(0x0800, 0x3800, 0x6400);
    next_step(bank, 0xD2C9);
}

void abi_MenuLoadPictureGrid(void)
{
    cyc(46);
    MenuPictureDC8CGrid0(cpu.db);
    cyc(22);
    queue_from_gsu_ram(0x0800, 0x3000, 0x6000);
    cyc(44);
    queue_from_gsu_ram(0x0800, 0x3800, 0x6400);
    cyc(68);
    step_exit(0xD2C9);
}

/*
 * @routine 0B:D2C9 MenuLoadPictureDD8C
 * WHAT IT DOES: Draws picture $DD8C (input $17) and queues $1000 bytes of it
 * to VRAM $4000; next step D304.
 * HOW: As MenuLoadPictureCC78.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: as MenuLoadPictureCC78, A =
 * $D304.
 */
void MenuLoadPictureDD8C(uint8_t bank)
{
    GsuCF42To3000(0xDD8C, 0x0017);
    queue_from_gsu_ram(0x1000, 0x3000, 0x4000);
    next_step(bank, 0xD304);
}

void abi_MenuLoadPictureDD8C(void)
{
    cyc(110);
    GsuCF42To3000(0xDD8C, 0x0017);
    cyc(40);
    queue_from_gsu_ram(0x1000, 0x3000, 0x4000);
    cyc(68);
    step_exit(0xD304);
}

/*
 * @routine 0B:D304 MenuLoadGsu4000
 * WHAT IT DOES: Queues $1000 bytes from $70:4000 to VRAM $4800; next step
 * D333.
 * HOW: queue_from_gsu_ram.
 * Entry: D = $0000. Exit: A = $D333; X = the object; M 16-bit, X 16-bit; N
 * set, Z clear.
 */
void MenuLoadGsu4000(uint8_t bank)
{
    queue_from_gsu_ram(0x1000, 0x4000, 0x4800);
    next_step(bank, 0xD333);
}

void abi_MenuLoadGsu4000(void)
{
    queue_from_gsu_ram(0x1000, 0x4000, 0x4800);
    cyc(68);
    step_exit(0xD333);
}

/*
 * @routine 0B:D333 MenuLoadGsu5000
 * WHAT IT DOES: Queues $1000 bytes from $70:5000 to VRAM $5000; next step
 * D362.
 * HOW: As MenuLoadGsu4000.
 * Entry: as MenuLoadGsu4000. Exit: as MenuLoadGsu4000, A = $D362.
 */
void MenuLoadGsu5000(uint8_t bank)
{
    queue_from_gsu_ram(0x1000, 0x5000, 0x5000);
    next_step(bank, 0xD362);
}

void abi_MenuLoadGsu5000(void)
{
    queue_from_gsu_ram(0x1000, 0x5000, 0x5000);
    cyc(68);
    step_exit(0xD362);
}

/*
 * @routine 0B:D362 MenuLoadGsu6000
 * WHAT IT DOES: Queues $1000 bytes from $70:6000 to VRAM $5800; next step
 * D391.
 * HOW: As MenuLoadGsu4000.
 * Entry: as MenuLoadGsu4000. Exit: as MenuLoadGsu4000, A = $D391.
 */
void MenuLoadGsu6000(uint8_t bank)
{
    queue_from_gsu_ram(0x1000, 0x6000, 0x5800);
    next_step(bank, 0xD391);
}

void abi_MenuLoadGsu6000(void)
{
    queue_from_gsu_ram(0x1000, 0x6000, 0x5800);
    cyc(68);
    step_exit(0xD391);
}

/* Copy `bytes` from the Super FX picture at $70:3000 to bank $7F. */
static void picture_to_7f(uint16_t dest, uint16_t bytes)
{
    uint16_t x;

    cyc(0);
    for (x = 0; x != bytes; x = (uint16_t)(x + 2))
        { cyc(96); wr16(0x7F0000 + (uint16_t)(dest + x), cyc_then(32, rd16(0x703000 + x))); cyc(10); }
    cyc(82);
}

/*
 * @routine 0B:D391 MenuLoadPicturesTo7F
 * WHAT IT DOES: Draws picture $9068 (input $1D) and keeps its first $2000
 * bytes at $7F:A7D7, then picture $F8E8 (input $17) and keeps $6800 bytes at
 * $7F:3FD7; next step D3D8.
 * HOW: GsuCF42To3000 and a word copy, twice.
 * Entry: D = $0000. Exit: A = $D3D8; X = the object; M 16-bit, X 16-bit; N
 * set, Z clear.
 */
void MenuLoadPicturesTo7F(uint8_t bank)
{
    GsuCF42To3000(0x9068, 0x001D);
    picture_to_7f(0xA7D7, 0x2000);
    GsuCF42To3000(0xF8E8, 0x0017);
    picture_to_7f(0x3FD7, 0x6800);
    next_step(bank, 0xD3D8);
}

void abi_MenuLoadPicturesTo7F(void)
{
    cyc(132);
    GsuCF42To3000(0x9068, 0x001D);
    picture_to_7f(0xA7D7, 0x2000);
    cyc(86);
    GsuCF42To3000(0xF8E8, 0x0017);
    picture_to_7f(0x3FD7, 0x6800);
    step_exit(0xD3D8);
}

/*
 * @routine 0B:D3D8 MenuLoadClearGsu
 * WHAT IT DOES: Clears $3400 bytes of Super FX RAM from $70:3000 and queues
 * $1000 of them to VRAM $0000; next step D418.
 * HOW: A word loop of zero stores, then queue_from_gsu_ram.
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = $D418; X = the object; N
 * set, Z clear.
 */
static void clear_gsu_picture(void)
{
    uint16_t x;

    x = 0;
    cyc(80);
    do {
        wr16(0x703000 + x, 0x0000);
        x = (uint16_t)(x + 2);
    } while (cyc_loop(106, x != 0x3400));
    cyc(68);
    queue_from_gsu_ram(0x1000, 0x3000, 0x0000);
}

void MenuLoadClearGsu(uint8_t bank)
{
    clear_gsu_picture();
    next_step(bank, 0xD418);
}

void abi_MenuLoadClearGsu(void)
{
    clear_gsu_picture();
    cyc(68);
    step_exit(0xD418);
}

/*
 * @routine 0B:D418 MenuLoadBlank0800
 * WHAT IT DOES: Queues $1000 bytes from $70:3000 (cleared) to VRAM $0800;
 * next step D447.
 * HOW: As MenuLoadGsu4000.
 * Entry: as MenuLoadGsu4000. Exit: as MenuLoadGsu4000, A = $D447.
 */
void MenuLoadBlank0800(uint8_t bank)
{
    queue_from_gsu_ram(0x1000, 0x3000, 0x0800);
    next_step(bank, 0xD447);
}

void abi_MenuLoadBlank0800(void)
{
    queue_from_gsu_ram(0x1000, 0x3000, 0x0800);
    cyc(68);
    step_exit(0xD447);
}

/*
 * @routine 0B:D447 MenuLoadBlank1000
 * WHAT IT DOES: Queues $1000 bytes from $70:3000 to VRAM $1000; next step
 * D476.
 * HOW: As MenuLoadGsu4000.
 * Entry: as MenuLoadGsu4000. Exit: as MenuLoadGsu4000, A = $D476.
 */
void MenuLoadBlank1000(uint8_t bank)
{
    queue_from_gsu_ram(0x1000, 0x3000, 0x1000);
    next_step(bank, 0xD476);
}

void abi_MenuLoadBlank1000(void)
{
    queue_from_gsu_ram(0x1000, 0x3000, 0x1000);
    cyc(68);
    step_exit(0xD476);
}

/*
 * @routine 0B:D476 MenuLoadBlank1800
 * WHAT IT DOES: Queues $400 bytes from $70:3000 to VRAM $1800; next step
 * D4A5 (MenuSetExit).
 * HOW: As MenuLoadGsu4000.
 * Entry: as MenuLoadGsu4000. Exit: as MenuLoadGsu4000, A = $D4A5.
 */
void MenuLoadBlank1800(uint8_t bank)
{
    queue_from_gsu_ram(0x0400, 0x3000, 0x1800);
    next_step(bank, 0xD4A5);
}

void abi_MenuLoadBlank1800(void)
{
    queue_from_gsu_ram(0x0400, 0x3000, 0x1800);
    cyc(68);
    step_exit(0xD4A5);
}
