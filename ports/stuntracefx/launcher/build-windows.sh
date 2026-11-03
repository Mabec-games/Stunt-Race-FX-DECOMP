#!/bin/sh
# build-windows.sh: Stunt Race FX's beta for Windows (x86_64), cross-built
# with MinGW-w64, a portable folder and its zip.
#
#   sh ports/stuntracefx/launcher/build-windows.sh [ART.png]
#
# from stuntracefx/. Needs x86_64-w64-mingw32-gcc and SDL3's MinGW
# development package (SDL3-devel-*-mingw from libsdl-org/SDL's releases),
# its x86_64-w64-mingw32 folder in SDL3_MINGW (default
# ~/.cache/srfx-sdl3-mingw; a path without spaces). The game is built
# without the ROM in it (ROM_EXTERNAL=1: the player chooses their own in the
# launcher). The folder holds the launcher ("Stunt Race FX.exe"), the game
# (stuntracefx.exe), SDL3.dll, the launcher's background and a README; all
# the settings, saves and logs go into it (portable). Out:
# ../dist/StuntRaceFX-$VERSION-windows/ and its .zip.
set -e
ART=${1:-../img/2.png}
VERSION=${VERSION:-beta-0.2}
SDL3_MINGW=${SDL3_MINGW:-$HOME/.cache/srfx-sdl3-mingw}
CC=${CC:-x86_64-w64-mingw32-gcc -O2}
B=build/windows
OUT=build/stuntracefx/port/win64rel
DIST=../dist
DIR=$DIST/StuntRaceFX-$VERSION-windows

[ -f "$SDL3_MINGW/bin/SDL3.dll" ] || { echo "build-windows: no SDL3 for MinGW in $SDL3_MINGW" >&2; exit 2; }
mkdir -p "$B/pkgconfig"
cat > "$B/pkgconfig/sdl3.pc" <<EOF
prefix=$SDL3_MINGW
Name: sdl3
Description: SDL3 for MinGW
Version: 3
Libs: -L\${prefix}/lib -l:libSDL3.dll.a
Cflags: -I\${prefix}/include
EOF

# the game: no ROM inside, no console window
PKG_CONFIG_LIBDIR="$PWD/$B/pkgconfig" HOST=sdl3 RENDER=soft ROM_EXTERNAL=1 OUT=$OUT CC="$CC" LDEXTRA=-mwindows \
    sh ports/stuntracefx/link.sh
if strings "$OUT/stuntracefx.exe" | grep -q "STUNT RACE FX"; then
    echo "build-windows: the game has a ROM in it: not packed" >&2
    exit 2
fi

rm -rf "$DIR"
mkdir -p "$DIR"
cp "$OUT/stuntracefx.exe" "$DIR/"
# the launcher
$CC -Wall -o "$DIR/Stunt Race FX.exe" ports/stuntracefx/launcher/launcher.c -I"$SDL3_MINGW/include" \
    -L"$SDL3_MINGW/lib" -l:libSDL3.dll.a -mwindows -static-libgcc
cp "$SDL3_MINGW/bin/SDL3.dll" "$DIR/"
ffmpeg -v error -y -i "$ART" -pix_fmt bgr24 "$DIR/launcher-bg.bmp"
cat > "$DIR/README.txt" <<EOF
Stunt Race FX - native PC port, $VERSION (Windows)
https://github.com/Mabec-games/Stunt-Race-FX-DECOMP

Start "Stunt Race FX.exe" (the launcher). Choose your own ROM file there:
Stunt Race FX (USA) (Rev 1), SHA-1 781e376ff453a83c7bcf72339abe6b842efd9c86.
No ROM is included.

Portable: the launcher's settings (launcher.cfg), the saves and the session
logs (the save folder) are kept in this folder. Nothing is written elsewhere.

The same as the Linux version: look-ahead, high-resolution 3D, wide view,
in-between pictures. Keys in the game: F2-F8 (see CONTROLS in the launcher).

Known issues (beta):
- Some stages may still show artifacts at tunnel entrances.
- The Windows version was built and checked under Wine; tell us how it runs
  on a real Windows PC.

Unofficial fan project, not affiliated with or endorsed by Nintendo.
Stunt Race FX is (c) Nintendo.
EOF
( cd "$DIST" && rm -f "StuntRaceFX-$VERSION-windows.zip" && \
  python3 -I -c 'import shutil,sys; shutil.make_archive(sys.argv[1], "zip", ".", sys.argv[1])' "StuntRaceFX-$VERSION-windows" )
echo "build-windows: $DIR (and .zip)"
