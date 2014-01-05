#include "DevFs.h"

#include <console/Console.h>
#include <utilities/assert.h>

#include <graphics/Graphics.h>
#include <graphics/GraphicsService.h>

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

FramebufferFile::FramebufferFile(String str, size_t inode, Filesystem *pParentFS, File *pParentNode) :
    File(str, 0, 0, 0, inode, pParentFS, 0, pParentNode), m_pProvider(0)
{
}

FramebufferFile::~FramebufferFile()
{
    delete m_pProvider;
}

bool FramebufferFile::initialise()
{
    ServiceFeatures *pFeatures = ServiceManager::instance().enumerateOperations(String("graphics"));
    Service         *pService  = ServiceManager::instance().getService(String("graphics"));
    if(pFeatures->provides(ServiceFeatures::probe))
    {
        if(pService)
        {
            m_pProvider = new GraphicsService::GraphicsProvider;
            if(!pService->serve(ServiceFeatures::probe, m_pProvider, sizeof(*m_pProvider)))
            {
                delete m_pProvider;
                m_pProvider = 0;

                return false;
            }
            else
            {
                // Set the file size to reflect the size of the framebuffer.
                setSize(m_pProvider->pFramebuffer->getHeight() * m_pProvider->pFramebuffer->getBytesPerLine());
            }
        }
    }

    return true;
}

uintptr_t FramebufferFile::readBlock(uint64_t location)
{
    if(!m_pProvider)
        return 0;

    /// \todo If this is NOT virtual, we need to do something about that.
    return reinterpret_cast<uintptr_t>(m_pProvider->pFramebuffer->getRawBuffer()) + location;
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
        for(const char *ptyc = ptyletters; *ptyc != 0; ++ptyc)
        {
            const char c = *ptyc;
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

    // Create /dev/fb for the framebuffer device.
    FramebufferFile *pFb = new FramebufferFile(String("fb"), ++baseInode, this, m_pRoot);
    if(pFb->initialise())
        m_pRoot->addEntry(pFb->getName(), pFb);
    else
        delete pFb;

    // Create /dev/textui for the text-only UI device.
    TextIO *pTty = new TextIO(String("textui"), ++baseInode, this, m_pRoot);
    if(pTty->initialise())
    {
        m_pRoot->addEntry(pTty->getName(), pTty);
    }
    else
    {
        WARNING("POSIX: no /dev/tty - TextIO failed to initialise.");
        delete pTty;
    }

    return true;
}
