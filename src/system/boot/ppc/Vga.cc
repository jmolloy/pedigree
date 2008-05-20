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

#include "Vga.h"
#include "prom.h"
#include "ppc_font.c"

const int width = 800;
const int height = 600;
const int bytes_per_pixel = 4;
//extern char *ppc_font;

extern void* prom_screen;

#define MAKE_16(r,g,b)  ( ((r&0x1f)<<11) | ((g&0x1f)<<6) | (b&0x1f) )

void vga_putchar(char c, int x, int y, unsigned short f, unsigned short b, unsigned short *buf);

void vga_init()
{
  unsigned int addr;
  prom_getprop(prom_screen, "address", (void*)&addr, 4);
//  writeHex(addr);

  // Set device depth here!

  prom_map(addr, 0xb0000000, 0x01000000);
  unsigned short *p = (unsigned short *)0xb0000000;
  for(int i = 0; i < width*height; i ++)
  {
    p[i] = MAKE_16(0,0,0);
  }
  for (int i = 0; i < 0x7f; i++)
  {
    vga_putchar((char)i, (i*8)%width, 16*((i*8)/width), MAKE_16(i, 0xff, 0x7f-i), MAKE_16(0x00, 0x00, 0x00), p);
  }
}

#define FONT_HEIGHT 16
void vga_putchar(char c, int x, int y, unsigned short f, unsigned short b, unsigned short *buf)
{
  int idx = ((int)c) * 16;
  for (int i = 0; i < 16; i++)
  {
    unsigned char row = ppc_font[idx+i];
    for (int j = 0; j < 8; j++)
    {
      unsigned short col;
      if ( (row & (0x80 >> j)) != 0 )
      {
        col = f;
      }
      else
      {
        col = b;
      }
      buf[y*width + i*width + x + j] = col;
    }
  }

}
