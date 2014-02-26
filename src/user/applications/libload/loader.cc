#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>

#include <string>
#include <list>
#include <map>
#include <set>

#define PACKED __attribute__((packed))

#define DEBUG_LIBLOAD

#define _NO_ELF_CLASS
#include <Elf.h>

#define STB_LOCAL       0
#define STB_GLOBAL      1
#define STB_WEAK        2
#define STB_LOPROC     13
#define STB_HIPROC     15

typedef void (*entry_point_t)(char*[], char **);
typedef void (*init_fini_func_t)();

enum LookupPolicy {
    LocalFirst,
    NotThisObject,
    LocalLast
};

typedef struct _object_meta {
    _object_meta() :
        filename(), path(), entry(0), mapped_file(0), mapped_file_sz(0),
        relocated(false), load_base(0), running(false), debug(false),
        memory_regions(), phdrs(0), shdrs(0), sh_symtab(0), sh_strtab(0),
        symtab(0), strtab(0), sh_shstrtab(0), shstrtab(0), ph_dynamic(0),
        needed(0), dyn_symtab(0), dyn_strtab(0), dyn_strtab_sz(0), rela(0),
        rel(0), rela_sz(0), rel_sz(0), uses_rela(false), got(0), plt_rela(0),
        plt_rel(0), init_func(0), fini_func(0), plt_sz(0), hash(0), hash_buckets(0),
        hash_chains(0), preloads(), objects(), parent(0)
    {}

    std::string filename;
    std::string path;
    entry_point_t entry;

    void *mapped_file;
    size_t mapped_file_sz;

    bool relocated;
    uintptr_t load_base;

    bool running;

    bool debug;

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

    struct _object_meta *parent;
} object_meta_t;

#define IS_NOT_PAGE_ALIGNED(x) (((x) & (getpagesize() - 1)) != 0)

extern "C" void *pedigree_sys_request_mem(size_t len);

bool loadObject(const char *filename, object_meta_t *meta, bool envpath = false);

bool loadSharedObjectHelper(const char *filename, object_meta_t *parent, object_meta_t **out = NULL, bool bRelocate = true);

bool findSymbol(const char *symbol, object_meta_t *meta, ElfSymbol_t &sym, LookupPolicy policy = LocalFirst);

bool lookupSymbol(const char *symbol, object_meta_t *meta, ElfSymbol_t &sym, bool bWeak, bool bGlobal = true);

void doRelocation(object_meta_t *meta);

uintptr_t doThisRelocation(ElfRel_t rel, object_meta_t *meta);
uintptr_t doThisRelocation(ElfRela_t rel, object_meta_t *meta);

std::string symbolName(const ElfSymbol_t &sym, object_meta_t *meta, bool bNoDynamic = false);

std::string findObject(std::string name, bool envpath);

extern "C" void *_libload_dlopen(const char *file, int mode);
extern "C" void *_libload_dlsym(void *handle, const char *name);
extern "C" int _libload_dlclose(void *handle);

extern "C" uintptr_t _libload_resolve_symbol();

object_meta_t *g_MainObject = 0;

std::list<std::string> g_lSearchPaths;

std::set<std::string> g_LoadedObjects;

std::map<std::string, uintptr_t> g_LibLoadSymbols;

extern char __elf_start;
extern char __start_bss;
extern char __end_bss;

extern "C" void _init();
extern "C" void _fini();
extern "C" int _start(char *argv[], char *env[]);

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

extern char **environ;

/**
 * Entry point for the dynamic linker. Maps in .bss (as libload.so is loaded
 * as a simple mmap + jump to entry - no proper section loading) and calls
 * the init and fini functions around the call to crt0's _start, which is
 * the normal entry point for an application.
 */
extern "C" int _libload_main(char *argv[], char *env[])
{
    /// \note THIS IS CALLED BEFORE CRT0. Do not use argc/argv/environ,
    ///       and keep everything as minimal as possible.

    // Get .bss loaded and ready early.
    uintptr_t bssStart = reinterpret_cast<uintptr_t>(&__start_bss);
    uintptr_t bssEnd = reinterpret_cast<uintptr_t>(&__end_bss);
    mmap(
        &__start_bss,
        bssEnd - bssStart,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON | MAP_FIXED | MAP_USERSVD,
        -1,
        0);

    // Run .init stuff.
    _init();

    int ret = _start(argv, env);

    // Run .fini stuff.
    _fini();

    return ret;
}

extern "C" int main(int argc, char *argv[])
{
    // Sanity check: do we actually have a program to load?
    if(argc == 0) {
        return 0;
    }

    syslog(LOG_INFO, "libload.so starting...");

    char *ld_libpath = getenv("LD_LIBRARY_PATH");
    char *ld_preload = getenv("LD_PRELOAD");
    char *ld_debug = getenv("LD_DEBUG");

    g_lSearchPaths.push_back(std::string("/libraries"));
    g_lSearchPaths.push_back(std::string("."));

    // Prepare for libload hooks.
    g_LibLoadSymbols.insert(std::pair<std::string, uintptr_t>("_libload_dlopen", (uintptr_t) _libload_dlopen));
    g_LibLoadSymbols.insert(std::pair<std::string, uintptr_t>("_libload_dlsym", (uintptr_t) _libload_dlsym));
    g_LibLoadSymbols.insert(std::pair<std::string, uintptr_t>("_libload_dlopen", (uintptr_t) _libload_dlclose));

    if(ld_libpath) {
        // Parse, write.
        const char *entry;
        while((entry = strtok(ld_libpath, ":"))) {
            g_lSearchPaths.push_back(std::string(entry));
        }
    }

    if(ld_debug) {
        fprintf(stderr, "libload.so: search path is\n");
        for(std::list<std::string>::iterator it = g_lSearchPaths.begin();
            it != g_lSearchPaths.end();
            ++it) {
            printf(" -> %s\n", it->c_str());
        }
    }

    syslog(LOG_INFO, "libload.so loading main object");

    // Ungodly hack.
    if(!strcmp(argv[0], "sh") || !strcmp(argv[0], "/bin/sh"))
    {
        // Get user's shell.
        argv[0] = getenv("SHELL");
        if(!argv[0])
        {
            // Assume bash.
            syslog(LOG_WARNING, "libload: $SHELL is undefined and /bin/sh was requested");
            argv[0] = "bash";
        }
    }

    syslog(LOG_INFO, "libload.so main object is %s", argv[0]);

    // Load the main object passed on the command line.
    object_meta_t *meta = g_MainObject = new object_meta_t;
    meta->debug = false;
    meta->running = false;
    if(!loadObject(argv[0], meta, true)) {
        delete meta;
        return ENOEXEC;
    }

    g_LoadedObjects.insert(meta->filename);

    syslog(LOG_INFO, "libload.so loading preload, if one exists");

    // Preload?
    if(ld_preload) {
        object_meta_t *preload = new object_meta_t;
        if(!loadObject(ld_preload, preload)) {
            printf("Loading preload '%s' failed.\n", ld_preload);
        } else {
            preload->parent = meta;
            meta->preloads.push_back(preload);

            g_LoadedObjects.insert(preload->filename);
        }
    }

    syslog(LOG_INFO, "libload.so loading dependencies");

    // Any libraries to load?
    if(meta->needed.size()) {
        for(std::list<std::string>::iterator it = meta->needed.begin();
            it != meta->needed.end();
            ++it) {
            if(g_LoadedObjects.find(*it) == g_LoadedObjects.end())
                loadSharedObjectHelper(it->c_str(), meta, NULL, false);
        }
    }

    syslog(LOG_INFO, "libload.so relocating dependencies");

    // Relocate preloads.
    for(std::list<struct _object_meta *>::iterator it = meta->preloads.begin();
        it != meta->preloads.end();
        ++it) {
        doRelocation(*it);
    }

    // Relocate all other loaded objects.
    for(std::list<struct _object_meta *>::iterator it = meta->objects.begin();
        it != meta->objects.end();
        ++it) {
        doRelocation(*it);
    }

    syslog(LOG_INFO, "libload.so relocating main object");

    // Do initial relocation of the binary (non-GOT entries)
    doRelocation(meta);

    syslog(LOG_INFO, "libload.so running entry point");

    // All done - run the program!
    meta->running = true;

    // Run init functions in loaded objects.
    for(std::list<struct _object_meta *>::iterator it = meta->objects.begin();
        it != meta->objects.end();
        ++it) {
        if((*it)->init_func) {
            init_fini_func_t init = (init_fini_func_t) (*it)->init_func;
            init();
        }
    }

    // Run init function, if one exists.
    if(meta->init_func) {
        init_fini_func_t init = (init_fini_func_t) meta->init_func;
        init();
    }

    meta->entry(argv, environ);

    return 0;
}

std::string findObject(std::string name, bool envpath) {
    std::list<std::string>::iterator it = g_lSearchPaths.begin();
    if(it != g_lSearchPaths.end())
    {
        std::string fixed_path;

        // Don't do fixup for an absolute path.
        if((name[0] == '.') || (name[0] == '/') || (name.find("»") != std::string::npos))
            fixed_path = name;
        else {
            fixed_path = *it;
            fixed_path += "/";
            fixed_path += name;
        }

        do {
#ifdef SUPERDEBUG
            syslog(LOG_INFO, "Trying %s", fixed_path.c_str());
#endif

            struct stat st;
            int l = stat(fixed_path.c_str(), &st);
            if((l == 0) && S_ISREG(st.st_mode)) {
                return fixed_path;
            }

            fixed_path = *it;
            fixed_path += "/";
            fixed_path += name;
        } while(++it != g_lSearchPaths.end());
    }

    if(envpath) {
        // Check $PATH for the file.
        char *path = getenv("PATH");

        if(path) {
            // Parse, write.
            const char *entry = strtok(path, ":");
            if(entry) {
                do {
                    g_lSearchPaths.push_back(std::string(entry));

                    /// \todo Handle environment variables in entry if needed.
                    std::string fixed_path(entry);
                    fixed_path += "/";
                    fixed_path += name;

#ifdef SUPERDEBUG
                    syslog(LOG_INFO, "Trying %s", fixed_path.c_str());
#endif

                    std::string result = findObject(fixed_path, false);

                    if(result.compare("<not found>") != 0)
                        return result;

                    // Remove from the search paths - wasn't found.
                    g_lSearchPaths.pop_back();
                } while((entry = strtok(NULL, ":")));
            }
        }
    }

    return std::string("<not found>");
}

bool loadSharedObjectHelper(const char *filename, object_meta_t *parent, object_meta_t **out, bool bRelocate) {
    object_meta_t *object = new object_meta_t;
    bool bSuccess = true;
    if(!loadObject(filename, object)) {
        printf("Loading '%s' failed.\n", filename);
        bSuccess = false;
    } else {
        object->parent = parent;
        parent->objects.push_back(object);
        g_LoadedObjects.insert(object->filename);

        if(object->needed.size()) {
            for(std::list<std::string>::iterator it = object->needed.begin();
                it != object->needed.end();
                ++it) {
                if(g_LoadedObjects.find(*it) == g_LoadedObjects.end()) {
                    object_meta_t *child = 0;
                    bSuccess = loadSharedObjectHelper(it->c_str(), parent, &child, bRelocate);
                }
            }
        }

    }

    if(out) {
        *out = object;
    }

    // Relocate object - now loaded.
    if(bRelocate) {
        doRelocation(object);
    }

    return bSuccess;
}

#include <syslog.h>
bool loadObject(const char *filename, object_meta_t *meta, bool envpath) {
    meta->filename = filename;
    meta->path = findObject(meta->filename, envpath);
    syslog(LOG_INFO, "libload.so loading %s", filename);

    // Okay, let's open up the file for reading...
    int fd = open(meta->path.c_str(), O_RDONLY);
    if(fd < 0) {
        fprintf(stderr, "libload.so: couldn't load object '%s' (%s) (%s)\n", filename, meta->path.c_str(), strerror(errno));
        return false;
    }

    // Check header.
    ElfHeader_t header;
    int n = read(fd, &header, sizeof(header));
    if(n < 0) {
#ifdef DEBUG_LIBLOAD
        fprintf(stderr, "libload.so: couldn't read file header (%s)\n", strerror(errno));
#endif
        close(fd);
        return false;
    } else if(n != sizeof(header)) {
#ifdef DEBUG_LIBLOAD
        fprintf(stderr, "libload.so: read was not the correct size\n");
#endif
        close(fd);
        errno = ENOEXEC;
        return false;
    }

    if(header.ident[1] != 'E' ||
       header.ident[2] != 'L' ||
       header.ident[3] != 'F' ||
       header.ident[0] != 127) {
#ifdef DEBUG_LIBLOAD
        fprintf(stderr, "libload.so: bad ELF magic\n");
#endif
        close(fd);
        errno = ENOEXEC;
        return false;
    }

    // Fairly confident we have an ELF binary now. Valid class?
    if(!(header.ident[4] == 1 || header.ident[4] == 2)) {
#ifdef DEBUG_LIBLOAD
        fprintf(stderr, "libload.so: not a valid ELF class\n");
#endif
        close(fd);
        errno = ENOEXEC;
        return false;
    }

    meta->entry = (entry_point_t) header.entry;

    // Grab the size of the file - we'll mmap the entire thing, and pull out what we want.
    struct stat st;
    fstat(fd, &st);

    meta->mapped_file_sz = st.st_size;

    const char *pBuffer = (const char *) mmap(0, meta->mapped_file_sz, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if(pBuffer == MAP_FAILED) {
#ifdef DEBUG_LIBLOAD
        fprintf(stderr, "libload.so: could not mmap binary\n");
#endif
        close(fd);
        errno = ENOEXEC;
        return false;
    }
    meta->mapped_file = (void *) pBuffer;

    meta->phdrs = (ElfProgramHeader_t *) &pBuffer[header.phoff];
    meta->shdrs = (ElfSectionHeader_t *) &pBuffer[header.shoff];

    if(header.type == ET_REL) {
        meta->relocated = true;
    } else if(header.type == ET_EXEC || header.type == ET_DYN) {
        // First program header zero?
        if(header.phnum) {
            meta->relocated = (meta->phdrs[0].vaddr & ~(getpagesize() - 1)) == 0;
        }
    }

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
    meta->load_base = 0;
    if(header.phnum) {
        if(meta->relocated) {
            // Reserve space for the full loaded virtual address space of the process.
            meta->load_base = meta->phdrs[0].vaddr & ~(getpagesize() - 1);
            uintptr_t finalAddress = 0;
            for(size_t i = 0; i < header.phnum; ++i) {
                uintptr_t endAddr = meta->phdrs[i].vaddr + meta->phdrs[i].memsz;
                finalAddress = std::max(finalAddress, endAddr);
            }
            size_t mapSize = finalAddress - meta->load_base;
            if(mapSize & (getpagesize() - 1)) {
                mapSize = (mapSize + getpagesize()) & ~(getpagesize() - 1);
            }

            void *p = pedigree_sys_request_mem(mapSize);
            if(!p) {
                munmap((void *) pBuffer, meta->mapped_file_sz);
                errno = ENOEXEC;
                return false;
            }

            meta->load_base = (uintptr_t) p;
            if(meta->load_base & (getpagesize() - 1)) {
                meta->load_base = (meta->load_base + getpagesize()) & ~(getpagesize() - 1);
            }

            // Patch up section headers, quickly.
            for(size_t shdx = 0; shdx < header.shnum; ++shdx) {
                meta->shdrs[shdx].addr += meta->load_base;
            }
        }

        // NEEDED libraries are stored as offsets into the dynamic string table.
        std::list<uintptr_t> tmp_needed;

        for(size_t i = 0; i < header.phnum; i++) {
            if(meta->phdrs[i].type == PT_DYNAMIC) {
                meta->ph_dynamic = &meta->phdrs[i];
                if(meta->relocated) {
                    meta->ph_dynamic->vaddr += meta->load_base;
                }

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
                if((meta->phdrs[i].flags & PF_R) == 0)
                    flags &= ~PROT_READ;

                if(meta->relocated) {
                    meta->phdrs[i].vaddr += meta->load_base;
                }

                uintptr_t phdr_base = meta->phdrs[i].vaddr;

                size_t pagesz = getpagesize();

                size_t base_addend = phdr_base & (pagesz - 1);
                phdr_base &= ~(pagesz - 1);

#if 0
                size_t offset = meta->phdrs[i].offset & ~(pagesz - 1);
                size_t offset_addend = meta->phdrs[i].offset & (pagesz - 1);
                size_t mapsz = offset_addend + meta->phdrs[i].filesz;
#else
                size_t offset = 0;
                if(meta->phdrs[i].offset)
                {
                    offset = meta->phdrs[i].offset - base_addend;
                }

                size_t mapsz = base_addend + meta->phdrs[i].filesz;
#endif

                // Already mapped?
                if((msync((void *) phdr_base, mapsz, MS_SYNC) != 0) && (errno == ENOMEM)) {
                    int mapflags = MAP_FIXED | MAP_PRIVATE;
                    if(meta->relocated) {
                        mapflags |= MAP_USERSVD;
                    }

                    int map_fd = fd;
                    if((flags & PROT_WRITE) == 0)
                    {
                        // If the program header is read-only, we can quite
                        // happily share the mapping and reduce memory
                        // usage across the system.
                        mapflags &= ~(MAP_PRIVATE);
                    }

                    // Zero out additional space.
                    if(meta->phdrs[i].memsz > meta->phdrs[i].filesz)
                    {
                        uintptr_t vaddr_start = meta->phdrs[i].vaddr + meta->phdrs[i].filesz;
                        uintptr_t vaddr_end = meta->phdrs[i].vaddr + meta->phdrs[i].memsz;
                        if((vaddr_start & ~(getpagesize() - 1)) !=
                            (vaddr_end & ~(getpagesize() - 1)))
                        {
                            // Not the same page, won't be mapped in with the
                            // rest of the program header. Map the rest as an
                            // anonymous mapping.
                            vaddr_start += getpagesize();
                            vaddr_start &= ~(getpagesize() - 1);
                            size_t totalLength = vaddr_end - vaddr_start;
                            void *p = mmap((void *) vaddr_start, totalLength, PROT_READ | PROT_WRITE, mapflags | MAP_ANON, 0, 0);
                            if(p == MAP_FAILED)
                            {
                                /// \todo cleanup.
                                errno = ENOEXEC;
                                return false;
                            }
                            meta->memory_regions.push_back(std::pair<void *, size_t>(p, totalLength));
                        }
                    }

                    void *p = mmap((void *) phdr_base, mapsz, PROT_READ | PROT_WRITE, mapflags, map_fd, map_fd ? offset : 0);
                    if(p == MAP_FAILED) {
                        /// \todo cleanup.
                        errno = ENOEXEC;
                        return false;
                    }

                    meta->memory_regions.push_back(std::pair<void *, size_t>(p, mapsz));
                }

                // mprotect accordingly.
                size_t alignExtra = meta->phdrs[i].vaddr & (getpagesize() - 1);
                uintptr_t protectaddr = meta->phdrs[i].vaddr & ~(getpagesize() - 1);
                mprotect((void *) protectaddr, meta->phdrs[i].memsz + alignExtra, flags);
            }
        }

        if(meta->relocated) {
            uintptr_t base_vaddr = meta->load_base;

            // Patch up references.
            if(meta->dyn_strtab)
                meta->dyn_strtab += base_vaddr;
            if(meta->dyn_symtab)
                meta->dyn_symtab = (ElfSymbol_t *) (((uintptr_t) meta->dyn_symtab) + base_vaddr);
            if(meta->got)
                meta->got = (uintptr_t *) (((uintptr_t) meta->got) + base_vaddr);
            if(meta->rela)
                meta->rela = (ElfRela_t *) (((uintptr_t) meta->rela) + base_vaddr);
            if(meta->rel)
                meta->rel = (ElfRel_t *) (((uintptr_t) meta->rel) + base_vaddr);
            if(meta->plt_rela)
                meta->plt_rela = (ElfRela_t *) (((uintptr_t) meta->plt_rela) + base_vaddr);
            if(meta->plt_rel)
                meta->plt_rel = (ElfRel_t *) (((uintptr_t) meta->plt_rel) + base_vaddr);
            if(meta->init_func)
                meta->init_func += base_vaddr;
            if(meta->fini_func)
                meta->fini_func += base_vaddr;
        }

        if(meta->dyn_strtab) {
            for(std::list<uintptr_t>::iterator it = tmp_needed.begin();
                it != tmp_needed.end();
                ++it) {
                std::string s(meta->dyn_strtab + *it);
                meta->needed.push_back(s);
            }
        }
    } else {
        meta->phdrs = 0;
    }

    // Do another pass over section headers to try and get the hash table.
    meta->hash = 0;
    meta->hash_buckets = 0;
    meta->hash_chains = 0;
    for(size_t i = 0; i < header.shnum; i++) {
        if(meta->shdrs[i].type == SHT_HASH) {
            uintptr_t vaddr = meta->shdrs[meta->shdrs[i].link].addr;
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

    // mmap complete - don't need the file descriptors open any longer.
    close(fd);

    return true;
}

bool lookupSymbol(const char *symbol, object_meta_t *meta, ElfSymbol_t &sym, bool bWeak, bool bGlobal) {
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

    // Try a local lookup first.
    do {
        sym = meta->dyn_symtab[y];
        if(symbolName(sym, meta) == sname) {
            if(ST_BIND(sym.info) == STB_LOCAL) {
                if(sym.shndx) {
                    break;
                }
            }
            if(bWeak) {
                if(ST_BIND(sym.info) == STB_WEAK) {
                    // sym.value = (uintptr_t) ~0UL;
                    break;
                }
            }
        }
        y = meta->hash_chains[y];
    } while(y != 0);

    // No local symbols found - try a global lookup.
    if((bGlobal) && (y == 0)) {
        y = meta->hash_buckets[hash % meta->hash->nbucket];
        do {
            sym = meta->dyn_symtab[y];
            if(symbolName(sym, meta) == sname) {
                if(ST_BIND(sym.info) == STB_GLOBAL) {
                    if(sym.shndx) {
                        break;
                    }
                }
            }
            y = meta->hash_chains[y];
        } while(y != 0);
    }

    if(y != 0) {
        // Patch up the value.
        if(ST_TYPE(sym.info) < 3) { // && ST_BIND(sym.info) != STB_WEAK) {
            if(sym.shndx && meta->relocated) {
                sym.value += meta->load_base;
            }
        }
    }

    return y != 0;
}

bool findSymbol(const char *symbol, object_meta_t *meta, ElfSymbol_t &sym, LookupPolicy policy) {
    if(!meta) {
        return false;
    }

    // Do we override or not?
    std::map<std::string, uintptr_t>::iterator it = g_LibLoadSymbols.find(std::string(symbol));
    if(it != g_LibLoadSymbols.end()) {
        if(it->first == symbol) {
            sym.value = it->second;
            return true;
        }
    }

    object_meta_t *ext_meta = meta;
    while(ext_meta->parent) {
        ext_meta = ext_meta->parent;
    }

    // Try preloads.
    for(std::list<object_meta_t*>::iterator it = ext_meta->preloads.begin();
        it != ext_meta->preloads.end();
        ++it) {
        if(lookupSymbol(symbol, *it, sym, false))
            return true;
    }

    // If we are allowed, check for non-weak symbols in this binary.
    if(policy != NotThisObject && policy != LocalLast) {
        if(lookupSymbol(symbol, meta, sym, false, false))
            return true;
    }

    // Try the parent object.
    if((meta != ext_meta) && lookupSymbol(symbol, ext_meta, sym, false))
        return true;

    // Now, try any loaded objects we might have.
    for(std::list<object_meta_t*>::iterator it = ext_meta->objects.begin();
        it != ext_meta->objects.end();
        ++it) {
        if(*it == meta)
            continue; // Already handling.

        if(lookupSymbol(symbol, *it, sym, false)) {
            return true;
        }
    }

    // Try a local lookup if not found.
    if(policy == LocalLast) {
        if(lookupSymbol(symbol, meta, sym, false, false))
            return true;
    }

    // Try libraries again but accept weak symbols.
    for(std::list<object_meta_t*>::iterator it = ext_meta->objects.begin();
        it != ext_meta->objects.end();
        ++it) {
        if(*it == meta)
            continue; // Already handling this object.

        if(lookupSymbol(symbol, *it, sym, true)) {
            return true;
        }
    }

    // Try weak symbols in the parent object.
    if((meta != ext_meta) && lookupSymbol(symbol, ext_meta, sym, true))
        return true;

    // No luck? Try weak symbols in the main object.
    if(policy != NotThisObject)
    {
        if(lookupSymbol(symbol, meta, sym, true))
            return true;
    }

    return false;
}

std::string symbolName(const ElfSymbol_t &sym, object_meta_t *meta, bool bNoDynamic) {
    if(!meta) {
        return std::string("");
    } else if(sym.name == 0) {
        return std::string("");
    }

    ElfSymbol_t *symtab = meta->symtab;
    const char *strtab = meta->strtab;

    if((!bNoDynamic) && meta->dyn_symtab) {
        symtab = meta->dyn_symtab;
    }

    if(ST_TYPE(sym.info) == 3) {
        strtab = meta->shstrtab;
    } else if((!bNoDynamic) && meta->dyn_strtab) {
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

    // Relocated binaries need to have the GOTPLT fixed up, as each entry points to
    // a non-relocated address (that is also not relative).
    if(meta->relocated) {
        uintptr_t base = meta->load_base;

        if(meta->plt_rel) {
            for(size_t i = 0; i < (meta->plt_sz / sizeof(ElfRel_t)); i++) {
                uintptr_t *addr = (uintptr_t *) (base + meta->plt_rel[i].offset);
                *addr += base;
            }
        }

        if(meta->plt_rela) {
            for(size_t i = 0; i < (meta->plt_sz / sizeof(ElfRela_t)); i++) {
                uintptr_t *addr = (uintptr_t *) (base + meta->plt_rela[i].offset);
                *addr += base;
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

uintptr_t doThisRelocation(ElfRel_t rel, object_meta_t *meta) {
    ElfSymbol_t *symtab = meta->symtab;
    if(meta->dyn_symtab) {
        symtab = meta->dyn_symtab;
    }

    ElfSymbol_t *sym = &symtab[R_SYM(rel.info)];
    ElfSectionHeader_t *sh = 0;
    if(sym->shndx) {
        sh = &meta->shdrs[sym->shndx];
    }

    uintptr_t B = meta->load_base;
    uintptr_t P = rel.offset;
    if(meta->relocated) {
        P += B;
    }

    uintptr_t A = *((uintptr_t*) P);
    uintptr_t S = 0;

    std::string symbolname = symbolName(*sym, meta);
    size_t symbolSize = sizeof(uintptr_t);

    // Patch in section header?
    if(symtab && ST_TYPE(sym->info) == 3) {
        S = sh->addr;
    } else if(R_TYPE(rel.info) != R_386_RELATIVE) {
        if(sym->name == 0) {
            S = sym->value;
        } else {
            ElfSymbol_t lookupsym;
            LookupPolicy policy = LocalFirst;
            if(R_TYPE(rel.info) == R_386_COPY) {
                policy = NotThisObject;
            }

            // Attempt to find the symbol.
            if(!findSymbol(symbolname.c_str(), meta, lookupsym, policy)) {
                printf("symbol lookup for '%s' (needed in '%s') failed.\n", symbolname.c_str(), meta->path.c_str());
                lookupsym.value = (uintptr_t) ~0UL;
            }

            S = lookupsym.value;
            symbolSize = lookupsym.size;
        }
    }

    if(S == (uintptr_t) ~0UL) {
        S = 0;
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
            break;
        case R_386_COPY:
            // Only copy if not null.
            if(S)
            {
                memcpy((void *) P, (void *) S, symbolSize);
                result = P;
            }
            break;
        case R_386_RELATIVE:
            result = B + A;
            break;
    }

    if(R_TYPE(rel.info) != R_386_COPY) {
        *((uint32_t *) P) = result;
    }
    return result;
}

uintptr_t doThisRelocation(ElfRela_t rel, object_meta_t *meta) {
    ElfSymbol_t *symtab = meta->symtab;
    if(meta->dyn_symtab) {
        symtab = meta->dyn_symtab;
    }

    ElfSymbol_t *sym = &symtab[R_SYM(rel.info)];
    ElfSectionHeader_t *sh = 0;
    if(sym->shndx) {
        sh = &meta->shdrs[sym->shndx];
    }

    uintptr_t B = meta->load_base;
    uintptr_t P = rel.offset;
    if(meta->relocated) {
        P += B;
    }

    uintptr_t A = *((uintptr_t*) P);
    uintptr_t S = 0;

    std::string symbolname = symbolName(*sym, meta);
    size_t symbolSize = sizeof(uintptr_t);

    // Patch in section header?
    if(symtab && ST_TYPE(sym->info) == 3) {
        S = sh->addr;
    } else if(R_TYPE(rel.info) != R_X86_64_RELATIVE) {
        if(sym->name == 0) {
            S = sym->value;
        } else {
            ElfSymbol_t lookupsym;
            LookupPolicy policy = LocalFirst;
            if(R_TYPE(rel.info) == R_X86_64_COPY) {
                policy = NotThisObject;
            }

            // Attempt to find the symbol.
            if(!findSymbol(symbolname.c_str(), meta, lookupsym, policy)) {
                printf("symbol lookup for '%s' (needed in '%s') failed.\n", symbolname.c_str(), meta->path.c_str());
                lookupsym.value = (uintptr_t) ~0UL;
            }

            S = lookupsym.value;
            symbolSize = lookupsym.size;
        }
    }

    // Valid S?
    if((S == 0) && (R_TYPE(rel.info) != R_X86_64_RELATIVE)) {
        return (uintptr_t) ~0UL;
    }

    // Weak symbol.
    if(S == (uintptr_t) ~0UL) {
        S = 0;
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
            // Only copy if not null.
            if(S)
            {
                memcpy((void *) P, (void *) S, symbolSize);
                result = P;
            }
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
        default:
            syslog(LOG_WARNING, "libload: unsupported relocation for '%s' in %s: %d", symbolname.c_str(), meta->filename.c_str(), R_TYPE(rel.info));
    }

    if(R_TYPE(rel.info) != R_X86_64_COPY)
    {
        *((uintptr_t *) P) = result;
    }
    return result;
}

extern "C" uintptr_t _libload_dofixup(uintptr_t id, uintptr_t symbol) {
    object_meta_t *meta = (object_meta_t *) id;
    uintptr_t returnaddr = 0;

    // Save FP state (glue may use SSE, and we are not called by normal means
    // so there's no caller-save).
    typedef float xmm_t __attribute__((__vector_size__(16)));
    xmm_t fixup_xmm_save[8];
#define XMM_SAVE(N) asm volatile("movdqa %%xmm" #N ", %0" : "=m" (fixup_xmm_save[N]));
#define XMM_RESTORE(N) asm volatile("movdqa %0, %%xmm" #N :: "m" (fixup_xmm_save[N]));
    XMM_SAVE(0);
    XMM_SAVE(1);
    XMM_SAVE(2);
    XMM_SAVE(3);
    XMM_SAVE(4);
    XMM_SAVE(5);
    XMM_SAVE(6);
    XMM_SAVE(7);

#ifdef BITS_32
    ElfRel_t rel = meta->plt_rel[symbol / sizeof(ElfRel_t)];
#else
    ElfRela_t rel = meta->plt_rela[symbol]; // / sizeof(ElfRela_t)];
#endif

    // Verify the symbol is sane.
    if(meta->hash && (R_SYM(rel.info) > meta->hash->nchain)) {
        fprintf(stderr, "symbol lookup failed (symbol not in hash table)\n");
        abort();
    }

    ElfSymbol_t *sym = &meta->dyn_symtab[R_SYM(rel.info)];

    uintptr_t result = doThisRelocation(rel, meta);
    if(result == (uintptr_t) ~0UL) {
        fprintf(stderr, "symbol lookup failed (couldn't relocate)\n");
        abort();
    }

    XMM_RESTORE(0);
    XMM_RESTORE(1);
    XMM_RESTORE(2);
    XMM_RESTORE(3);
    XMM_RESTORE(4);
    XMM_RESTORE(5);
    XMM_RESTORE(6);
    XMM_RESTORE(7);

    return result;
}

#include <dlfcn.h>

void *_libload_dlopen(const char *file, int mode) {
    object_meta_t *result = 0;

    std::set<std::string>::iterator it = g_LoadedObjects.find(std::string(file));
    if(it != g_LoadedObjects.end()) {
        /// \todo implement me :(
        fprintf(stderr, "Not yet able to dlopen already-opened objects.\n");
        return 0;
    }

    bool bLoad = loadSharedObjectHelper(file, g_MainObject, &result);
    if(!bLoad) {
        return 0;
    }

    if(result->init_func) {
        init_fini_func_t init = (init_fini_func_t) result->init_func;
        init();
    }

    return (void *) result;
}

void *_libload_dlsym(void *handle, const char *name) {
    if(!handle)
        return 0;

    object_meta_t *obj = (object_meta_t *) handle;

    ElfSymbol_t sym;
    bool bFound = findSymbol(name, obj, sym);
    if(!bFound) {
        return 0;
    }

    return (void *) sym.value;
}

int _libload_dlclose(void *handle) {
    return 0;
}
