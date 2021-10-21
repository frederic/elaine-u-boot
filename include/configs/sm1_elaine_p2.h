
/*
 * board/amlogic/configs/sm1_elaine_p2.h
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
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

#ifndef __SM1_ELAINE_P2_H__
#define __SM1_ELAINE_P2_H__

#include <asm/arch/cpu.h>

//#define CONFIG_SYS_GENERIC_BOARD  1
/*
#ifndef CONFIG_AML_MESON
#warning "include warning"
#endif
*/
//#define CONFIG_ZIRCON_BOOT_IMAGE

/*
 * platform power init config
 */

#define AML_VCCK_INIT_VOLTAGE    810     // VCCK power up voltage
#define AML_VDDEE_INIT_VOLTAGE   840     // VDDEE power up voltage
#define AML_VDDEE_SLEEP_VOLTAGE  731     // VDDEE suspend voltage

/* SMP Definitinos */
#define CPU_RELEASE_ADDR		secondary_boot_func

/* Bootloader Control Block function
   That is used for recovery and the bootloader to talk to each other
  */

/* Serial config */
#define CONFIG_CONS_INDEX 2
#define CONFIG_BAUDRATE  115200
/* #define CONFIG_AML_MESON_SERIAL   1 */

/* Enable ir remote wake up for bl30 */
#define AML_IR_REMOTE_POWER_UP_KEY_VAL1 0xef10fe01 //amlogic tv ir --- power
#define AML_IR_REMOTE_POWER_UP_KEY_VAL2 0XBB44FB04 //amlogic tv ir --- ch+
#define AML_IR_REMOTE_POWER_UP_KEY_VAL3 0xF20DFE01 //amlogic tv ir --- ch-
#define AML_IR_REMOTE_POWER_UP_KEY_VAL4 0xFFFFFFFF
#define AML_IR_REMOTE_POWER_UP_KEY_VAL5 0xe51afb04
#define AML_IR_REMOTE_POWER_UP_KEY_VAL6 0xFFFFFFFF
#define AML_IR_REMOTE_POWER_UP_KEY_VAL7 0xFFFFFFFF
#define AML_IR_REMOTE_POWER_UP_KEY_VAL8 0xFFFFFFFF
#define AML_IR_REMOTE_POWER_UP_KEY_VAL9 0xFFFFFFFF

/*config the default parameters for adc power key*/
#define AML_ADC_POWER_KEY_CHAN   2  /*channel range: 0-7*/
#define AML_ADC_POWER_KEY_VAL    0  /*sample value range: 0-1023*/

/* args/envs */
#define CONFIG_SYS_MAXARGS  64
#define CONFIG_EXTRA_ENV_SETTINGS \
	"loadaddr=1080000\0" \
	"outputmode=panel\0" \
	"hdmimode=1080p60hz\0" \
	"display_width=1920\0" \
	"display_height=1080\0" \
	"display_bpp=16\0" \
	"display_color_index=16\0" \
	"display_layer=osd0\0" \
	"display_color_fg=0xffff\0" \
	"display_color_bg=0\0" \
	"dtb_mem_addr=0x1000000\0" \
	"fb_addr=0x5f800000\0" \
	"fb_width=608\0" \
	"fb_height=1024\0" \
	"usb_burning=update 1000\0" \
	"fdt_high=0x20000000\0" \
	"try_auto_burn=update 700 750;\0" \
	"sdcburncfg=aml_sdc_burn.ini\0" \
	"sdc_burning=sdc_burn ${sdcburncfg}\0" \
	"wipe_data=successful\0" \
	"wipe_cache=successful\0" \
	"EnableSelinux=enforcing\0" \
	"recovery_part=recovery\0" \
	"recovery_offset=0\0" \
	"active_slot=_a\0" \
	"boot_part=boot\0" \
	"ext4_factory=1:4\0" \
	"ext4_cache=1:12\0" \
	"storeargs=" \
		"get_rebootmode;" \
		"get_chiptype;" \
		"setenv bootargs " \
			"gpt " \
			"logo=${display_layer},loaded,${fb_addr} " \
			"vout=${outputmode},enable " \
			"panel_type=${panel_type} " \
			"androidboot.selinux=${EnableSelinux} " \
			"test_mode=${test_mode} " \
			"androidboot.hardware="__stringify(BOARD_NAME)" " \
			"hw_id=${hw_id} " \
			"chip_type=${chip_type} " \
			"androidboot.reboot_mode=${reboot_mode} " \
			"androidboot.slot_suffix=${active_slot};" \
		"\0" \
	"switch_bootmode=" \
		"get_rebootmode;" \
		"echo reboot_mode:${reboot_mode};"\
		"if test ${reboot_mode} = factory_reset; then " \
			"run recovery_from_flash;" \
		"else if test ${reboot_mode} = update; then " \
			"run update;" \
		"else if test ${reboot_mode} = cold_boot; then " \
			"echo cold_boot; " \
		"else if test ${reboot_mode} = fastboot; then " \
			"fastboot;" \
		"else if test ${reboot_mode} = factory_boot; then " \
			"if imgread kernel system_b ${loadaddr}; then " \
				"bootm ${loadaddr};" \
			"fi;" \
		"fi;fi;fi;fi;fi;" \
			"\0" \
	"detect_ab_slot=" \
		"get_valid_slot;" \
		"\0" \
	"storeboot="\
		"if imgread kernel ${boot_part} ${loadaddr}; then " \
			"bootm ${loadaddr};" \
		"fi;" \
		"run update;" \
		"\0" \
	"factory_reset_poweroff_protect=" \
		"echo wipe_data=${wipe_data};" \
		"echo wipe_cache=${wipe_cache};" \
		"if test ${wipe_data} = failed; then " \
			"run init_display;" \
			"run get_hw_id;" \
			"run storeargs;" \
			"if usb start 0; then " \
				"run recovery_from_udisk;" \
			"fi;" \
			"run recovery_from_flash;" \
		"fi;" \
		"if test ${wipe_cache} = failed; then " \
			"run init_display;" \
			"run get_hw_id;" \
			"run storeargs;" \
			"if usb start 0; then " \
				"run recovery_from_udisk;" \
			"fi;" \
			"run recovery_from_flash;" \
		"fi;" \
		"\0" \
	"update=" \
		"run usb_burning;" \
		"run recovery_from_flash;" \
		"\0" \
	"recovery_from_udisk=" \
		"while true ;do " \
			"usb reset; " \
			"if fatload usb 0 ${loadaddr} recovery.img; then "\
				"bootm ${loadaddr};" \
			"fi;" \
		"done;" \
		"\0" \
	"recovery_from_flash=" \
		"setenv bootargs ${bootargs} " \
			"aml_dt=${aml_dt} " \
			"recovery_part=${recovery_part} " \
			"recovery_offset=${recovery_offset};" \
		"if imgread kernel ${recovery_part} ${loadaddr} ${recovery_offset}; then " \
			"wipeisb;" \
			"bootm ${loadaddr};" \
		"fi" \
		"\0" \
	"reset_ddic=" \
		"gpio clear GPIOZ_13" \
		"\0" \
	"detect_panel=" \
		"if gpio input GPIOZ_11; then " \
			"if gpio input GPIOZ_12; then " \
				"setenv panel_type kd_fiti9364_7;" \
			"else " \
				"setenv panel_type boe_fiti9364_7;" \
			"fi;" \
		"else " \
			"setenv panel_type inx_fiti9364_7;" \
		"fi;" \
		"\0" \
	"set_gamma="\
		"if ext4load mmc ${ext4_factory} 0x8000000 gamma_calibration.txt; then " \
			"configure_gamma 0x8000000;" \
		"else " \
			"configure_gamma;" \
		"fi;" \
		"\0" \
	"get_backlight=" \
		"if ext4load mmc ${ext4_cache} 0x8000000 last_brightness; then " \
			"get_cached_brightness 0x8000000;" \
		"else " \
			"get_cached_brightness;" \
		"fi;" \
		"\0" \
	"start_backlight=" \
		"if ext4load mmc ${ext4_factory} 0x8000000 bl_calibration.txt; then " \
			"configure_backlight 0x8000000;" \
		"else " \
			"configure_backlight;" \
		"fi;" \
		"\0" \
	"init_display="\
		"run reset_ddic;" \
		"check_fdr_for_backlight_brightness;" \
		"osd open;" \
		"osd clear;" \
		"osd setcolor 0x5b0d5b0d;" \
		"get_rebootmode;" \
		/* logo must not fail */ \
		"ext4load mmc ${ext4_factory} 0x8000000 logo.bmp;" \
		"bmp display 0x8000000;" \
		/* do this sequentially so we could reuse the address and do
		 * not need to know the size of the images */ \
		"if ext4load mmc ${ext4_factory} 0x8000000 corner_BL.bmp; then " \
			"bmp display 0x8000000 0 1008;" \
		"fi;" \
		"if ext4load mmc ${ext4_factory} 0x8000000 corner_BR.bmp; then " \
			"bmp display 0x8000000 592 1008;" \
		"fi;" \
		"if ext4load mmc ${ext4_factory} 0x8000000 corner_TL.bmp; then " \
			"bmp display 0x8000000 0 0;" \
		"fi;" \
		"if ext4load mmc ${ext4_factory} 0x8000000 corner_TR.bmp; then " \
			"bmp display 0x8000000 592 0;" \
		"fi;" \
		"bmp scale;" \
		"vout output ${outputmode};" \
		"run set_gamma;" \
		"run get_backlight;" \
		"run start_backlight;" \
		"\0" \
	"get_hw_id=" \
		"get_elaine_hw_id;" \
		"\0" \
	"enable_audio_amp_boost=" \
		"if test ${hw_id} != 0x09; then " \
			"if test ${hw_id} != 0x0a; then " \
				"echo Enabling Amp Boost;" \
				"enable_amp_boost;" \
			"fi;" \
		"fi;" \
		"\0" \
	"upgrade_key=" \
		"if gpio input GPIOZ_5; then " \
			"echo detect VOL_UP pressed;" \
			"if gpio input GPIOZ_6; then " \
				"echo VOL_DN pressed;" \
				"setenv boot_external_image 1;" \
				"run recovery_from_udisk;" \
			"fi;" \
		"else " \
			"echo upgrade key not pressed;" \
			"setenv boot_external_image 0;" \
		"fi;" \
		"\0" \
	"test_mode_check=" \
		"setenv test_mode false;" \
		"if gpio input GPIOZ_2; then " \
			"echo MUTE not engaged;" \
		"else " \
			"echo MUTE engaged;" \
			"if gpio input GPIOZ_5; then " \
				"echo VOL_UP pressed;" \
				"if gpio input GPIOZ_6; then " \
					"echo VOL_DN pressed;" \
				"else " \
					"echo VOL_DN not pressed;" \
					"setenv test_mode true;" \
				"fi;" \
			"else " \
				"echo VOL_UP not pressed;" \
			"fi;" \
		"fi;" \
		"\0" \

#ifdef CONFIG_G_AB_SYSTEM
#define CONFIG_PREBOOT  \
		"run factory_reset_poweroff_protect;"\
		"run init_display;"\
		"run get_hw_id;" \
		"run enable_audio_amp_boost;" \
		"run test_mode_check;" \
		"run detect_ab_slot;" \
		"run storeargs;" \
		"run upgrade_key;" \
		"run switch_bootmode;"
#else
#define CONFIG_PREBOOT  \
		"run factory_reset_poweroff_protect;"\
		"run get_hw_id;" \
		"run test_mode_check;" \
		"run storeargs;" \
		"run upgrade_key;" \
		"run switch_bootmode;"
#endif

/* #define CONFIG_ENV_IS_NOWHERE  1 */
#define CONFIG_ENV_SIZE   (64*1024)
#define CONFIG_FIT 1
#define CONFIG_OF_LIBFDT 1
#define CONFIG_ANDROID_BOOT_IMAGE 1
#define CONFIG_SYS_BOOTM_LEN (64<<20) /* Increase max gunzip size*/

/* cpu */
/* #define CONFIG_CPU_CLK					1200 //MHz. Range: 360-2000, should be multiple of 24 */

/* ATTENTION */
/* DDR configs move to board/amlogic/[board]/firmware/timing.c */

//#define CONFIG_NR_DRAM_BANKS			1
/* ddr functions */
#define DDR_FULL_TEST            0 //0:disable, 1:enable. ddr full test
#define DDR_LOW_POWER            0 //0:disable, 1:enable. ddr clk gate for lp
#define DDR_ZQ_PD                0 //0:disable, 1:enable. ddr zq power down
#define DDR_USE_EXT_VREF         0 //0:disable, 1:enable. ddr use external vref
#define DDR4_TIMING_TEST         0 //0:disable, 1:enable. ddr4 timing test function
#define DDR_PLL_BYPASS           0 //0:disable, 1:enable. ddr pll bypass function

/* storage: emmc/nand/sd */
#define 	CONFIG_ENV_OVERWRITE
/* #define 	CONFIG_CMD_SAVEENV */
/* fixme, need fix*/

#if (defined(CONFIG_ENV_IS_IN_AMLNAND) || defined(CONFIG_ENV_IS_IN_MMC)) && defined(CONFIG_STORE_COMPATIBLE)
#error env in amlnand/mmc already be compatible;
#endif

/*
*				storage
*		|---------|---------|
*		|					|
*		emmc<--Compatible-->nand
*					|-------|-------|
*					|				|
*					MTD<-Exclusive->NFTL
*/
/* axg only support slc nand */
/* swither for mtd nand which is for slc only. */
/* support for mtd */

/* #define CONFIG_AML_MTD 1 */

/* support for nftl */
/*#define CONFIG_AML_NAND	1*/

#if defined(CONFIG_AML_NAND) && defined(CONFIG_AML_MTD)
#error CONFIG_AML_NAND/CONFIG_AML_MTD can not support at the sametime;
#endif

#ifdef CONFIG_AML_MTD

/* bootlaoder is construct by bl2 and fip
 * when DISCRETE_BOOTLOADER is enabled, bl2 & fip
 * will not be stored continuously, and nand layout
 * would be bl2|rsv|fip|normal, but not
 * bl2|fip|rsv|noraml anymore
 */

/* #define CONFIG_CMD_NAND 1 */
#define CONFIG_MTD_DEVICE y
/* mtd parts of ourown.*/
#define CONFIG_AML_MTDPART	1
/* mtd parts by env default way.*/
/*
#define MTDIDS_NAME_STR		"aml_nand.0"
#define MTDIDS_DEFAULT		"nand1=" MTDIDS_NAME_STR
#define MTDPARTS_DEFAULT	"mtdparts=" MTDIDS_NAME_STR ":" \
					"3M@8192K(logo),"	\
					"10M(recovery),"	\
					"8M(kernel),"	\
					"40M(rootfs),"	\
					"-(data)"
*/
#define CONFIG_CMD_UBI
#define CONFIG_CMD_UBIFS
#define CONFIG_MTD_UBI_WL_THRESHOLD 4096
#define CONFIG_MTD_UBI_BEB_LIMIT 20
#define CONFIG_RBTREE
#define CONFIG_CMD_NAND_TORTURE 1
#define CONFIG_CMD_MTDPARTS   1
#define CONFIG_MTD_PARTITIONS 1
#define CONFIG_SYS_MAX_NAND_DEVICE  2
#define CONFIG_SYS_NAND_BASE_LIST   {0}
#endif
/* endof CONFIG_AML_MTD */
/* #define		CONFIG_AML_SD_EMMC 1 */
#ifdef		CONFIG_AML_SD_EMMC
	#define 	CONFIG_GENERIC_MMC 1
	#define 	CONFIG_CMD_MMC 1
	#define CONFIG_CMD_GPT 1
	#define	CONFIG_SYS_MMC_ENV_DEV 1
	#define CONFIG_EMMC_DDR52_EN 0
	#define CONFIG_EMMC_DDR52_CLK 35000000
#endif
#define		CONFIG_PARTITIONS 1

#if defined CONFIG_AML_MTD || defined CONFIG_SPI_NAND
	#define CONFIG_CMD_NAND 1
	#define CONFIG_MTD_DEVICE y
	/* #define CONFIG_RBTREE */
	#define CONFIG_CMD_NAND_TORTURE 1
	#define CONFIG_CMD_MTDPARTS   1
	#define CONFIG_MTD_PARTITIONS 1
	#define CONFIG_SYS_MAX_NAND_DEVICE  2
	#define CONFIG_SYS_NAND_BASE_LIST   {0}
#endif

/* vpu */
#define AML_VPU_CLK_LEVEL_DFT 7

/* osd */
#define OSD_SCALE_ENABLE
#define AML_OSD_HIGH_VERSION

/* USB
 * Enable CONFIG_MUSB_HCD for Host functionalities MSC, keyboard
 * Enable CONFIG_MUSB_UDD for Device functionalities.
 */
/* #define CONFIG_MUSB_UDC		1 */
/* #define CONFIG_CMD_USB 1 */

#define AML_TXLX_USB        1
#define USB_PHY2_PLL_PARAMETER_1	0x09400414
#define USB_PHY2_PLL_PARAMETER_2	0x927e0000
#define USB_PHY2_PLL_PARAMETER_3	0xAC5F69E5
#define USB_G12x_PHY_PLL_SETTING_1  (0xfe18)
#define USB_G12x_PHY_PLL_SETTING_2  (0xfff)
#define USB_G12x_PHY_PLL_SETTING_3  (0x78000)
#define USB_G12x_PHY_PLL_SETTING_4  (0xe0004)
#define USB_G12x_PHY_PLL_SETTING_5  (0xe000c)

/* UBOOT Facotry usb/sdcard burning config */
#ifndef AML_DISABLE_UPDATE_MODE
#define CONFIG_AML_V2_FACTORY_BURN              1       //support facotry usb burning
#define CONFIG_USB_BURNING_TOOL                 1
#define CONFIG_AML_FACTORY_BURN_LOCAL_UPGRADE   1       //support factory sdcard burning
#define CONFIG_SD_BURNING_SUPPORT_UI            1       //Displaying upgrading progress bar when sdcard/udisk burning
#endif

/* net */
#if defined(CONFIG_CMD_NET)
	#define CONFIG_DESIGNWARE_ETH 1
	#define CONFIG_PHYLIB	1
	#define CONFIG_NET_MULTI 1
	#define CONFIG_CMD_PING 1
	#define CONFIG_CMD_DHCP 1
	#define CONFIG_CMD_RARP 1
	#define CONFIG_HOSTNAME        "arm_gxbb"
	#define CONFIG_ETHADDR         00:15:18:01:81:31   /* Ethernet address */
	#define CONFIG_IPADDR          10.18.9.97          /* Our ip address */
	#define CONFIG_GATEWAYIP       10.18.9.1           /* Our getway ip address */
	#define CONFIG_SERVERIP        10.18.9.113         /* Tftp server ip address */
	#define CONFIG_NETMASK         255.255.255.0
#endif /* (CONFIG_CMD_NET) */

/* other devices */
#define CONFIG_SHA1 1
#define CONFIG_MD5 1

/* commands */
/* #define CONFIG_CMD_FDT 1 */

/*file system*/
#define CONFIG_DOS_PARTITION 1
#define CONFIG_EFI_PARTITION 1

/* #define CONFIG_MMC 1 */
#define CONFIG_LZO 1

/* Cache Definitions */
/* #define CONFIG_SYS_DCACHE_OFF */
/* #define CONFIG_SYS_ICACHE_OFF */

/* other functions */
#define CONFIG_FIP_IMG_SUPPORT  1
/* #define CONFIG_SYS_MEM_TOP_HIDE 0x08000000 */ /* hide 128MB for kernel reserve */

#define CONFIG_CPU_ARMV8

/* #define CONFIG_MULTI_DTB    1 */
#define DTB_BIND_KERNEL      1

/* support secure boot */
#define CONFIG_AML_SECURE_UBOOT   1

#if defined(CONFIG_AML_SECURE_UBOOT)

/* for SRAM size limitation just disable NAND
   as the socket board default has no NAND */
/* #undef CONFIG_AML_NAND */

/* unify build for generate encrypted bootloader "u-boot.bin.encrypt" */
#define CONFIG_AML_CRYPTO_UBOOT   1

/* unify build for generate encrypted kernel image
   SRC : "board/amlogic/(board)/boot.img"
   DST : "fip/boot.img.encrypt" */
/* #define CONFIG_AML_CRYPTO_IMG       1 */

#endif /* CONFIG_AML_SECURE_UBOOT */

/* build with uboot auto test */
/* #define CONFIG_AML_UBOOT_AUTO_TEST 1 */

/* board customer ID */
/* #define CONFIG_CUSTOMER_ID  (0x6472616F624C4D41) */

/* Choose One of Ethernet Type */
#undef CONFIG_ETHERNET_NONE
#define ETHERNET_INTERNAL_PHY
#undef ETHERNET_EXTERNAL_PHY

#if defined(CONFIG_CMD_AML_MTEST)
#if !defined(CONFIG_SYS_MEM_TOP_HIDE)
#error CONFIG_CMD_AML_MTEST depends on CONFIG_SYS_MEM_TOP_HIDE;
#endif
#if !(CONFIG_SYS_MEM_TOP_HIDE)
#error CONFIG_SYS_MEM_TOP_HIDE should not be zero;
#endif
#endif

#endif
