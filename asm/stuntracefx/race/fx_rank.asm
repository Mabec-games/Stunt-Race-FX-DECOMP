; fx_rank.asm: the Super FX ranks the cars into race positions.
;
; The cars' Super FX records are a list from the word at $70:262E, linked
; through +$74. A car's rank goes to +$88 (1 = leading, 0 = not ranked
; yet); it is ordered by laps (+$7A), then the last segment counted
; (+$86), both higher first, then +$84, lower first.
; C: src/stuntracefx/race/fx_rank.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 04:CE1D FxRankCars
; WHAT IT DOES: Ranks all the cars: clears every +$88, then FxRankNext once
; per car with ranks 1, 2, 3, ...
; HOW: Two walks of the list; the second keeps the walking car on the R10
; stack across each call, as the return address is kept across the
; routine. It runs under CACHE (the code cache only; nothing the C has to
; do).
; Entry: R10 = stack; R11 = return address. Exit: R0 = R8 = 0; R5 = 0;
; with cars: R9 = their number + 1, R1-R7 from the last FxRankNext, R11 =
; $CE54; S, Z from R10; the last RAM address R10.
%at($04CE1D)
FxRankCars:
    from r11
    stw (r10)
    inc r10
    inc r10
    cache
    ibt r5,#0
    lm r0,(!fx_cars)
    moves r0,r0
    beq .done
    nop
.clear:
    moves r8,r0
    beq .rank
    nop
    iwt r0,#!fxrec_rank
    add r8
    from r5
    stw (r0)
    ibt r0,#!fxrec_next_car
    add r8
    ldw (r0)
    bra .clear
    nop
.rank:
    lm r0,(!fx_cars)
    ibt r9,#1
    cache
.next:
    moves r8,r0
    beq .done
    nop
    from r8
    stw (r10)
    inc r10
    inc r10
    link #4
    iwt r15,#FxRankNext
    nop
    dec r10
    dec r10
    to r8
    ldw (r10)
    inc r9
    ibt r0,#!fxrec_next_car
    add r8
    ldw (r0)
    bra .next
    nop
.done:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 04:CE65 FxRankNext
; WHAT IT DOES: Gives rank R9 to the leading car not ranked yet (the first
; car when none is).
; HOW: A walk of the list keeping the best keys in R1-R3 (starting at -1,
; 0, 0) and its car in R4; each unranked car's keys go to R5-R7 and are
; compared in turn with CMP and the sign (BMI). A car on lap 0 has a
; correction (its lap count lowered when +$86 - +$86 + 5 is negative,
; which it never is: FxRankNextLapZero, 04:CEA5).
; Entry: R9 = the rank; R11 = return address. Exit: R0 = the car + $88; R4
; = the car; R1-R3 = its keys; R5-R7 = the last unranked car's keys; R8 =
; 0; OV, S, CY, Z from the ADD of R0; the last RAM address the car + $88.
%at($04CE65)
FxRankNext:
    sub r0
    move r2,r0
    move r3,r0
    ibt r1,#-1
    lm r0,(!fx_cars)
    moves r4,r0
.car:
    moves r8,r0
    beq .give
    nop
    iwt r0,#!fxrec_rank
    add r8
    ldw (r0)
    moves r0,r0
    bne .after
    nop
    ibt r5,#!fxrec_laps
    with r5
    add r8
    to r5
    ldw (r5)
    iwt r6,#!fxrec_counted
    with r6
    add r8
    to r6
    ldw (r6)
    iwt r7,#!fxrec_to_go
    with r7
    add r8
    to r7
    ldw (r7)
    moves r5,r5
    bne .compare
    nop
    iwt r0,#!fxrec_counted
    add r8
    ldw (r0)
    sub r6
    add #5
    bpl .compare
    nop
;
; @routine 04:CEA5 FxRankNextLapZero
; WHAT IT DOES: FxRankNext's lap 0 correction, entered on its own: the
; lap count R5 lowered by one, then FxRankNext's compare of this car's
; keys and the rest of its walk. The game never runs it: the test before
; it subtracts a car's +$86 from itself, so +5 is never negative and the
; BPL at 04:CEA2 always branches past it. A unit start runs it so the C
; is judged against the original.
; HOW: DEC R5; then as FxRankNext from its compare (04:CEA6).
; Entry: R1-R3 = the best keys so far, R4 = their car; R5-R7 = car R8's
; keys; R9 = the rank; R11 = return address. Exit: as FxRankNext's.
%at($04CEA5)
    dec r5
.compare:
    with r5
    cmp r1
    bmi .after
    nop
    bne .take
    nop
    with r6
    cmp r2
    bmi .after
    nop
    bne .take
    nop
    with r7
    cmp r3
    bpl .after
    nop
.take:
    move r4,r8
    move r1,r5
    move r2,r6
    move r3,r7
.after:
    ibt r0,#!fxrec_next_car
    add r8
    ldw (r0)
    bra .car
    nop
.give:
    iwt r0,#!fxrec_rank
    add r4
    from r9
    stw (r0)
    move r15,r11
    nop

arch 65816
