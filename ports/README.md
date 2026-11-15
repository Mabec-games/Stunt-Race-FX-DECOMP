# ports/: the earlier front ends (no longer built)

> **Superseded (2026-11-14).** Nothing builds from this folder now. The game
> is `native/stuntracefx`, an ordinary native game on nativeshvc (the
> README's "The native game" section), and Stunt Race FX Builder builds
> that. Jeff deferred removing this folder (2026-10-28); it is kept as the
> record of the earlier work until then. Two tools still refer to files here:
> `tools/stuntracefx/rmwsites.py` and `tools/stuntracefx/gsuprice.py`.

What it held, as of its last change (November 5, 2026):

| folder | what |
|---|---|
| `stuntracefx/front` | the second front end (October 27 to November 5, 2026): the decomp's C (`src/stuntracefx`) played as a cartridge on nativeshvc's register-level console API, with Original and Enhanced modes, its Super FX on nativeshvc's chip connector (`superfx_cart.c`), its cost profile (`cost_profile.c`), touch, the pause screen, the Metal and Vulkan fills, and the macOS and iOS packaging |
| `stuntracefx/icon` | the game's icon and `icons.sh` (the native game's icon is now `native/stuntracefx/icon/icon.png`, made into each platform's form by nativeshvc) |
| `common/snes` | the cartridge's Super FX (`superfx.c`); the rest is the first port's own console model |
| `common/host`, `common/render`, `common/audio`, `common/video` | the first port's platform layer and renderers |
| `stuntracefx` | the first port's runtime and build (`link.sh`, `PORT.md`); `PLAN.md`, `HANDOFF.md`, `WIDESCREEN.md` |

nativeshvc no longer offers the register-level API these were written
against, so they do not build against its current version.
