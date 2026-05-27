
/* Highest address of the user mode stack */
STACK_ADDRESS = ORIGIN(RAM) + LENGTH(RAM); /* end of RAM */

/* Required amount of stack for interrupt stack (Main stack) */
MAIN_STACK_SIZE = 1024;

/* Maximum bootloader code size */
BOOTLOADER_SIZE = 0;

/* Minimum Heap Size (indicative) */
MIN_HEAP_SIZE = 0;

/* Highest heap address */
HEAP_ADDRESS = STACK_ADDRESS - MAIN_STACK_SIZE;

/* Specify the memory areas */
MEMORY
{
  CODE_FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 64K
  DATA_FLASH (r)       : ORIGIN = 0x08010000, LENGTH = 64K
  RAM (xrw)            : ORIGIN = 0x20000000, LENGTH = 16K
}

PROVIDE(__code_flash_start = ORIGIN(CODE_FLASH));
PROVIDE(__data_flash_start = ORIGIN(DATA_FLASH));
PROVIDE(__data_flash_end   = ORIGIN(DATA_FLASH) + LENGTH(DATA_FLASH));

REGION_ALIAS("REGION_BOOTLOADER",   CODE_FLASH);
REGION_ALIAS("REGION_ISR_VECT",     CODE_FLASH);
REGION_ALIAS("REGION_TEXT",         CODE_FLASH);
REGION_ALIAS("REGION_TEXT_STORAGE", CODE_FLASH);

REGION_ALIAS("REGION_DATA", RAM);
REGION_ALIAS("REGION_DATA_STORAGE", DATA_FLASH);
REGION_ALIAS("REGION_BSS", RAM);
REGION_ALIAS("REGION_RAM", RAM);
