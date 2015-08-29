SECTIONS
{
    .text :
    {
        . = ALIGN(4096);
        *(.text*)
    }

    .data :
    {
        . = ALIGN(4096);
        start_ctors = .;
        *(.init_array*)
        end_ctors = .;

        . = ALIGN(4096);
        start_module_ctors = .;
        *(.module_ctors*)
        end_module_ctors = .;

        start_modinfo = .;
        *(.modinfo*);
        end_modinfo = .;
    }

    .bss :
    {
        . = ALIGN(4096);
        *(.asm.bss*)
        *(.bss*)
    }

    .end :
    {
        . = ALIGN(4096);
        end = .; _end = .; __end = .;
    }
}

/* Inserts our linker scripts before the default script's .init */
INSERT BEFORE .init;
