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
