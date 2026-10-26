#!/bin/sh
# build-appimage.sh: Stunt Race FX's beta AppImage (Linux x86_64, SDL3).
#
#   sh ports/stuntracefx/launcher/build-appimage.sh [ART.png]
#
# from stuntracefx/. The game is built without the ROM in it (ROM_EXTERNAL=1:
# the player chooses their own in the launcher); the AppImage holds the
# launcher, the game, the launcher's background (ART, default
# ../img/2.png) and SDL3. Out: ../dist/StuntRaceFX-Beta-x86_64.AppImage.
set -e
ART=${1:-../img/2.png}
VERSION=${VERSION:-beta-0.1}
B=build/appimage
APPDIR=$B/AppDir
DIST=../dist
CC=${CC:-clang -D_GNU_SOURCE -O2}

# the game, no ROM inside
HOST=sdl3 ROM_EXTERNAL=1 OUT=build/stuntracefx/port/sdl3 CC="$CC" sh ports/stuntracefx/link.sh
if strings build/stuntracefx/port/sdl3/stuntracefx | grep -q "STUNT RACE FX"; then
    echo "build-appimage: the game has a ROM in it: not packed" >&2
    exit 2
fi

rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/stuntracefx" "$APPDIR/usr/share/applications" \
         "$APPDIR/usr/share/icons/hicolor/256x256/apps" "$DIST"

# the launcher
$CC -Wall -o "$APPDIR/usr/bin/srfx-launcher" ports/stuntracefx/launcher/launcher.c $(pkg-config --cflags --libs sdl3)
cp build/stuntracefx/port/sdl3/stuntracefx "$APPDIR/usr/bin/stuntracefx"

# the background and the icon (a square of the art's car)
ffmpeg -v error -y -i "$ART" -pix_fmt bgr24 "$APPDIR/usr/share/stuntracefx/launcher-bg.bmp"
ffmpeg -v error -y -i "$ART" -vf "crop=ih:ih:(iw-ih)/2:0,scale=256:256" \
       "$APPDIR/usr/share/icons/hicolor/256x256/apps/stuntracefx.png"

cat > "$APPDIR/usr/share/applications/stuntracefx.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Stunt Race FX
Comment=Native PC port (beta)
Exec=srfx-launcher
Icon=stuntracefx
Categories=Game;
Terminal=false
X-AppImage-Version=$VERSION
EOF

cd "$B"
rm -f StuntRaceFX*.AppImage
NO_STRIP=1 LDAI_OUTPUT="StuntRaceFX-$VERSION-x86_64.AppImage" linuxdeploy --appdir AppDir \
    -e AppDir/usr/bin/srfx-launcher -e AppDir/usr/bin/stuntracefx \
    -d AppDir/usr/share/applications/stuntracefx.desktop \
    -i AppDir/usr/share/icons/hicolor/256x256/apps/stuntracefx.png \
    --output appimage
cd - >/dev/null
mv "$B/StuntRaceFX-$VERSION-x86_64.AppImage" "$DIST/"
echo "build-appimage: $DIST/StuntRaceFX-$VERSION-x86_64.AppImage"
