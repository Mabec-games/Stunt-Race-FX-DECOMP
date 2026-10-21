layout(location = 0) flat in uint r0;
layout(location = 1) flat in uint r1;
layout(location = 2) flat in uint r2;
layout(location = 0) out uvec4 o;
void main()
{
    uint j = uint(gl_FragCoord.x) - (r0 & 255u);
    o = uvec4(pack_byte(r1 + j, r2), 0u, 0u, 0u);
}
