# Every hardware touch in the game's C, and what replaces it

> The conversion's starting inventory, taken 2026-11-09 over the decomp's C. On the native game (`native/stuntracefx`) the
> finder now reports nothing; what is still left of section 8 is in [PLAN.md](PLAN.md), "Where it stands".

`tools/nshvc-find-hardware` of nativeshvc 6456fe0 (API 4.4) over `src/stuntracefx` at c0c3b709: 32,613 lines. Its "bus access" lines are divided here by what the address is. Every site of a real hardware touch is in `hardware-touches-sites.txt` (3,554 lines), with its replacement. The finder also reports `ports/stuntracefx/front`: 7 lines, the console connector.

## Hardware

| Touch | What it is | Sites | Replaced by |
|---|---|---:|---|
| `spc_rd`, `spc_wr`, `spc_wait_until` | SPC700 state | 1640 | nshvc_voice_*, nshvc_echo, nshvc_sound_* from the music code |
| `DMAP0`, `DMA_`, `MDMAEN`, `A1B0`, `A1T0L`, `DAS0L`, `A1B1`, `BBAD0`, `BBAD1`, `DMAP1`, `A1B2`, `BBAD2`, `DMAP2`, `A1B3`, `A1B4`, `A1B5`, `BBAD3`, `BBAD4`, `BBAD5`, `DMAP3`, `DMAP4`, `DMAP5`, `A1T1L`, `A1B6`, `A1B7`, `A1T0H`, `A1T2L`, `A1T3L`, `A1T4L`, `A1T5L`, `BBAD6`, `BBAD7`, `DAS0H`, `DAS4L`, `DAS5L`, `DMAP6`, `DMAP7` | DMA | 388 | an object write: nshvc_tiles_write, nshvc_map_write_snes, nshvc_palette_write, nshvc_sprites_set |
| `VMAIN`, `VMADDL`, `VMDATAL`, `VMADDH`, `VMDATAH` | video memory port | 187 | nshvc_tiles_write, nshvc_map_write_snes, nshvc_map_write |
| `NMITIMEN`, `VTIMEL`, `HTIMEL`, `TIMEUP`, `VTIMEH`, `RDNMI`, `HTIMEH` | interrupts | 118 | gone: the work moves to update and draw; a raster change is nshvc_line |
| `BG2VOFS`, `BG1VOFS`, `BG2HOFS`, `BG1HOFS`, `BG4VOFS`, `BG3HOFS`, `BG3VOFS`, `BG4HOFS` | layer scroll | 113 | nshvc_layer.scroll_x/_y; by line NSHVC_LINE_SCROLL_X/_Y + layer |
| `GSU_` | Super FX register | 92 | gone: a plain C call of the program's function |
| `HDMAEN`, `DASB1`, `DASB3`, `DASB4`, `DASB5` | HDMA | 76 | nshvc_lines in draw, from the game's table |
| `INIDISP` | screen brightness and blank | 73 | nshvc_screen_set (.brightness, .blank); by line NSHVC_LINE_BRIGHTNESS, NSHVC_LINE_BLANK |
| `jump_routine` | the console connector | 72 | a direct C call; nshvc_game for the entry |
| `CGWSEL`, `CGADSUB` | colour maths | 69 | nshvc_blend_set; by line the NSHVC_LINE_SUBTRACT group |
| `CGDATA`, `CGADD` | palette port | 69 | nshvc_palette_write; by line NSHVC_LINE_PALETTE + index |
| `APUIO0`, `APUIO1`, `APUIO2`, `APUIO3` | sound port | 65 | nshvc_music_send, read with nshvc_music_receive |
| `W12SEL`, `WOBJSEL`, `WBGLOG`, `WOBJLOG`, `W34SEL` | window use and logic | 60 | nshvc_windows.layer_use; by line NSHVC_LINE_WIN_USE + layer |
| `OAMDATA`, `OAMADDL`, `OAMADDH`, `OAM` | sprite table port | 58 | nshvc_sprites_set |
| `JOY1L`, `HVBJOY`, `JOY2L`, `JOYSER0`, `JOY1H`, `JOY2H`, `JOY3H`, `JOY3L`, `JOY4H`, `JOY4L`, `JOYSER1` | joypads | 56 | the nshvc_input given to update |
| `M7A`, `M7B`, `MPYL`, `MPYM` | used as the signed multiplier | 53 | ordinary C multiplication |
| `TM`, `TS` | layers on the main and sub screens | 49 | nshvc_layer.enabled_main/_sub, nshvc_sprites_show; by line NSHVC_LINE_MAIN/_SUB + layer |
| `TMW`, `TSW` | window masks | 49 | nshvc_windows.mask_main/_sub; by line NSHVC_LINE_MASK_MAIN/_SUB + layer |
| `BG2SC`, `BG1SC`, `BG3SC`, `BG4SC` | layer map | 44 | nshvc_layer.map (a split: one map and a line scroll) |
| `call_routine` | a call by address | 43 | a direct C call; nshvc_game for the entry |
| `WH0`, `WH1`, `WH2`, `WH3` | window edges | 36 | nshvc_windows.w[]; by line NSHVC_LINE_WIN1_LEFT and its three neighbours |
| `WRMPYA`, `RDDIV`, `RDMPYH`, `WRMPYB`, `RDDIVL`, `RDMPYL`, `WRDIVB`, `WRDIVL` | multiplier and divider | 34 | ordinary C |
| `COLDATA` | fixed colour | 31 | nshvc_blend.fixed_bgr15; by line NSHVC_LINE_FIXED |
| `BGMODE` | picture mode | 17 | nshvc_mode_set; by line NSHVC_LINE_MODE |
| `M7C`, `M7D`, `M7SEL`, `M7X`, `M7Y` | the multiplier's second write, and names in hw.h | 12 | ordinary C multiplication; no Mode 7 picture exists (BGMODE is only ever 1 or 3) |
| `OBSEL` | sprite sizes and tile base | 11 | nshvc_sprite.width/.height/.tiles, from the game's size table |
| `BG12NBA`, `BG34NBA` | layer tile base | 11 | nshvc_layer.tiles (a split: one tile set and a line scroll) |
| `nshvc_wait_until`, `nshvc_wait_vblank` | wait | 9 | return from update; the waiter becomes a state of its screen |
| `SLHV`, `OPVCT`, `STAT78` | the beam position | 7 | the debug time marks only: not converted (dead on retail) |
| `WMADDH`, `WMADDL`, `WMDATA` | work RAM port | 6 | plain copies in the game's own data |
| `SETINI` | overscan, interlace | 4 | nshvc_screen.overscan/.interlace |
| `MOSAIC` | mosaic | 2 | nshvc_layer.mosaic; by line NSHVC_LINE_MOSAIC |

## Data at SNES addresses

| Data at SNES addresses (the finder's "bus access" lines that are not registers) | Lines | Replaced by |
|---|---:|---|
| work RAM, through the data bank | 5136 | fields of the game's typed state |
| computed addresses | 1821 | pointers and indices into typed tables and records, one by one |
| the Super FX's RAM (bank $70) | 592 | fields of the game's typed state; the chip's work areas are locals and arguments |
| work RAM, long addresses | 412 | fields of the game's typed state |
| ROM data | 72 | typed tables from nshvc_asset_* |

## Register style

| Register style (not hardware; the same pass removes it) | Lines | audio | data | gsu | hud | include | input | math | menu | object | race | remaster | render | save | system | video |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| register file | 11715 | 216 | 18 | 610 | 358 | 38 | 21 | 180 | 1248 | 4541 | 1737 | 5 | 0 | 428 | 579 | 1736 |
| register-convention routine | 5997 | 506 | 12 | 305 | 131 | 3 | 11 | 88 | 646 | 1967 | 1045 | 10 | 281 | 123 | 335 | 534 |
| instruction helper | 3476 | 24 | 0 | 265 | 164 | 24 | 1 | 33 | 364 | 880 | 904 | 0 | 0 | 218 | 206 | 393 |
