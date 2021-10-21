
/*
 * common/cmd_factory_boot.c
 *
 * Copyright (C) 2018 Amlogic, Inc. All rights reserved.
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
#include <command.h>
#include <amlogic/flash_ts.h>

#include <asm/arch-g12b/reboot.h>
#include <asm/arch/secure_apb.h>
#include <asm/io.h>
#include <asm/arch/bl31_apis.h>
#include <asm/arch/watchdog.h>

int set_factory_boot (char * const value)
{
	int ret = 1;

	if(is_flash_inited()) {
		flash_ts_init();

		char key[] = "bootloader.command";
		ret = flash_ts_set(key, value);

		printf("FTS set:\n%s -> %s\nReturn: %d\n", key, value, ret);
	}
	return ret;
}

int do_factory_boot (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char value[] = "boot-factory";
	return set_factory_boot(value);
}

int do_not_factory_boot (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char value[] = "";
	return set_factory_boot(value);
}

U_BOOT_CMD(
  enable_factory_boot, 1,  0,  do_factory_boot,
  "Set FTS flag to enable factory boot.",
  "enable_factory_boot\n"
);

U_BOOT_CMD(
  disable_factory_boot, 1,  0,  do_not_factory_boot,
  "Resets FTS flag to disable factory boot.",
  "disable_factory_boot\n"
);
