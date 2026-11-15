# Stunt Race FX on the native engine: conversion plan

Status: **the game plays; the conversion is not finished** (Progress below says what is not). Done on branch
`native-engine` (cut from `main` at c0c3b709), merged into `main`, where the work goes on. Engine: nativeshvc API 6.4 (the
plan was written against 4.4 and 4.5). Sections 1 to 10 were written from reading the source before anything was built; the
Progress table's numbers come from the runs since, each dated.

## Where it stands (2026-11-14)

What is not done, first:
- **Section 8 and stage 11 are not met.** The engine's hardware finder reports nothing, but the interim forms in rows 5, 7
  and "Race objects" remain: a Super FX record link is still its place in the chip's RAM, read and written by place (193
  `rec_get`/`rec_put` calls at 778c79f7), and other chip words are still read by address. No final audit has been run.
- **Platforms (row 11):** Windows and Linux are built by the builder (0.99); Windows was run headless only (it matched the
  Mac byte for byte), Linux not run. iOS and Android were run from logs only by this plan's runs; the iPhone build has since
  been played by hand.
- **The view from above** is one player only (two players and the course screen keep the game's camera); sprites' facing
  frames and the rival arrows are still the chase camera's.
- The 2D between updates has not been run on a display faster than 60 Hz (row "Race picture").

Landed since the rows below were last written (each in the history by its commit):
- **The land and water on every course** (stage 10's ground design; each course shown to Jeff): Easy Ride's mountains, King's
  Forest's instanced 3D trees (engine 6.2's `nshvc_draw_mesh`), Night Owl's and Night Cruise's city ground and Night Cruise's
  buildings, the oceans of Aqua Tunnel, Sea Breeze, Lake Side and Harbor City, Sunset Valley's and Big Ravine's rock, White
  Land's snow, Sky Ramp's sea of clouds, the battle and bonus courses; the stunt arenas keep their floors.
- **Depth-tested 3D** on the engine's depth settings (6.3), with exact corners and the cars seated on the track the game
  drives them on: [DEPTH.md](DEPTH.md).
- **The view from above** (`race/overhead.c`): 25 degrees down, fixed heading, the see-through about the player's car in
  quarter steps (engine 6.4's `nshvc_pixel`, `shaders/fade.shade`), a view button switching over 2 seconds, the choice saved.
- **Rain and snow** at the window's pixels across the whole view (23b97c5d); draw calls batched (8a15572e).
- **The pause screen** on the iPhone and iPad as a menu (resume, Auto-Accelerate, quit the race), and quitting a race through
  the game's own pause quit (132eef78).
- **Builder 0.92 to 0.99** builds this game (not the old front end) for the Mac, the iPhone and iPad, Windows and Linux
  (section 1, item 8, is settled).
- The bug round of 2026-11-14 (merges 35302838 to 3d487e2c): every car's eyes solid, the race's text kept together and
  eased, the start lamps at the window's edges, the course screen's wipe over the whole window, the ending's demos lit.

Not part of the game (Jeff, 2026-11-10): the art redrawn as geometry, pattern textures included, is shelved on branches
`shelf/hires-pictures` and `shelf/pattern-textures`.

How this meets the direction in one line: the game's own typed C calls `nshvc_*` directly from `update` and `draw`, with no
register-shaped state, translation layer or imitated hardware anywhere between them.

> **Read this first (Jeff, 2026-11-09, decision 4 and its repeat).** "Enhanced IS the native game. We are abandoning
> 'original' mode entirely. This isn't an emulation project." So, everywhere in this plan and in the code:
> - There is no original mode, and no option, flag or check-only path that runs the game the original's way: no recorded
>   pacing (`--pacing`), no exact fill rule, no picture held to the original's 256 x 224 screen, no work spent lining frames up
>   with bsnes or matching its timing.
> - The game's own logic (physics, objects, menus, rules, sound commands) is still the game's: a crash, a wrong value, an object
>   in the wrong place or a missing sound is a bug. Recordings of the original are kept only to find such bugs, never as a target.
> - Where the rest of this plan speaks of matching the original, lining frames up, edges against bsnes, `nshvc-check` frame
>   reports or recorded pacing, that part is superseded by this.

## Progress (updated as work lands)

Engine 6.1 (2026-11-11, nativeshvc 6271570, the new renderer; Jeff's request through the engine's session): the game
builds against it on macOS, iOS, Android, Windows and headless, shaders included; not run on any of them. What 6.0
changed for the game: a vertex's colour is 0xAARRGGBB, no longer a palette entry, so the shapes in one palette entry (the
3D's flat faces, the plotted rows, the solids' tyres) are drawn as that entry's texel of a 256 x 1 indexed image, through
the palette row by row as the textures are (`fx_draw.c`); every vertex is zeroed, so `z` and `w` are 0 and the lists draw
in their order as before; the shaders' prelude now declares a block `W`, so the shaders' row width is `IMAGE_W`;
`nshvc_picture_read` gives the window's pixels with the overlays, so the pause screen is the window's size filling it,
a pixel a pixel. The pause blur matches the earlier one (Jeff, 2026-11-11: "Match the old blur"): a Gaussian of 3 of the
screen's pixels, made once as the screen opens in three pictures with no overlays (the game's own, blurred across, blurred
down), each read back, the last kept and drawn as it is; the touch controls and the corner button are no longer in it.
Compiled on every host; not run.

The land as polygons (2026-11-11, Jeff: "clear solid polygons ... terrain matching the style of the track design",
"extend into the horizon point with no gaps"; track by track, Easy Ride first): Easy Ride's land is a plane at the
course's level out to within 0.01 px of the horizon in every direction, never above the eye, and a ring of 10
five-sided polygon mountains outside the course's footprint, each face shaded as the chip shades a lit face of a model
(class 10's shade row; the level's scale and sign fitted from the course's own lit faces), drawn past the 3D's depth 0
with their own depths, so they cover none of the course: 52 triangles, flat colours, no shader (c8b535bc). Compiled on
every host; Jeff looked at it in his own window (launched with `SRFX_COURSE=0`, 2449d64b): "Easy Ride is really good
now!". The other courses keep the land below until their turn; Aqua Tunnel's waves and partly see-through water not
started. For checking as the courses are done: `SRFX_COURSE=<course>` starts that course's race at once (logo, title,
menus and course screen left out, their choices made as data), and an FPS counter (31d5ad46) shows the pictures drawn a
second at the top left (`SRFX_FPS=0` hides it); neither run by me.

Where a picture's time goes (2026-11-11, measured; `SRFX_PROFILE=1`, ad4be3e7): Easy Ride's start line headless on
the Mac, 89 pictures a second at 5120 x 2880, 124 at 2560 x 1440, 137 at 1280 x 720. The game's draw and the engine's
build are about 0.9 ms a picture at every size (the far objects 0.55 of it); the render thread is 85-90%: about 6 ms
that does not change with the window's size (about 4,500 draw calls a picture, one 3D face a call, about 1.3 us each)
and about 0.28 ms a megapixel more. Whether the fixed part is encoding or the GPU is not told from inside the game. The
full breakdown is in the notes folder (perf-2026-11-11-breakdown.md). Jeff's windowed 51 FPS at 5K was taken with the
course launch's sound loop spinning the audio thread (fixed, 16484a5f) and is not yet measured again.

The land (2026-11-11, from Jeff's pictures of Easy Ride; a scratch run dumped the courses' height grids and the eye's
places, compared offline): (1) the land showed above a tunnel's opening: the grid keeps the lowest place a shape meets the
ground, so it has a trench where the road runs under the ground, which the 24 planes (9a513326), flat over it as the
ground over a tunnel is, do not; the eye was tested against the grid, so in the tunnel it counted as above the ground and
the planes, above it, were drawn from below. The eye is now tested against the land as drawn: on Easy Ride's demo the land
is left out at exactly its four tunnel samples. (2) The land beside the road was the sea's teal: its colour for a depth
took the eye as 180 over the ground (the eye over the road), but Easy Ride's road runs high over its ground (the eye a
median 529 over it; 770 on the night city), so land by the road took the colours of rows near the horizon. It now takes
the eye's height over the course's level (the grid's median height), so flat ground has the band's colour of its own row,
as the original shows it. Compiled on every host; not run since the change.

Not done (2026-11-09, evening): perspective textures, the hills, weather and particles between steps; section 8 in the Super FX code (a rewrite, under way) and the chip-RAM reads that wait on it; a won championship's closing screen not reached (four races won by input); the controllers' rebinding not run with a controller; Windows and Linux not run (other machines, Jeff's permission); the picture, sound and touch on iOS and Android not looked at (3-minute runs from logs only); the final section 8 audit and stage 11. The game plays headless from boot through the menus, races and battles to the results and back, as the Enhanced game (decisions 4 and 5): full window at any shape, high-resolution 3D drawn between steps, solids, unlimited view distance, the game's own pause screen. New C outside `src/fx`: about 39,900 lines; `src/fx`: 44,300 (the Super FX logic from the generator and its drawing on the GPU).

| Stage | State |
|---|---|
| 0 Set up | Done: `native/stuntracefx` builds and opens empty on the engine |
| 1 Record and replay | Picture done: `sfxbox --native`, core patch 0014, `native/checks/replay` and `compare.py`. On the attract play 5 of 5 recorded frames match bsnes, one of them a race frame. Not yet run: two-player, more than one play. Voice replay through the engine not written; the sequencer is checked directly instead |
| 3 Sound | Written; the routine map with the dead ones is `audio/ROUTINES.md`. Driver, with the original's pacing, on three recordings (attract, race, battle): all 11,897 key-ons and every voice's pitch and volumes at every pass match (`native/checks/music_check`). Loads: the recordings' sound memory byte for byte. Through the engine's mixer (`music_check/mix`): the original's own register writes correlate 0.99 to 1.0 with bsnes; the driver's 0.4 to 0.99, the rest being when within a pass each write lands. The game's own run of the attract play (title, demo, title, demo, 105 s) against its recording: the same sound throughout at a steady offset (the game reaches the title 1.8 s sooner, 3.4 s after the first demo, drifting 0.1% as the known difference says), loudness correlation 0.68 to 0.94 per 4 s window, waveform 0.06 to 0.88. The noise sounds and the second course set, by the code (2026-11-09): sound $67 is chained by sound $37's script ($EC to $BCC9), which the game sends as $3C each time a Stunt Trax score marker is hit (bonus markers 2 and 5, courses $0C-$0F); $4D is $52 from course $03's lap crowd gate, on the player's first hit in a lap it has not counted; $09 is $0E from course $07's posts (their sky balls) and course $0A's cannon gate (laps 0 and 2) and wave gates; the second course set only in the ending after the last class (BootSoftRestart). $67 reached by input (the Stunt Trax play to Blue Lake, course $0F, the accelerator held): it starts on channel 3, keys voice 3 on with source $23's variant $96, 10 ticks later its second note on sample $0A, 48 ticks after that its $E0 ends it and the voice goes back, as its script says. Not reached: $4D and $09 need the player to drive into those objects on later laps, and the second set needs the Master class won; the courses were reached by input (the controller 2 course code) but no recording drives a lap by input (the championship plays poke the lap count), and 25 s of straight driving reaches none of them. Race at the steady step (decision 4), the menus' race walk, 11,500 updates: every port-0 command the game sends reaches the driver in order (227 of 227, 2.2 frames typical, 10.5 at most, with the sends now every frame instead of the original's odd frames); the countdown ($49 three times 54 frames apart, $4A, $01) and the finish ($31, $51) come out as in race1. Fixed on the way: the engine bytes now come from the race's own fields, and a command sent twice running is no longer heard once (the original's echo could not tell a fresh 0 from a stale one; 2 of 227 merged). The time bonus's command queues as any other (the original's decimal-mode slot is gone). The engine follows the speed: with the race's gauge fix (574371ef) the speed byte runs 0-252 over a race and the engine pitch follows it (correlation 0.89, the rest the driver's own smoothing). Three port-1 commands dropped in that race are the driver's own rule (19:CE82: $09 and $0D ignored while channel 5 plays $17 or $21). |
| 2 Game state | Under way, a screen at a time: the menu objects (`menu/actors.c`), the frame's input, brightness and sound sides (`system/frame.c`, `audio/sound.c`) |
| 4 Boot to title | Logo and title run on the engine with no VRAM imitation, the objects and the per-line tables ported. Against bsnes: 6 logo frames and 19 title frames (fade in, the drop, the idle screen) exact in 5-bit colour. The original is 60 frames slower to reach the logo (sound upload) and about 127 frames to reach the title (picture unpacking on the Super FX while blanked); the native game does that work in one update. Leaving the title, the attract demos and the menus since done (rows 6 and 7) |
| 5 Super FX logic | Under way. `src/fx` (41,700 lines) runs recorded jobs: `native/checks/fx_check` over five plays leaves RAM the same apart from register spills and each job's last store, which the recording's STOP mark catches unfinished. On the GPU through draw lists with the exact fill rule: polygons, lines, object sprites as textured quads, particles, HUD glyphs and pictures as image quads, scaled HUD sprites, weather as points. Measured by `native/checks/fx_view` on 1,100 frames of a race with its HUD against the original's bitmap: mean 989 of 26,624 pixels apart (3.7%) in the picture, the differences in the median frame being polygon edges (accepted, decision 3). Known differences: the static noise is rows from a square of the original's generator's numbers at seed-chosen places, not the original's exact pixels. The game's own frame draws through this since (row "Race 3D"; the exact rule since removed, decision 5). Not done: mirrored sprites, the unreached glyph sizes and weather are written from the source and not measured Section 8 in fx/object, fx/race and fx/math (the sound session, design under stage 5): 464 of 13,400 lines and 6 of 105 functions converted (fx_math.c, e4866230; FxAngleVector and FxTurn in fx_rotate.c); each change checked native against native over five recordings, every job leaving the chip's RAM the same apart from the stack (69,851 jobs in the kept recordings; the check shown to catch a planted error). FxTurn is reached by none of them (it runs only for a collision model turned off the quarter turns), so its arithmetic is checked by reading only; it reads its tables in fx_table_bank where its caller chooses no bank, as the chip did. |
| 7 Race (S-CPU side) | Interim. Every routine of `race/`, `hud/`, `math/`, `input/`, `system/` and `gsu/` (outside `fx_*.c`) is converted or listed in `race/NOTES` (folded, replaced by the engine by design, dead with the original's proof, or owed by another session). Pacing (stage 8, decisions 4 and 5): a step every 6 updates in every mode, its work told 6 frames; no recorded pacing (17e97ac4). Runs headless with no crash or warning: the attract demos (6,400 updates) and a race from the menus to game over, the results and back to the menus (11,500 updates). Bugs found with the recordings and fixed: the demo text's bank, the first step's camera at work RAM $0000 (with the objects' `camera_at_link`), fields named twice ($06CF, $0E37, $0F2B; $18D3 waits on the menus), the race's layers left on behind the menus (now handed back through `race_screen_end`). The battle traced (notes: race-battle-trace): its parting at frame 2676 is when the original reads the buttons within its step (not a logic bug); one logic bug found on the way, a car part's break-off handler given to the part instead of the new piece (objects, a81bdbbd). With the original's button reads as input and its seed (through the menu exit's timing, input only), a scratch run of the battle with the recorded pacing had the random numbers, both cars, their records and the whole object list equal to the recording at all 898 two-player steps. The speed gauges read the car records again (574371ef; the port number had been passed). Stage 10's race side (574371ef): the steps' clock, `race_step_progress`, `RaceViewForPicture`; the fx session's `fx_tween_step_end` and `fx_tween_reset` in `step_end` and the clock's restarts (5e877dfc); the full window's view words (35718c30). Checked at HEAD 362244a6 against a copy with the pictures between steps off: the menus' race walk (11,500 updates) and the attract demos (6,400) give byte-identical sound and run with no crash or message, and with them on the race's picture changes every update (a step holds it six). Touch auto-accelerate (e8920e96) not run on a touch device. Section 8 in these directories is not done; section 8.2 says "under any name", and two kinds still break it, both partly done. The Super FX's inputs and results as words in its RAM: the HUD's pieces are done (c9f2e377: the 18 glyph, banner and sprite programs in `fx/render/hud.c` take an `FxPiece`, checked frame-identical on the attract demos and the race walk); 75 uses are left (`system/chip.c` 31, `race/race_frame.c` 19, the rest in `race/`, `hud/hud_setup.c` and `chip.h`) with 33 address constants in `system/chip.h`, waiting on the fx session's next groups (the math programs, then the frame inputs, which also take the scaled sprites' interim view-edge reads); `fx/render/hud.c`'s noise fill is plain C too (b310ce3b; FxNoiseFillScreen dead), its two tile converters not converted (they write chip RAM images video uploads). The cartridge's data by bank and address: the plain tables are done (ce626eb4: 65 of the 69 read lines through `data/race_tables`, by the ruling in section 3, checked the same way); the race load's packed art blocks are done too (94961072, `RaceBlock`); so are the HUD's glyphs and pictures, named by what they show (376bf405: the words as text, the time frame's pieces by place; ec88cb1a: `fx/render/hud.c`'s banks and banner from `data/`). Video's `OamFromList` reads its lists through `data/race_tables` (video's e66bfa16), the course scripts are references from `data/` (objects' f976df93), and the results' packed pictures go through `data/race_tables` (6ea24fad): `race/`, `hud/`, `math/`, `system/` and `pause/` hold no cartridge read, bank or table address. Left on that side: the two shade bases in `system/chip.h` (cartridge addresses written to a chip word, so with the chip's RAM), and fx's row hand-offs `fx_glyph_row` and `fx_glyph_scaled_row`, which `fx/render/hud.c` feeds from `data/` but which still take a bank and an address (fx's interface). The sound session's sweep, section8-sweep-race.md, lists every place. Met: the hardware finder reports nothing; no carry passed (the decimal sums as values, 889fc601, 9bb77f35, each byte-identical in sound against its parent on the race walk and the demos); 59 routines and the fields named by what they do, the dead fields gone with the original's proof of no reader; no address as a bare number; the save's records read by its fields (6e9ed206). The pause screen's AUTO-ACCELERATE switch (b51eee0c) and a controller's buttons rebound and saved (4113aeec, engine 4.6), compiled on every host, not run (a controller's rebinding needs a live run with one connected) |
| 6 Menus and saves | Interim: the title's exit, loader, selection, settings, initials and erase-save screens, the course preview flow, the championship screen, the records pages, the two-player screen's set-up and the save file with its checks are converted (`menu/`, `save/`). Walked since by input only: the two-player (Battle Trax) menus against bsnes, matching but for timing (the slide, the car camera's swing, the granite's and highlight's animation phase); the championship's closing screen, which a game over in a championship reaches (its boxes' course names fixed, f5536bbb; one line at the boxes' bottom edge differed: every per-line table's entries were placed a line early, the original writing a table's first entry during line 0 so that it shows from line 1, as the championship and title recordings show; fixed in 7dc572a3; checked by input on the game-over walk: the closing screen with its boxes now equals bsnes's pixel for pixel on every compared frame of the open panel, the closed panel's frames differing only in the granite's animation phase; the menus' other screens, whose effects moved by the same line, not run again). The won championship's closing screen, reached as Jeff directed by setting the data (a scratch build writes the player car's lap count every 50 updates while racing, as the championship recording's play does; the four Novice races and the bonus in about 15,000 updates): against bsnes on the same play (shots every 4 frames through the screen) it now matches but for the times' digits (the races ended at other moments) and the granite's animation phase during the panel's rise. Found and fixed on the way (feb22183): the course menu's mask channel, left marked on from before a race, cleared the closing screen's sprite mask, so the four times showed at once instead of being revealed by the bands' opening; the course menu's opening had the same order. The game-over walk not run again since. Found with the recordings and fixed: the fade's fixed colour (the original's queue keeps the first one queued in a frame), the sprite art rows queued to the frame's end as the original's VRAM queue (after a race the fourth choice read FREE TRAX), the records pages' sprite slots in the shared words $0D71-$0D7B (the records panel's colours). Against bsnes, 5-bit colour, outside the car window: title, exit and loader exact; erase-save exact on every shot; `menus_settings` 657 of 720 shots exact, 52 under 300 pixels; the menu walk to the car selection exact; the course screen's records panel exact, the rest of that page being the course screen's backdrop (video). The car window: the car's pose and colours match; it now shows the newest picture (decision 5), one step ahead of the original. After a race the car window also shows the race frame's HUD boxes, drawn into every composed picture (`fx_draw_picture`, reported to its owners). The slide from the game selection to the car selection runs 2-4 frames ahead for about 40 frames, cause not found Stage 10: the menus fill the window at any shape (ad628fd1, menu_window.c): each screen's background goes on past the original picture, the course menu's granite through a map built at the window's size, the other layers and sprites kept to the picture, the windows moved by the origin; seen at 16:9, 21:9 and 9:16 on the title, selection, car selection with the car, after a race and after a demo. Panels slide in at the picture's edge, not the window's. Typing the initials on a keyboard (32bb46cd, engine 4.5) and the auto-accelerate option kept in the save (050c477a; the race applies it, e8920e96): compiled, not run on a keyboard or touch device; the options' switch waits on the pause screen. Section 8 in menu/ and save/: the finder reports nothing; pad bits by name; the per-line tables, cartridge tables, pictures, palettes and sound sets named once; the panel words named and two unread words gone. The cartridge's data by the ruling in section 3 (0b343804 to e53238fc): every table, picture, tile block, palette and sound set the menus and the save read, and the objects' block with its groups and per-line tables, is placed once in `data/menu_tables.c` as the image's offset and read through typed accessors; the objects' handlers are a `MenuHandler` enum the data's words resolve to; the per-line tables' runs name the menus' words through `data/`; `menu/` and `save/` hold no bank, address constant or cartridge read. Compiled only; not run. Two of those tables video also reads (the sprite groups' counts, the course pictures' places) and the sound uploads (called from `data/` until the sound session settles a reference type) are told to their owners. |
| 8 to 10 | 8 Pacing: done, a step every 6 updates (row 7). 9 Input and options: the engine's native inputs (4.5) in; the pause screen the game's own (`src/pause`: Escape, Start or the corner button; the picture behind blurred by a shader, 0cf4e86b); typing the initials and auto-accelerate compiled, not run on a keyboard or touch device. 10 Enhanced: see the menus, video and 3D rows; unlimited view distance done (56ea23c6); the ground and water built (stage 10, design in the 3D section) and shown to Jeff on every course |
| 11 Platforms | macOS (Metal): runs headless through the menus and races. iOS Simulator and Android emulator (arm64): built and run 3 minutes each through the title and attract race with no crash or error, judged from logs and the game's state; picture, sound and touch not looked at. Windows (Vulkan, cross-built with mingw) not run, Linux not built: both need another machine, which needs Jeff's permission. Since (2026-11-14): the builder builds Windows (Vulkan) and Linux (x86_64 and arm64, glibc 2.35, built on a Linux machine); Windows run headless only, matching the Mac; Linux not run; the iPhone build played by hand |
| Race objects | Interim: run only inside the race session's runs: the attract demos, the menu walk to a race and back, and the recorded battle, whose objects equalled the recording at all 898 two-player steps in a scratch run. A one-player race is not compared past its first step. `native/stuntracefx/src/object`: 9,900 lines, every live routine of the 752 in `src/stuntracefx/object` converted or among the 228 dead listed with proof in `object/NOTES`; handlers found by number in `object/handler_table.c` (376 cases). Section 8 audit (2026-11-09): `nshvc-find-hardware` reports nothing; no work RAM addresses; the course script dispatches on its command byte, not the jump table's routine addresses; the Super FX RAM words, the ROM tables read through and the trigger areas are named by meaning; the shared fields once named by address are named (1e610f7e). The earlier remaster's Enhanced calls (44 sites) settled against stage 10 in `object/NOTES`: most are whole steps' (places and angles set between by fx_tween; frames, colours and counts a step at a time as in the original). The far view's rules are in `object/animate.c` and `draw_list.c`: nothing drawn far before its set-up has run, stopped short lives left out, decorations kept running, race objects' looks going on, parked cars' tyres. So are the gaze between steps and cars kept models however far. The looks and gazes between steps go by the 3D session's `fx_tween_t`; circling pieces keep to their circle between steps and parked cars' tyres draw among the near objects (011ff881). Compiled, not run on their own. The values the code hands on are named too: models by their number in the cartridge's model table (28-byte records from $A040), looks, object scripts, the car scripts (F-Type, Coupe, 4WD and 2WD in the menu's order, their rivals, the bonus truck, told apart by their wheels in the ROM), the records' Super FX handlers and the camera paths as offsets into the path table. The Object record is a union of views by kind (`object/views.h`, 2026-11-09): all 178 handler groups' code on 39 views, 88 per-kind fields named by what their kind keeps there (bytes a kind writes and never reads named `unread`, those no routine touches `unused`), the record's offsets kept and asserted per view, and its common fields named by what the chip reads; routines once named by addresses, offsets or values named by what they do. Checked: builds of HEAD with and without the views gave the same objects, cameras, chip RAM and game state at every step end of the menus' race walk (1,723) and the battle input (1,621). Not worked out from the code, and said so where it is named: what the chip does with `draw_flags` bit 1; the emitters' second effect byte, named by the chip's argument. The Super FX record's fields are one table in fx.h, settled with the 3D session, and the objects' 58 raw offsets use it (d04d903a). Section 3's data rule: the course scripts are passed as an opaque `CartRef` (`data/ref.h`, f976df93; race's two calls switched with it), and every cartridge table object/ read goes through `data/object_tables` (eb96f35f): object/ holds no cartridge read and no table address. The named references, by the conversion session's ruling (8.2: addresses under any name): the 32 named models and the 28 raw model numbers (the crash sets' and the ring's parts), 21 looks, 3 object scripts and 9 car scripts are ids by meaning that `data/object_tables` resolves to the words the records keep (b2d9aebb); the 248 handler names and the dispatch's 375 cases are `ObjectHandler` ids that `data/object_handlers` resolves both ways, `handler_table.c` dispatching on the id (4570d625). Records keep the original's words. Checked: builds of HEAD with and without b2d9aebb and 4570d625 (both with the record handlers' 3a791048, which the chip side now uses) gave the same objects, cameras, chip RAM and game state at every step end of the menus' race walk (1,723) and the battle input (1,621), with no handler reported unconverted. The Super FX record handlers are the record pool's `FxHandler` in fx.h, agreed with the sound session, with their words in `data/fx_handlers`; the record makers give one (the chip side's own uses switch with their files). Not done: the S-CPU objects' reads of the chip's RAM by address (53 when stage 5's design counted them) wait on `FxShared` (stage 5, step 5). Interim, named: a record link (+$22) is the record's place in the chip's RAM, read and written by place (247 calls of `rec_get`/`rec_put` in 8 files) until stage 5's step 5 makes it an `FxRecord *` outside the original bytes (the Object grows, by the coordinator's ruling); 38 other calls read or write chip RAM words by address (7 files). The render share (fx's split, the coordinator's yes, 2026-11-09): `fx/render/sort.c` and `fx/render/fx_points.c`, 2 of 2 functions and 412 of 412 generated lines, plain C to section 8 (152 and 76 lines now). The sort turns each draw record into view space, culls it by its model's radius (fx's wide-view sides kept) and links it by depth; `FxModelPoints` takes the script as a `CartRef` and the model's frame (sound's two calls and fx_check's entry switched with it). Both read the cartridge through `data/object_tables` (the model's radius and depth bias, the point script); neither keeps a stack, cursor, ROM bank or result register (sound's chip-state analysis: no later job reads the bank the sort left). Interim, named: the draw records at $0AAA, their count (`GSU_RECORDS_N`), the list's head at $023C, the view's matrix ($00E4) and centre and edges ($0034-$003A) stay in the chip's RAM in the original's layout until fx's piece (c) removes the queue: 6 places in `sort.c`, read and written through two helpers; the point table at $2488 likewise until sound's `FxCollideModel` takes it into `FxStateBlock`: 1 place in `fx_points.c`. Checked: fx_check over the five kept recordings (attract, speed_novice_coupe, battle, stunt_bluelake, testrun; 63,038 jobs, 7,263 of them the sort) leaves the chip's RAM after every job the same as HEAD's apart from the stack and the picture; against the recordings the sort is 0 jobs apart as before, and the physics jobs come closer (the dropped spills). Planted errors are caught: a depth key + 1 in all 7,263 sort jobs, unmirrored points in 68-367 physics jobs a play |
| Race picture (video) | Interim. Not done: the 2D between updates (32ccbe1b, b6529299, whole frames of the update before) not run on a display faster than 60 Hz, nor checked on the battle or the course screen; the section 8 sweep's video items (section8-sweep-video.md) not yet worked; the course screen's stray row of dots at the original's line 135 not looked at; side by side, the second player's colours $E1-$E3 can't change mid-line and are unchecked; Aqua Tunnel's ripple cells for the 3D's water come only in a one-player race (offered for two players); with the screen wider than 340, the HUD's in-view sprites are not kept to the HUD's box. Done: the full window at any size and shape for the one-player race, two players (side by side from 288 across, above and below otherwise) and the course screen, the HUD anchored inside the safe area and the bezel gone, checked at several shapes against scripts (boxes exact, hills and sky the same at every shape); two players' start lamps left out as the frame covered them (f117897e); the uploads by named destinations, no VRAM or sprite-table addresses (11233afd, c5d475f2) and video's routines named by what they do (f7de3387), frames identical before and after; the ground's colour by row and the ripple cells for the 3D's surface (12ea9b6a, matched video's rows on 4.7 million rows). Earlier: every routine of `src/stuntracefx/video` (without `unused_screen.c`) has a function or is listed in `video/NOTES`; the two-player battle runs from the menus to the race's end, its bugs against the battle recording fixed. The race starts 61 frames sooner than the original's because the original's loads take 64 blank frames the game does in one update. |
| Race 3D (fx, stage 10) | Interim: one-player race and course menu done and seen in runs; not done: the hills of a picture between steps (video reading `fx_tween_view`; they still move per step), weather drops and particles move per step, perspective textures (textured faces are cut by depth instead), the course menu's in-between pictures not looked at, the full window's ground beyond the original's view cone (the line-by-line sky shows where no course polygon is: object triggers or video's ground colour), solids (F). Since then: the sort's side cull follows the view's width (it was the chip's fixed 45 degrees, 128 pixels a side: in a 16:9 race it now keeps 1-6% more objects, those at the wide edges); a paused step's picture stays the last step's; objects made in a step are drawn where they stand (the objects' slot counts); no exact rule, chip-screen picture or switch back to them remains (decision 5). Seen in a race from the menus at 1280 x 720 (16:9): the 3D fills the window, drawn at the window's pixels with the smooth rule, the panel boxes and HUD anchored, and the 3D changes at every update (1,800-7,000 pixels an update, none frozen). Checked on the recorded race at 4 times (step A): every on-screen vertex within the chip's own rounding (0 of 32,894 past it; a planted centre + 2: 32,891 past). Done: A-D on the fx side (`fx_face.c`, `fx_tween.c`, `fx_draw.c`) Solids: tyres, the bonus trucks' wheels and trees written (fx/fx_solid.c, 2ba1ab24) and shown to Jeff side by side with the sprites; on in the game since he approved the look (2026-11-09); the 4WD's eyes on too (the objects' gaze per draw record), approved from the pictures. A sprite's quads are kept to its own texels (thin lines beside magnified sprites, found in the tree pictures). Two players in the full window on (with video's layout and race's words): seen in a battle at 16:9 side by side and 4:3 above and below, each player's 3D in its rectangle; their 4-bit pictures keep sprite tyres. |

Known differences so far, for the final report: sound $23's noise sample is rewritten while it plays in the original; here it is one long noise made once, at the same rates, because samples do not change on the audio thread. The driver runs one pass per timer period instead of the original's variable passes, so it never loses the music tick a pass of 8 or more periods loses at this tempo; the title song is about 0.1% quicker than on the console (some 2,400 samples after a minute).

## How the conversion is shared between sessions

Several sessions convert different areas at once, all in this worktree and on this branch. The rules every one of them follows:

**The goal and its bar.** Section 8 is the definition of done. The game's C calls `nshvc_*` directly. No bridge, no imitated PPU, DMA, DSP, APU, bus or clock, and no `cpu.`, `abi_`, carried registers, flag helpers, computed goto or SNES addresses in code. Real parameters and return values. `tools/nshvc-find-hardware` must end at zero and that alone is not enough. Dead routines (section 1 item 7) are not converted; list them.

**The source.** Read the C in `src/stuntracefx` (the proven record, never edited) and `asm/stuntracefx` where the C is unclear. Write new C in `native/stuntracefx/src`. Do not copy register-style code across and tidy it later: write what it means. `native/stuntracefx/src/menu/actors.c`, `menu/screens.c` and `audio/driver_music.c` are examples of the form.

**The frame.** Each `update` is one video frame of the original. A blocking loop becomes an explicit state machine; a wait becomes returning from `update`. Hardware writes queued for the vertical blank become changes to the fields of `pic` (`video/picture.h`) or object writes (`nshvc_tiles_write`, `nshvc_map_write_snes`). HDMA tables become `nshvc_lines` calls.

**The state.** Shared variables live in `src/system/state.h` as fields of `g`, under the section of the code that mostly writes them, each with its original address in a comment. Search that file for the address before adding a field. Use the Edit tool with a small exact match, never rewrite the whole file, because others are editing it too. Variables only one file uses stay `static` in that file. Super FX RAM is `fx_ram`, named through the `FX...` constants in `src/fx/fx.h`.

**Cross-area calls.** A converted routine keeps its original name (`ObjectsRunHandlers`) with real parameters, declared in its area's header (`object/object.h`, `race/race.h`, `hud/hud.h`, `video/video.h`, `menu/menu.h`, `save/save.h`, `audio/sound.h`). If you need a routine from another area and its prototype is not there yet, add the prototype you need under a `/* wanted */` comment in that header; its owner implements it or corrects it. An unconverted routine is never replaced by a silent stub. If you must leave one, make it print its name once to stderr and list it in your report.

**Your files.** Each session owns the directories named in its brief. Do not edit another session's files apart from the two cases above. Save a file only when it compiles: other sessions build the same tree. Commit often with `git commit -- <your paths>`, never `git add -A` or a bare commit, because the index is shared. Never push. Commit messages and comments describe the work and nothing else.

**Building.** Use absolute paths, the shell's working folder does not persist. Check your own files with `clang -std=gnu11 -Wall -I native/stuntracefx/src -I <nativeshvc>/include -c FILE -o /dev/null`, where `<nativeshvc>` is `/Users/jeffreywilbur/projects/strutb/nativeshvc`. For a full build use your own build folder so that sessions do not collide: `cmake -S native/stuntracefx -B <your scratch folder>/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DNSHVC_ROOT=<nativeshvc> -DNSHVC_HOST=macos -DNSHVC_RENDER=metal && cmake --build <your scratch folder>/build`.

**Checks.** `build/stuntracefx/native` holds `sfxbox` and the core. The recordings of the original made so far are kept in the main checkout at `/Users/jeffreywilbur/projects/strutb/stuntracefx/recordings-irreplaceable/` (attract, battle, champ, fx, logo, menus_course_grid, speed_novice_coupe, stunt_bluelake, testrun, title; the only copies, excluded from git locally); `/private/tmp` and `build/` are cleared by disk clean-ups, so keep new ones there too. `native/checks/record.sh PLAY OUT` records a play of the original (plays in `tools/stuntracefx/plays`); `sfxbox --script FILE` with your own `wait N` / `shot NAME` script records any frames you want. `native/checks/compare.py BSNES_DIR SHOTS_DIR OFFSET` compares pictures exactly. The game takes `--headless --updates N --shots DIR --input FILE`. Do not change `nativeshvc`; if the engine seems wrong, get the evidence and tell the session that started you.

**Reporting.** Lead with what is not done. Never call a stepping stone finished. Update your row of the Progress table when something lands.

## 1. What this plan cannot promise

Read these first.

1. **This is a rewrite of about 120,000 live lines, plus 12,447 of the game's own features, not an engine hookup.** Of the finder's
   32,613 lines, about 1,900 touch real S-CPU-side hardware. The rest are register-style code or data at SNES addresses: `cpu.`
   11,715, `rd8`/`wr8` on data 8,033, `abi_` 5,997, `op_` 3,476, the SPC700's 1,640. zelda3 was already ordinary C; this game is not.
2. **The finder sees only part of it.** It does not look for the Super FX's register state (`R[`, `gsu.`: 21,137 uses), the
   SPC700's (`spc.`: 6,691) or `goto` over translated assembly (3,573). A finder result of 0 is required and not sufficient.
   Section 8 defines done.
3. **Polygon edges will not all match bsnes.** `NSHVC_FILL_EXACT` rounds vertices to whole pixels and lets the GPU decide
   coverage, so every GPU fills the same spans (`src/engine/picture.c:507`). The chip walks each edge with an 8.8 slope from a
   reciprocal table in ROM, with its own rounding and clamping (`render/draw_list.c:131`). Those are different rules. Expected
   result: flat areas and interiors identical, edges off by up to a pixel on some rows, most on long shallow edges. The number is
   not known until it is measured. Generating the chip's spans on the CPU and drawing them row by row would match exactly, but
   it is the CPU deciding pixels, so this plan does not do it. Matching byte for byte would need a general edge rule in the
   engine, which is your call (decision 3).
4. **Races will not line up with bsnes in free play.** The original's step took 4 to 9 video frames depending on the work. With
   the game's own pacing the steps differ, so the race's frames are comparable only under the check's recorded pacing
   (section 6).
5. **Sound will be compared by voice events first, and sample for sample only where it aligns.** The music code runs on its own
   thread in the engine's time; zelda3's music never aligned with its ROM.
6. **It cannot be seen working until most of it exists.** Stage 4 is the first end-to-end run (boot, title, sound). A race needs
   stages 2 to 7.
7. **Dead code is not converted.** Routines the census proves never run in retail stay in the record and are listed, not
   rewritten: `render/unused.c`, `video/unused_screen.c`, `object/fx_unused.c`, the debug panels and the like (about 4,700 lines
   in those files). `FxHazeSample` (`render/effects.c:197`) reads real pixels with RPIX; the census found no play reaching it.
   Stage 5 settles from the assembly whether it can run. If it can, I stop and tell you, because a GPU picture cannot be read back
   for game logic.
8. **The Builder app is not converted by this plan.** `builder/` packages the old front end and stops working on this branch once
   the front end is deleted. It needs its own decision after the game builds. (Settled: Builder 0.92, 2026-11-12, builds the
   native game; 0.99 adds Linux.)

## 2. What is there

`src/stuntracefx`: 143,219 lines, 149 C files, 2,402 tagged routines.

| Area | Lines | State today |
|---|---:|---|
| `object` | 36,847 | S-CPU register style, plus Super FX physics translated with `goto` |
| `audio` | 18,936 | two SPC700 drivers, 105 and 70 routines, translated instruction by instruction, 17,240 lines |
| `render`, `gsu` | 21,713 | Super FX programs in register style; a typed draw-list fill exists |
| `race` | 13,721 | S-CPU register style |
| `remaster` | 12,447 | newer C on the old console's extensions, including a CPU span generator (`fullwindow.c`, 6,988) |
| `video`, `system`, `hud`, `input` | 20,140 | registers, DMA, HDMA, interrupt stages |
| `menu`, `save` | 13,358 | register style; loops that wait |
| `math`, `data`, `include` | 6,057 | helpers, the hardware headers |

Blocking: 276 functions wait for a vertical blank, directly or through a callee (106 menu, 89 race, 36 system, 17 video, 11 audio,
7 object, 7 hud, 3 chip). Every one becomes a state of its screen. 124 further sites use `call_routine`, `jump_routine`,
restarts or pushed frames.

The shelved branches hold usable work, and this plan starts from them where it can (decision 1):

| Branch | Holds | Use |
|---|---|---|
| `native-port` (d82f1cd6) | Work RAM named from the assembly: `wram_struct.h`, 1,438 fields; record field names; `tools/wramname.py`; some truly typed functions | Field names. Its 2,734 "carried register" functions are not acceptable as a finished form and are not reused as such |
| `fx3-merge`, `fx3-phys` | Hand-typed Super FX: `fx_unused`, unpack, HUD, effects, frame, `fx_sums` | Reading and reuse, re-proved here |
| `fxgen-w` | Generated Super FX C: 0 gotos, 0 register references, data-flow names | Approved as the starting text for the Super FX logic (decision 1) |

The full hardware list is in [`hardware-touches.md`](hardware-touches.md), with every site in `hardware-touches-sites.txt`.
The real hardware touches:

| Hardware | Sites | Replaced by |
|---|---:|---|
| DMA channels and start | 388 | object writes when the data changes |
| HDMA | 76 | `nshvc_lines` in `draw` |
| VRAM, palette and sprite ports, sprite sizes | 325 | `nshvc_tiles_write`, `nshvc_map_write_snes`, `nshvc_palette_write`, `nshvc_sprites_set` |
| Interrupts and waits | 127 | work in `update` and `draw` |
| Layers, windows, colour maths, mode, brightness | 558 | `nshvc_layer_set`, `nshvc_windows_set`, `nshvc_blend_set`, `nshvc_mode_set`, `nshvc_screen_set`, per line `nshvc_lines` |
| Calls by address, the console connector | 115 | direct C calls; `nshvc_game` for the entry |
| Super FX registers | 92 | a C call of the program's function |
| Sound ports | 65 | `nshvc_music_send` |
| Joypads | 56 | `nshvc_input` |
| Multiplier, divider, M7A/M7B as a multiplier | 99 | ordinary C. The game shows no Mode 7 picture: `BGMODE` is only ever 1 or 3 |
| Beam position, work RAM port | 13 | the beam reads are debug time marks, not converted; the port becomes plain copies |

## 3. The converted game

The new tree is `native/stuntracefx`, with the same area names as the record. `src/stuntracefx` and `asm/stuntracefx` are not
touched on any branch, so the record survives a merge. (`docs/stuntracefx/PLAN.md` rule 20, "no separate copy", is superseded by
your brief for this work only.) Each function keeps its `@routine` tag and its `WHAT IT DOES`.

**Loop.** `nshvc_game` with `update_hz` 60. `update` does what one vertical blank's worth of the original did: the interrupt
stages' work (frame counters, pads, sound sends, fades, windows, palettes), then the current screen's next state. `draw` sets the
whole picture from the game's state. There are no threads, fibres or coroutines imitating a wait: the 276 waiting functions become
explicit states.

**State.** One typed `struct` for work RAM, with records as records (objects, actors, cameras). Stack and direct-page scratch
become locals. The Super FX's RAM splits into shared fields, which go in the game state, and the chip's scratch, which becomes
locals and arguments. No field is at an SNES address and none holds a register's byte.

**Picture.** Screen setup says which picture kind this is; each kind holds its own tile sets and maps as engine objects. `draw`
describes the layers from named game fields. This is deliberately not a register shadow: there is no `bg12nba`, and no function
that takes such state and works out calls. DMA queues become object writes where they are queued, the one numeric part being data
tables that name destinations, which one function maps to objects (as zelda3's did). Sprite builders write `nshvc_sprite` entries
rather than an OAM image. HDMA tables become per-line arrays passed to `nshvc_lines`.

The split screen changes BG2's map and tile base at a line (`system/raster.c`, `split_bg2`), and `nshvc_line_field` has no such
field. The conversion puts both halves in one map and one tile set and uses a per-line scroll, which the API allows. No engine
change is expected; stage 1's replay confirms it.

**Super FX logic.** The chip's compute programs (physics, matrices, sorting, unpacking) become ordinary functions run where the
S-CPU started them. Its drawing feeds `nshvc_draw_list`s:

| Drawn by the chip | Becomes |
|---|---|
| Flat polygons, 3 to 8 vertices | `nshvc_draw_tris` under `NSHVC_FILL_EXACT`, fanned, palette index as colour |
| Lines | `nshvc_draw_lines` |
| Textured polygons | triangles on an indexed `nshvc_image` from the ROM's texels |
| Scaled sprites, HUD art, glyphs | `nshvc_draw_image` from the game's art as images |
| Particles, weather, single plots | one-pixel quads |
| Dither (the chip's plot option) | a small shader in the engine's language, the game's own |

The chip's bitmap, its tile layout and the staged uploads of it do not exist in the new game. No pixel is made on the CPU; a
picture becomes visible at the update on which the original's last upload stage would have completed.

**Sound.** `nshvc_music.tick` at 32,040 Hz counts samples and steps the driver at its timer's rate, the method zelda3 uses. The
driver is structured C written from the record: sequencer, voice state, vibrato, tremolo, slides, sound effects. It calls
`nshvc_voice_*`, `nshvc_echo` and `nshvc_sound_*`. It never waits, locks, allocates or does file work. The game thread loads a
bank's samples with `nshvc_sample_load`, then sends a message saying it is ready; commands go by `nshvc_music_send`. Both drivers
convert; they share code only where reading shows the behaviour is the same.

**Data.** A tool reads the player's own ROM at build time, verified by checksum, and writes the game's data (not its code) to one
asset, read through `nshvc_asset_*`. The game's tables, models and scripts refer to each other by bank and address in their own
format, so that data stays in that format. How the game's C reaches it (ruled 2026-11-09, under section 8.2: a named address is still
an SNES address under another name, so `cart8/16(BANK, NAMED_TABLE + i)` does not pass):
- **Where tables live:** each table's place in the cartridge image is written once, in `data/` only, as the asset's index (as in
  `assets.csv`).
- **What game code sees:** each table is typed data with its own accessor or array, named for what it holds, for example a struct
  array filled once at load, or `uint8_t course_lap_count(int course)`. Outside `data/` the game's C holds no bank, no address
  constant and no `cart8`/`cart16` call.
- **Data that points to other data** by bank and address in its own format (scripts, models, lists) is passed as an opaque reference
  value that `data/` resolves. Such a reference is data, not a constant in code.

`ports/stuntracefx/assets.csv` (2,052 ranges) is the starting inventory.

**Saves and input.** `save/records.c`'s data as the game's own struct through `nshvc_save_*`, written where the SRAM was written.
`nshvc_input` replaces the joypad code. The keyboard, touch and controller code in `ports/stuntracefx/front` goes, since the engine
provides it. No per-platform code in the game.

**Pacing.** See stage 8. The race sim already takes "video frames since the last step" as an input (`$0306` into `$06D4`,
`race/race_frame.c:54`), so its pace is that one number.

## 4. Stages

Each stage deletes what it replaces from the new tree in the same change. Compiling is allowed throughout. Every run is listed in
section 7 and asked for first.

### Stage 0. Set up
New tree, `CMakeLists.txt` with `nshvc_game()`, an empty game that opens. **Delete:** nothing. **Check:** compiles.

### Stage 1. Record and replay (section 6)
The tooling section 10 of the engine plan requires, before any conversion, because it tests the API's fit while a misfit is still
cheap. **Check:** the replay matches bsnes on the frames in section 6, and is shown able to fail by planting a change.

### Stage 2. Data and state
Asset extraction; typed tables; the game-state struct with records; the math helpers as plain functions (`math`, 2,595 lines).
**Delete:** `machine.h`, `cpu_ops.h`, `wram.h`, `hw.h` are never brought over. **Check:** every table's extraction against the ROM
bytes it came from; compile.

### Stage 3. Sound (18,936 lines)
Both drivers as music code; the sample and bank loaders; the game's sound queue. It goes early because it is self-contained and can
be proved without the rest of the game. **Delete:** `spc.h`, `spc_ops.h`, `apu_upload.c`, every `spc_` access. **Check:** the
recorded port commands fed to the driver give the recorded voice events, tick by tick, on the race and battle drivers. Expected: the
same events and values, with differences only in the order of writes inside one driver step.

### Stage 4. Loop, video, boot to title (about 20,000 lines; first end-to-end)
`update`, `draw`, interrupt stages as ordered work, HDMA to `nshvc_lines`, DMA to object writes, boot to title and attract's start.
**Delete:** `waits.h`, `console_ext.h`, NMI and IRQ handlers, every DMA and PPU access. **Check:** `nshvc-check` picture and sound on
the boot and title frames. Expected: byte identical on frames with no chip picture.

### Stage 5. Super FX logic (about 30,000 lines; the highest risk)
`gsu/`, `render/`, `object/fx_*`, `gsu_record`: compute programs to functions, drawing to draw lists. The 3D frame program, physics
and `render/model.c` are the parts that stalled last time. **Delete:** `gsu.h`, `gsu_ops.h`, the fill in `draw_list.c`,
`ports/common/snes/superfx.c`'s role. **Check:** each compute program against the recording of its job (inputs at start, outputs at
STOP), and each frame's polygons against the recorded draw list. Expected: compute results identical; polygons identical in vertices,
colours and order.

#### The Super FX state (section 3's split, for section 8)

Agreed with the 3D session (fx.h, the drawing side) on 2026-11-09, with objects (the record links, `FxHandler`) and race
(`FxFrameInputs`). The types are in fx.h: `FxRecord`, `FxPool`, `FxShared` with race's frame inputs as its `frame`
member, and the aggregate `FxStateBlock fx_state` (landed by the sound session with the 3D session's yes; the name
`FxState` was taken by the chip's registers in fx_rt.h).

Today the chip's 64 KB of RAM is one byte array that both sides read by address: 256 named addresses (72 also read by the
S-CPU side), 64 record offsets and 129 literal addresses in `src/fx`. Section 3 splits it into **shared state**, kept as
named fields, and **the chip's scratch**, which becomes locals and arguments. No field is at an SNES address, none holds a
register's byte, and nothing in the game reads the chip's RAM by address. `fx.h` (the 3D session's) holds the types.

**1. Records.** One `FxRecord` struct with the fields fx and objects have named (`fx-answer-to-objects-record-names.md`):
the all-records links, pool, size, handler, model offsets, flags, frame, script, list head and next, mode, place, heading,
pitch, roll, mass, contact flags, velocity, turn rates, the car's and the part's own fields. Kinds, from the record makers:

| Kind | Bytes in the original | Pool | Made by |
|---|---|---|---|
| car | $98 | first | `FxCarNew`, from a car script in bank 9 |
| car part | $52 | first | `FxCarAddPart` |
| falling part, falling, trigger, camera follower | $34 | first | the record makers, modes $0424, $0426, $042D, $042E |
| still | $2A | second | mode $0422, counted, on the still list |

Every link that holds a record's RAM address today becomes an `FxRecord *`: the all-records list (+$04, +$06, head at
$2332), the still list (head $2398, +$26), a record's contacts (+$18 head; `FxContact`, the $12-byte blocks of the records it touches, +$0E next), a car's parts (+$3E, +$34), the
next car (+$74), the partner (+$8E), the player and view records ($2476-$247A), the S-CPU objects' record links. A test
that a word "is a record" by its address range becomes a null test.

**2. The pool keeps its accounting.** The record pool is the original's memory manager: first fit over two byte pools,
splitting a larger block and merging neighbours (`FxPoolTake`, `FxPoolsMerge`, `FxListSweep`, `FxObjectFree`). When no
block fits, the record is not made, and that is game behaviour. So `FxPool` keeps each pool's free blocks as places and
sizes and decides as the original does. A record's place is the allocator's bookkeeping and the check writer's, nothing
else; the game reaches records by pointer. The structs live in a fixed array; a slot is taken with the block and given
back with it.

**3. Shared state.** One `FxShared` struct (in fx.h, an instance in the game state) holds every word that outlives a job
or is read by the S-CPU side: the race timers ($2470, $2472), the physics busy flag, the car class's physics parameters
($2652-$2686), steering ($2662-$266A), the near car ($2642-$264C), traffic, effects, haze and weather, the text's place,
the drive level, the track table, the far-object list, the new car's slots, the counts. 72 named words are read by the
S-CPU side today (38 in $0000-$023F, the job inputs `FXIN_*` among them; 22 in $2330-$2BFF; 6 job parameters at $0AAA-
$108E; 6 more); which of the other 184 outlive a job is measured, not guessed (step 6). Measured on five recordings (attract, a novice race, battle, a Stunt Trax course, Test Run): of the named words
90 shared, 124 scratch, 25 not reached; the list is `fx-ram-split-measured.tsv` in the notes folder. An array a job partly
rewrites can look shared when a job reads a slot first, so arrays are read in the code before they are made fields.

**3a. One aggregate, one copy.** `FxStateBlock` holds `FxShared`, the fixed `FxRecord` array and `FxPool`'s bookkeeping
as one block (`fx_state`), never allocated, so a record pointer stays valid when the pictures between steps (`fx_tween`) and
the far list (`fx_far`) copy the chip's state whole and put it back. The per-frame words race proposed as `FxFrameInputs`
(the view, the records, the shade base) are a part of `FxShared`, not a second struct. The far records the draw list
writes into bank 1 (the far list and the view's world place) become a far-list array in `FxShared`, filled by the S-CPU
side and read by `fx_far`.

**4. The chip's scratch becomes locals and arguments.** The drawing's own scratch is the render share (the 3D session's)
and needs no field: the frame program's points ($0340, $04E4), facing flags ($07C6), queue ($08AA), slots ($0AAA) and
stack ($0240); the float hooks that read some of them by address change with that conversion. The stack at $0240 and every push, pull and spill; R14 and the
ROM buffer (a ROM table read becomes a read of that named table); the RAM address register (`fx_sbk` stores to the field it
last read); the bank registers; the per-job temporaries in $0000-$023F and $2330-$27FF that no later job reads; the
generated `rom_at`, `stack`, `*_scratch`, `caller_scratch_back` and `up` parameters. A job's inputs that the S-CPU writes
just before starting it (`FXIN_*`) become the job function's arguments, its outputs the function's result. A return past
several callers becomes each caller returning.

**5. The S-CPU side.** Its reads of chip RAM by address (hud 138, race 62, system 37, object 53, menu 5, video 5, sound 1)
move to the same fields and record pointers, each area switching its own. Until then an adapter may answer the old reads,
named interim in its own comment and in the row with the count of callers left on it; it goes at 0.

**6. Checks.** `fx_check` stays the check, with a check-only writer that lays `FxShared` and each live record (at its pool
place) back out as the original image, so the comparison with the recordings is unchanged. Before converting, one run of an
instrumented `fx_check` (a scratch copy of the RAM accessors, not the game's) records, per job, each word read before it
is written and each word written that a later job or the S-CPU reads: that decides the shared/scratch split of step 3.
After: `fx_check` over the five plays and the battle recording's 898 steps, one run each when a share is done.

**7. Order.** This design agreed (fx owns fx.h and the drawing side; objects the S-CPU object code); the instrumented run
and the split's list added here; `FxRecord`, `FxPool` and `FxShared` in fx.h; then the shares file by file (sound:
fx/object, fx/race, fx/math, 13,400 lines and 105 functions), each row counting lines and functions converted.

#### The render share in native C (the fx session's design, 2026-11-09)

How it meets the direction: the CPU walks each model's script and builds its faces, sprites and particles as values; fx_face
and fx_draw clip and project them and hand them to the GPU, which fills every pixel; no pixel is made on the CPU and no part of
the chip's program is imitated.

**The files.** `fx/render/frame.c`, `model.c`, `polygon.c`, `raster.c`, `project.c`, `points.c` and `effects.c` (only the
drawing calls the last three), and matrix.c's polygon clips (FxPolygonClipNear/Left/Right/Top/Bottom, FxSegmentCrossZ; matrix.c
is race's): about 10,200 live generated lines, 12,100 with the dead. Dead with the original's proof (nothing in the original jumps or branches to them; only fx_check's entry table
calls them): FxWeatherUnused, FxWeatherUnusedLoop, FxWeatherUnusedClear, FxWeatherUnusedFull, FxEffectsAddUnused,
FxModelProjectEach and FxModelFacesGatheredUV, listed in `fx/NOTES`. (FxModelDrawFlags8000's note says only an uncalled
program reaches it, but FxModelDraw falls into it whenever $01B4's low byte is 0: it is live, and so is FxModelDrawWithFlags.) Not reached by any recording but reachable, so converted from the code: the model commands $08,
$10, $34, $4C, $50, $54, the textured screen clip and FxModelFacesOnScreenUV.

**Today.** The objects' sort fills the slots at $0AAA; FxFrameProgram walks them (FxObjectsDraw), picking each model's level of
detail by depth from its record in the cartridge; model.c interprets the model's byte script command by command, turning its
points into $0340 and projecting them into $04E4 in fixed point; faces are gathered, clipped to the screen in fixed point and
queued at $08AA (polygon.c, raster.c, project.c, points.c, matrix.c's clips); FxDrawList walks the queue and draws each entry as
the float shape fx_face took from the same points (the chip's own screen corners when it took none).

**Native form.**
1. Models through `data/`: a model's record (its script, points, levels of detail, material table, detail) read as a reference
   and a byte cursor over the script, no bank or address. The interpreter is one loop with a switch on the command byte; the
   script's calls and returns are an explicit stack of cursors, the chip's two (R10's and $0056's) kept where their order
   matters.
2. Points: turned by the object's matrix in the chip's integer arithmetic (the same numbers as now) into a local array in view
   space; no tables at $0340 or $04E4 and no fixed-point projection.
3. Facing: from the points in floats (fx_face_facing, as now), the flags a local array the script's branches read.
4. Faces: each face's corners (its points) and material go to fx_face as values, which clips, projects and keeps the shape in
   drawing order. The queue, its walk, the chip's screen corners and its fixed-point clips go. Materials (shade rows by tier and
   light, textures) as values from `data/`; a texture's key a data reference, not a bank and address.
5. Object sprites, weather and effects: functions taking their records (FxRecord pointers and FxShared fields as they land).
6. FxFrameProgram with real inputs: the ground, the far pass, the objects, the parked tyres, weather, effects and the notice
   text in order; the generated stack, `rom_at` and `up` parameters gone.
7. fx_face, fx_far, fx_solid, fx_tween and fx_ground read these structures instead of the chip's RAM.

**Check.** Each piece lands with the game's pictures compared frame by frame against the build before it, on the attract
demos, the menus' race walk and the battle input (one and two players); identical, or every difference counted and explained
before it lands. fx_check's frame-program entries go with the queue (their RAM is drawing scratch); it keeps the compute jobs.
The limit, said before building: in piece c fx_face decides each face's visibility in floats where the chip's fixed-point
clip and queue decided it, so faces at the screen's edges and the chip's overflow guards can differ; those are counted and
shown, not called identical.

**Order.** (a) The dead routines out with their proof. (b) The interpreter and the points as C, the queue still feeding
FxDrawList (pictures identical). (c) Faces straight to fx_face; the queue, its walk, the fixed-point clips, project.c's vertex
projection and points.c gone. (d) Materials and textures through `data/`. (e) Sprites, weather, effects. (f) The frame
program's inputs and the hooks' reads. Each lands on its own with its check.

### Stage 6. Menus and saves (13,358 lines)
Menus, course preview, records, the 3D car display, saves. **Check:** `nshvc-check` on the menu plays. Expected: frames with no
polygons byte identical; 3D frames differ on polygon edges only (section 1, item 3).

### Stage 7. Race and objects (about 40,000 lines; the largest)
`race/`, the rest of `object/`, HUD logic. **Check:** the game state compared with the recording at each race step under the recorded
pacing (section 6); then `nshvc-check` on race plays. Expected: state identical at every step; pictures differ on polygon edges only.

### Stage 8. Pacing (decision 4)
The game is processor-speed independent. The race runs one whole step of the original's code every 6 updates at 60 updates a
second, with the step's frame count given as 6 (the approved whole-step design of 2026-10-28), so the race's speed in real time
is fixed and there is no slowdown. Every picture between two steps is drawn from the state between them (`draw(between)` and
the updates in between), so the 3D moves at the display's rate. The recordings' medians (6 one player, 7 two players, 5 demos)
are no longer a target. **Check:** steady 60 updates a second in every mode; distance per real second the same in every run.

### Stage 9. Input, options and platform glue
**Delete:** `ports/stuntracefx/front`, `ports/stuntracefx/runtime`, `ports/common`.

### Stage 10. The Enhanced game (decisions 2 and 4)
This is the game now, not an option beside an original mode. Matching the original's pictures and pace is no longer the bar; the
game's own logic (its physics, objects, menus, rules) stays the game's, and recordings are kept only to find bugs.
- The race fills the window at any size and shape, the view's width following the window's aspect; the HUD and panels float at
  the screen's edges and corners; the sky and hills reach any width; view distance unlimited.
- High-resolution 3D: the frame's shapes projected at the output's resolution (the view's words scaled, sub-pixel vertices) and
  filled by the GPU with the smooth rule. The exact rule and the 256 x 224 Super FX screen are not kept.
- Frame rate: whole steps every 6 updates, the 3D drawn between them from interpolated cameras and objects, and pictures between
  updates on displays faster than 60 Hz. The engine (4.5) does this: each backend shows a picture a refresh (Metal's
  `nextDrawable`, Vulkan's FIFO, D3D12's `Present(1)`) and the game is drawn once a picture with `between`, the part of the
  update's time gone. The race's 3D and its hills take it (`race_step_progress`); the 2D is drawn between the last two updates
  too (`video/between.h`, 32ccbe1b, b6529299): a picture is one moment, the update before's whole frame (its art and maps,
  digits, colours and switches; the game's writes during an update land at the next) with what moves eased towards the
  newest (the sprites, the layers' scrolls, the windows, the per-line edges and scrolls); two players' hills and sky between
  steps follow the 3D's angles. Headless pictures (between 0) are whole frames of the update before: checked on the menu walk
  (results pages frame for frame). The menu screens join when the menus switch to the whole-frame hook. Not run on a display
  faster than 60 Hz.
- Menus in the whole window.
- Unlimited view distance: nothing culled or faded out by distance where the original cut it off for the chip's sake
  (earlier work: 218f69f1).
- The ground as a real surface: each course's colour-band ground and water replaced by a generated ground or water plane
  with its own polygons, textures and layers, the track drawn over it, water seen from above and overhead when under it
  (earlier work, merged on main then: 8931a216..c68f1373, af4032ea, be60bbb9; `remaster/fullwindow.c`'s surface table;
  Aqua Tunnel's sea level -1280).
- Solid shapes for the sprites that show solid things: the cars' tyres, the 4WD's eyes, the bonus trucks' wheels, the trees
  (faceted, as approved); effects stay sprites. No filtered or upscaled art.
- Simple pattern textures as geometry (Jeff, 2026-11-09: "create high resolution textures of simple pattern textures like
  this yellow/black border just like we created 3d models for tires and eyes"): a textured face whose texture is a simple
  pattern is drawn as the pattern's own shapes, flat polygons laid on the face in 3D, sharp at any resolution. Design below
  ("Pattern textures as geometry"); first texture: the yellow and black stripes. Shelved (Jeff, 2026-11-10): out of the
  game, kept on branch `shelf/pattern-textures`.
- The pause screen with the controller panel and key remapping on every host, the options (auto-accelerate), and typing the
  initials on a keyboard. In (`src/pause`): the panel, a keyboard's keys rebound and saved, the screen opened by Escape,
  Select + Start (a touch screen's own controls) or the corner button, moved through with any pad, and the AUTO-ACCELERATE
  switch (b51eee0c); a controller's buttons rebound and saved with engine 4.6's `nshvc_control_bind_controller`, DEFAULTS
  putting the keys and buttons back (4113aeec; compiled on every host, not run: a controller's input reaches only a live
  run with one connected). A touch screen's controls are the engine's own, with nothing to rebind.
`fullwindow.c`'s CPU span generator is not brought over; its decisions (view words, anchors, sky) are the reference.

#### Stage 10's 3D (the fx session's design, 2026-11-09)

How it meets the direction in one line: the CPU makes vertices (geometry) and lists them; the GPU fills every pixel with the
smooth rule at the window's resolution; no span, row or pixel is made on the CPU.

Facts it rests on, from reading: the engine renders a draw list into a target of the list's own size and samples it across the
whole screen (`rr_metal.m` draw_lists, `picture_body.h` kind 3), so the 3D's resolution is the size the list is cleared at;
`nshvc_vertex` is 2D (x, y, u, v, colour), so projection stays on the CPU and texture mapping is affine per triangle unless a
shader is given the depth. Every polygon and line the race draws is queued by one routine, `FxModelFaces` (01:90CD), from the
model's projected point table ($04E4 + 6i), which `FxModelProject` (01:8BAA) makes from view-space points ($0340 + 6i plus the
object's place at $0026-$002A, summed in 16 bits) with focal length 128 (`FxRatioClamp`: R1 * 256 / R3 of the halved pair; the
far table at $14:896C gives the same). The chip holds a point's offset at $3FFF, clamps far depths at $43FE and clips at z = 0
and at the view's edges in 2D, at whole pixels.

**Where the precision comes from.** The frame program goes on deciding everything at the console's scale: which objects
(the sort and its frustum), level of detail, facing, painter's order, materials, which faces are queued. Only positions are made
again, in floats, from the same view-space points:
- Faces. `FxModelProject` keeps each point's view-space sum as floats beside its table; `FxModelFaces` notes each face's queue
  entry, its point indices and its u, v. A face is committed when the list's end has moved past its entry (settled at the next
  face and at `FxDrawList`); its shape is then made from its points: clipped in view space against a near plane, projected
  x = cx + 128 X / Z, y = cy + 128 Y / Z unrounded and unheld, clipped in 2D against the screen with a margin (the GPU's guard
  band), and a textured face cut by depth planes until each piece's far over near depth is at most 1.25 (affine pieces; the
  original cut once, at 3). `FxDrawList` draws that shape in place of the chip's entries from the face's first entry to its
  last (the chip's near split makes two). The chip's held, clamped and edge-clipped vertices are not used. This is
  `fullwindow.c`'s `project_exact` done once per face in floats; neither a 16-bit list nor view words scaled by k are needed for
  the precision.
- Scaled sprites (`FxObjectSprite`): centre and size from `FxProjectScaled`'s inputs in floats; the chip's row quads mapped
  from its whole-pixel box onto that one.
- Particles, weather, labels and the HUD's chip-drawn pieces: the console's pixels, scaled with nearest texels (not sub-pixel
  until step E).

**Resolution.** The list is cleared at the window pixels the screen covers (`nshvc_display_info` and the screen's shape; 4 times
the screen when no window is known, as in a check), rule smooth. Recordings keep vertices in the screen's pixels as floats;
composing scales them. Quads put u, v at their corners for nearest texels: no filtered or upscaled art.

**The window's shape (the full window).** The view words come from the screen, not the original's window: centre across at
W / 2, down at the original's horizon row (screen line 96) plus half of any rows a taller screen adds; edges the whole screen
(each half for two players); the frustum constants $92 = 128 * 2^14 / half-width and $94 = 128 * 2^13 / the larger
half-height, as the original's are made (0x4EC4, 0x4000 for 208 x 128). The sort's cull and the frame program's clips then cover
the window. The words stay 16-bit (W up to 16,383). The race sets `g.view_*` from `fx_view_words()` at its set-up and when the
screen's shape changes. The HUD's chip-drawn pieces are placed by the video session's anchors.

**In-between pictures.** At each step's end the positions and angles are kept: the camera records (+08 point, +0E angles), the
objects (+0C place, +14 angles), the Super FX records (the list at $2332: +1E place, +24 angles, the car parts). A picture at
draw(between) is t = (updates since the step + between) / 6 of the way (the course menu's steps: / 4): the whole state is saved
(`g`, `objects`, `cameras`, `fx_ram`, `fxs`), the camera, objects and records set t of the way from the last step's end to this
one's (angles the short way; a field that moved more than $800 in a step, or an object made in the step, keeps the end's), the
picture's part of a step run (`ViewFromRecord`, `ObjectDrawListBuild`, `GsuRecordsUpload`, `ViewUpdateAndGsuAEF8`,
`BackdropAnglesFromView`, `GsuParamsUpdate`, `FxFrameProgram` into a third recording), the backdrop's scrolls for that picture
handed to the video session, and the state put back. Nothing the game keeps changes, so the redraw's weather drops, particles
and seed move per step (a known limit). The step's end shows a step late, as the original's picture showed after its step's
frames.

**Needed from other sessions.**
- Race: a call of `fx_tween_step_end()` at the end of `RaceFrameRun`, `RaceFrameRunTwoPlayers` and `RaceFrameStep` after the
  picture; `race_step_progress(float between)` (0 at a step, 1 at the next); that the calls above are a picture's whole part of
  a step and touch nothing but `g`, `objects`, `cameras` and `fx_ram`; `g.view_*` from `fx_view_words()`.
- Objects: that `ObjectDrawListBuild` and `GsuRecordsUpload` change nothing but `g.frame_records` and its count; how to tell an
  object made or freed during a step.
- Video: the screen's size and where the 3D's picture origin goes; `race_frame_art_draw` scaled to the list
  (`fx_draw_list_scale`); in the full window, anchors for the chip-drawn HUD pieces; the backdrop scroll of an in-between
  picture taken from `fx_tween_backdrop()`.

**Steps, each with its check (fx_view on the recorded race, 1,100 frames, unless said).**
- A. Sub-pixel picture in today's layout: faces and sprites in floats, the list at the output's pixels, smooth rule. Checks:
  every vertex of a face whose points all lay on the screen within 1.02 pixels of the chip's (the chip floors or ceils by sign,
  so 0 apart is expected; a planted centre + 2 must fail it); the picture drawn 4 times larger and read at each console pixel's
  centre against the exact rule's picture of the same frames: a pixel apart only where the exact picture has a pixel of another
  colour among its 8 neighbours (the shapes sit at their unfloored places, on average half a pixel right and down of the chip's
  floored vertices, so edges move by up to a pixel and no further), except inside textured faces, whose texels now follow the
  depth cuts; the count against the original's bitmap is reported, not predicted; game shots at 4 times for looking.
  Result (2026-11-09, fx_view on the recorded race, 184 pictures): geometry 0 of 32,894 on-screen vertices past the chip's own
  error (59 more than 1.02 pixels apart: the chip's far-depth table is truncated and indexed by the depth rounded down to even,
  and a near point's pair is halved, so the bound is 1 + |x| / 65536 + 128 |x| / z^2 for a far point and 1 + 128 / z for a near
  one, both read from the chip's code and its table); planted centre + 2: 32,891 past. Picture: 4.67% of pixels apart from the
  exact rule's, of which 2,999 in all (16 a picture) have no pixel of another colour among their 8 neighbours, so the criterion
  above is not met as written; in the two worst pictures every cluster is one of: the view's top row and last column (floored
  vertices clipped to the border; the original fills column 207, the exact rule leaves it empty), posts thinner than a pixel that
  the exact rule drops, sprite sizes the chip floors (21 against 21.7), and the chip's clipped vertices on long shallow edges (up to
  1.44 pixels). Against the original's bitmap: 3.92% apart (the exact rule 2.79%).
- B. The full window: view words from the screen, the 3D over the whole screen, HUD pieces anchored. Needs video and race.
- C. In-between pictures. Needs race, objects and video. Check: a race run's pictures between two steps lie between the two
  steps' pictures (vertices of the same face moving monotonically), and the state after a run with in-between pictures equals one
  without.
- D. The course menu's 3D: A's precision in its window, C at its 4-update steps.
- E. Perspective-correct textures (a game shader given the face's 1/z plane, u/z and v/z as vertex u, v), angle fractions for
  turning objects between steps, particles, weather and labels sub-pixel, facing from the 3D determinant if slivers show.
- F. Solid tyres, eyes and trees (`remaster/solids.c`).
- G. The view distance (218f69f1 ported, 56ea23c6): the draw list writes a far record for every other object it would draw, in
  every map cell, into the chip's RAM's second 64 KB (`fx/fx_far.h`); the frame program draws them first, farthest first,
  turned and culled as the sort does, each halved until its place fits 14 bits with its matrix and sprite size, its level of
  detail and colour tier from its true depth; the chip's first 64 KB put back after. Kept as the original has them: the levels
  of detail and the four colour tiers (they change a model's detail and colour, they leave nothing out).
- H. The ground and water as a surface (design below).

#### The ground and water (design, 2026-11-09)

What carries over from the earlier work (be60bbb9, 8931a216..c68f1373, af4032ea; `remaster/fullwindow.c`): each course's surface
by its number ($0E3D, `g.course`): none (Sky Ramp), land or water by its band's ground colours, at the height its scenery is
planted at (Aqua Tunnel's sea level -1280); none in Aqua Tunnel's sealed tunnel (bit 3 of $0DA5 set by its dark gates); the
looks: water as the underwater backdrop's ripple cells at the size the original shows them, their colours by distance from its
strip, the water between them the band's colour for that distance; land as tufts in its band's shades on cells of the same size;
the track occludes the ground, a road dipping below a surface is never left out, and only Aqua Tunnel's water cuts the track and
is seen from under. What does not carry over: the per-pixel depth test and the water pass, which were the old front end's own
fill kernels; the engine's draw lists paint in order and have no depth.

How it is drawn, in one line against the direction: the CPU makes one polygon and eight numbers a picture; the GPU works out
every pixel of the surface, its texel and its colour.
- **The polygon.** The plane at the surface's height, turned to the view (the matrix at $00E4 and the eye's height), clipped in
  view space against a near plane and the view's sides and out to a depth where it meets the horizon within a hundredth of a
  pixel, projected as the faces are (fx_face.c): one convex polygon covering the ground's part of the picture, made per picture
  and per player, so the pictures between steps and the wide window follow.
- **The shader** (`shaders/ground.shade`, fragment only, as the engine's game shaders are). The vertices' u, v are their screen
  places, so the GPU hands every pixel its own place exactly. The plane's coordinates from a screen place are one projective map
  (x' = (a x + b y + c) / (g x + h y + 1), y' the same with d, e, f): its eight numbers are the material's eight inputs. From
  them the pixel's texel (the art repeating across the plane) and its depth (the map's denominator), which picks the colour by
  distance. The material's image holds the cell art and a strip of the band's colours by depth, written when the course's
  colours change (art prepared as the sprites' textures are, never pixels of the picture).
- **The order.** Land, and water other than Aqua Tunnel's: the surface is drawn first, after the screen clear and before the far
  and near objects, so everything the race draws covers it (the track occludes the ground; a road below it is never hidden), and
  it is not drawn while the eye is below it. Aqua Tunnel's water: with one flat surface, anything on the far side of it from the
  eye is behind it along every ray, so the depth test's picture comes from order alone. Each face's shape (fx_face.c, already in
  view space) is cut by the plane; the part on the eye's side goes to a layer drawn after the surface, the other part to one
  drawn before it, each layer keeping the frame program's order; a sprite or solid goes whole by its place's side; particles,
  weather and labels after. Under the water (the glass tube) the same with the sides swapped: the surface seen overhead.
- **Two players**: each panel draws its own surface in its rectangle (the frame program runs per panel).
- No engine feature is needed: draw lists take a material with a game shader, its image read at any texel (`nshvc_image`, as
  `pause_blur.shade` does) and eight inputs.
- Shown to Jeff on a couple of courses (land and water, Aqua Tunnel from above and in the tube) before it is switched on.

**Land as a ground mesh (Jeff, 2026-11-09; replaces the plane for land; water stays a plane).** Measured: land is not flat (Easy
Ride's road runs from -1669 to -863 while its plane sat at -1280), so one plane's marks slid against the road. Every course
object is placed at the course's base height and its relief is in its shapes; all of them are in the map's cells from the course
script on (125-150 a course), so the whole course's shapes are known from the first picture.
- **Heights.** Once a course, the fx side takes every course shape's points in world space through the chip's own point
  routine (the frame program's object walk over the near and far records, with the view matrix at $00E4 set to the identity and
  each object placed at a fixed point in front, the points caught by fx_face_point and moved back by the object's place); sprites,
  cars and effects are left out. A grid of 512-unit cells over the course takes in each cell the lowest point that falls in it
  (a pillar's foot, a road's surface, a wall's base: where the shapes meet the ground). Cells with no point take the nearest
  known cell's height, then are smoothed from their neighbours, the known cells held and the grid's edge free: past the course
  the land goes on at the height of the course beside it. (A base height held at the edge, the median of the objects' places,
  raised hills that hid the scenery: the canyon's was -1920 against its road at 0.)
- **Drawing (built; replaces the triangles).** `shaders/terrain.shade` over one quad covering the view: each pixel's ray from
  the eye goes out over the grid's heights (in the image with the header: the view's turn, the eye, the grid) until it passes
  under the ground, then is narrowed to the meeting point; past the grid the ground is at its edge's height. Drawn first, before
  every object: the track always covers the ground. Not drawn while the eye is under the ground (a tunnel).
- **Colour.** Land other than rock: the band's colour for the pixel's distance, the row a flat ground 180 below the eye (the
  camera's measured height over the road, median 186 on Easy Ride and 172 on the canyon) would show that distance on, under the
  3D's horizon at the pixel's column (fx_draw_horizon_now), read from video's race_ground_colour; lit a little by its slope;
  the kind's marks a shade darker. Rock (Jeff: the canyon's land red rock): the course's own rock, from the survey's untextured
  faces: the colour with the most area on steep faces and the palette entries next to it that the faces also use (the canyons'
  are runs of four or five reds and browns, each face's shade baked by the way it faces), and how a face's shade goes with its
  normal, fitted by least squares over those faces (shade = light . n + bias), so the land at any slope takes the shade a rock
  face facing that way has. (The chip's lit materials were tried first: only the cars' paint is lit.)
- **Kinds, from the course survey (pictures of every course, 2026-11-09) and the bands.** Grass: Easy Ride ($00), the foggy
  forest ($04), Sea Breeze ($05) and Lake Side ($08) (their bands green beside the road, the sea's teal only toward the horizon,
  which the land takes by distance), the grass stunt arena ($0D), the countryside and beach arenas ($10, $12), the courses
  $14-$16. Rock: the canyon ($02), the wild-west desert ($09), the red-rock arena ($11). Cement: the airport, the night city
  ($03, Jeff), and the sunset city ($07). Snow: White Land ($06); ice: the ice stunt arena ($0E). Dirt: the dirt stunt arena
  ($0C). Water: Aqua Tunnel ($01) at -1280 in the underwater wave art; the night harbour ($0B) at -3840 (its old 3840 was a
  sign slip, under its own floor); the water stunt arena ($0F) and the night-city arena ($13, its band the harbour's) at their
  floors, 0. A water level is where the course's lowest road's pieces end (a road piece reaches 640 below the road), so the road
  stands in the water and none of it hangs above. None: Sky Ramp ($0A) and the checkered test arenas ($17-$1A). The arenas
  ($10-$13) were surveyed in two players.

#### Pattern textures as geometry (design, 2026-11-09)

**Shelved (2026-11-10).** Out of the game at Jeff's direction, with the pictures-as-geometry work: kept on branch
`shelf/pattern-textures` (fx_pattern.c and its hooks in fx_face.c and fx_draw.c). Every textured face is drawn from its
texture again. The text below records the design.

How it meets the direction: the CPU builds flat polygons into the draw list, as for the solids; the GPU fills them at the
output's resolution with the smooth rule; no pixel is made on the CPU, and no art is filtered or scaled; one path for every
backend.

- **What a pattern is.** A texture whose texels are a few flat colours bounded by straight edges, drawn as stairs at the
  chip's texel size. Each is described once in its tile's texel space (a texel x, y covers [x, x+1) x [y, y+1)) by the
  straight edges the stairs stand for, checked against every texel of the tile when the table is made (each texel's centre
  falls in the region of its own value). Found by the texture's key as the face's draw-list entry gives it: ROM bank, base,
  mask, and which part of the byte the options read (all eight bits, the high or the low four).
- **Regions in order, and holes.** Regions are convex polygons laid in order, a later one over an earlier (a sign's frame line
  passing behind its arrow); a place no region covers shows nothing, as the texture's see-through texels. The check: the last
  region holding a texel's centre shows its value, a centre no region holds is a 0 texel. A pattern with holes is used only
  where the options make 0 see-through.
- **Done (2026-11-09):** the stripes below; the red and the blue chevron boards (bank $12, bases $E040 and $A080, the high
  four bits: the fill left of X - Y = 16.5 and X + Y = 48.5); the chevron sign ($12:$E000, the low four bits: the frame's
  lines, then the chevron between X - Y = -0.5 and 11.5 and X + Y = 30.5 and 42.5, its fill between 1.5 and 9.5 and 32.5
  and 40.5; four see-through panes). Each checked against all 1,024 texels of its tile.
- **The curved arrow signs (Jeff, 2026-11-09: "Edges should be totally smooth and clean as the original pixelated one
  intended").** Built from clean geometry in `fx_pattern.c` rather than traced from the stairs: circle arcs (an arch's two
  arcs about one centre), straight runs of one width, 45-degree heads with sharp points, sized and placed by fitting them
  to the texels. The blue U-turn ($12:$C0A0: radii 4 and 12 about (15, 12.5)), the red U-turn with its white outline
  ($E060: the same about (15, 13.36), the outline the red grown by one texel), the turn ($C0E0: a quarter ring of radius 18,
  8.4 wide, about (22, 24)), the S-bend ($C0C0: three 45-degree runs 11 wide in X +- Y, outer corners rounded 5.4, inner
  sharp). So a few edge texels fall on the other side of a clean edge: 12, 50 (mostly the red U-turn's outline, two texels
  a row along its head's slants), 0 and 8; the table states each count and the check confirms it exactly. A first try
  traced and smoothed the stairs and was wobbly; Jeff turned it down. Not done: any pattern on a course no recording or
  demo has visited (banks $06, $12 and $13 looked at as a whole: no other tile with slanted or curved edges and few
  colours).
- **The first: the yellow and black stripes** (bank $12, base $A040, mask $1F1F, the high four bits; the barriers' walls, Aqua Tunnel's among them). Tile
  32 x 32 texels, two values ($A, $F), stripes 8 texels wide that move one texel left every 4 rows. Their edges are the lines
  X = 8k - Y/4 (through the middle of each step); value $A where X + Y/4 mod 16 is below 8, else $F. The texel centres
  give the tile's 1,024 texels exactly. The tile repeats every 32 in both directions as the mask has it (the stripes jump at
  a vertical repeat as the texels do).
- **On a face.** The textured face (fx_face.c, its corners in view space with their u, v, before the near clip) is fanned
  into triangles; each triangle maps u, v to view space by its own three corners, as the GPU would lay the texels. Each
  pattern region of each tile the triangle's u, v cover is clipped to the triangle in u, v, its corners mapped into view
  space, and the piece goes the way of any face: near clip, projection, the view's rectangle, fanned. Each piece is flat, its
  colour the texel value made into a colour as the texture's own texels are (the colour register's high four bits, the
  transparency rule, the palette row and depth mask), so it matches the texture's colours. In place of the face's textured
  pieces cut by depth; the face's outline, order and everything else are unchanged.
- **Where.** `fx/fx_pattern.c` holds the table and the cutting; fx_face.c asks it for a textured face and keeps the pieces'
  values per corner; fx_draw.c's shape drawing colours a pattern shape's corners instead of laying the texture.
- **Bounds.** A face over more than 64 tiles keeps the texture as it is drawn now (no seen face is near that).
- Shown to Jeff before and after (a scratch build's attract demo, 2026-11-09; the game's own on Aqua Tunnel once wired in); more patterns added the same way, each checked against its texels.

**Pictures as geometry (the HUD's fonts and pieces, console art): shelved (2026-11-10).** Out of the game, kept on
branch `shelf/hires-pictures` (the 239 pictures, the engine's later console-art extensions, and the fitting tools in
`tools/hires_art`). Jeff: the 239 are close to right, but they stand out against the rest of the game's art, which
cannot be converted the same way; the automatic fits of the other console art (race panels, menus, sprites, course
outlines) were rejected. Decided 2026-11-10: not used; it stays shelved (a one-pass redraw of the title cars read no better than an emulator filter: painted art needs repainting, not tracing).

### Stage 11. Whole game
Finder zero on `native/stuntracefx`; section 8's criteria; every play through `nshvc-check`; every reachable platform.

## 5. What gets deleted

From the new tree, by never being brought over: `machine.h`, `cpu_ops.h`, `hw.h`, `wram.h`, `gsu.h`, `gsu_ops.h`, `spc.h`, `spc_ops.h`,
`waits.h`, `console_ext.h`, `nshvc_handoff.h`, every `abi_` adapter, every `cpu.`, `gsu.`, `spc.` reference, the flag helpers, the
interrupt handlers, the chip connector.

From the branch, at the end: `ports/stuntracefx` (its icon moved over), `ports/common`. (Not yet: the icon is moved over and
nothing builds from `ports/` now, but Jeff deferred removing it, 2026-10-28.)

Kept: `src/stuntracefx`, `asm/stuntracefx`, `config/stuntracefx`, `tools/stuntracefx` including `sfxbox`, as the record and the
reference.

## 6. Recording and comparing

Built in stage 1, with this tooling and nothing more.

**What is recorded from the ROM in bsnes** (`sfxbox`, on the input-only plays in `tools/stuntracefx/plays`, using its existing
`--pacing`, `--apulog`, `--voices` and `--sound`, extended where needed):

- Picture state: for each frame, per-line effective PPU state, the palette, the sprite table and video memory changes, and the bsnes
  picture.
- Voice events: every DSP write with its sample time, and the port commands.
- Polygons: at each Super FX STOP, the draw list (`$08AA`), the other drawn primitives, and the inputs and outputs of each job.
- Logic: the work RAM and chip RAM at each logical step, and `$0306` for the pacing.

**Replay** (`native/checks`, a small program on the engine, not part of the game): takes one recording and makes the API calls a
converted game would make. Its picture compared with the bsnes picture tests the mapping and the engine with no game logic involved;
its voice output compared with the bsnes samples does the same for sound.

**State comparison.** A CSV made from the game's struct gives each field's offset and its original address. A check-only tool
compares the native game's state with the recording at each logical step, field by field. Addresses live in that CSV only; the
game's C has none.

**Recorded pacing.** Race frames can line up only if the steps do, so a check-only option given to the game's `main` reads each
step's frame count from the recording in place of N. The option is one branch in the pacing function and nothing else.

**Which frames line up.** `nshvc-check` compares what is alignable, and the final report says, per play: frames compared, frames
identical, frames with a difference and the cause of each kind of difference. Expected kinds: polygon edges; frames during the
original's staged upload, where the ROM shows part of a picture being replaced; sound offset from thread timing.

Anything the replay shows is treated by the boundary rule: establish from evidence whether the conversion or the engine is wrong.
If it is the engine, I stop and tell you before changing nativeshvc.

## 7. Runs

Under the standing rule each run below needs your typed approval, and I will state its measured length before it. Compile-only
builds are the only thing done without asking.

| Run | Stage | Length (estimate) |
|---|---|---|
| Record all input-only plays in bsnes | 1 | about 10 min |
| Replay against bsnes, then a planted break to show it fails | 1 | about 5 min |
| Driver voice-event comparison, both drivers | 3 | about 2 min |
| `nshvc-check` boot and title | 4 | about 2 min |
| Chip job and polygon comparison | 5 | about 5 min |
| `nshvc-check` menu plays | 6 | about 5 min |
| Race state comparison; `nshvc-check` race plays | 7 | about 10 min |
| Pacing record, and the speed comparison | 8 | about 10 min |
| Every play on macOS (Metal, Vulkan), iOS Simulator, Android emulator, Windows under Wine | 11 | about 30 min each |

A Linux VM counts as another machine and needs its own permission.

## 8. Done means

All of these, for `native/stuntracefx`:

1. `tools/nshvc-find-hardware` reports nothing.
2. No `R[`, `gsu.`, `spc.`, flag helpers, register-named carried values, computed `goto`, or SNES addresses in the game's C, under any
   name. Renaming a register file does not satisfy this; a function passing an `a`, `x`, `y` or status value along as its working state
   is a failure, however it is spelt.
3. Every function has real parameters and returns, and structured control flow.
4. No platform code; builds through `nshvc_game()` and `tools/nshvc-build`.
5. Every live routine in the record has a converted form, or an entry in the list of dead routines with its proof.
6. Each stage's check has passed, and the final report states which frames match and which do not, and why.

Interim forms will be named as interim, with their counts, in the same sentence each time.

## 9. Decisions

Settled by the maintainer, 2026-11-09:

| # | Question | Answer |
|---|---|---|
| 1 | May the physics and `model.c` start from the generated `fxgen-w` output, then be renamed and restructured by hand? | Yes |
| 2 | Are Frame rate 60, the wide picture, solid tyres, the pause screen and the other remaster features part of this conversion? | Yes, all of them, as stage 10 |
| 3 | Polygon edges: accept the engine's exact rule, with edges measured against bsnes? | Yes |
| 4 | Race pacing: a fixed step length from the recordings' median makes races 0.8-1.8% slow; which way? (asked 2026-11-09) | Jeff: "We can abandon matching the original and go all in on the enhanced. This isn't an emulation effort at all anymore." |
| 5 | (Jeff, unasked, the same day) | "Enhanced IS the native game. we are abandoning 'original' mode entirely. This isn't an emulation project. Be sure." |

## 10. What is not known

- Whether the voice calls cover both drivers. Stage 1 and stage 3 will show.
- Whether a draw-list layer's priority and per-line scroll behave as the race screen needs. Stage 1 will show.
- How the edge difference looks in numbers.
- The state comparison's masks: which work-RAM bytes are scratch the new game no longer has. Each masked byte needs a reason.
- Whether every use of a computed address (1,821 lines) resolves to a typed table or record by reading. If any does not, it is reported.
