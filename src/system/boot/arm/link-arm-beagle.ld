/* Link.ld -- Linker script for the bootloader - ensure everything goes in the */
/*            Correct place.  */
OUTPUT_ARCH(arm)
ENTRY(start)
SECTIONS
{
    . = 0x87000000;

    .init :
    {
        *(.init)
    }    
    
    .text :
    {
        code = .; _code = .; __code = .;
        *(.text*)
    }

    .data :
    {
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
        bss = .; _bss = .; __bss = .;
        *(.bss*)
        *(.sbss*)
    }

    end = .; _end = .; __end = .;
}
