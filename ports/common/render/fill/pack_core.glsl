// pack_core.glsl: the 3D canvas copied into the compositor's copy of the
// tiles as the Super FX's screen lays it out in its RAM (rr.h rr_pack):
// each byte one bit plane of eight pixels across, PLOT's layout. One
// source for GLSL and Metal: the prelude for each defines
//   FETCH_CANVAS(x, y)  (the canvas: 0x100 | colour, or 0)
// and CP / CA, the canvas as a helper's parameter and argument.

// the corner vid (0-5) of run word r0's rectangle in the copy (256 bytes
// a row): x | y << 16
uint run_corner(uint r0, uint vid)
{
    uint i = vid == 0u ? 0u : vid == 1u ? 1u : vid == 2u ? 2u : vid == 3u ? 2u : vid == 4u ? 1u : 3u;
    uint b = r0 & 0xFFFFu, x = b & 255u, y = b >> 8;
    return ((i & 1u) != 0u ? x + ((r0 >> 16) & 511u) : x) | ((i & 2u) != 0u ? y + 1u : y) << 16;
}

// the byte at the screen's offset o: bit plane p of the pixels from
// (x0, y), the colour depth and layout from word s
uint pack_byte(uint o, uint s CP)
{
    uint lay = s & 3u, md = (s >> 2) & 3u, bpp = 2u << (md - (md >> 1));
    uint cn = o / (bpp * 8u), k = o % (bpp * 8u), plane = (k >> 4) * 2u + (k & 1u);
    uint rows = lay == 0u ? 16u : lay == 1u ? 20u : 24u;
    uint x0 = cn / rows * 8u, y = cn % rows * 8u + ((k & 15u) >> 1), v = 0u;
    if (x0 >= 256u || y >= 256u) return 0u;
    for (uint i = 0u; i < 8u; i++)
        v |= ((FETCH_CANVAS(x0 + i, y) >> plane) & 1u) << (7u - i);
    return v;
}

// The characters' atlas (rr.h rr_chars): each character the screen
// upload completes copied from the canvas, 8 x 8 pixels (8s x 8s at
// scale s), into its slot (the VRAM byte address / 32), 64 slots a row.

// the corner vid (0-5) of character word c0's slot in the atlas: x | y << 16
uint char_corner(uint c0, uint s, uint vid)
{
    uint i = vid == 0u ? 0u : vid == 1u ? 1u : vid == 2u ? 2u : vid == 3u ? 2u : vid == 4u ? 1u : 3u;
    uint slot = c0 & 0xFFFFu, side = 8u * s;
    uint x = (slot & 63u) * side, y = (slot >> 6) * side;
    return ((i & 1u) != 0u ? x + side : x) | ((i & 2u) != 0u ? y + side : y) << 16;
}

// the atlas pixel (ax, ay) of character words c0, c1: 0x100 | the colour
// of the canvas pixel it copies, or 0 for a character the upload did not
// make whole (c0 bit 31)
uint char_pixel(uint c0, uint c1, uint s, uint ax, uint ay CP)
{
    uint side = 8u * s, slot = c0 & 0xFFFFu;
    uint lx = ax - (slot & 63u) * side, ly = ay - (slot >> 6) * side;
    if ((c0 & 0x80000000u) != 0u) return 0u;
    return 0x100u | (FETCH_CANVAS((c1 & 0xFFFFu) * s + lx, (c1 >> 16) * s + ly) & 255u);
}
