/* Link.ld -- Linker script for modules */

SECTIONS
{
  .text : {*(.text.*)}
  .module_ctors :
  {
    *(.ctors)
  }

  /DISCARD/ : { *(.comment) *(.eh_frame) *(.debug_abbrev) *(.debug_info) *(.debug_line) *(.debug_loc) *(.debug_pubnames) *(.debug_aranges) *(.debug_str) *(.debug_macinfo) *(.debug_ranges) *(.stab) *(.stabstr) }
}
