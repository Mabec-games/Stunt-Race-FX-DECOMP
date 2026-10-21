#include <metal_stdlib>
using namespace metal;
// Metal: the record's textures passed down to each helper
#define TP , texture2d<uint> t_vram, texture2d<uint> t_pal, texture2d<uint> t_oam, texture2d<uint> t_line, texture2d<uint> t_ext, texture2d<uint> t_mirror
#define TA , t_vram, t_pal, t_oam, t_line, t_ext, t_mirror
#define FETCH_VRAM_REC(v, w) t_vram.read(uint2((w) & 255u, (v) * 128u + ((w) >> 8))).r
#define FETCH_EXT(v, i) t_ext.read(uint2((i) & 255u, (v) * 8u + ((i) >> 8))).r
#define FETCH_MIRROR(v, b) t_mirror.read(uint2((b) & 255u, (v) * 256u + ((b) >> 8))).r
#define FETCH_PAL(l, i) t_pal.read(uint2((i), (l))).r
#define FETCH_OAM(v, i) t_oam.read(uint2((i), (v))).r
#define FETCH_LINE(l, i) t_line.read(uint2((i), (l))).r
