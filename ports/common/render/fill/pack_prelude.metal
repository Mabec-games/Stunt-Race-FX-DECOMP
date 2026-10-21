#include <metal_stdlib>
using namespace metal;
// Metal: the canvas passed down to each helper
#define CP , texture2d<uint> t_canvas
#define CA , t_canvas
#define FETCH_CANVAS(x, y) t_canvas.read(uint2((x), (y))).r
