#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <string>
#include <list>
#include <map>

#define PACKED __attribute__((packed))

#define _NO_ELF_CLASS
#include <Elf.h>

#define STB_LOCAL       0
#define STB_GLOBAL      1
#define STB_WEAK        2
#define STB_LOPROC     13
#define STB_HIPROC     15

typedef void (*entry_point_t)(int, char*[]);

typedef struct _object_meta {
    std::string path;
    entry_point_t entry;

    void *mapped_file;
    size_t mapped_file_sz;

    uintptr_t loadBase;
    bool relocated;

    std::list<std::pair<void*, size_t> > memory_regions;

    ElfProgramHeader_t *phdrs;
    ElfSectionHeader_t *shdrs;

    ElfSectionHeader_t *sh_symtab;
    ElfSectionHeader_t *sh_strtab;

    ElfSymbol_t *symtab;
    const char *strtab;

    ElfSectionHeader_t *sh_shstrtab;
    const char *shstrtab;

    ElfProgramHeader_t *ph_dynamic;

    std::list<std::string> needed;

    ElfSymbol_t *dyn_symtab;
    const char *dyn_strtab;
    size_t dyn_strtab_sz;

    ElfRela_t *rela;
    ElfRel_t *rel;
    size_t rela_sz;
    size_t rel_sz;

    bool uses_rela;

    uintptr_t *got;

    ElfRela_t *plt_rela;
    ElfRel_t *plt_rel;

    uintptr_t init_func;
    uintptr_t fini_func;

    size_t plt_sz;

    ElfHash_t *hash;
    Elf_Word *hash_buckets;
    Elf_Word *hash_chains;

    std::list<struct _object_meta*> preloads;
    std::list<struct _object_meta*> objects;
} object_meta_t;

#define IS_NOT_PAGE_ALIGNED(x) (((x) & (getpagesize() - 1)) != 0)

bool loadObject(const char *filename, object_meta_t *meta);

bool findSymbol(const char *symbol, object_meta_t *meta, ElfSymbol_t &sym);

bool lookupSymbol(const char *symbol, object_meta_t *meta, ElfSymbol_t &sym, bool bWeak);

void doRelocation(object_meta_t *meta);

void doThisRelocation(ElfRel_t rel, object_meta_t *meta);
void doThisRelocation(ElfRela_t rel, object_meta_t *meta);

std::string symbolName(const ElfSymbol_t &sym, object_meta_t *meta);

std::string findObject(std::string name);

extern "C" void _libload_resolve_symbol();

std::list<std::string> g_lSearchPaths;

size_t elfhash(const char *name) {
    size_t h = 0, g = 0;
    while(*name) {
        h = (h << 4) + *name++;
        g = h & 0xF0000000;
        h ^= g;
        h ^= g >> 24;
    }

    return h;
}

extern "C" int main(int argc, char *argv[])
{
    // Sanity check: do we actually have a program to load?
    if(argc == 0) {
        return 0;
    }

    char *ld_libpath = getenv("LD_LIBRARY_PATH");
    char *ld_preload = getenv("LD_PRELOAD");

    g_lSearchPaths.push_back(std::string("/libraries"));
    g_lSearchPaths.push_back(std::string("."));

    if(ld_libpath) {
        // Parse, write.
        const char *entry;
        while((entry = strtok(ld_libpath, ":"))) {
            g_lSearchPaths.push_back(std::string(entry));
        }
    }

    printf("Search path:\n");
    for(std::list<std::string>::iterator it = g_lSearchPaths.begin();
        it != g_lSearchPaths.end();
        ++it) {
        printf(" -> %s\n", it->c_str());
    }

    /// \todo Implement dlopen etc in here.

    // Load the main object passed on the command line.
    object_meta_t *meta = new object_meta_t;
    if(!loadObject(argv[0], meta)) {
        delete meta;
        return ENOEXEC;
    }

    // Preload?
    if(ld_preload) {
        object_meta_t *preload = new object_meta_t;
        if(!loadObject(ld_preload, preload)) {
            printf("Loading preload '%s' failed.\n", ld_preload);
        } else {
            meta->preloads.push_back(preload);
        }
    }

    // Any libraries to load?
    if(meta->needed.size()) {
        for(std::list<std::string>::iterator it = meta->needed.begin();
            it != meta->needed.end();
            ++it) {
            object_meta_t *object = new object_meta_t;
            if(!loadObject(it->c_str(), object)) {
                printf("Loading '%s' failed.\n", it->c_str());
            } else {
                meta->objects.push_back(object);
            }
        }
    }

    // All done - run the program!
    printf("Calling entry point %p!\n", meta->entry);
    meta->entry(argc, argv);

    return 0;
}

std::string findObject(std::string name) {
    std::string fixed_path = name;
    std::list<std::string>::iterator it = g_lSearchPaths.begin();
    do {
        struct stat st;
        int l = stat(fixed_path.c_str(), &st);
        if(l == 0) {
            return fixed_path;
        }

        fixed_path = *it;
        fixed_path += "/";
        fixed_path += name;
    } while(++it != g_lSearchPaths.end());

    return std::string("<not found>");
}

bool loadObject(const char *filename, object_meta_t *meta) {
    printf("filename: %p\n", filename);
    meta->path = findObject(std::string(filename));

    // Okay, let's open up the file for reading...
    int fd = open(meta->path.c_str(), O_RDONLY);
    if(fd < 0) {
        fprintf(stderr, "libload.so: couldn't load object '%s' (%s) (%s)\n", filename, meta->path.c_str(), strerror(errno));
        return false;
    }

    // Open /dev/zero for BSS.
    int zerofd = open("/dev/zero", O_RDONLY);
    if(zerofd < 0) {
        zerofd = open("/dev/null", O_RDONLY);
        if(zerofd < 0) {
            printf("couldn't open /dev/zero or /dev/null\n");
            errno = ENOEXEC;
            return false;
        }
    }

    // Check header.
    ElfHeader_t header;
    int n = read(fd, &header, sizeof(header));
    if(n < 0) {
        close(fd);
        return false;
    } else if(n != sizeof(header)) {
        close(fd);
        errno = ENOEXEC;
        return false;
    }

    if(header.ident[1] != 'E' ||
       header.ident[2] != 'L' ||
       header.ident[3] != 'F' ||
       header.ident[0] != 127) {
        close(fd);
        errno = ENOEXEC;
        return false;
    }

    // Fairly confident we have an ELF binary now. Valid class?
    if(!(header.ident[4] == 1 || header.ident[4] == 2)) {
        close(fd);
        errno = ENOEXEC;
        return false;
    }

    meta->entry = (entry_point_t) header.entry;

    // Grab the size of the file - we'll mmap the entire thing, and pull out what we want.
    struct stat st;
    fstat(fd, &st);

    meta->mapped_file_sz = st.st_size;

    const char *pBuffer = (const char *) mmap(0, meta->mapped_file_sz, PROT_READ, MAP_PRIVATE, fd, 0);
    if(pBuffer == MAP_FAILED) {
        close(fd);
        errno = ENOEXEC;
        return false;
    }
    meta->mapped_file = (void *) pBuffer;

    meta->shdrs = (ElfSectionHeader_t *) &pBuffer[header.shoff];

    meta->sh_shstrtab = &meta->shdrs[header.shstrndx];
    meta->shstrtab = (const char *) &pBuffer[meta->sh_shstrtab->offset];

    // Find the symbol and string tables (these are not the dynamic ones).
    meta->sh_symtab = 0;
    meta->sh_strtab = 0;
    for(int i = 0; i < header.shnum; i++) {
        const char *name = meta->shstrtab + meta->shdrs[i].name;
        if(meta->shdrs[i].type == SHT_SYMTAB && !strcmp(name, ".symtab")) {
            meta->sh_symtab = &meta->shdrs[i];
        } else if(meta->shdrs[i].type == SHT_STRTAB && !strcmp(name, ".strtab")) {
            meta->sh_strtab = &meta->shdrs[i];
        }
    }

    meta->symtab = 0;
    meta->strtab = 0;

    if(meta->sh_symtab != 0) {
        meta->symtab = (ElfSymbol_t *) &pBuffer[meta->sh_symtab->offset];
    }

    if(meta->sh_strtab != 0) {
        meta->strtab = (const char *) &pBuffer[meta->sh_strtab->offset];
    }

    // Load program headers.
    meta->loadBase = 0;
    if(header.phnum) {
        meta->phdrs = (ElfProgramHeader_t *) &pBuffer[header.phoff];

        // NEEDED libraries are stored as offsets into the dynamic string table.
        std::list<uintptr_t> tmp_needed;

        for(size_t i = 0; i < header.phnum; i++) {
            if(meta->phdrs[i].type == PT_DYNAMIC) {
                ElfDyn_t *dyn = (ElfDyn_t *) &pBuffer[meta->phdrs[i].offset];

                while(dyn->tag != DT_NULL) {
                    switch(dyn->tag) {
                        case DT_NEEDED:
                            tmp_needed.push_back(dyn->un.ptr);
                            break;
                        case DT_SYMTAB:
                            meta->dyn_symtab = (ElfSymbol_t *) dyn->un.ptr;
                            break;
                        case DT_STRTAB:
                            meta->dyn_strtab = (const char *) dyn->un.ptr;
                            break;
                        case DT_STRSZ:
                            meta->dyn_strtab_sz = dyn->un.val;
                            break;
                        case DT_RELA:
                            meta->rela = (ElfRela_t *) dyn->un.ptr;
                            break;
                        case DT_REL:
                            meta->rel = (ElfRel_t *) dyn->un.ptr;
                            break;
                        case DT_RELASZ:
                            meta->rela_sz = dyn->un.val;
                            break;
                        case DT_RELSZ:
                            meta->rel_sz = dyn->un.val;
                            break;
                        case DT_PLTGOT:
                            meta->got = (uintptr_t *) dyn->un.ptr;
                            break;
                        case DT_JMPREL:
                            if(meta->uses_rela) {
                                meta->plt_rela = (ElfRela_t *) dyn->un.ptr;
                            } else {
                                meta->plt_rel = (ElfRel_t *) dyn->un.ptr;
                            }
                            break;
                        case DT_PLTREL:
                            meta->uses_rela = dyn->un.val == DT_RELA;
                            break;
                        case DT_PLTRELSZ:
                            meta->plt_sz = dyn->un.val;
                            break;
                        case DT_INIT:
                            meta->init_func = dyn->un.val;
                            break;
                        case DT_FINI:
                            meta->fini_func = dyn->un.val;
                            break;
                    }
                    dyn++;
                }
            } else if(meta->phdrs[i].type == PT_LOAD) {
                // Loadable data - use the flags to determine how we'll mmap.
                int flags = PROT_READ;
                if(meta->phdrs[i].flags & PF_X)
                    flags |= PROT_EXEC;
                if(meta->phdrs[i].flags & PF_W)
                    flags |= PROT_WRITE;

                size_t mapSize = meta->phdrs[i].memsz;
                if(IS_NOT_PAGE_ALIGNED(mapSize)) {
                    mapSize = (mapSize & ~(getpagesize() - 1)) + getpagesize();
                }

                size_t offset = meta->phdrs[i].offset;
                if(IS_NOT_PAGE_ALIGNED(offset)) {
                    offset &= ~(getpagesize() - 1);
                }

                uintptr_t realAddr = meta->phdrs[i].vaddr & ~(getpagesize() - 1);
                if(meta->loadBase == 0 && realAddr == 0) {
                    meta->relocated = true;
                } else if(meta->relocated && meta->loadBase) {
                    realAddr += meta->loadBase;
                }

                // Anonymous maps are used for the case where .bss exists, as some binaries
                // have .bss and .data in the same page. Which makes mmap hard.
                int type = 0;
                if(meta->phdrs[i].memsz != meta->phdrs[i].filesz) {
                    type = MAP_ANON;
                    offset = 0;
                } else if(!meta->relocated) {
                    type = MAP_FIXED;
                }

                void *loaded = mmap((void *) realAddr, mapSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | type, fd, offset);
                if(loaded == MAP_FAILED) {
                    // Unmap all previous mappings.
                    for(std::list<std::pair<void*, size_t> >::iterator it = meta->memory_regions.begin();
                        it != meta->memory_regions.end();
                        ++it) {
                        munmap(it->first, it->second);
                        it = meta->memory_regions.erase(it);
                    }

                    munmap(meta->mapped_file, meta->mapped_file_sz);
                    close(fd);

                    errno = ENOEXEC;
                    return false;
                }

                if(meta->phdrs[i].memsz > meta->phdrs[i].filesz) {
                    uintptr_t vaddr = meta->phdrs[i].vaddr;
                    if(meta->relocated) {
                        vaddr += meta->loadBase;
                    }

                    memcpy((void *) vaddr, &pBuffer[meta->phdrs[i].offset], meta->phdrs[i].filesz);
                }

                if((meta->phdrs[i].flags & PF_R) == 0) {
                    flags &= ~PROT_READ;
                }

                mprotect(loaded, mapSize, flags);

                meta->memory_regions.push_back(std::pair<void*, size_t>(loaded, mapSize));

                if(!meta->loadBase) {
                    if(meta->relocated) {
                        meta->loadBase = (uintptr_t) loaded;
                    } else {
                        meta->loadBase = meta->phdrs[i].vaddr;
                    }
                }
            }
        }

        if(meta->dyn_strtab) {
            for(std::list<uintptr_t>::iterator it = tmp_needed.begin();
                it != tmp_needed.end();
                ++it) {
                std::string s(meta->dyn_strtab + *it);
                meta->needed.push_back(s);
                it = tmp_needed.erase(it);
            }
        }
    } else {
        meta->phdrs = 0;
    }

    // Do another pass over section headers to try and get the hash table.
    for(size_t i = 0; i < header.shnum; i++) {
        if(meta->shdrs[i].type == SHT_HASH) {
            uintptr_t vaddr = meta->shdrs[meta->shdrs[i].link].offset + meta->loadBase;
            if(((uintptr_t) meta->dyn_symtab) == vaddr) {
                meta->hash = (ElfHash_t *) &pBuffer[meta->shdrs[i].offset];
                meta->hash_buckets = (Elf_Word *) &pBuffer[meta->shdrs[i].offset + sizeof(ElfHash_t)];
                meta->hash_chains = (Elf_Word *) &pBuffer[meta->shdrs[i].offset + sizeof(ElfHash_t) + (sizeof(Elf_Word) * meta->hash->nbucket)];
            }
        }
    }

    // Patch up the GOT so we can start resolving symbols when needed.
    if(meta->got) {
        meta->got[1] = (uintptr_t) meta;
        meta->got[2] = (uintptr_t) _libload_resolve_symbol;
    }

    // Do initial relocation of the binary.
    doRelocation(meta);

    // mmap complete - don't need the file descriptors open any longer.
    close(zerofd);
    close(fd);

    return true;
}

bool lookupSymbol(const char *symbol, object_meta_t *meta, ElfSymbol_t &sym, bool bWeak) {
    if(!meta) {
        return false;
    }

    // Allow preloads to override the main object symbol table, as well as any others.
    for(std::list<object_meta_t*>::iterator it = meta->preloads.begin();
        it != meta->preloads.end();
        ++it) {
        if(lookupSymbol(symbol, *it, sym, false))
            return true;
    }

    std::string sname(symbol);

    size_t hash = elfhash(symbol);
    size_t y = meta->hash_buckets[hash % meta->hash->nbucket];
    if(y > meta->hash->nchain) {
        return false;
    }

    do {
        sym = meta->dyn_symtab[y];
        if(symbolName(sym, meta) == sname) {
            if(bWeak) {
                if(ST_BIND(sym.info) == STB_WEAK) {
                    break;
                }
            } else if(ST_BIND(sym.info) == STB_GLOBAL || ST_BIND(sym.info) == STB_LOCAL) {
                if(sym.shndx) {
                    break;
                }
            }
        }
        y = meta->hash_chains[y];
    } while(y != 0);

    if(y != 0) {
        // Patch up the value.
        if(ST_TYPE(sym.info) < 3) {
            ElfSectionHeader_t *sh = &meta->shdrs[sym.shndx];
            sym.value += sh->addr;

            if(meta->relocated) {
                sym.value += meta->loadBase;
            }
        }
    }

    return y != 0;
}

bool findSymbol(const char *symbol, object_meta_t *meta, ElfSymbol_t &sym) {
    if(!meta) {
        return false;
    }

    for(std::list<object_meta_t*>::iterator it = meta->preloads.begin();
        it != meta->preloads.end();
        ++it) {
        if(lookupSymbol(symbol, *it, sym, false))
            return true;
    }

    for(std::list<object_meta_t*>::iterator it = meta->objects.begin();
        it != meta->objects.end();
        ++it) {
        if(lookupSymbol(symbol, *it, sym, false))
            return true;
    }

    // No luck? Try weak symbols in the main object.
    if(lookupSymbol(symbol, meta, sym, true))
        return true;

    return false;
}

std::string symbolName(const ElfSymbol_t &sym, object_meta_t *meta) {
    if(!meta) {
        return std::string("");
    } else if(sym.name == 0) {
        return std::string("<no name>");
    }

    ElfSymbol_t *symtab = meta->symtab;
    const char *strtab = meta->strtab;
    if(meta->dyn_symtab) {
        symtab = meta->dyn_symtab;
    }

    if(ST_TYPE(sym.info) == 3) {
        strtab = meta->shstrtab;
    } else if(meta->dyn_strtab) {
        strtab = meta->dyn_strtab;
    }

    const char *name = strtab + sym.name;
    return std::string(name);
}

void doRelocation(object_meta_t *meta) {
    if(meta->rel) {
        for(size_t i = 0; i < (meta->rel_sz / sizeof(ElfRel_t)); i++) {
            doThisRelocation(meta->rel[i], meta);
        }
    }

    if(meta->rela) {
        for(size_t i = 0; i < (meta->rela_sz / sizeof(ElfRela_t)); i++) {
            doThisRelocation(meta->rela[i], meta);
        }
    }

    if(meta->plt_rel) {
        for(size_t i = 0; i < (meta->plt_sz / sizeof(ElfRel_t)); i++) {
            if(meta->relocated) {
                uintptr_t *entry = (uintptr_t *) (meta->loadBase + meta->plt_rel[i].offset);
                *entry += meta->loadBase;
            }
        }
    }

    if(meta->plt_rela) {
        for(size_t i = 0; i < (meta->plt_sz / sizeof(ElfRela_t)); i++) {
            if(meta->relocated) {
                uintptr_t *entry = (uintptr_t *) (meta->loadBase + meta->plt_rela[i].offset);
                *entry += meta->loadBase;
            }
        }
    }
}

#define R_X86_64_NONE       0
#define R_X86_64_64         1
#define R_X86_64_PC32       2
#define R_X86_64_GOT32      3
#define R_X86_64_PLT32      4
#define R_X86_64_COPY       5
#define R_X86_64_GLOB_DAT   6
#define R_X86_64_JUMP_SLOT  7
#define R_X86_64_RELATIVE   8
#define R_X86_64_GOTPCREL   9
#define R_X86_64_32         10
#define R_X86_64_32S        11
#define R_X86_64_PC64       24
#define R_X86_64_GOTOFF64   25
#define R_X86_64_GOTPC32    26
#define R_X86_64_GOT64      27
#define R_X86_64_GOTPCREL64 28
#define R_X86_64_GOTPC64    29
#define R_X86_64_GOTPLT64   30
#define R_X86_64_PLTOFF64   31

#define R_386_NONE     0
#define R_386_32       1
#define R_386_PC32     2
#define R_386_GOT32    3
#define R_386_PLT32    4
#define R_386_COPY     5
#define R_386_GLOB_DAT 6
#define R_386_JMP_SLOT 7
#define R_386_RELATIVE 8
#define R_386_GOTOFF   9
#define R_386_GOTPC    10

void doThisRelocation(ElfRel_t rel, object_meta_t *meta) {
    ElfSymbol_t *symtab = meta->symtab;
    const char *strtab = meta->strtab;
    if(meta->dyn_symtab) {
        symtab = meta->dyn_symtab;
    }
    if(meta->dyn_strtab) {
        strtab = meta->dyn_strtab;
    }

    ElfSymbol_t *sym = &symtab[R_SYM(rel.info)];
    ElfSectionHeader_t *sh = 0;
    if(sym->shndx) {
        sh = &meta->shdrs[sym->shndx];
    }

    uintptr_t B = 0;
    if(meta->relocated) {
        B = meta->loadBase;
    }
    uintptr_t P = rel.offset;
    if(meta->relocated) {
        P += (sh ? sh->addr : B);
    }

    uintptr_t A = *((uintptr_t*) P);
    uintptr_t S = 0;

    std::string symbolname = symbolName(*sym, meta);
    printf("fixup for '%s'\n", symbolname.c_str());

    // Patch in section header?
    if(symtab && ST_TYPE(sym->info) == 3) {
        S = sh->addr;
    } else if(R_TYPE(rel.info) != R_386_RELATIVE) {
        if(sym->name == 0) {
            S = sym->value;
        } else {
            ElfSymbol_t lookupsym;
            if(R_TYPE(rel.info) == R_386_COPY) {
                // Search anything except the current object.
                if(!findSymbol(symbolname.c_str(), meta, lookupsym)) {
                    printf("symbol lookup for '%s' failed.\n", symbolname.c_str());
                    return;
                }
            } else {
                // Attempt a local lookup first.
                if(!lookupSymbol(symbolname.c_str(), meta, lookupsym, false)) {
                    printf("Couldn't look up in main object, trying others\n");
                    // No local symbol of that name - search other objects.
                    if(!findSymbol(symbolname.c_str(), meta, lookupsym)) {
                        printf("symbol lookup for '%s' failed.\n", symbolname.c_str());
                        return;
                    }
                }
            }

            S = lookupsym.value;
        }
    }

    uint32_t result = A;
    switch(R_TYPE(rel.info)) {
        case R_386_NONE:
            break;
        case R_386_32:
            result = S + A;
            break;
        case R_386_PC32:
            result = S + A - P;
            break;
        case R_386_JMP_SLOT:
        case R_386_GLOB_DAT:
            result = S;
        case R_386_COPY:
            result = *((uint32_t *) S);
            break;
        case R_386_RELATIVE:
            result = B + A;
            break;
    }

    *((uint32_t *) P) = result;
}

void doThisRelocation(ElfRela_t rel, object_meta_t *meta) {
    ElfSymbol_t *symtab = meta->symtab;
    const char *strtab = meta->strtab;
    if(meta->dyn_symtab) {
        symtab = meta->dyn_symtab;
    }
    if(meta->dyn_strtab) {
        strtab = meta->dyn_strtab;
    }

    ElfSymbol_t *sym = &symtab[R_SYM(rel.info)];
    ElfSectionHeader_t *sh = 0;
    if(sym->shndx) {
        sh = &meta->shdrs[sym->shndx];
    }

    uintptr_t A = rel.addend;
    uintptr_t S = 0;
    uintptr_t B = 0;
    if(meta->relocated) {
        B = meta->loadBase;
    }
    uintptr_t P = rel.offset;
    if(meta->relocated) {
        P += (sh ? sh->addr : B);
    }

    std::string symbolname = symbolName(*sym, meta);
    printf("fixup for '%s'\n", symbolname.c_str());

    // Patch in section header?
    if(symtab && ST_TYPE(sym->info) == 3) {
        S = sh->addr;
    } else if(R_TYPE(rel.info) != R_X86_64_RELATIVE) {
        if(sym->name == 0) {
            S = sym->value;
        } else {
            ElfSymbol_t lookupsym;
            if(R_TYPE(rel.info) == R_X86_64_COPY) {
                // Search anything except the current object.
                if(!findSymbol(symbolname.c_str(), meta, lookupsym)) {
                    printf("symbol lookup for '%s' failed.\n", symbolname.c_str());
                    return;
                }
            } else {
                // Attempt a local lookup first.
                if(!lookupSymbol(symbolname.c_str(), meta, lookupsym, false)) {
                    printf("Couldn't look up in main object, trying others\n");
                    // No local symbol of that name - search other objects.
                    if(!findSymbol(symbolname.c_str(), meta, lookupsym)) {
                        printf("symbol lookup for '%s' failed.\n", symbolname.c_str());
                        return;
                    }
                }
            }

            S = lookupsym.value;
        }
    }

    // Valid S?
    if((S == 0) && (R_TYPE(rel.info) != R_X86_64_RELATIVE)) {
        return;
    }

    uintptr_t result = *((uintptr_t *) P);
    switch(R_TYPE(rel.info)) {
        case R_X86_64_NONE:
            break;
        case R_X86_64_64:
            result = S + A;
            break;
        case R_X86_64_PC32:
            result = (result & 0xFFFFFFFF00000000) | ((S + A - P) & 0xFFFFFFFF);
            break;
        case R_X86_64_COPY:
            result = *((uintptr_t *) S);
            break;
        case R_X86_64_JUMP_SLOT:
        case R_X86_64_GLOB_DAT:
            result = S;
            break;
        case R_X86_64_RELATIVE:
            result = B + A;
            break;
        case R_X86_64_32:
        case R_X86_64_32S:
            result = (result & 0xFFFFFFFF00000000) | ((S + A) & 0xFFFFFFFF);
            break;
    }
    *((uintptr_t *) P) = result;
}

extern "C" void _libload_dofixup(uintptr_t id, uintptr_t symbol) {
    object_meta_t *meta = (object_meta_t *) id;
    printf("fixup for id=%p, symbol=%p\n", id, symbol);

    if(!symbol)
        return;

#ifdef BITS_32
    ElfRel_t rel = meta->rel[symbol];
#else
    ElfRela_t rel = meta->rela[symbol];
#endif
    doThisRelocation(rel, meta);

    printf("fixup done\n");
}
