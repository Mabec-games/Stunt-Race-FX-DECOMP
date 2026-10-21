// core.glsl: the picture composed on the GPU, one pixel at a time, from a
// frame's record (ports/common/video/video.h's VidFrame): the shader form
// of video.c's drawing, step for step, so the two agree on every pixel.
// One source for GLSL and Metal: the prelude for each defines
//   FETCH_VRAM_REC(version, word)  FETCH_PAL(line, index)
//   FETCH_OAM(version, byte)       FETCH_LINE(line, word)
//   FETCH_EXT(version, word)       FETCH_MIRROR(version, byte)
// (integer textures: the tiles 256 words a row, 128 rows a copy; the
// palettes a row a line; the sprite tables 544 bytes a row a copy; the
// lines' packed state, VID_LINE_WORDS a row; the bits of the tiles' bytes
// that came from the 3D canvas, 2,048 words a copy, 256 a row; the
// compositor's copies of those bytes, 256 a row, 256 rows a copy) and TP / TA, the textures
// as a helper's parameters and arguments (empty where they are global).

uint lw(uint line, uint i TP) { return FETCH_LINE(line, i); }

// a word of the tiles: each byte from the record, or from the canvas's
// copy where it came from the 3D canvas
uint vram_word(uint ver, uint w TP)
{
    uint e = (FETCH_EXT(ver, w >> 4) >> ((w & 15u) * 2u)) & 3u, r = FETCH_VRAM_REC(ver, w);
    if ((e & 1u) != 0u) r = (r & 0xFF00u) | FETCH_MIRROR(ver, w * 2u);
    if ((e & 2u) != 0u) r = (r & 0x00FFu) | FETCH_MIRROR(ver, w * 2u + 1u) << 8;
    return r;
}
#define FETCH_VRAM(v, w) vram_word(v, w TA)
uint byte_of(uint w, uint k) { return (w >> (k * 8u)) & 255u; }

// the depths of each layer's two priorities and the sprites' four
uint bg_depth(uint mode, uint bgmode, uint n, uint prio)
{
    uint z = 0u;
    if (mode == 0u) {
        z = n == 0u ? (prio != 0u ? 11u : 8u) : n == 1u ? (prio != 0u ? 10u : 7u) : n == 2u ? (prio != 0u ? 5u : 2u) : (prio != 0u ? 4u : 1u);
    } else if (mode == 1u) {
        if (n == 0u) z = prio != 0u ? 9u : 6u;
        else if (n == 1u) z = prio != 0u ? 8u : 5u;
        else z = prio != 0u ? ((bgmode & 8u) != 0u ? 11u : 3u) : 1u;
    } else if (mode == 7u) {
        z = n == 0u ? 3u : (prio != 0u ? 5u : 1u);
    } else {
        z = n == 0u ? (prio != 0u ? 7u : 3u) : (prio != 0u ? 5u : 1u);
    }
    return z;
}

uint obj_depth(uint mode, uint prio)
{
    if (mode == 0u) return prio == 0u ? 3u : prio == 1u ? 6u : prio == 2u ? 9u : 12u;
    if (mode == 1u) return prio == 0u ? 2u : prio == 1u ? 4u : prio == 2u ? 7u : 10u;
    if (mode == 7u) return prio == 0u ? 2u : prio == 1u ? 4u : prio == 2u ? 6u : 7u;
    return prio == 0u ? 2u : prio == 1u ? 4u : prio == 2u ? 6u : 8u;
}

uint mode_bpp(uint mode, uint n)
{
    if (mode == 0u) return 2u;
    if (mode == 1u) return n < 2u ? 4u : n == 2u ? 2u : 0u;
    if (mode == 2u) return n < 2u ? 4u : 0u;
    if (mode == 3u) return n == 0u ? 8u : n == 1u ? 4u : 0u;
    if (mode == 4u) return n == 0u ? 8u : n == 1u ? 2u : 0u;
    if (mode == 5u) return n == 0u ? 4u : n == 1u ? 2u : 0u;
    if (mode == 6u) return n == 0u ? 4u : 0u;
    return n == 0u ? 8u : 0u;
}

uint direct_color(uint c, uint pal)
{
    uint r = (c & 7u) << 2 | (pal & 1u) << 1;
    uint g = (c & 0x38u) >> 1 | (pal & 2u);
    uint b = (c & 0xC0u) >> 3 | (pal & 4u);
    return r | g << 5 | b << 10;
}

uint map_entry(uint ver, uint sc, uint tx, uint ty TP)
{
    uint base = (sc & 0xFCu) << 8;
    uint off = (ty & 31u) << 5 | (tx & 31u);
    if ((tx & 32u) != 0u && (sc & 1u) != 0u) off += 0x400u;
    if ((ty & 32u) != 0u && (sc & 2u) != 0u) off += (sc & 1u) != 0u ? 0x800u : 0x400u;
    return FETCH_VRAM(ver, (base + off) & 0x7FFFu);
}

uint tile_pixel(uint ver, uint ca, uint y, uint x, uint bpp TP)
{
    uint bit = 7u - x, c = 0u;
    for (uint p = 0u; p < bpp; p += 2u) {
        uint w = FETCH_VRAM(ver, (ca + p * 4u + y) & 0x7FFFu);
        c |= ((w >> bit) & 1u) << p;
        c |= ((w >> (bit + 8u)) & 1u) << (p + 1u);
    }
    return c;
}

// a background's pixel: colour | priority << 16 | opaque << 17
uint bg_pixel(uint line, uint ver, uint n, uint mode, uint x TP)
{
    uint w0 = lw(line, 0u TA), w1 = lw(line, 1u TA), w2 = lw(line, 2u TA), w6 = lw(line, 6u TA);
    uint bgmode = byte_of(w0, 2u), mosaic = byte_of(w0, 3u);
    uint bpp = mode_bpp(mode, n), big = (bgmode >> (4u + n)) & 1u, tshift = big != 0u ? 4u : 3u;
    uint msize = (mosaic & (1u << n)) != 0u ? (mosaic >> 4) + 1u : 1u;
    uint y = msize > 1u ? byte_of(lw(line, 12u TA), 3u) : line;
    uint nbaw = n < 2u ? byte_of(w2, 0u) : byte_of(w2, 1u);
    uint nba = ((nbaw >> ((n & 1u) * 4u)) & 0x0Fu) << 12;
    bool opt = mode == 2u || mode == 4u || mode == 6u;
    bool hires = mode == 5u || mode == 6u;
    uint sx = msize > 1u ? x - x % msize : x;
    uint hofs = (lw(line, 7u + n / 2u TA) >> ((n & 1u) * 16u)) & 0xFFFFu;
    uint vofs = (lw(line, 9u + n / 2u TA) >> ((n & 1u) * 16u)) & 0xFFFFu;
    uint sc = byte_of(w1, n);
    if (hires) sx <<= 1;
    if (opt && n < 2u) {
        uint col = (sx + (hofs & 7u)) >> 3;
        if (col > 0u) {
            uint h3 = lw(line, 8u TA) & 0xFFFFu, v3 = lw(line, 10u TA) & 0xFFFFu, sc3 = byte_of(w1, 2u);
            uint bx = ((col - 1u) * 8u + (h3 & ~7u)) >> 3, by = v3 >> 3;
            uint hv = map_entry(ver, sc3, bx, by TA);
            if (mode == 4u) {
                if ((hv & 0x8000u) != 0u) {
                    if ((hv & (0x2000u << n)) != 0u) vofs = hv & 0x3FFu;
                } else if ((hv & (0x2000u << n)) != 0u) {
                    hofs = (hofs & 7u) | (hv & 0x3F8u);
                }
            } else {
                uint vv = map_entry(ver, sc3, bx, by + 1u TA);
                if ((hv & (0x2000u << n)) != 0u) hofs = (hofs & 7u) | (hv & 0x3F8u);
                if ((vv & (0x2000u << n)) != 0u) vofs = vv & 0x3FFu;
            }
        }
    }
    uint xx = (sx + hofs) & 0x3FFu, yy = (y + vofs) & 0x3FFu;
    uint e = map_entry(ver, sc, xx >> tshift, yy >> tshift TA);
    uint px = xx & ((1u << tshift) - 1u), py = yy & ((1u << tshift) - 1u);
    if ((e & 0x4000u) != 0u) px = (1u << tshift) - 1u - px;
    if ((e & 0x8000u) != 0u) py = (1u << tshift) - 1u - py;
    uint ch = e & 0x3FFu;
    if (big != 0u) {
        if (px >= 8u) ch += 1u;
        if (py >= 8u) ch += 16u;
    }
    uint ca = (nba + (ch & 0x3FFu) * (bpp * 4u)) & 0xFFFFu;
    uint c = tile_pixel(ver, ca, py & 7u, px & 7u, bpp TA);
    uint pal = (e >> 10) & 7u, prio = (e >> 13) & 1u, color;
    if (c == 0u) return prio << 16;
    if (bpp == 8u) color = (byte_of(w6, 1u) & 1u) != 0u ? direct_color(c, pal) : FETCH_PAL(line, c);
    else if (bpp == 4u) color = FETCH_PAL(line, (pal * 16u + c) & 0xFFu);
    else color = FETCH_PAL(line, ((mode == 0u ? n * 32u : 0u) + pal * 4u + c) & 0xFFu);
    return color | prio << 16 | 1u << 17;
}

int sext13(int v) { return (v & 0x1000) != 0 ? (v | ~0x1FFF) : (v & 0x1FFF); }
int clip10(int v) { return (v & 0x2000) != 0 ? (v | ~0x3FF) : (v & 0x3FF); }
int s16(uint v) { return int(v << 16) >> 16; }

// Mode 7's pixel, as bg_pixel's
uint m7_pixel(uint line, uint ver, uint n, uint x TP)
{
    uint w0 = lw(line, 0u TA), w2 = lw(line, 2u TA), w6 = lw(line, 6u TA);
    uint mosaic = byte_of(w0, 3u), m7sel = byte_of(w2, 2u);
    int a = s16(lw(line, 13u TA)), b = s16(lw(line, 13u TA) >> 16), c = s16(lw(line, 14u TA)), d = s16(lw(line, 14u TA) >> 16);
    int cx = sext13(s16(lw(line, 15u TA))), cy = sext13(s16(lw(line, 15u TA) >> 16));
    int hofs = sext13(int(lw(line, 16u TA) & 0xFFFFu)), vofs = sext13(int(lw(line, 16u TA) >> 16));
    uint msize = (mosaic & (1u << n)) != 0u ? (mosaic >> 4) + 1u : 1u;
    int y = int(msize > 1u ? byte_of(lw(line, 12u TA), 3u) : line);
    if ((m7sel & 2u) != 0u) y = 255 - y;
    int x0 = clip10(hofs - cx), y0 = clip10(vofs - cy);
    int ox = ((a * x0) & ~63) + ((b * y) & ~63) + ((b * y0) & ~63) + (cx << 8);
    int oy = ((c * x0) & ~63) + ((d * y) & ~63) + ((d * y0) & ~63) + (cy << 8);
    int sx = int(msize > 1u ? x - x % msize : x);
    if ((m7sel & 1u) != 0u) sx = 255 - sx;
    int px = (ox + a * sx) >> 8, py = (oy + c * sx) >> 8;
    uint tile;
    if (((px | py) & ~0x3FF) != 0) {
        if ((m7sel & 0xC0u) == 0x80u) return 0u;
        if ((m7sel & 0xC0u) == 0xC0u) tile = 0u;
        else tile = FETCH_VRAM(ver, uint(((py >> 3 & 127) << 7 | (px >> 3 & 127)) & 0x7FFF)) & 0xFFu;
    } else {
        tile = FETCH_VRAM(ver, uint(((py >> 3) << 7 | (px >> 3)) & 0x7FFF)) & 0xFFu;
    }
    uint pix = (FETCH_VRAM(ver, (tile * 64u + uint(py & 7) * 8u + uint(px & 7)) & 0x7FFFu) >> 8) & 0xFFu, prio = 0u;
    if (n == 1u) {
        prio = pix >> 7;
        pix &= 0x7Fu;
    }
    if (pix == 0u) return prio << 16;
    uint color = (byte_of(w6, 1u) & 1u) != 0u && n == 0u ? direct_color(pix, 0u) : FETCH_PAL(line, pix);
    return color | prio << 16 | 1u << 17;
}

uint obj_w(uint sz, uint big)
{
    uint s0 = sz == 0u ? 8u : sz == 1u ? 8u : sz == 2u ? 8u : sz == 3u ? 16u : sz == 4u ? 16u : sz == 5u ? 32u : 16u;
    uint s1 = sz == 0u ? 16u : sz == 1u ? 32u : sz == 2u ? 64u : sz == 3u ? 32u : sz == 4u ? 64u : sz == 5u ? 64u : 32u;
    return big != 0u ? s1 : s0;
}

uint obj_h(uint sz, uint big)
{
    uint s0 = sz == 0u ? 8u : sz == 1u ? 8u : sz == 2u ? 8u : sz == 3u ? 16u : sz == 4u ? 16u : sz == 5u ? 32u : 32u;
    uint s1 = sz == 0u ? 16u : sz == 1u ? 32u : sz == 2u ? 64u : sz == 3u ? 32u : sz == 4u ? 64u : sz == 5u ? 64u : sz == 6u ? 64u : 32u;
    return big != 0u ? s1 : s0;
}

uint obj_hi(uint ver, uint s TP) { return (FETCH_OAM(ver, 0x200u + (s >> 2)) >> ((s & 3u) * 2u)) & 3u; }

int obj_x(uint ver, uint s, uint hi TP)
{
    int x = int(FETCH_OAM(ver, s * 4u) | (hi & 1u) << 8);
    return x >= 256 ? x - 512 : x;
}

// the sprites' pixel at x on this line: colour | priority << 16 | opaque
// << 18 | colour maths (palettes 4-7) << 19. The line's sprites (the first
// 32 in range from the first sprite) and tiles (34, from the last in the
// list) as the hardware takes them; the first in the list on top.
uint obj_pixel(uint line, uint ver, uint x TP)
{
    uint w0 = lw(line, 0u TA), w6 = lw(line, 6u TA), w12 = lw(line, 12u TA);
    uint obsel = byte_of(w0, 1u), sz = obsel >> 5;
    uint first = byte_of(w6, 3u) != 0u ? ((w12 & 0xFFFFu) >> 1) & 0x7Fu : 0u;
    uint row = (line - 1u) & 0xFFu;
    uint base = (obsel & 7u) << 13, gap = (((obsel >> 3) & 3u) + 1u) << 12;
    uint list[32];
    int keep[32];
    uint n = 0u, tiles = 0u;
    for (uint i = 0u; i < 128u; i++) {
        uint s = (first + i) & 0x7Fu, hi = obj_hi(ver, s TA), big = (hi >> 1) & 1u;
        int sx = obj_x(ver, s, hi TA);
        uint y = FETCH_OAM(ver, s * 4u + 1u);
        if (((row - y) & 0xFFu) >= obj_h(sz, big)) continue;
        if (sx <= -int(obj_w(sz, big))) continue;
        if (n == 32u) break;
        list[n] = s;
        n++;
    }
    for (uint k = 0u; k < n; k++) keep[k] = 0;
    for (int k = int(n) - 1; k >= 0; k--) {
        uint s = list[k], hi = obj_hi(ver, s TA), big = (hi >> 1) & 1u, cols = obj_w(sz, big) >> 3, on = 0u;
        int sx = obj_x(ver, s, hi TA);
        for (uint c = 0u; c < cols; c++) {
            int tx = sx + int(c) * 8;
            if (tx > -8 && tx < 256) on++;
        }
        if (tiles + on > 34u) {
            keep[k] = int(34u - tiles);
            for (int j = k - 1; j >= 0; j--) keep[j] = -1;
            break;
        }
        tiles += on;
        keep[k] = int(on);
    }
    for (uint k = 0u; k < n; k++) {
        if (keep[k] < 0) continue;
        uint s = list[k], hi = obj_hi(ver, s TA), big = (hi >> 1) & 1u;
        uint h = obj_h(sz, big), w = obj_w(sz, big), cols = w >> 3;
        int sx = obj_x(ver, s, hi TA);
        uint y = FETCH_OAM(ver, s * 4u + 1u), attr = FETCH_OAM(ver, s * 4u + 3u);
        uint name = FETCH_OAM(ver, s * 4u + 2u) | (attr & 1u) << 8;
        uint r = (row - y) & 0xFFu;
        if ((attr & 0x80u) != 0u) r = h - 1u - r;
        int dx = int(x) - sx;
        if (dx < 0 || dx >= int(w)) continue;
        uint c = uint(dx) >> 3;
        // the tile's place among this sprite's tiles on the line
        uint drawn = 0u;
        for (uint j = 0u; j < c; j++) {
            int tx = sx + int(j) * 8;
            if (tx > -8 && tx < 256) drawn++;
        }
        if (int(drawn) >= keep[k]) continue;
        uint col = (attr & 0x40u) != 0u ? cols - 1u - c : c;
        uint t = (((name & 0xF0u) + ((r >> 3) << 4)) & 0xF0u) | ((name + col) & 0x0Fu);
        uint ca = (base + t * 16u + ((name & 0x100u) != 0u ? gap : 0u)) & 0xFFFFu;
        uint px = uint(dx) & 7u, ix = (attr & 0x40u) != 0u ? 7u - px : px;
        uint ci = tile_pixel(ver, ca, r & 7u, ix, 4u TA);
        if (ci == 0u) continue;
        uint pal = (attr >> 1) & 7u, prio = (attr >> 4) & 3u;
        return FETCH_PAL(line, 128u + pal * 16u + ci) | prio << 16 | 1u << 18 | (pal >= 4u ? 1u : 0u) << 19;
    }
    return 0u;
}

uint win_mask(uint sel, uint logic, uint x, uint wh)
{
    bool w1 = x >= byte_of(wh, 0u) && x <= byte_of(wh, 1u), w2 = x >= byte_of(wh, 2u) && x <= byte_of(wh, 3u);
    bool e1 = (sel & 2u) != 0u, e2 = (sel & 8u) != 0u;
    if (!e1 && !e2) return 0u;
    if ((sel & 1u) != 0u) w1 = !w1;
    if ((sel & 4u) != 0u) w2 = !w2;
    if (e1 && !e2) return w1 ? 1u : 0u;
    if (e2 && !e1) return w2 ? 1u : 0u;
    uint l = logic & 3u;
    bool r = l == 0u ? (w1 || w2) : l == 1u ? (w1 && w2) : l == 2u ? (w1 != w2) : (w1 == w2);
    return r ? 1u : 0u;
}

uint layer_sel(uint layer, uint w3)
{
    if (layer == 0u) return byte_of(w3, 0u) & 0x0Fu;
    if (layer == 1u) return byte_of(w3, 0u) >> 4;
    if (layer == 2u) return byte_of(w3, 1u) & 0x0Fu;
    if (layer == 3u) return byte_of(w3, 1u) >> 4;
    if (layer == 4u) return byte_of(w3, 2u) & 0x0Fu;
    return byte_of(w3, 2u) >> 4;
}

uint layer_logic(uint layer, uint w3, uint w5)
{
    if (layer < 4u) return byte_of(w3, 3u) >> (layer * 2u);
    if (layer == 4u) return byte_of(w5, 0u);
    return byte_of(w5, 0u) >> 2;
}

bool region(uint mode, uint inside)
{
    uint m = mode & 3u;
    return m == 0u ? false : m == 1u ? inside == 0u : m == 2u ? inside != 0u : true;
}

// the pixel at x on line (1-224): 0xFFRRGGBB
uint compose_pixel(uint x, uint line TP)
{
    uint w0 = lw(line, 0u TA), w3 = lw(line, 3u TA), w4 = lw(line, 4u TA), w5 = lw(line, 5u TA), w6 = lw(line, 6u TA);
    uint w2 = lw(line, 2u TA), w11 = lw(line, 11u TA), ver = (lw(line, 12u TA) >> 16) & 0xFFu;
    uint inidisp = byte_of(w0, 0u), bgmode = byte_of(w0, 2u), mode = bgmode & 7u;
    uint tm = byte_of(w5, 1u), ts = byte_of(w5, 2u), tmw = byte_of(w5, 3u), tsw = byte_of(w6, 0u);
    uint cgwsel = byte_of(w6, 1u), cgadsub = byte_of(w6, 2u), setini = byte_of(w2, 3u);
    uint fixc = byte_of(w11, 0u) | byte_of(w11, 1u) << 5 | byte_of(w11, 2u) << 10;
    if ((inidisp & 0x80u) != 0u) return 0u;
    uint mc = FETCH_PAL(line, 0u), mz = 0u, ml = 5u;
    uint sc = fixc, sz = 0u, sl = 5u;
    for (uint n = 0u; n < 4u; n++) {
        uint p, z;
        if (mode == 7u) {
            if (n >= 2u || (n == 1u && (setini & 0x40u) == 0u)) continue;
            if (((tm | ts) >> n & 1u) == 0u) continue;
            p = m7_pixel(line, ver, n, x TA);
        } else {
            if (mode_bpp(mode, n) == 0u || ((tm | ts) >> n & 1u) == 0u) continue;
            p = bg_pixel(line, ver, n, mode, x TA);
        }
        if ((p >> 17 & 1u) == 0u) continue;
        z = bg_depth(mode, bgmode, n, (p >> 16) & 1u);
        if (z == 0u) continue;
        uint inside = ((tmw | tsw) >> n & 1u) != 0u ? win_mask(layer_sel(n, w3), layer_logic(n, w3, w5), x, w4) : 0u;
        if ((tm >> n & 1u) != 0u && z > mz && !(inside != 0u && (tmw >> n & 1u) != 0u)) { mc = p & 0xFFFFu; mz = z; ml = n; }
        if ((ts >> n & 1u) != 0u && z > sz && !(inside != 0u && (tsw >> n & 1u) != 0u)) { sc = p & 0xFFFFu; sz = z; sl = n; }
    }
    if (((tm | ts) & 0x10u) != 0u) {
        uint p = obj_pixel(line, ver, x TA);
        if ((p >> 18 & 1u) != 0u) {
            uint z = obj_depth(mode, (p >> 16) & 3u), lay = (p >> 19 & 1u) != 0u ? 4u : 6u;
            uint inside = ((tmw | tsw) >> 4 & 1u) != 0u ? win_mask(layer_sel(4u, w3), layer_logic(4u, w3, w5), x, w4) : 0u;
            if ((tm >> 4 & 1u) != 0u && z > mz && !(inside != 0u && (tmw >> 4 & 1u) != 0u)) { mc = p & 0xFFFFu; mz = z; ml = lay; }
            if ((ts >> 4 & 1u) != 0u && z > sz && !(inside != 0u && (tsw >> 4 & 1u) != 0u)) { sc = p & 0xFFFFu; sz = z; sl = lay; }
        }
    }
    uint cw = win_mask(layer_sel(5u, w3), layer_logic(5u, w3, w5), x, w4);
    bool clip = region(cgwsel >> 6, cw), prevent = region(cgwsel >> 4, cw);
    uint bit = ml == 5u ? 5u : ml == 6u ? 8u : ml;
    uint c = clip ? 0u : mc;
    uint r = c & 31u, g = (c >> 5) & 31u, b = (c >> 10) & 31u;
    if (bit < 8u && (cgadsub >> bit & 1u) != 0u && !prevent) {
        uint o;
        bool halve = (cgadsub & 0x40u) != 0u && !clip;
        if ((cgwsel & 2u) != 0u) {
            if (sl == 5u) { o = fixc; halve = false; } else o = sc;
        } else {
            o = fixc;
        }
        if ((cgadsub & 0x80u) != 0u) {
            int rr = int(r) - int(o & 31u), gg = int(g) - int((o >> 5) & 31u), bb = int(b) - int((o >> 10) & 31u);
            r = rr < 0 ? 0u : uint(rr);
            g = gg < 0 ? 0u : uint(gg);
            b = bb < 0 ? 0u : uint(bb);
            if (halve) { r >>= 1; g >>= 1; b >>= 1; }
        } else {
            r += o & 31u;
            g += (o >> 5) & 31u;
            b += (o >> 10) & 31u;
            if (halve) { r >>= 1; g >>= 1; b >>= 1; }
            if (r > 31u) r = 31u;
            if (g > 31u) g = 31u;
            if (b > 31u) b = 31u;
        }
    }
    uint bright = inidisp & 0x0Fu;
    if (bright != 15u) {
        r = r * (bright + 1u) / 16u;
        g = g * (bright + 1u) / 16u;
        b = b * (bright + 1u) / 16u;
    }
    return 0xFF000000u | (r << 3 | r >> 2) << 16 | (g << 3 | g >> 2) << 8 | (b << 3 | b >> 2);
}
