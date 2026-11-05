/* Illustrative flash/RAM addresses, to be replaced for the actual board. */
ENTRY(_start)
MEMORY {
  ROM (rx) : ORIGIN = 0x10000, LENGTH = 1M
  RAM (rw) : ORIGIN = 0x200000, LENGTH = 1M
}
PHDRS { text PT_LOAD FLAGS(5); data PT_LOAD FLAGS(6); }
SECTIONS {
  .text : { KEEP(*(.text.start)) *(.text .text.*) } > ROM :text
  .rodata : { *(.rodata .rodata.*) } > ROM :text
  .preinit_array ALIGN(4) : {
    __preinit_array_start = .; KEEP(*(.preinit_array))
    __preinit_array_end = .;
  } > ROM :text
  .init_array ALIGN(4) : {
    __init_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*))) KEEP(*(.init_array))
    __init_array_end = .;
  } > ROM :text
  .fini_array ALIGN(4) : {
    __fini_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*))) KEEP(*(.fini_array))
    __fini_array_end = .;
  } > ROM :text
  .data : {
    __data_start = .; *(.data .data.*) __data_end = .;
  } > RAM AT> ROM :data
  __data_load_start = LOADADDR(.data);
  .bss (NOLOAD) : {
    __bss_start = .; *(.bss .bss.*) *(COMMON) __bss_end = .;
  } > RAM :data
  .stack (NOLOAD) : {
    . = ALIGN(8); . += 16384; __stack_top = .;
  } > RAM :data
}
