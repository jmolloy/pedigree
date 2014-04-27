/*
 * 
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
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

#ifdef X86

#include <types.h>
#include <test.h>

#include <stdio.h>
#include <string.h>
#include <alloca.h>

#include <iostream>

using namespace std;

#include <graphics/Graphics.h>

PedigreeGraphics::Framebuffer *pRootFramebuffer = 0;
PedigreeGraphics::Framebuffer *pFramebuffer = 0;

int main(int argc, char **argv)
{
    printf("Userspace graphics framework test!\n");
    
    // Connects to the display automatically
    pRootFramebuffer = new PedigreeGraphics::Framebuffer();
    pFramebuffer = pRootFramebuffer->createChild(64, 64, 128, 128);
    
    // Check for a valid framebuffer (will return null on error)
    if(!pFramebuffer->getRawBuffer())
    {
        printf("Couldn't get a valid framebuffer.\n");
        return 0;
    }
    
    // Draw a red rectangle
    pFramebuffer->rect(0, 0, 128, 128, 0xff0000, PedigreeGraphics::Bits24_Rgb);
    pFramebuffer->redraw(0, 0, 128, 128, true);
    
    printf("Should now be a red square on the screen.\n");
    
    cout << "C++ output for the win!" << endl;
    
    return 0;
}

#else

// Architectures which don't support the native interface will pretend :)

#include <stdio.h>

int main(int argc, char **argv)
{
    printf("Boring old printf works... No native API on this architecture yet");
    return 0;
}

#endif
