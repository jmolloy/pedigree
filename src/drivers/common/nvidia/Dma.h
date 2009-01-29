

#ifndef DMA_H
#define DMA_H

#include <processor/MemoryRegion.h>
#include <processor/MemoryMappedIo.h>
#include "nv_macros.h"

/* FIFO channels */
#define NV_GENERAL_FIFO_CH0             0x0000
#define NV_GENERAL_FIFO_CH1             0x2000
#define NV_GENERAL_FIFO_CH2             0x4000
#define NV_GENERAL_FIFO_CH3             0x6000
#define NV_GENERAL_FIFO_CH4             0x8000
#define NV_GENERAL_FIFO_CH5             0xa000
#define NV_GENERAL_FIFO_CH6             0xc000
#define NV_GENERAL_FIFO_CH7             0xe000

/* sub-command offsets within FIFO channels */
#define NV_GENERAL_DMAPUT                                                       0x0040
#define NV_GENERAL_DMAGET                                                       0x0044
#define NV_ROP5_SOLID_SETROP5                                           0x0300
#define NV_IMAGE_BLACK_RECTANGLE_TOPLEFT                        0x0300
#define NV_IMAGE_PATTERN_SETCOLORFORMAT                         0x0300
#define NV_IMAGE_PATTERN_SETSHAPE                                       0x0308
#define NV_IMAGE_PATTERN_SETCOLOR0                                      0x0310
#define NV_IMAGE_BLIT_SOURCEORG                                         0x0300
#define NV4_GDI_RECTANGLE_TEXT_SETCOLORFORMAT           0x0300
#define NV4_GDI_RECTANGLE_TEXT_COLOR1A                          0x03fc
#define NV4_GDI_RECTANGLE_TEXT_UCR0_LEFTTOP                     0x0400
#define NV4_SURFACE_FORMAT                                                      0x0300
#define NV_SCALED_IMAGE_FROM_MEMORY_SETCOLORFORMAT      0x0300
#define NV_SCALED_IMAGE_FROM_MEMORY_SOURCEORG           0x0308
#define NV_SCALED_IMAGE_FROM_MEMORY_SOURCESIZE          0x0400

/* handles to pre-defined engine commands */
#define NV_ROP5_SOLID         0x00000000 /* 2D */
#define NV_IMAGE_BLACK_RECTANGLE    0x00000001 /* 2D/3D */
#define NV_IMAGE_PATTERN        0x00000002 /* 2D */
#define NV_SCALED_IMAGE_FROM_MEMORY   0x00000003 /* 2D */
#define NV_TCL_PRIMITIVE_3D       0x00000004 /* 3D */ //2007
#define NV4_SURFACE           0x00000010 /* 2D */
#define NV10_CONTEXT_SURFACES_2D    0x00000010 /* 2D */
#define NV_IMAGE_BLIT         0x00000011 /* 2D */
#define NV12_IMAGE_BLIT         0x00000011 /* 2D */
#define NV4_GDI_RECTANGLE_TEXT      0x00000012 /* 2D */
#define NV4_CONTEXT_SURFACES_ARGB_ZS  0x00000013 /* 3D */
#define NV10_CONTEXT_SURFACES_ARGB_ZS 0x00000013 /* 3D */
#define NV4_DX5_TEXTURE_TRIANGLE    0x00000014 /* 3D */
#define NV10_DX5_TEXTURE_TRIANGLE   0x00000014 /* 3D */
#define NV4_DX6_MULTI_TEXTURE_TRIANGLE  0x00000015 /* unused (yet?) */
#define NV10_DX6_MULTI_TEXTURE_TRIANGLE 0x00000015 /* unused (yet?) */
#define NV1_RENDER_SOLID_LIN      0x00000016 /* 2D: unused */

/* card_type in order of date of NV chip design */
enum NvType {
  NV04 = 0,
  NV05,
  NV05M64,
  NV06,
  NV10,
  NV11,
  NV11M,
  NV15,
  NV17,
  NV17M,
  NV18,
  NV18M,
  NV20,
  NV25,
  NV28,
  NV30,
  NV31,
  NV34,
  NV35,
  NV36,
  NV38,
  NV40,
  NV41,
  NV43,
  NV44,
  NV45,
  G70,
  G71,
  G72,
  G73
};

/* card_arch in order of date of NV chip design */
enum NvCard {
  NV04A = 0,
  NV10A,
  NV20A,
  NV30A,
  NV40A
};

class Dma
{
public:
  Dma(IoBase *pRegs, IoBase *pFb, NvCard card, NvType type, uintptr_t ramSize);
  ~Dma();
  void init();
  void screenToScreenBlit(uint16_t src_x, uint16_t src_y, uint16_t dest_x, uint16_t dest_y, uint16_t h, uint16_t w);
  void fillRectangle(uint16_t x, uint16_t y, uint16_t h, uint16_t w);

private:

  void dmaCmd(uint32_t cmd, uint32_t offset, uint16_t size);
  void writeBuffer(uint32_t arg);
  void start();
  void ensureFree(uint16_t cmd_size);
  void initFifo(uint32_t ch, uint32_t handle);
  void setUpReverseEngineeredMagicRegs();

  IoBase *m_pRegs;
  IoBase *m_pFramebuffer;
  NvCard m_Card;
  NvType m_Type;
  uintptr_t m_nRamSize;

  uint32_t m_pFifos[32];
  uint32_t m_pFifoPtrs[32];

  uintptr_t m_nDmaBuffer;
  uint32_t *m_pDmaBuffer;
  MemoryRegion m_DmaBuffer;
  uintptr_t m_nPut;
  uintptr_t m_nCurrent;
  uintptr_t m_nMax;
  uintptr_t m_nFree;
};

#endif
