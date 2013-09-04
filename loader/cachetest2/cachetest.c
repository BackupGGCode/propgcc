#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <propeller.h>

#ifndef TESTDRIVER
#define TESTDRIVER      binary_spi_sram_xmem_dat_start
#endif

#define INC_PIN     8
#define MOSI_PIN    9
#define MISO_PIN    10
#define CLK_PIN     11
#define CLR_PIN     25

#define SRAM1_ADDR  1
#define SRAM2_ADDR  2

#define CS_CLR_PIN_MASK     0x01    // if is set, then byte aa contains the CS or C3-style CLR pin number
#define INC_PIN_MASK        0x02    // if is set, then byte bb contains the C3-style INC pin number
#define MUX_START_BIT_MASK  0x04    // if is set, then byte bb contains the starting bit number of the mux field
#define MUX_WIDTH_MASK      0x08    // if is set, then byte cc contains the width of the mux field
#define ADDR_MASK           0x10    // if is set, then byte dd contains either the C3-style address or the value to write to the mux field
#define QUAD_SPI_HACK_MASK  0x20    // if is set, assume that pins miso+1 and miso+2 are /WP and /HOLD and assert them

#define XMEM_CONFIG1   ((MOSI_PIN << 24) | (MISO_PIN << 16) | (CLK_PIN << 8) | CS_CLR_PIN_MASK | INC_PIN_MASK | ADDR_MASK)
#define XMEM_CONFIG2   ((CLR_PIN << 24) | (INC_PIN << 16) | SRAM1_ADDR)
#define XMEM_CONFIG3   0
#define XMEM_CONFIG4   0

#define BUF_SIZE        1024

/* for the c3 */
#define LED_PIN         15
#define LED_MASK        (1 << LED_PIN)
#define LED_ON()        (OUTA &= ~LED_MASK)
#define LED_OFF()       (OUTA |= LED_MASK)

/* cache driver initialization structure */
typedef struct {
  void *mboxes;             // mailbox array terminated with XMEM_END
  uint32_t config1;         // driver-specific parameter
  uint32_t config2;         // driver-specific parameter
  uint32_t config3;         // driver-specific parameter
  uint32_t config4;         // driver-specific parameter
} xmem_init_t;

#define XMEM_WRITE          0x8
#define XMEM_SIZE_16        0x1
#define XMEM_SIZE_32        0x2
#define XMEM_SIZE_64        0x3
#define XMEM_SIZE_128       0x4
#define XMEM_SIZE_256       0x5
#define XMEM_SIZE_512       0x6
#define XMEM_SIZE_1024      0x7
#define XMEM_END            0x8

/* cache driver communications mailbox */
typedef struct {
  volatile uint32_t hubaddr;
  volatile uint32_t extaddr;
} xmem_mbox_t;

/* external memory base addresses */
#define RAM_BASE            0x20000000
#define ROM_BASE            0x30000000

/* globals */
static xmem_mbox_t xmem_mboxes[2];
static xmem_mbox_t  *xmem_mbox = &xmem_mboxes[0];
static uint8_t padded_buf[BUF_SIZE + 15];

int xmemStart(void *code, xmem_mbox_t *mboxes, int count, uint32_t config1, uint32_t config2, uint32_t config3, uint32_t config4);
void readBlock(uint32_t extaddr, void *buf, uint32_t count);
void writeBlock(uint32_t extaddr, void *buf, uint32_t count);

int main(void)
{
    extern char TESTDRIVER[];
    uint32_t *buf = (uint32_t *)(((uint32_t)padded_buf + 15) & ~15);
    int cogn;
    
    cogn = xmemStart(TESTDRIVER, xmem_mboxes, 1, XMEM_CONFIG1, XMEM_CONFIG2, XMEM_CONFIG3, XMEM_CONFIG4);
    
    printf("mbox        %08x\n", (uint32_t)&xmem_mbox);
    printf("padded buf  %08x\n", (uint32_t)padded_buf);
    printf("buf         %08x\n", (uint32_t)buf);
    printf("cog         %d\n", cogn);
    
#if 0

{
    int i, j;
    
    printf("Flash test\n");
    
    srand(CNT);
    j = rand();
    printf("Starting with %08x\n", j);
    for (i = 0; i < BUF_SIZE; ++i)
        buf[i] = j++;
    eraseFlashBlock(0);
    writeFlashBlock(0, buf, sizeof(buf));
    memset(buf, 0, sizeof(buf));
    readBlock(ROM_BASE, buf, sizeof(buf));
    for (i = 0; i < BUF_SIZE; ++i)
        printf("buf[%d] = %08x\n", i, buf[i]);

    printf("done\n");
}

#endif

#if 1

{
    int i;

    printf("Small RAM test\n");
    
    for (i = 0; i < 32; ++i)
        buf[i] = 0xbeef0000 + i;
    writeBlock(RAM_BASE, buf, XMEM_SIZE_128);
    
    memset(buf, 0, sizeof(buf));
    
    readBlock(RAM_BASE, buf, XMEM_SIZE_128);
    for (i = 0; i < 32; ++i)
        printf("buf[%d] = %08x\n", i, buf[i]);

    printf("done\n");
}

#endif
        
#if 1

{
    uint32_t addr;
    int i, j;
    
    printf("Big RAM test\n");

    printf("Filling RAM\n");
    addr = RAM_BASE;
    for (j = 0; j < 8; ++j) {
        uint32_t startAddr = addr;
        for (i = 0; i < 256; ++i, addr += sizeof(uint32_t))
            buf[i] = addr;
        printf("Writing %08x\n", startAddr);
        writeBlock(startAddr, buf, XMEM_SIZE_1024);
    }
    
    printf("Checking RAM\n");
    addr = RAM_BASE;
    for (j = 0; j < 8; ++j) {
        printf("Reading %08x\n", addr);
        readBlock(addr, buf, XMEM_SIZE_1024);
        for (i = 0; i < 256; ++i, addr += sizeof(uint32_t))
            if (buf[i] != addr)
                printf("%08x: expected %08x, found %08x\n", addr, addr, buf[i]);
    }

    printf("done\n");
}

#endif

#if 0

{
    int i;
    
    printf("cache test\n");
    
    for (i = 0; i < 32*1024; i += sizeof(uint32_t))
        writeLong(RAM_BASE + i, i);
        
    for (i = 0; i < 32*1024; i += sizeof(uint32_t)) {
        uint32_t data = readLong(RAM_BASE + i);
        if (data != i)
            printf("%08x: expected %08x, found %08x\n", RAM_BASE + i, i, data);
    }

    printf("done\n");
}
    
#endif

    return 0;
}

int xmemStart(void *code, xmem_mbox_t *mboxes, int count, uint32_t config1, uint32_t config2, uint32_t config3, uint32_t config4)
{
    xmem_init_t params;
    int cogn;
    
    memset(mboxes, 0, sizeof(xmem_mbox_t) * count);
    mboxes[count].hubaddr = XMEM_END;
    
    params.mboxes = mboxes;
    params.config1 = config1;
    params.config2 = config2;
    params.config3 = config3;
    params.config4 = config4;
    cogn = cognew(code, &params);
    
    while (params.mboxes)
        ;

    return cogn;
}

void readBlock(uint32_t extaddr, void *buf, uint32_t size)
{
    xmem_mbox->extaddr = extaddr;
    xmem_mbox->hubaddr = (uint32_t)buf | size;
    //LED_ON();
    while (xmem_mbox->hubaddr)
        ;
    //LED_OFF();
    //printf("read returned %08x\n", xmem_mbox->addr);
    //usleep(1);
}

void writeBlock(uint32_t extaddr, void *buf, uint32_t size)
{
    xmem_mbox->extaddr = extaddr;
    xmem_mbox->hubaddr = (uint32_t)buf | XMEM_WRITE | size;
    //LED_ON();
    while (xmem_mbox->hubaddr)
        ;
    //LED_OFF();
    //printf("read returned %08x\n", xmem_mbox->addr);
    //usleep(1);
}
