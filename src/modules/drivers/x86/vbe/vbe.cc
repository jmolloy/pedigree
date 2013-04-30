#include <Log.h>
#include <Module.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <utilities/StaticString.h>
#include <utilities/List.h>
#include <machine/Machine.h>
#include <machine/Display.h>
#include <config/Config.h>
#include "VbeDisplay.h"

#include <graphics/Graphics.h>
#include <graphics/GraphicsService.h>

#include <machine/x86_common/Bios.h>

#define REALMODE_PTR(x) ((x[1] << 4) + x[0])

VbeDisplay *g_pDisplays[4];
size_t g_nDisplays = 0;

struct vbeControllerInfo {
   char signature[4];             // == "VESA"
   short version;                 // == 0x0300 for VBE 3.0
   short oemString[2];            // isa vbeFarPtr
   unsigned char capabilities[4];
   unsigned short videomodes[2];           // isa vbeFarPtr
   short totalMemory;             // as # of 64KB blocks
} __attribute__((packed));

struct vbeModeInfo {
  short attributes;
  char winA,winB;
  short granularity;
  short winsize;
  short segmentA, segmentB;
  unsigned short realFctPtr[2];
  short pitch; // chars per scanline

  short Xres, Yres;
  char Wchar, Ychar, planes, bpp, banks;
  uint8_t memory_model, bank_size, image_pages;
  char reserved0;

  char red_mask, red_position;
  char green_mask, green_position;
  char blue_mask, blue_position;
  char rsv_mask, rsv_position;
  char directcolor_attributes;

  // --- VBE 2.0 ---
  unsigned int framebuffer;
  unsigned int offscreen;
  short sz_offscreen; // In KB.
} __attribute__((packed));

Device *searchNode(Device *pDev, uintptr_t fbAddr)
{
  for (unsigned int i = 0; i < pDev->getNumChildren(); i++)
  {
    Device *pChild = pDev->getChild(i);
    String name;
    pChild->getName(name);
    // Get its addresses, and search for fbAddr.
    for (unsigned int j = 0; j < pChild->addresses().count(); j++)
    {
      if (pChild->getPciClassCode() == 0x03 &&
          pChild->addresses()[j]->m_Address <= fbAddr && (pChild->addresses()[j]->m_Address+pChild->addresses()[j]->m_Size) > fbAddr)
      {
        return pChild;
      }
    }

    // Recurse.
    Device *pRet = searchNode(pChild, fbAddr);
    if (pRet) return pRet;
  }
  return 0;
}

extern "C" void vbeModeChangedCallback(char *pId, char *pModeId)
{
    size_t id = strtoul(pId, 0, 10);
    size_t mode_id = strtoul(pModeId, 0, 10);

    if (id >= g_nDisplays) return;

    if (g_pDisplays[id]->getModeId() != mode_id)
    {
        g_pDisplays[id]->setScreenMode(mode_id);
    }
}

class VbeFramebuffer : public Framebuffer
{
    public:
        VbeFramebuffer() : Framebuffer()
        {
        }
        
        VbeFramebuffer(Display *pDisplay) :
            Framebuffer(), m_pDisplay(pDisplay), m_pBackbuffer(0), m_nBackbufferBytes(0)
        {
        }

        virtual void hwRedraw(size_t x = ~0UL, size_t y = ~0UL,
                              size_t w = ~0UL, size_t h = ~0UL)
        {
            if(x == ~0UL)
                x = 0;
            if(y == ~0UL)
                y = 0;
            if(w == ~0UL)
                w = getWidth();
            if(h == ~0UL)
                h = getHeight();

            /// \todo Subregions
            memcpy(m_pDisplay->getFramebuffer(), m_pBackbuffer, m_nBackbufferBytes);
        }
        
        virtual ~VbeFramebuffer()
        {
        }

        virtual void setFramebuffer(uintptr_t p)
        {
            Display::ScreenMode sm;
            m_pDisplay->getCurrentScreenMode(sm); /// \todo handle error
            m_nBackbufferBytes = sm.bytesPerLine * sm.height;
            if(m_nBackbufferBytes)
            {
                m_pBackbuffer = 0;

                if(m_pFramebufferRegion)
                {
                    delete m_pFramebufferRegion;
                }

                size_t nPages = (m_nBackbufferBytes + PhysicalMemoryManager::instance().getPageSize()) / PhysicalMemoryManager::instance().getPageSize();

                m_pFramebufferRegion = new MemoryRegion("VBE Backbuffer");
                if(!PhysicalMemoryManager::instance().allocateRegion(
                                            *m_pFramebufferRegion,
                                            nPages,
                                            0,
                                            VirtualAddressSpace::Write))
                {
                    delete m_pFramebufferRegion;
                    m_pFramebufferRegion = 0;
                }
                else
                {
                    NOTICE("VBE backbuffer is at " << reinterpret_cast<uintptr_t>(m_pFramebufferRegion->virtualAddress()));
                    m_pBackbuffer = reinterpret_cast<char*>(m_pFramebufferRegion->virtualAddress());
                }

                Framebuffer::setFramebuffer(reinterpret_cast<uintptr_t>(m_pBackbuffer));
            }
        }

    private:
        Display *m_pDisplay;
        char *m_pBackbuffer;
        size_t m_nBackbufferBytes;

        MemoryRegion *m_pFramebufferRegion;
};

void entry()
{

  List<Display::ScreenMode*> modeList;

  // Allocate some space for the information structure and prepare for a BIOS call.
  vbeControllerInfo *info = reinterpret_cast<vbeControllerInfo*> (Bios::instance().malloc(/*sizeof(vbeControllerInfo)*/256));
  vbeModeInfo *mode = reinterpret_cast<vbeModeInfo*> (Bios::instance().malloc(/*sizeof(vbeModeInfo)*/256));
  strncpy (info->signature, "VBE2", 4);
  Bios::instance().setAx (0x4F00);
  Bios::instance().setEs (0x0000);
  Bios::instance().setDi (static_cast<uint16_t>(reinterpret_cast<uintptr_t>(info)&0xFFFF));

  Bios::instance().executeInterrupt (0x10);

  // Check the signature.
  if (Bios::instance().getAx() != 0x004F || strncmp(info->signature, "VESA", 4) != 0)
  {
    ERROR("VBE: VESA not supported!");
    Bios::instance().free(reinterpret_cast<uintptr_t>(info));
    Bios::instance().free(reinterpret_cast<uintptr_t>(mode));
    return;
  }

  VbeDisplay::VbeVersion vbeVersion;
  switch (info->version)
  {
    case 0x0102:
      vbeVersion = VbeDisplay::Vbe1_2;
      break;
    case 0x0200:
      vbeVersion = VbeDisplay::Vbe2_0;
      break;
    case 0x0300:
      vbeVersion = VbeDisplay::Vbe3_0;
      break;
    default:
      ERROR("VBE: Unrecognised VESA version: " << Hex << info->version);
      Bios::instance().free(reinterpret_cast<uintptr_t>(info));
      Bios::instance().free(reinterpret_cast<uintptr_t>(mode));
      return;
  }

  size_t maxWidth = 0;
  size_t maxHeight = 0;
  size_t maxBpp = 0;

  uintptr_t fbAddr = 0;
  uint16_t *modes = reinterpret_cast<uint16_t*> (REALMODE_PTR(info->videomodes));
  for (int i = 0; modes[i] != 0xFFFF; i++)
  {
    Bios::instance().setAx(0x4F01);
    Bios::instance().setCx(modes[i]);
    Bios::instance().setEs (0x0000);
    Bios::instance().setDi (static_cast<uint16_t>(reinterpret_cast<uintptr_t>(mode)&0xFFFF));

    Bios::instance().executeInterrupt (0x10);

    if (Bios::instance().getAx() != 0x004F) continue;

    // Check if this is a graphics mode with LFB support.
    if ( (mode->attributes & 0x90) != 0x90 ) continue;

    // Check if this is a packed pixel or direct colour mode.
    if (mode->memory_model != 4 && mode->memory_model != 6) continue;

    // Add this pixel mode.
    Display::ScreenMode *pSm = new Display::ScreenMode;
    pSm->id = modes[i];
    pSm->width = mode->Xres;
    pSm->height = mode->Yres;
    pSm->refresh = 0;
    pSm->framebuffer = mode->framebuffer;
    fbAddr = mode->framebuffer;
    pSm->pf.mRed = mode->red_mask;
    pSm->pf.pRed = mode->red_position;
    pSm->pf.mGreen = mode->green_mask;
    pSm->pf.pGreen = mode->green_position;
    pSm->pf.mBlue = mode->blue_mask;
    pSm->pf.pBlue = mode->blue_position;
    pSm->pf.nBpp = mode->bpp;
    pSm->pf.nPitch = mode->pitch;
    modeList.pushBack(pSm);

    if(mode->Xres > maxWidth)
      maxWidth = mode->Xres;
    if(mode->Yres > maxHeight)
      maxHeight = mode->Yres;
    if(mode->bpp > maxBpp)
      maxBpp = mode->bpp;
  }

  // Total video memory, in bytes.
  size_t totalMemory = info->totalMemory * 64*1024;

  Bios::instance().free(reinterpret_cast<uintptr_t>(info));
  Bios::instance().free(reinterpret_cast<uintptr_t>(mode));

  NOTICE("VBE: Detected compatible display modes:");

  for (List<Display::ScreenMode*>::Iterator it = modeList.begin();
       it != modeList.end();
       it++)
  {
    Display::ScreenMode *pSm = *it;
    NOTICE(Hex << pSm->id << "\t " << Dec << pSm->width << "x" << pSm->height << "x" << pSm->pf.nBpp << "\t " << Hex <<
           pSm->framebuffer);
    NOTICE("    " << pSm->pf.mRed << "<<" << pSm->pf.pRed << "    " << pSm->pf.mGreen << "<<" << pSm->pf.pGreen << "    " << pSm->pf.mBlue << "<<" << pSm->pf.pBlue);
  }
  NOTICE("VBE: End of compatible display modes.");

  // Now that we have a framebuffer address, we can (hopefully) find the device in the device tree that owns that address.
  Device *pDevice = searchNode(&Device::root(), fbAddr);
  if (!pDevice)
  {
    ERROR("VBE: Device mapped to framebuffer address '" << Hex << fbAddr << "' not found.");
    return;
  }

  // Create a new VbeDisplay device node.
  VbeDisplay *pDisplay = new VbeDisplay(pDevice, vbeVersion, modeList, totalMemory, g_nDisplays);

  g_pDisplays[g_nDisplays] = pDisplay;

  // Does the display already exist in the database?
  bool bDelayedInsert = false;
  size_t mode_id = 0;
  String str;
  str.sprintf("SELECT * FROM displays WHERE pointer=%d", reinterpret_cast<uintptr_t>(pDisplay));
  Config::Result *pResult = Config::instance().query(str);
  if(!pResult)
  {
      ERROR("vbe: Got no result when selecting displays");
  }
  else if (pResult->succeeded() && pResult->rows() == 1)
  {
      mode_id = pResult->getNum(0, "mode_id");
      delete pResult;
      str.sprintf("UPDATE displays SET id=%d WHERE pointer=%d", g_nDisplays, reinterpret_cast<uintptr_t>(pDisplay));
      pResult = Config::instance().query(str);
      if (!pResult->succeeded())
          FATAL("Display update failed: " << pResult->errorMessage());
      delete pResult;
  }
  else if (pResult->succeeded() && pResult->rows() > 1)
  {
      delete pResult;
      FATAL("Multiple displays for pointer `" << reinterpret_cast<uintptr_t>(pDisplay) << "'");
  }
  else if (pResult->succeeded())
  {
      delete pResult;
      bDelayedInsert = true;
  }
  else
  {
      FATAL("Display select failed: " << pResult->errorMessage());
      delete pResult;
  }

  g_nDisplays++;

  /// \todo Desired mode should be in the configuration database with a fallback to
  ///       a default if the desired mode cannot be entered.
  /*
  bool switchedSuccessfully = true;
  NOTICE("Switching display mode...");
  if(!pDisplay->setScreenMode(mode_id = 0x117)) // 0x118 for 24-bit
  {
      NOTICE("vbe: Falling back to 800x600");

      // Attempt to fall back to 800x600
      if(!pDisplay->setScreenMode(mode_id = 0x114)) // 0x115 for 24-bit
      {
          NOTICE("vbe: Falling back to 640x480");

          // Finally try and fall back to 640x480
          if(!pDisplay->setScreenMode(mode_id = 0x111)) // 0x112 for 24-bit
          {
              ERROR("Couldn't find a suitable display mode for this system (tried: 1024x768, 800x600, 640x480.");
              switchedSuccessfully = false;
          }
      }
  }
  */
  
  VbeFramebuffer *pFramebuffer = new VbeFramebuffer(pDisplay);
  pDisplay->setLogicalFramebuffer(pFramebuffer);

  GraphicsService::GraphicsProvider *pProvider = new GraphicsService::GraphicsProvider;
  pProvider->pDisplay = pDisplay;
  pProvider->pFramebuffer = pFramebuffer;
  pProvider->maxWidth = maxWidth;
  pProvider->maxHeight = maxHeight;
  pProvider->maxDepth = maxBpp;
  pProvider->bHardwareAccel = false;
  pProvider->bTextModes = true;

  // Register with the graphics service
  ServiceFeatures *pFeatures = ServiceManager::instance().enumerateOperations(String("graphics"));
  Service         *pService  = ServiceManager::instance().getService(String("graphics"));
  if(pFeatures->provides(ServiceFeatures::touch))
    if(pService)
      pService->serve(ServiceFeatures::touch, reinterpret_cast<void*>(pProvider), sizeof(*pProvider));

  // Insert into the display table if it worked out
  /*
  if(switchedSuccessfully && bDelayedInsert)
  {
      str.sprintf("INSERT INTO displays VALUES (%d,%d,%d)", g_nDisplays, reinterpret_cast<uintptr_t>(pDisplay), mode_id);
      pResult = Config::instance().query(str);
      if (!pResult->succeeded())
          FATAL("Display insert failed: " << pResult->errorMessage());
      delete pResult;
  }
  */

  // Replace pDev with pDisplay.
  pDisplay->setParent(pDevice->getParent());
  pDevice->getParent()->replaceChild(pDevice, pDisplay);
}

void exit()
{
}

MODULE_INFO("vbe", &entry, &exit, "pci", "config");

