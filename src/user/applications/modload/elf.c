#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "elf.h"

Elf_t *elf_create(uint8_t *buffer, size_t len)
{
    Elf_t *elf = (Elf_t*)malloc(sizeof(Elf_t));
    // The main header will be at buffer[0].
    elf->header = (ElfHeader_t*)buffer;

    // Check the ident.
    if ( (elf->header->ident[1] != 'E') ||
        (elf->header->ident[2] != 'L') ||
        (elf->header->ident[3] != 'F') ||
        (elf->header->ident[0] != 127) )
    {
        printf("ELF file: ident check failed!");
        return 0;
    }

    // Check the bit-length.
    if (elf->header->ident[4] != 1)
    {
        printf("ELF file: wrong bit length!");
        return 0;
    }

    // Load in the section headers.
    elf->section_headers = (ElfSectionHeader_t*)malloc(sizeof(ElfSectionHeader_t)*elf->header->shnum);
    memcpy ((uint8_t*)elf->section_headers, &buffer[elf->header->shoff], elf->header->shnum*sizeof(ElfSectionHeader_t));

    // Find the section header string table.
    ElfSectionHeader_t *shstrtab_section = &elf->section_headers[elf->header->shstrndx];

    // Load the section header string table.
    char *shstrtab = (char*)malloc(shstrtab_section->size);
    memcpy ((uint8_t*)shstrtab, &buffer[shstrtab_section->offset], shstrtab_section->size);

    ElfSectionHeader_t *symtab=0, *strtab=0;
    // Go through each section header, trying to find .symtab.
    for (int i = 0; i < elf->header->shnum; i++)
    {
        const char *str = shstrtab + elf->section_headers[i].name;
        if (!strcmp(str, ".symtab"))
            symtab = &elf->section_headers[i];
        if (!strcmp(str, ".strtab"))
            strtab = &elf->section_headers[i];
    }

    if (symtab == 0)
    {
        printf("ELF: symbol table not found!");
        return 0;
    }
    else
    {
        elf->symtabsz = symtab->size/sizeof(ElfSymbol_t);
        elf->symtab = (ElfSymbol_t*)malloc(symtab->size);
        memcpy ((uint8_t*)elf->symtab, &buffer[symtab->offset], symtab->size);
    }

    if (strtab == 0)
    {
        printf("ELF: string table not found!");
        return 0;
    }
    else
    {
        elf->strtabsz = strtab->size;
        elf->strtab = (char*)malloc(strtab->size);
        memcpy ((uint8_t*)elf->strtab, &buffer[strtab->offset], strtab->size);
    }

    elf->buffer = buffer;

    return elf;
}

ElfSymbol_t *elf_get_symbol(Elf_t *elf, const char *name)
{
    for(size_t i = 0;i<elf->symtabsz;i++)
        if(!strcmp((char*)(elf->strtab + elf->symtab[i].name), name))
            return &(elf->symtab[i]);
    return 0;
}

uint8_t *elf_relptr(Elf_t *elf, ElfSymbol_t *sym, uint32_t ptr)
{
    ElfSectionHeader_t *relsh = 0;
    for(size_t i = 0;i<elf->header->shnum;i++)
    {
        if(elf->section_headers[i].type == SHT_REL && elf->section_headers[i].info == sym->shndx)
        {
            relsh = &(elf->section_headers[i]);
            break;
        }
    }
    if(!relsh)
        return 0;
    ElfRel_t *rel = 0;
    for(size_t i = 0;i<(relsh->size/sizeof(ElfRel_t));i++)
    {
        ElfRel_t *tmp = (ElfRel_t*)&(elf->buffer[relsh->offset+i*sizeof(ElfRel_t)]);
        if(tmp->offset == sym->value)
        {
            rel = tmp;
            break;
        }
    }
    if(!rel)
        return 0;
    ElfSymbol_t *relsym = &(elf->symtab[R_SYM(rel->info)]);
    if(ST_TYPE(relsym->info) != 3)
        return 0;
    return &elf->buffer[elf->section_headers[relsym->shndx].offset + ptr];
}


