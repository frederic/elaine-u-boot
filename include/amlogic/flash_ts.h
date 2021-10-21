/*
 * Flash-based transactional key-value store
 *
 * Copyright (C) 2010 Google, Inc.
 */
#ifndef _FLASH_TS_H
#define _FLASH_TS_H

#include <fs.h>
#include <linux/kernel.h>
#include <linux/compat.h>

#define DRV_NAME        	"fts"
#define DRV_VERSION     	"0.999"

#define CONFIG_FLASH_TS_PARTITION "fts"

/* Keep in sync with 'struct flash_ts' */
#define FLASH_TS_HDR_SIZE	(4 * sizeof(u32))
#define FLASH_TS_MAX_SIZE	(16 * 1024)
#define FLASH_TS_MAX_DATA_SIZE	(FLASH_TS_MAX_SIZE - FLASH_TS_HDR_SIZE)

#define FLASH_TS_MAGIC		0x53542a46

/* Physical flash layout */
struct flash_ts {
	u32 magic;		/* "F*TS" */
	u32 crc;		/* doesn't include magic and crc fields */
	u32 len;		/* real size of data */
	u32 version;		/* generation counter, must be positive */

	/* data format is very similar to Unix environment:
	 *   key1=value1\0key2=value2\0\0
	 */
	char data[FLASH_TS_MAX_DATA_SIZE];
};

int flash_ts_init(void);
void flash_ts_get(const char *key, char *value, unsigned int size);
int flash_ts_set(const char *key, const char *value);
int is_flash_inited(void);

#endif  /* _FLASH_TS_H */
