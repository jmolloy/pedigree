
#ifndef _SYS_FB_H
#define _SYS_FB_H

/// Commands that can be performed against the framebuffer device.
enum FbCommands
{
    PEDIGREE_FB_CMD_MIN = 0x5000,
    PEDIGREE_FB_SETMODE = 0x5000,
    PEDIGREE_FB_GETMODE = 0x5001,
    PEDIGREE_FB_REDRAW  = 0x5002,
    PEDIGREE_FB_CMD_MAX = 0x5002,
};

/// All zeroes = 'revert to text mode'.
typedef struct {
    size_t width;
    size_t height;
    size_t depth;
} pedigree_fb_modeset;

typedef struct {
    size_t width;
    size_t height;
    size_t depth;
    size_t bytes_per_pixel;
    uint32_t format;
} pedigree_fb_mode;

typedef struct {
    size_t x;
    size_t y;
    size_t w;
    size_t h;
} pedigree_fb_rect;

#endif
