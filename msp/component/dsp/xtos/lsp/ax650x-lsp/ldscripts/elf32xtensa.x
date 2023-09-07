/* This linker script generated from xt-genldscripts.tpp for LSP lsp/ax650x-lsp */
/* Linker Script for default link */
MEMORY
{
  iram0_0_seg :                       	org = 0x00000000, len = 0x8000
  dram0_0_seg :                       	org = 0x18500000, len = 0x40000
  dram1_0_seg :                       	org = 0x18540000, len = 0x3C000
  rambypass_0_seg :                   	org = 0x20000000, len = 0xF00000
}

PHDRS
{
  iram0_0_phdr PT_LOAD;
  dram0_0_phdr PT_LOAD;
  dram0_0_bss_phdr PT_LOAD;
  dram1_0_phdr PT_LOAD;
  dram1_0_bss_phdr PT_LOAD;
  dram1_stack_phdr PT_LOAD;
  rambypass_0_phdr PT_LOAD;
  rambypass_0_bss_phdr PT_LOAD;
}


/*  Default entry point:  */
ENTRY(_ResetVector)


/*  Memory boundary addresses:  */
_memmap_mem_iram0_start = 0x0;
_memmap_mem_iram0_end   = 0x8000;
_memmap_mem_dram0_start = 0x18500000;
_memmap_mem_dram0_end   = 0x18540000;
_memmap_mem_dram1_start = 0x18540000;
_memmap_mem_dram1_end   = 0x1857c000;
_memmap_mem_dram1_stack_start = 0x1857c000;
_memmap_mem_dram1_stack_end   = 0x18580000;

/*  Memory segment boundary addresses:  */
_memmap_seg_iram0_0_start = 0x0;
_memmap_seg_iram0_0_max   = 0x8000;
_memmap_seg_dram0_0_start = 0x18500000;
_memmap_seg_dram0_0_max   = 0x18540000;
_memmap_seg_dram1_0_start = 0x18540000;
_memmap_seg_dram1_0_max   = 0x1857c000;
_memmap_seg_rambypass_0_start = 0x20000000;
_memmap_seg_rambypass_0_max   = 0x20f00000;

_rom_store_table = 0;
PROVIDE(_memmap_reset_vector = 0x0);
PROVIDE(_memmap_vecbase_reset = 0x0);
/* Various memory-map dependent cache attribute settings: */
_memmap_cacheattr_wb_base = 0x00000014;
_memmap_cacheattr_wt_base = 0x00000034;
_memmap_cacheattr_bp_base = 0x00000044;
_memmap_cacheattr_unused_mask = 0xFFFFFF00;
_memmap_cacheattr_wb_trapnull = 0x44444414;
_memmap_cacheattr_wba_trapnull = 0x44444414;
_memmap_cacheattr_wbna_trapnull = 0x44444424;
_memmap_cacheattr_wt_trapnull = 0x44444434;
_memmap_cacheattr_bp_trapnull = 0x44444444;
_memmap_cacheattr_wb_strict = 0x00000014;
_memmap_cacheattr_wt_strict = 0x00000034;
_memmap_cacheattr_bp_strict = 0x00000044;
_memmap_cacheattr_wb_allvalid = 0x44444414;
_memmap_cacheattr_wt_allvalid = 0x44444434;
_memmap_cacheattr_bp_allvalid = 0x44444444;
_memmap_region_map = 0x00000003;
PROVIDE(_memmap_cacheattr_reset = _memmap_cacheattr_wb_trapnull);

SECTIONS
{

  .DispatchVector.text : ALIGN(4)
  {
    _DispatchVector_text_start = ABSOLUTE(.);
    KEEP (*(.DispatchVector.text))
    . = ALIGN (4);
    _DispatchVector_text_end = ABSOLUTE(.);
  } >iram0_0_seg :iram0_0_phdr

  .ResetVector.text : ALIGN(4)
  {
    _ResetVector_text_start = ABSOLUTE(.);
    *(.ResetVector.text)
    . = ALIGN (4);
    _ResetVector_text_end = ABSOLUTE(.);
  } >iram0_0_seg :iram0_0_phdr

  .ResetHandler.text : ALIGN(4)
  {
    _ResetHandler_text_start = ABSOLUTE(.);
    *(.ResetHandler.literal .ResetHandler.text)
    . = ALIGN (4);
    _ResetHandler_text_end = ABSOLUTE(.);
  } >iram0_0_seg :iram0_0_phdr

  .DispatchHandler.text : ALIGN(4)
  {
    _DispatchHandler_text_start = ABSOLUTE(.);
    *(.DispatchHandler.literal .DispatchHandler.text)
    . = ALIGN (4);
    _DispatchHandler_text_end = ABSOLUTE(.);
  } >iram0_0_seg :iram0_0_phdr

  .iram0.text : ALIGN(4)
  {
    _iram0_text_start = ABSOLUTE(.);
    *(.iram0.literal .iram.literal .iram.text.literal .iram0.text .iram.text)
    . = ALIGN (4);
    _iram0_text_end = ABSOLUTE(.);
    _memmap_seg_iram0_0_end = ALIGN(0x8);
  } >iram0_0_seg :iram0_0_phdr

  _itb_default = 0x0;
  _memmap_mem_iram0_max = ABSOLUTE(.);

  .dram0.rodata : ALIGN(4)
  {
    _dram0_rodata_start = ABSOLUTE(.);
    *(.dram0.rodata)
    . = ALIGN (4);
    _dram0_rodata_end = ABSOLUTE(.);
  } >dram0_0_seg :dram0_0_phdr

  .dram0.data : ALIGN(4)
  {
    _dram0_data_start = ABSOLUTE(.);
    *(.dram0.data)
    . = ALIGN (4);
    _dram0_data_end = ABSOLUTE(.);
  } >dram0_0_seg :dram0_0_phdr

  .dram0.bss (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (8);
    _dram0_bss_start = ABSOLUTE(.);
    *(.dram0.bss)
    . = ALIGN (8);
    _dram0_bss_end = ABSOLUTE(.);
    _memmap_seg_dram0_0_end = ALIGN(0x8);
  } >dram0_0_seg :dram0_0_bss_phdr

  _memmap_mem_dram0_max = ABSOLUTE(.);

  .dram1.rodata : ALIGN(4)
  {
    _dram1_rodata_start = ABSOLUTE(.);
    *(.dram1.rodata)
    . = ALIGN (4);
    _dram1_rodata_end = ABSOLUTE(.);
  } >dram1_0_seg :dram1_0_phdr

  .dram1.data : ALIGN(4)
  {
    _dram1_data_start = ABSOLUTE(.);
    *(.dram1.data)
    . = ALIGN (4);
    _dram1_data_end = ABSOLUTE(.);
  } >dram1_0_seg :dram1_0_phdr

  .dram1.bss (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (8);
    _dram1_bss_start = ABSOLUTE(.);
    *(.dram1.bss)
    . = ALIGN (8);
    _dram1_bss_end = ABSOLUTE(.);
    _memmap_seg_dram1_0_end = ALIGN(0x8);
  } >dram1_0_seg :dram1_0_bss_phdr

  _memmap_mem_dram1_max = ABSOLUTE(.);

  _stack_sentry = 0x1857c000;
  PROVIDE(__stack = 0x18580000);
  _isb_default = 0x1857fb50;
  _memmap_mem_dram1_stack_max = ABSOLUTE(.);

  .sram.rodata : ALIGN(4)
  {
    _sram_rodata_start = ABSOLUTE(.);
    KEEP (*(.sram.rodata))
    . = ALIGN (4);
    _sram_rodata_end = ABSOLUTE(.);
  } >rambypass_0_seg :rambypass_0_phdr

  .rodata : ALIGN(4)
  {
    _rodata_start = ABSOLUTE(.);
    *(.rodata)
    *(SORT(.rodata.sort.*))
    KEEP (*(SORT(.rodata.keepsort.*) .rodata.keep.*))
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    *(.rodata1)
    __XT_EXCEPTION_TABLE__ = ABSOLUTE(.);
    KEEP (*(.xt_except_table))
    KEEP (*(.gcc_except_table))
    *(.gnu.linkonce.e.*)
    *(.gnu.version_r)
    KEEP (*(.eh_frame))
    /*  C++ constructor and destructor tables, properly ordered:  */
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    /*  C++ exception handlers table:  */
    __XT_EXCEPTION_DESCS__ = ABSOLUTE(.);
    *(.xt_except_desc)
    *(.gnu.linkonce.h.*)
    __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
    *(.xt_except_desc_end)
    *(.dynamic)
    *(.gnu.version_d)
    . = ALIGN(4);		/* this table MUST be 4-byte aligned */
    _bss_table_start = ABSOLUTE(.);
    LONG(_dram0_bss_start)
    LONG(_dram0_bss_end)
    LONG(_dram1_bss_start)
    LONG(_dram1_bss_end)
    LONG(_bss_start)
    LONG(_bss_end)
    _bss_table_end = ABSOLUTE(.);
    . = ALIGN (4);
    _rodata_end = ABSOLUTE(.);
  } >rambypass_0_seg :rambypass_0_phdr

  .sram.text : ALIGN(4)
  {
    _sram_text_start = ABSOLUTE(.);
    *(.sram.literal .sram.text)
    . = ALIGN (4);
    _sram_text_end = ABSOLUTE(.);
  } >rambypass_0_seg :rambypass_0_phdr

  .text : ALIGN(4)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);
    *(.entry.text)
    *(.init.literal)
    KEEP(*(.init))
    *(.literal.sort.* SORT(.text.sort.*))
    KEEP (*(.literal.keepsort.* SORT(.text.keepsort.*) .literal.keep.* .text.keep.* .literal.*personality* .text.*personality*))
    *(.literal .text .literal.* .text.* .stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.fini.literal)
    KEEP(*(.fini))
    *(.gnu.version)
    . = ALIGN (4);
    _text_end = ABSOLUTE(.);
    _etext = .;
  } >rambypass_0_seg :rambypass_0_phdr

  .clib.percpu.data : ALIGN(4)
  {
    _clib_percpu_data_start = ABSOLUTE(.);
    *(.clib.percpu.data)
    . = ALIGN (4);
    _clib_percpu_data_end = ABSOLUTE(.);
  } >rambypass_0_seg :rambypass_0_phdr

  .rtos.percpu.data : ALIGN(4)
  {
    _rtos_percpu_data_start = ABSOLUTE(.);
    *(.rtos.percpu.data)
    . = ALIGN (4);
    _rtos_percpu_data_end = ABSOLUTE(.);
  } >rambypass_0_seg :rambypass_0_phdr

  .rtos.data : ALIGN(4)
  {
    _rtos_data_start = ABSOLUTE(.);
    *(.rtos.data)
    . = ALIGN (4);
    _rtos_data_end = ABSOLUTE(.);
  } >rambypass_0_seg :rambypass_0_phdr

  .sram.data : ALIGN(4)
  {
    _sram_data_start = ABSOLUTE(.);
    *(.sram.data)
    . = ALIGN (4);
    _sram_data_end = ABSOLUTE(.);
  } >rambypass_0_seg :rambypass_0_phdr

  .data : ALIGN(4)
  {
    _data_start = ABSOLUTE(.);
    *(.data)
    *(SORT(.data.sort.*))
    KEEP (*(SORT(.data.keepsort.*) .data.keep.*))
    *(.data.*)
    *(.gnu.linkonce.d.*)
    KEEP(*(.gnu.linkonce.d.*personality*))
    *(.data1)
    *(.sdata)
    *(.sdata.*)
    *(.gnu.linkonce.s.*)
    *(.sdata2)
    *(.sdata2.*)
    *(.gnu.linkonce.s2.*)
    KEEP(*(.jcr))
    *(__llvm_prf_cnts)
    *(__llvm_prf_data)
    *(__llvm_prf_vnds)
    . = ALIGN (4);
    _data_end = ABSOLUTE(.);
  } >rambypass_0_seg :rambypass_0_phdr

  .clib.data : ALIGN(4)
  {
    _clib_data_start = ABSOLUTE(.);
    *(.clib.data)
    . = ALIGN (4);
    _clib_data_end = ABSOLUTE(.);
  } >rambypass_0_seg :rambypass_0_phdr

  .clib.rodata : ALIGN(4)
  {
    _clib_rodata_start = ABSOLUTE(.);
    *(.clib.rodata)
    . = ALIGN (4);
    _clib_rodata_end = ABSOLUTE(.);
  } >rambypass_0_seg :rambypass_0_phdr

  .rtos.rodata : ALIGN(4)
  {
    _rtos_rodata_start = ABSOLUTE(.);
    *(.rtos.rodata)
    . = ALIGN (4);
    _rtos_rodata_end = ABSOLUTE(.);
  } >rambypass_0_seg :rambypass_0_phdr

  .clib.text : ALIGN(4)
  {
    _clib_text_start = ABSOLUTE(.);
    *(.clib.literal .clib.text)
    . = ALIGN (4);
    _clib_text_end = ABSOLUTE(.);
  } >rambypass_0_seg :rambypass_0_phdr

  .rtos.text : ALIGN(4)
  {
    _rtos_text_start = ABSOLUTE(.);
    *(.rtos.literal .rtos.text)
    . = ALIGN (4);
    _rtos_text_end = ABSOLUTE(.);
  } >rambypass_0_seg :rambypass_0_phdr

  .bss (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (8);
    _bss_start = ABSOLUTE(.);
    *(.dynsbss)
    *(.sbss)
    *(.sbss.*)
    *(.gnu.linkonce.sb.*)
    *(.scommon)
    *(.sbss2)
    *(.sbss2.*)
    *(.gnu.linkonce.sb2.*)
    *(.dynbss)
    *(.bss)
    *(SORT(.bss.sort.*))
    KEEP (*(SORT(.bss.keepsort.*) .bss.keep.*))
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    *(__llvm_prf_names.clib.bss)
    *(.clib.percpu.bss)
    *(.clib.bss)
    *(.rtos.percpu.bss)
    *(.rtos.bss)
    *(.sram.bss)
    . = ALIGN (8);
    _bss_end = ABSOLUTE(.);
    _end = ALIGN(0x8);
    PROVIDE(end = ALIGN(0x8));
    _memmap_seg_rambypass_0_end = ALIGN(0x8);
  } >rambypass_0_seg :rambypass_0_bss_phdr

  _heap_sentry = 0x20f00000;

  _memmap_mem_rambypass_max = ABSOLUTE(.);

  .debug  0 :  { *(.debug) }
  .line  0 :  { *(.line) }
  .debug_srcinfo  0 :  { *(.debug_srcinfo) }
  .debug_sfnames  0 :  { *(.debug_sfnames) }
  .debug_aranges  0 :  { *(.debug_aranges) }
  .debug_pubnames  0 :  { *(.debug_pubnames) }
  .debug_info  0 :  { *(.debug_info) }
  .debug_abbrev  0 :  { *(.debug_abbrev) }
  .debug_line  0 :  { *(.debug_line) }
  .debug_frame  0 :  { *(.debug_frame) }
  .debug_str  0 :  { *(.debug_str) }
  .debug_loc  0 :  { *(.debug_loc) }
  .debug_macinfo  0 :  { *(.debug_macinfo) }
  .debug_weaknames  0 :  { *(.debug_weaknames) }
  .debug_funcnames  0 :  { *(.debug_funcnames) }
  .debug_typenames  0 :  { *(.debug_typenames) }
  .debug_varnames  0 :  { *(.debug_varnames) }
  .xt.insn 0 :
  {
    KEEP (*(.xt.insn))
    KEEP (*(.gnu.linkonce.x.*))
  }
  .xt.prop 0 :
  {
    KEEP (*(.xt.prop))
    KEEP (*(.xt.prop.*))
    KEEP (*(.gnu.linkonce.prop.*))
  }
  .xt.lit 0 :
  {
    KEEP (*(.xt.lit))
    KEEP (*(.xt.lit.*))
    KEEP (*(.gnu.linkonce.p.*))
  }
  .debug.xt.callgraph 0 :
  {
    KEEP (*(.debug.xt.callgraph .debug.xt.callgraph.* .gnu.linkonce.xt.callgraph.*))
  }
  .note.gnu.build-id 0 :
  {
    KEEP(*(.note.gnu.build-id))
  }
}

