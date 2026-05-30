
/* Highest address of the user mode stack */
STACK_ADDRESS = ORIGIN(RAM) + LENGTH(RAM); /* end of RAM */

/* Required amount of stack for interrupt stack (Main stack) */
MAIN_STACK_SIZE = 112;

/* Maximum bootloader code size */
BOOTLOADER_SIZE = 0;

/* Minimum Heap Size (indicative) */
MIN_HEAP_SIZE = 0;

/* Highest heap address */
HEAP_ADDRESS = STACK_ADDRESS - MAIN_STACK_SIZE;

/* Specify the memory areas */
/* GD32F3x0: first 64K has zero-wait flash access, second 64K requires wait states
   FIRMWARE_FLASH = 64K fast + 48K slow = 112K for firmware code + rodata + data LMA */
MEMORY
{
  FIRMWARE_FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 124K
  STORAGE_FLASH (rw)  : ORIGIN = 0x0801F000, LENGTH = 4K
  RAM (xrw)           : ORIGIN = 0x20000000, LENGTH = 16K
}

PROVIDE(__code_flash_start = ORIGIN(FIRMWARE_FLASH));
PROVIDE(__data_flash_start = ORIGIN(STORAGE_FLASH));
PROVIDE(__data_flash_end   = ORIGIN(STORAGE_FLASH) + LENGTH(STORAGE_FLASH));
PROVIDE(__storage_flash_size = LENGTH(STORAGE_FLASH));

REGION_ALIAS("REGION_BOOTLOADER",   FIRMWARE_FLASH);
REGION_ALIAS("REGION_ISR_VECT",     FIRMWARE_FLASH);
REGION_ALIAS("REGION_TEXT",         FIRMWARE_FLASH);
REGION_ALIAS("REGION_TEXT_STORAGE", FIRMWARE_FLASH);
REGION_ALIAS("REGION_RODATA",       FIRMWARE_FLASH);

REGION_ALIAS("REGION_DATA", RAM);
REGION_ALIAS("REGION_DATA_STORAGE", FIRMWARE_FLASH);
REGION_ALIAS("REGION_BSS", RAM);
REGION_ALIAS("REGION_RAM", RAM);
