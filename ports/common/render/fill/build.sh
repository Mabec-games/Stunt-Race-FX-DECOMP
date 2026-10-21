#!/bin/sh
# The 3D canvas's fill (core.glsl) and pack (pack_core.glsl) for each
# backend, written into ../fill_src.h (Metal's and OpenGL's sources as C
# strings, compiled at start-up) and ../vulkan/fill_spv.h (SPIR-V,
# glslangValidator). Both are checked in: building the game needs no
# shader compiler. Run after editing a core, a prelude or a main.
set -e
cd "$(dirname "$0")"
cstr() { sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/"/' -e 's/$/\\n"/'; }
{
  echo "/* fill_src.h: the 3D canvas's fill, Metal and OpenGL sources, written by"
  echo " * fill/build.sh from core.glsl: do not edit. */"
  echo "static const char k_fill_msl[] ="
  cat prelude.metal core.glsl main.metal | cstr
  echo ";"
  echo "static const char k_fill_gl_vert[] ="
  cat prelude_330.glsl core.glsl vert_330.glsl | cstr
  echo ";"
  echo "static const char k_fill_gl_frag[] ="
  cat prelude_330.glsl core.glsl frag_330.glsl | cstr
  echo ";"
  echo "static const char k_pack_msl[] ="
  cat pack_prelude.metal pack_core.glsl pack_main.metal | cstr
  echo ";"
  echo "static const char k_pack_gl_vert[] ="
  cat pack_prelude_330.glsl pack_core.glsl pack_vert_330.glsl | cstr
  echo ";"
  echo "static const char k_pack_gl_frag[] ="
  cat pack_prelude_330.glsl pack_core.glsl pack_frag_330.glsl | cstr
  echo ";"
  echo "static const char k_char_gl_vert[] ="
  cat pack_prelude_330.glsl pack_core.glsl char_vert_330.glsl | cstr
  echo ";"
  echo "static const char k_char_gl_frag[] ="
  cat pack_prelude_330.glsl pack_core.glsl char_frag_330.glsl | cstr
  echo ";"
} > ../fill_src.h
cat prelude_450.glsl core.glsl vert_450.glsl > fill.vert
cat prelude_450.glsl core.glsl frag_450.glsl > fill.frag
glslangValidator -V --vn k_fill_vert -o vert.h fill.vert >/dev/null
glslangValidator -V --vn k_fill_frag -o frag.h fill.frag >/dev/null
cat pack_prelude_450.glsl pack_core.glsl pack_vert_450.glsl > pack.vert
cat pack_prelude_450.glsl pack_core.glsl pack_frag_450.glsl > pack.frag
glslangValidator -V --vn k_pack_vert -o pvert.h pack.vert >/dev/null
glslangValidator -V --vn k_pack_frag -o pfrag.h pack.frag >/dev/null
cat pack_prelude_450.glsl pack_core.glsl char_vert_450.glsl > char.vert
cat pack_prelude_450.glsl pack_core.glsl char_frag_450.glsl > char.frag
glslangValidator -V --vn k_char_vert -o cvert.h char.vert >/dev/null
glslangValidator -V --vn k_char_frag -o cfrag.h char.frag >/dev/null
{
  echo "/* fill_spv.h: SPIR-V of fill/ (the 3D canvas's fill), written by"
  echo " * fill/build.sh: do not edit. */"
  for f in vert.h frag.h pvert.h pfrag.h cvert.h cfrag.h; do sed -n '/^const uint32_t/,$p' $f | sed 's/^const/static const/'; done
} > ../vulkan/fill_spv.h
rm -f vert.h frag.h pvert.h pfrag.h cvert.h cfrag.h fill.vert fill.frag pack.vert pack.frag char.vert char.frag
echo "wrote fill_src.h, vulkan/fill_spv.h"
