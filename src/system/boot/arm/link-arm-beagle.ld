/* Link.ld -- Linker script for the bootloader - ensure everything goes in the */
/*            Correct place.  */
OUTPUT_ARCH(arm)
ENTRY(start)
SECTIONS
{
    . = 0x87000000;

    .init :
    {
        . = ALIGN(4096);
        *(.init)
    }    
    
    .text :
    {
        . = ALIGN(4096);
        code = .; _code = .; __code = .;
        *(.text*)
    }

    .data :
    {
        . = ALIGN(4096);
        start_ctors = .;
        *(.ctors*)
        end_ctors = .;

        data = .; _data = .; __data = .;
        *(.data*)
        *(.rodata*)
        
    	__strtab = .;
	    *(.strtab*)
    	__strtab_end = .;
    }

    .bss :
    {
        . = ALIGN(4096);
        bss = .; _bss = .; __bss = .;
        *(.bss*)
        *(.sbss*)
        . = ALIGN(4096);
        *(.stacks*)
    }

    end = .; _end = .; __end = .;
}
