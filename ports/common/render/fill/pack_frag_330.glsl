flat in uint r0, r1, r2;
out uvec4 o;
void main()
{
    uint j = uint(gl_FragCoord.x) - (r0 & 255u);
    o = uvec4(pack_byte(r1 + j, r2), 0u, 0u, 0u);
}
