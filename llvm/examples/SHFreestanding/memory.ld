/* Illustrative ELF load map for the QEMU user harness, not a board memory map. */
ENTRY(_start)
MEMORY {
  ROM (rx) : ORIGIN = 0x10000, LENGTH = 1M
  RAM (rw) : ORIGIN = 0x200000, LENGTH = 1M
}
/* Keep text and read-only data in one load segment. Separate segments sharing
   a page can otherwise replace that page's execute permission in a loader. */
PHDRS {
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
}
SECTIONS {
  .text : { KEEP(*(.text.start)) *(.text .text.*) } > ROM :text
  .rodata : { *(.rodata .rodata.*) } > ROM :text
  .preinit_array ALIGN(4) : {
    __preinit_array_start = .;
    KEEP(*(.preinit_array))
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
  .data : { *(.data .data.*) } > RAM :data
  .bss (NOLOAD) : { *(.bss .bss.*) *(COMMON) } > RAM :data
}
