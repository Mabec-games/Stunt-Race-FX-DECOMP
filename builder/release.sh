#!/bin/sh
# builder/release.sh: build Stunt Race FX Builder for macOS, Windows and Linux
# and publish them on the repository's GitHub Releases page as vVERSION.
#
# VERSION: 0.99 unless set. The release is tagged at HEAD as it is when this
# starts, which must already be on GitHub. Running it again at the same
# version replaces the files of that release.
#
# --no-build publishes the zips already in build/builder/dist, which must
# have been built from HEAD (build.sh writes the commit to dist/REV): for
# zips built and checked before the release.
#
# Usage: builder/release.sh [--dry-run] [--no-build]
set -e
cd "$(dirname "$0")/.."
DRY=0 BUILD=1
for a in "$@"; do
    [ "$a" = --dry-run ] && DRY=1
    [ "$a" = --no-build ] && BUILD=0
done

VERSION=${VERSION:-0.99}
TAG=v$VERSION
DIST=build/builder/dist
MAC=$DIST/StuntRaceFXBuilder-$VERSION-macOS.zip
WIN=$DIST/StuntRaceFXBuilder-$VERSION-Windows.zip
LINUX_X64=$DIST/StuntRaceFXBuilder-$VERSION-Linux-x86_64.tar.gz
LINUX_ARM=$DIST/StuntRaceFXBuilder-$VERSION-Linux-arm64.tar.gz

if [ -n "$(git status --porcelain -- builder native/stuntracefx)" ]; then
    echo "release: builder/ or native/stuntracefx/ has uncommitted changes; commit them first" >&2
    exit 1
fi
git fetch -q origin
REV=$(git rev-parse HEAD)          # the commit checked here is the one tagged, whatever lands meanwhile
if [ $DRY = 0 ] && ! git merge-base --is-ancestor $REV origin/main; then
    echo "release: HEAD is not on origin/main; push it first" >&2
    exit 1
fi

if [ $BUILD = 1 ]; then
    VERSION=$VERSION REV=$REV builder/build.sh
elif [ "$(cat $DIST/REV 2>/dev/null)" != "$REV" ] || [ ! -f $MAC ] || [ ! -f $WIN ] || [ ! -f $LINUX_X64 ] || [ ! -f $LINUX_ARM ]; then
    echo "release: the zips in $DIST are not built from HEAD ($REV); build them first" >&2
    exit 1
fi

NOTES=$(mktemp)
cat > $NOTES <<EOF
Stunt Race FX Builder makes a native build of **Stunt Race FX** (Super Nintendo, 1994) from your own copy of the cartridge: the game as C, compiled for your machine and running on nativeshvc, with everything drawn by the GPU. On a Mac it builds the game for the Mac, or for the iPhone and iPad; on Windows, for Windows; on Linux, for Linux.

The game's data is copyrighted, so the builder does not include it: you provide your ROM, the builder checks it by MD5, takes the data the game reads from it (the ROM's code is not used: the C stands for it), and produces a game that runs on its own. The ROM is not needed once the build is done.

### Features
- **Native**: the game's own C for your machine, no emulator; its 3D is drawn by the GPU (Metal on the Mac, iPhone and iPad, Vulkan on Windows and Linux).
- **Any window, any shape**: the race and the menus fill the window, wide or tall, and the 3D is drawn at the window's own resolution, sharp at any size.
- **60 frames a second**: the race runs the cartridge's own physics step for step, and the 3D is drawn on every frame between its steps.
- **The courses' ground as polygon scenery**: green polygon mountains, oceans of waves, snowfields, to the horizon; solid 3D tyres and trees, and no distance where the scenery is cut off.
- **Its own pause screen** (Escape, Select + Start, or the corner button): remap the keyboard and a game controller, and turn Auto-Accelerate on or off; on the iPhone and iPad, a menu to resume, turn Auto-Accelerate on or off, or quit the race.
- **A view from above**: the view button beside the pause button switches the race between the view behind the car and an angled view from above.
- **Depth-tested 3D**: the courses, cars and scenery drawn at their true depths, the cars riding on the track itself.
- **Touch controls** on the iPhone and iPad: a steering wheel under your right thumb, brake and boost pedals under your left, the gas held for you while you drive, a pause button; swipes and taps in the menus.
- **Game controllers** and the keyboard, on every system.
- **Records kept** as the cartridge's battery saved them, in your user folder, so a rebuild never loses them.

### Download
- **macOS** (12 or later, Apple silicon and Intel; the game it builds needs the same): \`StuntRaceFXBuilder-$VERSION-macOS.zip\`. Unzip and open Stunt Race FX Builder. It is not notarized: if macOS refuses to open it, Control-click it and choose Open, or allow it in System Settings, Privacy & Security.
- **Windows** (10 or later, 64-bit; the game it builds needs the same, and a graphics driver with Vulkan): \`StuntRaceFXBuilder-$VERSION-Windows.zip\`. Unzip it and run \`StuntRaceFXBuilder.exe\` from the unzipped folder (the \`game\` folder beside it holds the game's code). It is not signed: if SmartScreen stops it, choose More info, then Run anyway.
- **Linux** (x86_64 or arm64; glibc 2.35 or later, which is Ubuntu 22.04, Debian 12, Fedora 36 and anything newer; X11 or XWayland; a graphics driver with Vulkan; GTK 3 for the builder's window): \`StuntRaceFXBuilder-$VERSION-Linux-x86_64.tar.gz\` or \`StuntRaceFXBuilder-$VERSION-Linux-arm64.tar.gz\`. Extract it and run \`StuntRaceFXBuilder\` in the extracted folder (the \`game\` folder beside it holds the game's code). Without a desktop it builds from the terminal: \`./StuntRaceFXBuilder --rom YOUR_ROM.sfc --dest FOLDER --build\`.

### What it needs
| Your dump | MD5 |
|---|---|
| The No-Intro ROM of Stunt Race FX (USA) (Rev 1), \`.sfc\` (a \`.smc\` with a copier's 512-byte header is accepted) | \`128b316a74caf17fdc216b3ab46d4a9a\` |

The first release (Rev 0) and the European and Japanese cartridges are not supported yet.

### What it makes
- Mac: \`Stunt Race FX.app\`.
- iPhone and iPad (iOS 15 or later), from the Mac builder: \`Stunt Race FX.ipa\`. When your Mac has Xcode signing (a development certificate with a provisioning profile for it), the .ipa is signed with it, ready to install from Xcode (Devices and Simulators) or Apple Configurator. Otherwise the builder says so in red and the .ipa is unsigned, for you to sign and install with a tool such as AltStore or Sideloadly.
- Windows: a folder, \`Stunt Race FX\`, with \`Stunt Race FX.exe\` in it.
- Linux: a folder, \`Stunt Race FX\`, with the program \`Stunt Race FX\` in it, and \`Stunt Race FX.desktop\` to copy to \`~/.local/share/applications\` to list the game with your other programs.

### Playing
- A keyboard or a game controller drives; on the iPhone and iPad, touch, or a game controller. The pause screen shows and changes what each key and button does (on a computer).
- Full screen: View, Enter Full Screen (Control-Command-F) on a Mac, Alt+Enter or F11 on Windows.
- Records and settings live in your user folder (Application Support on macOS, the iPhone and the iPad, AppData on Windows, ~/.local/share on Linux), so rebuilding never touches them.

Source: [builder/](https://github.com/jeff-strutb/stuntracefx/tree/$REV/builder), [native/stuntracefx/](https://github.com/jeff-strutb/stuntracefx/tree/$REV/native/stuntracefx).
EOF

if [ $DRY = 1 ]; then
    cat $NOTES
    ls -la $MAC $WIN $LINUX_X64 $LINUX_ARM
    rm -f $NOTES
    exit 0
fi
if gh release view $TAG >/dev/null 2>&1; then
    # the same version again: its tag moves to the source these files are from
    git tag -f $TAG $REV >/dev/null
    git push -q -f origin refs/tags/$TAG
    gh release upload $TAG $MAC $WIN $LINUX_X64 $LINUX_ARM --clobber
    gh release edit $TAG --notes-file $NOTES
else
    gh release create $TAG $MAC $WIN $LINUX_X64 $LINUX_ARM --target "$REV" --title "Stunt Race FX Builder $VERSION" --notes-file $NOTES
fi
rm -f $NOTES
gh release view $TAG --json url -q .url
