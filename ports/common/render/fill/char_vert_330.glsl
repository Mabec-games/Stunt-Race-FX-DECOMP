layout(location = 0) in uvec4 a_char;
flat out uint c0, c1, cs;
void main()
{
    // the scale, then the atlas's width | height << 16, in the character's words
    uint p = char_corner(a_char.x, a_char.z, uint(gl_VertexID));
    gl_Position = vec4(float(p & 0xFFFFu) / float(a_char.w & 0xFFFFu) * 2.0 - 1.0, float(p >> 16) / float(a_char.w >> 16) * 2.0 - 1.0, 0.0, 1.0);
    c0 = a_char.x; c1 = a_char.y; cs = a_char.z;
}
