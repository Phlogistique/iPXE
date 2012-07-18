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

#include <ipxe/linux_compat.h>

#include "e1000_api.h"
#include "gcu/gcu_if.h"
#include "e1000_defines.h"

static s32 e1000_init_phy_params_ep80579(struct e1000_hw *hw);
static s32 e1000_init_nvm_params_ep80579(struct e1000_hw *hw);
static s32 e1000_init_mac_params_ep80579(struct e1000_hw *hw);

static s32 e1000_reset_hw_ep80579(struct e1000_hw *hw);
static s32 e1000_phy_read_reg_ep80579(struct e1000_hw *hw, u32 reg_addr,
				      u16 *phy_data);
static s32 e1000_phy_write_reg_ep80579(struct e1000_hw *hw, u32 reg_addr,
				       u16 phy_data);
static s32 e1000_init_hw_ep80579(struct e1000_hw *hw);
static s32 e1000_setup_copper_link_ep80579(struct e1000_hw *hw);
static void e1000_clear_hw_cntrs_ep80579(struct e1000_hw *hw);
static s32 e1000_config_mac_to_phy_ep80579(struct e1000_hw *hw);
s32 e1000_xioh_read_mac_addr(struct e1000_hw *hw);
s32 e1000_phy_has_link_ep80579(struct e1000_hw *hw, bool *isUp);
static s32 e1000_phy_hw_reset_ep80579(struct e1000_hw *hw);

/**
 *  e1000_init_phy_params_ep80579 - Init PHY func ptrs.
 *  @hw: pointer to the HW structure
 **/
static s32 e1000_init_phy_params_ep80579(struct e1000_hw *hw)
{
	struct e1000_phy_info *phy = &hw->phy;
	int err;

	DBGF;

	phy->ops.read_reg = e1000_phy_read_reg_ep80579;
	phy->ops.write_reg = e1000_phy_write_reg_ep80579;

	err = e1000_get_phy_id(hw);
	if (err < 0)
		return err;

	switch (phy->id) {
	case M88E1141_E_PHY_ID:
		DBG("PHY OK\n");
		break;
	default:
		DBG("PHY not supported for EP80579! id: 0x%08x rev: 0x%08x\n",
		    phy->id, phy->revision);
		return -E1000_ERR_PHY;
	}

	phy->ops.get_info = e1000_get_phy_info_m88;
	phy->ops.reset = e1000_phy_hw_reset_ep80579;
	phy->ops.get_cfg_done = e1000_get_cfg_done_generic;
	phy->ops.commit = e1000_phy_sw_reset_generic;

	phy->autoneg_mask = AUTONEG_ADVERTISE_SPEED_DEFAULT;
	phy->type = e1000_phy_m88;

	return E1000_SUCCESS;
}

static s32 e1000_phy_read_reg_ep80579(struct e1000_hw *hw, u32 reg_addr,
				      u16 *phy_data)
{
	int err = gcu_read_eth_phy(hw->dev_spec.ep80579.device_number, reg_addr,
				   phy_data);
#ifdef DEBUG_REGS
	DBG("%s reg: 0x%08X data: 0x%04X\n", __func__, reg_addr, *phy_data);
#endif
	return err;
}

static s32 e1000_phy_write_reg_ep80579(struct e1000_hw *hw, u32 reg_addr,
				       u16 phy_data)
{
#ifdef DEBUG_REGS
	DBG("%s reg: 0x%08X data: 0x%04X\n", __func__, reg_addr, phy_data);
#endif
	return gcu_write_eth_phy(hw->dev_spec.ep80579.device_number, reg_addr,
				 phy_data);
}

/**
 *  e1000_init_nvm_params_ep80579 - Init NVM func ptrs.
 *  @hw: pointer to the HW structure
 **/
static s32 e1000_init_nvm_params_ep80579(struct e1000_hw *hw)
{
	(void) hw;
	DBGF;
	return E1000_SUCCESS;
}

/**
 *  e1000_init_mac_params_ep80579 - Init MAC func ptrs.
 *  @hw: pointer to the HW structure
 **/
static s32 e1000_init_mac_params_ep80579(struct e1000_hw *hw)
{
	static const u8 device_number[] = {
		[E1000_DEV_ID_EP80579_MAC0] = 0,
		[E1000_DEV_ID_EP80579_QA_MAC0] = 0,
		[E1000_DEV_ID_EP80579_RESERVED0_MAC0] = 0,
		[E1000_DEV_ID_EP80579_RESERVED1_MAC0] = 0,
		[E1000_DEV_ID_EP80579_MAC1] = 1,
		[E1000_DEV_ID_EP80579_QA_MAC1] = 1,
		[E1000_DEV_ID_EP80579_RESERVED0_MAC1] = 1,
		[E1000_DEV_ID_EP80579_RESERVED1_MAC1] = 1,
		[E1000_DEV_ID_EP80579_MAC2] = 2,
		[E1000_DEV_ID_EP80579_QA_MAC2] = 2,
		[E1000_DEV_ID_EP80579_RESERVED0_MAC2] = 2,
		[E1000_DEV_ID_EP80579_RESERVED1_MAC2] = 2,
	};

	struct e1000_mac_info *mac = &hw->mac;
	DBGF;

	hw->dev_spec.ep80579.device_number = device_number[hw->device_id];

	mac->mta_reg_count = 128;

	mac->ops.reset_hw = e1000_reset_hw_ep80579;
	mac->ops.read_mac_addr = e1000_xioh_read_mac_addr;
	mac->ops.init_hw = e1000_init_hw_ep80579;
	mac->ops.setup_link = e1000_setup_link_generic;
	mac->ops.get_link_up_info = e1000_get_speed_and_duplex_copper_generic;
	mac->ops.setup_physical_interface = e1000_setup_copper_link_ep80579;
	mac->ops.clear_vfta = e1000_clear_vfta_generic;
	mac->ops.clear_hw_cntrs = e1000_clear_hw_cntrs_ep80579;

	return E1000_SUCCESS;
}

static s32 e1000_init_hw_ep80579(struct e1000_hw *hw)
{
	struct e1000_mac_info *mac = &hw->mac;
	u32 txdctl;
	s32 err;
	int i;

	DBGF;

	hw->phy.media_type = e1000_media_type_copper;

	mac->rar_entry_count = E1000_RAR_ENTRIES;

	SILENT
		mac->ops.clear_vfta(hw);

	e1000_init_rx_addrs_generic(hw, mac->rar_entry_count);

	SILENT {
		/* Zero out the Multicast HASH table */
		DBG("Zeroing the MTA\n");
		for (i = 0; i < mac->mta_reg_count; i++) {
			E1000_WRITE_REG_ARRAY(hw, E1000_MTA, i, 0);
			E1000_WRITE_FLUSH(hw);
		}
		DBG("Zeroing the Flexible Filter tables\n");
		for (i = 0; i < E1000_FFMT_SIZE; i++) {
			E1000_WRITE_REG_ARRAY(hw, E1000_FFMT, i, 0);
			E1000_WRITE_FLUSH(hw);
		}
		for (i = 0; i < E1000_FFVT_SIZE; i++) {
			E1000_WRITE_REG_ARRAY(hw, E1000_FFVT, i, 0);
			E1000_WRITE_FLUSH(hw);
		}
	}

	err = e1000_setup_link(hw);
	if (err < 0) {
		DBG("Couldn't setup link\n");
		return err;
	}

	/* Set the transmit descriptor write-back policy */
	txdctl = E1000_READ_REG(hw, E1000_TXDCTL(0));
	txdctl = (txdctl & ~E1000_TXDCTL_WTHRESH) |
	    E1000_TXDCTL_FULL_TX_DESC_WB;
	E1000_WRITE_REG(hw, E1000_TXDCTL(0), txdctl);

	/* Clear all of the statistics registers (clear on read).  It is
	 * important that we do this after we have tried to establish link
	 * because the symbol error count will increment wildly if there
	 * is no link.
	 */
	e1000_clear_hw_cntrs_ep80579(hw);

	return E1000_SUCCESS;
}

static s32 e1000_phy_hw_reset_ep80579(struct e1000_hw *hw)
{
	u16 reg;
	int ret_val;

	ret_val = hw->phy.ops.read_reg(hw, PHY_CONTROL, &reg);
	if (ret_val)
		goto out;

	reg |= MII_CR_RESET;

	ret_val = hw->phy.ops.write_reg(hw, PHY_CONTROL, reg);
	if (ret_val)
		goto out;

	udelay(1);

out:
	return ret_val;
}

#if 0
static s32 e1000_phy_loopback_setup_m88_autoneg(struct e1000_hw *hw)
{
	u16 reg;
	int ret_val;

	ret_val = hw->phy.ops.read_reg(hw, M88E1000_EXT_PHY_SPEC_CTRL, &reg);
	if (ret_val)
		goto out;

	reg |= 5 << 4; //M88E1000_EPSCR_TX_CLK_25;

	ret_val = hw->phy.ops.write_reg(hw, M88E1000_EXT_PHY_SPEC_CTRL, reg);
	if (ret_val)
		goto out;


	ret_val = hw->phy.ops.commit(hw);
	if (ret_val)
		goto out;

	ret_val = hw->phy.ops.read_reg(hw, PHY_CONTROL, &reg);
	if (ret_val)
		goto out;

	reg |= PHY_CONTROL_LB;

	ret_val = hw->phy.ops.write_reg(hw, PHY_CONTROL, reg);
	if (ret_val)
		goto out;

out:
	return ret_val;
}

static s32 e1000_phy_loopback_setup_m88_force(struct e1000_hw *hw)
{
	u32 ctrl;
	int ret_val;

	ret_val = hw->phy.ops.write_reg(hw, PHY_CONTROL, 0xa100);
	if (ret_val)
		goto out;

	ret_val = hw->phy.ops.write_reg(hw, PHY_CONTROL, 0x6100);
	if (ret_val)
		goto out;

	ctrl = E1000_READ_REG(hw, E1000_CTRL);
	ctrl &= ~E1000_CTRL_SPD_SEL; /* Clear the speed sel bits */
	ctrl |= (E1000_CTRL_FRCSPD /* Set the Force Speed Bit */
	     | E1000_CTRL_FRCDPX /* Set the Force Duplex Bit */
	     | E1000_CTRL_SPD_100 /* Force Speed to 1000 */
	     | E1000_CTRL_FD); /* Force Duplex to FULL */
	/*   | E1000_CTRL_ILOS); *//* Invert Loss of Signal */

	E1000_WRITE_REG(hw, E1000_CTRL, ctrl);

out:
	return ret_val;
}
#endif

static s32 e1000_setup_copper_link_ep80579(struct e1000_hw *hw)
{
	u32 ctrl;
	s32 ret_val;
	bool link;

	DBGF;

	ctrl = E1000_READ_REG(hw, E1000_CTRL) | E1000_CTRL_SLU |
	                                        E1000_CTRL_FRCSPD |
						E1000_CTRL_FRCDPX;
	E1000_WRITE_REG(hw, E1000_CTRL, ctrl);
	ret_val = hw->phy.ops.reset(hw);
	if (ret_val)
		goto out;

	hw->phy.reset_disable = false;

	/* Set MDI/MDI-X, Polarity Reversal, and downshift settings */
	ret_val = e1000_copper_link_setup_m88(hw);
	if (ret_val)
		goto out;

	ret_val = e1000_copper_link_autoneg(hw);
	if (ret_val)
		goto out;

	ret_val = e1000_phy_has_link_ep80579(hw, &link);
	if (ret_val)
		goto out;

	if (!link) {
		DBG("Unable to establish link!!!\n");
		goto out;
	}

	DBG("Valid link established!!!\n");

	/* Config the MAC and PHY after link is up */
	ret_val = e1000_config_mac_to_phy_ep80579(hw);
	if (ret_val)
		goto out;

	ret_val = e1000_config_fc_after_link_up_generic(hw);
	if (ret_val)
		goto out;

out:
	return ret_val;
}

static s32 e1000_config_mac_to_phy_ep80579(struct e1000_hw *hw)
{
	u32 ctrl;
	s32 ret_val = E1000_SUCCESS;
	u16 phy_data;

	DBGF;

	/* Set the bits to force speed and duplex */
	ctrl = E1000_READ_REG(hw, E1000_CTRL);
	ctrl |= (E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPX);
	ctrl &= ~(E1000_CTRL_SPD_SEL | E1000_CTRL_ILOS);

	/*
	 * Set up duplex in the Device Control and Transmit Control
	 * registers depending on negotiated values.
	 */
	ret_val = hw->phy.ops.read_reg(hw, M88E1000_PHY_SPEC_STATUS, &phy_data);
	if (ret_val)
		goto out;

	ctrl &= ~E1000_CTRL_FD;
	if (phy_data & M88E1000_PSSR_DPLX)
		ctrl |= E1000_CTRL_FD;

	e1000_config_collision_dist_generic(hw);

	/*
	 * Set up speed in the Device Control register depending on
	 * negotiated values.
	 */
	if ((phy_data & M88E1000_PSSR_SPEED) == M88E1000_PSSR_1000MBS)
		ctrl |= E1000_CTRL_SPD_1000;
	else if ((phy_data & M88E1000_PSSR_SPEED) == M88E1000_PSSR_100MBS)
		ctrl |= E1000_CTRL_SPD_100;

	E1000_WRITE_REG(hw, E1000_CTRL, ctrl);

out:
	return ret_val;
}

s32 e1000_phy_has_link_ep80579(struct e1000_hw *hw, bool *isUp)
{
	struct e1000_phy_info *phy = &hw->phy;
	u16 phy_data;
	s32 ret_val;

	DBGF;

	/* WHY is this read TWICE? */
	phy->ops.read_reg(hw, M88E1000_PHY_SPEC_STATUS, &phy_data);
	ret_val = phy->ops.read_reg(hw, M88E1000_PHY_SPEC_STATUS, &phy_data);

	if (ret_val)
		return ret_val;

	*isUp = !!(phy_data & M88E1000_PSSR_LINK);

	return E1000_SUCCESS;
}

static void e1000_display_frame_cnt_ep80579(struct e1000_hw *hw)
{
	u16 phy_data;
	static int prev_frame_cnt;
	static int prev_crc_cnt;
	int curr_frame_cnt, curr_crc_cnt;
	DBGP("%s\n", __func__);

	hw->phy.ops.read_reg(hw, M88E1000_PHY_PAGE_SELECT, &phy_data);
	phy_data &= ~0x1f;
	phy_data |= 12;
	hw->phy.ops.write_reg(hw, M88E1000_PHY_PAGE_SELECT, phy_data);

	hw->phy.ops.read_reg(hw, 30, &phy_data);
	curr_frame_cnt = phy_data >> 8;
	curr_crc_cnt = phy_data & 0xff;

	if (curr_frame_cnt != prev_frame_cnt) {
		dbg_printf("Frame count: %d\n", curr_frame_cnt);
		prev_frame_cnt = curr_frame_cnt;
	}

	if (curr_crc_cnt != prev_crc_cnt) {
		dbg_printf("CRC error count: %d\n", curr_crc_cnt);
		prev_crc_cnt = curr_crc_cnt;
	}
}

/**
 * e1000_clear_hw_cntrs_ep80579 - Clears all hardware statistics counters.
 */
static void e1000_clear_hw_cntrs_ep80579(struct e1000_hw *hw)
{
	DBGP("%s\n", __func__);

	e1000_display_frame_cnt_ep80579(hw);

	E1000_DUMP_CNTR(hw, E1000_CRCERRS);
	E1000_DUMP_CNTR(hw, E1000_SYMERRS);
	E1000_DUMP_CNTR(hw, E1000_MPC);
	E1000_DUMP_CNTR(hw, E1000_SCC);
	E1000_DUMP_CNTR(hw, E1000_ECOL);
	E1000_DUMP_CNTR(hw, E1000_MCC);
	E1000_DUMP_CNTR(hw, E1000_LATECOL);
	E1000_DUMP_CNTR(hw, E1000_COLC);
	E1000_DUMP_CNTR(hw, E1000_DC);
	E1000_DUMP_CNTR(hw, E1000_SEC);
	E1000_DUMP_CNTR(hw, E1000_RLEC);
	E1000_DUMP_CNTR(hw, E1000_XONRXC);
	E1000_DUMP_CNTR(hw, E1000_XONTXC);
	E1000_DUMP_CNTR(hw, E1000_XOFFRXC);
	E1000_DUMP_CNTR(hw, E1000_XOFFTXC);
	E1000_DUMP_CNTR(hw, E1000_FCRUC);

	E1000_DUMP_CNTR(hw, E1000_PRC64);
	E1000_DUMP_CNTR(hw, E1000_PRC127);
	E1000_DUMP_CNTR(hw, E1000_PRC255);
	E1000_DUMP_CNTR(hw, E1000_PRC511);
	E1000_DUMP_CNTR(hw, E1000_PRC1023);
	E1000_DUMP_CNTR(hw, E1000_PRC1522);

	E1000_DUMP_CNTR(hw, E1000_GPRC);
	E1000_DUMP_CNTR(hw, E1000_BPRC);
	E1000_DUMP_CNTR(hw, E1000_MPRC);
	E1000_DUMP_CNTR(hw, E1000_GPTC);
	E1000_DUMP_CNTR(hw, E1000_GORCL);
	E1000_DUMP_CNTR(hw, E1000_GORCH);
	E1000_DUMP_CNTR(hw, E1000_GOTCL);
	E1000_DUMP_CNTR(hw, E1000_GOTCH);
	E1000_DUMP_CNTR(hw, E1000_RNBC);
	E1000_DUMP_CNTR(hw, E1000_RUC);
	E1000_DUMP_CNTR(hw, E1000_RFC);
	E1000_DUMP_CNTR(hw, E1000_ROC);
	E1000_DUMP_CNTR(hw, E1000_RJC);
	E1000_DUMP_CNTR(hw, E1000_TORL);
	E1000_DUMP_CNTR(hw, E1000_TORH);
	E1000_DUMP_CNTR(hw, E1000_TOTL);
	E1000_DUMP_CNTR(hw, E1000_TOTH);
	E1000_DUMP_CNTR(hw, E1000_TPR);
	E1000_DUMP_CNTR(hw, E1000_TPT);

	E1000_DUMP_CNTR(hw, E1000_PTC64);
	E1000_DUMP_CNTR(hw, E1000_PTC127);
	E1000_DUMP_CNTR(hw, E1000_PTC255);
	E1000_DUMP_CNTR(hw, E1000_PTC511);
	E1000_DUMP_CNTR(hw, E1000_PTC1023);
	E1000_DUMP_CNTR(hw, E1000_PTC1522);

	E1000_DUMP_CNTR(hw, E1000_MPTC);
	E1000_DUMP_CNTR(hw, E1000_BPTC);

	E1000_DUMP_CNTR(hw, E1000_ALGNERRC);
	E1000_DUMP_CNTR(hw, E1000_RXERRC);
	E1000_DUMP_CNTR(hw, E1000_TNCRS);
	E1000_DUMP_CNTR(hw, E1000_CEXTERR);
	E1000_DUMP_CNTR(hw, E1000_TSCTC);
	E1000_DUMP_CNTR(hw, E1000_TSCTFC);
}

#define XIOH_MAC_OFFSET 0xfff00000

s32 e1000_xioh_read_mac_addr(struct e1000_hw *hw)
{
	static const u8 xioh_mac_magic[] = {'X','I','O','H', 0x00, 0x00};
	unsigned i;
	u8 *mac;

	DBGF;

	mac = ioremap(XIOH_MAC_OFFSET, 24);

	for (i = 0; i < ARRAY_SIZE(xioh_mac_magic); i++)
		if (xioh_mac_magic[i] != mac[i])
			return -E1000_ERR_NVM;

	for (i = 0; i < ETH_ALEN; i++)
		hw->mac.perm_addr[i] =
		    mac[ARRAY_SIZE(xioh_mac_magic) +
			ETH_ALEN * hw->dev_spec.ep80579.device_number + i];

	iounmap(mac);

	for (i = 0; i < ETH_ALEN; i++)
		hw->mac.addr[i] = hw->mac.perm_addr[i];

	return E1000_SUCCESS;
}

/**
 *  e1000_init_function_pointers_ep80579 - Init func ptrs.
 *  @hw: pointer to the HW structure
 *
 *  Called to initialize all function pointers and parameters.
 **/
void e1000_init_function_pointers_ep80579(struct e1000_hw *hw)
{
	DBGF;

	hw->mac.ops.init_params = e1000_init_mac_params_ep80579;
	hw->nvm.ops.init_params = e1000_init_nvm_params_ep80579;
	hw->phy.ops.init_params = e1000_init_phy_params_ep80579;
}

static s32 e1000_reset_hw_ep80579(struct e1000_hw *hw)
{
	s32 ret_val = E1000_SUCCESS;
	u32 ctrl;

	DBGF;

	DBG("Masking off all interrupts\n");
	E1000_WRITE_REG(hw, E1000_IMC, 0xffffffff);

	E1000_WRITE_REG(hw, E1000_RCTL, 0);
	E1000_WRITE_REG(hw, E1000_TCTL, E1000_TCTL_PSP);
	E1000_WRITE_FLUSH(hw);

	/*
	 * Delay to allow any outstanding PCI transactions to complete before
	 * resetting the device
	 */
	mdelay(10);

	ctrl = E1000_READ_REG(hw, E1000_CTRL);

	DBG("Issuing a soft reset to GbE MAC.\n");

	E1000_WRITE_REG(hw, E1000_CTRL, ctrl | E1000_CTRL_RST);
	mdelay(6); /* As datasheet says */

	E1000_WRITE_REG(hw, E1000_IMC, 0xffffffff);
	E1000_READ_REG(hw, E1000_ICR);

	return ret_val;
}

static struct pci_device_id e1000_ep80579_nics[] = {
	PCI_ROM(0x8086, 0x5040, "EP80579_MAC0", "Intel EP80579 Integrated Processor Gigabit Ethernet MAC 0", e1000_ep80579),
	PCI_ROM(0x8086, 0x5041, "EP80579_QA_MAC0", "Intel EP80579 Integrated Processor with QuickAssist Gigabit Ethernet MAC 0", e1000_ep80579),
	PCI_ROM(0x8086, 0x5042, "EP80579_RESERVED0_MAC0", "Intel EP80579 Integrated Processor (reserved) Gigabit Ethernet MAC 0", e1000_ep80579),
	PCI_ROM(0x8086, 0x5043, "EP80579_RESERVED1_MAC0", "Intel EP80579 Integrated Processor (reserved) Gigabit Ethernet MAC 0", e1000_ep80579),
	PCI_ROM(0x8086, 0x5044, "EP80579_MAC1", "Intel EP80579 Integrated Processor Gigabit Ethernet MAC 1", e1000_ep80579),
	PCI_ROM(0x8086, 0x5045, "EP80579_QA_MAC1", "Intel EP80579 Integrated Processor with QuickAssist Gigabit Ethernet MAC 1", e1000_ep80579),
	PCI_ROM(0x8086, 0x5046, "EP80579_RESERVED0_MAC1", "Intel EP80579 Integrated Processor (reserved) Gigabit Ethernet MAC 1", e1000_ep80579),
	PCI_ROM(0x8086, 0x5047, "EP80579_RESERVED1_MAC1", "Intel EP80579 Integrated Processor (reserved) Gigabit Ethernet MAC 1", e1000_ep80579),
	PCI_ROM(0x8086, 0x5048, "EP80579_MAC2", "Intel EP80579 Integrated Processor Gigabit Ethernet MAC 2", e1000_ep80579),
	PCI_ROM(0x8086, 0x5049, "EP80579_QA_MAC2", "Intel EP80579 Integrated Processor with QuickAssist Gigabit Ethernet MAC 2", e1000_ep80579),
	PCI_ROM(0x8086, 0x504A, "EP80579_RESERVED0_MAC2", "Intel EP80579 Integrated Processor (reserved) Gigabit Ethernet MAC 2", e1000_ep80579),
	PCI_ROM(0x8086, 0x504B, "EP80579_RESERVED1_MAC2", "Intel EP80579 Integrated Processor (reserved) Gigabit Ethernet MAC 2", e1000_ep80579),
};

struct pci_driver e1000_ep80579_driver __pci_driver = {
	.ids = e1000_ep80579_nics,
	.id_count = (sizeof (e1000_ep80579_nics) / sizeof (e1000_ep80579_nics[0])),
	.probe = e1000_probe,
	.remove = e1000_remove,
};
