
/*
 * drivers/amlogic/media/vout/lcd/lcd_tablet/lcd_tablet.h
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

#ifndef __AML_LCD_TABLET_H__
#define __AML_LCD_TABLET_H__

//**********************************
//lcd driver version
//**********************************
#define LCD_DRV_TYPE      "tablet"

//**********************************

extern void lcd_tablet_config_update(struct lcd_config_s *pconf);
extern void lcd_tablet_driver_init_pre(void);
extern int lcd_tablet_driver_init(void);
extern void lcd_tablet_driver_disable(void);

#endif
