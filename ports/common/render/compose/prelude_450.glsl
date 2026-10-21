#version 450
// Vulkan: the record's textures in set 0
layout(set = 0, binding = 0) uniform usampler2D t_vram;
layout(set = 0, binding = 1) uniform usampler2D t_pal;
layout(set = 0, binding = 2) uniform usampler2D t_oam;
layout(set = 0, binding = 3) uniform usampler2D t_line;
#define TP
#define TA
#define FETCH_VRAM(v, w) texelFetch(t_vram, ivec2(int((w) & 255u), int((v) * 128u + ((w) >> 8))), 0).r
#define FETCH_PAL(l, i) texelFetch(t_pal, ivec2(int(i), int(l)), 0).r
#define FETCH_OAM(v, i) texelFetch(t_oam, ivec2(int(i), int(v)), 0).r
#define FETCH_LINE(l, i) texelFetch(t_line, ivec2(int(i), int(l)), 0).r
