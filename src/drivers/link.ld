/* Link.ld -- Linker script for the kernel - ensure everything goes in the */
/*            Correct place.  */

SECTIONS
{
  .text : {*(.text.*)}

  /DISCARD/ : { *(.comment) *(.eh_frame) *(.debug_abbrev) *(.debug_info) *(.debug_line) *(.debug_loc) *(.debug_pubnames) *(.debug_aranges) *(.debug_str) *(.debug_macinfo) *(.debug_ranges) *(.stab) *(.stabstr) }
}
