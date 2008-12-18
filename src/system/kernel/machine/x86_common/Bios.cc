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
#include <machine/x86_common/Bios.h>
#include <processor/Processor.h>
#include <utilities/StaticString.h>

#include "../../core/BootIO.h"
#include <x86emu.h>

extern BootIO bootIO;

Bios Bios::m_Instance;

u8 rdb (u32 addr)
{
  return  * (u8*)(addr);
}
u16 rdw (u32 addr)
{
  return * (u16*)(addr);
}
u32 rdl (u32 addr)
{
  return * (u32*)(addr);
}
void wrb (u32 addr, u8 val)
{
  * (u8*) (addr) = val;
}
void wrw (u32 addr, u16 val)
{
 * (u16*) (addr) = val;
}
void wrl (u32 addr, u32 val)
{
  * (u32*) (addr) = val;
}


u8 inb (X86EMU_pioAddr addr)
{
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a" (ret) : "dN" (addr));
  return ret;
}
u16 inw (X86EMU_pioAddr addr)
{
  uint16_t ret;
  asm volatile("inw %1, %0" : "=a" (ret) : "dN" (addr));
  return ret;
}
u32 inl (X86EMU_pioAddr addr)
{
  uint32_t ret;
  asm volatile("inl %1, %0" : "=a" (ret) : "dN" (addr));
  return ret;
}
void outb (X86EMU_pioAddr addr, u8 val)
{
  asm volatile ("outb %1, %0" : : "dN" (addr), "a" (val));
}
void outw (X86EMU_pioAddr addr, u16 val)
{
  asm volatile ("outw %1, %0" : : "dN" (addr), "a" (val));
}
void outl (X86EMU_pioAddr addr, u32 val)
{
  asm volatile ("outl %1, %0" : : "dN" (addr), "a" (val));
}

extern "C" int abs (int i)
{
  return (i>0)?i:-i;
}

extern "C" int exit (int code)
{
  for(;;);
}

extern "C" void sscanf()
{}

extern "C" void printk(const char *fmt, ...)
{
  HugeStaticString buf2;
  char buf[1024];

  va_list args;
  int i;

  va_start(args, fmt);
  i = vsprintf(buf,fmt,args);
  va_end(args);

  buf[i] = '\0';
  buf2.clear();
  buf2 += buf;

  bootIO.write(buf2, BootIO::White, BootIO::Black);
}

Bios::Bios () : mallocLoc(0x8000)
{
  X86EMU_memFuncs mf;
  mf.rdb = &rdb;
  mf.rdw = &rdw;
  mf.rdl = &rdl;
  mf.wrb = &wrb;
  mf.wrw = &wrw;
  mf.wrl = &wrl;
  X86EMU_pioFuncs iof;
  iof.inb = &inb;
  iof.inw = &inw;
  iof.inl = &inl;
  iof.outb = &outb;
  iof.outw = &outw;
  iof.outl = &outl;

  memset(&M, 0, sizeof(M));
  M.x86.debug = 0|DEBUG_MEM_TRACE_F|DEBUG_DECODE_F;
  M.x86.mode = 0;
  memset((void*)0x7C00, 0xF4, 0x100);

  X86EMU_setupMemFuncs(&mf);
  X86EMU_setupPioFuncs(&iof);
  M.x86.R_SS = 0x0000;
  M.x86.R_SP = 0x7F00;
  M.x86.R_IP = 0x7C00; // Set IP to 0x7C00 as there are 0xFF's there which will halt the emulation.
  M.x86.R_CS = 0x0000;
}

Bios::~Bios ()
{
}

uintptr_t Bios::malloc (int n)
{
  uintptr_t loc = mallocLoc;
  mallocLoc += n;
  return loc;
}

void Bios::executeInterrupt (int i)
{
  X86EMU_prepareForInt(i);
  X86EMU_exec();
}

void Bios::setAx (int n)
{
  M.x86.R_AX = n;
}
void Bios::setBx (int n)
{
  M.x86.R_BX = n;
}
void Bios::setCx (int n)
{
  M.x86.R_CX = n;
}
void Bios::setDx (int n)
{
  M.x86.R_DX = n;
}
void Bios::setDi (int n)
{
  M.x86.R_DI = n;
}
void Bios::setEs (int n)
{
  M.x86.R_ES = n;
}

int Bios::getAx ()
{
  return M.x86.R_AX;
}
int Bios::getBx ()
{
  return M.x86.R_BX;
}
int Bios::getCx ()
{
  return M.x86.R_CX;
}
int Bios::getDx ()
{
  return M.x86.R_DX;
}
int Bios::getDi ()
{
  return M.x86.R_DI;
}
int Bios::getEs ()
{
  return M.x86.R_ES;
}
