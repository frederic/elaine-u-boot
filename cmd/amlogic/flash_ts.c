/*
 * Flash-based transactional key-value store
 *
 * Copyright (C) 2010 Google, Inc.
 * Author: Eugene Surovegin <es@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 */
#include <amlogic/flash_ts.h>
#include <amlogic/nand_ts.h>
#include <amlogic/mmc_ts.h>

int flash_ts_set(const char *key, const char *value)
{
	int res = 0;
#ifdef CONFIG_NAND_FTS
	res = nand_ts_set(key, value);
#endif

#ifdef CONFIG_MMC_FTS
	res = mmc_ts_set(key, value);
#endif
	return res;
}

void flash_ts_get(const char *key, char *value, unsigned int size)
{
#ifdef CONFIG_NAND_FTS
	nand_ts_get(key, value, size);
#endif

#ifdef CONFIG_MMC_FTS
	mmc_ts_get(key, value, size);
#endif
}

int flash_ts_init(void)
{
	int res = 0;
#ifdef CONFIG_NAND_FTS
	res = nand_ts_init();
#endif

#ifdef CONFIG_MMC_FTS
	res = mmc_ts_init();
#endif
	return res;
}

int is_flash_inited(void)
{
#if defined(CONFIG_NAND_FTS)
	extern bool amlnf_is_inited(void);
	return amlnf_is_inited()
#endif
#if defined(CONFIG_MMC_FTS)
	extern bool amlmmc_is_inited(void);
	return amlmmc_is_inited();
#endif
}

/* Make sure MTD subsystem is already initialized */
late_initcall(flash_ts_init);
