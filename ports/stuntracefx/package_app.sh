#!/bin/sh
# Package the macOS build as build/stuntracefx/port/macos/Stunt Race FX.app:
# the executable (the game's data built in from SRFX_ROM, see link.sh) and
# an Info.plist. The app needs no ROM and nothing from this tree.
set -e
cd "$(dirname "$0")/../.."
OUT=build/stuntracefx/port/macos
HOST=macos OUT=$OUT ports/stuntracefx/link.sh
APP="$OUT/Stunt Race FX.app"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
cp "$OUT/stuntracefx" "$APP/Contents/MacOS/Stunt Race FX"
cat > "$APP/Contents/Info.plist" <<'EOP'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleName</key><string>Stunt Race FX</string>
  <key>CFBundleDisplayName</key><string>Stunt Race FX</string>
  <key>CFBundleExecutable</key><string>Stunt Race FX</string>
  <key>CFBundleIdentifier</key><string>com.strutb.stuntracefx</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>1.0</string>
  <key>CFBundleVersion</key><string>1</string>
  <key>LSMinimumSystemVersion</key><string>11.0</string>
  <key>NSHighResolutionCapable</key><true/>
  <key>LSApplicationCategoryType</key><string>public.app-category.games</string>
</dict>
</plist>
EOP
codesign --force --sign - "$APP" >/dev/null 2>&1 || true
echo "package: $APP"
