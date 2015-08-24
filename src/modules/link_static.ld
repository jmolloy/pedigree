/* Link.ld -- Linker script for modules */

SECTIONS
{
  .text : {*(.text*)}
  .data : {*(.data*)}
  .rodata : {*(.rodata*)}
  .bss : {*(.bss*)}

  .module_ctors :
  {
    *(.init_array*)
    *(.ctors*)
  }

  /* We do a strip to create a real kernel binary, so don't wipe out debug
   * data from the kernel binary here. */
  /DISCARD/ : { *(.comment) *(.eh_frame) }
}
