/* Link.ld -- Linker script for the kernel - ensure everything goes in the */
/*            Correct place.  */
ENTRY(start)
SECTIONS
{
    . = 0xDF000000; /* Must be within 24 bits of the kernel */
    .sdr1_trampoline :
    {
      *(.sdr1_trampoline)
    }

    . = 0xe0000000;
    .text :
    {
        code = .; _code = .; __code = .;
        *(.text*)
        . = ALIGN(4096);
    }

    .data :
    {
        start_ctors = .;
        *(.ctors)
        end_ctors = .;
        

        data = .; _data = .; __data = .;
        *(.data)
        *(.rodata*)
        . = ALIGN(4096);
        __strtab = .;
        *(.strtab)
        __strtab_end = .;
    }

    .bss :
    {
        bss = .; _bss = .; __bss = .;
        *(.bss)
        *(.sbss)
        . = ALIGN(4096);
    }

    /DISCARD/ : { *(.comment) *(.eh_frame) *(.debug_abbrev) *(.debug_info) *(.debug_line) *(.debug_loc) *(.debug_pubnames) *(.debug_aranges) *(.debug_str) *(.debug_macinfo) *(.debug_ranges) *(.gnu.attributes) *(.stab) *(.stabstr) }

    end = .; _end = .; __end = .;
}
