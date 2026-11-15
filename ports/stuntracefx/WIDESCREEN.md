# Widescreen: the race out of its frame, any aspect

> **Superseded (2026-11-14).** The widescreen design for the earlier front end, with its Original mode beside it. Nothing
> builds from `ports/` now: the game is now `native/stuntracefx` on nativeshvc (the README's "The native game" section, and `docs/stuntracefx/native/PLAN.md`), which fills the window at any shape and has no Original mode.

A Remaster Option, as Boss Rally's Widescreen: **On**, a race's 3D fills the
window at any aspect, the bezel gone, the HUD anchored to the window's
edges over the 3D; **Off**, the original 256 x 224 at 4:3, exactly as now
(the gated mode). `RM_OPT_WIDESCREEN` in the existing Remaster Options
(runtime/rm_options.c, `remaster.cfg`, `RM_OPT_WIDESCREEN=N` for a check),
switchable while the game runs.

This replaces PLAN.md's phase 5 for the port as it now stands (the console
model under it, plain C paused).

## Game and console

Stunt Race FX is the game; **nativeshvc** (strutb/nativeshvc) is the
console, lifted out of `ports/common`. nativeshvc's rule: a faithful SNES
whose limits are lifted. An extension must be something the hardware could
have had, with its own defined meaning, usable by any game, and exact when
off. Nothing game-specific, no side paths. So the game draws its wide view
the console's way (Super FX PLOT into its RAM, DMA to VRAM, shown through
BG1) on a bigger machine, and places its own HUD.

| the game (this repo) | the console (nativeshvc) |
|---|---|
| the option; asking for the extensions on race screens, one player | a wider PPU screen, `nshvc_ext_screen(w, h)`, w 256-512, hardware column 0 at (w - 256) / 2; BGs, per-line HDMA state, colour maths across the extra columns; windows treat them as outside (accepted) |
| the wide view's words, focal length and cull | a bigger Super FX screen, `nshvc_ext_gsu_screen(w, h)`: PLOT/RPIX on full 16-bit coordinates, the character layout extended, more Super FX RAM (accepted, after the base console is verified) |
| its 3D bitmap's place in Super FX RAM, VRAM and the BG1 map (64 wide) | more VRAM, `nshvc_ext_vram` (accepted) |
| its HUD at the new edges: BG1 map cells and sprites | sprites past X 255: an extended X bit per sprite (accepted on request: the right-hand HUD needs it) |
| the pieces' positions (messages centred, corner pieces to corners) | host safe-area insets, `nshvc_safe_area` (accepted) |
| which layers to extend, which to drop | output at any size and aspect, sharp bilinear (there) |
| | the untimed race frame (nshvc_set_untimed, nshvc_wait_vblank; agreed with the 60 Hz session) |

Declined by nativeshvc, and dropped here: the shadow run on a copy of the
Super FX's RAM, span lists and GPU canvases (taken out of nativeshvc's
renderer), a canvas in place of BG1, and a compositor that places or
scales picture regions. Original mode is the extensions switched off.

## The race screen as it is

Read from the game's own setup and from frames of the plays (attract,
stunt_bluelake, speed_novice_coupe):

| what | where | how it is made |
|---|---|---|
| 3D view | 208 x 128 at about (23, 31) | BG1, mode 3 (8 bpp), the Super FX's screen buffer ($2C00 / $9400, $6800 bytes each) |
| view's rounded corners | the view's edges | window 1 by HDMA (channel 1, $7F:3410 to WH0/WH1) |
| sky | behind the view | colour 0/1 per line by HDMA (gradient.c: CGADD + CGDATA) |
| hills, horizon | behind the view | BG2 (backdrop.c), 32 x 64 map at $5000 (BG2SC $52): **256 pixels wide, wrapping** |
| bezel | around the view | BG1 tiles |
| panels: SPEED, DAMAGE, RANK, REST (Speed Trax); SPEED, DAMAGE, STARS (Stunt Trax) | lines ~168-208 | BG1 tiles below a BG1VOFS split (HDMA channel 2, $10AA) |
| panel numbers, gauges | in the panels | sprites (hud_setup.c, gauges.c, digits.c) |
| in-view HUD: big rank digit (top right), course map (left), BOOST gauge and lap time (bottom) | inside the view | Super FX pieces (bank 07 programs, render/hud.c) and sprites (map.c markers) |
| messages ("rising picture", glyph words, lap digits) | inside the view | Super FX pieces at fixed view positions |

HUD modes ($0D2B) 0-3 (RaceVideoSetup's table at $03:DA53, RaceHudLoad's
at $03:DD56); two players ($0D62) have their own setup
(RaceHudSetupTwoPlayers); see "Two players" below.

## Audit (step 1), done

**(a) The view's eight words.** ViewParamsByPlayers (7E:F6E9) sets them;
GsuParamsFromDp (03:AEB6) copies them to the Super FX:

| | one player | two players |
|---|---|---|
| X edges $70:0038 / $003A | 0, $CF | 0, $8F |
| Y edges $003C / $003E | 0, $80 | 0, $50 |
| centre $0034 / $0036 | $68, $40 | $48, $28 |
| X frustum plane $0092 | $4EC4 | $71C7 |
| Y frustum plane $0094 | $4000 | $6666 |

The projection's focal length is **128** (view units to pixels), on both
of its paths: FxRatioClamp gives (X / 2) * 256 / Z, and the far path's
table at $14:896C holds 2^23 / Z (read from the ROM: Z * entry / 2^16 =
128.0 to within 0.4 from $100 to $4400). The frustum planes are
z = x * s * 4 / 2^16 (FxClipXPlane, FMULT keeping the high word) and
z = y * s * 8 / 2^16 (FxClipYPlane), so the plane's tangent is half-width
/ 128, and

    X plane = floor(2^21 / half-width),  Y plane = floor(2^20 / half-height)

which gives all four values above exactly (2^21/104 = 20164.9 → $4EC4;
2^20/64 → $4000; 2^21/72 = 29127.1 → $71C7; 2^20/40 = 26214.4 → $6666).
The planes are signed 1.15 multipliers, so the half-width must be at least
64 and the half-height at least 32; a wider view only makes them smaller.

**(b) Angles and the cull.** The original's view is ±39.1° across and
±26.6° up and down. The sort's cull (render/sort.c, 01:AF80-AFBB) drops
an object when |X| - radius is not below |Z|: ±45°, which a view reaches
at 256 columns (half-width 128). So up to 256 wide (21:9) the cull never
removes anything in view; past it (32:9: 390 columns, ±56.7°) the wide run
must cull at its own half-width / 128 instead.

**(c) What gets drawn is not chosen by the view's direction.** The objects
drawn come from map buckets in a rectangle of cells around the view
(BucketTriggersFromView 7E:C520 picks the rectangle record by the view's
height, BucketTriggersFromArea 7E:C58D lists the cells, ObjectDrawListBuild
7E:EF50 writes at most $31 draw records). No heading test anywhere: a
wider view sees the same set. The same trigger list also activates the
objects (ObjectGroupsActivate 7E:F0E1), so it must never change; nor need
it.

**(d) Readers of the view words.** In the Super FX: the projection and
outcodes (project.c), the 2D clamp of clipped polygons (polygon.c 01:C447-
C467), the effects' bounds and centre (effects.c), text placement
(text.c). On the S-CPU: GsuPieceOffsetFromView (07:8219: half the right
and bottom edges into $0E82 / $0E84, game RAM, then a piece position) and
GsuPieceScaleFromView (07:825F: piece scale from the edges through the
multiplier). The other "$70:003x + X" reads in object/ are fields of a
Super FX record indexed by X, not the view.

**(e) The pieces.** Over a hundred S-CPU writes set a piece's position
($70:020A / $020C) for the bank 07 programs, most constants in the
original's 208 x 128 view (HudLapDigitsDraw at (48, 40), HudLeadWordDraw
(60, 72), GlyphWordAt1C28 (28, 40), BigSpritesNine, GlyphGrid1A (56, 48)
and so on). They are separate Super FX runs after the frame program, on
the same buffer. In the wide view the game moves them by changing those
positions under the option: messages by the centring offset, the corner
pieces (big rank digit, map, BOOST, lap time) to their corners. A table
names each race piece program and its anchor.

**(f) The backdrop wraps at 256.** BG2's map is 32 x 64 (256 wide), so
past 256 columns it repeats, as it already does as the view turns; the
wide backdrop is the same layer drawn across the extra columns at the
same scroll and scale (the focal length does not change).

## The design

### The wide picture

The whole race picture is one PPU screen, 224 lines high and as wide as
the window's aspect (7:6 pixels): 256 at 4:3, 341 at 16:9, 416 at an
iPhone's 2.17, 448 at 21:9, 512 (the extension's limit) from 2.67 up,
pillarboxed past that. The 3D fills all of it; the HUD stays at its
original pixel size, now 224 lines tall like the original's.

### The 3D

The view is the whole screen, so the focal length grows from 128 to fill
224 lines while the view still contains the original's field:

    f = min(224, width / 1.625)

| window | picture | focal | across | up/down | bitmap (8 bpp) | pixels vs original |
|---|---|---|---|---|---|---|
| 4:3 | 256 x 224 | 157.5 | ±39.1° | ±35.4° | 56 KB | 2.2x |
| 16:9 | 341 x 224 | 209.8 | ±39.1° | ±28.1° | 75 KB | 2.9x |
| 2.17 (iPhone) | 416 x 224 | 224 | ±42.9° | ±26.6° | 91 KB | 3.5x |
| 21:9 | 448 x 224 | 224 | ±45.0° | ±26.6° | 98 KB | 3.8x |
| 2.67 and up | 512 x 224 | 224 | ±48.8° | ±26.6° | 112 KB | 4.3x |

So the 3D gains up to 1.75x the original's density, with no separate
high-resolution path. In the game's C, under the option:

- the eight words for the wide view: edges at the picture's sides,
  centre at its middle, planes from (a)'s formula generalised to the
  focal length: X plane = floor(2^14 f / half-width), Y plane =
  floor(2^13 f / half-height) (f = 128 gives the original's);
- the focal length in the projection's two paths: FxRatioClamp's
  multiply by 256 becomes 2f, and the scale table at $14:896C (2^23 / Z)
  a wide table of 2^16 f / Z, built from the same rule;
- the sort's cull at the wide half-angle when it passes 45° (2.67 up);
- the bitmap at its wide size: Super FX RAM for two buffers, SCBR and
  the screen mode for the extended layout, the uploads' sizes and
  destinations, the BG1 map 64 wide;
- the pieces (bank 07 programs) at positions moved by the game: messages
  by the centring offset, the rank digit, map, BOOST and lap time to
  their corners.

### The 2D

- **Sky**: the gradient's per-line colours hold across the line (the
  console's extension does it); its lines rescaled to the new horizon.
- **Hills (BG2)**: the 256-wide map repeats across the width; its scroll
  scaled by f / 128 so it turns with the 3D, and placed at the new
  horizon. Its art stays at 1x, so the hills look smaller against the
  3D than in the original (mode 3 cannot scale a BG). Visible change.
- **Bezel and window**: off.
- **Panels**: the BG1 map's cells along the bottom take the panels'
  characters at the left and right edges (inside the safe area); the
  rest of BG1 is the 3D. The panels' rounded corners are colour 0 there,
  which shows BG2 behind rather than the 3D: four small notches per
  box, unless the Super FX plots those corner cells (a few cells a box).
- **Panel sprites**: moved with their panels; those on the right past
  X 255 use the extended X bit.
- **Menus, results**: no extension (256 x 224), pillarboxed.

### The cost, and pacing

The Super FX draws 2.2x to 4.3x the original's pixels, and the S-CPU
must move that much more bitmap to VRAM. The original uploads the race
screen a third per vertical blank ($2200 bytes, three blanks a screen).
At the hardware's rates a 416-wide screen would take about eleven blanks
to upload and several times the Super FX's frame time: a race at a few
frames a second. Two console extensions in the hardware's own shape would
cover it, but a faster modelled Super FX or DMA is ruled out (Jeff,
2026-10-26, to the 60 Hz session: no hacks or shortcuts).

**With Frame rate 60 on, none of this costs anything.** The 60 Hz
session's work (uncommitted) runs each race frame untimed: the Super FX
runs straight to its STOP and the finished screen is uploaded at once
(RmRaceScreenShow: the upload stages back to back), the simulation
stepping on its own ticks. A wide bitmap only takes more native time. So:

- Widescreen with Frame rate 60: no clock or DMA extension needed.
- Widescreen with the original frame rate: the original's pace comes
  from the Super FX's time for the original 208 x 128 view, which a
  single wide run never measures. Choices (Jeff's):
  1. Widescreen runs its races on the untimed frame at 60 Hz pacing
     whatever the Frame rate option says (simplest; honest about it).
  2. Untimed at the original rate, the game frame waiting the vblanks the
     console counts from the clocks the wide run states, scaled by the
     original's pixels over the wide's: close to the original's pace,
     not equal to it.
  The exact original pace stays with Widescreen Off.

The untimed race frame moves to nativeshvc (agreed between the 60 Hz
session and nativeshvc: nshvc_set_untimed, nshvc_untimed,
nshvc_wait_vblank). The 60 Hz upload loop (RmRaceScreenShow) caps at 8
stages; a wide bitmap needs 11 to 14, and that session will raise it.

## Steps

Each a commit; each verified before the next. (C) is nativeshvc's work.

1. Audit. **Done** (above).
2. The option: `RM_OPT_WIDESCREEN` in rm_options' table (shared with the
   60 Hz session), default On; a key to toggle. Check: same.py 0 with it
   Off.
3. (C) wide PPU screen, sprite X bit, VRAM; (C) extended Super FX screen.
   Check, game side: the extensions on at the original's size (256 x 224,
   208 x 128 view, focal 128) give the original's run exactly (same.py 0).
   **Game side so far (88c2c004):** the focal length is a parameter of the
   projection (FxRatioClamp, the scale-table multiplies) and the planes
   follow it, the 45 degree cull widens with it; checked with PORT_FOCAL
   on the original view (PORT_FOCALCHECK: the division at 128 equal to
   the original's on 431,812 calls; untimed, whole races end in identical
   work RAM at focal 96, 128 and 160). Waiting on the console's wider
   screens: edges, centre, the bitmap's size and place.
4. The wide 3D at focal 128: wide edges and planes, the bitmap wide, the
   uploads. Checks, from saved states, one frame each way: the wide
   bitmap's centre 208 x 128 against the original's, differing only on
   polygons the original clipped (counted, listed); nothing in view culled.
5. The focal length: FxRatioClamp's factor, the wide scale table. Check:
   every projected vertex against the exact f / 128 scaling of the focal
   128 run, within the rounding the table and division allow (computed
   per vertex).
6. The 2D: sky, hills, bezel and window off, panels and sprites, pieces.
   Checks: each placed panel's pixels against the original picture's;
   the hills' scroll against the 3D's horizon over a turn (a measured
   drift of 0 pixels at the centre).
7. The pace: with Frame rate 60, the wide race at a game frame each video
   frame (measured over the race plays at 16:9, 2.17, 21:9); at the
   original rate, per Jeff's choice above.
8. Hosts: macOS, Windows, iOS (safe areas, touch controls clear of the
   panels). One whole run: same.py with the option Off; wide plays at
   three aspects; the README's Remaster Options.

## Risks left

The pace at the original frame rate (Jeff's choice, above); 16-bit overflow in
the clip and fill with coordinates past 255 (step 4's checks); Super FX
RAM for two wide buffers beside the game's own records (where they go is
the game's choice once the RAM is larger); the hills looking smaller.

**The later high-resolution polygon mode** was planned on the GPU span
fill that nativeshvc has removed. With this design the extended Super FX
screen is the high-resolution path (focal up to 224 already gives 1.75x);
anything sharper means a bigger extended screen. Jeff's earlier rule that
the GPU draws both polygon modes no longer fits nativeshvc's rule; that
is Jeff's call.

## Decided here (say if any is wrong)

- Widescreen default On, as Boss Rally.
- The picture 224 lines at the window's aspect, up to 512 wide; the 3D
  fills it, containing the original's field; HUD at original pixel size.
- Bezel dropped; panels at the bottom corners in the BG1 map; in-view HUD
  pieces to their corners, messages centred.
- No faster modelled chips; wide races run untimed (choice 1 or 2 above
  is Jeff's); the original mode is the exact one.
- Menus and results at 4:3 for now. Two-player races: below.

## Two players

With Widescreen on, a two-player race fills the window too. `remaster/fullwindow.c`
(search `two`):

- **The split follows the window, each frame:** left and right once each column
  holds the original's 144 columns (a screen 288 wide, 3:2), else top and bottom.
  A black seam of 2 console pixels between.
- **The 3D:** one picture, each player's view words on its rectangle, focal length
  128 as one player's, so the view grows with the rectangle. `fw_clip` holds each
  player's fill and render pass to it. The chip's bitmap is 4 bits a pixel, palette 2:
  `fw_hi`, `fw_face_colour`; BG1 goes to Mode 3 so the canvas takes whole CGRAM entries.
- **The boxes:** DAMAGE, the map, the speed and TIME float with no bezel, along the
  rectangle's bottom (left and right) or down its right side (top and bottom), scaled
  together (`two_q`). Their sprites are drawn into their texture, since a console
  sprite cannot scale; BOOST and RANK stay sprites and pieces at the console's size.
- **Hills and sky:** BG2 has one scroll a line, which two views side by side
  cannot share, so both are drawn per player on the GPU: the hills as quads from BG2's
  map, the sky by `sky.comp` from each player's 80-line band, for both layouts.
- **Kept as they are:** the sprite wheels (the solids' colours are the 8-bit
  palette's); no water surface pass (one surface a picture); the pause and results
  screens the original's framed ones, as one player's.
- **Not per player:** the colour window and COLDATA are the console's, by line, so a
  crash's tint is not held to its own view when side by side.

`FW_WINDOW=w,h` sets the window's shape for a headless check.
