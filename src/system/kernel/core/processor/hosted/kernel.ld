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
        start_dtors = .;
        *(.fini_array*)
        end_dtors = .;

        start_modinfo = .;
        KEEP(*(.modinfo*));
        end_modinfo = .;
    }

    old_loc = .;
    .syscall (0x399000) :
    {
        start_syscall = .;
        *(.syscall*)
        end_syscall = .;
    }
    . = old_loc;

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
