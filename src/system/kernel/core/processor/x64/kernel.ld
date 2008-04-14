/* Link.ld -- Linker script for the kernel - ensure everything goes in the */
/*            Correct place.  */

ENTRY(start)
SECTIONS
{

    . = 0xFFFFFFFF80000000 + SIZEOF_HEADERS;
    .text : AT(ADDR(.text) - 0xFFFFFFFF7FF00000)
    {
        . = ALIGN(4096);
        code = .; _code = .; __code = .;
        *(.bootstrap)
        *(.text)
    }

    .data : AT(ADDR(.data) - 0xFFFFFFFF7FF00000)
    {
        . = ALIGN(4096);
        start_ctors = .;
        *(.ctors)
        end_ctors = .;
        

        data = .; _data = .; __data = .;
        *(.data)
        *(.rodata)
    }

    .bss : AT(ADDR(.bss) - 0xFFFFFFFF7FF00000)
    {
        . = ALIGN(4096);
        bss = .; _bss = .; __bss = .;
        *(.bss)
    }

    . = ALIGN(4096);
    end = .; _end = .; __end = .;
}
