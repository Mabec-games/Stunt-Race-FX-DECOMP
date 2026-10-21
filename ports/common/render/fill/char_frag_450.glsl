layout(location = 0) flat in uint c0;
layout(location = 1) flat in uint c1;
layout(location = 2) flat in uint cs;
layout(location = 0) out uvec4 o;
void main()
{
    o = uvec4(char_pixel(c0, c1, cs, uint(gl_FragCoord.x), uint(gl_FragCoord.y)), 0u, 0u, 0u);
}
