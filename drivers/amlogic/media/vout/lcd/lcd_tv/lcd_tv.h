
/*
 * drivers/amlogic/media/vout/lcd/lcd_tv/lcd_tv.h
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

#ifndef __AML_LCD_TV_H__
#define __AML_LCD_TV_H__
#include <amlogic/media/vout/lcd/aml_lcd.h>

//**********************************
//lcd driver version
//**********************************
#define LCD_DRV_TYPE      "tv"

//**********************************

extern void lcd_tv_config_update(struct lcd_config_s *pconf);
extern void lcd_tv_driver_init_pre(void);
extern int lcd_tv_driver_init(void);
extern void lcd_tv_driver_disable(void);

#endif
