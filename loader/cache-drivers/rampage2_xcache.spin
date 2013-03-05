CON

  ' SPI pins
  SIO0_PIN              = 0
  SCK_PIN               = 8
  CS_PIN                = 9
  
#define FLASH
#undef RW
#include "cache_common.spin"

'----------------------------------------------------------------------------------------------------
'
' init - initialize the memory functions
'
'----------------------------------------------------------------------------------------------------

init
        ' set the pin directions
        call    #release
                
        ' initialize the flash
        call    #sst_read_jedec_id
        cmp     t1, jedec_id_1 wz
  if_nz jmp     #:next
        cmp     t2, jedec_id_2 wz
  if_nz jmp     #:next
        cmp     t3, jedec_id_3 wz
  if_z  jmp     #:unprot
:next   call    #read_jedec_id
        cmp     t1, jedec_id wz
  if_nz jmp     #halt
        cmp     t2, jedec_id wz
  if_nz jmp     #halt
        call    #select
        mov     data, sst_quadmode
        call    #spiSendByte
        call    #release
        call    #sst_read_jedec_id
        cmp     t1, jedec_id_1 wz
  if_nz jmp     #halt
        cmp     t2, jedec_id_2 wz
  if_nz jmp     #halt
        cmp     t3, jedec_id_3 wz
  if_nz jmp     #halt
:unprot call    #sst_write_enable
        mov     cmd, sst_wrblkprot
        call    #sst_start_sqi_cmd_1
        andn    outa, sck_mask
        mov     data, #0
        call    #sst_sqi_write_word
        call    #sst_sqi_write_word
        call    #sst_sqi_write_word
        call    #sst_sqi_write_word
        call    #sst_sqi_write_word
        call    #sst_sqi_write_word
        call    #release
init_ret
        ret
        
halt    jmp     #halt

'----------------------------------------------------------------------------------------------------
'
' BREAD - read data from external memory
'
' on input:
'   vmaddr is the virtual memory address to read
'   hubaddr is the hub memory address to write
'   count is the number of longs to read
'
'----------------------------------------------------------------------------------------------------

BREAD
        mov     cmd, vmaddr
        and     cmd, flashmask
        shr     cmd, #1
        or      cmd, sst_read
        mov     bytes, #4
        mov     dbytes, count
        call    #sst_sqi_read
BREAD_RET
        ret

'----------------------------------------------------------------------------------------------------
'
' erase_4k_block
'
' on input:
'   vmaddr is the virtual memory address to erase
'
'----------------------------------------------------------------------------------------------------

erase_4k_block
        shr     vmaddr, #1
        test    vmaddr, mask_4k wz
 if_nz  jmp     erase_4k_block_ret
        call    #sst_write_enable
        mov     cmd, vmaddr
        and     cmd, flashmask
        or      cmd, ferase4kblk
        mov     bytes, #4
        call    #sst_start_sqi_cmd
        call    #release
        call    #wait_until_done
erase_4k_block_ret
        ret

mask_4k long    $00000fff

'----------------------------------------------------------------------------------------------------
'
' write_block
'
' on input:
'   vmaddr is the virtual memory address to write
'   hubaddr is the hub memory address to read
'   count is the number of bytes to write
'
'----------------------------------------------------------------------------------------------------

write_block
        add     count, #1               ' round to an even number of bytes
        andn    count, #1
        mov     t1, vmaddr
        add     t1, #256
        andn    t1, #255
        sub     t1, vmaddr
        max     t1, count
wloop   call    #sst_write_enable
        mov     cmd, vmaddr
        and     cmd, flashmask
        shr     cmd, #1
        or      cmd, sst_program
        mov     bytes, #4
        mov     dbytes, t1
        call    #sst_sqi_write
        call    #wait_until_done
        sub     count, t1 wz
write_block_ret
 if_z   ret
        add     vmaddr, t1
        mov     t1, count
        max     t1, #256
        jmp     #wloop
                
read_jedec_id
        call    #select
        mov     data, frdjedecid
        call    #spiSendByte
        call    #spiRecvByte
        mov     t1, data_hi
        shl     t1, #8
        mov     t2, data_lo
        shl     t2, #8
        call    #spiRecvByte
        or      t1, data_hi
        shl     t1, #8
        or      t2, data_lo
        shl     t2, #8
        call    #spiRecvByte
        or      t1, data_hi
        or      t2, data_lo
        call    #release
read_jedec_id_ret
        ret
        
sst_read_jedec_id
        mov     cmd, sst_rdjedecid
        call    #sst_start_sqi_cmd_1
        andn    dira, sio_mask
        andn    outa, sck_mask
        call    #sst_sqi_read_word
        mov     t1, data
        call    #sst_sqi_read_word
        mov     t2, data
        call    #sst_sqi_read_word
        mov     t3, data
        call    #release
sst_read_jedec_id_ret
        ret
        
sst_write_enable
        mov     cmd, fwrenable
        call    #sst_start_sqi_cmd_1
        call    #release
sst_write_enable_ret
        ret

sst_sqi_write_word
        mov     sio_t1, data
        shr     sio_t1, #8
        shl     sio_t1, sio_shift
        and     sio_t1, sio_mask
        andn    outa, sio_mask
        or      outa, sio_t1
        or      outa, sck_mask
        andn    outa, sck_mask
        shl     data, sio_shift
        and     data, sio_mask
        andn    outa, sio_mask
        or      outa, data
        or      outa, sck_mask
        andn    outa, sck_mask
sst_sqi_write_word_ret
        ret

sst_sqi_write
        call    #sst_start_sqi_cmd
        andn    outa, sck_mask
        tjz     dbytes, #:done
:loop   rdbyte  data, hubaddr
        add     hubaddr, #1
        shl     data, sio_shift
        andn    outa, sio_mask
        or      outa, data
        or      outa, sck_mask
        andn    outa, sck_mask
        djnz    dbytes, #:loop
:done   call    #release
sst_sqi_write_ret
        ret

sst_sqi_read_word
        or      outa, sck_mask
        mov     data, ina
        andn    outa, sck_mask
        and     data, sio_mask
        shr     data, sio_shift
        shl     data, #8
        or      outa, sck_mask
        mov     sio_t1, ina
        andn    outa, sck_mask
        and     sio_t1, sio_mask
        shr     sio_t1, sio_shift
        or      data, sio_t1
sst_sqi_read_word_ret
        ret

sst_sqi_read
        call    #sst_start_sqi_cmd
        andn    outa, sck_mask
        andn    outa, sio_mask
        or      outa, sck_mask  ' hi dummy nibble
        andn    outa, sck_mask
        or      outa, sck_mask  ' lo dummy nibble
        andn    dira, sio_mask
        andn    outa, sck_mask
        tjz     dbytes, #:done
:loop   or      outa, sck_mask
        mov     data, ina
        andn    outa, sck_mask
        shr     data, sio_shift
        wrbyte  data, hubaddr
        add     hubaddr, #1
        djnz    dbytes, #:loop
:done   call    #release
sst_sqi_read_ret
        ret

sst_start_sqi_cmd_1
        mov     bytes, #1
        
sst_start_sqi_cmd
        or      dira, sio_mask      ' set data pins to outputs
        call    #select             ' select the chip
:loop   rol     cmd, #8
        mov     sio_t1, cmd         ' send the high nibble
        and     sio_t1, #$f0
        shl     sio_t1, sio_shift
        mov     sio_t2, sio_t1
        shr     sio_t2, #4
        or      sio_t1, sio_t2
        andn    outa, sio_mask
        or      outa, sio_t1
        or      outa, sck_mask
        andn    outa, sck_mask
        mov     sio_t1, cmd         ' send the low nibble
        and     sio_t1, #$0f
        shl     sio_t1, sio_shift
        mov     sio_t2, sio_t1
        shl     sio_t2, #4
        or      sio_t1, sio_t2
        andn    outa, sio_mask
        or      outa, sio_t1
        or      outa, sck_mask
        cmp     bytes, #1 wz
 if_nz  andn    outa, sck_mask
        djnz    bytes, #:loop
sst_start_sqi_cmd_1_ret
sst_start_sqi_cmd_ret
        ret
        
wait_until_done
        mov     cmd, frdstatus
        call    #sst_start_sqi_cmd_1
        andn    dira, sio_mask
        andn    outa, sck_mask
:wait   call    #sst_sqi_read_word
        test    data, busy_bits wz
  if_nz jmp     #:wait
        call    #release
wait_until_done_ret
        ret

sio_t1          long    0
sio_t2          long    0
busy_bits       long    $8800

jedec_id            long    $00bf2601    ' SST26VF016
jedec_id_1          long    $bbff
jedec_id_2          long    $2266
jedec_id_3          long    $0011

sst_rdjedecid       long    $af000000    ' read the manufacturers id, device type and device id
sst_quadmode        long    $38          ' enable quad mode
sst_wrblkprot       long    $42000000    ' write block protect register
sst_program         long    $02000000    ' flash program byte/page
sst_read            long    $0b000000    ' flash read command

frdjedecid          long    $9f          ' read the manufacturers id, device type and device id
ferase4kblk         long    $20000000    ' flash erase a 4k block
frdstatus           long    $05000000    ' flash read status
fwrenable           long    $06000000    ' flash write enable

'----------------------------------------------------------------------------------------------------
' SPI routines
'----------------------------------------------------------------------------------------------------

select
        andn    outa, cs_mask
select_ret
        ret

release
        mov     outa, pinout
        mov     dira, pindir
release_ret
        ret
        
spiSendByte
        shl     data, #24
        mov     sio_t1, #8
:loop   rol     data, #1 wc
        muxc    outa, mosi_lo_mask
        muxc    outa, mosi_hi_mask
        or      outa, sck_mask
        andn    outa, sck_mask
        djnz    sio_t1, #:loop
spiSendByte_ret
        ret

spiRecvByte
        mov     data_lo, #0
        mov     data_hi, #0
        mov     sio_t1, #8
:loop   or      outa, sck_mask
        test    miso_lo_mask, ina wc
        rcl     data_lo, #1
        test    miso_hi_mask, ina wc
        rcl     data_hi, #1
        andn    outa, sck_mask
        djnz    sio_t1, #:loop
spiRecvByte_ret
        ret

' mosi_lo, mosi_hi, sck, cs
pindir          long    (1 << SIO0_PIN) | (1 << (SIO0_PIN + 4)) | (1 << SCK_PIN) | (1 << CS_PIN)

' mosi_lo, mosi_hi, cs
pinout          long    (1 << SIO0_PIN) | (1 << (SIO0_PIN + 4)) | (1 << CS_PIN)

mosi_lo_mask    long    1 << SIO0_PIN
miso_lo_mask    long    2 << SIO0_PIN
mosi_hi_mask    long    1 << (SIO0_PIN + 4)
miso_hi_mask    long    2 << (SIO0_PIN + 4)
sck_mask        long    1 << SCK_PIN
cs_mask         long    1 << CS_PIN
sio_mask        long    $ff << SIO0_PIN
sio_shift       long    SIO0_PIN

' variables used by the spi send/receive functions
cmd             long    0
bytes           long    0
dbytes          long    0
data            long    0
data_lo         long    0
data_hi         long    0

flashmask       long    $00ffffff       ' mask to isolate the flash offset bits

        fit     496             ' out of 496
