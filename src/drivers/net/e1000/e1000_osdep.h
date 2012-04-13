/*******************************************************************************

  Intel PRO/1000 Linux driver
  Copyright(c) 1999 - 2008 Intel Corporation.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Linux NICS <linux.nics@intel.com>
  e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
  Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497

*******************************************************************************/

FILE_LICENCE ( GPL2_OR_LATER );

/* glue for the OS-dependent part of e1000
 * includes register access macros
 */

#ifndef _E1000_OSDEP_H_
#define _E1000_OSDEP_H_

#include "e1000_osdep_early.h"

u32 e1000_translate_register_82542(u32 reg);
#define E1000_REGISTER(a, reg) (((a)->mac.type >= e1000_82543) \
                               ? reg                           \
                               : e1000_translate_register_82542(reg))

#define E1000_WRITE_FLUSH(a) E1000_READ_REG(a, E1000_STATUS)

#define DBGF DEBUGFUNC(__func__)
#define DEBUGFUNC(F) DBG("%s\n", F)

#undef DEBUG_REGS

#define E1000_DUMP_CNTR(a, reg) e1000_dump_cntr(a, reg, #reg)
static inline u32 e1000_dump_cntr(struct e1000_hw *a, u32 reg, char *regname)
{
	u32 res = readl((a)->hw_addr + E1000_REGISTER(a, reg));
//#ifdef DEBUG_REGS
	if (res)
		dbg_printf("NON-ZERO ERROR COUNTER: %s -> 0x%08x\n", regname, res);
//#else
//	(void) regname;
//#endif
	return res;
}
#ifndef DEBUG_REGS

#define SILENT

#define E1000_WRITE_REG(a, reg, value) \
    writel((value), ((a)->hw_addr + E1000_REGISTER(a, reg)))

#define E1000_READ_REG(a, reg) (readl((a)->hw_addr + E1000_REGISTER(a, reg)))

#define E1000_WRITE_REG_ARRAY(a, reg, offset, value) \
    writel((value), ((a)->hw_addr + E1000_REGISTER(a, reg) + ((offset) << 2)))

#define E1000_READ_REG_ARRAY(a, reg, offset) ( \
    readl((a)->hw_addr + E1000_REGISTER(a, reg) + ((offset) << 2)))

#define E1000_READ_REG_ARRAY_DWORD E1000_READ_REG_ARRAY
#define E1000_WRITE_REG_ARRAY_DWORD E1000_WRITE_REG_ARRAY

#define E1000_WRITE_REG_ARRAY_WORD(a, reg, offset, value) ( \
    writew((value), ((a)->hw_addr + E1000_REGISTER(a, reg) + ((offset) << 1))))

#define E1000_READ_REG_ARRAY_WORD(a, reg, offset) ( \
    readw((a)->hw_addr + E1000_REGISTER(a, reg) + ((offset) << 1)))

#define E1000_WRITE_REG_ARRAY_BYTE(a, reg, offset, value) ( \
    writeb((value), ((a)->hw_addr + E1000_REGISTER(a, reg) + (offset))))

#define E1000_READ_REG_ARRAY_BYTE(a, reg, offset) ( \
    readb((a)->hw_addr + E1000_REGISTER(a, reg) + (offset)))

#define E1000_WRITE_REG_IO(a, reg, value) do { \
    outl(reg, ((a)->io_base));                  \
    outl(value, ((a)->io_base + 4));      } while(0)

#define E1000_WRITE_FLASH_REG(a, reg, value) ( \
    writel((value), ((a)->flash_address + reg)))

#define E1000_WRITE_FLASH_REG16(a, reg, value) ( \
    writew((value), ((a)->flash_address + reg)))

#define E1000_READ_FLASH_REG(a, reg) (readl((a)->flash_address + reg))

#define E1000_READ_FLASH_REG16(a, reg) (readw((a)->flash_address + reg))

#else

extern int e1000_dbg_reg_access;

#define SILENT for(e1000_dbg_reg_access = 0; \
		   !e1000_dbg_reg_access;    \
		   e1000_dbg_reg_access = 1)

static inline void e1000_write_reg(struct e1000_hw *a, u32 reg, u32 value, char *regname)
{
	if (e1000_dbg_reg_access) dbg_printf("%s <- 0x%08x\n", regname, value);
	writel((value), ((a)->hw_addr + E1000_REGISTER(a, reg)));
}

static inline u32 e1000_read_reg(struct e1000_hw *a, u32 reg, char *regname)
{
	u32 res = readl((a)->hw_addr + E1000_REGISTER(a, reg));
	if (e1000_dbg_reg_access) dbg_printf("%s -> 0x%08x\n", regname, res);
	return res;
}

static inline void e1000_write_reg_array(struct e1000_hw *a, u32 reg, int offset, u32 value, char *regname)
{
	if (e1000_dbg_reg_access) dbg_printf("%s[%d] <- 0x%08x\n", regname, offset, value);
	writel((value), ((a)->hw_addr + E1000_REGISTER(a, reg) + ((offset) << 2)));
}

static inline u32 e1000_read_reg_array(struct e1000_hw *a, u32 reg, int offset, char *regname)
{
	u32 res = readl((a)->hw_addr + E1000_REGISTER(a, reg) + ((offset) << 2));
	if (e1000_dbg_reg_access) dbg_printf("%s[%d] -> 0x%08x\n", regname, offset, res);
	return res;
}

static inline void e1000_write_reg_io(struct e1000_hw *a, u32 reg, u32 value, char *regname)
{
	if (e1000_dbg_reg_access) dbg_printf("%s <--IO-- 0x%08x\n", regname, value);
	outl(reg, ((a)->io_base));
	outl(value, ((a)->io_base + 4));
}

#define E1000_WRITE_REG(a, reg, value) e1000_write_reg(a, reg, value, #reg)
#define E1000_READ_REG(a, reg) e1000_read_reg(a, reg, #reg)
#define E1000_WRITE_REG_ARRAY(a, reg, offset, value) e1000_write_reg_array(a, reg, offset, value, #reg)
#define E1000_READ_REG_ARRAY(a, reg, offset) e1000_read_reg_array(a, reg, offset, #reg)
#define E1000_WRITE_REG_IO(a, reg, value) e1000_write_reg_io(a, reg, value, #reg)

#endif

#endif /* _E1000_OSDEP_H_ */
