/******************************************************************************

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

******************************************************************************/
/**************************************************************************
 * @ingroup GCU_GENERAL
 *
 * @file gcu_main.c
 *
 * @description
 *   This module contains the upper-edge routines of the driver
 *   interface that handle initialization, resets, and shutdowns
 *   of the GCU.
 *
 **************************************************************************/

#include "gcu.h"

char gcu_driver_name[] = "GCU";
char gcu_driver_string[] = "Global Configuration Unit Driver";
#define DRV_VERSION "1.0.0"
char gcu_driver_version[] = DRV_VERSION;
char gcu_copyright[] = "Copyright (c) 1999-2007 Intel Corporation.";

/* gcu_pci_tbl - PCI Device ID Table
 *
 * Last entry must be all 0s
 *
 * Macro expands to...
 *   {PCI_DEVICE(PCI_VENDOR_ID_INTEL, device_id)}
 */
static struct pci_device_id gcu_pci_tbl[] = {
	PCI_ID(0x8086, 0x503E, "gcu", "Global Configuration Unit", 0),
};

MODULE_DEVICE_TABLE(pci, gcu_pci_tbl);

enum gcu_err_type { err_ioremap, err_alloc_gcu_adapter };

static int gcu_probe(struct pci_dev *pdev);
static void gcu_probe_err(enum gcu_err_type err, struct gcu_adapter *adapter);
static void gcu_remove(struct pci_dev *pdev);
static struct gcu_adapter *alloc_gcu_adapter(void);
static void free_gcu_adapter(struct gcu_adapter *adapter);

struct pci_driver gcu_driver __pci_driver = {
	.ids = gcu_pci_tbl,
	.id_count = ARRAY_SIZE(gcu_pci_tbl),
	.probe = gcu_probe,
	.remove = gcu_remove,
	.load_order = 1,
};

static struct gcu_adapter *global_adapter = 0;

/**
 * gcu_probe - Device Initialization Routine
 * @pdev: PCI device information struct
 * @ent: entry in gcu_pci_tbl
 *
 * Returns 0 on success, negative on failure
 *
 * gcu_probe initializes an adapter identified by a pci_dev structure.
 * The OS initialization, configuring of the adapter private structure,
 * and a hardware reset occur.
 **/
static int
gcu_probe(struct pci_dev *pdev)
{
	struct gcu_adapter *adapter = 0;
	uint32_t mmio_start, mmio_len;

	GCU_DBG("%s\n", __func__);

	/** If we already have an adapter then we must already have been probed. */
	if (global_adapter != 0)
		return 0;

	adjust_pci_device(pdev);

	/*
	 * acquire the adapter spinlock. Once the module is loaded, it is
	 * possible for someone to access the adapter struct via the interface
	 * functions exported in gcu_if.c
	 */
	spin_lock(&global_adapter_spinlock);

	adapter = alloc_gcu_adapter();
	if (!adapter) {
		gcu_probe_err(err_alloc_gcu_adapter, adapter);
		spin_unlock(&global_adapter_spinlock);
		return -ENOMEM;
	}

	pci_set_drvdata(pdev, adapter);

	adapter->pdev = pdev;

	mmio_start = pci_bar_start(pdev, PCI_BASE_ADDRESS_0);
	mmio_len = pci_bar_size(pdev, PCI_BASE_ADDRESS_0);

	adapter->hw_addr = ioremap(mmio_start, mmio_len);
	if (!adapter->hw_addr) {
		GCU_DBG("Unable to map mmio\n");
		gcu_probe_err(err_ioremap, adapter);
		spin_unlock(&global_adapter_spinlock);
		return -EIO;
	}

	adapter->mem_start = mmio_start;
	adapter->mem_end = mmio_start + mmio_len;

	adapter->vendor_id = pdev->vendor;
	adapter->device_id = pdev->device;
	pci_read_config_word(pdev, PCI_SUBSYSTEM_VENDOR_ID,
			     &adapter->subsystem_vendor_id);
	pci_read_config_word(pdev, PCI_SUBSYSTEM_ID, &adapter->subsystem_id);
	pci_read_config_byte(pdev, PCI_REVISION_ID, &adapter->revision_id);

	pci_read_config_byte(pdev, PCI_REVISION_ID, &adapter->revision_id);

	pci_read_config_word(pdev, PCI_COMMAND, &adapter->pci_cmd_word);

	global_adapter = adapter;
	spin_unlock(&global_adapter_spinlock);

	DBG("Intel(R) GCU Initialized\n");

	return 0;
}

/**
 * gcu_probe_err - gcu_probe error handler
 * @err: gcu_err_type
 *
 * encapsulated error handling for gcu_probe
 **/
static void
gcu_probe_err(enum gcu_err_type err, struct gcu_adapter *adapter)
{
	switch (err) {
	case err_ioremap:
		iounmap(adapter->hw_addr);
	case err_alloc_gcu_adapter:
	default:
		free_gcu_adapter(adapter);
		break;
	}
}

/**
 * gcu_remove - Device Removal Routine
 * @pdev: PCI device information struct
 *
 * gcu_remove is called by the PCI subsystem to alert the driver
 * that it should release a PCI device.  The could be caused by a
 * Hot-Plug event, or because the driver is going to be removed from
 * memory.
 **/
static void gcu_remove(struct pci_dev *pdev)
{
	struct gcu_adapter *adapter = pci_get_drvdata(pdev);

	GCU_DBG("%s\n", __func__);

	iounmap(adapter->hw_addr);
	free_gcu_adapter(adapter);
	pci_set_drvdata(pdev, NULL);
}

/**
 * alloc_gcu_adapter
 *
 * alloc_gcu_adapter is a wrapper for the kmalloc call for the
 * device specific data block plus inits the global_adapter variable.
 *
 * Note that this function assumes that the spinlock for the global
 * gcu_adapter struct as been acquired.
 **/
static struct gcu_adapter *alloc_gcu_adapter()
{
	struct gcu_adapter *adapter;

	GCU_DBG("%s\n", __func__);

	adapter = malloc(sizeof(*adapter));

	global_adapter = adapter;

	if (!adapter) {
		GCU_DBG("Unable to allocate space for global gcu_adapter");
		return 0;
	}

	memset(adapter, 0, sizeof(*adapter));

	return adapter;
}

/**
 * free_gcu_adapter
 * @adapter: gcu_adapter struct to be free'd
 *
 * free_gcu_adapter is a wrapper for the kfree call for the
 * device specific data block plus clears the global_adapter variable
 *
 * Note that this function assumes that the spinlock for the global
 * gcu_adapter struct as been acquired.
 **/
static void free_gcu_adapter(struct gcu_adapter *adapter)
{
	GCU_DBG("%s\n", __func__);

	global_adapter = 0;

	free(adapter);
}

/**
 * gcu_get_adapter
 *
 * gcu_get_adapter is used by the functions exported in gcu_if.c to get
 * access to the memory addresses needed to access the MMIO registers
 * of the GCU
 **/
const struct gcu_adapter *gcu_get_adapter(void)
{
	GCU_DBG("%s\n", __func__);

	if (global_adapter == NULL) {
		GCU_DBG("global gcu_adapter is not available\n");
		return NULL;
	}

	return global_adapter;
}

/**
 * gcu_release_adapter
 *
 * gcu_release_adapter is used by the functions exported in gcu_if.c to get
 * release the adapter spinlock and the handle to the adapter
 **/
void gcu_release_adapter(const struct gcu_adapter **adapter)
{
	GCU_DBG("%s\n", __func__);

	if (adapter == NULL)
		GCU_ERR("global gcu_adapter handle is invalid\n");
	else
		*adapter = 0;

	return;
}

/* gcu_main.c */
