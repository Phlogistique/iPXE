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

/**************************************************************************
 * @ingroup GCU_INTERFACE
 *
 * @file gcu_if.c
 *
 * @description
 *   This module contains shared functions for accessing and configuring
 *   the GCU.
 *
 **************************************************************************/

#include "gcu.h"
#include "gcu_reg.h"
#include "gcu_if.h"

#if DBG_LOG
/* forward declaration for write verify used in gcu_write_eth_phy */
static int32_t gcu_write_verify(uint32_t phy_num,
				uint32_t reg_addr,
				uint16_t written_data,
				const struct gcu_adapter *adapter);
#endif

/**
 * gcu_write_eth_phy
 * @phy_num: phy we want to write to, either 0, 1, or 2
 * @reg_addr: address in PHY's register space to write to
 * @phy_data: data to be written
 *
 * interface function for other modules to access the GCU
 **/
int32_t
gcu_write_eth_phy(uint32_t phy_num, uint32_t reg_addr, uint16_t phy_data)
{
	const struct gcu_adapter *adapter;
	uint32_t data = 0;
	uint32_t timeoutCounter = 0;
	const uint32_t timeoutCounterMax = GCU_MAX_ATTEMPTS;
	uint32_t pending;

	GCU_DBG("%s\n", __func__);

	if (phy_num > MDIO_COMMAND_PHY_ADDR_MAX) {
		GCU_ERR("phy_num = %d, which is greater than "
			"MDIO_COMMAND_PHY_ADDR_MAX\n", phy_num);

		return -1;
	}

	if (reg_addr > MDIO_COMMAND_PHY_REG_MAX) {
		GCU_ERR("reg_addr = %d, which is greater than "
			"MDIO_COMMAND_PHY_REG_MAX\n", phy_num);

		return -1;
	}

	/* format the data to be written to the MDIO_COMMAND_REG */
	data = phy_data;
	data |= (reg_addr << MDIO_COMMAND_PHY_REG_OFFSET);
	data |= (phy_num << MDIO_COMMAND_PHY_ADDR_OFFSET);
	data |= MDIO_COMMAND_OPER_MASK | MDIO_COMMAND_GO_MASK;

	adapter = gcu_get_adapter();
	if (!adapter) {
		GCU_ERR("gcu_adapter not available, cannot access MMIO\n");
		return -1;
	}

	/*
	 * We write to MDIO_COMMAND_REG initially, then read that
	 * same register until its MDIO_GO bit is cleared. When cleared,
	 * the transaction is complete
	 */
	iowrite32(data, adapter->hw_addr + MDIO_COMMAND_REG);
	do {
		timeoutCounter++;
		udelay(0x32);	/* 50 microsecond delay */
		data = ioread32(adapter->hw_addr + MDIO_COMMAND_REG);
		pending =
		    (data & MDIO_COMMAND_GO_MASK) >> MDIO_COMMAND_GO_OFFSET;
	} while (pending && timeoutCounter < timeoutCounterMax);

	if (timeoutCounter == timeoutCounterMax && pending) {
		GCU_ERR("Reached maximum number of retries"
			" accessing MDIO_COMMAND_REG\n");

		gcu_release_adapter(&adapter);

		return -1;
	}

	/* validate the write during debug */
#if DBG_LOG
	if (!gcu_write_verify(phy_num, reg_addr, phy_data, adapter))
		DBG("Write verification failed for PHY=%d and addr=%d\n",
		    phy_num, reg_addr);
#endif

	gcu_release_adapter(&adapter);

	return 0;
}
EXPORT_SYMBOL(gcu_write_eth_phy);

/**
 * gcu_read_eth_phy
 * @phy_num: phy we want to write to, either 0, 1, or 2
 * @reg_addr: address in PHY's register space to write to
 * @phy_data: data to be written
 *
 * interface function for other modules to access the GCU
 **/
int32_t
gcu_read_eth_phy(uint32_t phy_num, uint32_t reg_addr, uint16_t *phy_data)
{
	const struct gcu_adapter *adapter;
	uint32_t data = 0;
	uint32_t timeoutCounter = 0;
	const uint32_t timeoutCounterMax = GCU_MAX_ATTEMPTS;
	uint32_t pending = 0;

	GCU_DBG("%s\n", __func__);

	if (phy_num > MDIO_COMMAND_PHY_ADDR_MAX) {
		GCU_ERR("phy_num = %d, which is greater than "
			"MDIO_COMMAND_PHY_ADDR_MAX\n", phy_num);

		return -1;
	}

	if (reg_addr > MDIO_COMMAND_PHY_REG_MAX) {
		GCU_ERR("reg_addr = %d, which is greater than "
			"MDIO_COMMAND_PHY_REG_MAX\n", phy_num);

		return -1;
	}

	/* format the data to be written to MDIO_COMMAND_REG */
	data |= (reg_addr << MDIO_COMMAND_PHY_REG_OFFSET);
	data |= (phy_num << MDIO_COMMAND_PHY_ADDR_OFFSET);
	data |= MDIO_COMMAND_GO_MASK;

	adapter = gcu_get_adapter();
	if (!adapter) {
		GCU_ERR("gcu_adapter not available, cannot access MMIO\n");
		return -1;
	}

	/*
	 * We write to MDIO_COMMAND_REG initially, then read that
	 * same register until its MDIO_GO bit is cleared. When cleared,
	 * the transaction is complete
	 */
	iowrite32(data, adapter->hw_addr + MDIO_COMMAND_REG);
	do {
		timeoutCounter++;
		udelay(0x32);	/* 50 microsecond delay */
		data = ioread32(adapter->hw_addr + MDIO_COMMAND_REG);
		pending =
		    (data & MDIO_COMMAND_GO_MASK) >> MDIO_COMMAND_GO_OFFSET;
	} while (pending && timeoutCounter < timeoutCounterMax);

	if (timeoutCounter == timeoutCounterMax && pending) {
		GCU_ERR("Reached maximum number of retries"
			" accessing MDIO_COMMAND_REG\n");

		gcu_release_adapter(&adapter);

		return -1;
	}

	/* we retrieve the data from the MDIO_STATUS_REGISTER */
	data = ioread32(adapter->hw_addr + MDIO_STATUS_REG);
	if ((data & MDIO_STATUS_STATUS_MASK) != 0) {
		GCU_ERR("Unable to retrieve data from MDIO_STATUS_REG\n");

		gcu_release_adapter(&adapter);

		return -1;
	}

	*phy_data = (uint16_t) (data & MDIO_STATUS_READ_DATA_MASK);

	gcu_release_adapter(&adapter);

	return 0;
}
EXPORT_SYMBOL(gcu_read_eth_phy);

#if DBG_LOG
/**
 * gcu_write_verify
 * @phy_num: phy we want to write to, either 0, 1, or 2
 * @reg_addr: address in PHY's register space to write to
 * @phy_data: data to be checked
 * @adapter: pointer to global adapter struct
 *
 * This f(n) assumes that the spinlock acquired for adapter is
 * still in force.
 **/
int32_t
gcu_write_verify(uint32_t phy_num, uint32_t reg_addr, uint16_t written_data,
		 const struct gcu_adapter *adapter)
{
	u32 data = 0;
	u32 timeoutCounter = 0;
	const uint32_t timeoutCounterMax = GCU_MAX_ATTEMPTS;
	u32 pending = 0;
	u16 read_data;
	int success;

	GCU_DBG("%s\n", __func__);

	if (!adapter) {
		GCU_ERR("Invalid adapter pointer\n");
		return 0;
	}

	if (phy_num > MDIO_COMMAND_PHY_ADDR_MAX) {
		GCU_ERR("phy_num = %d, which is greater than "
			"MDIO_COMMAND_PHY_ADDR_MAX\n", phy_num);

		return 0;
	}

	if (reg_addr > MDIO_COMMAND_PHY_REG_MAX) {
		GCU_ERR("reg_addr = %d, which is greater than "
			"MDIO_COMMAND_PHY_REG_MAX\n", phy_num);

		return 0;
	}

	/* format the data to be written to MDIO_COMMAND_REG */
	data |= (reg_addr << MDIO_COMMAND_PHY_REG_OFFSET);
	data |= (phy_num << MDIO_COMMAND_PHY_ADDR_OFFSET);
	data |= MDIO_COMMAND_GO_MASK;

	/*
	 * We write to MDIO_COMMAND_REG initially, then read that
	 * same register until its MDIO_GO bit is cleared. When cleared,
	 * the transaction is complete
	 */
	iowrite32(data, adapter->hw_addr + MDIO_COMMAND_REG);
	do {
		timeoutCounter++;
		udelay(0x32);	/* 50 microsecond delay */
		data = ioread32(adapter->hw_addr + MDIO_COMMAND_REG);
		pending =
		    (data & MDIO_COMMAND_GO_MASK) >> MDIO_COMMAND_GO_OFFSET;
	} while (pending && timeoutCounter < timeoutCounterMax);

	if (timeoutCounter == timeoutCounterMax && pending) {
		GCU_ERR("Reached maximum number of retries"
			" accessing MDIO_COMMAND_REG\n");

		return 0;
	}

	/* we retrieve the data from the MDIO_STATUS_REGISTER */
	data = ioread32(adapter->hw_addr + MDIO_STATUS_REG);
	if ((data & MDIO_STATUS_STATUS_MASK) != 0) {
		GCU_ERR("Unable to retrieve data from MDIO_STATUS_REG\n");

		return 0;
	}

	read_data = data & MDIO_STATUS_READ_DATA_MASK;
	success = written_data == read_data;
	if (!success)
		DBG("WARNING: PHY%d reg %x: wrote 0x%04x, read 0x%04x\n",
		    phy_num, reg_addr, written_data, read_data);

	return success;
}
#endif
