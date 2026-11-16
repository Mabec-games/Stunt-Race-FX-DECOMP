#!/bin/sh
# build-appimage.sh: Stunt Race FX - DECOMP, Version 1.0 Rebooted: the AppImage (Linux x86_64): the
# earlier port's launcher (launcher.c), the builder (builder/src, its data
# table from this tree's assets.csv) and this tree's native game as the
# builder's payload (game/: the program, its shaders and icon). The player
# chooses their ROM in the launcher; nothing of the ROM is packed.
#
#   sh native/stuntracefx/launcher/build-appimage.sh [ART.png]
#
# from the repository's top. Needs cmake, ninja, gcc or clang, SDL3,
# glslangValidator, python3, ffmpeg and linuxdeploy; NSHVC_ROOT (default
# ../nativeshvc) and VK_INCLUDE (a folder holding vulkan/ and vk_video/).
# Out: ../dist/StuntRaceFX-DECOMP-$VERSION-x86_64.AppImage.
set -e
ART=${1:-../img/mbc_presents.png}
VERSION=${VERSION:-1.0-Rebooted}
NSHVC_ROOT=${NSHVC_ROOT:-$(cd .. && pwd)/nativeshvc}
VK_INCLUDE=${VK_INCLUDE:-/usr/include}
G=native/stuntracefx
B=build/reboot-appimage
APPDIR=$B/AppDir
DIST=../dist
CC=${CC:-cc}

# spirv-cross only writes the Metal and HLSL shaders: a stand-in where it is missing
if ! command -v spirv-cross >/dev/null 2>&1; then
    mkdir -p $B/bin
    printf '#!/bin/sh\nexit 0\n' > $B/bin/spirv-cross
    chmod +x $B/bin/spirv-cross
    PATH=$(pwd)/$B/bin:$PATH
fi

# the game
cmake -S $G -B $B/game -G Ninja -DCMAKE_BUILD_TYPE=Release -DNSHVC_ROOT="$NSHVC_ROOT" \
      -DNSHVC_HOST=linux -DNSHVC_RENDER=vulkan -DNSHVC_VULKAN_INCLUDE="$VK_INCLUDE" >/dev/null
cmake --build $B/game

# the builder, its table from the game's own list of what it reads
mkdir -p $B/gen
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

rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin/game" "$APPDIR/usr/share/stuntracefx" "$APPDIR/usr/share/applications" \
         "$APPDIR/usr/share/icons/hicolor/256x256/apps" "$DIST"
$CC -O2 -Wall -DRB_VERSION="\"$VERSION\"" -Ibuilder/src/core -I$B/gen builder/src/core/*.c builder/src/linux/main.c \
    -ldl -lpthread -s -o "$APPDIR/usr/bin/StuntRaceFXBuilder"
$CC -O2 -Wall -o "$APPDIR/usr/bin/srfx-launcher" $G/launcher/launcher.c $(pkg-config --cflags --libs sdl3)

# the payload: the game beside the builder, as its release lays it out
strip -o "$APPDIR/usr/bin/game/stuntracefx" $B/game/stuntracefx
cp -R $B/game/shaders "$APPDIR/usr/bin/game/shaders"
find "$APPDIR/usr/bin/game/shaders" -type f -size 0 -delete     # (the stand-in's empty Metal and HLSL: Linux reads the .spv)
cp $B/game/icon.png $B/game/icon.argb "$APPDIR/usr/bin/game/"
if strings "$APPDIR/usr/bin/game/stuntracefx" | grep -q "STUNT RACE FX"; then
    echo "build-appimage: the game has the ROM's title in it: not packed" >&2
    exit 2
fi

# the launcher's background and the icon (a square of the art's car)
ffmpeg -v error -y -i "$ART" -pix_fmt bgr24 "$APPDIR/usr/share/stuntracefx/launcher-bg.bmp"
ffmpeg -v error -y -i "$ART" -vf "crop=ih:ih:(iw-ih)/2:0,scale=256:256" \
       "$APPDIR/usr/share/icons/hicolor/256x256/apps/stuntracefx.png"
cat > "$APPDIR/usr/share/applications/stuntracefx.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Stunt Race FX - DECOMP
Comment=Version 1.0 Rebooted: the native GPU remaster (Jeff Wilbur, Strut B) with the DECOMP project's fixes
Exec=srfx-launcher
Icon=stuntracefx
Categories=Game;
Terminal=false
X-AppImage-Version=$VERSION
EOF

# SDL3 for the launcher; the game and the builder need only the C library
# (the game opens X11, ALSA and Vulkan as it runs)
cd "$B"
rm -f StuntRaceFX*.AppImage
NO_STRIP=1 LDAI_OUTPUT="StuntRaceFX-DECOMP-$VERSION-x86_64.AppImage" linuxdeploy --appdir AppDir \
    -e AppDir/usr/bin/srfx-launcher \
    -d AppDir/usr/share/applications/stuntracefx.desktop \
    -i AppDir/usr/share/icons/hicolor/256x256/apps/stuntracefx.png \
    --output appimage >/dev/null
cd - >/dev/null
mv "$B/StuntRaceFX-DECOMP-$VERSION-x86_64.AppImage" "$DIST/"
echo "build-appimage: $DIST/StuntRaceFX-DECOMP-$VERSION-x86_64.AppImage"
