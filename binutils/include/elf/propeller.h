/* Propeller ELF support for BFD.
   Copyright 2011 Parallax Inc.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _ELF_PROPELLER_H
#define _ELF_PROPELLER_H

/* processor specific flags for the ELF header e_flags field. */

/* file contains Propeller1 code */
#define EF_PROPELLER_PROP1      0x00000001

/* file contains Propeller2 code */
#define EF_PROPELLER_PROP2      0x00000002

/* mask for machine type */
#define EF_PROPELLER_MACH       0x0000000F

/* Flags for the st_other field, indicating information about
 * a symbol; whether it is assembled with the .cog_ram flag set,
 * or whether it was assembled with .compress on in effect.
 */
#define PROPELLER_OTHER_COG_RAM 0x80
#define PROPELLER_OTHER_COMPRESSED 0x40
#define PROPELLER_OTHER_FLAGS (0xC0)

#include "elf/reloc-macros.h"

/* Relocation types.  */
START_RELOC_NUMBERS (elf_propeller_reloc_type)
  RELOC_NUMBER (R_PROPELLER_NONE, 0)
  RELOC_NUMBER (R_PROPELLER_32, 1)
  RELOC_NUMBER (R_PROPELLER_23, 2)
  RELOC_NUMBER (R_PROPELLER_16, 3)
  RELOC_NUMBER (R_PROPELLER_8, 4)
  RELOC_NUMBER (R_PROPELLER_SRC_IMM, 5)
  RELOC_NUMBER (R_PROPELLER_SRC, 6)
  RELOC_NUMBER (R_PROPELLER_DST, 7)
  RELOC_NUMBER (R_PROPELLER_PCREL10, 8)
END_RELOC_NUMBERS (R_PROPELLER_max)

#endif /* _ELF_PROPELLER_H */
