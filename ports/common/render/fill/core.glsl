// core.glsl: the 3D canvas filled on the GPU from a draw list's spans
// (game/include/draw_list.h FxSpan, packed as rr.h's RrSpan words): the
// shader form of draw_list.c's fx_fill_span, step for step, so the two
// agree on every pixel. Each span is drawn as one row of pixels (two
// pieces when its x wraps past 255), in the list's order, so a later
// span's pixels cover an earlier one's as the Super FX's plots do.
// One source for GLSL and Metal: the prelude for each defines
//   FETCH_ROM(offset)  ROM_BYTES  (the cartridge ROM, 1,024 bytes a row)
// and TP / TA, the ROM as a helper's parameter and argument.

// the corner vid (0-11: two pieces, six corners each) of span word w0's
// rectangle, in canvas pixels: x | y << 16 (a piece that is not needed
// has no width)
uint span_corner(uint w0, uint vid)
{
    uint q = vid / 6u, c = vid % 6u;
    uint i = c == 0u ? 0u : c == 1u ? 1u : c == 2u ? 2u : c == 3u ? 2u : c == 4u ? 1u : 3u;
    uint x0 = w0 & 255u, y = (w0 >> 8) & 255u, end = x0 + ((w0 >> 16) & 511u);
    uint a = q == 0u ? x0 : 0u;
    uint b = q == 0u ? min(end, 256u) : (end > 256u ? end - 256u : 0u);
    return ((i & 1u) != 0u ? b : a) | ((i & 2u) != 0u ? y + 1u : y) << 16;
}

// draw_list.c rom_byte: bank:addr in the LoROM / HiROM halves
uint fill_rom_byte(uint bank, uint addr TP)
{
    uint off = bank < 0x40u ? bank * 0x8000u + (addr & 0x7FFFu) : (bank - 0x40u) * 0x10000u + addr;
    return FETCH_ROM(off % ROM_BYTES);
}

// the pixel (px, py) of a span: 0x100 | its colour, or 0 when PLOT does
// not write it
uint fill_pixel(uint w0, uint w1, uint w2, uint w3, uint w4, uint px, uint py TP)
{
    uint k = (px - (w0 & 255u)) & 255u;
    uint c = w1 & 255u, options = (w1 >> 8) & 31u, mode = (w1 >> 14) & 3u;
    if (((w1 >> 13) & 1u) != 0u) {
        uint tu = (w2 + k * (w3 & 0xFFFFu)) & 0xFFFFu;
        uint tv = ((w2 >> 16) + k * (w3 >> 16)) & 0xFFFFu;
        uint at = ((((tv & 0xFF00u) | (tu >> 8)) & (w4 >> 16)) + (w4 & 0xFFFFu)) & 0xFFFFu;
        uint v = fill_rom_byte((w1 >> 16) & 255u, at TA);
        // COLOR: the source's high nibble into the low one, or its low
        // nibble with the high one kept, or the byte
        if ((options & 4u) != 0u) c = (c & 0xF0u) | (v >> 4);
        else if ((options & 8u) != 0u) c = (c & 0xF0u) | (v & 0x0Fu);
        else c = v;
    }
    // PLOT: colour 0 not written unless the options say so; dither
    if ((options & 1u) == 0u) {
        if (mode == 3u) {
            if ((options & 8u) != 0u) {
                if ((c & 0x0Fu) == 0u) return 0u;
            } else if (c == 0u) {
                return 0u;
            }
        } else if ((c & 0x0Fu) == 0u) {
            return 0u;
        }
    }
    if ((options & 2u) != 0u && mode != 3u) {
        if (((px ^ py) & 1u) != 0u) c >>= 4;
        c &= 0x0Fu;
    }
    return 0x100u | c;
}
