// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2016 Carlo Caione <carlo@caione.org>
 */

#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <malloc.h>
#include <clk.h>
#include <mmc.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/cpu_sdio.h>
#include <asm/arch/sd_emmc.h>
#include <linux/log2.h>
#include "mmc_private.h"

static inline void *get_regbase(const struct mmc *mmc)
{
	struct meson_mmc_platdata *pdata = mmc->priv;

	return pdata->regbase;
}

static inline uint32_t meson_read(struct mmc *mmc, int offset)
{
	return readl(get_regbase(mmc) + offset);
}

static inline void meson_write(struct mmc *mmc, uint32_t val, int offset)
{
	writel(val, get_regbase(mmc) + offset);
}

void mmc_print_reg(struct udevice *dev)
{
	struct mmc *mmc = mmc_get_mmc_dev(dev);

	pr_info("%s: clk = %x, dly1 = %x, dly2 = %x, adj = %x, cfg = %x\n",
			mmc->cfg->name,
			meson_read(mmc, MESON_SD_EMMC_CLOCK),
			meson_read(mmc, MESON_SD_EMMC_DELAY1),
			meson_read(mmc, MESON_SD_EMMC_DELAY2),
			meson_read(mmc, MESON_SD_EMMC_ADJUST),
			meson_read(mmc, MESON_SD_EMMC_CFG));
}

static int mmc_controller_debug(struct udevice *dev,
		struct mmc_cmd *cmd, uint32_t status)
{
	int ret =0;
	struct mmc *mmc = mmc_get_mmc_dev(dev);
	struct meson_host *host = dev_get_priv(dev);

	if (status & STATUS_RXD_ERR_MASK) {
		ret |= SD_EMMC_RXD_ERROR;
		if (host->is_tuning == 0)
			pr_err("%s: read crc err, cmd%d, status=0x%x\n",
					mmc->cfg->name, cmd->cmdidx, status);
	}
	if (status & STATUS_TXD_ERR) {
		ret |= SD_EMMC_TXD_ERROR;
		if (host->is_tuning == 0)
			pr_err("%s: write tx err, cmd%d, status=0x%x\n",
					mmc->cfg->name, cmd->cmdidx, status);
	}
	if (status & STATUS_DESC_ERR) {
		ret |= SD_EMMC_DESC_ERROR;
		if (host->is_tuning == 0)
			pr_err("%s: desc error, cmd%d, status=0x%x\n",
					mmc->cfg->name, cmd->cmdidx, status);
	}
	if (status & STATUS_RESP_ERR) {
		ret |= SD_EMMC_RESP_CRC_ERROR;
		if (host->is_tuning == 0)
			pr_err("%s: resp crc error, cmd%d, status=0x%x\n",
					mmc->cfg->name, cmd->cmdidx, status);

	}
	if (status & STATUS_RESP_TIMEOUT) {
		ret |= SD_EMMC_RESP_TIMEOUT_ERROR;
		if (host->is_tuning == 0)
			pr_err("%s: resp timeout, cmd%d, status=0x%x\n",
					mmc->cfg->name, cmd->cmdidx, status);
	}
	if (status & STATUS_DESC_TIMEOUT) {
		ret |= SD_EMMC_DESC_TIMEOUT_ERROR;
		if (host->is_tuning == 0)
			pr_err("%s: desc timeout, cmd%d, status=0x%x\n",
					mmc->cfg->name, cmd->cmdidx, status);
	}

	pr_debug("cmd->cmdidx = %d, cmd->cmdarg=0x%x, ret=0x%x\n",cmd->cmdidx,cmd->cmdarg,ret);
	pr_debug("cmd->response[0]=0x%x;\n",cmd->response[0]);
	pr_debug("cmd->response[1]=0x%x;\n",cmd->response[1]);
	pr_debug("cmd->response[2]=0x%x;\n",cmd->response[2]);
	pr_debug("cmd->response[3]=0x%x;\n",cmd->response[3]);

	return ret;
}

static void meson_mmc_config_clock(struct meson_host *host)
{
	struct mmc *mmc = host->mmc;
	uint32_t clk = 0, clk_src = 0, clk_div = 0;
	uint32_t co_phase = 0, tx_phase = 0;
	uint32_t meson_mmc_clk = 0;

	if (!mmc->clock)
		return;

	if (mmc->clock > 12000000) {
		clk = 1000000000;
		clk_src = 1;
	} else {
		clk = 24000000;
		clk_src = 0;
	}

	clk_div = clk / mmc->clock;
	if (clk % mmc->clock)
		clk_div++;
	if (mmc->ddr_mode) {
		clk_div /= 2;
		pr_info("DDR: \n");
	}

	switch (mmc->selected_mode)
	{
		case MMC_LEGACY:
		case SD_LEGACY:
			co_phase = dev_read_u32_default(mmc->dev, "init_co_phase", 2);
			tx_phase = dev_read_u32_default(mmc->dev, "init_to_phase", 0);
			break;
		case MMC_HS:
		case MMC_HS_52:
			dev_read_u32(mmc->dev, "hs_co_phase", &co_phase);
			dev_read_u32(mmc->dev, "hs_to_phase", &tx_phase);
			break;
		case SD_HS:
			break;
		case MMC_HS_200:
			dev_read_u32(mmc->dev, "hs2_co_phase", &co_phase);
			dev_read_u32(mmc->dev, "hs2_to_phase", &tx_phase);
			break;
		case UHS_SDR104:
			dev_read_u32(mmc->dev, "sdr104_co_phase", &co_phase);
			dev_read_u32(mmc->dev, "sdr104_to_phase", &tx_phase);
			break;
		case MMC_DDR_52:
			dev_read_u32(mmc->dev, "ddr_co_phase", &co_phase);
			dev_read_u32(mmc->dev, "ddr_to_phase", &tx_phase);
			break;
		case MMC_HS_400:
			break;
		default:
			co_phase = dev_read_u32_default(mmc->dev, "init_co_phase", 2);
			tx_phase = dev_read_u32_default(mmc->dev, "init_to_phase", 0);
			break;
	}

	meson_mmc_clk =((0 << Cfg_irq_sdio_sleep_ds) |
					(0 << Cfg_irq_sdio_sleep) |
					(1 << Cfg_always_on) |
					(0 << Cfg_rx_delay) |
					(0 << Cfg_tx_delay) |
					(0 << Cfg_sram_pd) |
					(0 << Cfg_rx_phase) |
					(tx_phase << Cfg_tx_phase) |
					(co_phase << Cfg_co_phase) |
					(clk_src << Cfg_src) |
					(clk_div << Cfg_div));

	meson_write(mmc, meson_mmc_clk, MESON_SD_EMMC_CLOCK);
}

static int meson_dm_mmc_set_ios(struct udevice *dev)
{
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = mmc_get_mmc_dev(dev);
	uint32_t meson_mmc_cfg;

	if (!mmc->clock) {
		meson_write(mmc, 0, MESON_SD_EMMC_DELAY1);
		meson_write(mmc, 0, MESON_SD_EMMC_DELAY2);
		meson_write(mmc, 0, MESON_SD_EMMC_ADJUST);
	}

	meson_mmc_config_clock(host);

	meson_mmc_cfg = meson_read(mmc, MESON_SD_EMMC_CFG);

	meson_mmc_cfg &= ~CFG_BUS_WIDTH_MASK;
	if (mmc->bus_width == 1)
		meson_mmc_cfg |= CFG_BUS_WIDTH_1;
	else if (mmc->bus_width == 4)
		meson_mmc_cfg |= CFG_BUS_WIDTH_4;
	else if (mmc->bus_width == 8)
		meson_mmc_cfg |= CFG_BUS_WIDTH_8;
	else
		return -EINVAL;

	/* 512 bytes block length */
	meson_mmc_cfg &= ~CFG_BL_LEN_MASK;
	meson_mmc_cfg |= CFG_BL_LEN_512;

	/* Response timeout 256 clk */
	meson_mmc_cfg &= ~CFG_RESP_TIMEOUT_MASK;
	meson_mmc_cfg |= CFG_RESP_TIMEOUT_256;

	/* Command-command gap 16 clk */
	meson_mmc_cfg &= ~CFG_RC_CC_MASK;
	meson_mmc_cfg |= CFG_RC_CC_16;

	meson_write(mmc, meson_mmc_cfg, MESON_SD_EMMC_CFG);

	return 0;
}

static void meson_mmc_setup_cmd(struct mmc *mmc, struct mmc_data *data,
				struct mmc_cmd *cmd)
{
	uint32_t meson_mmc_cmd = 0, cfg, bl_len = 0;

	meson_mmc_cmd |= cmd->cmdidx << CMD_CFG_CMD_INDEX_SHIFT;

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136)
			meson_mmc_cmd |= CMD_CFG_RESP_128;

		if (cmd->resp_type & MMC_RSP_BUSY)
			meson_mmc_cmd |= CMD_CFG_R1B;

		if (!(cmd->resp_type & MMC_RSP_CRC))
			meson_mmc_cmd |= CMD_CFG_RESP_NOCRC;
	} else {
		meson_mmc_cmd |= CMD_CFG_NO_RESP;
	}

	if (data) {
		cfg = meson_read(mmc, MESON_SD_EMMC_CFG);
		bl_len = (cfg & CFG_BL_LEN_MASK) >> CFG_BL_LEN_SHIFT;
		if (bl_len != ilog2(data->blocksize)) {
			cfg &= ~CFG_BL_LEN_MASK;
			cfg |= ilog2(data->blocksize) << CFG_BL_LEN_SHIFT;
			meson_write(mmc, cfg, MESON_SD_EMMC_CFG);
		}

		if (data->flags == MMC_DATA_WRITE)
			meson_mmc_cmd |= CMD_CFG_DATA_WR;

		meson_mmc_cmd |= CMD_CFG_DATA_IO;
		if (data->blocks > 1) {
			meson_mmc_cmd |= CMD_CFG_BLOCK_MODE;
			meson_mmc_cmd |= data->blocks;
		} else
			meson_mmc_cmd |= data->blocksize;
	}

	meson_mmc_cmd |= CMD_CFG_TIMEOUT_4S | CMD_CFG_OWNER |
			 CMD_CFG_END_OF_CHAIN;

	meson_write(mmc, meson_mmc_cmd, MESON_SD_EMMC_CMD_CFG);
}

static void meson_mmc_setup_addr(struct mmc *mmc, struct mmc_data *data)
{
	struct meson_mmc_platdata *pdata = mmc->priv;
	unsigned int data_size;
	uint32_t data_addr = 0;

	if (data) {
		data_size = data->blocks * data->blocksize;

		if (data->flags == MMC_DATA_READ) {
			data_addr = (ulong) data->dest;
			invalidate_dcache_range(data_addr,
						data_addr + data_size);
		} else {
			pdata->w_buf = calloc(data_size, sizeof(char));
			data_addr = (ulong) pdata->w_buf;
			memcpy(pdata->w_buf, data->src, data_size);
			flush_dcache_range(data_addr, data_addr + data_size);
		}
	}

	meson_write(mmc, data_addr, MESON_SD_EMMC_CMD_DAT);
}

static void meson_mmc_read_response(struct mmc *mmc, struct mmc_cmd *cmd)
{
	if (cmd->resp_type & MMC_RSP_136) {
		cmd->response[0] = meson_read(mmc, MESON_SD_EMMC_CMD_RSP3);
		cmd->response[1] = meson_read(mmc, MESON_SD_EMMC_CMD_RSP2);
		cmd->response[2] = meson_read(mmc, MESON_SD_EMMC_CMD_RSP1);
		cmd->response[3] = meson_read(mmc, MESON_SD_EMMC_CMD_RSP);
	} else {
		cmd->response[0] = meson_read(mmc, MESON_SD_EMMC_CMD_RSP);
	}
}

static void meson_mmc_clear_response(unsigned * res_buf)
{
	int i;
	if (res_buf == NULL)
		return;

	for (i = 0; i < MAX_RESPONSE_BYTES; i++)
		res_buf[i]=0;
}

static int mmc_pre_dma(struct udevice *dev, struct mmc_data *data,
		struct sd_emmc_desc_info *desc_cur)
{
	struct meson_mmc_platdata *pdata = dev_get_platdata(dev);
	struct mmc *mmc = &pdata->mmc;
	uint32_t *meson_mmc_cmd = NULL;
	unsigned int blks = 0, desc_cnt = 0, bl_len = 0;
	uint32_t data_addr = 0;

	meson_mmc_cmd = &(desc_cur->cmd_info);
	if (data->flags == MMC_DATA_WRITE)
		data_addr = (ulong)pdata->w_buf;
	else
		data_addr = (ulong)data->dest;

	if ((data->blocks > 1)
			|| (data->blocksize >= MMC_MAX_BLOCK_LEN)) {
		blks = data->blocks;
		while (blks) {
			meson_mmc_cmd = &(desc_cur->cmd_info);
			*meson_mmc_cmd |= CMD_CFG_BLOCK_MODE;
			bl_len = (blks > mmc->cfg->b_max) ?
				mmc->cfg->b_max : blks;
			*meson_mmc_cmd &= ~CMD_CFG_LENGTH_MASK;
			*meson_mmc_cmd |= bl_len;
			blks -= bl_len;

			if (desc_cnt != 0) {
				*meson_mmc_cmd |= CMD_CFG_NO_RESP;
				*meson_mmc_cmd |= CMD_CFG_NO_CMD;
			}
			*meson_mmc_cmd &= ~CMD_CFG_RESP_NUM;
			*meson_mmc_cmd |= CMD_CFG_DATA_IO;
			*meson_mmc_cmd |= CMD_CFG_OWNER;
			*meson_mmc_cmd &= ~CMD_CFG_DATA_WR;
			if (data->flags == MMC_DATA_WRITE)
				*meson_mmc_cmd |= CMD_CFG_DATA_WR;
			*meson_mmc_cmd |= CMD_CFG_TIMEOUT_4S;
			desc_cur->data_addr = data_addr
				+ (desc_cnt * bl_len * mmc->read_bl_len);
			desc_cur->data_addr &= ~(1 << 0);
			if (blks) {
				desc_cur++;
				desc_cnt++;
				memset(desc_cur, 0, sizeof(struct sd_emmc_desc_info));
			}
		}
	} else {
		*meson_mmc_cmd &= ~CMD_CFG_BLOCK_MODE;
		*meson_mmc_cmd |= data->blocksize;
		desc_cur->data_addr = data_addr;
		desc_cur->data_addr &= ~(1 << 0);
	}

	return desc_cnt;
}

static int mmc_setup_data(struct udevice *dev, struct mmc_data *data,
		struct sd_emmc_desc_info *desc_cur)
{
	struct meson_mmc_platdata *pdata = dev_get_platdata(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = &pdata->mmc;
	uint32_t *meson_mmc_cmd = NULL;
	unsigned int data_size, blks = 0, desc_cnt = 0, bl_len = 0;
	uint32_t data_addr = 0;

	meson_mmc_cmd = &(desc_cur->cmd_info);
	*meson_mmc_cmd |= CMD_CFG_DATA_IO;
	*meson_mmc_cmd &= ~CMD_CFG_DATA_NUM;
	*meson_mmc_cmd &= ~CMD_CFG_LENGTH_MASK;

	data_size = data->blocks * data->blocksize;
	if (data->flags == MMC_DATA_WRITE) {
		*meson_mmc_cmd |= CMD_CFG_DATA_WR;
		pdata->w_buf = (u32 *)malloc(data_size);
		memset(pdata->w_buf, 0, data_size);
		memcpy(pdata->w_buf, (u32 *)data->src, data_size);
		flush_dcache_range((ulong)pdata->w_buf,
				(ulong)(pdata->w_buf + data_size));
		data_addr = (ulong)pdata->w_buf;
	} else {
		*meson_mmc_cmd &= ~CMD_CFG_DATA_WR;
		data_addr = (ulong)data->dest;
		invalidate_dcache_range(data_addr,
				data_addr + data_size);
	}

	desc_cnt = mmc_pre_dma(dev, data, desc_cur);

	return desc_cnt;
}

static void mmc_setup_desc(struct udevice *dev, struct mmc_cmd *cmd,
		struct mmc_data *data, struct sd_emmc_desc_info *desc_cur)
{
	struct meson_mmc_platdata *pdata = dev_get_platdata(dev);
	struct mmc *mmc = &pdata->mmc;
	uint32_t *meson_mmc_cmd = NULL;
	uint32_t cfg = 0, bl_len = 0, resp_addr;
	unsigned int desc_cnt = 0;

	meson_mmc_cmd = &(desc_cur->cmd_info);
	*meson_mmc_cmd |= cmd->cmdidx << CMD_CFG_CMD_INDEX_SHIFT;
	desc_cur->cmd_arg |= cmd->cmdarg;

	*meson_mmc_cmd |= CMD_CFG_OWNER;
	*meson_mmc_cmd &= ~CMD_CFG_ERR;
	*meson_mmc_cmd &= ~CMD_CFG_END_OF_CHAIN;

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		resp_addr = (unsigned long)cmd->response;
		*meson_mmc_cmd &= ~CMD_CFG_NO_RESP;
		if (cmd->resp_type & MMC_RSP_136)
			*meson_mmc_cmd |= CMD_CFG_RESP_128;

		if (cmd->resp_type & MMC_RSP_BUSY)
			*meson_mmc_cmd |= CMD_CFG_R1B;

		if (!(cmd->resp_type & MMC_RSP_CRC))
			*meson_mmc_cmd |= CMD_CFG_RESP_NOCRC;

		*meson_mmc_cmd &= ~CMD_CFG_RESP_NUM;
		desc_cur->resp_addr = resp_addr;
	} else {
		*meson_mmc_cmd |= CMD_CFG_NO_RESP;
	}

	if (data) {
		cfg = meson_read(mmc, MESON_SD_EMMC_CFG);
		bl_len = (cfg & CFG_BL_LEN_MASK) >> CFG_BL_LEN_SHIFT;
		if (bl_len != ilog2(data->blocksize)) {
			cfg &= ~CFG_BL_LEN_MASK;
			cfg |= ilog2(data->blocksize) << CFG_BL_LEN_SHIFT;
			meson_write(mmc, cfg, MESON_SD_EMMC_CFG);
		}

		desc_cnt = mmc_setup_data(dev, data, desc_cur);
		if (desc_cnt)
			desc_cur += desc_cnt;
	} else {
		*meson_mmc_cmd &= ~CMD_CFG_DATA_IO;
	}

	meson_mmc_cmd = &(desc_cur->cmd_info);
	*meson_mmc_cmd |= CMD_CFG_TIMEOUT_4S;
	*meson_mmc_cmd |= CMD_CFG_END_OF_CHAIN;
}

static int meson_mmc_desc_transfer(struct udevice *dev, struct mmc_cmd *cmd,
				 struct mmc_data *data)
{
	struct meson_mmc_platdata *pdata = dev_get_platdata(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = &pdata->mmc;
	struct sd_emmc_desc_info *desc_cur = NULL;
	uint32_t start = 0;

	if (host->desc_buf == NULL)
		return -EINVAL;

	start &= ~CFG_DESC_BUSY;
	meson_write(mmc, start, MESON_SD_EMMC_START);

	desc_cur = (struct sd_emmc_desc_info*)host->desc_buf;
	memset(desc_cur, 0, sizeof(struct sd_emmc_desc_info));
	meson_mmc_clear_response(cmd->response);

	mmc_setup_desc(dev, cmd, data, desc_cur);

	meson_write(mmc, STATUS_MASK, MESON_SD_EMMC_STATUS);
	invalidate_dcache_range((unsigned long)host->desc_buf,
			(unsigned long)(host->desc_buf
				+ MMC_MAX_DESC_NUM * (sizeof(struct sd_emmc_desc_info))));

	start = 0;
	start &= ~CFG_DESC_INIT;
	start |= CFG_DESC_BUSY;
	start |= ((unsigned long)host->desc_buf >> 2) << CFG_DESC_ADDR;
	meson_write(mmc, start, MESON_SD_EMMC_START);

	return 0;
}

static int meson_dm_mmc_send_cmd(struct udevice *dev, struct mmc_cmd *cmd,
				 struct mmc_data *data)
{
	struct meson_mmc_platdata *pdata = dev_get_platdata(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = &pdata->mmc;
	uint32_t status;
	ulong start;
	int ret = 0;

	/* max block size supported by chip is 512 byte */
	if (data && data->blocksize > MMC_MAX_BLOCK_LEN)
		return -EINVAL;

	ret = meson_mmc_desc_transfer(dev, cmd, data);

	/* use 10s timeout */
	start = get_timer(0);
	do {
		status = meson_read(mmc, MESON_SD_EMMC_STATUS);
	} while(!(status & STATUS_END_OF_CHAIN) && get_timer(start) < 10000);

	meson_mmc_read_response(mmc, cmd);

	ret = mmc_controller_debug(dev, cmd,status);
	if (data && data->flags == MMC_DATA_WRITE)
		free(pdata->w_buf);

	if (ret) {
			if (status & STATUS_RESP_TIMEOUT)
				return -ETIMEDOUT;
			else
				return ret;
	}
	return ret;
}

void meson_hw_reset(struct udevice *dev)
{
	struct meson_mmc_platdata *pdata = dev_get_platdata(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = &pdata->mmc;

	if (!strcmp(mmc->cfg->name, "emmc")) {
		dm_gpio_set_value(&host->gpio_reset, 0);
		mdelay(2);
		dm_gpio_set_value(&host->gpio_reset, 1);
		mdelay(2);
	}
}

int meson_get_cd(struct udevice *dev)
{
	struct meson_mmc_platdata *pdata = dev_get_platdata(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = &pdata->mmc;
	int ret = 0;

	if (!strcmp(mmc->cfg->name, "sd")) {
		ret = dm_gpio_get_value(&host->gpio_cd);
		if (ret < 0)
			pr_err("card detect get failed!\n");
		host->is_in = !ret;
	}

	return host->is_in;
}

int meson_send_cali_blks(struct udevice *dev, char *buffer, u32 start_blk, u32 cnt)
{
	struct meson_mmc_platdata *pdata = dev_get_platdata(dev);
	struct mmc *mmc = &pdata->mmc;
	struct meson_host *host = dev_get_priv(dev);
	int err = 0, ret = 0;
	struct mmc_cmd cmd = {0};
	struct mmc_cmd stop = {0};
	struct mmc_data data = {{0}, 0};

	stop.cmdidx = MMC_CMD_STOP_TRANSMISSION;
	stop.cmdarg = 0;
	stop.resp_type = MMC_RSP_R1b;

	if (cnt > 1)
		cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;
	cmd.cmdarg = start_blk;
	cmd.resp_type = MMC_RSP_R1;

	data.dest = buffer;
	data.blocks = cnt;
	data.blocksize = mmc->read_bl_len;
	data.flags = MMC_DATA_READ;
	memset(buffer, 0, data.blocks * data.blocksize);

	err = meson_dm_mmc_send_cmd(dev, &cmd, &data);
	if (err)
		pr_debug("%s: send calibration read blocks error %d cnt = %d\n",
				mmc->cfg->name, err, cnt);
	if (cnt > 1 || err) {
		ret = meson_dm_mmc_send_cmd(mmc, &stop, NULL);
		if (ret)
			pr_debug("%s: send calibration stop blocks error %d\n",
					mmc->cfg->name, ret);
	}
	if (ret || err)
		return -1;
	return 0;
}

u32 meson_tuning_transfer(struct udevice *dev, u32 opcode)
{
	struct meson_mmc_platdata *pdata = dev_get_platdata(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = &pdata->mmc;
	u32 vctrl = meson_read(mmc, MESON_SD_EMMC_CFG);
	u32 tuning_err = 0, start_blk = CALI_PATTERN_ADDR;
	int cmd_err = 0, n, nmatch;

	for (n = 0, nmatch = 0; n < TUNING_NUM_PER_POINT; n++) {
		if ((opcode == MMC_CMD_SEND_TUNING_BLOCK_HS200)
		   || (opcode == MMC_CMD_SEND_TUNING_BLOCK))
			tuning_err = mmc_send_tuning(mmc, opcode, &cmd_err);
		else {
			start_blk = CALI_PATTERN_ADDR;
			tuning_err = meson_send_cali_blks(dev,
					host->blk_test, start_blk, REFIX_BLK_CNT);
		}
		if (!tuning_err) {
			nmatch++;
		} else {
			pr_err("Tuning transfer error: nmatch=%d tuning_err:0x%x\n",
					nmatch, tuning_err);
			break;
		}
	}
	return nmatch;
}

int sd_emmc_test_adj(struct udevice *dev)
{
	struct meson_mmc_platdata *pdata = dev_get_platdata(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = &pdata->mmc;
	int err = 0, ret = 0;
	struct mmc_cmd cmd = {0};
	struct mmc_cmd stop = {0};
	struct mmc_data data = {{0}, 0};
	u32 vclk, clk_div, retry, adj, adj_dly;

	vclk = meson_read(mmc, MESON_SD_EMMC_CLOCK);
	adj = meson_read(mmc, MESON_SD_EMMC_ADJUST);
	clk_div = vclk & CLK_MAX_DIV;
	retry = clk_div;

	stop.cmdidx = MMC_CMD_STOP_TRANSMISSION;
	stop.cmdarg = 0;
	stop.resp_type = MMC_RSP_R1b;

	cmd.cmdarg = 0x14000;
	cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	cmd.resp_type = MMC_RSP_R1;

	data.dest = host->blk_test;
	data.blocks = 0x400;
	data.blocksize = mmc->read_bl_len;
	data.flags = MMC_DATA_READ;
	memset(host->blk_test, 0, (data.blocks * data.blocksize));

__Retry:
	err = meson_dm_mmc_send_cmd(dev, &cmd, &data);
	if (err)
		pr_err("%s: send test read blocks error %d\n", mmc->cfg->name, err);
	ret = meson_dm_mmc_send_cmd(dev, &stop, NULL);
	if (ret)
		pr_err("%s: send test stop blocks error %d\n", mmc->cfg->name, ret);

	if (ret || err) {
		if (retry == 0) {
			return 1;
		}
		retry--;
		adj = meson_read(mmc, MESON_SD_EMMC_ADJUST);
		adj_dly = ((adj & ADJ_DLY_MASK) >> Cfg_adj_dly);
		adj_dly--;
		adj &= ~ADJ_DLY_MASK;
		adj |= (adj_dly << Cfg_adj_dly);
		meson_write(mmc, adj, MESON_SD_EMMC_ADJUST);
		pr_err("adj retry sampling point:(%d)->(%d)",
				adj_dly + 1, adj_dly);
		pr_err("%s: dly1 = 0x%x, dly2 = %x, gadjust =0x%x\n",
				mmc->cfg->name,
				meson_read(mmc, MESON_SD_EMMC_DELAY1),
				meson_read(mmc, MESON_SD_EMMC_DELAY2),
				meson_read(mmc, MESON_SD_EMMC_ADJUST));
		goto __Retry;
	}
	return 0;
}

int meson_execute_tuning(struct udevice *dev, uint opcode)
{
	struct meson_mmc_platdata *pdata = dev_get_platdata(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = &pdata->mmc;
	u32 clk_src, clock;
	u32 vclk = 0, clk_div = 0, adj = 0, dly = 0, d1_dly, old_dly;
	int err = 0, ret = 0, adj_delay = 0;
	int tuning_num = 0;
#ifdef MMC_HS200_MODE
	int pre_status = 0;
	int start = 0;
#endif
	int n, nmatch;
	int wrap_win_start = -1, wrap_win_size = 0;
	int best_win_start = -1, best_win_size = -1;
	int curr_win_start = -1, curr_win_size = 0;
	u8 rx_tuning_result[25] = { 0 };

	if (host->blk_test == NULL)
		return -EINVAL;

	meson_write(mmc, 0, MESON_SD_EMMC_ADJUST);
	old_dly = meson_read(mmc, MESON_SD_EMMC_DELAY1);
	d1_dly = (old_dly & DLY_D1_MASK) >> Dly_d1;
	printf("Data 1 aligned delay is %d\n", d1_dly);

tuning:
	wrap_win_start = -1;
	wrap_win_size = 0;
	best_win_start = -1;
	best_win_size = 0;
	curr_win_start = -1;
	curr_win_size = 0;

	vclk = meson_read(mmc, MESON_SD_EMMC_CLOCK);
	clk_div = vclk & CLK_MAX_DIV;
	if (!(vclk & CLK_MAX_SRC))
		clk_src = 24000000;
	else
		clk_src = 1000000000;
	clock = (clk_src / clk_div);
	printf("%s: clk %d tuning start:\n", mmc->cfg->name, clock);

	host->is_tuning = 1;
	for (adj_delay = 0; adj_delay < clk_div; adj_delay++) {
		// Perform tuning ntries times per clk_div increment
		adj = 0;
		adj |= (adj_delay << Cfg_adj_dly);
		adj |= (1 << Cfg_adj_en);
		meson_write(mmc, adj, MESON_SD_EMMC_ADJUST);
		nmatch = meson_tuning_transfer(dev, opcode);
		if (adj_delay
				< ARRAY_SIZE(rx_tuning_result))
			rx_tuning_result[adj_delay] = nmatch;

		if (nmatch == TUNING_NUM_PER_POINT) {
			if (adj_delay == 0)
				wrap_win_start = adj_delay;

			if (wrap_win_start >= 0)
				wrap_win_size++;

			if (curr_win_start < 0)
				curr_win_start = adj_delay;

			curr_win_size++;
			printf("%s: rx_tuning_result[%d] = %d\n",
					mmc->cfg->name, adj_delay, nmatch);
		} else {
			if (curr_win_start >= 0) {
				if (best_win_start < 0) {
					best_win_start = curr_win_start;
					best_win_size = curr_win_size;
				}
				else {
					if (best_win_size < curr_win_size) {
						best_win_start = curr_win_start;
						best_win_size = curr_win_size;
					}
				}

				wrap_win_start = -1;
				curr_win_start = -1;
				curr_win_size = 0;
			}
		}
	}

	if (curr_win_start >= 0) {
		if (best_win_start < 0) {
			best_win_start = curr_win_start;
			best_win_size = curr_win_size;
		} else if (wrap_win_size > 0) {
			/* Wrap around case */
			if (curr_win_size + wrap_win_size > best_win_size) {
				best_win_start = curr_win_start;
				best_win_size = curr_win_size + wrap_win_size;
			}
		} else if (best_win_size < curr_win_size) {
			best_win_start = curr_win_start;
			best_win_size = curr_win_size;
		}

		curr_win_start = -1;
		curr_win_size = 0;
	}

	if (best_win_start < 0) {
		if ((tuning_num++ > MAX_TUNING_RETRY)
				|| (clk_div >= 10)) {
			pr_err("%s: final result of tuning failed\n",
					mmc->cfg->name);
			host->is_tuning = 0;
			return -1;
		}
		clk_div++;
		vclk &= ~CLK_MAX_DIV;
		vclk |= clk_div;
		meson_write(mmc, vclk, MESON_SD_EMMC_CLOCK);
		pr_err("%s: tuning failed, reduce freq and retuning\n",
				mmc->cfg->name);
		goto tuning;
	} else if (best_win_size == clk_div) {
		dly = meson_read(mmc, MESON_SD_EMMC_DELAY1);
		d1_dly = (dly & DLY_D1_MASK) >> Dly_d1;
		pr_warn("%s: d1_dly %d, window start %d, size %d\n",
				mmc->cfg->name, d1_dly, best_win_start, best_win_size);
		if (++d1_dly > 0x3F) {
			pr_err("%s: tuning failed\n", mmc->cfg->name);
			host->is_tuning = 0;
			return -1;
		}
		dly &= ~DLY_D1_MASK;
		dly |= d1_dly << Dly_d1;
		meson_write(mmc, dly, MESON_SD_EMMC_DELAY1);
		goto tuning;
	} else
		printf("%s: best_win_start =%d, best_win_size =%d\n",
				mmc->cfg->name, best_win_start, best_win_size);

	adj_delay = best_win_start + (best_win_size - 1) / 2
		+ (best_win_size - 1) % 2;
	adj_delay = adj_delay % clk_div;

	adj = 0;
	adj |= (adj_delay << Cfg_adj_dly);
	adj |= (1 << Cfg_adj_en);
	meson_write(mmc, adj, MESON_SD_EMMC_ADJUST);
	meson_write(mmc, old_dly, MESON_SD_EMMC_DELAY1);
	mmc_print_reg(dev);
#ifdef MMC_HS200_MODE
	for (n = 0; n < clk_div; n++) {
		if (n == clk_div - 1) {
			if ((rx_tuning_result[n] == TUNING_NUM_PER_POINT)
					&& (pre_status == 1))
				pr_debug("meson-mmc: emmc: [ %d -- %d ] is ok\n", start, n);
			else if ((rx_tuning_result[n] != TUNING_NUM_PER_POINT)
					&& (pre_status == -1))
				pr_debug("meson-mmc: emmc: [ %d -- %d ] is nok\n", start, n);
			else if ((rx_tuning_result[n] == TUNING_NUM_PER_POINT)
					&& (pre_status != 1))
				pr_debug("meson-mmc: emmc: [ %d ] is ok\n", n);
			else if ((rx_tuning_result[n] != TUNING_NUM_PER_POINT)
					&& (pre_status == 1))
				pr_debug("meson-mmc: emmc: [ %d ] is nok\n", n);
		}
		if (rx_tuning_result[n] == TUNING_NUM_PER_POINT) {
			if (pre_status == -1) {
				if (start == n - 1)
					pr_debug("meson-mmc: emmc: [ %d ] is nok\n", start);
				else
					pr_debug("meson-mmc: emmc: [ %d -- %d] is nok\n", start, n-1);
			} else if (pre_status == 1)
				continue;
			start = n;
			pre_status = 1;
		} else if (rx_tuning_result[n] != TUNING_NUM_PER_POINT) {
			if (pre_status == 1) {
				if (start == n - 1)
					pr_debug("meson-mmc: emmc: [ %d ] is ok\n", start);
				else
					pr_debug("meson-mmc: emmc: [ %d -- %d ] is ok\n", start, n-1);
			} else if (pre_status == -1)
				continue;
			start = n;
			pre_status = -1;
		}
	}

	mmc_print_reg(dev);
#endif
	host->is_tuning = 0;
	/* test adj sampling point*/
	if (0)
		ret = sd_emmc_test_adj(dev);

	return ret;
}

static const struct dm_mmc_ops meson_dm_mmc_ops = {
	.send_cmd = meson_dm_mmc_send_cmd,
	.set_ios = meson_dm_mmc_set_ios,
	.send_init_stream = meson_hw_reset,
	.get_cd = meson_get_cd,
#ifdef MMC_SUPPORTS_TUNING
	.execute_tuning = meson_execute_tuning,
#endif
};

static int meson_mmc_ofdata_to_platdata(struct udevice *dev)
{
	struct meson_mmc_platdata *pdata = dev_get_platdata(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc_config *cfg = &pdata->cfg;
	struct udevice *clk_udevice;
	fdt_addr_t addr;
	int ret = 0;

	addr = devfdt_get_addr(dev);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	pdata->regbase = (void *)addr;

	ret = mmc_of_parse(dev, cfg);
	if (ret)
		return ret;

	dev->name = dev_read_string(dev, "pinname");
	if (dev_read_bool(dev, "non-removable"))
		host->is_in = 1;

	if (!strcmp(dev->name, "sd")) {
		ret = gpio_request_by_name(dev,
				"cd-detect", 0, &host->gpio_cd, GPIOD_IS_IN);
		if (ret)
			return ret;
	}
	if (!strcmp(dev->name, "emmc")) {
		ret = gpio_request_by_name(dev,
				"hw_reset", 0, &host->gpio_reset, GPIOD_IS_OUT);
		if (ret)
			return ret;
		dm_gpio_set_value(&host->gpio_reset, 1);
	}

	uclass_get_device_by_name(UCLASS_CLK, "amlogic,g12a-clkc", &clk_udevice);

	clk_get_by_name(dev, "core", &host->core);
	clk_get_by_name(dev, "clkin0", &host->xtal);
	clk_get_by_name(dev, "clkin1", &host->div2);
	clk_get_by_name(dev, "mux", &host->mux);
	clk_get_by_name(dev, "div", &host->div);
	clk_get_by_name(dev, "gate", &host->gate);

	clk_enable(&host->core);
	clk_enable(&host->gate);

	return 0;
}

static int meson_mmc_probe(struct udevice *dev)
{
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct meson_mmc_platdata *pdata = dev_get_platdata(dev);
	struct meson_host *host = dev_get_priv(dev);
	struct mmc *mmc = &pdata->mmc;
	struct mmc_config *cfg = &pdata->cfg;
	uint32_t val;
	int ret = -EINVAL;

	cfg->voltages = MMC_VDD_33_34 | MMC_VDD_32_33 |
		MMC_VDD_31_32 | MMC_VDD_165_195;
	cfg->f_min = 400000;
	cfg->b_max = 511; /* max 512 - 1 blocks */
	cfg->name = dev->name;

	host->mmc = &pdata->mmc;
	if (host->blk_test == NULL)
		host->blk_test = malloc(MMC_MAX_BLOCK_LEN * CALI_BLK_CNT);
	if (!host->blk_test) {
		ret = -ENOMEM;
		goto err;
	}
	if (host->desc_buf == NULL)
		host->desc_buf
			= malloc(MMC_MAX_DESC_NUM * (sizeof(struct sd_emmc_desc_info)));
	if (!host->desc_buf) {
		ret = -ENOMEM;
		goto err;
	}
	mmc->priv = pdata;
	upriv->mmc = mmc;

	mmc_set_clock(mmc, cfg->f_min, false);

	/* reset all status bits */
	meson_write(mmc, STATUS_MASK, MESON_SD_EMMC_STATUS);

	/* disable interrupts */
	meson_write(mmc, 0, MESON_SD_EMMC_IRQ_EN);

	/* enable auto clock mode */
	val = meson_read(mmc, MESON_SD_EMMC_CFG);
	val &= ~CFG_SDCLK_ALWAYS_ON;
	val |= CFG_AUTO_CLK;
	meson_write(mmc, val, MESON_SD_EMMC_CFG);
	pr_info("%s: probe success!\n", mmc->cfg->name);

	return 0;
err:
	pr_err("%s: probe fail, ret = %d!\n", mmc->cfg->name, ret);
	if (host->blk_test)
		free(host->blk_test);
	if (host->desc_buf)
		free(host->desc_buf);
	return ret;
}

int meson_mmc_bind(struct udevice *dev)
{
	struct meson_mmc_platdata *pdata = dev_get_platdata(dev);

	return mmc_bind(dev, &pdata->mmc, &pdata->cfg);
}

static const struct udevice_id meson_mmc_match[] = {
	{ .compatible = "amlogic,meson-gx-mmc" },
	{ .compatible = "amlogic,meson-axg-mmc" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(meson_mmc) = {
	.name = "meson_gx_mmc",
	.id = UCLASS_MMC,
	.of_match = meson_mmc_match,
	.ofdata_to_platdata = meson_mmc_ofdata_to_platdata,
	.bind = meson_mmc_bind,
	.probe = meson_mmc_probe,
	.ops = &meson_dm_mmc_ops,
	.platdata_auto_alloc_size = sizeof(struct meson_mmc_platdata),
	.priv_auto_alloc_size = sizeof(struct meson_host),
};
