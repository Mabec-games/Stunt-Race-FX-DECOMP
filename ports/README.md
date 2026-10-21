# ports/: native ports of the decompilation

Downstream of the decomp: a port runs the game on a platform it was never
written for. The decomp (`src/stuntracefx`) never changes for a port; the
port lifts its own copy (`stuntracefx/game`) and checks it against the
original (`stuntracefx/PLAN.md`).

| folder | what |
|---|---|
| `common/host` | what each OS supplies: time and files (`posix`, `win32`), a window, input and sound (`macos`, `windows`), or none (`null`, headless) |
| `common/snes` | the Super Nintendo without its processors: bus and clock, DMA and HDMA, PPU, Super FX, sound unit and DSP, coroutines; usable by any SNES decomp written against the same machine model |
| `stuntracefx` | Stunt Race FX: the port's game (`game`), the runtime under it, the front end, the build (`link.sh`, `package_app.sh`); `PORT.md` describes it |

```sh
HOST=null ports/stuntracefx/link.sh          # headless, for tests
ports/stuntracefx/package_app.sh             # macOS: Stunt Race FX.app
CC=x86_64-w64-mingw32-gcc HOST=windows OUT=build/stuntracefx/port/windows ports/stuntracefx/link.sh
python3 ports/stuntracefx/tools/compare.py SCRIPT   # shots and dumps against the original
```
