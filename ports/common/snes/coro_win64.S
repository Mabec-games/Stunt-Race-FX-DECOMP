/* coro_win64.S: the coroutine switch on 64-bit Windows (the Microsoft x64
   convention: rbx, rbp, rdi, rsi, r12-r15 and xmm6-xmm15 are the callee's
   to keep). The thread's stack bounds in its TEB (gs:8 the base, gs:16 the
   limit) go with each stack, as the system's fibers keep them. Our own
   switch rather than fibers: the stacks are ours (coro.c allocates them),
   so a snapshot of the program's memory holds them whole (the look-ahead's
   copy on Windows, la_win.c). */
#if defined(__x86_64__) && defined(_WIN32)
    .text

/* void coro_swap(void **save, void *load): rcx = save, rdx = load */
    .globl coro_swap
coro_swap:
    pushq %rbp
    pushq %rbx
    pushq %rdi
    pushq %rsi
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $160, %rsp
    movdqu %xmm6, 0(%rsp)
    movdqu %xmm7, 16(%rsp)
    movdqu %xmm8, 32(%rsp)
    movdqu %xmm9, 48(%rsp)
    movdqu %xmm10, 64(%rsp)
    movdqu %xmm11, 80(%rsp)
    movdqu %xmm12, 96(%rsp)
    movdqu %xmm13, 112(%rsp)
    movdqu %xmm14, 128(%rsp)
    movdqu %xmm15, 144(%rsp)
    pushq %gs:8
    pushq %gs:16
    movq %rsp, (%rcx)
    movq %rdx, %rsp
    popq %gs:16
    popq %gs:8
    movdqu 0(%rsp), %xmm6
    movdqu 16(%rsp), %xmm7
    movdqu 32(%rsp), %xmm8
    movdqu 48(%rsp), %xmm9
    movdqu 64(%rsp), %xmm10
    movdqu 80(%rsp), %xmm11
    movdqu 96(%rsp), %xmm12
    movdqu 112(%rsp), %xmm13
    movdqu 128(%rsp), %xmm14
    movdqu 144(%rsp), %xmm15
    addq $160, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rsi
    popq %rdi
    popq %rbx
    popq %rbp
    ret

/* a new coroutine's first frame: r12 = the coroutine */
    .globl coro_start
coro_start:
    movq %r12, %rcx
    xorl %ebp, %ebp
    andq $-16, %rsp
    subq $32, %rsp
    call coro_entry
    ud2
#endif
