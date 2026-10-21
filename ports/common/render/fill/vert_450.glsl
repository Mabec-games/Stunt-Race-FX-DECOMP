layout(location = 0) in uvec4 a_lo;
layout(location = 1) in uvec4 a_hi;
layout(location = 0) flat out uint w0;
layout(location = 1) flat out uint w1;
layout(location = 2) flat out uint w2;
layout(location = 3) flat out uint w3;
layout(location = 4) flat out uint w4;
void main()
{
    uint p = span_corner(a_lo.x, uint(gl_VertexIndex));
    // Vulkan's y runs down: canvas row 0 at NDC -1 is the top
    gl_Position = vec4(float(p & 0xFFFFu) / 128.0 - 1.0, float(p >> 16) / 128.0 - 1.0, 0.0, 1.0);
    w0 = a_lo.x; w1 = a_lo.y; w2 = a_lo.z; w3 = a_lo.w; w4 = a_hi.x;
}
