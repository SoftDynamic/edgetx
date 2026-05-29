/*
   Generic firmware linker script for GD32
*/

INCLUDE definitions.ld

/* Define output sections */
SECTIONS
{
  
  /* ISR vector to be loaded */
  .isr_vector :
  {
    . = ALIGN(4);
    _sisr_vector = .;
    KEEP(*(.isr_vector))

    . = ALIGN(4);
    _eisr_vector = .;
  } > REGION_ISR_VECT AT> REGION_TEXT_STORAGE

  _isr_load = LOADADDR(.isr_vector);

  /* Main code section - executable code only, in fast zero-wait flash */
  .text (READONLY) :
  {
    FILL(0xFFFF)
    CREATE_OBJECT_SYMBOLS

    _stext = .;
    KEEP(*(.fwversiondata))

    INCLUDE common_text.ld

    . = ALIGN(4);
    _etext = .;
  } > REGION_TEXT AT> REGION_TEXT_STORAGE

  _text_load = LOADADDR(.text);

  /* Read-only data in slower flash (wait states)
     String tables, font data, enum tables, etc. */
  .rodata (READONLY) :
  {
    . = ALIGN(4);
    _srodata = .;
    *(.rodata .rodata* .gnu.linkonce.r.*)
    . = ALIGN(4);
    _erodata = .;
  } > REGION_RODATA AT> REGION_RODATA

  INCLUDE common_sections.ld

  .text_end_section : {} > REGION_TEXT AT > REGION_TEXT_STORAGE
  _firmware_length = LOADADDR(.text_end_section);
  _firmware_version = _text_load;
}
