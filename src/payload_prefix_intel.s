# code stub to (hopefully) simplify payloads
# keep it portable between x86 and x64

.intel_syntax noprefix
.text
.global payload_prefix_intel
.type payload_prefix_intel, @function

payload_prefix_intel:
    call $+10
    jmp $-256   # force a jmp rel32

payload_prefix_intel_end:

.data
.global payload_prefix_intel_length
.type payload_prefix_intel_length, @object

payload_prefix_intel_length: .quad payload_prefix_intel_end - payload_prefix_intel
