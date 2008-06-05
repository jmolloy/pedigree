/* Link.ld -- Linker script for the kernel - ensure everything goes in the */
/*            Correct place.  */
/*OUTPUT_ARCH(elf32ppc)*/
ENTRY(start)
SECTIONS
{

    . = 0x002000000;
    .text :
    {
        code = .; _code = .; __code = .;
        *(.text)
        . = ALIGN(4096);
    }

    .data :
    {
        start_ctors = .;
        *(.ctors)
        end_ctors = .;
        

        data = .; _data = .; __data = .;
        *(.data)
        *(.rodata)
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

    end = .; _end = .; __end = .;
}
