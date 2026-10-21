flat in uint c0, c1, cs;
out uvec4 o;
void main()
{
    o = uvec4(char_pixel(c0, c1, cs, uint(gl_FragCoord.x), uint(gl_FragCoord.y)), 0u, 0u, 0u);
}
