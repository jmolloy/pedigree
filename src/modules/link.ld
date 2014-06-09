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
  
  . = ALIGN(4096);
  .module_dtors :
  {
    start_dtors = .;
    *(.dtors)
    end_dtors = .;

  }
}
