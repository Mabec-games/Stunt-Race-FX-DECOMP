#include <metal_stdlib>
using namespace metal;
// Metal: the record's textures passed down to each helper
#define TP , texture2d<uint> t_vram, texture2d<uint> t_pal, texture2d<uint> t_oam, texture2d<uint> t_line
#define TA , t_vram, t_pal, t_oam, t_line
#define FETCH_VRAM(v, w) t_vram.read(uint2((w) & 255u, (v) * 128u + ((w) >> 8))).r
#define FETCH_PAL(l, i) t_pal.read(uint2((i), (l))).r
#define FETCH_OAM(v, i) t_oam.read(uint2((i), (v))).r
#define FETCH_LINE(l, i) t_line.read(uint2((i), (l))).r
