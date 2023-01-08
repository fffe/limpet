.intel_syntax noprefix
.text

payload_prefix_intel:
    call payload_prefix_intel_end
    jmp payload_prefix_intel

payload_prefix_intel_end:
