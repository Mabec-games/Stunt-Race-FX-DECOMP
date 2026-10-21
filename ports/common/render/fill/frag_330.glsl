flat in uint w0, w1, w2, w3, w4;
out uvec4 o;
void main()
{
    uint c = fill_pixel(w0, w1, w2, w3, w4, uint(gl_FragCoord.x), uint(gl_FragCoord.y));
    if (c == 0u) discard;
    o = uvec4(c, 0u, 0u, 0u);
}
