/*****************************************************************************

GPL LICENSE SUMMARY

  Copyright(c) 2007,2008,2009 Intel Corporation. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
  The full GNU General Public License is included in this distribution
  in the file called LICENSE.GPL.

  Contact Information:
  Intel Corporation

 version: Embedded.Release.Patch.L.1.0.7-5

  Contact Information:

  Intel Corporation, 5000 W Chandler Blvd, Chandler, AZ 85226

*****************************************************************************/

/* Linux GCU Driver main header file */

#ifndef _GCU_H_
#define _GCU_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ipxe/linux_compat.h>
#include <ipxe/timer.h>
#include <ipxe/io.h>
#include <ipxe/pci.h>

#include "../../wlan_compat.h"

#define BAR_0		0

#define PFX "gcu: "
#define GCU_DBG(args...) DBG(args)
#define GCU_ERR(args...) printf(PFX args)

#define pci_dev pci_device
#define iowrite32 writel
#define ioread32 readl

struct gcu_adapter {
	struct pci_dev *pdev;
	uint32_t mem_start;
	uint32_t mem_end;
	uint32_t base_addr;
	uint8_t *hw_addr;
	uint32_t pci_state[16];
	uint16_t device_id;
	uint16_t vendor_id;
	uint16_t subsystem_id;
	uint16_t subsystem_vendor_id;
	uint16_t pci_cmd_word;
	uint8_t revision_id;
};

/*
 * Exported interface functions need access to the modules
 * gcu_adapter struct
 */
const struct gcu_adapter *gcu_get_adapter(void);
void gcu_release_adapter(const struct gcu_adapter **adapter);

#endif				/* _GCU_H_ */
