#define _GNU_SOURCE

#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "elftoolkit.h"

#ifdef HAVE_ARM
#include "stubs/stub_arm.h"
#endif

#ifdef HAVE_AARCH64
#include "stubs/stub_aarch64.h"
#endif

#ifdef HAVE_X86
#include "stubs/stub_x86.h"
#endif

#ifdef HAVE_X86_64
#include "stubs/stub_x86-64.h"
#endif

int get_elf_class(Mapped_Elf *elf);
Elf32_Shdr *elf32_find_section(Mapped_Elf *, char *);
Elf64_Shdr *elf64_find_section(Mapped_Elf *, char *);

Mapped_Elf *map_elf(char *filename) {
    Mapped_Elf *elf = malloc(sizeof(Mapped_Elf));
    if (elf == NULL)
        DIE("error allocating memory for metadata");

    // open and map the file
    elf->fd = open(filename, O_RDWR);
    if (elf->fd == -1)
        DIE("error opening file: %s\n", filename);

    struct stat st;
    if (fstat(elf->fd, &st) == -1)
        DIE("error getting target file info\n");
    elf->size = st.st_size;

    elf->data = mmap(NULL, elf->size, PROT_READ | PROT_WRITE, MAP_SHARED, elf->fd, 0);
    if (elf->data == MAP_FAILED)
        DIE("failed to map file: %s\n", filename);

    // check for magic bytes
    if (!(elf->data[EI_MAG0] == ELFMAG0 && elf->data[EI_MAG1] == ELFMAG1 && elf->data[EI_MAG2] == ELFMAG2 &&
          elf->data[EI_MAG3] == ELFMAG3))
        DIE("missing ELF magic bytes\n");

    // get pointers to the elf header, section and program header tables, and the string table if it exists
    elf->class = get_elf_class(elf);
    if (elf->class == ELFCLASS32) {
        elf->ehdr32 = (Elf32_Ehdr *)elf->data;
        elf->phdr32 = (Elf32_Phdr *)(elf->data + elf->ehdr32->e_phoff);
        elf->shdr32 = (Elf32_Shdr *)(elf->data + elf->ehdr32->e_shoff);

        if (elf->ehdr32->e_shstrndx == SHN_UNDEF || elf->ehdr32->e_shstrndx == SHN_XINDEX)
            elf->strtab = NULL;
        else
            elf->strtab = (uint8_t *)(elf->data + elf->shdr32[elf->ehdr32->e_shstrndx].sh_offset);
    } else if (elf->class == ELFCLASS64) {
        elf->ehdr64 = (Elf64_Ehdr *)elf->data;
        elf->phdr64 = (Elf64_Phdr *)(elf->data + elf->ehdr64->e_phoff);
        elf->shdr64 = (Elf64_Shdr *)(elf->data + elf->ehdr64->e_shoff);

        if (elf->ehdr64->e_shstrndx == SHN_UNDEF || elf->ehdr64->e_shstrndx == SHN_XINDEX)
            elf->strtab = NULL;
        else
            elf->strtab = (uint8_t *)(elf->data + elf->shdr64[elf->ehdr64->e_shstrndx].sh_offset);
    } else
        DIE("unsupported elf class (not 32/64 bit)\n");

    return elf;
}

void unmap_elf(Mapped_Elf *elf) {
    msync(elf->data, elf->size, MS_SYNC);
    munmap(elf->data, elf->size);
    close(elf->fd);
    free(elf);
}

Mapped_Payload *map_payload(char *filename) {
    Mapped_Payload *payload = malloc(sizeof(Mapped_Payload));
    if (payload == NULL)
        DIE("error allocating memory for payload metadata");

    // open and map the file
    payload->fd = open(filename, O_RDWR);
    if (payload->fd == -1)
        DIE("error opening file: %s\n", filename);

    struct stat st;
    if (fstat(payload->fd, &st) == -1)
        DIE("error getting target file info\n");
    payload->size = st.st_size;

    payload->data = mmap(NULL, payload->size, PROT_READ, MAP_PRIVATE, payload->fd, 0);
    if (payload->data == MAP_FAILED)
        DIE("failed to map file: %s\n", filename);

    return payload;
}

void unmap_payload(Mapped_Payload *payload) {
    munmap(payload->data, payload->size);
    close(payload->fd);
    free(payload);
}

void patch_text_gap(Mapped_Elf *target, Mapped_Payload *payload) {
    uint32_t stub_len = 0, entry_point_offset = 0;
    uint8_t *stub = NULL;

    if (target->class == ELFCLASS32) {
        // look for a loadable, executable segment with a large enough gap for the payload
        for (int i = 0; i <= target->ehdr32->e_phnum; i++) {
            if (target->phdr32[i].p_type != PT_LOAD)
                continue;

            if ((target->phdr32[i].p_flags & (PF_R | PF_X)) != (PF_R | PF_X))
                continue;

            if (0) {
            }
#ifdef HAVE_X86
            else if (target->ehdr32->e_machine == EM_386) {
                stub_len = stub_x86_len;
                stub = stub_x86;
            }
#endif

            if (!stub_len || !stub)
                DIE("no support for this architecture");

            uint32_t aligned_memsz = PAD(target->phdr32[i].p_memsz, 4);
            if (PAD(target->phdr32[i].p_memsz, target->phdr32[i].p_align) - aligned_memsz < payload->size + stub_len)
                continue;

            if (0) {
            }
#ifdef HAVE_X86
            else if (target->ehdr32->e_machine == EM_386) {
                entry_point_offset = target->ehdr32->e_entry - (target->phdr32[i].p_vaddr + aligned_memsz + stub_len);
            }
#endif

            // set entry point
            target->ehdr32->e_entry = target->phdr32[i].p_vaddr + aligned_memsz;

            // write the stub
            uint8_t *patch_offset = target->data + PAD(target->phdr32[i].p_offset + target->phdr32[i].p_filesz, 4);
            memcpy(patch_offset, stub, stub_len);

            // patch the stub's branch back to the original entry point
            if (0) {
            }
#ifdef HAVE_X86
            else if (target->ehdr32->e_machine == EM_386) {
                memcpy(patch_offset + 6, &entry_point_offset, sizeof(uint32_t));
            }
#endif

            // add payload
            uint32_t padding = aligned_memsz - target->phdr64[i].p_memsz;
            memcpy(patch_offset + stub_len, payload->data, payload->size);
            target->phdr32[i].p_filesz += payload->size + stub_len + padding;
            target->phdr32[i].p_memsz += payload->size + stub_len + padding;
        }
        return;
    } else if (target->class == ELFCLASS64) {
        // look for a loadable, executable segment with a large enough gap for the payload
        for (int i = 0; i <= target->ehdr64->e_phnum; i++) {
            if (target->phdr64[i].p_type != PT_LOAD)
                continue;

            if ((target->phdr64[i].p_flags & (PF_R | PF_X)) != (PF_R | PF_X))
                continue;

            if (0) {
            }
#ifdef HAVE_X86_64
            else if (target->ehdr32->e_machine == EM_X86_64) {
                stub_len = stub_x86_64_len;
                stub = stub_x86_64;
            }
#endif
#ifdef HAVE_AARCH64
            else if (target->ehdr32->e_machine == EM_AARCH64) {
                stub_len = stub_aarch64_len;
                stub = stub_aarch64;
            }
#endif

            if (!stub_len || !stub)
                DIE("no support for this architecture");

            uint32_t aligned_memsz = PAD(target->phdr64[i].p_memsz, 4);
            if (PAD(target->phdr64[i].p_memsz, target->phdr64[i].p_align) - aligned_memsz < payload->size + stub_len)
                continue;

            if (0) {
            }
#ifdef HAVE_X86_64
            else if (target->ehdr32->e_machine == EM_X86_64) {
                entry_point_offset = target->ehdr64->e_entry - (target->phdr64[i].p_vaddr + aligned_memsz + stub_len);
            }
#endif
#ifdef HAVE_AARCH64
            else if (target->ehdr32->e_machine == EM_AARCH64) {
                entry_point_offset = (target->ehdr64->e_entry - (target->phdr64[i].p_vaddr + aligned_memsz + 4)) / 4;
            }
#endif

            // set entry point
            target->ehdr64->e_entry = target->phdr64[i].p_vaddr + aligned_memsz;

            // write the stub
            uint8_t *patch_offset = target->data + PAD(target->phdr64[i].p_offset + target->phdr64[i].p_filesz, 4);
            memcpy(patch_offset, stub, stub_len);

            // patch the stub's branch back to the original entry point
            if (0) {
            }
#ifdef HAVE_X86_64
            else if (target->ehdr64->e_machine == EM_X86_64) {
                memcpy(patch_offset + 6, &entry_point_offset, sizeof(uint32_t));
            }
#endif
#ifdef HAVE_AARCH64
            else if (target->ehdr64->e_machine == EM_AARCH64) {
                uint32_t opcode = *(uint32_t *)&patch_offset[4];
                opcode = (opcode & 0xfc000000) | (entry_point_offset & 0x03ffffff);
                memcpy(patch_offset + 4, &opcode, sizeof(uint32_t));
            }
#endif

            // add payload
            uint32_t padding = aligned_memsz - target->phdr64[i].p_memsz;
            memcpy(patch_offset + stub_len, payload->data, payload->size);
            target->phdr64[i].p_filesz += payload->size + stub_len + padding;
            target->phdr64[i].p_memsz += payload->size + stub_len + padding;
        }
        return;
    }
}

Elf32_Shdr *elf32_find_section(Mapped_Elf *elf, char *name) {
    if (!elf->strtab)
        return NULL;

    for (int i = 0; i < elf->ehdr32->e_shnum; i++)
        if (!strcmp((char *)&(elf->strtab[elf->shdr32[i].sh_name]), name))
            return &(elf->shdr32[i]);

    return NULL;
}

Elf64_Shdr *elf64_find_section(Mapped_Elf *elf, char *name) {
    if (!elf->strtab)
        return NULL;

    for (int i = 0; i < elf->ehdr64->e_shnum; i++)
        if (!strcmp((char *)&(elf->strtab[elf->shdr64[i].sh_name]), name))
            return &(elf->shdr64[i]);

    return NULL;
}

int get_elf_class(Mapped_Elf *elf) {
    if (!elf->data[EI_CLASS] || elf->data[EI_CLASS] >= ELFCLASSNUM)
        return 0;

    return elf->data[EI_CLASS];
}
