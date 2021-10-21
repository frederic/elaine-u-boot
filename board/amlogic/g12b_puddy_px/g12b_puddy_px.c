
/*
 * board/amlogic/g12b_puddy_px/g12b_puddy_px.c
 *
 * Copyright (C) 2019 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <environment.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <asm/arch/cpu_id.h>
#ifdef CONFIG_SYS_I2C_MESON
#include <i2c.h>
#include <dt-bindings/i2c/meson-i2c.h>
#endif
#include <asm/arch/secure_apb.h>
#include <asm/arch/pinctrl_init.h>
#ifdef CONFIG_AML_VPU
#include <amlogic/media/vpu/vpu.h>
#endif
#ifdef CONFIG_AML_VPP
#include <amlogic/media/vpp/vpp.h>
#endif
#ifdef CONFIG_AML_V2_FACTORY_BURN
#include <amlogic/aml_v2_burning.h>
#endif// #ifdef CONFIG_AML_V2_FACTORY_BURN
#ifdef CONFIG_AML_HDMITX
#include <amlogic/media/vout/hdmitx.h>
#endif
#ifdef CONFIG_AML_LCD
#include <amlogic/media/vout/lcd/aml_lcd.h>
#endif
#include <asm/arch/eth_setup.h>
#include <phy.h>
#include <linux/mtd/partitions.h>
#include <linux/sizes.h>
#include <asm-generic/gpio.h>
#include <dm.h>
#ifdef CONFIG_AML_SPIFC
#include <amlogic/spifc.h>
#endif
#ifdef CONFIG_AML_SPICC
#include <amlogic/spicc.h>
#endif
#include <asm/armv8/mmu.h>

DECLARE_GLOBAL_DATA_PTR;

//new static eth setup
struct eth_board_socket*  eth_board_skt;

int do_get_ddictype (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#if defined(CONFIG_NAND_FTS) || defined(CONFIG_MMC_FTS)
#if defined(CONFIG_NAND_FTS)
	extern bool amlnf_is_inited(void);
	if (!amlnf_is_inited()) {
#endif

#if defined(CONFIG_MMC_FTS)
	extern bool amlmmc_is_inited(void);
	if (!amlmmc_is_inited()) {
#endif
		env_set("ddic_type", "FITI");
		return 0;
	}

	flash_ts_init();

	const char *fts_key = "ddic.type";

	char fts_value[256] = { 0 };
	flash_ts_get(fts_key, fts_value, sizeof(fts_value));

	pr_info("FTS read: ddic.type -> %s\n", fts_value);

	if (strncmp(fts_value, "NOVATEK", sizeof(fts_value)) == 0) {
		env_set("ddic_type", "NOVATEK");
	} else {
		env_set("ddic_type", "FITI");
	}
#endif
	return 0;
}

U_BOOT_CMD(
	get_ddictype, 1, 0, do_get_ddictype,
	"get ddic type",
	"  This command will get and setenv 'ddic_type'\n"
);

// Disable pull-up resistor GPIOH_3 so the mute switch status can be read.
// Disable pull-up resistors GPIOA_11 & GPIOA_12 so the HW ID can be read.
// The other HW ID GPIOS (GPIOH_0, GPIOH_1, GPIOH_2) are OD pins.
// Disable pull-up resistor for GPIOA0_1 for UART.
static void gpio_disable_pullup(void)
{
	int ret;

	ret = readl(PAD_PULL_UP_EN_REG5);
	writel(ret & (~(1 << 11)), PAD_PULL_UP_EN_REG5);

	ret = readl(PAD_PULL_UP_EN_REG5);
	writel(ret & (~(1 << 12)), PAD_PULL_UP_EN_REG5);

	ret = readl(PAD_PULL_UP_EN_REG3);
	writel(ret & (~(1 << 3)), PAD_PULL_UP_EN_REG3);

	// UART RX (GPIOAO_1)
	ret = readl(AO_RTI_PULL_UP_REG);
	writel(ret & (~(1 << 1)), AO_RTI_PULL_UP_REG);
}

int do_get_puddy_hw_id(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	unsigned int hw_id = 0, ret = 0;
	char hw_id_str[8] = {0};  // eg: 0x0A

	// Reading from highest bit to lowest bit
	// HW_ID_4: GPIOH_2
	ret = readl(P_PREG_PAD_GPIO3_I);
	hw_id |= (ret & (1<<2)) >> 2;
	hw_id = hw_id << 1;

	// HW_ID_3: GPIOH_1
	ret = readl(P_PREG_PAD_GPIO3_I);
	hw_id |= (ret & (1<<1)) >> 1;
	hw_id = hw_id << 1;

	// HW_ID_2: GPIOH_0
	ret = readl(P_PREG_PAD_GPIO3_I);
	hw_id |= (ret & 1);
	hw_id = hw_id << 1;

	// HW_ID_1: GPIOA_12
	ret = readl(P_PREG_PAD_GPIO5_I);
	hw_id |= (ret & (1<<12)) >> 12;
	hw_id = hw_id << 1;

	// HW_ID_0: GPIOA_11
	ret = readl(P_PREG_PAD_GPIO5_I);
	hw_id |= (ret & (1<<11)) >> 11;

	snprintf(hw_id_str, sizeof(hw_id_str), "0x%02x", hw_id);
	env_set("hw_id", hw_id_str);
	return 0;
}

U_BOOT_CMD(
	get_puddy_hw_id, 1, 0, do_get_puddy_hw_id,
	"get puddy's HW_ID and setenv 'hw_id'\n",
	"get_puddy_hw_id"
);

int serial_set_pin_port(unsigned long port_base)
{
    //UART in "Always On Module"
    //GPIOAO_0==tx,GPIOAO_1==rx
    //setbits_le32(P_AO_RTI_PIN_MUX_REG,3<<11);
    return 0;
}

#define SOC_ADCMUX_SEL "GPIOA_0"
static void soc_adcmux_sel(bool enable)
{
	int ret;
	struct gpio_desc soc_adcmux_desc;

	ret = dm_gpio_lookup_name("gpioa_0", &soc_adcmux_desc);
	if (ret) {
		printf("%s: not found\n", "gpioa_0");
		return ret;
	}

	ret = dm_gpio_request(&soc_adcmux_desc, SOC_ADCMUX_SEL);
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %s failed\n", SOC_ADCMUX_SEL);
		return;
	}
	ret = dm_gpio_set_dir_flags(&soc_adcmux_desc, GPIOD_IS_OUT);
	if (ret) {
		printf("set direction failed\n");
		return ret;
	}

	dm_gpio_set_value(&soc_adcmux_desc, enable ? 1 : 0);
}

#define USB_HOST_ENABLE_PIN_NAME "GPIOA_13"
static void enable_usb_host(bool enable)
{
	int ret;
	struct gpio_desc usb_host_desc;

	ret = dm_gpio_lookup_name("gpioa_13", &usb_host_desc);
	if (ret) {
		printf("%s: not found\n", "gpioa_13");
		return ret;
	}

	ret = dm_gpio_request(&usb_host_desc, USB_HOST_ENABLE_PIN_NAME);
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %s failed\n", USB_HOST_ENABLE_PIN_NAME);
		return;
	}
	ret = dm_gpio_set_dir_flags(&usb_host_desc, GPIOD_IS_OUT);
	if (ret) {
		printf("set direction failed\n");
		return ret;
	}

	dm_gpio_set_value(&usb_host_desc, enable ? 1 : 0);
}


//SOC_BL_ENABLE
//#define BL_ENABLE_PIN      GPIOEE(GPIOA_10)
#define BL_ENABLE_PIN_NAME "GPIOA_10"

static void enable_backlight(bool enable)
{
	int ret;
	struct gpio_desc bl_pow_desc;

	ret = dm_gpio_lookup_name("gpioa_10", &bl_pow_desc);
	if (ret) {
		printf("%s: not found\n", "gpioa_10");
		return ret;
	}

	ret = dm_gpio_request(&bl_pow_desc, BL_ENABLE_PIN_NAME);
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %s failed\n", BL_ENABLE_PIN_NAME);
		return;
	}
	//gpio_direction_output(BL_ENABLE_PIN, enable ? 1 : 0);
	ret = dm_gpio_set_dir_flags(&bl_pow_desc, GPIOD_IS_OUT);
	if (ret) {
		printf("set direction failed\n");
		return ret;
	}

	dm_gpio_set_value(&bl_pow_desc, enable ? 1 : 0);
}

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_1_SIZE;
	return 0;
}

/* secondary_boot_func
 * this function should be write with asm, here, is is only for compiling pass
 * */
void secondary_boot_func(void)
{
}
#ifdef  ETHERNET_INTERNAL_PHY
void internalPhyConfig(struct phy_device *phydev)
{
}

static int dwmac_meson_cfg_pll(void)
{
	writel(0x39C0040A, P_ETH_PLL_CTL0);
	writel(0x927E0000, P_ETH_PLL_CTL1);
	writel(0xAC5F49E5, P_ETH_PLL_CTL2);
	writel(0x00000000, P_ETH_PLL_CTL3);
	udelay(200);
	writel(0x19C0040A, P_ETH_PLL_CTL0);
	return 0;
}

static int dwmac_meson_cfg_analog(void)
{
	/*Analog*/
	writel(0x20200000, P_ETH_PLL_CTL5);
	writel(0x0000c002, P_ETH_PLL_CTL6);
	writel(0x00000023, P_ETH_PLL_CTL7);

	return 0;
}

static int dwmac_meson_cfg_ctrl(void)
{
	/*config phyid should between  a 0~0xffffffff*/
	/*please don't use 44000181, this has been used by internal phy*/
	writel(0x33000180, P_ETH_PHY_CNTL0);

	/*use_phy_smi | use_phy_ip | co_clkin from eth_phy_top*/
	writel(0x260, P_ETH_PHY_CNTL2);

	writel(0x74043, P_ETH_PHY_CNTL1);
	writel(0x34043, P_ETH_PHY_CNTL1);
	writel(0x74043, P_ETH_PHY_CNTL1);
	return 0;
}

static void setup_net_chip(void)
{
	eth_aml_reg0_t eth_reg0;

	eth_reg0.d32 = 0;
	eth_reg0.b.phy_intf_sel = 4;
	eth_reg0.b.rx_clk_rmii_invert = 0;
	eth_reg0.b.rgmii_tx_clk_src = 0;
	eth_reg0.b.rgmii_tx_clk_phase = 0;
	eth_reg0.b.rgmii_tx_clk_ratio = 4;
	eth_reg0.b.phy_ref_clk_enable = 1;
	eth_reg0.b.clk_rmii_i_invert = 1;
	eth_reg0.b.clk_en = 1;
	eth_reg0.b.adj_enable = 1;
	eth_reg0.b.adj_setup = 0;
	eth_reg0.b.adj_delay = 9;
	eth_reg0.b.adj_skew = 0;
	eth_reg0.b.cali_start = 0;
	eth_reg0.b.cali_rise = 0;
	eth_reg0.b.cali_sel = 0;
	eth_reg0.b.rgmii_rx_reuse = 0;
	eth_reg0.b.eth_urgent = 0;
	setbits_le32(P_PREG_ETH_REG0, eth_reg0.d32);// rmii mode

	dwmac_meson_cfg_pll();
	dwmac_meson_cfg_analog();
	dwmac_meson_cfg_ctrl();

	/* eth core clock */
	setbits_le32(HHI_GCLK_MPEG1, (0x1 << 3));
	/* eth phy clock */
	setbits_le32(HHI_GCLK_MPEG0, (0x1 << 4));

	/* eth phy pll, clk50m */
	setbits_le32(HHI_FIX_PLL_CNTL3, (0x1 << 5));

	/* power on memory */
	clrbits_le32(HHI_MEM_PD_REG0, (1 << 3) | (1<<2));
}
#endif

#ifdef ETHERNET_EXTERNAL_PHY

static int dwmac_meson_cfg_drive_strength(void)
{
	writel(0xaaaaaaa5, P_PAD_DS_REG4A);
	return 0;
}

static void setup_net_chip_ext(void)
{
	eth_aml_reg0_t eth_reg0;
	writel(0x11111111, P_PERIPHS_PIN_MUX_6);
	writel(0x111111, P_PERIPHS_PIN_MUX_7);

	eth_reg0.d32 = 0;
	eth_reg0.b.phy_intf_sel = 1;
	eth_reg0.b.rx_clk_rmii_invert = 0;
	eth_reg0.b.rgmii_tx_clk_src = 0;
	eth_reg0.b.rgmii_tx_clk_phase = 1;
	eth_reg0.b.rgmii_tx_clk_ratio = 4;
	eth_reg0.b.phy_ref_clk_enable = 1;
	eth_reg0.b.clk_rmii_i_invert = 0;
	eth_reg0.b.clk_en = 1;
	eth_reg0.b.adj_enable = 0;
	eth_reg0.b.adj_setup = 0;
	eth_reg0.b.adj_delay = 0;
	eth_reg0.b.adj_skew = 0;
	eth_reg0.b.cali_start = 0;
	eth_reg0.b.cali_rise = 0;
	eth_reg0.b.cali_sel = 0;
	eth_reg0.b.rgmii_rx_reuse = 0;
	eth_reg0.b.eth_urgent = 0;
	setbits_le32(P_PREG_ETH_REG0, eth_reg0.d32);// rmii mode

	setbits_le32(HHI_GCLK_MPEG1, 0x1 << 3);
	/* power on memory */
	clrbits_le32(HHI_MEM_PD_REG0, (1 << 3) | (1<<2));
}
#endif
extern struct eth_board_socket* eth_board_setup(char *name);
extern int designware_initialize(ulong base_addr, u32 interface);

int board_eth_init(bd_t *bis)
{
#ifdef CONFIG_ETHERNET_NONE
	return 0;
#endif

#ifdef ETHERNET_EXTERNAL_PHY
	dwmac_meson_cfg_drive_strength();
	setup_net_chip_ext();
#endif
#ifdef ETHERNET_INTERNAL_PHY
	setup_net_chip();
#endif
	udelay(1000);
	designware_initialize(ETH_BASE, PHY_INTERFACE_MODE_RMII);
	return 0;
}

#if CONFIG_AML_SD_EMMC
#include <mmc.h>
#include <asm/arch/sd_emmc.h>
static int  sd_emmc_init(unsigned port)
{
    switch (port)
	{
		case SDIO_PORT_A:
			break;
		case SDIO_PORT_B:
			//todo add card detect
			/* check card detect */
			clrbits_le32(P_PERIPHS_PIN_MUX_9, 0xF << 24);
			setbits_le32(P_PREG_PAD_GPIO1_EN_N, 1 << 6);
			setbits_le32(P_PAD_PULL_UP_EN_REG1, 1 << 6);
			setbits_le32(P_PAD_PULL_UP_REG1, 1 << 6);
			break;
		case SDIO_PORT_C:
			//enable pull up
			//clrbits_le32(P_PAD_PULL_UP_REG3, 0xff<<0);
			break;
		default:
			break;
	}

	return cpu_sd_emmc_init(port);
}

extern unsigned sd_debug_board_1bit_flag;


static void sd_emmc_pwr_prepare(unsigned port)
{
	cpu_sd_emmc_pwr_prepare(port);
}

static void sd_emmc_pwr_on(unsigned port)
{
    switch (port)
	{
		case SDIO_PORT_A:
			break;
		case SDIO_PORT_B:
//            clrbits_le32(P_PREG_PAD_GPIO5_O,(1<<31)); //CARD_8
//            clrbits_le32(P_PREG_PAD_GPIO5_EN_N,(1<<31));
			/// @todo NOT FINISH
			break;
		case SDIO_PORT_C:
			break;
		default:
			break;
	}
	return;
}
static void sd_emmc_pwr_off(unsigned port)
{
	/// @todo NOT FINISH
    switch (port)
	{
		case SDIO_PORT_A:
			break;
		case SDIO_PORT_B:
//            setbits_le32(P_PREG_PAD_GPIO5_O,(1<<31)); //CARD_8
//            clrbits_le32(P_PREG_PAD_GPIO5_EN_N,(1<<31));
			break;
		case SDIO_PORT_C:
			break;
				default:
			break;
	}
	return;
}

// #define CONFIG_TSD      1
static void board_mmc_register(unsigned port)
{
	struct aml_card_sd_info *aml_priv=cpu_sd_emmc_get(port);
    if (aml_priv == NULL)
		return;

	aml_priv->sd_emmc_init=sd_emmc_init;
	aml_priv->sd_emmc_detect=sd_emmc_detect;
	aml_priv->sd_emmc_pwr_off=sd_emmc_pwr_off;
	aml_priv->sd_emmc_pwr_on=sd_emmc_pwr_on;
	aml_priv->sd_emmc_pwr_prepare=sd_emmc_pwr_prepare;
	aml_priv->desc_buf = malloc(NEWSD_MAX_DESC_MUN*(sizeof(struct sd_emmc_desc_info)));

	if (NULL == aml_priv->desc_buf)
		printf(" desc_buf Dma alloc Fail!\n");
	else
		printf("aml_priv->desc_buf = 0x%p\n",aml_priv->desc_buf);

	sd_emmc_register(aml_priv);
}
int board_mmc_init(bd_t	*bis)
{
#if 0
#ifdef CONFIG_VLSI_EMULATOR
	//board_mmc_register(SDIO_PORT_A);
#else
	//board_mmc_register(SDIO_PORT_B);
#endif
	board_mmc_register(SDIO_PORT_B);
	board_mmc_register(SDIO_PORT_C);
//	board_mmc_register(SDIO_PORT_B1);
#endif
	return 0;
}
#endif

/* Skip the first line and parse one uint
 * Return negative if not found */
int parse_backlight_calibration_file_string(const char *str)
{
	if (str == NULL)
		return -EINVAL;

	str = strchr(str, '\n');
	if (str == NULL)
		return -EINVAL;
	++str;

	return simple_strtoul(str, NULL, 10);
}

/* If zero, this value has not been set and the default brightness should not be
 * overwritten. To store zero, store a non-zero value with its bottom 12 bits
 * all zero, like 0x1000.
 */
int get_persistent_brightness(void)
{
	u32 brightness_sticky_val = readl(AO_RTI_STICKY_REG2);
	if (brightness_sticky_val != 0)
		return (brightness_sticky_val & 0x0fff) >> 1;
	return -1;
}

int do_configure_backlight(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[])
{
#ifdef CONFIG_SYS_I2C_MESON
	int ret, i, attempt;
	const int retries = 3;
	char *addr_cal;
	int calibrated_current, persistent_brightness;

	/* Values to write:
	 * [0]:   Brightness register control only, backlight enabled
	 * [1-2]: Standby disabled, 23 mA MAX_CURRENT,
	 *        CURRENT scale = 20 * 4095 / 23
	 * [3]:   Enable undervoltage protection at 2.5 V, "disable" backlight
	 *        (i2c only), disable set resistors
	 * [4]:   6-phase, 6 drivers, 9.6kHz PWM rate
	 * [5]:   EN_DRV3, EN_DRV2, boost inductor current limit = 1.6 A
	 * [6]:   VBOOST_MAX = 25 V, JUMP_EN = 0
	 * [7]:   STEP_UP = 105 mV, STEP_DN = 105 mV, LED_FAULT_TH = 3V,
	 *        LED_COMP_HYST = DRIVER_HEADROOM + 750 mV
	 * [8-9]: 12-bit brightness (default: 33.3%)
	 * Important: Write brightness last to apply current calibration */
	const __u8 addrs[] = {0x01, 0xa0, 0xa1, 0xa2, 0xa5, 0xa7, 0xa9, 0xae,
		0x10, 0x11};
	__u8 values[] = {0x85, 0xd3, 0x4b, 0x20, 0x04, 0xf4, 0x60, 0x09,
		0x55, 0x05};
	const int n_bytes = sizeof(addrs) / sizeof(addrs[0]);
	struct udevice *bl_devp = NULL;

	if (argc > 2) {
		printf("%s: Too many args: %d\n", __func__, argc);
		return CMD_RET_USAGE;
	}

	if (argc == 2) {
		addr_cal = (char *)simple_strtoul(argv[1], NULL, 16);
		calibrated_current =
			parse_backlight_calibration_file_string(addr_cal);
		if (calibrated_current >= 0 && calibrated_current <= 4095) {
			/* CURRENT_LSB */
			values[1] = 0xff & calibrated_current;
			/* CURRENT_MSB */
			values[2] = (values[2] & 0xf0) |
				    (0x0f & (calibrated_current >> 8));
		}
	}

	// Apply persistent brightness if found
	persistent_brightness = get_persistent_brightness();
	if (persistent_brightness >= 0) {
		pr_info("Applying persistent_brightness=%d\n", persistent_brightness);
		/* Get LSB and MSB */
		values[n_bytes - 2] = persistent_brightness & 0xff;
		values[n_bytes - 1] = persistent_brightness >> 8 & 0x0f;
	} else {
		pr_info("Persistent_brightness not set\n");
	}

	enable_backlight(true);

	ret = i2c_get_chip_for_busnum(MESON_I2C_M3, 0X2c, 1, &bl_devp);
	if (ret) {
		printf("%s(%d):i2c get bus fail!\n", __func__, __LINE__);
	}

	for (i = 0; i < n_bytes; ++i) {
		for (attempt = 0; attempt < retries; ++attempt) {
			ret = dm_i2c_write(bl_devp, addrs[i], &values[i], 1);
			if (ret)
				printf("%s: Attempt=%d to write byte=0x%02x to reg=0x%02x of backlight failed\n",
					__func__, attempt, values[i], addrs[i]);
			else
				break;
		}
	 }
	 return ret;
#else
	 enable_backlight(true);
	 return 0;
#endif	/* CONFIG_SYS_I2C_MESON */
}

U_BOOT_CMD(
	configure_backlight, 2, 0, do_configure_backlight,
	"configures the lp8556 backlight",
	"Usage: configure_backlight [calibration_string_addr]\n"\
	"       Sets up required parameters of the backlight.\n"\
	"       Default calibration is 100%.\n"\
	"       calibration_str_addr (optional):\n"\
	"           Address of the calibration file string to parse.\n"
);

#ifdef CONFIG_AML_LCD
/* Parses one line of the gamma calibration file. */
/* Returns true on success. */
#define GAMMA_SIZE (256)
bool parse_gamma_string(const char *str, uint16_t *table)
{
	char buf[4] = {'\0'};
	int i;

	for (i = 0; i < GAMMA_SIZE; ++i) {
		strncpy(buf, &str[3 * i], 3);
		table[i] = simple_strtol(buf, NULL, 16);
	}

	return true;
}

/* Check header then parse RGB gamma tables */
/* Returns negative value on error */
int parse_gamma_calibration_file_string(const char *str, u16 *r, u16 *g, u16 *b)
{
	static const char supported_header_prefix[] = "Gamma Calibration 1.";
	static const int length = sizeof(supported_header_prefix) - 1;
	char *tables[3] = {r, g, b};
	const char *start, *end;
	int i;

	if (!str)
		return -EINVAL;

	// Check header prefix - only care about major version.
	if (strncmp(str, supported_header_prefix, length) != 0) {
		printf("Unknown gamma header: \"%.*s\"\n", length, str);
		return -EINVAL;
	}

	// Parse all three tables
	end = strchr(str, '\n');
	for (i = 0; i < 3; ++i) {
		start = end + 1;
		end = strchr(start, '\n');

		if ((end - start) / 3 != GAMMA_SIZE) {
			printf("Gamma table has invalid length.\n");
			return -EINVAL;
		}
		if (!parse_gamma_string(start, tables[i])) {
			printf("Could not parse gamma table %d.\n", i);
			return -EINVAL;
		}
	}

	return 0;
}
#endif  // CONFIG_AML_LCD

int do_configure_gamma(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
#ifdef CONFIG_AML_LCD
	static const int N_CHAN = 3;
	char *addr_cal;
	u16 gamma_tables[N_CHAN][GAMMA_SIZE];
	int i, j;

	if (argc == 1) {
		// Set to default
		for (i = 0; i < N_CHAN; ++i) {
			for (j = 0; j < GAMMA_SIZE; ++j)
				gamma_tables[i][j] = j << 2;  /* 10-bit */
		}
	} else if (argc == 2) {
		// Load from passed file string
		addr_cal = (char *)simple_strtoul(argv[1], NULL, 16);
		if (parse_gamma_calibration_file_string(addr_cal,
							&gamma_tables[0],
							&gamma_tables[1],
							&gamma_tables[2]) != 0)
			return -EINVAL;  // Error logged
	} else {
		return CMD_RET_USAGE;
	}

	vpp_set_rgb_gamma_table(&gamma_tables[0], &gamma_tables[1],
				&gamma_tables[2]);
#endif  // CONFIG_AML_LCD
	return 0;
}

U_BOOT_CMD(configure_gamma, 2, 0, do_configure_gamma,
	   "configures gamma the gamma tables with the provided calibration\n",
	   "Usage: configure_gamma [calibration_string_addr]\n"\
	   "       Applies the provided calibration file.\n"\
	   "       If a file is not provided, sets tables to default.\n"\
	   "       calibration_str_addr (optional):\n"\
	   "           Address of the calibration file string to parse.\n"
);

#if defined(CONFIG_BOARD_EARLY_INIT_F)
int board_early_init_f(void){
	/*add board early init function here*/
	return 0;
}
#endif

#ifdef CONFIG_USB_XHCI_AMLOGIC_V2
#include <asm/arch/usb-v2.h>
#include <asm/arch/gpio.h>
#define CONFIG_GXL_USB_U2_PORT_NUM	2

#ifdef CONFIG_USB_XHCI_AMLOGIC_USB3_V2
#define CONFIG_GXL_USB_U3_PORT_NUM	1
#else
#define CONFIG_GXL_USB_U3_PORT_NUM	0
#endif

static void gpio_set_vbus_power(char is_power_on)
{
	int ret;

	ret = gpio_request(CONFIG_USB_GPIO_PWR,
		CONFIG_USB_GPIO_PWR_NAME);
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n",
			CONFIG_USB_GPIO_PWR);
		return;
	}

	if (is_power_on) {
		gpio_direction_output(CONFIG_USB_GPIO_PWR, 1);
	} else {
		gpio_direction_output(CONFIG_USB_GPIO_PWR, 0);
	}
}

struct amlogic_usb_config g_usb_config_GXL_skt={
	CONFIG_GXL_XHCI_BASE,
	USB_ID_MODE_HARDWARE,
	gpio_set_vbus_power,//gpio_set_vbus_power, //set_vbus_power
	CONFIG_GXL_USB_PHY2_BASE,
	CONFIG_GXL_USB_PHY3_BASE,
	CONFIG_GXL_USB_U2_PORT_NUM,
	CONFIG_GXL_USB_U3_PORT_NUM,
	.usb_phy2_pll_base_addr = {
		CONFIG_USB_PHY_20,
		CONFIG_USB_PHY_21,
	}
};

#endif /*CONFIG_USB_XHCI_AMLOGIC*/

#ifdef CONFIG_AML_HDMITX20
static void hdmi_tx_set_hdmi_5v(void)
{
}
#endif

#ifdef CONFIG_AML_SPIFC
/*
 * BOOT_3: NOR_HOLDn:reg0[15:12]=3
 * BOOT_4: NOR_D:reg0[19:16]=3
 * BOOT_5: NOR_Q:reg0[23:20]=3
 * BOOT_6: NOR_C:reg0[27:24]=3
 * BOOT_7: NOR_WPn:reg0[31:28]=3
 * BOOT_14: NOR_CS:reg1[27:24]=3
 */
#define SPIFC_NUM_CS 1
static int spifc_cs_gpios[SPIFC_NUM_CS] = {54};

static int spifc_pinctrl_enable(void *pinctrl, bool enable)
{
	unsigned int val;

	val = readl(P_PERIPHS_PIN_MUX_0);
	val &= ~(0xfffff << 12);
	if (enable)
		val |= 0x33333 << 12;
	writel(val, P_PERIPHS_PIN_MUX_0);

	val = readl(P_PERIPHS_PIN_MUX_1);
	val &= ~(0xf << 24);
	writel(val, P_PERIPHS_PIN_MUX_1);
	return 0;
}

#if 0
static const struct spifc_platdata spifc_platdata = {
	.reg = 0xffd14000,
	.mem_map = 0xf6000000,
	.pinctrl_enable = spifc_pinctrl_enable,
	.num_chipselect = SPIFC_NUM_CS,
	.cs_gpios = spifc_cs_gpios,
};

U_BOOT_DEVICE(spifc) = {
	.name = "spifc",
	.platdata = &spifc_platdata,
};
#endif
#endif /* CONFIG_AML_SPIFC */

#if 0
#ifdef CONFIG_AML_SPICC
/* generic config in arch gpio/clock.c */
extern int spicc1_clk_set_rate(int rate);
extern int spicc1_clk_enable(bool enable);
extern int spicc1_pinctrl_enable(bool enable);

static const struct spicc_platdata spicc1_platdata = {
	.compatible = "amlogic,meson-g12a-spicc",
	.reg = (void __iomem *)0xffd15000,
	.clk_rate = 666666666,
	.clk_set_rate = spicc1_clk_set_rate,
	.clk_enable = spicc1_clk_enable,
	.pinctrl_enable = spicc1_pinctrl_enable,
	/* case one slave without cs: {"no_cs", 0} */
	.cs_gpio_names = {"GPIOH_6", 0},
};

U_BOOT_DEVICE(spicc1) = {
	.name = "spicc",
	.platdata = &spicc1_platdata,
};
#endif /* CONFIG_AML_SPICC */
#endif

extern void aml_pwm_cal_init(int mode);

int board_init(void)
{
	int ret = 0;
	unsigned char value = 0;
#ifdef CONFIG_SYS_I2C_MESON
	struct udevice *lcd_devp = NULL;
#endif
	//Please keep CONFIG_AML_V2_FACTORY_BURN at first place of board_init
	//As NOT NEED other board init If USB BOOT MODE
#ifdef CONFIG_AML_V2_FACTORY_BURN
	*(volatile uint32_t *)P_RESET1_LEVEL |= (3 << 16);
	if ((0x1b8ec003 != readl(P_PREG_STICKY_REG2)) && (0x1b8ec004 != readl(P_PREG_STICKY_REG2))) {
				aml_try_factory_usb_burning(0, gd->bd);
	}
#endif// #ifdef CONFIG_AML_V2_FACTORY_BURN
#ifdef CONFIG_USB_XHCI_AMLOGIC_V2
	board_usb_pll_disable(&g_usb_config_GXL_skt);
	board_usb_init(&g_usb_config_GXL_skt,BOARD_USB_MODE_HOST);
#endif /*CONFIG_USB_XHCI_AMLOGIC*/
	pinctrl_devices_active(PIN_CONTROLLER_NUM);
	enable_backlight(false);

#ifdef CONFIG_SYS_I2C_MESON
	//board_i2c_init();
	//GPIOZ_11 needs to be pulled down first to fit power on sequence
	/* clear GPIOZ_11 pinmux */
	ret = readl(P_PERIPHS_PIN_MUX_7);
	writel(ret & (~(0xf<<12)), P_PERIPHS_PIN_MUX_7);
	/* set GPIOZ_11 output mode */
	ret = readl(P_PREG_PAD_GPIO4_EN_N);
	writel(ret & (~(1<<11)), P_PREG_PAD_GPIO4_EN_N);
	/* set GPIOZ_11 output low */
	ret = readl(P_PREG_PAD_GPIO4_O);
	writel(ret & (~(1<<11)), P_PREG_PAD_GPIO4_O);
	udelay(1000);

	//set lcd bias voltage
	value = 0x0f;  // +/- 5.5V

	ret = i2c_get_chip_for_busnum(MESON_I2C_M3, 0X3E, 1, &lcd_devp);
	if (ret) {
		printf("%s(%d):i2c get bus fail!\n", __func__, __LINE__);
	}

	ret = dm_i2c_write(lcd_devp, 0x0, &value, 1); // VPOS
	if (ret)
		printf("Failed to set VPOS to 5.5V for LCD\n");

	ret = dm_i2c_write(lcd_devp, 0x1, &value, 1); // VNEG
	if (ret)
		printf("Failed to set VNEG to -5.5V for LCD\n");
#endif
#if 0
	aml_pwm_cal_init(0);
#endif//
#ifdef CONFIG_AML_NAND
	extern int amlnf_init(unsigned char flag);
	amlnf_init(0);
#endif

	gpio_disable_pullup();

	// Enable USB Host for Puddy px
	enable_usb_host(true);

	// Pull SOC_ADC_MUX_SEL High for Puddy px
	soc_adcmux_sel(true);
	return 0;
}

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	printf("board late init\n");
	run_command("mmc dev 1", 0);
	run_command("defenv", 0);
	run_command("run detect_panel", 0);
#if 0
		//update env before anyone using it
		run_command("get_rebootmode; echo reboot_mode=${reboot_mode}; "\
						"if test ${reboot_mode} = factory_reset; then "\
						"defenv_reserv;save; fi;", 0);
		run_command("if itest ${upgrade_step} == 1; then "\
						"defenv_reserv; setenv upgrade_step 2; saveenv; fi;", 0);
		/*add board late init function here*/
#ifndef DTB_BIND_KERNEL
		int ret;
		ret = run_command("store dtb read $dtb_mem_addr", 1);
        if (ret) {
				printf("%s(): [store dtb read $dtb_mem_addr] fail\n", __func__);
#ifdef CONFIG_DTB_MEM_ADDR
				char cmd[64];
				printf("load dtb to %x\n", CONFIG_DTB_MEM_ADDR);
				sprintf(cmd, "store dtb read %x", CONFIG_DTB_MEM_ADDR);
				ret = run_command(cmd, 1);
                if (ret) {
						printf("%s(): %s fail\n", __func__, cmd);
				}
#endif
		}
#elif defined(CONFIG_DTB_MEM_ADDR)
		{
				char cmd[128];
				int ret;
                if (!getenv("dtb_mem_addr")) {
						sprintf(cmd, "setenv dtb_mem_addr 0x%x", CONFIG_DTB_MEM_ADDR);
						run_command(cmd, 0);
				}
				sprintf(cmd, "imgread dtb boot ${dtb_mem_addr}");
				ret = run_command(cmd, 0);
                if (ret) {
						printf("%s(): cmd[%s] fail, ret=%d\n", __func__, cmd, ret);
				}
		}
#endif// #ifndef DTB_BIND_KERNEL

		/* load unifykey */
		run_command("keyunify init 0x1234", 0);
#endif
/*open vpu  hdmitx and cvbs driver*/
#ifdef CONFIG_AML_VPU
	vpu_probe();
#endif

#ifdef CONFIG_AML_VPP
	vpp_init();
#endif

#ifdef CONFIG_AML_HDMITX
	hdmi_tx_init();
#endif

#ifdef CONFIG_CMD_CVBS
	run_command("cvbs init", 0);
#endif

#ifdef CONFIG_AML_LCD
	lcd_probe();
#endif

#if 0
	/**/
	aml_config_dtb();
#endif
#ifdef CONFIG_AML_V2_FACTORY_BURN
	if (0x1b8ec003 == readl(P_PREG_STICKY_REG2))
		aml_try_factory_usb_burning(1, gd->bd);
	aml_try_factory_sdcard_burning(0, gd->bd);
#endif// #ifdef CONFIG_AML_V2_FACTORY_BURN

	return 0;
}
#endif

#ifdef CONFIG_AML_TINY_USBTOOL
int usb_get_update_result(void)
{
	unsigned long upgrade_step;
	upgrade_step = simple_strtoul (getenv ("upgrade_step"), NULL, 16);
	printf("upgrade_step = %d\n", (int)upgrade_step);
	if (upgrade_step == 1)
	{
		run_command("defenv", 1);
		run_command("setenv upgrade_step 2", 1);
		run_command("saveenv", 1);
		return 0;
	}
	else
	{
		return -1;
	}
}
#endif

phys_size_t get_effective_memsize(void)
{
	// >>16 -> MB, <<20 -> real size, so >>16<<20 = <<4
#if defined(CONFIG_SYS_MEM_TOP_HIDE)
	return (((readl(AO_SEC_GP_CFG0)) & 0xFFFF0000) << 4) - CONFIG_SYS_MEM_TOP_HIDE;
#else
	return (((readl(AO_SEC_GP_CFG0)) & 0xFFFF0000) << 4);
#endif
}

#ifdef CONFIG_MULTI_DTB
int checkhw(char * name)
{
	/*
	 * read board hw id
	 * set and select the dts according the board hw id.
	 *
	 * hwid = 1	p321 v1
	 * hwid = 2	p321 v2
	 */
	unsigned int hwid = 1;
	char loc_name[64] = {0};

	/* read hwid */
	hwid = (readl(P_AO_SEC_GP_CFG0) >> 8) & 0xFF;

	printf("checkhw:  hwid = %d\n", hwid);


	switch (hwid) {
		case 1:
			strcpy(loc_name, "txl_p321_v1\0");
			break;
		case 2:
			strcpy(loc_name, "txl_p321_v2\0");
			break;
		default:
			strcpy(loc_name, "txl_p321_v1");
			break;
	}
	strcpy(name, loc_name);
	setenv("aml_dt", loc_name);
	return 0;
}
#endif

static struct mm_region bd_mem_map[] = {
	{
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0x80000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0x80000000UL,
		.phys = 0x80000000UL,
		.size = 0x80000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = bd_mem_map;

void board_nand_init(void) {
	printf("board_nand_init\n");
	return;
}

int print_cpuinfo(void) {
	printf("print_cpuinfo\n");
	return 0;
}

int mach_cpu_init(void) {
	printf("mach_cpu_init\n");
	return 0;
}

int ft_board_setup(void *blob, bd_t *bd)
{
	/* eg: bl31/32 rsv */
	return 0;
}

/* workaround for VDDEE issue */
/* VCCK PWM table */
#define VCCK_VAL_REG_800      0x00150007
#define VCCK_VAL_REG_810      0x00140008
#define VCCK_VAL_REG_820      0x00130009
#define VCCK_VAL_REG_830      0x0012000a
#define VCCK_VAL_REG_840      0x0011000b
#define VCCK_VAL_REG_850      0x0010000c
#define VCCK_VAL_REG_860      0x000f000d
#define VCCK_VAL_REG_870      0x000e000e
#define VCCK_VAL_REG_880      0x000d000f
#define VCCK_VAL_REG_890      0x000c0010
#define VCCK_VAL_REG_900      0x000b0011
#define VCCK_VAL_REG_910      0x000a0012
#define VCCK_VAL_REG_920      0x00090013
#define VCCK_VAL_REG_930      0x00080014
#define VCCK_VAL_REG_940      0x00070015
#define VCCK_VAL_REG_950      0x00060016
#define VCCK_VAL_REG_960      0x00050017
#define VCCK_VAL_REG_970      0x00040018
#define VCCK_VAL_REG_980      0x00030019
#define VCCK_VAL_REG_990      0x0002001a
#define VCCK_VAL_REG_1000     0x0001001b
#define VCCK_VAL_REG_1010     0x0000001c
#define VCCK_VAL_REG_DEFAULT1 0x00500008
#define VCCK_VAL_REG_DEFAULT2 0x00860086


/* VDDEE PWM table */
#define VDDEE_VAL_REG_800      0x0010000c
#define VDDEE_VAL_REG_810      0x000f000d
#define VDDEE_VAL_REG_820      0x000e000e
#define VDDEE_VAL_REG_830      0x000d000f
#define VDDEE_VAL_REG_840      0x000c0010
#define VDDEE_VAL_REG_850      0x000b0011
#define VDDEE_VAL_REG_860      0x000a0012
#define VDDEE_VAL_REG_870      0x00090013
#define VDDEE_VAL_REG_880      0x00080014
#define VDDEE_VAL_REG_890      0x00070015
#define VDDEE_VAL_REG_900      0x00060016
#define VDDEE_VAL_REG_910      0x00050017
#define VDDEE_VAL_REG_920      0x00040018
#define VDDEE_VAL_REG_930      0x00030019
#define VDDEE_VAL_REG_940      0x0002001a
#define VDDEE_VAL_REG_950      0x0001001b
#define VDDEE_VAL_REG_960      0x0000001c
#define VDDEE_VAL_REG_DEFAULT  0x00500008

void reset_misc(void)
{
	unsigned int value;

	/* adjust VDDCPU_B to Hiz value step by step */
	writel(VCCK_VAL_REG_830, AO_PWM_PWM_D);
	udelay(1);
	writel(VCCK_VAL_REG_860, AO_PWM_PWM_D);
	udelay(1);

	/* GPIOE_0 & GPIOE_1 & GPIOE_2 to gpio pin */
	value = readl(AO_RTI_PINMUX_REG1);
	value &= ~(0xfff << 16);
	writel(value, AO_RTI_PINMUX_REG1);

	/* disable pwm_ao_b - VDDEE */
	value = readl(AO_PWM_MISC_REG_AB);
	value &= ~((0x1 << 1) | (0x1 << 23));
	writel(value, AO_PWM_MISC_REG_AB);
	writel(VDDEE_VAL_REG_DEFAULT, AO_PWM_PWM_B);

	/* disable pwm_ao_d - VDDCPU_B*/
	value = readl(AO_PWM_MISC_REG_CD);
	value &= ~((0x1 << 1) | (0x1 << 23));
	writel(value, AO_PWM_MISC_REG_CD);
	writel(VCCK_VAL_REG_DEFAULT1, AO_PWM_PWM_D);

	/* disable pwm_a - VDDCPU_A */
	value = readl(P_PWM_MISC_REG_AB);
	value &= ~((0x1 << 0) | (0x1 << 15));
	writel(value, P_PWM_MISC_REG_AB);
	writel(VCCK_VAL_REG_DEFAULT2, P_PWM_PWM_A);
}