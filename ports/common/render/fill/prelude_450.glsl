#version 450
// Vulkan: the ROM in set 0
layout(set = 0, binding = 0) uniform usampler2D t_rom;
#define TP
#define TA
#define FETCH_ROM(o) texelFetch(t_rom, ivec2(int((o) & 1023u), int((o) >> 10)), 0).r
#define ROM_BYTES uint(textureSize(t_rom, 0).x * textureSize(t_rom, 0).y)
