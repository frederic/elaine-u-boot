/*
 * drivers/amlogic/media/vout/lcd/bl_ldim/ob3350_bl.c
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
#include <amlogic/media/vout/lcd/bl_ldim.h>
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include "ldim_drv.h"
#include "ldim_dev_drv.h"

static int ob3350_on_flag;

static int ob3350_hw_init_on(void)
{
	struct aml_ldim_driver_s *ldim_drv = aml_ldim_get_driver();

	ldim_gpio_set(ldim_drv->ldev_conf->en_gpio, ldim_drv->ldev_conf->en_gpio_on);
	mdelay(2);
	ldim_drv->pinmux_ctrl(1);
	mdelay(20);

	return 0;
}

static int ob3350_hw_init_off(void)
{
	struct aml_ldim_driver_s *ldim_drv = aml_ldim_get_driver();

	ldim_drv->pinmux_ctrl(0);
	ldim_gpio_set(ldim_drv->ldev_conf->en_gpio, ldim_drv->ldev_conf->en_gpio_off);

	return 0;
}

static unsigned int ob3350_get_value(unsigned int level)
{
	struct aml_ldim_driver_s *ldim_drv = aml_ldim_get_driver();

	unsigned int val;
	unsigned int dim_max, dim_min;

	dim_max = ldim_drv->ldev_conf->dim_max;
	dim_min = ldim_drv->ldev_conf->dim_min;

	val = dim_min + ((level * (dim_max - dim_min)) / LD_DATA_MAX);

	return val;
}

static int ob3350_smr(unsigned short *buf, unsigned char len)
{
	struct aml_ldim_driver_s *ldim_drv = aml_ldim_get_driver();
	unsigned short val;

	val = ob3350_get_value(buf[0]);
	ldim_drv->ldev_conf->pwm_config.pwm_duty = val;

	if (ob3350_on_flag == 0) {
		if (lcd_debug_print_flag)
			LDIMPR("%s: on_flag=%d\n", __func__, ob3350_on_flag);
		return 0;
	}

	if (len != 1) {
		LDIMERR("%s: data len %d invalid\n", __func__, len);
		return -1;
	}

	ldim_set_duty_pwm(&(ldim_drv->ldev_conf->pwm_config));

	return 0;
}

static int ob3350_power_on(void)
{
	struct aml_ldim_driver_s *ldim_drv = aml_ldim_get_driver();

	ob3350_hw_init_on();
	ob3350_on_flag = 1;
	/* init brightness level */
	ldim_set_duty_pwm(&(ldim_drv->ldev_conf->pwm_config));

	LDIMPR("%s: ok\n", __func__);
	return 0;
}

static int ob3350_power_off(void)
{
	ob3350_on_flag = 0;
	ob3350_hw_init_off();

	LDIMPR("%s: ok\n", __func__);
	return 0;
}

static int ob3350_ldim_driver_update(struct aml_ldim_driver_s *ldim_drv)
{
	ldim_drv->device_power_on = ob3350_power_on;
	ldim_drv->device_power_off = ob3350_power_off;
	ldim_drv->device_bri_update = ob3350_smr;

	return 0;
}

int ldim_dev_ob3350_probe(struct aml_ldim_driver_s *ldim_drv)
{
	ob3350_on_flag = 0;
	ob3350_ldim_driver_update(ldim_drv);

	return 0;
}

int ldim_dev_ob3350_remove(struct aml_ldim_driver_s *ldim_drv)
{
	return 0;
}

