SECTIONS
{
  . = 0x400000;

  .text :
  {
    . = ALIGN(4096);
    code = .; _code = .; __code = .;
    *(.text*)
  }

  .data :
  {
    . = ALIGN(4096);
    start_ctors = .;    
    *(.ctors*)
    end_ctors = .;

    __init_array_start = .;
    *(.init_array*)
    __init_array_end = .;

    . = ALIGN(4096);
    __fini_array_start = .;    
    *(.fini_array*)
    __fini_array_end = .;
    
    . = ALIGN(4096);
    start_module_ctors = .;
    *(.module_ctors*)
    end_module_ctors = .;

    data = .; _data = .; __data = .;
    *(.data*)
    *(.rodata*)

    start_modinfo = .;
    *(.modinfo*);
    end_modinfo = .;
  }

  .bss :
  {
    . = ALIGN(4096);
    bss = .; _bss = .; __bss = .;
    *(.asm.bss*)
    *(.bss*)
    . = ALIGN(4096);
  }

  .gnu.hash :
  {
    *(.gnu.hash*)
  }

  .dynstr :
  {
    *(.dynstr*)
  }

  .dynsym :
  {
    *(.dynsym*)
  }

  .gnu.version_r :
  {
    *(.gnu.version_r*)
  }

  .gnu.version :
  {
    *(.gnu.version*)
  }

  .catchall :
  {
    *(.*)
  }

  . = ALIGN(4096);
  end = .; _end = .; __end = .;
}
