#!/bin/sh
# The compositor (core.glsl) for each backend, written into ../compose_src.h
# (Metal's and OpenGL's sources as C strings, compiled at start-up) and
# ../vulkan/compose_spv.h (SPIR-V, glslangValidator). Both are checked in:
# building the game needs no shader compiler. Run after editing core.glsl
# or a prelude.
set -e
cd "$(dirname "$0")"
cstr() { sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/"/' -e 's/$/\\n"/'; }
{
  echo "/* compose_src.h: the compositor's Metal and OpenGL sources, written by"
  echo " * compose/build.sh from core.glsl: do not edit. */"
  echo "static const char k_compose_msl[] ="
  cat prelude.metal core.glsl main.metal | cstr
  echo ";"
  echo "static const char k_compose_gl[] ="
  cat prelude_330.glsl core.glsl main_330.glsl | cstr
  echo ";"
} > ../compose_src.h
cat prelude_450.glsl core.glsl main_450.glsl > compose.frag
glslangValidator -V --vn k_compose_frag -o frag.h compose.frag >/dev/null
glslangValidator -V --vn k_fullscreen_vert -o vert.h fullscreen_450.vert >/dev/null
{
  echo "/* compose_spv.h: SPIR-V of compose/ (the compositor), written by"
  echo " * compose/build.sh: do not edit. */"
  for f in vert.h frag.h; do sed -n '/^const uint32_t/,$p' $f | sed 's/^const/static const/'; done
} > ../vulkan/compose_spv.h
rm -f frag.h vert.h compose.frag
echo "wrote compose_src.h, vulkan/compose_spv.h"
