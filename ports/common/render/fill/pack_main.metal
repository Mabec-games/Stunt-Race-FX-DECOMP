struct PO { float4 pos [[position]]; uint r0 [[flat]]; uint r1 [[flat]]; uint r2 [[flat]]; };
vertex PO pack_vs(uint vid [[vertex_id]], uint iid [[instance_id]], const device uint *runs [[buffer(0)]])
{
    PO o;
    const device uint *r = runs + iid * 4u;
    uint p = run_corner(r[0], vid);
    // Metal's y runs up: the copy's row 0 is the top
    o.pos = float4(float(p & 0xFFFFu) / 128.0 - 1.0, 1.0 - float(p >> 16) / 128.0, 0, 1);
    o.r0 = r[0]; o.r1 = r[1]; o.r2 = r[2];
    return o;
}
fragment uint4 pack_fs(PO in [[stage_in]], texture2d<uint> t_canvas [[texture(0)]])
{
    uint j = uint(in.pos.x) - (in.r0 & 255u);
    return uint4(pack_byte(in.r1 + j, in.r2 CA), 0, 0, 0);
}
struct CO2 { float4 pos [[position]]; uint c0 [[flat]]; uint c1 [[flat]]; uint s [[flat]]; };
vertex CO2 char_vs(uint vid [[vertex_id]], uint iid [[instance_id]], const device uint *chars [[buffer(0)]])
{
    CO2 o;
    const device uint *c = chars + iid * 4u;
    uint p = char_corner(c[0], c[2], vid);
    // the atlas is c[3] & $FFFF x c[3] >> 16 pixels; Metal's y runs up
    o.pos = float4(float(p & 0xFFFFu) / float(c[3] & 0xFFFFu) * 2.0 - 1.0, 1.0 - float(p >> 16) / float(c[3] >> 16) * 2.0, 0, 1);
    o.c0 = c[0]; o.c1 = c[1]; o.s = c[2];
    return o;
}
fragment uint4 char_fs(CO2 in [[stage_in]], texture2d<uint> t_canvas [[texture(0)]])
{
    return uint4(char_pixel(in.c0, in.c1, in.s, uint(in.pos.x), uint(in.pos.y) CA), 0, 0, 0);
}
