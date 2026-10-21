struct FO { float4 pos [[position]]; uint w0 [[flat]]; uint w1 [[flat]]; uint w2 [[flat]]; uint w3 [[flat]]; uint w4 [[flat]]; uint w5 [[flat]]; };
vertex FO fill_vs(uint vid [[vertex_id]], uint iid [[instance_id]], const device uint *spans [[buffer(0)]])
{
    FO o;
    const device uint *s = spans + iid * 8u;
    uint p = span_corner(s[0], s[1], vid);
    // Metal's y runs up: canvas row 0 is the top
    o.pos = float4(float(p & 0xFFFFu) / 128.0 - 1.0, 1.0 - float(p >> 16) / 128.0, 0, 1);
    o.w0 = s[0]; o.w1 = s[1]; o.w2 = s[2]; o.w3 = s[3]; o.w4 = s[4]; o.w5 = s[5];
    return o;
}
fragment uint4 fill_fs(FO in [[stage_in]], texture2d<uint> t_rom [[texture(0)]])
{
    uint c = fill_pixel(in.w0, in.w1, in.w2, in.w3, in.w4, in.w5, uint(in.pos.x), uint(in.pos.y) TA);
    if (c == 0u) discard_fragment();
    return uint4(c, 0, 0, 0);
}
