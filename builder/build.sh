#!/bin/sh
# builder/build.sh: build Stunt Race FX Builder for macOS, Windows and Linux.
#
#   build/builder/dist/StuntRaceFXBuilder-VERSION-macOS.zip    Stunt Race FX Builder.app, universal:
#                                                              builds Stunt Race FX for the Mac or
#                                                              for the iPhone and iPad
#   build/builder/dist/StuntRaceFXBuilder-VERSION-Windows.zip  Stunt Race FX Builder\StuntRaceFXBuilder.exe
#                                                              and game\ (the payload), x64
#   build/builder/dist/StuntRaceFXBuilder-VERSION-Linux-x86_64.tar.gz  Stunt Race FX Builder/StuntRaceFXBuilder
#   build/builder/dist/StuntRaceFXBuilder-VERSION-Linux-arm64.tar.gz   and game/ (the payload), per CPU
#
# Each carries the game (native/stuntracefx on nativeshvc) built with none of
# the cartridge's data in it: the game reads cart.dat, which the builder
# writes from the player's ROM; check_payload.py proves that before anything
# is packaged. The payload is built from HEAD, or REV (a worktree of it at
# build/builder/tree, so cart.dat, which is never committed, cannot be in
# it), never from the working tree, so what is released is what is committed.
# Its icon, and the builder's, is the game's own (native/stuntracefx/icon/
# icon.png, made into each system's form by nativeshvc's nshvc-icons).
#
# VERSION: 0.99 unless set.
# Needs nativeshvc's checkout (NSHVC, ../nativeshvc), the ROM (ROM, default
# reference/stuntracefx/: check_payload.py reads it to prove its absence),
# Xcode's clang and the iOS SDK, cmake and ninja, glslangValidator and
# spirv-cross (the game's shaders; dxc for their D3D12 form), mingw-w64
# (x86_64-w64-mingw32-*) and the Vulkan headers (brew's vulkan-headers) for
# Windows and Linux. Linux is built on a Linux machine over ssh (LINUX_HOST,
# default strutb@192.168.1.25, in ~/srfx-linux there): builder/linux/
# build-linux.sh says what it needs on it.
set -e
cd "$(dirname "$0")/.."
ROOT=$PWD
VERSION=${VERSION:-0.99}
NSHVC=$(cd "${NSHVC:-../nativeshvc}" && pwd)
B=$ROOT/build/builder
DIST=$B/dist
TREE=$B/tree
ROM=${ROM:-"$ROOT/reference/stuntracefx/Stunt Race FX (USA) (Rev 1).sfc"}
[ -f "$ROM" ] || { echo "build.sh: no ROM at $ROM (set ROM)" >&2; exit 1; }
echo "Stunt Race FX Builder $VERSION"

# ---- the source: HEAD, as committed ----------------------------------------------------------
REV=${REV:-$(git rev-parse HEAD)}   # (release.sh: the commit it tags)
if [ -d "$TREE" ]; then
    git worktree remove --force "$TREE" 2>/dev/null || rm -rf "$TREE"
    git worktree prune
fi
git worktree add -q --detach "$TREE" "$REV"
G=$TREE/native/stuntracefx
echo "source: $REV"
if [ -n "$(ls -A "$G/assets" | grep -v '^\.gitkeep$')" ]; then
    echo "build.sh: the game's assets folder holds files at $REV; the payload must carry no data" >&2
    exit 1
fi
rm -rf $B/payload-macos $B/payload-windows $B/payload-linux-* $B/gen $B/game-* $B/linux
mkdir -p $DIST $B/payload-macos/ios $B/payload-windows $B/win $B/gen $B/linux

# the game for one system, as nativeshvc's nshvc-build configures it
game() {    # DIR CMAKE-ARGS...
    name=$1 d=$B/$1; shift
    if ! { cmake -S "$G" -B "$d" -G Ninja -DCMAKE_BUILD_TYPE=Release -DNSHVC_ROOT="$NSHVC" "$@" &&
           cmake --build "$d"; } > "$d.log" 2>&1; then
        grep -E "error|Error" "$d.log" | head -20
        echo "build.sh: the game did not build ($d.log)" >&2
        exit 1
    fi
    echo "built $name"
}

# ---- the payload: the Mac (universal) ---------------------------------------------------------
for A in arm64 x86_64; do
    game game-macos-$A -DNSHVC_HOST=macos -DNSHVC_RENDER=metal -DCMAKE_OSX_ARCHITECTURES=$A -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
done
MACAPP=$B/game-macos-arm64/stuntracefx.app/Contents
lipo -create $MACAPP/MacOS/stuntracefx $B/game-macos-x86_64/stuntracefx.app/Contents/MacOS/stuntracefx \
    -output $B/payload-macos/stuntracefx
strip -S $B/payload-macos/stuntracefx
cp $MACAPP/Resources/AppIcon.icns $B/payload-macos/AppIcon.icns
cp -R $MACAPP/Resources/shaders $B/payload-macos/shaders

# ---- the payload: the iPhone (unsigned; the builder adds the data and signs it) ----------------
game game-ios -DNSHVC_HOST=ios -DNSHVC_RENDER=metal -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos \
    -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0
IAPP=$B/payload-macos/ios/app
mkdir -p $IAPP
cp $B/game-ios/stuntracefx.app/stuntracefx "$IAPP/Stunt Race FX"
strip -S "$IAPP/Stunt Race FX"
cp $B/game-ios/stuntracefx.app/Assets.car "$IAPP/"
cp -R $B/game-ios/stuntracefx.app/shaders "$IAPP/shaders"
sed "s/@VERSION@/$VERSION/g" builder/src/mac/ios-Info.plist > "$IAPP/Info.plist"
plutil -lint "$IAPP/Info.plist" >/dev/null
printf 'APPL????' > "$IAPP/PkgInfo"
cp builder/src/mac/ios_sign.sh $B/payload-macos/ios_sign.sh

# ---- the payload: Windows ---------------------------------------------------------------------
game game-windows -DNSHVC_HOST=windows -DNSHVC_RENDER=vulkan -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
    -DNSHVC_VULKAN_INCLUDE="$(brew --prefix)/include"
x86_64-w64-mingw32-strip -o $B/payload-windows/stuntracefx.exe $B/game-windows/stuntracefx.exe
cp -R $B/game-windows/shaders $B/payload-windows/shaders

python3 builder/check_payload.py "$ROM" "$G/tools/assets.csv" $B/payload-macos/stuntracefx "$IAPP/Stunt Race FX" \
    $B/payload-windows/stuntracefx.exe

# ---- the builder's data table: the game's own list of what it reads ---------------------------
python3 - "$G/tools/assets.csv" > $B/gen/cart_table.h <<'EOF'
import csv, sys
rows = [(int(r['start'], 16), int(r['end'], 16)) for r in csv.DictReader(open(sys.argv[1]))]
print('/* cart_table.h, written by builder/build.sh from native/stuntracefx/tools/assets.csv:')
print('   each range [start, end) of the ROM the game reads */')
print('#define SRFX_CART_RANGES %d' % len(rows))
print('static const uint32_t k_srfx_cart[SRFX_CART_RANGES][2] = {')
for s, e in rows:
    print('    { 0x%06X, 0x%06X },' % (s, e))
print('};')
EOF

# ---- the payload and builder: Linux, x86_64 and arm64 -----------------------------------------
# built on a Linux machine against Ubuntu 22.04's C library (builder/linux/), from the same commit
LINUX_HOST=${LINUX_HOST:-strutb@192.168.1.25}
LW=srfx-linux
ssh -o BatchMode=yes $LINUX_HOST "mkdir -p $LW/src/native $LW/nshvc $LW/vk $LW/gen"
rsync -a --delete --exclude build --exclude assets "$G" $LINUX_HOST:$LW/src/native/
rsync -a --delete --exclude build "$TREE/builder" $LINUX_HOST:$LW/src/
rsync -a --delete "$NSHVC/src" "$NSHVC/include" "$NSHVC/cmake" "$NSHVC/tools" "$NSHVC/CMakeLists.txt" $LINUX_HOST:$LW/nshvc/
rsync -aL --delete "$(brew --prefix)/include/vulkan" "$(brew --prefix)/include/vk_video" $LINUX_HOST:$LW/vk/
rsync -a $B/gen/cart_table.h $LINUX_HOST:$LW/gen/
ssh -o BatchMode=yes $LINUX_HOST "mkdir -p $LW/src/native/stuntracefx/assets && sh $LW/src/builder/linux/build-linux.sh $LW $VERSION" \
    > $B/linux/build.log 2>&1 || { tail -30 $B/linux/build.log; echo "build.sh: the Linux build failed ($B/linux/build.log)" >&2; exit 1; }
grep -E "^built|needs glibc" $B/linux/build.log
for A in amd64 arm64; do
    rsync -a $LINUX_HOST:$LW/out-$A/ $B/payload-linux-$A/
done
python3 builder/check_payload.py "$ROM" "$G/tools/assets.csv" $B/payload-linux-amd64/stuntracefx \
    $B/payload-linux-arm64/stuntracefx
rm -f $DIST/StuntRaceFXBuilder-*-Linux-*.tar.gz
for A in amd64 arm64; do
    [ $A = amd64 ] && CPU=x86_64 || CPU=arm64
    L="$B/linux/$CPU/Stunt Race FX Builder"
    mkdir -p "$L/game"
    cp $B/payload-linux-$A/StuntRaceFXBuilder "$L/"
    cp $B/payload-linux-$A/stuntracefx $B/payload-linux-$A/icon.png $B/payload-linux-$A/icon.argb "$L/game/"
    cp -R $B/payload-linux-$A/shaders "$L/game/shaders"
    chmod 755 "$L/StuntRaceFXBuilder" "$L/game/stuntracefx"
    COPYFILE_DISABLE=1 tar -C "$B/linux/$CPU" --no-mac-metadata --uid 0 --gid 0 --uname root --gname root -czf "$DIST/StuntRaceFXBuilder-$VERSION-Linux-$CPU.tar.gz" \
        "Stunt Race FX Builder"
done

# ---- macOS: Stunt Race FX Builder.app ---------------------------------------------------------
APP="$B/mac/Stunt Race FX Builder.app"
rm -rf "$B/mac"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources/payload"
clang -arch arm64 -arch x86_64 -mmacosx-version-min=12.0 -O2 -Wall -Wextra -fobjc-arc -DRB_VERSION="\"$VERSION\"" \
    -Ibuilder/src/core -I$B/gen builder/src/core/*.c builder/src/mac/main.m -framework Cocoa \
    -o "$APP/Contents/MacOS/StuntRaceFXBuilder"
sed "s/@VERSION@/$VERSION/g" builder/src/mac/Info.plist > "$APP/Contents/Info.plist"
printf 'APPL????' > "$APP/Contents/PkgInfo"
cp $B/payload-macos/AppIcon.icns "$APP/Contents/Resources/AppIcon.icns"
cp -R $B/payload-macos/ "$APP/Contents/Resources/payload/"
codesign --force --sign - "$APP/Contents/Resources/payload/stuntracefx"
codesign --force --sign - "$APP"
codesign --verify --deep --strict "$APP"
rm -f $DIST/StuntRaceFXBuilder-*-macOS.zip
ditto -c -k --keepParent "$APP" $DIST/StuntRaceFXBuilder-$VERSION-macOS.zip

# ---- Windows: StuntRaceFXBuilder.exe ----------------------------------------------------------
MAJOR=${VERSION%%.*}
MINOR=$(echo "${VERSION#*.}" | sed 's/^0*\([0-9]\)/\1/')
cat > $B/win/payload.h <<EOF
#define RB_VER_MAJOR $MAJOR
#define RB_VER_MINOR $MINOR
#define RB_VERSION_STR "$VERSION"
EOF
cp $B/game-windows/stuntracefx-icon/icon.ico $B/win/builder.ico
x86_64-w64-mingw32-windres -I$B/win -Ibuilder/src/win builder/src/win/builder.rc -O coff -o $B/win/builder.res.o
rm -rf $DIST/StuntRaceFXBuilder-*-Windows.* "$B/win/Stunt Race FX Builder"
mkdir -p "$B/win/Stunt Race FX Builder/game"
OBJS=
for s in builder/src/core/*.c builder/src/win/main.c; do
    o=$B/win/$(basename $s .c).o
    x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -DUNICODE -D_UNICODE -DRB_VERSION="\"$VERSION\"" -Ibuilder/src/core -I$B/gen \
        -c $s -o $o
    OBJS="$OBJS $o"
done
x86_64-w64-mingw32-gcc -municode -mwindows -static -s -o "$B/win/Stunt Race FX Builder/StuntRaceFXBuilder.exe" \
    $OBJS $B/win/builder.res.o -lcomctl32 -lole32 -lshell32 -luuid
# the game beside it as plain files: the builder copies them, it carries no
# executable inside itself
cp $B/payload-windows/stuntracefx.exe "$B/win/Stunt Race FX Builder/game/"
cp -R $B/payload-windows/shaders "$B/win/Stunt Race FX Builder/game/shaders"
(cd $B/win && zip -qrX "$DIST/StuntRaceFXBuilder-$VERSION-Windows.zip" "Stunt Race FX Builder")

git worktree remove --force "$TREE"
echo $REV > $DIST/REV          # the source these zips are from (release.sh --no-build)
ls -la $DIST
