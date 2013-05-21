/* libload.ld: linker script for the libload.so userspace dynamic linker  */

ENTRY(_libload_main)

PHDRS
{
    headers PT_PHDR PHDRS ;
    text PT_LOAD FILEHDR PHDRS ;
    data PT_LOAD ;
    dynamic PT_DYNAMIC ;
}

SECTIONS
{
    /* Fit in just beneath the userspace heap. */
    . = 0x4FA00000;
    __elf_start = .;

    . += SIZEOF_HEADERS;

    .hash : { *(.hash); } :text

    .dynsym : { *(.dynsym*); } :text
    .dynstr : { *(.dynstr*); } :text

    .rel.plt : { *(.rel.plt*); } :text
    .rel.dyn : { *(.rel.dyn*); } :text

    .init : { *(.init*); } :text

    .fini : { *(.fini*); } :text

    .plt : { *(.plt*); } :text

    . = ALIGN(4096);
    .text : { *(.text*); } :text

    .rodata : { *(.rodata*); } :text

    .eh_frame : { *(.eh_frame*); } :text

    .ctors : { *(.ctors*); } :data
    .dtors : { *(.dtors*); } :data

    .dynamic : { *(.dynamic*); } :data :dynamic

    .got.plt : { *(.got.plt*); } :data

    .data : { *(.data*); } :data

    /* .bss page aligned - allows the loader to map it in. */
    . = ALIGN(4096);
    __start_bss = .;
    .bss : { *(.bss*); } :data
    __end_bss = .;

    /DISCARD/ : { *(.comment); *(.interp) }
}
