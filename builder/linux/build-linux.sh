#!/bin/sh
# build-linux.sh: the Linux game and builder for x86_64 and arm64, run on a
# Linux machine by builder/build.sh (over ssh, in the folder it sends).
#
#   WORK/src/        the source at the commit built: native/stuntracefx, builder/
#   WORK/nshvc/      nativeshvc
#   WORK/vk/         Vulkan's headers (vulkan/, vk_video/)
#   WORK/gen/        cart_table.h, the builder's data table
#   WORK/sysroot-*   Ubuntu 22.04's C library per CPU (sysroot.py makes them once)
#
# Out: WORK/out-ARCH/{stuntracefx, shaders/, icon.png, icon.argb, StuntRaceFXBuilder}.
# Needs gcc, gcc-aarch64-linux-gnu, cmake, ninja, python3, glslangValidator and
# spirv-cross on the machine.
#
# Usage: build-linux.sh WORK VERSION
set -e
WORK=$(cd "$1" && pwd)
VERSION=$2
for A in amd64 arm64; do
    [ -d "$WORK/sysroot-$A" ] || python3 "$WORK/src/builder/linux/sysroot.py" $A "$WORK/sysroot-$A"
done
for A in amd64 arm64; do
    if [ $A = arm64 ]; then T=aarch64-linux-gnu-; M=aarch64-linux-gnu; else T=; M=x86_64-linux-gnu; fi
    SYS=$WORK/sysroot-$A
    # gcc searches its own folders before the sysroot's: the build machine's C
    # library (its libm.so; a cross gcc's /usr/aarch64-linux-gnu, headers too),
    # which can be far newer than 2.35. Only gcc's own headers, then the
    # sysroot's; the sysroot's start files and libraries first.
    CFLAGS="-nostdinc -isystem $(${T}gcc -print-file-name=include) -isystem $SYS/usr/include/$M -isystem $SYS/usr/include"
    LDFLAGS="-B$SYS/usr/lib/$M/ -L$SYS/lib/$M -L$SYS/usr/lib/$M"
    D=$WORK/game-$A
    O=$WORK/out-$A
    rm -rf "$D" "$O"
    mkdir -p "$O"
    if ! { cmake -S "$WORK/src/native/stuntracefx" -B "$D" -G Ninja -DCMAKE_BUILD_TYPE=Release \
               -DCMAKE_TOOLCHAIN_FILE="$WORK/src/builder/linux/toolchain.cmake" -DSRFX_ARCH=$A -DSRFX_SYSROOT="$SYS" \
               -DSRFX_CFLAGS="$CFLAGS" -DSRFX_LDFLAGS="$LDFLAGS" \
               -DNSHVC_ROOT="$WORK/nshvc" -DNSHVC_HOST=linux -DNSHVC_RENDER=vulkan -DNSHVC_VULKAN_INCLUDE="$WORK/vk" &&
           cmake --build "$D"; } > "$D.log" 2>&1; then
        grep -E "error|Error" "$D.log" | head -20
        echo "build-linux.sh: the game did not build for $A ($D.log)" >&2
        exit 1
    fi
    ${T}strip -o "$O/stuntracefx" "$D/stuntracefx"
    cp -R "$D/shaders" "$O/shaders"
    cp "$D/icon.png" "$D/icon.argb" "$O/"
    echo "built game-linux-$A"
    ${T}gcc --sysroot="$SYS" $CFLAGS $LDFLAGS -O2 -Wall -Wextra -DRB_VERSION="\"$VERSION\"" -I"$WORK/src/builder/src/core" -I"$WORK/gen" \
        "$WORK"/src/builder/src/core/*.c "$WORK/src/builder/src/linux/main.c" -ldl -lpthread -s \
        -o "$O/StuntRaceFXBuilder"
    echo "built builder-linux-$A"
    # the newest glibc each needs: 2.35 or older, so Linux from 2022 on runs them
    for f in "$O/stuntracefx" "$O/StuntRaceFXBuilder"; do
        v=$(${T}objdump -T "$f" | grep -o 'GLIBC_2\.[0-9]*' | cut -d. -f2 | sort -n | tail -1)
        echo "$(basename "$f") $A needs glibc 2.$v"
        if [ "$v" -gt 35 ]; then echo "build-linux.sh: $f needs glibc 2.$v, newer than 2.35" >&2; exit 1; fi
    done
done
