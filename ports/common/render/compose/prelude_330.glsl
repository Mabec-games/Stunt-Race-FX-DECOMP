#version 330 core
// OpenGL: the record's textures on units 0-3 (rows top first, as uploaded)
uniform usampler2D t_vram;
uniform usampler2D t_pal;
uniform usampler2D t_oam;
uniform usampler2D t_line;
uniform usampler2D t_ext;
uniform usampler2D t_mirror;
uniform usampler2D t_atlas;
#define TP
#define TA
#define FETCH_VRAM_REC(v, w) texelFetch(t_vram, ivec2(int((w) & 255u), int((v) * 128u + ((w) >> 8))), 0).r
#define FETCH_EXT(v, i) texelFetch(t_ext, ivec2(int((i) & 255u), int((v) * 8u + ((i) >> 8))), 0).r
#define FETCH_MIRROR(v, b) texelFetch(t_mirror, ivec2(int((b) & 255u), int((v) * 256u + ((b) >> 8))), 0).r
#define FETCH_ATLAS(v, ax, ay) texelFetch(t_atlas, ivec2(int(ax), int((v) * uint(textureSize(t_atlas, 0).y / 4) + (ay))), 0).r
#define ATLAS_WIDTH uint(textureSize(t_atlas, 0).x)
#define FETCH_PAL(l, i) texelFetch(t_pal, ivec2(int(i), int(l)), 0).r
#define FETCH_OAM(v, i) texelFetch(t_oam, ivec2(int(i), int(v)), 0).r
#define FETCH_LINE(l, i) texelFetch(t_line, ivec2(int(i), int(l)), 0).r
