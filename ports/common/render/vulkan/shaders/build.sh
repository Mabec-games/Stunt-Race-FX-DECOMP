#!/bin/sh
# Compile the shaders to SPIR-V and write them into ../rr_spv.h, which is
# checked in: building the game needs no shader compiler. Run after editing
# quad.vert, copy.frag or sharp.frag (glslangValidator, from glslang or the
# Vulkan SDK).
set -e
cd "$(dirname "$0")"
glslangValidator -V --vn k_quad_vert -o vert.h quad.vert >/dev/null
glslangValidator -V --vn k_copy_frag -o copy.h copy.frag >/dev/null
glslangValidator -V --vn k_sharp_frag -o sharp.h sharp.frag >/dev/null
{
  echo "/* rr_spv.h: SPIR-V of shaders/quad.vert, copy.frag and sharp.frag,"
  echo " * written by shaders/build.sh: do not edit. */"
  for f in vert.h copy.h sharp.h; do sed -n '/^const uint32_t/,$p' $f | sed 's/^const/static const/'; done
} > ../rr_spv.h
rm -f vert.h copy.h sharp.h
echo "wrote rr_spv.h"
