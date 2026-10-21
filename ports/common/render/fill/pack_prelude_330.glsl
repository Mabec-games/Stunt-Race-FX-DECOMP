#version 330 core
// OpenGL: the canvas on unit 0
uniform usampler2D t_canvas;
#define CP
#define CA
#define FETCH_CANVAS(x, y) texelFetch(t_canvas, ivec2(int(x), int(y)), 0).r
