SECTIONS
{
  .data :
  {
    start_ctors = .;
    *(.init_array*)
    *(.ctors*)
    end_ctors = .;

    . = ALIGN(4096);
    start_module_ctors = .;
    *(.module_ctors*)
    end_module_ctors = .;

    start_modinfo = .;
    *(.modinfo*);
    end_modinfo = .;

    __init_array_start = .;
    __init_array_end = .;
  }
}
