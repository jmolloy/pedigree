#ifdef X86

#include <types.h>
#include <test.h>

#include <stdio.h>

#include <graphics/Graphics.h>

PedigreeGraphics::Framebuffer *pFramebuffer = 0;

int main(int argc, char **argv)
{
    printf("Userspace graphics framework test!\n");
    
    // Connects to the display automatically
    pFramebuffer = new PedigreeGraphics::Framebuffer();
    
    // Check for a valid framebuffer (will return null on error)
    if(!pFramebuffer->getRawBuffer())
    {
        printf("Couldn't get a valid framebuffer.\n");
        return 0;
    }
    
    // Draw a red rectangle
    pFramebuffer->rect(64, 64, 128, 128, 0xff0000, PedigreeGraphics::Bits24_Rgb);
    pFramebuffer->redraw(64, 64, 128, 128);
    
    printf("Should now be a red square on the screen.");
    
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
