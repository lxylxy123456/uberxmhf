














OUTPUT_ARCH(i386:x86-64)






ENTRY(_sl_start)
MEMORY
{
  low (rwxai) : ORIGIN = 0, LENGTH = 64K
  high (rwxai) : ORIGIN = 64K, LENGTH = 1984K
  debug (rwxai) : ORIGIN = 0, LENGTH = 1024M
  unaccounted (rwxai) : ORIGIN = 0, LENGTH = 0
}


SECTIONS
{
  . = 0x00000000;
  .sl_header : {
    *(.sl_header)
    . = ALIGN(4096);
  } >low

  .text : {
    *(.text)
    *(.text.*)
    *(.rel.text)
    *(.rel.text.*)
  } >low

  .data : {
    *(.data)
    *(.data.*)
    *(.rel.data)
    *(.rel.data.*)
  } >low

  .rodata : {
    *(.rodata)
    *(.rodata.*)
    *(.rel.rodata)
    *(.rel.rodata.*)
  } >low

  .bss : {
    *(.bss)
    *(.bss.*)
    *(.rel.bss)
    *(.rel.bss.*)

    . = ALIGN(16);
  } >low






  .sl_stack : {
    *(.sl_stack)
    . = ALIGN(0x10000);
  } >low




  .sl_untrusted_params : {
    *(.sl_untrusted_params)
    *(.comment)
    . = ALIGN(4096);

    *(.sl_long_pt)
    . = ALIGN(0x200000);
  } >high




  /DISCARD/ : {
    *(.eh_frame)
    *(.note.gnu.property)
    *(.gnu.build.attributes)
  } >low


  .debug_abbrev : { *(.debug_abbrev) } >debug
  .debug_aranges : { *(.debug_aranges) } >debug
  .debug_info : { *(.debug_info) } >debug
  .debug_line : { *(.debug_line) } >debug
  .debug_line_str : { *(.debug_line_str) } >debug
  .debug_loc : { *(.debug_loc) } >debug
  .debug_loclists : { *(.debug_loclists) } >debug
  .debug_ranges : { *(.debug_ranges) } >debug
  .debug_rnglists : { *(.debug_rnglists) } >debug
  .debug_str : { *(.debug_str) } >debug
  .rel.debug_info : { *(.rel.debug_info) } >debug
  .rel.debug_aranges : { *(.rel.debug_aranges) } >debug
  .rel.debug_line : { *(.rel.debug_line) } >debug







  .unaccounted : {
    *(*)
  } >unaccounted
}

