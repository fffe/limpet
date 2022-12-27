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
    size_t size;
    int fd;
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

Mapped_Elf *map_elf(char *, int);
void unmap_elf(Mapped_Elf *, int);
void patch_text_gap(Mapped_Elf *, Mapped_Elf *);
#endif
