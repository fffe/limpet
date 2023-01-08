.arch armv8-a
.align 4
.text

payload_prefix_aarch64:
    bl payload_prefix_aarch64_end
    b payload_prefix_aarch64

payload_prefix_aarch64_end:
