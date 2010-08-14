#ifdef X86

#include <types.h>
#include <test.h>

#include <stdio.h>

extern "C"
{
    int pedigree_gfx_get_provider(void *p);
    int pedigree_gfx_get_curr_mode(void *p, void *sm);
    uintptr_t pedigree_gfx_get_raw_buffer(void *p);
    int pedigree_gfx_create_buffer(void *p, void **b, void *args);
    int pedigree_gfx_destroy_buffer(void *p, void *b);
    int pedigree_gfx_convert_pixel(void *p, uint32_t *in, uint32_t *out, uint32_t infmt, uint32_t outfmt);
    void pedigree_gfx_redraw(void *p, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    void pedigree_gfx_blit(void *p, void *args);
    void pedigree_gfx_set_pixel(void *p, uint32_t x, uint32_t y, uint32_t colour, uint32_t fmt);
    void pedigree_gfx_rect(void *p, void *args);
    void pedigree_gfx_copy(void *p, void *args);
    void pedigree_gfx_line(void *p, void *args);
};

namespace Graphics
{
    enum PixelFormat
    {
        Bits32_Argb,        // Alpha + RGB, with alpha in the highest byte
        Bits32_Rgba,        // RGB + alpha, with alpha in the lowest byte
        Bits32_Rgb,         // RGB, no alpha, essentially the same as above
        
        Bits24_Rgb,         // RGB in a 24-bit pack
        Bits24_Bgr,         // R and B bytes swapped
        
        Bits16_Argb,        // 4:4:4:4 ARGB, alpha most significant nibble
        Bits16_Rgb565,      // 5:6:5 RGB
        Bits16_Rgb555,      // 5:5:5 RGB
        
        Bits8_Idx           // Index into a palette
    };
        
    inline size_t bitsPerPixel(PixelFormat format)
    {
        switch(format)
        {
            case Bits32_Argb:
            case Bits32_Rgba:
            case Bits32_Rgb:
                return 32;
            case Bits24_Rgb:
            case Bits24_Bgr:
                return 24;
            case Bits16_Argb:
            case Bits16_Rgb565:
            case Bits16_Rgb555:
                return 16;
            case Bits8_Idx:
                return 8;
            default:
                return 4;
        }
    }
    
    inline size_t bytesPerPixel(PixelFormat format)
    {
        return bitsPerPixel(format) / 8;
    }
    
    /// Creates a 24-bit RGB value (Bits24_Rgb)
    inline uint32_t createRgb(uint32_t r, uint32_t g, uint32_t b)
    {
        return (r << 16) | (g << 8) | b;
    }
    
    struct Buffer
    {
        /// Base of this buffer in memory. For internal use only.
        uintptr_t base;
        
        /// Width of the buffer in pixels
        size_t width;
        
        /// Height of the buffer in pixels
        size_t height;
        
        /// Output format of the buffer. NOT the input format. Used for
        /// byte-per-pixel calculations.
        PixelFormat format;
        
        /// Buffer ID, for easy identification within drivers
        size_t bufferId;
        
        /// Backing pointer, for drivers. Typically holds a MemoryRegion
        /// for software-only framebuffers to identify the memory's location.
        void *pBacking;
    };
    
    struct GraphicsProvider
    {
        void *pDisplay;
        
        /* Some form of hardware caps here... */
        
        void *pFramebuffer;
        
        size_t maxWidth;
        size_t maxHeight;
        size_t maxDepth;
    };
};

int main(int argc, char **argv)
{
    printf("Userspace graphics framework test!\n");
    
    Graphics::GraphicsProvider provider;
    pedigree_gfx_get_provider(&provider);
    
    struct sixargs
    {
        uint32_t a, b, c, d, e, f;
    } __attribute__((packed));
    
    sixargs whee;
    
    whee.a = 64;
    whee.b = 64;
    whee.c = 128;
    whee.d = 128;
    whee.e = 0xFFFFFF;
    whee.f = Graphics::Bits24_Rgb;
    
    pedigree_gfx_rect(&provider, &whee);
    
    pedigree_gfx_redraw(&provider, 64, 64, 128, 128);
    
    printf("Should now be a white square on the screen.");
    
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
