#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "elftoolkit.h"

int main(int argc, char **argv) {
    if (argc != 3)
        DIE("usage: %s <target> <payload>\n", argv[0]);

    Mapped_Elf *target = map_elf(argv[1], 0);
    Mapped_Elf *payload = map_elf(argv[2], 1);

    if (target->class != payload->class)
        DIE("payload and target elf classes don't match\n");

    patch_text_gap(target, payload);

    unmap_elf(payload, 1);
    unmap_elf(target, 0);
}
