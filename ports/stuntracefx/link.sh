#!/bin/sh
# Build Stunt Race FX native: the port's game (ports/stuntracefx/game, lifted
# from the decomp's C and checked against the original), the machine layer (ports/common/snes), the port's
# runtime (ports/stuntracefx/runtime) and one host (ports/common/host).
#   env: HOST     null (default: headless, for tests) | macos | windows |
#                 lib (a static library, libstuntracefx.a, for an app that
#                 brings its own host: main() is srfx_main)
#        CC       the compiler; one aimed at Windows (x86_64-w64-mingw32-gcc)
#                 builds stuntracefx.exe with host/win32 under the host
#        OUT      build directory (default build/stuntracefx/port/HOST)
#        GAME     the game's C (default ports/stuntracefx/game; src/stuntracefx
#                 builds the reference the port's copy is checked against)
#        SRFX_ROM your cartridge's ROM (default reference/stuntracefx/Stunt
#                 Race FX (USA) (Rev 1).sfc), checked by its SHA-1 and built
#                 into the executable: the game needs no ROM once built
set -e
cd "$(dirname "$0")/../.."
HOST=${HOST:-null}
CC=${CC:-clang}
OUT=${OUT:-build/stuntracefx/port/$HOST}
GAME=${GAME:-ports/stuntracefx/game}
ROM=${SRFX_ROM:-reference/stuntracefx/Stunt Race FX (USA) (Rev 1).sfc}
case "$OUT" in build/*) ;; *) echo "link: OUT must be under build/ (got $OUT)" >&2; exit 2;; esac
TARGET=$($CC -dumpmachine)
SHA=$(python3 -c "import hashlib,sys;print(hashlib.sha1(open(sys.argv[1],'rb').read()).hexdigest())" "$ROM")
[ "$SHA" = 781e376ff453a83c7bcf72339abe6b842efd9c86 ] || { echo "link: $ROM is not Stunt Race FX (USA) (Rev 1)" >&2; exit 2; }
mkdir -p "$OUT/gen" "$OUT/obj"

SRFX_GAME="$GAME" python3 ports/stuntracefx/tools/routines.py "$ROM" "$OUT/gen/routines.c" >/dev/null

# the ROM, as data in the executable
ROMABS=$(cd "$(dirname "$ROM")" && pwd)/$(basename "$ROM")
case "$TARGET" in
  *apple*)          SYM=_srfx_rom; SEC='.section __TEXT,__const';;
  *mingw*|*windows*) SYM=srfx_rom;  SEC='.section .rdata,"dr"';;
  *)                SYM=srfx_rom;  SEC='.section .rodata';;
esac
cat > "$OUT/gen/romdata.S" <<EOS
    $SEC
    .globl ${SYM}
    .globl ${SYM}_end
    .p2align 4
${SYM}:
    .incbin "$ROMABS"
${SYM}_end:
EOS

CFLAGS="-O2 -g -std=gnu11 -fno-strict-aliasing -ffp-contract=off"
INC="-Iports/common/snes -Iports/common/video -Iports/common/host -I$GAME/include -Iports/stuntracefx/runtime"
case "$TARGET" in
  *mingw*|*windows*) OSHOST=ports/common/host/win32/host_win32.c; EXE=.exe; OSLIBS="-static -lwinmm";;
  *)                 OSHOST=ports/common/host/posix/host_posix.c; EXE=;     OSLIBS=;;
esac

OBJS=
# the newest header: an object older than it is built again
HDR="$OUT/obj/.headers"
NEWEST=$(ls -t "$GAME"/include/*.h ports/common/*/*.h ports/stuntracefx/runtime/*.h | head -1)
[ -f "$HDR" ] && [ ! "$NEWEST" -nt "$HDR" ] || touch -r "$NEWEST" "$HDR"
compile() {   # source, extra flags
  o="$OUT/obj/$(echo "$1" | sed 's#/#__#g').o"
  if [ ! -f "$o" ] || [ "$1" -nt "$o" ] || [ ports/stuntracefx/link.sh -nt "$o" ] || [ "$HDR" -nt "$o" ]; then
    $CC $CFLAGS $2 -c "$1" -o "$o"
  fi
  OBJS="$OBJS $o"
}

# the game: every C file
for f in $(find "$GAME" -name '*.c' | sort); do
  compile "$f" "-w -I$GAME/include"
done
# the machine layer and the runtime (an app with its own host calls srfx_main)
for f in ports/common/snes/*.c ports/common/video/*.c ports/stuntracefx/runtime/*.c "$OUT/gen/routines.c"; do
  if [ "$HOST" = lib ] && [ "$f" = ports/stuntracefx/runtime/main.c ]; then
    compile "$f" "-Wall $INC -Dmain=srfx_main"
  else
    compile "$f" "-Wall $INC"
  fi
done
case "$TARGET" in
  *mingw*|*windows*) ;;
  aarch64*|arm64*) compile ports/common/snes/coro_arm64.S "";;
  x86_64*)         compile ports/common/snes/coro_x86_64.S "";;
esac
compile "$OUT/gen/romdata.S" ""
compile "$OSHOST" "-Wall -Iports/common/host"

LIBS="$OSLIBS"
case "$HOST" in
  null)    compile ports/common/host/null/host_null.c "-Wall -Iports/common/host";;
  macos)   compile ports/common/host/macos/host_macos.m "-Wall -fobjc-arc -Iports/common/host"
           LIBS="$LIBS -framework Cocoa -framework QuartzCore -framework AudioToolbox -framework CoreAudio -framework GameController";;
  windows) compile ports/common/host/windows/host_windows.c "-Wall -Iports/common/host"
           ports/common/host/windows/game_rc.sh "Stunt Race FX" "Stunt Race FX.exe" "$OUT/obj/game.res.o"
           OBJS="$OBJS $OUT/obj/game.res.o"
           LIBS="$LIBS -lole32 -luuid -lxinput9_1_0 -lshell32 -lgdi32 -luser32 -mwindows";;
  lib)     ;;
  *) echo "link: unknown HOST $HOST" >&2; exit 2;;
esac

if [ "$HOST" = lib ]; then
  rm -f "$OUT/libstuntracefx.a"
  ar rcs "$OUT/libstuntracefx.a" $OBJS
  echo "link: $OUT/libstuntracefx.a"
else
  $CC -o "$OUT/stuntracefx$EXE" $OBJS $LIBS -lm
  echo "link: $OUT/stuntracefx$EXE"
fi
