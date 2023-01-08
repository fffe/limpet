#ifndef ELFUTILS_H
#define ELFUTILS_H

#include <elf.h>

#define PAD(x, align) ((x + (align - 1)) & ~(align - 1))
#define DIE(...)                                                                                                       \
    do {                                                                                                               \
        fprintf(stderr, __VA_ARGS__);                                                                                  \
        exit(1);                                                                                                       \
    } while (0);

typedef struct mapped_elf {
    int fd;
    size_t size;
    int class;
    union {
        Elf32_Ehdr *ehdr32;
        Elf64_Ehdr *ehdr64;
    };
    union {
        Elf32_Phdr *phdr32;
        Elf64_Phdr *phdr64;
    };
    union {
        Elf32_Shdr *shdr32;
        Elf64_Shdr *shdr64;
    };
    uint8_t *strtab;
    uint8_t *data;
} Mapped_Elf;

typedef struct mapped_payload {
    int fd;
    size_t size;
    uint8_t *data;
} Mapped_Payload;

Mapped_Elf *map_elf(char *);
Mapped_Payload *map_payload(char *);
void unmap_elf(Mapped_Elf *);
void unmap_payload(Mapped_Payload *);
void patch_text_gap(Mapped_Elf *, Mapped_Payload *);
#endif
