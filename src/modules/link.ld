/* Link.ld -- Linker script for the kernel - ensure everything goes in the */
/*            Correct place.  */

SECTIONS
{
  /DISCARD/ : {
    *(.text.crt0*);
  }

  . = ALIGN(4096);
  .text :
  {
    *(.text*)
    *(.init*)
    *(.fini*)
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
    *(.init_array*)
    *(.ctors*)
    end_ctors = .;
  }
  
  . = ALIGN(4096);
  .module_dtors :
  {
    start_dtors = .;
    *(.fini_array*)
    *(.dtors*)
    end_dtors = .;
  }
}
