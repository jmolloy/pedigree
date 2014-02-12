#include "DevFs.h"

#define MACHINE_FORWARD_DECL_ONLY
#include <machine/Machine.h>
#include <machine/Vga.h>

#include <console/Console.h>
#include <utilities/assert.h>

#include <graphics/Graphics.h>
#include <graphics/GraphicsService.h>

#include <utilities/utility.h>

#include <sys/fb.h>

DevFs DevFs::m_Instance;

uint64_t RandomFile::read(uint64_t location, uint64_t size, uintptr_t buffer, bool bCanBlock)
{
    /// \todo Endianness issues?

    size_t realSize = size;

    if(size < sizeof(uint64_t))
    {
        uint64_t val = rand();
        char *pBuffer = reinterpret_cast<char *>(buffer);
        while(size--)
        {
            *pBuffer++ = val & 0xFF;
            val >>= 8;
        }
    }
    else
    {
        // Align.
        char *pBuffer = reinterpret_cast<char *>(buffer);
        if(size % 8)
        {
            uint64_t align = rand();
            while(size % 8)
            {
                *pBuffer++ = align & 0xFF;
                --size;
                align >>= 8;
            }
        }

        uint64_t *pBuffer64 = reinterpret_cast<uint64_t *>(buffer);
        while(size)
        {
            *pBuffer64++ = rand();
            size -= 8;
        }
    }

    return realSize - size;
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
    File(str, 0, 0, 0, inode, pParentFS, 0, pParentNode), m_pProvider(0), m_bTextMode(false)
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

bool FramebufferFile::supports(const int command)
{
    return (PEDIGREE_FB_CMD_MIN <= command) && (command <= PEDIGREE_FB_CMD_MAX);
}

int FramebufferFile::command(const int command, void *buffer)
{
    NOTICE("FramebufferFile::command(" << command << ", " << reinterpret_cast<uintptr_t>(buffer) << ")");
    if(!m_pProvider)
    {
        ERROR("FramebufferFile::command called on an invalid FramebufferFile");
        return -1;
    }

    Display *pDisplay = m_pProvider->pDisplay;
    Framebuffer *pFramebuffer = m_pProvider->pFramebuffer;

    switch(command)
    {
        case PEDIGREE_FB_SETMODE:
            {
                pedigree_fb_modeset *arg = reinterpret_cast<pedigree_fb_modeset *>(buffer);
                size_t desiredWidth = arg->width;
                size_t desiredHeight = arg->height;
                size_t desiredDepth = arg->depth;

                // Are we seeking a text mode?
                if(!(desiredWidth && desiredHeight && desiredDepth))
                {
                    bool bSuccess = false;
                    if(!m_pProvider->bTextModes)
                    {
                        bSuccess = pDisplay->setScreenMode(0);
                    }
                    else
                    {
                        // Set via VGA method.
                        if(Machine::instance().getNumVga())
                        {
                            /// \todo What if there is no text mode!?
                            Vga *pVga = Machine::instance().getVga(0);
                            pVga->rememberMode();
                            pVga->setLargestTextMode();

                            m_bTextMode = true;

                            bSuccess = true;
                        }
                    }

                    if(bSuccess)
                    {
                        NOTICE("FramebufferFile: set text mode");
                        return 0;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else if(m_pProvider->bTextModes && m_bTextMode)
                {
                    // Okay, we need to 'undo' the text mode.
                    if(Machine::instance().getNumVga())
                    {
                        /// \todo What if there is no text mode!?
                        Vga *pVga = Machine::instance().getVga(0);
                        pVga->restoreMode();
                    }
                }

                bool bSet = false;
                while(desiredDepth > 8)
                {
                    if(pDisplay->setScreenMode(desiredWidth, desiredHeight, desiredDepth))
                    {
                        NOTICE("FramebufferFile: set mode " << Dec << desiredWidth << "x" << desiredHeight << "x" << desiredDepth << Hex << ".");
                        bSet = true;
                        break;
                    }
                    desiredDepth -= 8;
                }

                if(bSet)
                {
                    setSize(m_pProvider->pFramebuffer->getHeight() * m_pProvider->pFramebuffer->getBytesPerLine());
                }

                return bSet ? 0 : -1;
            }
            break;
        case PEDIGREE_FB_GETMODE:
            {
                pedigree_fb_mode *arg = reinterpret_cast<pedigree_fb_mode *>(buffer);
                arg->width = pFramebuffer->getWidth();
                arg->height = pFramebuffer->getHeight();
                arg->bytes_per_pixel = pFramebuffer->getBytesPerPixel();
                arg->format = pFramebuffer->getFormat();

                return 0;
            }
            break;
        case PEDIGREE_FB_REDRAW:
            {
                pedigree_fb_rect *arg = reinterpret_cast<pedigree_fb_rect *>(buffer);
                pFramebuffer->redraw(arg->x, arg->y, arg->w, arg->h, true);

                return 0;
            }
            break;
        default:
            return -1;
    }

    return -1;
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

    NOTICE("POSIX: random: " << rand());
    NOTICE("POSIX: random: " << rand());
    NOTICE("POSIX: random: " << rand());
    NOTICE("POSIX: random: " << rand());
    NOTICE("POSIX: random: " << rand());

    return true;
}
