/* Link.ld -- Linker script for the kernel - ensure everything goes in the */
/*            Correct place.  */

SECTIONS
{
  . = ALIGN(4096);
  .text :
  {
    *(.text*)
  }
  
  . = ALIGN(4096);
  .data :
  {
    *(.data*)
  }
  
  . = ALIGN(4096);
  .bss :
  {
    *(.bss*)
  }
  
  . = ALIGN(4096);
  .rodata :
  {
    *(.rodata*)
  }
  
  . = ALIGN(4096);
  .module_ctors :
  {
    start_ctors = .;
    *(.ctors)
    end_ctors = .;

  }

  /DISCARD/ : { *(.comment) *(.eh_frame) *(.debug_abbrev) *(.debug_info) *(.debug_line) *(.debug_loc) *(.debug_pubnames) *(.debug_aranges) *(.debug_str) *(.debug_macinfo) *(.debug_ranges) *(.stab) *(.stabstr) }
}
