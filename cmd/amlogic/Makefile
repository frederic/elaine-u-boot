# SPDX-License-Identifier: GPL-2.0+
#

obj-$(CONFIG_CMD_VPU) += cmd_vpu.o
obj-$(CONFIG_CMD_VPP) += cmd_vpp.o
obj-$(CONFIG_CMD_VOUT) += cmd_vout.o
obj-$(CONFIG_CMD_CVBS) += cmd_cvbs.o
obj-$(CONFIG_CMD_HDMITX) += cmd_hdmitx.o
obj-$(CONFIG_CMD_LCD) += cmd_lcd.o
obj-$(CONFIG_AML_CMD_OSD) += cmd_osd.o

obj-$(CONFIG_AML_IMG_READ) += imgread.o
obj-$(CONFIG_AML_DEFENV) += defenv_without.o

# amlogic commands
obj-$(CONFIG_CMD_REBOOT) += cmd_reboot.o flash_ts.o nand_ts.o mmc_ts.o
obj-$(CONFIG_CMD_FACTORY_BOOT) += cmd_factory_boot.o flash_ts.o nand_ts.o mmc_ts.o

obj-$(CONFIG_CMD_RSVMEM) += cmd_rsvmem.o

ifdef CONFIG_BOOTLOADER_CONTROL_BLOCK
obj-y += cmd_bcb.o
ifdef CONFIG_CMD_BOOTCTOL_AVB
obj-y += cmd_bootctl_avb.o
else
obj-y += cmd_bootctl.o
endif
endif
ifndef CONFIG_AML_RPMB_DISABLE
obj-y += cmd_rpmb.o
endif
obj-y += cmd_avb.o
obj-$(CONFIG_CMD_TSENSOR) += cmd_tsensor.o
obj-$(CONFIG_CMD_OSCRING) += cmd_ringmsr.o
obj-$(CONFIG_CMD_DDR_D2PLL) += cmd_d2pll.o
obj-$(CONFIG_CMD_DDR_TEST) += cmd_ddr_test.o
obj-$(CONFIG_CMD_STORAGE) += storage.o
