/* Link.ld -- Linker script for the kernel - ensure everything goes in the */
/*            Correct place.  */

ENTRY(start)
SECTIONS
{
  . = 0xFFFFFFFF80000000 + SIZEOF_HEADERS;
  .init : AT(ADDR(.init) - 0xFFFFFFFF7FF00000)
  {
    . = ALIGN(4096);
    init = .; _init = .; __init = .;
    *(.init.multiboot)
    *(.init.text);
    *(.init.data);
    trampoline = .;
    *(.trampoline.text);
    trampoline_end = .;
    . = ALIGN(4096);
  }

  .text : AT(ADDR(.text) - 0xFFFFFFFF7FF00000)
  {
    . = ALIGN(4096);
    code = .; _code = .; __code = .;
    *(.text*)
    . = ALIGN(4096);
  }

  .data : AT(ADDR(.data) - 0xFFFFFFFF7FF00000)
  {
    . = ALIGN(4096);
    start_ctors = .;
    *(.ctors)
    end_ctors = .;
    
    start_module_ctors = .;
    *(.module_ctors)
    end_module_ctors = .;

    data = .; _data = .; __data = .;
    *(.data)
    *(.rodata*)
    . = ALIGN(4096);
    
    start_modinfo = .;
    *(.modinfo)
    end_modinfo = .;
  }


  .bss : AT(ADDR(.bss) - 0xFFFFFFFF7FF00000)
  {
    . = ALIGN(4096);
    bss = .; _bss = .; __bss = .;
    *(.asm.bss)
    *(.bss*)
    . = ALIGN(4096);
  }
  
  .mptrampoline : AT(ADDR(.mptrampoline) - 0xFFFFFFFF7FF00000)
  {
    . = ALIGN(4);
    
    trampoline16 = .;
    *(.trampoline.text16)
    trampoline16_end = .;
    
    . = ALIGN(4);
    
    trampoline32 = .;
    *(.trampoline.text32)
    trampoline32_end = .;
    
    . = ALIGN(4);
    
    trampolinegdtr = .;
    *(.trampoline.data.gdtr)
    trampolinegdtr_end = .;
    
    . = ALIGN(4);
    
    trampolinegdt = .;
    *(.trampoline.data.gdt)
    trampolinegdt_end = .;
  }

  /DISCARD/ : { *(.comment) *(.eh_frame) *(.debug_abbrev) *(.debug_info) *(.debug_line) *(.debug_loc) *(.debug_pubnames) *(.debug_aranges) *(.debug_str) *(.debug_macinfo) *(.debug_ranges) }

  . = ALIGN(4096);
  end = .; _end = .; __end = .;
}
