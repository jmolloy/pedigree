/* Link.ld -- Linker script for the kernel - ensure everything goes in the */
/*            Correct place.  */
OUTPUT_ARCH(arm)
ENTRY(start)
SECTIONS
{
    . = 0x80100000;
    __start = .;
    
    .init :
    {
        . = ALIGN(4096);
        init = .; _init = .; __init = .;
        *(.init.text);
        *(.init.data);
    }
    
    .text :
    {
        . = ALIGN(4096);
        code = .; _code = .; __code = .;
        *(.text*);
    }

    .ctors :
    {
        . = ALIGN(4096);
        start_ctors = .;
        *(.init_array);
        *(.ctors);
        end_ctors = .;
    }

    .data :
    {
        
        . = ALIGN(4096);
        data = .; _data = .; __data = .;
        *(.data);
        *(.rodata*);
    }

    .bss :
    {
        . = ALIGN(4096);
        bss = .; _bss = .; __bss = .;
        *(.asm.bss);
        *(.bss*);
        *(.sbss);
    }
    
    .ivt :
    {
        *(.ivt);
    }

  /DISCARD/ : { *(.comment) *(.eh_frame) *(.debug_abbrev) *(.debug_info) *(.debug_line) *(.debug_loc) *(.debug_pubnames) *(.debug_aranges) *(.debug_str) *(.debug_macinfo) *(.debug_ranges) *(.stab) *(.stabstr) }

    . = ALIGN(4096);
    end = .; _end = .; __end = .;
}
