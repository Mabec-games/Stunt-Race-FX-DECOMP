layout(location = 0) flat in uint w0;
layout(location = 1) flat in uint w1;
layout(location = 2) flat in uint w2;
layout(location = 3) flat in uint w3;
layout(location = 4) flat in uint w4;
layout(location = 0) out uvec4 o;
void main()
{
    uint c = fill_pixel(w0, w1, w2, w3, w4, uint(gl_FragCoord.x), uint(gl_FragCoord.y));
    if (c == 0u) discard;
    o = uvec4(c, 0u, 0u, 0u);
}
