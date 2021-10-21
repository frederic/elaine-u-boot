/*
 * board/amlogic/sm1_elaine_b1/lcd_extern.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef _DFT_LCD_EXTERN_H_
#define _DFT_LCD_EXTERN_H_

#define LCD_EXT_I2C_GPIO_SCK    0xff /* 0xff for invalid */
#define LCD_EXT_I2C_GPIO_SDA    0xff /* 0xff for invalid */
#define LCD_EXT_SPI_GPIO_CS     0xff /* 0xff for invalid */
#define LCD_EXT_SPI_GPIO_CLK    0xff /* 0xff for invalid */
#define LCD_EXT_SPI_GPIO_DATA   0xff /* 0xff for invalid */
#define LCD_EXT_PINMUX_GPIO_OFF 0

static char lcd_ext_gpio[LCD_EXTERN_GPIO_NUM_MAX][LCD_EXTERN_GPIO_LEN_MAX] = {
	"invalid", /* ending flag */
};

static struct lcd_pinmux_ctrl_s lcd_ext_pinmux_ctrl[LCD_PINMX_MAX] = {
	{
		.name = "invalid",
	},
};

static unsigned char ext_init_on_table[LCD_EXTERN_INIT_ON_MAX] = {
	0xff, 0,   //ending flag
};

static unsigned char ext_init_off_table[LCD_EXTERN_INIT_OFF_MAX] = {
	0xff, 0,   //ending flag
};

static struct lcd_extern_config_s ext_config[LCD_EXTERN_NUM_MAX] = {
	{
		.index = 0,
		.name = "ext_default",
		.type = LCD_EXTERN_I2C, /* LCD_EXTERN_I2C, LCD_EXTERN_SPI, LCD_EXTERN_MIPI, LCD_EXTERN_MAX */
		.status = 0, /* 0=disable, 1=enable */
		.i2c_addr = 0x1c, /* 7bit i2c address */
		.i2c_addr2 = 0xff, /* 7bit i2c address, 0xff for invalid */
		.cmd_size = LCD_EXT_CMD_SIZE_DYNAMIC,
		.table_init_on = ext_init_on_table,
		.table_init_on_cnt = sizeof(ext_init_on_table),
		.table_init_off = ext_init_off_table,
		.table_init_off_cnt = sizeof(ext_init_off_table),
	},
	{ /* P070ACB_FT9364 */
		.index = 1,
		.name = "mipi_default",
		.type = LCD_EXTERN_MIPI, /* LCD_EXTERN_I2C, LCD_EXTERN_SPI, LCD_EXTERN_MIPI, LCD_EXTERN_MAX */
		.status = 1, /* 0=disable, 1=enable */
		.cmd_size = LCD_EXT_CMD_SIZE_DYNAMIC,
		/*we define initial codes in panel.dtsi*/
		.table_init_on = ext_init_on_table,
		.table_init_on_cnt = sizeof(ext_init_on_table),
		.table_init_off = ext_init_off_table,
		.table_init_off_cnt = sizeof(ext_init_off_table),
	},
	{ /* KD070D82_FT9364*/
		.index = 2,
		.name = "mipi_default",
		.type = LCD_EXTERN_MIPI, /* LCD_EXTERN_I2C, LCD_EXTERN_SPI, LCD_EXTERN_MIPI, LCD_EXTERN_MAX */
		.status = 1, /* 0=disable, 1=enable */
		.cmd_size = LCD_EXT_CMD_SIZE_DYNAMIC,
		/*we define initial codes in panel.dtsi*/
		.table_init_on = ext_init_on_table,
		.table_init_on_cnt = sizeof(ext_init_on_table),
		.table_init_off = ext_init_off_table,
		.table_init_off_cnt = sizeof(ext_init_off_table),
	},
	{ /* TV070WSM_FT9364*/
		.index = 3,
		.name = "mipi_default",
		.type = LCD_EXTERN_MIPI, /* LCD_EXTERN_I2C, LCD_EXTERN_SPI, LCD_EXTERN_MIPI, LCD_EXTERN_MAX */
		.status = 1, /* 0=disable, 1=enable */
		.cmd_size = LCD_EXT_CMD_SIZE_DYNAMIC,
		/*we define initial codes in panel.dtsi*/
		.table_init_on = ext_init_on_table,
		.table_init_on_cnt = sizeof(ext_init_on_table),
		.table_init_off = ext_init_off_table,
		.table_init_off_cnt = sizeof(ext_init_off_table),
	},
	{
		.index = LCD_EXTERN_INDEX_INVALID,
	},
};

#endif

