#version 450
// Vulkan: the canvas in set 0
layout(set = 0, binding = 0) uniform usampler2D t_canvas;
#define CP
#define CA
#define FETCH_CANVAS(x, y) texelFetch(t_canvas, ivec2(int(x), int(y)), 0).r
