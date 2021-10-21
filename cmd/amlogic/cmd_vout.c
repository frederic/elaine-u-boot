/*
 * (C) Copyright 2012
 * Amlogic. Inc. jets.yan@amlogic.com
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <malloc.h>
#include <asm/byteorder.h>
#ifdef CONFIG_AML_VPP
#include <amlogic/media/vpp/vpp.h>
#endif
#include <amlogic/media/vout/aml_vout.h>
#ifdef CONFIG_AML_HDMITX
#include <amlogic/media/vout/hdmitx.h>
#endif

#ifdef CONFIG_AML_CVBS
#include <amlogic/media/vout/aml_cvbs.h>
#endif
#ifdef CONFIG_AML_LCD
#include <amlogic/media/vout/lcd/aml_lcd.h>
#endif

static int do_vout_list(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
#ifdef CONFIG_AML_LCD
	struct aml_lcd_drv_s *lcd_drv = aml_lcd_get_driver();
#endif

#ifdef CONFIG_AML_HDMITX
	printf("\nvalid hdmi mode:\n");
	hdmitx_device.HWOp.list_support_modes();
#endif

#ifdef CONFIG_AML_CVBS
	printf("\nvalid cvbs mode:\n");
	cvbs_show_valid_vmode();
#endif

#ifdef CONFIG_AML_LCD
	printf("\nvalid lcd mode:\n");
	if (lcd_drv) {
		if (lcd_drv->list_support_mode)
			lcd_drv->list_support_mode();
		else
			printf("no lcd list_support_mode\n");
	} else {
		printf("no lcd driver\n");
	}
#endif

	return CMD_RET_SUCCESS;
}

static int do_vout_output(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
#ifdef CONFIG_AML_HDMITX
	char mode[64];
#endif
#ifdef CONFIG_AML_LCD
	struct aml_lcd_drv_s *lcd_drv = NULL;
#endif

	if (argc != 2)
		return CMD_RET_FAILURE;

#ifdef CONFIG_AML_CVBS
	if (cvbs_outputmode_check(argv[1]) == 0) {
#ifdef CONFIG_AML_VPP
		vpp_matrix_update(VPP_CM_YUV);
#endif
		if (cvbs_set_vmode(argv[1]) == 0)
			return CMD_RET_SUCCESS;
	}
#endif

#ifdef CONFIG_AML_HDMITX
	if (hdmi_outputmode_check(argv[1]) == 0) {
#ifdef CONFIG_AML_VPP
		vpp_matrix_update(VPP_CM_YUV);
#endif
		memset(mode, 0, sizeof(mode));
		sprintf(mode, "hdmitx output %s", argv[1]);
		run_command(mode, 0);
		return CMD_RET_SUCCESS;
	}
#endif

#ifdef CONFIG_AML_LCD
	lcd_drv = aml_lcd_get_driver();
	if (lcd_drv) {
		if (lcd_drv->lcd_outputmode_check) {
			if (lcd_drv->lcd_outputmode_check(argv[1]) == 0) {
#ifdef CONFIG_AML_VPP
				vpp_matrix_update(VPP_CM_RGB);
#endif
				if (lcd_drv->lcd_enable) {
					lcd_drv->lcd_enable(argv[1]);
					return CMD_RET_SUCCESS;
				} else
					printf("no lcd enable\n");
			}
		}
	} else {
		printf("no lcd driver\n");
	}
#endif

	return CMD_RET_FAILURE;
}

static int do_vout_info(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	vout_vinfo_dump();

	return CMD_RET_SUCCESS;
}

static cmd_tbl_t cmd_vout_sub[] = {
	U_BOOT_CMD_MKENT(list, 1, 1, do_vout_list, "", ""),
	U_BOOT_CMD_MKENT(output, 3, 1, do_vout_output, "", ""),
	U_BOOT_CMD_MKENT(info, 1, 1, do_vout_info, "", ""),
};

static int do_vout(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		return cmd_usage(cmdtp);

	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_vout_sub[0], ARRAY_SIZE(cmd_vout_sub));

	if (c)
		return  c->cmd(cmdtp, flag, argc, argv);
	else
		return cmd_usage(cmdtp);
}

U_BOOT_CMD(vout, CONFIG_SYS_MAXARGS, 1, do_vout,
	"VOUT sub-system",
	"vout [list | output format | info]\n"
	"    list : list for valid video mode names.\n"
	"    format : perfered output video mode\n"
	"    info : dump vinfo\n"
);
