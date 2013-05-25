/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <Log.h>
#include <linker/Elf.h>
#include <utilities/utility.h>
#include <BootstrapInfo.h>
#include <processor/Processor.h>
#include <processor/PhysicalMemoryManager.h>
#include <linker/KernelElf.h>
#include <process/Process.h>

/** Helper function: copies data into a new buffer given a certain number of bytes */
template <typename T>
T *copy(T *buff, size_t numBytes)
{
    T *ret = new T[numBytes / sizeof(T)];
    memcpy(ret, buff, numBytes);
    return ret;
}

/** Helper function: iterates through all program headers given to find where pCurrent resides, then copies that data into a new
    buffer and returns it.*/
template<typename T>
T *elfCopy(uint8_t *pBuffer, Elf::ElfProgramHeader_t *pProgramHeaders, size_t nProgramHeaders, T *pCurrent, size_t size)
{
    for (size_t i = 0; i < nProgramHeaders; i++)
    {
        Elf::ElfProgramHeader_t ph = pProgramHeaders[i];
        if ( (ph.vaddr <= reinterpret_cast<uintptr_t>(pCurrent)) &&
             (reinterpret_cast<uintptr_t>(pCurrent) < ph.vaddr+ph.filesz) )
        {
            uintptr_t loc = reinterpret_cast<uintptr_t>(pCurrent) - ph.vaddr;
            pCurrent = new T[size/sizeof(T)];
            memcpy (reinterpret_cast<uint8_t*>(pCurrent), &pBuffer[loc],
                    size);
            return pCurrent;
        }
    }
    return 0;
}

Elf::Elf() :
    m_pSymbolTable(0),
    m_nSymbolTableSize(0),
    m_pStringTable(0),
    m_nStringTableSize(0),
    m_pShstrtab(0),
    m_nShstrtabSize(0),
    m_pGotTable(0),
    m_pRelTable(0),
    m_pRelaTable(0),
    m_nRelTableSize(0),
    m_nRelaTableSize(0),
    m_pPltRelTable(0),
    m_pPltRelaTable(0),
    m_bUsesRela(false),
    m_pDebugTable(0),
    m_nDebugTableSize(0),
    m_pDynamicSymbolTable(0),
    m_nDynamicSymbolTableSize(0),
    m_pDynamicStringTable(0),
    m_nDynamicStringTableSize(0),
    m_pSectionHeaders(0),
    m_nSectionHeaders(0),
    m_pProgramHeaders(0),
    m_nProgramHeaders(0),
    m_nPltSize(0),
    m_nEntry(0),
    m_NeededLibraries(),
    m_SymbolTable(this),
    m_InitFunc(0),
    m_FiniFunc(0)
{
}

Elf::~Elf()
{
    delete m_pSymbolTable;
    delete m_pStringTable;
    delete m_pShstrtab;
    // delete m_pGotTable;
    delete m_pRelTable;
    delete m_pRelaTable;
    delete m_pPltRelTable;
    delete m_pPltRelaTable;
    delete m_pDebugTable;
    delete m_pDynamicSymbolTable;
    delete m_pDynamicStringTable;
    delete m_pSectionHeaders;
    delete m_pProgramHeaders;
}

Elf::Elf(const Elf &elf) :
    m_pSymbolTable(0),
    m_nSymbolTableSize(elf.m_nSymbolTableSize),
    m_pStringTable(),
    m_nStringTableSize(elf.m_nStringTableSize),
    m_pShstrtab(0),
    m_nShstrtabSize(elf.m_nShstrtabSize),
    m_pGotTable(elf.m_pGotTable),
    m_pRelTable(0),
    m_pRelaTable(0),
    m_nRelTableSize(elf.m_nRelTableSize),
    m_nRelaTableSize(elf.m_nRelaTableSize),
    m_pPltRelTable(0),
    m_pPltRelaTable(0),
    m_bUsesRela(elf.m_bUsesRela),
    m_pDebugTable(0),
    m_nDebugTableSize(elf.m_nDebugTableSize),
    m_pDynamicSymbolTable(0),
    m_nDynamicSymbolTableSize(elf.m_nDynamicSymbolTableSize),
    m_pDynamicStringTable(0),
    m_nDynamicStringTableSize(elf.m_nDynamicStringTableSize),
    m_pSectionHeaders(0),
    m_nSectionHeaders(elf.m_nSectionHeaders),
    m_pProgramHeaders(0),
    m_nProgramHeaders(elf.m_nProgramHeaders),
    m_nPltSize(elf.m_nPltSize),
    m_nEntry(elf.m_nEntry),
    m_NeededLibraries(elf.m_NeededLibraries),
    m_SymbolTable(this),
    m_InitFunc(elf.m_InitFunc),
    m_FiniFunc(elf.m_FiniFunc)
{
    // Copy the symbol table
    m_pSymbolTable = copy(elf.m_pSymbolTable, m_nSymbolTableSize);

    // Copy the string table, somehow?
    m_pStringTable = copy(elf.m_pStringTable, m_nStringTableSize);

    // Copy the section header string table, somehow?
    m_pShstrtab = copy(elf.m_pShstrtab, m_nShstrtabSize);

    // Copy the REL and RELA tables
    m_pRelTable = copy(elf.m_pRelTable, m_nRelTableSize);
    m_pRelaTable = copy(elf.m_pRelaTable, m_nRelaTableSize);

    // Copy the PLT
    if(m_bUsesRela)
        m_pPltRelaTable = copy(elf.m_pPltRelaTable, m_nPltSize);
    else
        m_pPltRelTable = copy(elf.m_pPltRelTable, m_nPltSize);

    // Copy the debug table
    m_pDebugTable = copy(elf.m_pDebugTable, m_nDebugTableSize);

    // Copy the dynamic symbol table
    m_pDynamicSymbolTable = copy(elf.m_pDynamicSymbolTable, m_nDynamicSymbolTableSize);

    // Copy the dynamic string table, somehow?
    m_pDynamicStringTable = copy(elf.m_pDynamicStringTable, m_nDynamicStringTableSize);

    // Copy the section headers
    m_pSectionHeaders = copy(elf.m_pSectionHeaders, m_nSectionHeaders * sizeof(ElfSectionHeader_t));

    // Copy the program headers
    m_pProgramHeaders = copy(elf.m_pProgramHeaders, m_nProgramHeaders * sizeof(ElfProgramHeader_t));

    // Copy needed libraries info, modifying pointers as we go
    intptr_t diff = reinterpret_cast<uintptr_t>(m_pDynamicStringTable) - reinterpret_cast<uintptr_t>(elf.m_pDynamicStringTable);
    for (List<char*>::Iterator it = m_NeededLibraries.begin();
         it != m_NeededLibraries.end();
         it++)
    {
        *it = *it + diff;
    }

    // Copy symbol table
    m_SymbolTable.copyTable(this, elf.m_SymbolTable);
}

bool Elf::createNeededOnly(uint8_t *pBuffer, size_t length)
{
    NOTICE("Elf::createNeededOnly: buffer at " << Hex << reinterpret_cast<uintptr_t>(pBuffer) << ", len " << length);
    if(!pBuffer || !length)
        return false;

    // The main header will be at pBuffer[0].
    ElfHeader_t *pHeader = reinterpret_cast<ElfHeader_t *>(pBuffer);

    // Check the ident.
    if ( (pHeader->ident[1] != 'E') ||
         (pHeader->ident[2] != 'L') ||
         (pHeader->ident[3] != 'F') ||
         (pHeader->ident[0] != 127) )
    {
        ERROR("ELF file: ident check failed [" << String(reinterpret_cast<const char *>(pHeader->ident)) << "]!");
        return false;
    }

    // Check the bit-length.
    if (pHeader->ident[4] !=
#ifdef BITS_32
        1 /* ELFCLASS32 */
#else
        2 /* ELFCLASS64 */
#endif
        )
    {
        ERROR("ELF file: wrong bit length!");
    }

    // Attempt to load in some program headers, if they exist.
    if (pHeader->phnum > 0)
    {
        m_nProgramHeaders = pHeader->phnum;
        m_pProgramHeaders = new ElfProgramHeader_t[pHeader->phnum];
        memcpy(reinterpret_cast<uint8_t*>(m_pProgramHeaders), &pBuffer[pHeader->phoff], sizeof(ElfProgramHeader_t)*pHeader->phnum);

        size_t nDynamicStringTableSize = 0;

        // Look for the dynamic program header.
        for (size_t i = 0; i < m_nProgramHeaders; i++)
        {
            if (m_pProgramHeaders[i].type == PT_DYNAMIC)
            {
                ElfProgramHeader_t *pDynamic = &m_pProgramHeaders[i];
                ElfDyn_t *pDyn = reinterpret_cast<ElfDyn_t*> (&pBuffer[pDynamic->offset]);

                // Cycle through all dynamic entries until the NULL entry.
                while (pDyn->tag != DT_NULL)
                {
                    switch (pDyn->tag)
                    {
                        case DT_NEEDED:
                            m_NeededLibraries.pushBack(reinterpret_cast<char*> (pDyn->un.ptr));
                        case DT_STRTAB:
                            m_pDynamicStringTable = reinterpret_cast<char*> (pDyn->un.ptr);
                            break;
                        case DT_STRSZ:
                            nDynamicStringTableSize = pDyn->un.val;
                            break;
                    }

                    pDyn++;
                }
            }
            else if(m_pProgramHeaders[i].type == PT_INTERP)
            {
                ElfProgramHeader_t *pInterp = &m_pProgramHeaders[i];
                m_sInterpreter = String(reinterpret_cast<char*> (&pBuffer[pInterp->offset]));

                NOTICE("ELF::createNeededOnly interpreter is " << m_sInterpreter);
            }
        }

        if (m_pDynamicStringTable)
        {
            m_pDynamicStringTable = elfCopy(pBuffer, m_pProgramHeaders, m_nProgramHeaders, m_pDynamicStringTable, nDynamicStringTableSize);

            // Make sure the string references to needed libraries actually work as pointers...
            for (List<char*>::Iterator it = m_NeededLibraries.begin();
                 it != m_NeededLibraries.end();
                 it++)
            {
                *it = *it + reinterpret_cast<uintptr_t>(m_pDynamicStringTable);
            }
        }
    }

    // Success.
    return true;
}

bool Elf::create(uint8_t *pBuffer, size_t length)
{
    NOTICE("Elf::create: buffer at " << Hex << reinterpret_cast<uintptr_t>(pBuffer) << ", len " << length);
    // The main header will be at pBuffer[0].
    ElfHeader_t *pHeader = reinterpret_cast<ElfHeader_t *>(pBuffer);

    // Check the ident.
    if ( (pHeader->ident[1] != 'E') ||
         (pHeader->ident[2] != 'L') ||
         (pHeader->ident[3] != 'F') ||
         (pHeader->ident[0] != 127) )
    {
        ERROR("ELF file: ident check failed [" << String(reinterpret_cast<const char *>(pHeader->ident)) << "]!");
        return false;
    }

    // Check the bit-length.
    if (pHeader->ident[4] !=
#ifdef BITS_32
        1 /* ELFCLASS32 */
#else
        2 /* ELFCLASS64 */
#endif
        )
    {
        ERROR("ELF file: wrong bit length!");
    }

    // Load in the section headers.
    m_nSectionHeaders = pHeader->shnum;
    m_pSectionHeaders = new ElfSectionHeader_t[pHeader->shnum];
    memcpy (reinterpret_cast<uint8_t*>(m_pSectionHeaders), &pBuffer[pHeader->shoff], pHeader->shnum*sizeof(ElfSectionHeader_t));

    // Find the section header string table.
    ElfSectionHeader_t *pShstrtab = &m_pSectionHeaders[pHeader->shstrndx];

    // Load the section header string table.
    m_nShstrtabSize = pShstrtab->size;
    m_pShstrtab = new char[m_nShstrtabSize];
    memcpy (reinterpret_cast<uint8_t*>(m_pShstrtab), &pBuffer[pShstrtab->offset], m_nShstrtabSize);

    ElfSectionHeader_t *pSymbolTable=0, *pStringTable=0;
    // Go through each section header, trying to find .symtab.
    for (int i = 0; i < pHeader->shnum; i++)
    {
        const char *pStr = m_pShstrtab + m_pSectionHeaders[i].name;
        if (!strcmp(pStr, ".symtab"))
            pSymbolTable = &m_pSectionHeaders[i];
        if (!strcmp(pStr, ".strtab"))
            pStringTable = &m_pSectionHeaders[i];
    }

    if (pSymbolTable == 0)
    {
        WARNING("ELF: symbol table not found!");
    }
    else
    {
        m_nSymbolTableSize = pSymbolTable->size;
        m_pSymbolTable = new ElfSymbol_t[m_nSymbolTableSize/sizeof(ElfSymbol_t)];
        memcpy (reinterpret_cast<uint8_t*>(m_pSymbolTable), &pBuffer[pSymbolTable->offset], pSymbolTable->size);
    }

    if (pStringTable == 0)
    {
        WARNING("ELF: string table not found!");
    }
    else
    {
        m_nStringTableSize = pStringTable->size;
        m_pStringTable = new char[m_nStringTableSize];
        memcpy (reinterpret_cast<uint8_t*>(m_pStringTable), &pBuffer[pStringTable->offset], m_nStringTableSize);
    }

    // Attempt to load in some program headers, if they exist.
    if (pHeader->phnum > 0)
    {
        m_nProgramHeaders = pHeader->phnum;
        m_pProgramHeaders = new ElfProgramHeader_t[pHeader->phnum];
        memcpy(reinterpret_cast<uint8_t*>(m_pProgramHeaders), &pBuffer[pHeader->phoff], sizeof(ElfProgramHeader_t)*pHeader->phnum);

        //size_t nDynamicStringTableSize = 0;

        // Look for the dynamic program header.
        for (size_t i = 0; i < m_nProgramHeaders; i++)
        {
            if (m_pProgramHeaders[i].type == PT_DYNAMIC)
            {
                ElfProgramHeader_t *pDynamic = &m_pProgramHeaders[i];
                ElfDyn_t *pDyn = reinterpret_cast<ElfDyn_t*> (&pBuffer[pDynamic->offset]);

                // Cycle through all dynamic entries until the NULL entry.
                while (pDyn->tag != DT_NULL)
                {
                    switch (pDyn->tag)
                    {
                        case DT_NEEDED:
                            m_NeededLibraries.pushBack(reinterpret_cast<char*> (pDyn->un.ptr));
                        case DT_SYMTAB:
                            m_pDynamicSymbolTable = reinterpret_cast<ElfSymbol_t*> (pDyn->un.ptr);
                            break;
                        case DT_STRTAB:
                            m_pDynamicStringTable = reinterpret_cast<char*> (pDyn->un.ptr);
                            break;
                        case DT_SYMENT:
                            // This gives the size of *each entity*, not the table as a whole.
                            //m_nDynamicSymbolTableSize = pDyn->un.val;
                            break;
                        case DT_STRSZ:
                            m_nDynamicStringTableSize = pDyn->un.val;
                            break;
                        case DT_RELA:
                            m_pRelaTable = reinterpret_cast<ElfRela_t*> (pDyn->un.ptr);
                            break;
                        case DT_REL:
                            m_pRelTable = reinterpret_cast<ElfRel_t*> (pDyn->un.ptr);
                            break;
                        case DT_RELSZ:
                            m_nRelTableSize = pDyn->un.val;
                            break;
                        case DT_RELASZ:
                            m_nRelaTableSize = pDyn->un.val;
                            break;
                        case DT_PLTGOT:
                            m_pGotTable = reinterpret_cast<uintptr_t*> (pDyn->un.ptr);
                            break;
                        case DT_JMPREL:
                        {
                            if (m_bUsesRela)
                                m_pPltRelaTable = reinterpret_cast<ElfRela_t*> (pDyn->un.ptr);
                            else
                                m_pPltRelTable = reinterpret_cast<ElfRel_t*> (pDyn->un.ptr);
                            break;
                        }
                        case DT_PLTREL:
                        {
                            if (pDyn->un.val == DT_RELA)
                            {
                                m_bUsesRela = true;
                            }
                            break;
                        }
                        case DT_PLTRELSZ:
                            m_nPltSize = pDyn->un.val;
                            break;
                        case DT_INIT:
                            m_InitFunc = pDyn->un.val;
                            break;
                        case DT_FINI:
                            m_FiniFunc = pDyn->un.val;
                            break;
                    }

                    pDyn++;
                }
            }
            else if(m_pProgramHeaders[i].type == PT_INTERP)
            {
                ElfProgramHeader_t *pInterp = &m_pProgramHeaders[i];
                m_sInterpreter = String(reinterpret_cast<char*> (&pBuffer[pInterp->offset]));

                NOTICE("ELF::create interpreter is " << m_sInterpreter);
            }
        }

        m_nDynamicSymbolTableSize = reinterpret_cast<uintptr_t>(m_pDynamicStringTable) -
            reinterpret_cast<uintptr_t>(m_pDynamicSymbolTable);

        // If we found a dynamic symbol table, string table and Rel(a) table, attempt to find the segment they reside in
        // and copy them locally.
        if (m_pDynamicSymbolTable)
            m_pDynamicSymbolTable = elfCopy(pBuffer, m_pProgramHeaders, m_nProgramHeaders, m_pDynamicSymbolTable, m_nDynamicSymbolTableSize);
        if (m_pDynamicStringTable)
        {
            m_pDynamicStringTable = elfCopy(pBuffer, m_pProgramHeaders, m_nProgramHeaders, m_pDynamicStringTable, m_nDynamicStringTableSize);

            // Make sure the string references to needed libraries actually work as pointers...
            for (List<char*>::Iterator it = m_NeededLibraries.begin();
                 it != m_NeededLibraries.end();
                 it++)
            {
                *it = *it + reinterpret_cast<uintptr_t>(m_pDynamicStringTable);
            }
        }
        if (m_pRelTable)
            m_pRelTable = elfCopy(pBuffer, m_pProgramHeaders, m_nProgramHeaders, m_pRelTable, m_nRelTableSize);
        if (m_pRelaTable)
            m_pRelaTable = elfCopy(pBuffer, m_pProgramHeaders, m_nProgramHeaders, m_pRelaTable, m_nRelaTableSize);
        if (m_pPltRelTable)
            m_pPltRelTable = elfCopy(pBuffer, m_pProgramHeaders, m_nProgramHeaders, m_pPltRelTable, m_nPltSize);
        if (m_pPltRelaTable)
            m_pPltRelaTable = elfCopy(pBuffer, m_pProgramHeaders, m_nProgramHeaders, m_pPltRelaTable, m_nPltSize);
    }

    m_nEntry = pHeader->entry;

    // Success.
    return true;
}

bool Elf::loadModule(uint8_t *pBuffer, size_t length, uintptr_t &loadBase, size_t &loadSize, SymbolTable *pSymbolTableCopy)
{
    // Run through the sections to calculate the size required.
    loadSize = 0;
    for (size_t i = 0; i < m_nSectionHeaders; i++)
    {
        if (m_pSectionHeaders[i].flags & SHF_ALLOC)
        {
            loadSize += m_pSectionHeaders[i].addr; // If .addr is set, add it as an offset.
            // Ensure the alignment is as required.
            while ( (loadSize % m_pSectionHeaders[i].addralign) != 0) loadSize ++;
            loadSize += m_pSectionHeaders[i].size;
        }
    }
    if(loadSize & 0xFFF)    // Make sure two modules don't accidentally end up in the same page.
        loadSize += 0x1000; // ie, end page == start page of another module.
    loadSize = (loadSize+0x1000)&0xFFFFF000;
    if (!KernelElf::instance().getModuleAllocator().allocate(loadSize, loadBase))
    {
        return false;
    }

    // Now actually map and populate the sections.
    uintptr_t offset = loadBase;
    for (size_t i = 0; i < m_nSectionHeaders; i++)
    {
        if (m_pSectionHeaders[i].flags & SHF_ALLOC)
        {
            // Add load-base into the equation.
            if (m_pSectionHeaders[i].addr == 0)
                m_pSectionHeaders[i].addr = offset;
            else
            {
                int tmp = m_pSectionHeaders[i].addr;
                m_pSectionHeaders[i].addr += offset;
                /// \todo Total bollocks?
                offset += tmp; // The .addr won't be accounted for in the .size, so add it here so we don't
                // end up overwriting what we just wrote!
            }

            // Ensure the alignment is as required.
            while ( (m_pSectionHeaders[i].addr % m_pSectionHeaders[i].addralign) != 0)
            {
                m_pSectionHeaders[i].addr ++;
                offset ++;
            }

            // We now know where to place this section, so map some memory for it.
            for (uintptr_t j = m_pSectionHeaders[i].addr;
                 j < (m_pSectionHeaders[i].addr + m_pSectionHeaders[i].size) + 0x1000; /// \todo This isn't the correct formula - fix.
                 j += 0x1000)
            {
                bool bCanWrite = m_pSectionHeaders[i].flags & SHF_WRITE;
                bool bCanExecute = m_pSectionHeaders[i].flags & SHF_EXECINSTR;
                
                void *virt = reinterpret_cast<void*> (j&~(PhysicalMemoryManager::getPageSize()-1));
                if(!Processor::information().getVirtualAddressSpace().isMapped(virt))
                {
                    physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
                    Processor::information().getVirtualAddressSpace().map(phys,
                                                                          virt,
                                                                          (bCanWrite ? VirtualAddressSpace::Write : 0) | VirtualAddressSpace::KernelMode | (bCanExecute ? VirtualAddressSpace::Execute : 0));
                }
                else
                {
                    // Already mapped: check to see if we need to make the area writable/executable now
                    size_t flags = 0; physical_uintptr_t phys = 0;
                    Processor::information().getVirtualAddressSpace().getMapping(virt, phys, flags);
                    if((flags & VirtualAddressSpace::Write) == 0)
                    {
                        if(bCanWrite)
                        {
                            flags |= VirtualAddressSpace::Write;
                        }
                    }
                    
                    if((flags & VirtualAddressSpace::Execute) == 0)
                    {
                        if(bCanExecute)
                        {
                            flags |= VirtualAddressSpace::Execute;
                        }
                    }
                    
                    Processor::information().getVirtualAddressSpace().setFlags(virt, flags);
                }
            }

            if (m_pSectionHeaders[i].type != SHT_NOBITS)
            {
                // Copy section data from the file.
                memcpy (reinterpret_cast<uint8_t*> (m_pSectionHeaders[i].addr),
                        &pBuffer[m_pSectionHeaders[i].offset],
                        m_pSectionHeaders[i].size);
            }
            else
            {
                memset (reinterpret_cast<uint8_t*> (m_pSectionHeaders[i].addr),
                        0,
                        m_pSectionHeaders[i].size);
            }
#if defined(PPC_COMMON) || defined(MIPS_COMMON)
            Processor::flushDCacheAndInvalidateICache(m_pSectionHeaders[i].addr, m_pSectionHeaders[i].addr+m_pSectionHeaders[i].size);
#endif
            offset += m_pSectionHeaders[i].size;
        }
        else
        {
            //  Load information from non-allocated sections here
            const char* pStr = m_pShstrtab + m_pSectionHeaders[i].name;
            if (!strcmp(pStr, ".debug_frame"))
            {
                m_pDebugTable = reinterpret_cast<uint8_t*> (m_pSectionHeaders[i].addr);
                m_nDebugTableSize = m_pSectionHeaders[i].size;

                /*  Go through the debug table relocating debug frame information
                    based on the loadBase.                                        */
                size_t nIndex = 0;
                while (nIndex < m_nDebugTableSize)
                {
                    // Get the length of this entry.
                    uint32_t nLength = * reinterpret_cast<uint32_t*> (m_pDebugTable+nIndex);

                    nIndex += sizeof(uint32_t);

                    const uint32_t k_nCieId = 0xFFFFFFFF;

                    if (nLength == 0xFFFFFFFF)
                    {
                        ERROR("64-bit DWARF file detected, but not supported!");
                        return false;
                    }

                    // Get the type of this entry (or CIE pointer if this is a FDE).
                    uint32_t nCie = * reinterpret_cast<uint32_t*> (m_pDebugTable+nIndex);
                    nIndex += sizeof(uint32_t);

                    // Is this a CIE?
                    if (nCie == k_nCieId)
                    {
                        // Skip over everything.
                        nIndex += nLength - sizeof(processor_register_t);
                        continue;
                    }

                    // This is a FDE. Get its initial location.
                    uintptr_t *nInitialLocation = reinterpret_cast<uintptr_t*> (m_pDebugTable+nIndex);
                    *nInitialLocation+= loadBase;

                    nIndex += nLength - sizeof(processor_register_t);
                }
            }
        }
    }

    // Firstly, we need to change the symbol table so that the ::value member is actually valid.
    // Currently, it's the offset into the symbol's section - we add the section base address
    // on to that to make it a valid pointer.
    ElfSymbol_t *pSymbol = m_pSymbolTable;
    for (size_t i = 0; i < m_nSymbolTableSize / sizeof(ElfSymbol_t); i++)
    {
        // Only relocate functions, variables and notypes.
        if (ST_TYPE(pSymbol->info) < 3 && pSymbol->shndx < m_nSectionHeaders)
        {
            ElfSectionHeader_t *pSh = &m_pSectionHeaders[pSymbol->shndx];
            pSymbol->value += pSh->addr;
        }
        pSymbol ++;
    }

    if (m_pSymbolTable && m_pStringTable)
    {
        ElfSymbol_t *pSymbol = reinterpret_cast<ElfSymbol_t *>(m_pSymbolTable);

        const char *pStrtab = reinterpret_cast<const char *>(m_pStringTable);

        for (size_t i = 0; i < m_nSymbolTableSize / sizeof(ElfSymbol_t); i++)
        {
            const char *pStr;

            if (ST_TYPE(pSymbol->info) == 3)
            {
                // Section type - the name will be the name of the section header it refers to.
                ElfSectionHeader_t *pSh = &m_pSectionHeaders[pSymbol->shndx];
                // If it's not allocated, it's a link-once-only section that we can ignore.
                if (!(pSh->flags & SHF_ALLOC))
                {
                    pSymbol++;
                    continue;
                }
                // Grab the shstrtab
                pStr = reinterpret_cast<const char*> (m_pShstrtab) + pSh->name;
            }
            else
                pStr = pStrtab + pSymbol->name;

            // Insert the symbol into the symbol table.
            SymbolTable::Binding binding;
            switch (ST_BIND(pSymbol->info))
            {
                case 0: // STB_LOCAL
                    binding = SymbolTable::Local;
                    break;
                case 1: // STB_GLOBAL
                    binding = SymbolTable::Global;
                    break;
                case 2: // STB_WEAK
                    binding = SymbolTable::Weak;
                    break;
                default:
                    binding = SymbolTable::Global;
            }

            // If the shndx == UND (0x0), the symbol is in the table but undefined!
            if (*pStr != '\0' && pSymbol->shndx != 0)
            {
                m_SymbolTable.insert(String(pStr), binding, this, pSymbol->value);
                if (pSymbolTableCopy)
                    pSymbolTableCopy->insert(String(pStr), binding, this, pSymbol->value);
            }
            pSymbol++;
        }
    }

    relocateModinfo(pBuffer, length);

    return true;
}

bool Elf::finaliseModule(uint8_t *pBuffer, size_t length)
{
    return relocate(pBuffer, length);
}

bool Elf::allocate(uint8_t *pBuffer, size_t length, uintptr_t &loadBase, SymbolTable *pSymtab, bool bAllocate, size_t *pSize)
{
#ifdef THREADS
    NOTICE("Elf::allocate: buffer at " << Hex << reinterpret_cast<uintptr_t>(pBuffer) << ", len " << length);

    Process *pProcess = Processor::information().getCurrentThread()->getParent();

    // Scan the segments to find the size.
    uintptr_t size = 0;
    uintptr_t start = ~0;
    for (size_t i = 0; i < m_nProgramHeaders; i++)
    {
        if (m_pProgramHeaders[i].type == PT_LOAD)
        {
            size = m_pProgramHeaders[i].vaddr + m_pProgramHeaders[i].memsz;
            if (m_pProgramHeaders[i].vaddr < start) start = m_pProgramHeaders[i].vaddr;
        }
    }
    // Currently size is actually the last loaded address - subtract the first loaded address to make it valid.
    size -= start;

    if (pSize)
        *pSize = (size+0x1000)&0xFFFFF000;

    // Here we use an atrocious heuristic for determining if the Elf needs relocating - if its entry point is < 1MB, it
    // is likely that it needs relocation.
    if (m_nEntry < 0x100000)
    {
        if(!pProcess->getSpaceAllocator().allocate((size+0x1000)&0xFFFFF000, loadBase))
            return false;
    }
    else
    {
        loadBase = start;

        // Make sure the Process knows that we've just plonked an Elf at a specific place, and doesn't try to allocate
        // mmaps or libraries over it!
        if (!pProcess->getSpaceAllocator().allocateSpecific(start, (size+0x1000)&0xFFFFF000))
            return false;
    }

    if (bAllocate)
    {
        uintptr_t loadAddr = (loadBase==0) ? start : loadBase;
        for (unsigned int j = loadAddr; j < loadAddr+size+0x1000; j += 0x1000)
        {
            physical_uintptr_t phys = PhysicalMemoryManager::instance().allocatePage();
            bool b = Processor::information().getVirtualAddressSpace().map(phys,
                                                                           reinterpret_cast<void*> (j&~(PhysicalMemoryManager::getPageSize()-1)),
                                                                           VirtualAddressSpace::Write | VirtualAddressSpace::Execute);
            if (!b)
                WARNING("map() failed for address " << Hex << j);
        }
    }

    if (m_pDynamicSymbolTable && m_pDynamicStringTable)
    {
        ElfSymbol_t *pSymbol = m_pDynamicSymbolTable;

        const char *pStrtab = m_pDynamicStringTable;

        /// \todo Don't rely on this. Look at nchain in the hash table.
        while (reinterpret_cast<uintptr_t>(pSymbol) < reinterpret_cast<uintptr_t>(m_pDynamicSymbolTable) +
               m_nDynamicSymbolTableSize)
        {
            const char *pStr = pStrtab + pSymbol->name;

            SymbolTable::Binding binding;
            switch (ST_BIND(pSymbol->info))
            {
                case 0: // STB_LOCAL
                    binding = SymbolTable::Local;
                    break;
                case 1: // STB_GLOBAL
                    binding = SymbolTable::Global;
                    break;
                case 2: // STB_WEAK
                    binding = SymbolTable::Weak;
                    break;
                default:
                    binding = SymbolTable::Global;
            }

            // If the shndx == UND (0x0), the symbol is in the table but undefined!
            if (pSymbol->shndx != 0)
            {

                if (*pStr != 0)
                {
                    m_SymbolTable.insert(String(pStr), binding, this, pSymbol->value);
                    if (pSymtab)
                        // Add loadBase in when adding to the user-defined symtab, to give the user a "real" value.
                        pSymtab->insert(String(pStr), binding, this, pSymbol->value + loadBase);
                }
            }
            else
            {
                // weak symbol? set it as undefined
                if(binding == SymbolTable::Weak)
                {
                    //if(ST_TYPE(pSymbol->info) == STT_FUNC)
                    //{
                    Elf_Xword value = pSymbol->value;
                    if(value == 0)
                        value = ~0;
                    if (*pStr != 0)
                    {
                        m_SymbolTable.insert(String(pStr), binding, this, value);
                        if (pSymtab)
                            // Add loadBase in when adding to the user-defined symtab, to give the user a "real" value.
                            pSymtab->insert(String(pStr), binding, this, value);
                    }
                    //}
                }
            }
            pSymbol ++;
        }
    }

    return true;
#else
    ERROR("Elf::allocate: no thread or process support, cannot allocate memory");
    return false;
#endif
}

bool Elf::load(uint8_t *pBuffer, size_t length, uintptr_t loadBase, SymbolTable *pSymtab, uintptr_t nStart, uintptr_t nEnd)
{
    for (size_t i = 0; i < m_nProgramHeaders; i++)
    {
        if (m_pProgramHeaders[i].type == PT_LOAD)
        {
            uintptr_t loadAddr = m_pProgramHeaders[i].vaddr + loadBase;

            if (nStart > (loadAddr+m_pProgramHeaders[i].memsz)) continue;
            if (nEnd <= loadAddr) continue;
            uintptr_t sectionStart = (loadAddr >= nStart) ? loadAddr : nStart;

            uintptr_t offset = m_pProgramHeaders[i].offset + (sectionStart-loadAddr);
            uintptr_t filesz = (loadAddr+m_pProgramHeaders[i].filesz >= nEnd)
                ? (nEnd-sectionStart)
                : (loadAddr+m_pProgramHeaders[i].filesz-sectionStart);
            if (loadAddr+m_pProgramHeaders[i].filesz < nStart) filesz = 0;
            uintptr_t memsz = (loadAddr+m_pProgramHeaders[i].memsz >= nEnd)
                ? (nEnd-sectionStart)
                : (loadAddr+m_pProgramHeaders[i].memsz-sectionStart);

            // Copy segment data from the file.
            memcpy (reinterpret_cast<uint8_t*> (sectionStart),
                    &pBuffer[offset],
                    filesz);

            memset (reinterpret_cast<uint8_t*> (sectionStart+filesz),
                    0,
                    memsz-filesz);

#if defined(PPC_COMMON) || defined(MIPS_COMMON)
            Processor::flushDCacheAndInvalidateICache(loadAddr, loadAddr+m_pProgramHeaders[i].filesz);
#endif
        }
    }

    // Apply relocations for the given area.

    // Is it a relocation section?
    if (m_pRelTable)
    {
        // For each relocation entry...
        for (ElfRel_t *pRel = m_pRelTable;
             pRel < (m_pRelTable+(m_nRelTableSize/sizeof(ElfRel_t)));
             pRel++)
        {
            if ( (pRel->offset + loadBase < nStart) || (pRel->offset + loadBase >= nEnd) )
                continue;
            if (!applyRelocation(*pRel, 0, pSymtab, loadBase))
                return false;
        }
    }
    // How about a relocation with addend?
    if (m_pRelaTable)
    {
        // For each relocation entry...
        for (ElfRela_t *pRel = m_pRelaTable;
             pRel < (m_pRelaTable+(m_nRelaTableSize/sizeof(ElfRela_t)));
             pRel++)
        {
            if ( (pRel->offset + loadBase < nStart) || (pRel->offset + loadBase >= nEnd) )
                continue;
            if (!applyRelocation(*pRel, 0, pSymtab, loadBase))
                return false;
        }
    }

    // We must also adjust the values in the GOTPLT, they currently point at relative values, we need them to point at absolute ones.
    if (m_pPltRelTable)
    {
        // For each relocation entry...
        ElfRel_t *pRel = m_pPltRelTable;
        for (size_t i = 0; i < m_nPltSize/sizeof(ElfRel_t); i++, pRel++)
        {
            if ( (pRel->offset + loadBase < nStart) || (pRel->offset + loadBase >= nEnd) )
                continue;
            uintptr_t *address = reinterpret_cast<uintptr_t*> (loadBase + pRel->offset);
            *address += loadBase;
        }
    }
    if (m_pPltRelaTable)
    {
        // For each relocation entry...
        ElfRela_t *pRel = m_pPltRelaTable;
        for (size_t i = 0; i < m_nPltSize/sizeof(ElfRela_t); i++, pRel++)
        {
            if ( (pRel->offset + loadBase < nStart) || (pRel->offset + loadBase >= nEnd) )
                continue;
            uintptr_t *address = reinterpret_cast<uintptr_t*> (loadBase + pRel->offset);
            *address += loadBase;
        }
    }

    // Success.
    return true;
}

uintptr_t Elf::getLastAddress()
{
    // TODO
    return 0;
}

const char *Elf::lookupSymbol(uintptr_t addr, uintptr_t *startAddr)
{
    if (!m_pSymbolTable || !m_pStringTable)
        return 0; // Just return null if we haven't got a symbol table.

    ElfSymbol_t *pSymbol = reinterpret_cast<ElfSymbol_t *>(m_pSymbolTable);

    const char *pStrtab = reinterpret_cast<const char *>(m_pStringTable);

    for (size_t i = 0; i < m_nSymbolTableSize / sizeof(ElfSymbol_t); i++)
    {
        // Make sure we're looking at an object or function.
        if (ST_TYPE(pSymbol->info) != 0x2 /* function */ &&
            ST_TYPE(pSymbol->info) != 0x0 /* notype (asm functions) */)
        {
            pSymbol++;
            continue;
        }

        // If we're checking for a symbol that is apparently zero-sized, add one so we can actually
        // count it!
        Elf_Xword size = pSymbol->size;
        if (size == 0)
            size = 0x100;
        if ( (addr >= pSymbol->value) &&
             (addr < (pSymbol->value + size)) )
        {
            const char *pStr = pStrtab + pSymbol->name;
            if (startAddr)
                *startAddr = pSymbol->value;
            return pStr;
        }
        pSymbol ++;
    }
    return 0;

}

uintptr_t Elf::lookupSymbol(const char *pName)
{
    return m_SymbolTable.lookup(String(pName), this);
}

uintptr_t Elf::lookupDynamicSymbolAddress(const char *sym, uintptr_t loadBase)
{
    uintptr_t value = m_SymbolTable.lookup(String(sym), this);
    if (!value)
        return 0;
    else
        return value + loadBase;
}

uintptr_t Elf::getGlobalOffsetTable()
{
    return reinterpret_cast<uintptr_t> (m_pGotTable);
}

uintptr_t Elf::getEntryPoint()
{
    return m_nEntry;
}

bool Elf::relocate(uint8_t *pBuffer, uintptr_t length)
{
    // For every section...
    for (size_t i = 0; i < m_nSectionHeaders; i++)
    {
        ElfSectionHeader_t *pSh = &m_pSectionHeaders[i];

        if (pSh->type != SHT_REL && pSh->type != SHT_RELA)
            continue;

        // Grab the section header that this relocation section refers to.
        ElfSectionHeader_t *pLink = &m_pSectionHeaders[pSh->info];

        // Grab the shstrtab
        const char *pStr = reinterpret_cast<const char*> (m_pShstrtab) + pLink->name;
        if (!strcmp(pStr, ".modinfo"))
            continue;

        // Is it a relocation section?
        if (pSh->type == SHT_REL)
        {
            // For each relocation entry...
            for (ElfRel_t *pRel = reinterpret_cast<ElfRel_t*> (&pBuffer[pSh->offset]);
                 pRel < reinterpret_cast<ElfRel_t*> (&pBuffer[pSh->offset+pSh->size]);
                 pRel++)
            {
                if (!applyRelocation(*pRel, pLink))
                    return false;
            }
        }
        // How about a relocation with addend?
        else if (pSh->type == SHT_RELA)
        {
            // For each relocation entry...
            for (ElfRela_t *pRel = reinterpret_cast<ElfRela_t*> (&pBuffer[pSh->offset]);
                 pRel < reinterpret_cast<ElfRela_t*> (&pBuffer[pSh->offset+pSh->size]);
                 pRel++)
            {
                if (!applyRelocation(*pRel, pLink))
                    return false;
            }
        }
    }

    // Success!
    return true;
}

bool Elf::relocateModinfo(uint8_t *pBuffer, uintptr_t length)
{
    // For every section...
    for (size_t i = 0; i < m_nSectionHeaders; i++)
    {
        ElfSectionHeader_t *pSh = &m_pSectionHeaders[i];
        if (pSh->type != SHT_REL && pSh->type != SHT_RELA)
            continue;
        // Grab the section header that this relocation section refers to.
        ElfSectionHeader_t *pLink = &m_pSectionHeaders[pSh->info];

        // Grab the shstrtab
        const char *pStr = reinterpret_cast<const char*> (m_pShstrtab) + pLink->name;
        if (strcmp(pStr, ".modinfo"))
            continue;

        // Is it a relocation section?
        if (pSh->type == SHT_REL)
        {
            // For each relocation entry...
            for (ElfRel_t *pRel = reinterpret_cast<ElfRel_t*> (&pBuffer[pSh->offset]);
                 pRel < reinterpret_cast<ElfRel_t*> (&pBuffer[pSh->offset+pSh->size]);
                 pRel++)
            {
                if (!applyRelocation(*pRel, pLink))
                    return false;
            }
        }
        // How about a relocation with addend?
        else if (pSh->type == SHT_RELA)
        {
            // For each relocation entry...
            for (ElfRela_t *pRel = reinterpret_cast<ElfRela_t*> (&pBuffer[pSh->offset]);
                 pRel < reinterpret_cast<ElfRela_t*> (&pBuffer[pSh->offset+pSh->size]);
                 pRel++)
            {
                if (!applyRelocation(*pRel, pLink))
                    return false;
            }
        }
    }

    // Success!
    return true;
}

uintptr_t Elf::applySpecificRelocation(uintptr_t off, SymbolTable *pSymtab, uintptr_t loadBase, SymbolTable::Policy policy)
{
    // Is it a relocation section?
    if (m_pPltRelTable)
    {
        // For each relocation entry...
        ElfRel_t *pRel = adjust_pointer(m_pPltRelTable, off);

        applyRelocation(*pRel, 0, pSymtab, loadBase, policy);

        uintptr_t address = loadBase + pRel->offset;

        return * reinterpret_cast<uintptr_t*> (address);
    }
    // How about a relocation with addend?
    if (m_pPltRelaTable)
    {
        // For each relocation entry...
        ElfRela_t pRel = m_pPltRelaTable[off];
        applyRelocation(pRel, 0, pSymtab, loadBase, policy);

        uintptr_t address = loadBase + pRel.offset;

        return * reinterpret_cast<uintptr_t*> (address);
    }
    return 0;
}

uintptr_t Elf::debugFrameTable()
{
    return reinterpret_cast<uintptr_t>(m_pDebugTable);
}

uintptr_t Elf::debugFrameTableLength()
{
    return m_nDebugTableSize;
}

List<char*> &Elf::neededLibraries()
{
    return m_NeededLibraries;
}

String &Elf::getInterpreter() {
    return m_sInterpreter;
}

size_t Elf::getPltSize ()
{
    return m_nPltSize;
}

void Elf::populateSymbolTable(SymbolTable *pSymtab, uintptr_t loadBase)
{
    if (m_pDynamicSymbolTable && m_pDynamicStringTable)
    {
        ElfSymbol_t *pSymbol = m_pDynamicSymbolTable;

        const char *pStrtab = m_pDynamicStringTable;

        /// \todo Don't rely on this. Look at nchain in the hash table.
        while (reinterpret_cast<uintptr_t>(pSymbol) < reinterpret_cast<uintptr_t>(m_pDynamicSymbolTable) +
               m_nDynamicSymbolTableSize)
        {
            const char *pStr = pStrtab + pSymbol->name;

            // If the shndx == UND (0x0), the symbol is in the table but undefined!
            if (pSymbol->shndx != 0)
            {
                SymbolTable::Binding binding;
                switch (ST_BIND(pSymbol->info))
                {
                    case 0: // STB_LOCAL
                        binding = SymbolTable::Local;
                        break;
                    case 1: // STB_GLOBAL
                        binding = SymbolTable::Global;
                        break;
                    case 2: // STB_WEAK
                        binding = SymbolTable::Weak;
                        break;
                    default:
                        binding = SymbolTable::Global;
                }

                if (*pStr != 0)
                {
                    if (pSymtab)
                        pSymtab->insert(String(pStr), binding, this, pSymbol->value + loadBase);
                }
            }
            pSymbol ++;
        }
    }
}
