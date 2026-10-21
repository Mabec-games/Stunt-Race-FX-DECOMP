#!/bin/sh
# Build rr_check against each backend that runs here and run it: the
# backends' pictures against the reference, pixel for pixel.
#   check.sh [soft|metal|gl|vulkan ...]    (default: every one)
set -e
cd "$(dirname "$0")/../../../.."
OUT=build/render-check
mkdir -p "$OUT"
R=ports/common/render
H=ports/common/host
INC="-I$R -I$H"
CF="-O2 -Wall -std=gnu11"
VK=${VULKAN_SDK:-$(brew --prefix 2>/dev/null)}
fail=0
for b in ${*:-soft metal gl vulkan}; do
  case "$b" in
    soft)   SRC="$R/soft/rr_soft.c"; LIBS=;;
    metal)  SRC="-fobjc-arc $R/metal/rr_metal.m"; LIBS="-framework Metal -framework QuartzCore -framework Foundation";;
    gl)     SRC="$R/gl/rr_gl.c $R/gl/gl_ctx.c"; LIBS="-framework OpenGL";;
    vulkan) SRC="$R/vulkan/rr_vulkan.c"; INC="$INC -I$VK/include"; LIBS="-L$VK/lib -lvulkan -Wl,-rpath,$VK/lib";;
    *) echo "check.sh: no backend $b" >&2; exit 2;;
  esac
  clang $CF $INC -o "$OUT/rr_check_$b" $R/check/rr_check.c $SRC $H/null/host_null.c $H/posix/host_posix.c $LIBS -lm
  "$OUT/rr_check_$b" || fail=1
done
exit $fail
