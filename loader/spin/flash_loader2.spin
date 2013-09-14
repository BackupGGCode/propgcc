CON

  _clkmode = XTAL1 + PLL16X
  _clkfreq = 80000000

  hub_memory_size = 32 * 1024
  
  FLASH_BASE = $30000000
  
OBJ

  cache : "cache_interface"

PUB start | index_width, offset_width, tags_size, cache_size, cache_lines, cache_tags, xmem_mbox, sp

  index_width := p_cache_geometry >> 8
  offset_width := p_cache_geometry & $ff
  tags_size := (1 << index_width) * 4
  cache_size := 1 << (index_width + offset_width)

  cache_lines := hub_memory_size - cache_size
  cache_tags := cache_lines - tags_size
  xmem_mbox := cache_tags - cache#_MBOX2_SIZE * 4 - 4
  
  ' start the external memory driver
  cache.start2(@xmem_code, xmem_mbox, 1, p_xmem_param1, p_xmem_param2, p_xmem_param3, p_xmem_param4)

  ' setup the stack
  ' at start stack contains xmem_mbox, cache_tags, cache_lines, cache_geometry, pc
  sp := xmem_mbox
  sp -= 4
  long[sp] := FLASH_BASE
  sp -= 4
  long[sp] := p_cache_geometry
  sp -= 4
  long[sp] := cache_lines
  sp -= 4
  long[sp] := cache_tags
  sp -= 4
  long[sp] := xmem_mbox

   ' start the xmm kernel boot code
  coginit(cogid, @vm_code, sp)

DAT

' parameters filled in before downloading flash_loader2.binary
p_cache_geometry    long    0
p_xmem_param1       long    0
p_xmem_param2       long    0
p_xmem_param3       long    0
p_xmem_param4       long    0
p_vm_code_off       long    @vm_code - @p_cache_geometry
p_xmem_code_off     long    @xmem_code - @p_cache_geometry

vm_code             long    0[496]
xmem_code           long    0[496]
