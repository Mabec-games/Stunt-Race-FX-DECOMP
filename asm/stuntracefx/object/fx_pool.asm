; fx_pool.asm: the Super FX's two pools of record memory.
;
; Each pool is a chain of blocks: a block's first word links to the next
; block (0 ends the chain), its second word is its size in bytes, negative
; while the block is in use. The first pool's chain starts at the word at
; $70:1098, the second's at $70:1D70 (the S-CPU's side: GsuRecordNew* in
; object/gsu_record.asm). Records in use sit on lists linked through +$0E,
; with bit 0 of +$10 marking those still wanted.
; C: src/stuntracefx/object/fx_pool.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 04:893A FxPoolTake
; WHAT IT DOES: Takes a block of R4 bytes from the pool whose chain head is
; at R3 ($1098 or $1D70) for a new record: R1 = the record, or 0 when no
; block is big enough. The record's bytes from +4 to its end are cleared,
; +$08 = the pool, it goes on the front of the list of all records at
; $2332 (+$04 next, +$06 previous) and bit 2 of +$1C is set.
; HOW: A first walk looks for a free block of exactly R4 bytes: it stays
; on the chain, its size made -R4 (in use). Failing that, a second walk
; takes the first block with more: the record is cut from its end, the
; block keeping the rest (its size lowered), and the record's size word
; is R4. A block with less than 4 bytes to spare is taken whole instead,
; its size negated (decoded from the ROM bytes and not reached in the
; traces so far). R4, R3 and the walk's start are kept on the R10 stack.
; The clearing is STB of 0 under LOOP. Two compares of R3 with the pool
; heads change nothing. The routine runs under CACHE (the code cache only;
; nothing the C has to do).
; Entry: R3 = the pool's chain head; R4 = bytes; R10 = stack; R11 =
; return address. Exit: R1 = the record or 0; R3, R4 as on entry; with a
; record: R0 = 4 | the old +$1C; R2 = that old word; R12 = 0 (when there
; was anything to clear); R13 = $89CB; OV, S, Z from the OR; the last RAM
; address the record + $1C. Without: S, Z from R1 (set), the last RAM
; address R10.
%at($04893A)
FxPoolTake:
    from r4
    stw (r10)
    inc r10
    inc r10
    from r3
    stw (r10)
    inc r10
    inc r10
    cache
    move r1,r4
    from r3
    stw (r10)
    inc r10
    inc r10
.exact:
    to r2
    ldw (r3)
    inc r3
    inc r3
    ldw (r3)
    sub r1
    beq .exact_found
    nop
    moves r3,r2
    bne .exact
    nop
    dec r10
    dec r10
    to r3
    ldw (r10)
.bigger:
    to r2
    ldw (r3)
    inc r3
    inc r3
    ldw (r3)
    sub r1
    bpl .bigger_found
    nop
    moves r3,r2
    bne .bigger
    nop
    ibt r1,#0
    bra .done
    nop
.exact_found:
    dec r10
    dec r10
    ldw (r10)
    with r1
    not
    inc r1
    sub r0
    bra .mark
    nop
.bigger_found:
    sub #4
    bpl .split
    nop
    add #4
    with r1
    add r0
    with r1
    not
    inc r1
    ibt r0,#-4
.split:
    add #4
    stw (r3)
.mark:
    from r3
    to r2
    add r0
    from r1
    stw (r2)
    dec r2
    dec r2
    move r1,r2
.done:
    dec r10
    dec r10
    to r3
    ldw (r10)
    dec r10
    dec r10
    to r4
    ldw (r10)
    moves r1,r1
    beq .return
    nop
    iwt r0,#!fx_pool_first
    cmp r3
    bne .pool_checked
    nop
.pool_checked:
    iwt r0,#!fx_pool_second
    cmp r3
    bne .pool_checked_2
    nop
.pool_checked_2:
    iwt r2,#$0002
    with r2
    add r1
    ldw (r2)
    inc r2
    inc r2
    moves r0,r0
    bpl .size
    nop
    not
    inc r0
.size:
    sub #4
    beq .link
    nop
    bmi .link
    nop
    move r12,r0
    sub r0
    move r13,r15
    stb (r2)
    inc r2
    loop
    nop
.link:
    ibt r0,#!fxrec_pool
    add r1
    from r3
    stw (r0)
    lm r2,(!fx_records)
    ibt r0,#!fxrec_all_next
    add r1
    from r2
    stw (r0)
    sm (!fx_records),r1
    moves r2,r2
    beq .flag
    nop
    ibt r0,#!fxrec_all_prev
    add r2
    from r1
    stw (r0)
.flag:
    ibt r2,#!fxrec_flags1c
    with r2
    add r1
    to r2
    ldw (r2)
    iwt r0,#$0004
    or r2
    sbk
.return:
    move r15,r11
    nop

; @routine 04:888E FxPoolsMergeProgram
; WHAT IT DOES: A program: FxPoolsMerge.
; HOW: R10 = $0240 for the stack, then the call.
; Entry: the S-CPU starts it. Exit (at its STOP): as FxPoolsMerge; R10 =
; $0240; R11 = $8896.
%at($04888E)
FxPoolsMergeProgram:
    iwt r10,#!fx_stack
    link #4
    iwt r15,#FxPoolsMerge
    nop
    stop
    nop

; @routine 04:8898 FxPoolsMerge
; WHAT IT DOES: Merges neighbouring free blocks in both pools, the first
; pool's chain and then the second's.
; HOW: A walk of each chain (pool_merge): a free block (size not negative)
; whose address plus size is the next block's address, the next block also
; free, gets the sum of the sizes and the next block's link. The walk goes
; on from the block merged in, so a run of three free blocks merges only
; the first two in one pass.
; Entry: R11 = return address. Exit: R2 = 0; R1 = the second chain's last
; block + 2; R0 and R3 from the walk's last block; OV, S clear, Z set; the
; last RAM address that block's link.
%at($048898)
FxPoolsMerge:
    iwt r1,#!fx_pool_first
.first:
    to r2
    ldw (r1)
    inc r1
    inc r1
    moves r2,r2
    beq .second_start
    nop
    ldw (r1)
    moves r0,r0
    bmi .first_on
    nop
    sub #2
    to r3
    add r1
    with r3
    cmp r2
    bne .first_on
    nop
    inc r2
    inc r2
    to r3
    ldw (r2)
    moves r3,r3
    bmi .first_used
    nop
    add r3
    add #2
    dec r2
    dec r2
    stw (r1)
    dec r1
    dec r1
    ldw (r2)
    stw (r1)
    inc r1
    inc r1
.first_on:
    move r1,r2
    bra .first
    nop
.first_used:
    dec r2
    dec r2
    move r1,r2
    bra .first
    nop
.second_start:
    iwt r1,#!fx_pool_second
.second:
    to r2
    ldw (r1)
    inc r1
    inc r1
    moves r2,r2
    beq .done
    nop
    ldw (r1)
    moves r0,r0
    bmi .second_on
    nop
    sub #2
    to r3
    add r1
    with r3
    cmp r2
    bne .second_on
    nop
    inc r2
    inc r2
    to r3
    ldw (r2)
    moves r3,r3
    bmi .second_used
    nop
    add r3
    add #2
    dec r2
    dec r2
    stw (r1)
    dec r1
    dec r1
    ldw (r2)
    stw (r1)
    inc r1
    inc r1
.second_on:
    move r1,r2
    bra .second
    nop
.second_used:
    dec r2
    dec r2
    move r1,r2
    bra .second
    nop
.done:
    move r15,r11
    nop

; @routine 04:A462 FxListSweep
; WHAT IT DOES: Sweeps the list whose head is at +$18 of record R6: each
; record with bit 0 of +$10 set stays and has the bit cleared; each with it
; clear is unlinked and its memory given back. A block with a positive size
; goes on the front of the first pool's chain ($1098); one with a negative
; size has the size negated in place.
; HOW: R2 follows the last record kept (0 before any), so an unlinked
; record's successor goes to R2's +$0E or to the head. The successor is
; pushed on the R10 stack around the freeing. The freeing first negates the
; size into R7 (when negative), a value overwritten at once by $1098.
; Entry: R6 = the record holding the list; R10 = stack; R11 = return
; address. Exit: R8 = 0; with records: R0 = 0, R12 = 1, R2 = the last
; record kept or 0, R9 and R7 from the last record seen; OV, S clear, Z
; set; the last RAM address the last word read or written.
%at($04A462)
FxListSweep:
    ibt r8,#!fxrec_list
    with r8
    add r6
    to r8
    ldw (r8)
    moves r8,r8
    beq .done
    nop
    iwt r12,#$0001
    ibt r2,#0
.record:
    ibt r0,#!fxrec_flags10
    add r8
    ldw (r0)
    to r9
    and r12
    beq .drop
    nop
    bic r12
    sbk
    move r2,r8
    ibt r0,#!fxrec_next
    add r8
    ldw (r0)
.advance:
    moves r8,r0
    bne .record
    nop
.done:
    move r15,r11
    nop
.drop:
    ibt r7,#!fxrec_next
    with r7
    add r8
    to r7
    ldw (r7)
    moves r2,r2
    bne .after_kept
    nop
    ibt r0,#!fxrec_list
    add r6
    from r7
    stw (r0)
    bra .free
    nop
.after_kept:
    ibt r0,#!fxrec_next
    add r2
    from r7
    stw (r0)
.free:
    from r7
    stw (r10)
    inc r10
    inc r10
    inc r8
    inc r8
    ldw (r8)
    moves r7,r0
    bpl .positive
    nop
    with r7
    not
    inc r7
.positive:
    dec r8
    dec r8
    iwt r7,#!fx_pool_first
    move r9,r8
    inc r9
    inc r9
    ldw (r9)
    moves r0,r0
    bpl .chain
    nop
    not
    inc r0
    stw (r9)
    bra .freed
    nop
.chain:
    ldw (r7)
    stw (r8)
    from r8
    stw (r7)
.freed:
    dec r10
    dec r10
    to r7
    ldw (r10)
    move r0,r7
    bra .advance
    nop

; A new record of <bytes> from <pool>, its address left at $238E; to
; <none> when there is none.
macro fx_record_new(bytes, pool, none)
    iwt r10,#!fx_stack
    iwt r4,#<bytes>
    iwt r3,#<pool>
    link #4
    iwt r15,#FxPoolTake
    nop
    sm (!gsu_record_arg),r1
    moves r1,r1
    beq <none>
    nop
endmacro

; Field <field> of the new record R1 = <value>.
macro fx_record_set(field, value)
    iwt r2,#<value>
    ibt r0,#<field>
    add r1
    from r2
    stw (r0)
endmacro

; @routine 04:8B33 FxRecordNew426
; WHAT IT DOES: A program: a new $34-byte record from the first pool with
; mass +$2A = $40, flags +$16 = 8 and mode +$1A = $0426; its address (or
; 0) to $238E. The S-CPU side is GsuRecordNew426 (object/gsu_record.asm).
; HOW: FxPoolTake with R10 = $0240, then three STWs through R2 and R0.
; Entry: the S-CPU starts it. Exit (at its STOP): R1 = the record; R0 = R1
; + $1A; R2 = $0426; R11 = $8B41; OV, S, CY, Z from the last ADD; the last
; RAM address R1 + $1A. No record: R1 = 0; Z set from R1.
%at($048B33)
FxRecordNew426:
    %fx_record_new($0034, !fx_pool_first, .done)
    %fx_record_set(!fxrec_mass, $0040)
    %fx_record_set(!fxrec_flags16, $0008)
    %fx_record_set(!fxrec_mode, $0426)
.done:
    stop
    nop

; @routine 04:8B64 FxRecordNew422
; WHAT IT DOES: A program: a new $2A-byte record from the second pool with
; mode +$1A = $0422 and bits $0181 set in its flags +$16; it is counted at
; $2482 and put at the head of the list at $2398 (the old head's +$26
; pointing to it); its address (or 0) to $238E. The S-CPU side is
; GsuRecordNew422 (object/gsu_record.asm).
; HOW: FxPoolTake with R10 = $0240; the count is LM, INC, SBK; the flags
; an LDW, OR and STW.
; Entry: the S-CPU starts it. Exit (at its STOP): R1 = the record; R0 = R1
; + $1A; R2 = $0422; R11 = $8B72; OV, S, CY, Z from the last ADD; the last
; RAM address R1 + $1A. No record: R1 = 0; Z set from R1.
%at($048B64)
FxRecordNew422:
    %fx_record_new($002A, !fx_pool_second, .done)
    lm r0,(!fx_count_422)
    inc r0
    sbk
    lm r2,(!fx_list_422)
    moves r2,r2
    beq .head
    nop
    ibt r0,#!fxrec_link_422
    add r2
    from r1
    stw (r0)
.head:
    sm (!fx_list_422),r1
    ibt r2,#!fxrec_flags16
    with r2
    add r1
    to r2
    ldw (r2)
    iwt r0,#$0181
    to r2
    or r2
    ibt r0,#!fxrec_flags16
    add r1
    from r2
    stw (r0)
    %fx_record_set(!fxrec_mode, $0422)
.done:
    stop
    nop

; @routine 04:8C0A FxRecordNew424
; WHAT IT DOES: A program: as FxRecordNew426 with mode $0424. The S-CPU
; side is GsuRecordNew424 (object/gsu_record.asm).
; HOW: As FxRecordNew426.
; Entry: the S-CPU starts it. Exit (at its STOP): as FxRecordNew426; R2 =
; $0424; R11 = $8C18.
%at($048C0A)
FxRecordNew424:
    %fx_record_new($0034, !fx_pool_first, .done)
    %fx_record_set(!fxrec_mass, $0040)
    %fx_record_set(!fxrec_flags16, $0008)
    %fx_record_set(!fxrec_mode, $0424)
.done:
    stop
    nop

; @routine 04:8C95 FxRecordNew42D
; WHAT IT DOES: A program: as FxRecordNew426 with mass 0, flags $20 and
; mode $042D. The S-CPU side is GsuRecordNew42D (object/gsu_record.asm).
; HOW: As FxRecordNew426.
; Entry: the S-CPU starts it. Exit (at its STOP): as FxRecordNew426; R2 =
; $042D; R11 = $8CA3.
%at($048C95)
FxRecordNew42D:
    %fx_record_new($0034, !fx_pool_first, .done)
    %fx_record_set(!fxrec_mass, $0000)
    %fx_record_set(!fxrec_flags16, $0020)
    %fx_record_set(!fxrec_mode, $042D)
.done:
    stop
    nop

arch 65816
