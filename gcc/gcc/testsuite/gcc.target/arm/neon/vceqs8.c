/* Test the `vceqs8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vceqs8 (void)
{
  uint8x8_t out_uint8x8_t;
  int8x8_t arg0_int8x8_t;
  int8x8_t arg1_int8x8_t;

  out_uint8x8_t = vceq_s8 (arg0_int8x8_t, arg1_int8x8_t);
}

/* { dg-final { scan-assembler "vceq\.i8\[ 	\]+\[dD\]\[0-9\]+, \[dD\]\[0-9\]+, \[dD\]\[0-9\]+!?\(\[ 	\]+@\[a-zA-Z0-9 \]+\)?\n" } } */
/* { dg-final { cleanup-saved-temps } } */
