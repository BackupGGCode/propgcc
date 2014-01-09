/* db_types.h - type definitions for a simple virtual machine
 *
 * Copyright (c) 2009 by David Michael Betz.  All rights reserved.
 *
 */

#ifndef __DB_TYPES_H__
#define __DB_TYPES_H__

/**********/
/* Common */
/**********/

#define VMTRUE      1
#define VMFALSE     0

/* compiler heap size */
#ifndef HEAPSIZE
#define HEAPSIZE            5000
#endif

/* size of image buffer */
#ifndef IMAGESIZE
#define IMAGESIZE           2500
#endif

/* edit buffer size */
#ifndef EDITBUFSIZE
#define EDITBUFSIZE         1500
#endif

/*********/
/* WIN32 */
/*********/

#ifdef WIN32

#include "db_inttypes.h"

#include <stdio.h>
#include <string.h>

typedef int16_t VMVALUE;
typedef uint16_t VMUVALUE;

#define ALIGN_MASK              3

#define FLASH_SPACE
#define DATA_SPACE

#define VMCODEBYTE(p)           *(uint8_t *)(p)
#define VMINTRINSIC(i)          Intrinsics[i]

#define ANSI_FILE_IO

#endif  // WIN32

/*****************/
/* MAC and LINUX */
/*****************/

#if defined(MAC) || defined(LINUX)

#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef int16_t VMVALUE;
typedef uint16_t VMUVALUE;

#define ALIGN_MASK              3

#define FLASH_SPACE
#define DATA_SPACE

#define VMCODEBYTE(p)           *(uint8_t *)(p)
#define VMINTRINSIC(i)          Intrinsics[i]

#define ANSI_FILE_IO

#endif  // MAC

/**********/
/* XGSPIC */
/**********/

#ifdef XGSPIC

//#include "FSIO.h"

#define XGS_COMMON
#define PIC_COMMON

#endif  // XGSPIC

/****************/
/* CHAMELEONPIC */
/****************/

#ifdef CHAMELEONPIC

#define PIC_COMMON

#endif  // CHAMELEONPIC

/**************/
/* PIC common */
/**************/

#ifdef PIC_COMMON

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <libpic30.h>
#include "ebasic_pic24/MDD File System/FSIO.h"

typedef int16_t VMVALUE;
typedef uint16_t VMUVALUE;
typedef FSFILE VMFILE;

struct VMDIR {
    SearchRec rec;
    int first;
};

struct VMDIRENT {
    char name[FILE_NAME_SIZE_8P3 + 2];
};

#ifdef PIC24
#include <p24hxxxx.h>
#endif
#ifdef dsPIC33
#include <p33fxxxx.h>
#endif

#define ALIGN_MASK              1

#define FLASH_SPACE             const
#define DATA_SPACE              __attribute__((far))
#define VMCODEBYTE(p)           *(p)
#define VMINTRINSIC(i)          Intrinsics[i]

#endif  // PIC_COMMON

/*********************/
/* PIC32 STARTER KIT */
/*********************/

#ifdef PIC32_STARTER_KIT

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

typedef int16_t VMVALUE;
typedef uint16_t VMUVALUE;

#define ALIGN_MASK              3

#define FLASH_SPACE             const
#define DATA_SPACE

#define VMCODEBYTE(p)           *(p)
#define VMINTRINSIC(i)          Intrinsics[i]

#define ANSI_FILE_IO

#endif  // PIC32

/**********/
/* XGSAVR */
/**********/

#ifdef XGSAVR

#define XGS_COMMON
#define AVR_COMMON

#endif  // XGSAVR

/****************/
/* CHAMELEONAVR */
/****************/

#ifdef CHAMELEONAVR

#define AVR_COMMON

#endif  // XGSAVR

/**********/
/* AVR328 */
/**********/

#ifdef AVR328

#define AVR_COMMON

#endif  // AVR328

/**********/
/* UZEBOX */
/**********/

#ifdef UZEBOX

#define AVR_COMMON

#endif  // UZEBOX

/**************/
/* AVR common */
/**************/

#ifdef AVR_COMMON

#include <stdint.h>
#include <string.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

typedef int16_t VMVALUE;
typedef uint16_t VMUVALUE;

#define ALIGN_MASK              1

#define FLASH_SPACE             PROGMEM
#define DATA_SPACE

#define VMCODEBYTE(p)           pgm_read_byte(p)
#define VMINTRINSIC(i)          ((IntrinsicFcn *)pgm_read_word(&Intrinsics[i]))

#define ANSI_FILE_IO

#endif  // AVR

/*****************/
/* PROPELLER_CAT */
/*****************/

#ifdef PROPELLER_CAT

#include <string.h>
#include <stdint.h>

/* this gets around warnings generated by Catalina when NULL is assigned to function pointer variables */
#undef NULL
#define NULL 0

int strcasecmp(const char *s1, const char *s2);

#define FLASH_SPACE
#define DATA_SPACE

#define VMCODEBYTE(p)           *(uint8_t *)(p)
#define VMINTRINSIC(i)          Intrinsics[i]

#define PROPELLER
#define ANSI_FILE_IO

#endif  // PROPELLER_CAT

/*****************/
/* PROPELLER_ZOG */
/*****************/

#ifdef PROPELLER_ZOG

#include <string.h>
#include "db_inttypes.h"

typedef int16_t VMVALUE;
typedef uint16_t VMUVALUE;

int strcasecmp(const char *s1, const char *s2);

#define FLASH_SPACE
#define DATA_SPACE

#define VMCODEBYTE(p)           *(uint8_t *)(p)
#define VMINTRINSIC(i)          Intrinsics[i]

#define PROPELLER
#define ANSI_FILE_IO

#endif  // PROPELLER_ZOG

/*****************/
/* PROPELLER_GCC */
/*****************/

#ifdef PROPELLER_GCC

#include <string.h>
#include <stdint.h>

int strcasecmp(const char *s1, const char *s2);

#define FLASH_SPACE
#define DATA_SPACE

#define VMCODEBYTE(p)           *(uint8_t *)(p)
#define VMINTRINSIC(i)          Intrinsics[i]

#define PROPELLER
#define ANSI_FILE_IO

#endif  // PROPELLER_GCC

/*****************/
/* PROPELLER_GCC */
/*****************/

#ifdef PROPELLER

typedef int32_t VMVALUE;
typedef uint32_t VMUVALUE;

#define ALIGN_MASK              3

#endif

/****************/
/* ANSI_FILE_IO */
/****************/

#ifdef ANSI_FILE_IO

#include <stdio.h>
#include <dirent.h>

typedef FILE VMFILE;

#define VM_fopen	fopen
#define VM_fclose	fclose
#define VM_fgets	fgets
#define VM_fputs	fputs

struct VMDIR {
    DIR *dirp;
};

struct VMDIRENT {
    char name[FILENAME_MAX];
};

#endif // ANSI_FILE_IO

#endif
