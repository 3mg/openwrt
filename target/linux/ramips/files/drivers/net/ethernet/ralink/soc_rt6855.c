// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2009-2015 John Crispin <blogic at openwrt.org>
 * Copyright (C) 2009-2015 Felix Fietkau <nbd at nbd.name>
 * Copyright (C) 2013-2015 Michael Lee <igvtee at gmail.com>
 */

#include <linux/module.h>
#include <linux/delay.h>

#include <asm/mach-ralink/ralink_regs.h>

#include "mtk_eth_soc.h"
#include "mdio_rt2880.h"

#define RT6856_MDIO_ACCESS	0xf004
#define RT6856_MDIO_BUSY	BIT(31)
#define RT6856_MDIO_WRITE	0x00050000
#define RT6856_MDIO_READ	0x00090000

static int rt6856_mdio_wait(void)
{
	int retries = 10000;

	while (fe_r32(RT6856_MDIO_ACCESS) & RT6856_MDIO_BUSY) {
		if (!--retries)
			return -ETIMEDOUT;
		udelay(10);
	}
	return 0;
}

static int rt6856_mdio_read(struct mii_bus *bus, int phy, int reg)
{
	u32 cmd;
	int ret = rt6856_mdio_wait();

	if (ret)
		return ret;
	cmd = (phy << 20) | (reg << 25) | RT6856_MDIO_READ;
	fe_w32(cmd, RT6856_MDIO_ACCESS);
	fe_w32(cmd | RT6856_MDIO_BUSY, RT6856_MDIO_ACCESS);
	ret = rt6856_mdio_wait();
	return ret ? ret : fe_r32(RT6856_MDIO_ACCESS) & 0xffff;
}

static int rt6856_mdio_write(struct mii_bus *bus, int phy, int reg, u16 val)
{
	u32 cmd;
	int ret = rt6856_mdio_wait();

	if (ret)
		return ret;
	cmd = (phy << 20) | (reg << 25) | RT6856_MDIO_WRITE | val;
	fe_w32(cmd, RT6856_MDIO_ACCESS);
	fe_w32(cmd | RT6856_MDIO_BUSY, RT6856_MDIO_ACCESS);
	return rt6856_mdio_wait();
}

static const u16 rt5350_reg_table[FE_REG_COUNT] = {
	[FE_REG_PDMA_GLO_CFG] = RT5350_PDMA_GLO_CFG,    // OK
	[FE_REG_PDMA_RST_CFG] = RT5350_PDMA_RST_CFG,    // RST_IDX
	[FE_REG_DLY_INT_CFG] = RT5350_DLY_INT_CFG,      // OK
	[FE_REG_TX_BASE_PTR0] = RT5350_TX_BASE_PTR0,
	[FE_REG_TX_MAX_CNT0] = RT5350_TX_MAX_CNT0,
	[FE_REG_TX_CTX_IDX0] = RT5350_TX_CTX_IDX0,
	[FE_REG_TX_DTX_IDX0] = RT5350_TX_DTX_IDX0,
	[FE_REG_RX_BASE_PTR0] = RT5350_RX_BASE_PTR0,
	[FE_REG_RX_MAX_CNT0] = RT5350_RX_MAX_CNT0,
	[FE_REG_RX_CALC_IDX0] = RT5350_RX_CALC_IDX0,
	[FE_REG_RX_DRX_IDX0] = RT5350_RX_DRX_IDX0,
	[FE_REG_FE_INT_ENABLE] = RT5350_FE_INT_ENABLE,  // OK
	[FE_REG_FE_INT_STATUS] = RT5350_FE_INT_STATUS,  // OK
	[FE_REG_FE_RST_GL] = FE_FE_RST_GL,
	[FE_REG_FE_DMA_VID_BASE] = 0,
};

static void rt5350_init_data(struct fe_soc_data *data,
			     struct net_device *netdev)
{
	struct fe_priv *priv = netdev_priv(netdev);

	/* The board switch is an external RTL8367RB, not the RT3050 ESW. */
	priv->flags = 0;
	netdev->hw_features = NETIF_F_SG | NETIF_F_RXCSUM;
}

static void rt5350_set_mac(struct fe_priv *priv, const unsigned char *mac)
{
	unsigned long flags;

	spin_lock_irqsave(&priv->page_lock, flags);
	fe_w32((mac[0] << 8) | mac[1], FE_GDMA1_MAC_ADRH);
	fe_w32((mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5],
	       FE_GDMA1_MAC_ADRL);
	spin_unlock_irqrestore(&priv->page_lock, flags);
}

static void rt5350_rxcsum_config(bool enable)
{
	if (enable)
		fe_w32(fe_r32(FE_GDMA1_FWD_CFG) | (FE_GDM1_ICS_EN |
				FE_GDM1_TCS_EN | FE_GDM1_UCS_EN),
				FE_GDMA1_FWD_CFG);
	else
		fe_w32(fe_r32(FE_GDMA1_FWD_CFG) & ~(FE_GDM1_ICS_EN |
				FE_GDM1_TCS_EN | FE_GDM1_UCS_EN),
				FE_GDMA1_FWD_CFG);
}

static int rt5350_fwd_config(struct fe_priv *priv)
{
	struct net_device *dev = priv_netdev(priv);
	u32 fwd_cfg;

	/*
	 * RT6855/RT6856 uses the non-RT5350 GDMA receive path. Route every
	 * frame class to the CPU explicitly: a direct SPI boot does not leave
	 * behind the forwarding state programmed by U-Boot's TFTP driver.
	 */
	fwd_cfg = fe_r32(FE_GDMA1_FWD_CFG);
	fwd_cfg &= ~0xffff;
	fe_w32(fwd_cfg, FE_GDMA1_FWD_CFG);

	rt5350_rxcsum_config((dev->features & NETIF_F_RXCSUM));

	return 0;
}

static struct fe_soc_data rt5350_data = {
	.init_data = rt5350_init_data,
	.reg_table = rt5350_reg_table,
	.set_mac = rt5350_set_mac,
	.fwd_config = rt5350_fwd_config,
	.pdma_glo_cfg = FE_PDMA_SIZE_32DWORDS,
	.checksum_bit = RX_DMA_L4VALID,
	.rx_int = RT5350_RX_DONE_INT,
	.tx_int = RT5350_TX_DONE_INT,
	.mdio_read = rt6856_mdio_read,
	.mdio_write = rt6856_mdio_write,
};

const struct of_device_id of_fe_match[] = {
	{ .compatible = "ralink,rt6855-eth", .data = &rt5350_data },
	{},
};

MODULE_DEVICE_TABLE(of, of_fe_match);
