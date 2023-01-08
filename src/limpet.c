#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "elftoolkit.h"

int main(int argc, char **argv) {
    if (argc != 3)
        DIE("usage: %s <target> <payload>\n", argv[0]);

    Mapped_Elf *target = map_elf(argv[1]);
    Mapped_Payload *payload = map_payload(argv[2]);

    patch_text_gap(target, payload);

    unmap_payload(payload);
    unmap_elf(target);
}
