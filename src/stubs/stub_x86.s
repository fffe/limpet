.intel_syntax noprefix
.text

payload_prefix_intel:
    call payload_prefix_intel_end

    # clang doesn't give a good way to force a rel32 jmp, so hardcode it
    # this is basically 'jmp payload_prefix_intel'
    .byte 0xe9
    .long ((payload_prefix_intel) - 4) - .

payload_prefix_intel_end:
