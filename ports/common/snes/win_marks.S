/* win_marks.S: the bounds of the program's own static data on Windows.
   link.sh moves each of its objects' .data and .bss into .data$srfx_m and
   .bss$srfx_m; the linker sorts .data$ and .bss$ sections by name, so these
   labels (in _a and _z) stand before and after them: a snapshot of the
   game's static data is the bytes between (la_win.c). */
#if defined(_WIN32)
    .section .data$srfx_a,"dw"
    .p2align 6
    .globl srfx_data_begin
srfx_data_begin:
    .section .data$srfx_z,"dw"
    .p2align 6
    .globl srfx_data_end
srfx_data_end:
    .section .bss$srfx_a,"bw"
    .p2align 6
    .globl srfx_bss_begin
srfx_bss_begin:
    .section .bss$srfx_z,"bw"
    .p2align 6
    .globl srfx_bss_end
srfx_bss_end:
#endif
