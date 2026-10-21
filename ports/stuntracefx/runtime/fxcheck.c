/*
 * fxcheck.c: the check of the game's fill lifted to plain C
 * (game/render/draw_list.c) against the Super FX's own (PORT_FXCHECK=1).
 * As FxDrawList starts, the lifted fill draws the same list into one
 * canvas; while it runs, each pixel the Super FX plots goes into another;
 * as it ends, the two are compared. At exit: the lists drawn and the
 * pixels the Super FX wrote in them, those that differed, and the pixels it
 * plotted outside any list.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "snes.h"
#include "gsu.h"
#include "draw_list.h"
#include "dispatch.h"

extern const uint8_t srfx_rom[], srfx_rom_end[];

static int on = -1, open_now;
static FxCanvas *mine, *chip;
static long lists, bad_lists, bad_pixels, first_bad = -1, pixels, outside;

static void chip_plot(uint8_t x, uint8_t y, uint8_t c)
{
    if (!open_now) {
        outside++;
        return;
    }
    chip->px[y << 8 | x] = c;
    chip->set[y << 8 | x] = 1;
}

static void report(void)
{
    fprintf(stderr, "fxcheck: %ld lists (%ld pixels), %ld differ (first at frame %ld), %ld pixels; %ld pixels plotted outside the lists\n",
            lists, pixels, bad_lists, first_bad, bad_pixels, outside);
}

void fx_list_begin(void)
{
    FxDrawSource s;
    if (on < 0) {
        on = getenv("PORT_FXCHECK") != NULL;
        if (on) {
            mine = (FxCanvas *)malloc(sizeof *mine);
            chip = (FxCanvas *)malloc(sizeof *chip);
            snes_gsu_plot_hook = chip_plot;
            atexit(report);
        }
    }
    if (!on)
        return;
    memset(mine, 0, sizeof *mine);
    memset(chip, 0, sizeof *chip);
    s.ram = snes_sfx_ram();
    s.rom = srfx_rom;
    s.rom_size = (uint32_t)(srfx_rom_end - srfx_rom);
    s.mode = gsu.scmr & 3;
    fx_draw_list(&s, mine);
    open_now = 1;
}

void fx_list_end(void)
{
    long d = 0;
    int i;
    if (!on || !open_now)
        return;
    open_now = 0;
    lists++;
    for (i = 0; i < 256 * 256; i++)
        pixels += chip->set[i];
    for (i = 0; i < 256 * 256; i++)
        if (mine->set[i] != chip->set[i] || (mine->set[i] && mine->px[i] != chip->px[i]))
            d++;
    if (d) {
        if (first_bad < 0)
            first_bad = (long)snes_frame();
        bad_lists++;
        bad_pixels += d;
    }
}
