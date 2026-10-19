/* coro_x86_64.S: the coroutine switch on x86-64 (System V: rbx, rbp and
   r12-r15 are the callee's to keep). Windows uses fibers (coro.c). */
#if defined(__x86_64__) && !defined(_WIN32)
#ifdef __APPLE__
#define SYM(x) _##x
#else
#define SYM(x) x
#endif
    .text
    .globl SYM(coro_swap)
SYM(coro_swap):
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    movq %rsp, (%rdi)
    movq %rsi, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    ret

    .globl SYM(coro_start)
SYM(coro_start):
    movq %r12, %rdi
    xorl %ebp, %ebp
    andq $-16, %rsp
    call SYM(coro_entry)
    ud2
#endif
#if defined(__linux__) && defined(__ELF__)
    .section .note.GNU-stack,"",%progbits
#endif
