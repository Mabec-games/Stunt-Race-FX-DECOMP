struct CO { float4 pos [[position]]; };
vertex CO compose_vs(uint i [[vertex_id]])
{
    CO o;
    float2 p = float2(float((i << 1) & 2u), float(i & 2u));
    o.pos = float4(p * 2.0 - 1.0, 0, 1);
    return o;
}
fragment float4 compose_fs(CO in [[stage_in]], texture2d<uint> t_vram [[texture(0)]], texture2d<uint> t_pal [[texture(1)]],
                           texture2d<uint> t_oam [[texture(2)]], texture2d<uint> t_line [[texture(3)]],
                           texture2d<uint> t_ext [[texture(4)]], texture2d<uint> t_mirror [[texture(5)]])
{
    uint c = compose_pixel(uint(in.pos.x), uint(in.pos.y) + 1u TA);
    return float4(float((c >> 16) & 255u), float((c >> 8) & 255u), float(c & 255u), 255.0) / 255.0;
}
