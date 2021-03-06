/*
 * drivers/amlogic/media/vout/lcd/lcd_extern/mipi_lt070me05.c
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

#include <common.h>
#include <malloc.h>
#include <asm/arch/gpio.h>
#include <fdtdec.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include <amlogic/media/vout/lcd/bl_extern.h>
#include "bl_extern.h"
#include "../lcd_common.h"


#define BL_EXTERN_NAME			"mipi_lt070me05"
#define BL_EXTERN_TYPE			BL_EXTERN_MIPI

static int mipi_lt070me05_power_on(void)
{
	return 0;
}

static int mipi_lt070me05_power_off(void)
{
	return 0;
}

/******************** mipi command ********************
 *format:  data_type, num, data....
 *special: data_type=0xff, num<0xff means delay ms, num=0xff means ending.
 */
static int mipi_lt070me05_set_level(unsigned int level)
{
	unsigned char payload[] = {0x15, 2, 0x51, 0xe6, 0xff, 0xff};

	level &= 0xff;

	payload[3] = level;
#ifdef CONFIG_AML_LCD_TABLET
	dsi_write_cmd(&payload[0]);
#endif

	return 0;
}

static int mipi_lt070me05_update(void)
{
	struct aml_bl_extern_driver_s *bl_extern = aml_bl_extern_get_driver();

	if (bl_extern == NULL) {
		BLEXTERR("%s driver is null\n", BL_EXTERN_NAME);
		return -1;
	}

	bl_extern->device_power_on = mipi_lt070me05_power_on;
	bl_extern->device_power_off = mipi_lt070me05_power_off;
	bl_extern->device_bri_update = mipi_lt070me05_set_level;

	return 0;
}

int mipi_lt070me05_probe(void)
{
	int ret = 0;

	ret = mipi_lt070me05_update();

	BLEXT("%s: %d\n", __func__, ret);

	return ret;
}


