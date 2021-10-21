/*
 * include/amlogic/media/vout/lcd/aml_lcd.h
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

#ifndef INC_AML_LCD_H
#define INC_AML_LCD_H

#include <common.h>
#include <linux/list.h>
#include <amlogic/media/vout/lcd/lcd_vout.h>
#ifdef CONFIG_AML_LCD_EXTERN
#include <amlogic/media/vout/lcd/lcd_extern.h>
#endif
#ifdef CONFIG_AML_BL_EXTERN
#include <amlogic/media/vout/lcd/bl_extern.h>
#endif

#define LCD_GPIO_MAX                  0xff
#define LCD_GPIO_OUTPUT_LOW           0
#define LCD_GPIO_OUTPUT_HIGH          1
#define LCD_GPIO_INPUT                2


#define LCD_EXT_I2C_BUS_0             0  //A
#define LCD_EXT_I2C_BUS_1             1  //B
#define LCD_EXT_I2C_BUS_2             2  //C
#define LCD_EXT_I2C_BUS_3             3  //D
#define LCD_EXT_I2C_BUS_4             4  //AO
#define LCD_EXT_I2C_BUS_MAX           0xff


#define LCD_EXT_I2C_BUS_INVALID       0xff
#define LCD_EXT_I2C_ADDR_INVALID      0xff
#define LCD_EXT_GPIO_INVALID          0xff

#define LCD_EXT_SPI_CLK_FREQ_DFT      10 /* unit: KHz */


#define LCD_EXT_CMD_TYPE_CMD_DELAY    0x00
#define LCD_EXT_CMD_TYPE_CMD2_DELAY   0x01  /* for i2c device 2nd addr */
#define LCD_EXT_CMD_TYPE_NONE         0x10
#define LCD_EXT_CMD_TYPE_CMD          0xc0
#define LCD_EXT_CMD_TYPE_CMD2         0xc1  /* for i2c device 2nd addr */
#define LCD_EXT_CMD_TYPE_GPIO         0xf0
#define LCD_EXT_CMD_TYPE_CHECK        0xfc
#define LCD_EXT_CMD_TYPE_DELAY        0xfd
#define LCD_EXT_CMD_TYPE_END          0xff

#define LCD_EXT_CMD_SIZE_DYNAMIC      0xff
#define LCD_EXT_DYNAMIC_SIZE_INDEX    1


#define LCD_GPIO_NAME_MAX             15
struct lcd_cpu_gpio_s {
	char name[LCD_GPIO_NAME_MAX];
	struct gpio_desc gpio;
	int probe_flag;
	int register_flag;
};


#define Rsv_val 0xffffffff
struct ext_lcd_config_s {
	const char panel_type[20];
	int lcd_type;
	unsigned char lcd_bits;

	unsigned short h_active;
	unsigned short v_active;
	unsigned short h_period;
	unsigned short v_period;
	unsigned short hsync_width;
	unsigned short hsync_bp;
	unsigned short hsync_pol;
	unsigned short vsync_width;
	unsigned short vsync_bp;
	unsigned short vsync_pol;

	unsigned int customer_val_0; //fr_adjust_type
	unsigned int customer_val_1; //ss_level
	unsigned int customer_val_2; //clk_auto_generate
	unsigned int customer_val_3; //pixel clock(unit in Hz)
	unsigned int customer_val_4;
	unsigned int customer_val_5;
	unsigned int customer_val_6;
	unsigned int customer_val_7;
	unsigned int customer_val_8;
	unsigned int customer_val_9;

	unsigned int if_attr_val0;
	unsigned int if_attr_val1;
	unsigned int if_attr_val2;
	unsigned int if_attr_val3;
	unsigned int if_attr_val4;
	unsigned int if_attr_val5;
	unsigned int if_attr_val6;
	unsigned int if_attr_val7;
	unsigned int if_attr_val8;
	unsigned int if_attr_val9;

	unsigned char *cmd_init_on;
	unsigned char *cmd_init_off;
	struct lcd_power_step_s *power_on_step;
	struct lcd_power_step_s *power_off_step;

	/* backlight */
	unsigned int level_default;
	unsigned int level_max;
	unsigned int level_min;
	unsigned int level_mid;
	unsigned int level_mid_mapping;

	unsigned int bl_method;
	unsigned int bl_en_gpio;
	unsigned short bl_en_gpio_on;
	unsigned short bl_en_gpio_off;
	unsigned short bl_power_on_delay;
	unsigned short bl_power_off_delay;

	unsigned int pwm_method;
	unsigned int pwm_port;
	unsigned int pwm_freq;
	unsigned int pwm_duty_max;
	unsigned int pwm_duty_min;
	unsigned int pwm_gpio;
	unsigned int pwm_gpio_off;

	unsigned int pwm2_method;
	unsigned int pwm2_port;
	unsigned int pwm2_freq;
	unsigned int pwm2_duty_max;
	unsigned int pwm2_duty_min;
	unsigned int pwm2_gpio;
	unsigned int pwm2_gpio_off;

	unsigned int pwm_level_max;
	unsigned int pwm_level_min;
	unsigned int pwm2_level_max;
	unsigned int pwm2_level_min;

	unsigned int pwm_on_delay;
	unsigned int pwm_off_delay;

	/* backlight extern */
	unsigned int bl_ext_index;
};

#define LCD_NUM_MAX         10

extern struct ext_lcd_config_s ext_lcd_config[LCD_NUM_MAX];

#endif /* INC_AML_LCD_H */
