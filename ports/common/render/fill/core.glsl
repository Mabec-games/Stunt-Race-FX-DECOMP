// core.glsl: the 3D canvas filled on the GPU from a draw list's spans
// (game/include/draw_list.h FxSpan, packed as rr.h's RrSpan words): the
// shader form of draw_list.c's fx_fill_span, step for step, so the two
// agree on every pixel. Each span is drawn as one row of pixels (two
// pieces when its x wraps past 255), in the list's order, so a later
// span's pixels cover an earlier one's as the Super FX's plots do.
// One source for GLSL and Metal: the prelude for each defines
//   FETCH_ROM(offset)  ROM_BYTES  (the cartridge ROM, 1,024 bytes a row)
// and TP / TA, the ROM as a helper's parameter and argument.

// where the screen's layout puts PLOT's line y: in the 128, 160 and
// 192-line layouts a line past the height is the line y mod the height,
// that many 8-pixel columns to the right (bits 0-15 the line, 16 up the
// shift)
uint span_place(uint w0, uint w1)
{
    uint y = (w0 >> 8) & 255u, lay = (w1 >> 26) & 3u;
    if (lay == 3u) return y;
    uint h = 128u + 32u * lay;
    return (y % h) | ((y / h) * 8u) << 16;
}

// the corner vid (0-11: two pieces, six corners each) of a span's
// rectangle, in canvas pixels: x | y << 16 (a piece that is not needed,
// or the part of one shifted past x 255, has no width)
uint span_corner(uint w0, uint w1, uint vid)
{
    uint q = vid / 6u, c = vid % 6u;
    uint i = c == 0u ? 0u : c == 1u ? 1u : c == 2u ? 2u : c == 3u ? 2u : c == 4u ? 1u : 3u;
    uint place = span_place(w0, w1), y = place & 0xFFFFu, shift = place >> 16;
    uint x0 = w0 & 255u, end = x0 + ((w0 >> 16) & 511u);
    uint a = q == 0u ? x0 : 0u;
    uint b = q == 0u ? min(end, 256u) : (end > 256u ? end - 256u : 0u);
    a = min(a + shift, 256u);
    b = min(b + shift, 256u);
    return ((i & 1u) != 0u ? b : a) | ((i & 2u) != 0u ? y + 1u : y) << 16;
}

// draw_list.c rom_byte: bank:addr in the LoROM / HiROM halves
uint fill_rom_byte(uint bank, uint addr TP)
{
    uint off = bank < 0x40u ? bank * 0x8000u + (addr & 0x7FFFu) : (bank - 0x40u) * 0x10000u + addr;
    return FETCH_ROM(off % ROM_BYTES);
}

// a 16-bit value as a signed integer
int s16(uint v) { return int(v & 0xFFFFu) - int((v & 0x8000u) << 1); }

// the pixel (px, py) of a span: 0x100 | its colour, or 0 when PLOT does
// not write it
uint fill_pixel(uint w0, uint w1, uint w2, uint w3, uint w4, uint w5, uint cx, uint cy TP)
{
    // PLOT's own (x, y), from the canvas pixel the layout put it at
    uint px = (cx - (span_place(w0, w1) >> 16)) & 255u, py = (w0 >> 8) & 255u;
    uint k = (px - (w0 & 255u)) & 255u;
    uint c = w1 & 255u, options = (w1 >> 8) & 31u, kind = (w1 >> 13) & 7u, mode = (w1 >> 16) & 3u;
    uint bank = (w1 >> 18) & 255u, base = w4 & 0xFFFFu;
    if (kind != 0u) {
        uint v;
        if (kind == 1u) {
            // textured: u and v stepped, v's high byte and u's
            uint tu = (w2 + k * (w3 & 0xFFFFu)) & 0xFFFFu;
            uint tv = ((w2 >> 16) + k * (w3 >> 16)) & 0xFFFFu;
            v = fill_rom_byte(bank, ((((tv & 0xFF00u) | (tu >> 8)) & (w4 >> 16)) + base) & 0xFFFFu TA);
        } else if (kind == 2u) {
            // texel runs: the carries of u's 16-bit sum count the texels
            uint i = ((w2 & 0xFFFFu) + k * (w3 & 0xFFFFu)) >> 16;
            v = fill_rom_byte(bank, ((w3 >> 16) != 0u ? base - i : base + i) & 0xFFFFu TA);
        } else if (kind == 3u) {
            // a bitmap's two bits through a palette of four bytes
            uint b = fill_rom_byte(bank, (base + (k >> 2)) & 0xFFFFu TA);
            v = (w5 >> (((b >> ((k & 3u) * 2u)) & 3u) * 8u)) & 255u;
        } else if (kind == 5u) {
            // noise: the generator stepped k + 1 times from the row's seed
            uint r = w2 & 0xFFFFu;
            for (uint j = 0u; j <= k; j++) {
                uint t = ((r & 255u) << 8) ^ r;
                uint q = (((t & 255u) * 2u) ^ (((t << 8) | (t >> 8)) & 0xFFFFu)) & 0xFFFFu;
                if ((q & 1u) != 0u) r = (q >> 1) ^ 0xFF00u ^ 0x8180u;
                else { r = (q >> 1) ^ 0xFF00u; if (r == 0xAA55u) r = 0u; }
            }
            uint i = r & 7u;
            v = i < 4u ? (w5 >> (i * 8u)) & 255u : ((i < 6u ? (w2 >> 16) : (w3 >> 16)) >> ((i & 1u) * 8u)) & 255u;
        } else {
            // a scaled bitmap's: the column by FMULT of the swapped offset
            uint d = (w2 + k) & 0xFFFFu;
            uint col = uint((s16((d << 8) | (d >> 8)) * s16(w3)) >> 16) & 0xFFFFu;
            uint b = fill_rom_byte(bank, (base + uint(s16(col) >> 2)) & 0xFFFFu TA);
            v = (w5 >> (((b >> ((col & 3u) * 2u)) & 3u) * 8u)) & 255u;
        }
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
