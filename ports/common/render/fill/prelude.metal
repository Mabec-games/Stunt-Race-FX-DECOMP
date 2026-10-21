#include <metal_stdlib>
using namespace metal;
// Metal: the ROM passed down to each helper
#define TP , texture2d<uint> t_rom
#define TA , t_rom
#define FETCH_ROM(o) t_rom.read(uint2((o) & 1023u, (o) >> 10)).r
#define ROM_BYTES (t_rom.get_width() * t_rom.get_height())
