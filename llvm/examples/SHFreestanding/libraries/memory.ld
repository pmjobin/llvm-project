/* QEMU user test load map. A board supplies its own ROM, RAM and heap bounds. */
ENTRY(_start)
MEMORY {
  ROM (rx) : ORIGIN = 0x10000, LENGTH = 1M
  RAM (rw) : ORIGIN = 0x200000, LENGTH = 1M
}
PHDRS {
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
}
SECTIONS {
  .text : { KEEP(*(.text.start)) *(.text .text.*) } > ROM :text
  .rodata : { *(.rodata .rodata.*) } > ROM :text
  .init_array ALIGN(4) : {
    __bothinit_array_start = .;
    KEEP(*(.preinit_array))
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*))) KEEP(*(.init_array))
    __bothinit_array_end = .;
  } > ROM :text
  .fini_array ALIGN(4) : {
    __fini_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*))) KEEP(*(.fini_array))
    KEEP(*(.fini_array_onexit))
    __fini_array_end = .;
  } > ROM :text
  .data : { *(.data .data.*) } > RAM :data
  .bss (NOLOAD) : { *(.bss .bss.*) *(COMMON) } > RAM :data
  .heap (NOLOAD) : {
    . = ALIGN(8);
    __heap_start = .;
    . += 256K;
    __heap_end = .;
  } > RAM :data
}
