# What the game needs from nativeshvc

Requests from the Stunt Race FX conversion to the engine's owner, through Jeff. Each is general, for every game, and
names no game. None has been made in the engine by the game's sessions.

## 1. Every native input, as true inputs (Jeff, 2026-11-09)

Done: keys and the pointer in API 4.5 (the pause screen's keys, 99f2701b's corner button); every device as itself in API
5.0, where the game maps each device itself (5ae88cab).

Jeff: "NativeSVHC should support all native inputs that it runs on, whether mouse, keyboard, or controller, and make those
available to the game as true inputs. If it doesn't now, it needs to."

Today (API 4.4) `nshvc_input` carries only SNES pad bits per player, two mice as SNES mouse deltas, and two light guns' aim
in screen pixels. A game cannot see:

- **The keyboard as keys.** Which key went down and up, as `nshvc_key`, for every key (arrows, Shift, Control, Escape,
  function keys), not only the letters, digits, space, backspace and enter that `nshvc_text_read` gives. Without it a game
  cannot offer "press the key you want" when remapping, though `nshvc_control_bind` takes an `nshvc_key`.
- **The pointer in window pixels.** Its position, its buttons and its wheel, and touches, as the host has them, for a
  game's own interface (a pause button, a panel), apart from an emulated SNES mouse or gun.
- **The controller as itself.** Its buttons and sticks as the host reports them, beside the SNES pad mapping.

## 2. Input while the game is paused

`nshvc_pause(1)` stops `update`, and input reaches the game only through `update`. Jeff: the game owns how it pauses, so the
game keeps updating and stops its own logic; this needs nothing from the engine. Listed so the engine's owner knows it was
considered.

## 3. Typing

Done in API 4.5: keys that type text press no pad button while text input is on (abe00b42).

- **Keys that type text, kept off the pad while typing.** Today every key that goes down is put in the text queue and
  also presses whatever pad button it is bound to. The default bindings put B, Y, A, X, L and R on Z, A, X, S, Q and W.
  So a game that turns on `nshvc_text_input(1)` to let the player type a name gets a pad button with each of those
  letters, and a button that confirms or backs out of the screen fires while the player types. A game cannot filter this
  itself, because the player may bind keys anywhere. Asked: while text input is on, keys that type text ('A'-'Z', digits,
  space, Backspace) go to the text queue only and press no pad button. Enter can keep pressing its button, so a typed
  name can be confirmed.

## 4. Binding a controller's buttons

Done in nativeshvc c190de1 (API 4.6): `nshvc_control_bind_controller`, `nshvc_control_defaults`, `nshvc_control_name`.

Part of Jeff's direction 1: every native input reaches the game as a true input, and a player can rebind it.

API 4.5 binds keyboard keys only: `nshvc_control_bind(player, button, nshvc_key)`. The engine maps every controller to
the pad itself, and a game cannot change which controller button presses which pad button. A game's pause screen can let
the player rebind keys ("press the key you want", from `keys_pressed`), but not a controller's buttons.

A game cannot do it with 4.5. `nshvc_input.pad[]` merges the keyboard, every controller and the touch screen. The bits a
controller presses there look the same as a keyboard's, so a game cannot take the engine's controller mapping back out
and put its own in. It sees `controller[].buttons` beside the pad, but a pad bit that a key and a controller button both
press can't be split between them. Mapping every device itself, the game would have to repeat the engine's own mapping of
keys, controllers and touch, which the engine owns.

Asked, for every game:
- `nshvc_control_bind_controller(player, button, ctl)`: the controller button `ctl` (an `NSHVC_CTL_*`, by its place)
  presses `button` for that player's controllers, taking `ctl` from whichever button had it, as `nshvc_control_bind` does
  for keys.
- `nshvc_control_name(player, button)` names the controller button bound to `button` when the player's source is a
  controller, in that controller family's own terms (A, Cross, and so on).
- The bindings last until changed, and a game can put the engine's defaults back. The game keeps them in its own save,
  as it does its keys.

With that, a game's pause screen takes a controller button pressed (from `controller[].buttons`) for a SNES button the
way it takes a key now.

A touch screen's on-screen controls are the engine's, with nothing to rebind; nothing is asked for touch.

## 5. The game's icon on every platform (Jeff's bug, 2026-11-09)

Done in nativeshvc c190de1 (API 4.6). Checked on macOS: the app's Info.plist names `AppIcon` and its `.icns` holds the art
at 16 to 1024. Not built yet: iOS, Windows, Linux.

Jeff: "Built app (mac at least, probably all platforms) doesn't use the app icon specified." The game now passes
`ICON icon/icon.png` (800 x 800, no alpha) to `nshvc_game()`. Today `cmake/NshvcGame.cmake` uses `ICON` only on Android
(one PNG in `res/mipmap`). On Apple it copies the file into `Resources` but sets no `MACOSX_BUNDLE_ICON_FILE`, so
Info.plist has no `CFBundleIconFile` and macOS shows the generic icon; iOS gets no asset catalog or `CFBundleIcons`;
Windows gets no resource script, so the window class finds no icon 1; Linux gets nothing.

Asked: `nshvc_game(... ICON <one square image>)` makes each platform's form at build time:
- macOS: an `.icns` (each size at 1x and 2x, via `sips` and `iconutil`) in Resources, named by `MACOSX_BUNDLE_ICON_FILE`.
- iOS: an asset catalog compiled by `actool`, its partial Info.plist merged (`CFBundleIcons`).
- Windows: an `.ico` (16 to 256) in a resource script linked into the program as the icon the window class loads.
- Linux: a PNG beside the program for a desktop file; Android as now.
A working recipe for all of these from one image is in the old port's `ports/stuntracefx/icon/icons.sh`.

## 6. Antialiased draw lists (Jeff, 2026-11-09: "That black arrow on yellow is still a bit aliased")

Done in nativeshvc 3800784 (API 4.7), Jeff's choice "on by default, game can override per list": 8 samples a pixel unless
`nshvc_draw_samples` says otherwise. Seen on the turn and S-bend signs replayed at 8 times the screen (edges shaded).

Every polygon edge a draw list fills is aliased: `NSHVC_FILL_SMOOTH` gives sub-pixel vertices, but each pixel is wholly in
or out, because a draw list is filled into a one-value-a-pixel texture (`R32Uint` on Metal: a palette index or
`0xFFRRGGBB`) and composed per window pixel from it. The game cannot smooth an edge from its side: a pixel holds one
value, and an index cannot be blended before the palette and the colour maths.

Asked: a draw list filled with several samples a pixel (a game's choice, e.g. `nshvc_draw_samples(list, 4)`), each sample
composed as a pixel is now (the palette, see-through 0, the layers' priorities, windows and colour maths per sample), and
the samples' final colours averaged into the window's pixel. The picture stays exact per sample; only the edges change.
The same on Metal, Vulkan and D3D12, and in the picture `nshvc_picture_read` returns.

## 7. Pixel art sharp and even at any size (Jeff, 2026-11-09)

Undone: nativeshvc main reset to 3800784 (4.7) on Jeff's word (on seeing it: "It went from sharp edges to blurry edges. I wanted high resolution
replacements like the others not smoothing"): textures go back to one read a pixel with hard texel edges; 4.7's polygon-edge
samples stay. Pixel art gets the game's own high-resolution replacements instead. Was done in nativeshvc 34f88d8 (4.7.1): a list's image and games' shaders read at each sample. Seen on a textured sign replayed at 8
times the screen: each texel solid, its edges shaded by coverage. A Vulkan device without sample-rate shading fills lists with one
sample.

Jeff: "Pixel art shouldn't be 'smoothed' it should be drawn with the hard edges a high resolution texture can be while still
being sharp." Each texel a solid square with its edges exactly where they fall in the window; a window pixel an edge
crosses mixed by how much of it each side covers; nothing blurred inside a texel; no texels 3 pixels wide beside ones 4
wide at a size that is not a whole multiple.

4.7 gives that to a triangle's outside edges only: inside a triangle the texture is read once a pixel, at its centre
(`list_fs` reads no `[[sample_id]]`), so an image's or a textured face's texel edges stay stepped.

Asked, on every backend and in `nshvc_picture_read`'s picture: a draw list of several samples reads its image at each
sample (sample-rate shading), so texel edges inside a triangle are covered as its outside edges are.

Withdrawn: a second part, asking for the window's pixel to be composed at several samples. It came from reading the call
`pb_window(uint(pos.x), uint(pos.y))` without its body; the engine already composes each window pixel from its footprint
on the texel grid (each texel solid, a pixel a texel edge crosses mixed by that coverage) for the maps, sprites, per-line
changes and draw-list rows, mode 7 by its own coverage. No picture had shown otherwise.

## Later engine features the game uses

Asked through the engine's own sessions with Jeff's approval, each general, for every game; listed here so this file shows
what the game depends on. The engine's documents are the reference for each.

- **6.0 and 6.1, the new renderer** (nativeshvc 6271570): a vertex's colour is a colour, no longer a palette entry; the game
  draws its one-entry shapes through a 256 x 1 indexed image (0840b5e2).
- **6.2, instanced meshes** (`nshvc_draw_mesh`, f8f8fba): King's Forest's land trees (4b21e1b7) and the other courses'
  instanced land (buildings, clouds).
- **6.3, depth settings** for a draw list (cull mode, depth bias, depth write, the list's own depth): the depth-tested 3D
  (d90d168e, [DEPTH.md](DEPTH.md)).
- **6.4, `nshvc_pixel`** in a game's shader: the view from above's stepped see-through (235f98fc, `shaders/fade.shade`).
  It uses no blended drawing: quarter steps are exact as sample coverage.
