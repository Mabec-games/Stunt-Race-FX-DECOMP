#!/bin/sh
# build-windows.sh: Stunt Race FX - DECOMP, Version 1.0 Rebooted for Windows
# (x86_64), cross-built on Linux with MinGW-w64: a portable folder and zip.
#
#   Stunt Race FX - DECOMP\
#     Stunt Race FX.exe            the launcher (SDL3), SDL3.dll, launcher-bg.bmp
#     StuntRaceFXBuilder.exe       the builder (builds the game from the ROM)
#     game\                        the builder's payload: stuntracefx.exe, shaders\
#     built\                       made on the first PLAY: the game from your ROM
#
#   sh native/stuntracefx/launcher/build-windows.sh [ART.png]
#
# from the repository's top. Needs mingw-w64 (x86_64-w64-mingw32-*), cmake,
# ninja, glslangValidator, python3, ffmpeg; SDL3_MINGW (SDL3's MinGW
# package, its x86_64-w64-mingw32 folder), VK_INCLUDE (a folder holding
# vulkan/ and vk_video/: Vulkan-Headers' include) and NSHVC_ROOT (default
# ../nativeshvc). Nothing of the ROM is packed.
# Out: ../dist/StuntRaceFX-DECOMP-$VERSION-windows-x86_64.zip.
set -e
ART=${1:-docs/media/banner.png}
VERSION=${VERSION:-1.0-Rebooted}
NSHVC_ROOT=${NSHVC_ROOT:-$(cd .. && pwd)/nativeshvc}
: "${SDL3_MINGW:?set SDL3_MINGW to SDL3's x86_64-w64-mingw32 folder}"
: "${VK_INCLUDE:?set VK_INCLUDE to a folder holding vulkan/ and vk_video/}"
G=native/stuntracefx
B=build/reboot-windows
NAME="Stunt Race FX - DECOMP"
OUT="$B/$NAME"
DIST=${DIST:-../dist}
MINGW=x86_64-w64-mingw32

# spirv-cross only writes the Metal and HLSL shaders: a stand-in where it is missing
if ! command -v spirv-cross >/dev/null 2>&1; then
    mkdir -p $B/bin
    printf '#!/bin/sh\nexit 0\n' > $B/bin/spirv-cross
    chmod +x $B/bin/spirv-cross
    PATH=$(pwd)/$B/bin:$PATH
fi

# the game (nativeshvc's Windows host, drawn with Vulkan)
cmake -S $G -B $B/game -G Ninja -DCMAKE_BUILD_TYPE=Release -DNSHVC_ROOT="$NSHVC_ROOT" \
      -DNSHVC_HOST=windows -DNSHVC_RENDER=vulkan -DCMAKE_SYSTEM_NAME=Windows \
      -DCMAKE_C_COMPILER=$MINGW-gcc -DCMAKE_RC_COMPILER=$MINGW-windres \
      -DNSHVC_VULKAN_INCLUDE="$VK_INCLUDE" >/dev/null
cmake --build $B/game

# the builder's data table, from the game's own list of what it reads
mkdir -p $B/gen $B/win
python3 - "$G/tools/assets.csv" > $B/gen/cart_table.h <<'EOF'
import csv, sys
rows = [(int(r['start'], 16), int(r['end'], 16)) for r in csv.DictReader(open(sys.argv[1]))]
print('/* cart_table.h, from native/stuntracefx/tools/assets.csv */')
print('#define SRFX_CART_RANGES %d' % len(rows))
print('static const uint32_t k_srfx_cart[SRFX_CART_RANGES][2] = {')
for s, e in rows:
    print('    { 0x%06X, 0x%06X },' % (s, e))
print('};')
EOF

rm -rf "$OUT"
mkdir -p "$OUT/game"

# the builder (its Win32 front end; scripted by the launcher)
cat > $B/win/payload.h <<EOF
#define RB_VER_MAJOR 1
#define RB_VER_MINOR 0
#define RB_VERSION_STR "$VERSION"
EOF
cp $B/game/stuntracefx-icon/icon.ico $B/win/builder.ico
$MINGW-windres -I$B/win -Ibuilder/src/win builder/src/win/builder.rc -O coff -o $B/win/builder.res.o
OBJS=
for s in builder/src/core/*.c builder/src/win/main.c; do
    o=$B/win/$(basename $s .c).o
    $MINGW-gcc -O2 -Wall -DUNICODE -D_UNICODE -DRB_VERSION="\"$VERSION\"" -Ibuilder/src/core -I$B/gen \
        -c $s -o $o
    OBJS="$OBJS $o"
done
$MINGW-gcc -municode -mwindows -static -s -o "$OUT/StuntRaceFXBuilder.exe" \
    $OBJS $B/win/builder.res.o -lcomctl32 -lole32 -lshell32 -luuid

# the launcher, with the game's icon
cat > $B/win/launcher.rc <<'EOF'
1 ICON "builder.ico"
EOF
$MINGW-windres -I$B/win $B/win/launcher.rc -O coff -o $B/win/launcher.res.o
$MINGW-gcc -O2 -Wall -mwindows -I"$SDL3_MINGW/include" -o "$OUT/$NAME.exe" $G/launcher/launcher.c \
    $B/win/launcher.res.o -L"$SDL3_MINGW/lib" -lSDL3 -s
cp "$SDL3_MINGW/bin/SDL3.dll" "$OUT/"

# the payload: the game beside the builder, as the builder expects it
$MINGW-strip -o "$OUT/game/stuntracefx.exe" $B/game/stuntracefx.exe
cp -R $B/game/shaders "$OUT/game/shaders"
find "$OUT/game/shaders" -type f -size 0 -delete
if strings "$OUT/game/stuntracefx.exe" | grep -q "STUNT RACE FX"; then
    echo "build-windows: the game has the ROM's title in it: not packed" >&2
    exit 2
fi

ffmpeg -v error -y -i "$ART" -pix_fmt bgr24 "$OUT/launcher-bg.bmp"
cp README.md "$OUT/README.md"

mkdir -p "$DIST"
ZIP="$(cd "$DIST" && pwd)/StuntRaceFX-DECOMP-$VERSION-windows-x86_64.zip"
rm -f "$ZIP"
(cd $B && python3 -m zipfile -c "$ZIP" "$NAME")
echo "build-windows: $ZIP"
