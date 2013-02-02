/* libload.ld: linker script for the libload.so userspace dynamic linker  */

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
    . = 0x4FF00000 + SIZEOF_HEADERS;

    .hash : { *(.hash); } :text

    .dynsym : { *(.dynsym*); } :text
    .dynstr : { *(.dynstr*); } :text

    .rel.plt : { *(.rel.plt*); } :text
    .rel.dyn : { *(.rel.dyn*); } :text

    .init : { *(.init*); } :text

    .plt : { *(.plt*); } :text

    .text : { *(.text*); } :text

    .fini : { *(.fini*); } :text

    .rodata : { *(.rodata*); } :text

    .eh_frame : { *(.eh_frame*); } :text

    .ctors : { *(.ctors*); } :data
    .dtors : { *(.dtors*); } :data

    .dynamic : { *(.dynamic*); } :data :dynamic

    .got.plt : { *(.got.plt*); } :data

    .data : { *(.data*); } :data

    .bss : { *(.bss*); } :data

    /DISCARD/ : { *(.comment); *(.interp) }
}
