

#include "Dma.h"

#include <Log.h>
#include <processor/PhysicalMemoryManager.h>

Dma::Dma(IoBase *pRegs, IoBase *pFb, NvCard card, NvType type, uintptr_t ramSize) :
  m_pRegs(pRegs), m_pFramebuffer(pFb), m_Card(card), m_Type(type), m_nRamSize(ramSize), m_DmaBuffer("nVidia DMA Buffer")
{

  if (!PhysicalMemoryManager::instance().allocateRegion(m_DmaBuffer,
                                                        10,
#ifdef X86_COMMON
                                                        PhysicalMemoryManager::below16MB | 
#endif
							PhysicalMemoryManager::continuous | PhysicalMemoryManager::nonRamMemory,
                                                        VirtualAddressSpace::KernelMode | VirtualAddressSpace::Write))
  {
    ERROR("NVIDIA: Dma buffer allocation failed!");
    return;
  }
  m_pDmaBuffer = reinterpret_cast<uint32_t*> (m_DmaBuffer.virtualAddress());
  m_nPut = 0;
  m_nCurrent = 0;
}

void Dma::init()
{
  IoBase *pRegs = m_pRegs;

  // Powerup all nvidia hardware function blocks.
  pRegs->write32(0x13111111, NV32_PWRUPCTRL);
  // Enable "enhanced mode" (?)
  pRegs->write8(0x04, NVCRTCX_REPAINT1);

  // Set timer numerator to 8 (in b0-15)
  pRegs->write32(0x00000008, NVACC_PT_NUMERATOR);
  // Set timer denominator to 3 (in b0-15)
  pRegs->write32(0x00000003, NVACC_PT_DENOMINATR);

  // Disable timer-alarm INT requests
  pRegs->write32(0x00000000, NVACC_PT_INTEN);
  // Reset timer-alarm INT status bit (b0)
  pRegs->write32(0xffffffff, NVACC_PT_INTSTAT);

  // Enable PRAMIN write access on pre NV10 before programming it!
  if (m_Card == NV04A)
  {
    // Set framebuffer config: type = notiling, PRAMIN write access enabled */
    pRegs->write32(0x0001114, NV32_PFB_CONFIG_0);
  }
  else if (m_Type <= NV40 || m_Type == NV45)
  {
    // Setup acc engine 'source' tile addressranges.
    pRegs->write32(0, NVACC_NV10_FBTIL0AD);
    pRegs->write32(0, NVACC_NV10_FBTIL1AD);
    pRegs->write32(0, NVACC_NV10_FBTIL2AD);
    pRegs->write32(0, NVACC_NV10_FBTIL3AD);
    pRegs->write32(0, NVACC_NV10_FBTIL4AD);
    pRegs->write32(0, NVACC_NV10_FBTIL5AD);
    pRegs->write32(0, NVACC_NV10_FBTIL6AD);
    pRegs->write32(0, NVACC_NV10_FBTIL7AD);
    pRegs->write32(m_nRamSize-1, NVACC_NV10_FBTIL0ED);
    pRegs->write32(m_nRamSize-1, NVACC_NV10_FBTIL1ED);
    pRegs->write32(m_nRamSize-1, NVACC_NV10_FBTIL2ED);
    pRegs->write32(m_nRamSize-1, NVACC_NV10_FBTIL3ED);
    pRegs->write32(m_nRamSize-1, NVACC_NV10_FBTIL4ED);
    pRegs->write32(m_nRamSize-1, NVACC_NV10_FBTIL5ED);
    pRegs->write32(m_nRamSize-1, NVACC_NV10_FBTIL6ED);
    pRegs->write32(m_nRamSize-1, NVACC_NV10_FBTIL7ED);
  }
  else
  {
    ERROR ("NVIDIA: Not implemented");
  }

  // First clear the entire RAM hashtable (RAMHT).
  for (int i = 0; i < 0x0400; i++)
    pRegs->write32(0x00000000, NVACC_HT_HANDL_00 + (i << 2));

  // Setting up the FIFO handles. I'm keeping the used handles IDENTICAL to the haiku driver to aid debugging.
  if (m_Card >= NV40A)
  {
    // (first set)
    pRegs->write32(0x80000000 | NV10_CONTEXT_SURFACES_2D, NVACC_HT_HANDL_00); // 32bit handle (not used)
    pRegs->write32(0x0010114c, NVACC_HT_VALUE_00); // Instance 0x114c, Channel = 0x00

    pRegs->write32(0x80000000 | NV_IMAGE_BLIT, NVACC_HT_HANDL_01); // 32bit handle (not used)
    pRegs->write32(0x00101148, NVACC_HT_VALUE_01); // Instance 0x1148, Channel = 0x00

    pRegs->write32(0x80000000 | NV4_GDI_RECTANGLE_TEXT, NVACC_HT_HANDL_02); // 32bit handle (not used)
    pRegs->write32(0x0010114a, NVACC_HT_VALUE_02); // Instance 0x114a, Channel = 0x00

    // (second set)
    pRegs->write32(0x80000000 | NV_ROP5_SOLID, NVACC_HT_HANDL_10); // 32bit handle (not used)
    pRegs->write32(0x00101142, NVACC_HT_VALUE_10); // Instance 0x1142, Channel = 0x00

    pRegs->write32(0x80000000 | NV_IMAGE_BLACK_RECTANGLE, NVACC_HT_HANDL_11); // 32bit handle (not used)
    pRegs->write32(0x00101144, NVACC_HT_VALUE_11); // Instance 0x1144, Channel = 0x00

    pRegs->write32(0x80000000 | NV_IMAGE_PATTERN, NVACC_HT_HANDL_12); // 32bit handle (not used)
    pRegs->write32(0x00101146, NVACC_HT_VALUE_12); // Instance 0x1146, Channel = 0x00

    pRegs->write32(0x80000000 | NV_SCALED_IMAGE_FROM_MEMORY, NVACC_HT_HANDL_13); // 32bit handle (not used)
    pRegs->write32(0x0010114e, NVACC_HT_VALUE_13); // Instance 0x114e, Channel = 0x00

    // Set up FIFO contexts in instance RAM.
    pRegs->write32(0x00003000, NVACC_PR_CTX0_R); // DMA page table present and of linear type.
    pRegs->write32(m_nRamSize-1, NVACC_PR_CTX1_R); // DMA limit: size is all cardRAM.
    pRegs->write32((0x00000000 & 0xfffff000) | 0x00000002, NVACC_PR_CTX2_R); // DMA access type is READ_AND_WRITE;
                                                                             // Memory starts at start of cardRAM (b12-31).
    pRegs->write32(0x00000002, NVACC_PR_CTX3_R); // Unknown.

    // Setting up for set '0', command NV_ROP5_SOLID.
    pRegs->write32(0x02080043, NVACC_PR_CTX0_0); // NVclass 0x43, patchcfg ROP_AND, nv10+: little endian.
    pRegs->write32(0x00000000, NVACC_PR_CTX1_0); // colorspace not set, notify instance invalid (b16-31)
    pRegs->write32(0x00000000, NVACC_PR_CTX2_0); // DMA0 and DMA1 instance invalid
    pRegs->write32(0x00000000, NVACC_PR_CTX3_0); // Method traps disabled.
    pRegs->write32(0x00000000, NVACC_PR_CTX0_1); // extra
    pRegs->write32(0x00000000, NVACC_PR_CTX1_1); // extra

    // Setting up for set '1', command NV_IMAGE_BLACK_RECTANGLE
    pRegs->write32(0x02080019, NVACC_PR_CTX0_2); // NVclass 0x019, patchvfg ROP_AND, nv10+: little endian.
    pRegs->write32(0x00000000, NVACC_PR_CTX1_2); // colorspace not set, notify instance invalid (b16-31)
    pRegs->write32(0x00000000, NVACC_PR_CTX2_2); // DMA0 and DMA1 instance invalid
    pRegs->write32(0x00000000, NVACC_PR_CTX3_2); // Method traps disabled.
    pRegs->write32(0x00000000, NVACC_PR_CTX0_3); // extra
    pRegs->write32(0x00000000, NVACC_PR_CTX1_3); // extra

    // Setting up for set '2', command NV_IMAGE_PATTERN
    pRegs->write32(0x02080018, NVACC_PR_CTX0_4); // NVclass 0x018, patchcfg ROP_AND, nv10+: little endian.
    pRegs->write32(0x00000000, NVACC_PR_CTX1_4); // colorspace not set, notify instance invalid (b16-31)
    pRegs->write32(0x00000000, NVACC_PR_CTX2_4); // DMA0 and DMA1 instance invalid
    pRegs->write32(0x00000000, NVACC_PR_CTX3_4); // Method traps disabled.
    pRegs->write32(0x00000000, NVACC_PR_CTX0_5); // extra
    pRegs->write32(0x00000000, NVACC_PR_CTX1_5); // extra

    // Setting up for set '4', command NV12_IMAGE_BLIT
    pRegs->write32(0x0208009f, NVACC_PR_CTX0_6); // NVclass 0x09f, patchcfg ROP_AND, nv10+: little endian
    pRegs->write32(0x00000000, NVACC_PR_CTX1_6); // colorspace not set, notify instance invalid (b16-31)
    pRegs->write32(0x00001140, NVACC_PR_CTX2_6); // DMA0 instance 0x1140 and DMA1 instance invalid
    pRegs->write32(0x00001140, NVACC_PR_CTX3_6); // trap 0 is 0x1140, trap 1 disabled.
    pRegs->write32(0x00000000, NVACC_PR_CTX0_7); // extra
    pRegs->write32(0x00000000, NVACC_PR_CTX1_7); // extra

    // Setting up for set '5', command NV4_GDI_RECTANGLE_TEXT
    pRegs->write32(0x0208004a, NVACC_PR_CTX0_8); // NVclass 0x04a, patchcfg ROP_AND, nv10+: little endian
    pRegs->write32(0x02000000, NVACC_PR_CTX1_8); // colorspace not set, notify instance is 0x0200
    pRegs->write32(0x00000000, NVACC_PR_CTX2_8); // DMA0 and DMA1 instance invalid
    pRegs->write32(0x00000000, NVACC_PR_CTX3_8); // Method traps disabled.
    pRegs->write32(0x00000000, NVACC_PR_CTX0_9); // extra
    pRegs->write32(0x00000000, NVACC_PR_CTX1_9); // extra

    // Setting up for set '6', command NV10_CONTEXT_SURFACES_2D
    pRegs->write32(0x02080062, NVACC_PR_CTX0_A); // NVclass 0x062, nv10+: little endian
    pRegs->write32(0x00000000, NVACC_PR_CTX1_A); // colorspace not set, notify instance invalid (b16-31)
    pRegs->write32(0x00001140, NVACC_PR_CTX2_A); // DMA0 instance 0x1140 and DMA1 instance invalid
    pRegs->write32(0x00001140, NVACC_PR_CTX3_A); // trap 0 is 0x1140, trap 1 disabled.
    pRegs->write32(0x00000000, NVACC_PR_CTX0_B); // extra
    pRegs->write32(0x00000000, NVACC_PR_CTX1_B); // extra

    // Setting up for set '7', command NV_SCALED_IMAGE_FROM_MEMORY
    pRegs->write32(0x02080077, NVACC_PR_CTX0_C); // NVclass 0x77, nv10+: little endian
    pRegs->write32(0x00000000, NVACC_PR_CTX1_C); // colorspace not set, notify instance invalid (b16-31)
    pRegs->write32(0x00001140, NVACC_PR_CTX2_C); // DMA0 instance 0x1140 and DMA1 instance invalid
    pRegs->write32(0x00001140, NVACC_PR_CTX3_C); // trap 0 is 0x1140, trap 1 disabled.
    pRegs->write32(0x00000000, NVACC_PR_CTX0_D); // extra
    pRegs->write32(0x00000000, NVACC_PR_CTX1_D); // extra

    // Setup DMA set pointed at by PF_CACH1_DMAI
    pRegs->write32(0x00003002, NVACC_PR_CTX0_E); // DMA page table present and of linear type.
    pRegs->write32(0x00007fff, NVACC_PR_CTX1_E); // DMA limit: tablesize is 32k bytes.
    pRegs->write32( ((m_nRamSize-1)&0xffff8000) | 0x00000002, NVACC_PR_CTX2_E); // DMA access in READ_AND_WRITE
                                                                               // table is located at end of cardRAM.

    m_nDmaBuffer = (m_nRamSize-1) & 0xffff8000; // This is the index into the FRAMEBUFFER, not CTRL area!!

  }
  else
  {
    // (first set)
    pRegs->write32(0x80000000 | NV4_SURFACE, NVACC_HT_HANDL_00); // 32bit handle (not used)
    pRegs->write32(0x80011145, NVACC_HT_VALUE_00); // Instance 0x114c, Channel = 0x00

    pRegs->write32(0x80000000 | NV_IMAGE_BLIT, NVACC_HT_HANDL_01); // 32bit handle (not used)
    pRegs->write32(0x80011146, NVACC_HT_VALUE_01); // Instance 0x1148, Channel = 0x00

    pRegs->write32(0x80000000 | NV4_GDI_RECTANGLE_TEXT, NVACC_HT_HANDL_02); // 32bit handle (not used)
    pRegs->write32(0x80011147, NVACC_HT_VALUE_02); // Instance 0x114a, Channel = 0x00

    pRegs->write32(0x80000000 | NV4_CONTEXT_SURFACES_ARGB_ZS, NVACC_HT_HANDL_03); // 32bit handle (not used)
    pRegs->write32(0x80011148, NVACC_HT_VALUE_03); // Instance 0x114a, Channel = 0x00

    pRegs->write32(0x80000000 | NV4_DX5_TEXTURE_TRIANGLE, NVACC_HT_HANDL_04); // 32bit handle (not used)
    pRegs->write32(0x80011149, NVACC_HT_VALUE_04); // Instance 0x114a, Channel = 0x00

    pRegs->write32(0x80000000 | NV4_DX6_MULTI_TEXTURE_TRIANGLE, NVACC_HT_HANDL_05); // 32bit handle (not used)
    pRegs->write32(0x8001114a, NVACC_HT_VALUE_05); // Instance 0x114a, Channel = 0x00

    pRegs->write32(0x80000000 | NV1_RENDER_SOLID_LIN, NVACC_HT_HANDL_06); // 32bit handle (not used)
    pRegs->write32(0x8001114c, NVACC_HT_VALUE_06); // Instance 0x114a, Channel = 0x00

    // (second set)
    pRegs->write32(0x80000000 | NV_ROP5_SOLID, NVACC_HT_HANDL_10); // 32bit handle (not used)
    pRegs->write32(0x80011142, NVACC_HT_VALUE_10); // Instance 0x1142, Channel = 0x00

    pRegs->write32(0x80000000 | NV_IMAGE_BLACK_RECTANGLE, NVACC_HT_HANDL_11); // 32bit handle (not used)
    pRegs->write32(0x80011143, NVACC_HT_VALUE_11); // Instance 0x1143, Channel = 0x00

    pRegs->write32(0x80000000 | NV_IMAGE_PATTERN, NVACC_HT_HANDL_12); // 32bit handle (not used)
    pRegs->write32(0x80011144, NVACC_HT_VALUE_12); // Instance 0x1144, Channel = 0x00

    pRegs->write32(0x80000000 | NV_SCALED_IMAGE_FROM_MEMORY, NVACC_HT_HANDL_13); // 32bit handle (not used)
    pRegs->write32(0x8001114b, NVACC_HT_VALUE_13); // Instance 0x114B, Channel = 0x00

    // Set up FIFO contexts in instance RAM.
    pRegs->write32(0x00003000, NVACC_PR_CTX0_R); // DMA page table present and of linear type.
    pRegs->write32(m_nRamSize-1, NVACC_PR_CTX1_R); // DMA limit: size is all cardRAM.
    pRegs->write32((0x00000000 & 0xfffff000) | 0x00000002, NVACC_PR_CTX2_R); // DMA access type is READ_AND_WRITE;
                                                                             // Memory starts at start of cardRAM (b12-31).
    pRegs->write32(0x00000002, NVACC_PR_CTX3_R); // Unknown.

    // Setting up for set '0', command NV_ROP5_SOLID.
    pRegs->write32(0x01080043, NVACC_PR_CTX0_0); // NVclass 0x43, patchcfg ROP_AND, nv10+: little endian.
    pRegs->write32(0x00000000, NVACC_PR_CTX1_0); // colorspace not set, notify instance invalid (b16-31)
    pRegs->write32(0x00000000, NVACC_PR_CTX2_0); // DMA0 and DMA1 instance invalid
    pRegs->write32(0x00000000, NVACC_PR_CTX3_0); // Method traps disabled.

    // Setting up for set '1', command NV_IMAGE_BLACK_RECTANGLE
    pRegs->write32(0x01080019, NVACC_PR_CTX0_1); // NVclass 0x019, patchvfg ROP_AND, nv10+: little endian.
    pRegs->write32(0x00000000, NVACC_PR_CTX1_1); // colorspace not set, notify instance invalid (b16-31)
    pRegs->write32(0x00000000, NVACC_PR_CTX2_1); // DMA0 and DMA1 instance invalid
    pRegs->write32(0x00000000, NVACC_PR_CTX3_1); // Method traps disabled.

    // Setting up for set '2', command NV_IMAGE_PATTERN
    pRegs->write32(0x01080018, NVACC_PR_CTX0_2); // NVclass 0x018, patchcfg ROP_AND, nv10+: little endian.
    pRegs->write32(0x00000002, NVACC_PR_CTX1_2); // colorspace not set, notify instance is $0200 (b16-31)
    pRegs->write32(0x00000000, NVACC_PR_CTX2_2); // DMA0 and DMA1 instance invalid
    pRegs->write32(0x00000000, NVACC_PR_CTX3_2); // Method traps disabled.

    // Setting up for set '3', command...
    if (m_Card > NV10A)
      // NV10_CONTEXT_SURFACES_2D
      pRegs->write32(0x01008062, NVACC_PR_CTX0_3);
    else
      // NV4_SURFACE
      pRegs->write32(0x01008042, NVACC_PR_CTX0_3);
    pRegs->write32(0x00000000, NVACC_PR_CTX1_3); // colorspace not set, notify instance invalid (b16-31)
    pRegs->write32(0x11401140, NVACC_PR_CTX2_3); // DMA0 and DMA1 instance invalid
    pRegs->write32(0x00000000, NVACC_PR_CTX3_3); // Method traps disabled.

    // Setting up for set '4', command NV12_IMAGE_BLIT
    if (m_Type >= NV11)
      pRegs->write32(0x0100809f, NVACC_PR_CTX0_4); // NVclass 0x09f, patchcfg ROP_AND, nv10+: little endian
    else
      // ... or NV_IMAGE_BLIT
      pRegs->write32(0x0100805f, NVACC_PR_CTX0_4);
    pRegs->write32(0x00000000, NVACC_PR_CTX1_4); // colorspace not set, notify instance invalid (b16-31)
    pRegs->write32(0x11401140, NVACC_PR_CTX2_4); // DMA0 instance 0x1140 and DMA1 instance invalid
    pRegs->write32(0x00000000, NVACC_PR_CTX3_4); // trap 0 is 0x1140, trap 1 disabled.

    // Setting up for set '5', command NV4_GDI_RECTANGLE_TEXT
    pRegs->write32(0x0100804a, NVACC_PR_CTX0_5); // NVclass 0x04a, patchcfg ROP_AND, nv10+: little endian
    pRegs->write32(0x00000002, NVACC_PR_CTX1_5); // colorspace not set, notify instance is 0x0200
    pRegs->write32(0x00000000, NVACC_PR_CTX2_5); // DMA0 and DMA1 instance invalid
    pRegs->write32(0x00000000, NVACC_PR_CTX3_5); // Method traps disabled.

    // Setting up for set '6', command ...
    if (m_Card >= NV10A)
      // NV10_CONTEXT_SURFACES_ARGB_ZS
      pRegs->write32(0x00000093, NVACC_PR_CTX0_6); // NVclass 0x062, nv10+: little endian
    else
      // NV4_CONTEXT_SURFACES_ARGB_ZS
      pRegs->write32(0x00000053, NVACC_PR_CTX0_6);
    pRegs->write32(0x00000000, NVACC_PR_CTX1_6); // colorspace not set, notify instance invalid (b16-31)
    pRegs->write32(0x11401140, NVACC_PR_CTX2_6); // DMA0 instance 0x1140 and DMA1 instance invalid
    pRegs->write32(0x00000000, NVACC_PR_CTX3_6); // trap 0 is 0x1140, trap 1 disabled.

    // Setting up for set '7', command ...
    if (m_Card >= NV10A)
      // NV10_DX5_TEXTURE_TRIANGLE
      pRegs->write32(0x0300a094, NVACC_PR_CTX0_7); // NVclass 0x77, nv10+: little endian
    else
      // NV4_DX5_TEXTURE_TRIANGLE
      pRegs->write32(0x0300a054, NVACC_PR_CTX0_7);
    pRegs->write32(0x00000000, NVACC_PR_CTX1_7); // colorspace not set, notify instance invalid (b16-31)
    pRegs->write32(0x11401140, NVACC_PR_CTX2_7); // DMA0 instance 0x1140 and DMA1 instance invalid
    pRegs->write32(0x00000000, NVACC_PR_CTX3_7); // trap 0 is 0x1140, trap 1 disabled.

    // Setting up for set '8', command ...
    if (m_Card >= NV10A)
      // NV10_DX6_MULTI_TEXTURE_TRIANGLE
      pRegs->write32(0x0300a095, NVACC_PR_CTX0_8); // NVclass 0x77, nv10+: little endian
    else
      // NV4_DX6_MULTI_TEXTURE_TRIANGLE
      pRegs->write32(0x0300a055, NVACC_PR_CTX0_8);
    pRegs->write32(0x00000000, NVACC_PR_CTX1_8); // colorspace not set, notify instance invalid (b16-31)
    pRegs->write32(0x11401140, NVACC_PR_CTX2_8); // DMA0 instance 0x1140 and DMA1 instance invalid
    pRegs->write32(0x00000000, NVACC_PR_CTX3_8); // trap 0 is 0x1140, trap 1 disabled.

    // Setting up for set '9', command 'NV_SCALED_IMAGE_FROM_MEMORY'
    pRegs->write32(0x01018077, NVACC_PR_CTX0_9); // NVclass 0x77, nv10+: little endian
    pRegs->write32(0x00000000, NVACC_PR_CTX1_9); // colorspace not set, notify instance invalid (b16-31)
    pRegs->write32(0x11401140, NVACC_PR_CTX2_9); // DMA0 instance 0x1140 and DMA1 instance invalid
    pRegs->write32(0x00000000, NVACC_PR_CTX3_9); // trap 0 is 0x1140, trap 1 disabled.

    // Setting up for set 'A', command 'NV1_RENDER_SOLID_LIN'
    pRegs->write32(0x0300a01c, NVACC_PR_CTX0_A); // NVclass 0x77, nv10+: little endian
    pRegs->write32(0x00000000, NVACC_PR_CTX1_A); // colorspace not set, notify instance invalid (b16-31)
    pRegs->write32(0x11401140, NVACC_PR_CTX2_A); // DMA0 instance 0x1140 and DMA1 instance invalid
    pRegs->write32(0x00000000, NVACC_PR_CTX3_A); // trap 0 is 0x1140, trap 1 disabled.

    // Setup DMA set pointed at by PF_CACH1_DMAI
    if (/* isAGP */ false)
      pRegs->write32(0x00033002, NVACC_PR_CTX0_C); // DMA page table present and of linear type.
    else
      // PCI, not AGP.
      pRegs->write32(0x00023002, NVACC_PR_CTX0_C); // DMA page table present and of linear type.
    pRegs->write32(0x00007fff, NVACC_PR_CTX1_C); // DMA limit: tablesize is 32k bytes.
    pRegs->write32( m_DmaBuffer.physicalAddress() | 0x00000002, NVACC_PR_CTX2_C); // DMA access in READ_AND_WRITE
                                                                               // table is located in main system RAM.
    // Stuff for 3D only?
    pRegs->write32(0x00000500, NVACC_BPIXEL); // 16-bit little endian RGB.
  }

  // Do an explicit engine reset.
  pRegs->write32(0xffffffff, NVACC_DEBUG0);
  pRegs->write32(0x00000000, NVACC_DEBUG0);

  // Disable all acceleration engine INT requests.
  pRegs->write32(0x00000000, NVACC_ACC_INTE);
  // Reset all acceleration engine INT status bits.
  pRegs->write32(0xffffffff, NVACC_ACC_INTS);

  // Context control enabled.
  pRegs->write32(0x10010100, NVACC_NV10_CTX_CTRL);
  // All acceleration buffers, pitches and colors are valid.
  pRegs->write32(0xffffffff, NVACC_NV10_ACC_STAT);
  // Enable acceleration engine command FIFO
  pRegs->write32(0x00000001, NVACC_FIFO_EN);
  // Setup surface type:
  pRegs->write32(pRegs->read32(NVACC_NV10_SURF_TYP) & 0x0007ff00, NVACC_NV10_SURF_TYP);
  pRegs->write32(pRegs->read32(NVACC_NV10_SURF_TYP) | 0x00020101, NVACC_NV10_SURF_TYP);

  // Does exactly what it says on the tin.
  setUpReverseEngineeredMagicRegs();

  // Setup clipping.
  pRegs->write32(0x00000000, NVACC_ABS_UCLP_XMIN);
  pRegs->write32(0x00000000, NVACC_ABS_UCLP_YMIN);
  pRegs->write32(0x00007fff, NVACC_ABS_UCLP_XMAX);
  pRegs->write32(0x00007fff, NVACC_ABS_UCLP_YMAX);

  // **PFIFO**

  // (setup caches)
  // disable caches reassign
  pRegs->write32(0x00000000, NVACC_PF_CACHES);
  // PFIFO mode: channel 0 is in DMA mode, channels 1-32 are in PIO mode.
  pRegs->write32(0x00000001, NVACC_PF_MODE);
  // Cache1 push0 access disabled
  pRegs->write32(0x00000000, NVACC_PF_CACH1_PSH0);
  // Cache1 pull0 access disabled
  pRegs->write32(0x00000000, NVACC_PF_CACH1_PUL0);
  // Cache1 push1 mode = DMA
  if (m_Card >= NV40A)
    pRegs->write32(0x00010000, NVACC_PF_CACH1_PSH1);
  else
    pRegs->write32(0x00000100, NVACC_PF_CACH1_PSH1);
  // Cache1 DMA Put offset = 0
  pRegs->write32(0x00000000, NVACC_PF_CACH1_DMAP);
  // Cache1 DMA Get offset = 0;
  pRegs->write32(0x00000000, NVACC_PF_CACH1_DMAG);
  // Cache1 DMA instance address  = 0x0114e (but haiku has 0x1150 for >=NV40A...
  if (m_Card >= NV40A)
    pRegs->write32(0x00001150, NVACC_PF_CACH1_DMAI);
  else
    pRegs->write32(0x0000114e, NVACC_PF_CACH1_DMAI);
  // Cache0 push0 access disabled
  pRegs->write32(0x00000000, NVACC_PF_CACH0_PSH0);
  // Cache0 pull0 access disabled.
  pRegs->write32(0x00000000, NVACC_PF_CACH0_PUL0);

  // RAMHT (hashtable) base address  = 0x10000, size=4k
  // search  = 128 (is byte offset between hash 'sets')
  // NOTE
  // so HT base is 0x00710000, last is 0x00710fff.
  // In this space you defined the engine command handles (HT_HANDL_XX) which
  // in turn points to the defines in CTX register space (which is sort of RAM)
  pRegs->write32(0x03000100, NVACC_PF_RAMHT);
  // RAM FC baseaddress = 0x11000 (fize is fixed to 0x5k(?))
  pRegs->write32(0x00000110, NVACC_PF_RAMFC);
  // RAM RO baseaddress = 0x11200
  pRegs->write32(0x00000112, NVACC_PF_RAMRO);
  // PFIFO size: ch0-15 = 512 bytes, ch16-31 = 124 bytes
  pRegs->write32(0x0000ffff, NVACC_PF_SIZE);
  // Cache1 hash instance = 0xffff
  pRegs->write32(0x0000ffff, NVACC_PF_CACH1_HASH);
  // Disable all PFIFO INTs
  pRegs->write32(0x00000000, NVACC_PF_INTEN);
  // Reset all PFOFP INT status bits
  pRegs->write32(0xffffffff, NVACC_PF_INTSTAT);
  // cach0 pull0 engine  = acceleration engine (graphics)
  pRegs->write32(0x00000001, NVACC_PF_CACH0_PUL1);
  // Cache1 DMA control: disable some stuff
  pRegs->write32(0x00000000, NVACC_PF_CACH1_DMAC);
  // cache1 engine 0 upto/including 7 is software (could also be graphics or DVD)
  pRegs->write32(0x00000000, NVACC_PF_CACH1_ENG);
  // cache1 DMA fetch: trigger at 128 bytes, size is 32 bytes, max requests is 15, use little endian.
  pRegs->write32(0x000f0078, NVACC_PF_CACH1_DMAF);
  // cache1 DMA push, b0 = 1: access is enabled
  pRegs->write32(0x00000001, NVACC_PF_CACH1_DMAS);
  // cache1 push0 access enabled.
  pRegs->write32(0x00000001, NVACC_PF_CACH1_PSH0);
  // cache1 pull0 access enabled.
  pRegs->write32(0x00000001, NVACC_PF_CACH1_PUL0);
  // cache1 pull1 engine = acceleration engine (graphics)
  pRegs->write32(0x00000001, NVACC_PF_CACH1_PUL1);
  // Enable PFIFO caches reassign
  pRegs->write32(0x00000001, NVACC_PF_CACHES);

  // Init DMA command buffer info.

  m_nPut = 0;
  m_nCurrent = 0;

  // The DMA buffer can hold 8k 32-bit words (it's 32kb in size),
  // or 256k 32-bit words (1Mb in size) dependant on architecture.
  // One word is reserved at the end of the buffer to be able to instruct the engine to do a buffer wrap-around!
  m_nMax = 8192-1;
  m_nFree = m_nMax;

  // Set up each FIFO.
  m_pFifoPtrs[NV_ROP5_SOLID]            = 0 * 0x00002000;
  m_pFifoPtrs[NV_IMAGE_BLACK_RECTANGLE] = 1 * 0x00002000;
  m_pFifoPtrs[NV_IMAGE_PATTERN]         = 2 * 0x00002000;
  m_pFifoPtrs[NV4_SURFACE]              = 3 * 0x00002000;
  m_pFifoPtrs[NV_IMAGE_BLIT]            = 4 * 0x00002000;
  m_pFifoPtrs[NV4_GDI_RECTANGLE_TEXT]   = 5 * 0x00002000;
  m_pFifoPtrs[NV4_CONTEXT_SURFACES_ARGB_ZS] = 6 * 0x00002000;
  m_pFifoPtrs[NV4_DX5_TEXTURE_TRIANGLE] = 7 * 0x00002000;

  m_pFifos[0] = NV_ROP5_SOLID;
  m_pFifos[1] = NV_IMAGE_BLACK_RECTANGLE;
  m_pFifos[2] = NV_IMAGE_PATTERN;
  m_pFifos[3] = NV4_SURFACE;
  m_pFifos[4] = NV_IMAGE_BLIT;
  m_pFifos[5] = NV4_GDI_RECTANGLE_TEXT;
  m_pFifos[6] = NV_SCALED_IMAGE_FROM_MEMORY;

  initFifo(NV_GENERAL_FIFO_CH0, m_pFifos[0]);
  initFifo(NV_GENERAL_FIFO_CH1, m_pFifos[1]);
  initFifo(NV_GENERAL_FIFO_CH2, m_pFifos[2]);
  initFifo(NV_GENERAL_FIFO_CH3, m_pFifos[3]);
  initFifo(NV_GENERAL_FIFO_CH4, m_pFifos[4]);
  initFifo(NV_GENERAL_FIFO_CH5, m_pFifos[5]);
  initFifo(NV_GENERAL_FIFO_CH6, m_pFifos[6]);

  // Set pixel width.
  ensureFree(5);
  dmaCmd(NV4_SURFACE, NV4_SURFACE_FORMAT, 4);
  writeBuffer(4); // Surface depth (for 16-bit little endian)
  writeBuffer(2048 | (2048<<16)); // bytes per row (copied?)
  writeBuffer(0); // OffsetSource
  writeBuffer(0); // OffsetDest - screen location.

  // Setup pattern colourdepth
  ensureFree(2);
  dmaCmd(NV_IMAGE_PATTERN, NV_IMAGE_PATTERN_SETCOLORFORMAT, 1);
  writeBuffer(1); // cmd depth - 1 for 16 bit little endian.

  ensureFree(2);
  dmaCmd(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_SETCOLORFORMAT, 1);
  writeBuffer(1); // cmd depth - 1 for 16 bit little endian.

  // Load our pattern into the engine.
  ensureFree(7);
  dmaCmd(NV_IMAGE_PATTERN, NV_IMAGE_PATTERN_SETSHAPE, 1);
  writeBuffer(0x00000000); // SetShape: 0 = 8x8, 1 = 64x1, 2 = 1x64
  dmaCmd(NV_IMAGE_PATTERN, NV_IMAGE_PATTERN_SETCOLOR0, 4);
  writeBuffer(0xffffffff); // Setcolor0
  writeBuffer(0xffffffff); // Setcolor1
  writeBuffer(0xffffffff); // Setpattern0
  writeBuffer(0xffffffff); // Setpattern1

  // Start DMA transfer!
  start();
}

Dma::~Dma()
{
}

void Dma::dmaCmd(uint32_t cmd, uint32_t offset, uint16_t size)
{
  if (m_Card >= NV40A)
    m_pFramebuffer->write32( (size<<18) | ((m_pFifoPtrs[cmd] + offset) & 0x0000fffc), m_nDmaBuffer + ((m_nCurrent++)<<2));
  else
    m_pDmaBuffer[ m_nCurrent++ ] = ((size<<18) | ((m_pFifoPtrs[cmd]+offset)&0x0000fffc));
  m_nFree -= (size+1);
}

void Dma::writeBuffer(uint32_t arg)
{
  if (m_Card >= NV40A)
    m_pFramebuffer->write32( arg, m_nDmaBuffer + ((m_nCurrent++)<<2) );
  else
    m_pDmaBuffer[ m_nCurrent++ ] = arg;
}

void Dma::start()
{
  if (m_nCurrent != m_nPut)
  {
    m_nPut = m_nCurrent;
    m_pRegs->write32(m_nPut << 2, NVACC_FIFO + NV_GENERAL_DMAPUT);
  }
}

void Dma::ensureFree(uint16_t cmd_size)
{
  while ( (m_nFree < cmd_size) )
  {
    uint32_t dmaget = m_pRegs->read32(NVACC_FIFO + NV_GENERAL_DMAGET) >> 2;
    NOTICE("Get: " << Hex << dmaget);
    if (m_nPut >= dmaget)
    {
      // Engine is fetching 'behind us', the last piece of the buffer is free.

      m_nFree = m_nMax - m_nCurrent;
      if (m_nFree < cmd_size)
      {
        // Not enough room left, so instruct DMA engine to reset the buffer when it reaches the end of it.
        writeBuffer(0x20000000);
        m_nCurrent = 0;
        start();

        // NOW the engine is fetching 'in front of us', so the first piece of the buffer is free.
        m_nFree = dmaget - m_nCurrent;

        // Leave a gap between where we put new commands and where the engine is executing. Else we can crash the engine.
        if (m_nFree < 256)
          m_nFree = 0;
        else
          m_nFree -= 256;
      }
    }
    else
    {
      // Engine is fetching 'in front of us', so the first piece of the buffer is free.
      m_nFree = dmaget - m_nCurrent;

      // Leave a gap between where we put new commands and where the engine is executing. Else we can crash the engine.
      if (m_nFree < 256)
        m_nFree = 0;
      else
        m_nFree -= 256;
    }
  }
}

void Dma::initFifo(uint32_t ch, uint32_t handle)
{
  // Issue FIFO channel assign cmd.
  NOTICE("initFifo: put: " << Hex << m_nPut << ", cur: " << m_nCurrent);
  writeBuffer( (1 << 18) | ch );
  writeBuffer( 0x80000000 | handle );

  m_nFree -= 2;
}

void Dma::screenToScreenBlit(uint16_t src_x, uint16_t src_y, uint16_t dest_x, uint16_t dest_y, uint16_t h, uint16_t w)
{
  ensureFree(2);

  // Now setup ROP (Raster OP) for GXCopy */
  dmaCmd(NV_ROP5_SOLID, NV_ROP5_SOLID_SETROP5, 1);
  writeBuffer(0xcc); // SetRop5

  ensureFree(4);

  dmaCmd(NV_IMAGE_BLIT, NV_IMAGE_BLIT_SOURCEORG, 3);
  writeBuffer( (src_y << 16) | src_x );
  writeBuffer( (dest_y << 16) | dest_x );
  writeBuffer( ((h+1) << 16) | (w+1) );

  start();
  NOTICE("screenToScreenBlit -- end");
}

void Dma::fillRectangle(uint16_t x, uint16_t y, uint16_t h, uint16_t w)
{
  uint32_t c = 0xFFFFFFFF;

  ensureFree(2);

  // Now setup ROP (Raster OP) for GXCopy
  dmaCmd(NV_ROP5_SOLID, NV_ROP5_SOLID_SETROP5, 1);
  writeBuffer(0xcc); // SetRop5

  // Setup fill colour
  dmaCmd(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_COLOR1A, 1);
  writeBuffer(c);

  ensureFree(3);

  dmaCmd(NV4_GDI_RECTANGLE_TEXT, NV4_GDI_RECTANGLE_TEXT_UCR0_LEFTTOP, 2);
  writeBuffer( (x << 16) | (y&0xffff) );
  writeBuffer( ((w+1) << 16) | (h+1) );

  start();
}

void Dma::setUpReverseEngineeredMagicRegs()
{
  switch(m_Card)
  {
  case NV40A:
  {
    // Init some function blocks.
    m_pRegs->write32(0x401287c0, NVACC_DEBUG1);
    m_pRegs->write32(0x60de8051, NVACC_DEBUG3);
    // Disable specific functions, but enable SETUP_SPARE2.
    m_pRegs->write32(0x00008000, NVACC_NV10_DEBUG4);
    m_pRegs->write32(0x00be3c5f, NVACC_NV25_WHAT0);

    // Setup some unknown serially accessed registers
    uint32_t tmp = m_pRegs->read32(NV32_NV4X_WHAT0) & 0xff;
    for (int cnt = 0; (tmp && !(tmp&0x1)); tmp >>= 1, cnt++)
      m_pRegs->write32(cnt, NVACC_NV4X_WHAT2);


    int m_CardType = NV40;
    switch(m_CardType)
    {
      case NV40:
      case NV45:
        m_pRegs->write32(0x83280fff, NVACC_NV40_WHAT0);
        m_pRegs->write32(0x000000a0, NVACC_NV40_WHAT1);
        m_pRegs->write32(0x0078e366, NVACC_NV40_WHAT2);
        m_pRegs->write32(0x0000014c, NVACC_NV40_WHAT3);
        break;
    }

    m_pRegs->write32(0x2ffff800, NVACC_NV10_TIL3PT);
    m_pRegs->write32(0x00006000, NVACC_NV10_TIL3ST);
    m_pRegs->write32(0x01000000, NVACC_NV4X_WHAT1);
    // Engine data source DMA instance = $1140
    m_pRegs->write32(0x00001140, NVACC_NV4X_DMA_SRC);
    break;
  }

  case NV30A:
    // Init some function blocks, but most is unknown.
    m_pRegs->write32(0x40108700, NVACC_DEBUG1);
    m_pRegs->write32(0x00140000, NVACC_NV25_WHAT1);
    m_pRegs->write32(0xf00e0431, NVACC_DEBUG3);
    m_pRegs->write32(0x00008000, NVACC_NV10_DEBUG4);
    m_pRegs->write32(0xf04b1f36, NVACC_NV25_WHAT0);
    m_pRegs->write32(0x1002d888, NVACC_NV20_WHAT3);
    m_pRegs->write32(0x62ff007f, NVACC_NV25_WHAT2);

    // Copy tile setup stuff.
    for (int i = 0; i < 32; i++)
    {
      // Copy NV10_FBTIL0AD upto/including NV10_FBTIL7ST
      m_pRegs->write32(m_pRegs->read32(NVACC_NV10_FBTIL0AD + (i<<2)), NVACC_NV20_WHAT0 + (i<<2));
      m_pRegs->write32(m_pRegs->read32(NVACC_NV10_FBTIL0AD + (i<<2)), NVACC_NV20_2_WHAT0 + (i<<2));
    }

    // Copy some RAM configuration info(?)
    m_pRegs->write32(m_pRegs->read32(NV32_PFB_CONFIG_0), NVACC_NV20_WHAT_T0);
    m_pRegs->write32(m_pRegs->read32(NV32_PFB_CONFIG_1), NVACC_NV20_WHAT_T1);

    m_pRegs->write32(0x00ea0000, NVACC_RDI_INDEX);
    m_pRegs->write32(m_pRegs->read32(NV32_PFB_CONFIG_0), NVACC_RDI_DATA);

    m_pRegs->write32(0x00ea0004, NVACC_RDI_INDEX);
    m_pRegs->write32(m_pRegs->read32(NV32_PFB_CONFIG_1), NVACC_RDI_DATA);

    // Setup location of active screen in framebuffer.
    m_pRegs->write32(0, NVACC_NV20_OFFSET0);
    m_pRegs->write32(0, NVACC_NV20_OFFSET1);
    // Setup accessible card memory range.
    m_pRegs->write32(m_nRamSize-1, NVACC_NV20_BLIMIT6);
    m_pRegs->write32(m_nRamSize-1, NVACC_NV20_BLIMIT7);

    m_pRegs->write32(0x00000000, NVACC_NV10_TIL2AD);
    m_pRegs->write32(0xffffffff, NVACC_NV10_TIL0ED);
    break;
  }
}

