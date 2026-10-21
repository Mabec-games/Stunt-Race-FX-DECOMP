layout(location = 0) in uvec4 a_lo;
layout(location = 1) in uvec4 a_hi;
flat out uint w0, w1, w2, w3, w4;
void main()
{
    uint p = span_corner(a_lo.x, uint(gl_VertexID));
    // canvas row 0 at NDC -1: the framebuffer's first row in memory
    gl_Position = vec4(float(p & 0xFFFFu) / 128.0 - 1.0, float(p >> 16) / 128.0 - 1.0, 0.0, 1.0);
    w0 = a_lo.x; w1 = a_lo.y; w2 = a_lo.z; w3 = a_lo.w; w4 = a_hi.x;
}
