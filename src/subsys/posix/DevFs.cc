#include "DevFs.h"

#include <console/Console.h>
#include <utilities/assert.h>

DevFs DevFs::m_Instance;

uint64_t RandomFile::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    // http://xkcd.com/221/
    /// \todo a real RNG could be nice...
    memset(reinterpret_cast<void *>(buffer), 4, size);
    return size;
}

uint64_t RandomFile::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    return 0;
}

uint64_t NullFile::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    memset(reinterpret_cast<void *>(buffer), 0, size);
    return size;
}

uint64_t NullFile::write(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    return 0;
}

bool DevFs::initialise(Disk *pDisk)
{
    // Deterministic inode assignment to each devfs node
    size_t baseInode = 0;

    m_pRoot = new DevFsDirectory(String(""), 0, 0, 0, ++baseInode, this, 0, 0);

    // Create /dev/null and /dev/zero nodes
    NullFile *pNull = new NullFile(String("null"), ++baseInode, this, m_pRoot);
    NullFile *pZero = new NullFile(String("zero"), ++baseInode, this, m_pRoot);
    m_pRoot->addEntry(pNull->getName(), pNull);
    m_pRoot->addEntry(pZero->getName(), pZero);

    // Create nodes for psuedoterminals.
    const char *ptyletters = "pqrstuvwxyzabcde";
    for(size_t i = 0; i < 16; ++i)
    {
        for(; *ptyletters != 0; ++ptyletters)
        {
            char c = *ptyletters;
            char master[] = {'p', 't', 'y', c, (i > 9) ? ('a' + (i % 10)) : ('0' + i), 0};
            char slave[] = {'t', 't', 'y', c, (i > 9) ? ('a' + (i % 10)) : ('0' + i), 0};

            String masterName(master), slaveName(slave);

            File *pMaster = ConsoleManager::instance().getConsole(masterName);
            File *pSlave = ConsoleManager::instance().getConsole(slaveName);
            assert(pMaster && pSlave);

            m_pRoot->addEntry(masterName, pMaster);
            m_pRoot->addEntry(slaveName, pSlave);
        }
    }

    // Create /dev/urandom for the RNG.
    RandomFile *pRandom = new RandomFile(String("urandom"), ++baseInode, this, m_pRoot);
    m_pRoot->addEntry(pRandom->getName(), pRandom);

    return true;
}
