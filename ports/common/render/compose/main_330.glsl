out vec4 o;
void main()
{
    /* row 0 of the target is the picture's first line (the rows as the
       pictures are uploaded and sampled) */
    uint c = compose_pixel(uint(gl_FragCoord.x), uint(gl_FragCoord.y) + 1u);
    o = vec4(float((c >> 16) & 255u), float((c >> 8) & 255u), float(c & 255u), 255.0) / 255.0;
}
