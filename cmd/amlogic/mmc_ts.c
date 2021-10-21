/*
 * MMC-based transactional key-value store
 *
 * Copyright (C) 2010 Google, Inc.
 * Author: Bill he <yuegui.he@amlogic.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 */
#include <amlogic/flash_ts.h>
#include <amlogic/mmc_ts.h>
#include <common.h>
#include <malloc.h>
#include <mmc.h>
#include <partition_table.h>
#include <emmc_partitions.h>
#include <asm/arch/cpu_sdio.h>
#include <asm/arch/sd_emmc.h>
#include <linux/sizes.h>
#include <amlogic/aml_mmc.h>

#define DRV_DESC        	"MMC-based key-value storage"

/* Internal state */
struct mmc_ts_priv {
	struct mutex lock;
	struct mmc *mmc;
	struct partitions *part_info;

	/* chunk size, >= sizeof(struct mmc_ts) */
	size_t chunk;

	int mmc_read_write_unit;

	/* current record offset within mmc partition device */
	loff_t offset;

	/* fts partition offset on*/
	u64 mmc_ts_offset;

	/* fts partition size */
	u64 mmc_ts_size;

	/* mmc dev*/
	int dev;

	/* in-memory copy of mmc content */
	struct flash_ts cache;

	/* temporary buffers
	 *  - one backup for failure rollback
	 *  - another for read-after-write verification
	 */
	struct flash_ts cache_tmp_backup;
	struct flash_ts cache_tmp_verify;
};

static struct mmc_ts_priv *__ts;
static inline void __mmc_ts_put(struct mmc_ts_priv *ts)
{
	mutex_unlock(&ts->lock);
}

static void set_to_default_empty_state(struct mmc_ts_priv *ts)
{
	ts->offset = ts->part_info->size - ts->chunk;
	ts->cache.version = 0;
	ts->cache.len = 1;
	ts->cache.magic = FLASH_TS_MAGIC;
	ts->cache.data[0] = '\0';
}

static struct mmc_ts_priv *__mmc_ts_get(void)
{
	struct mmc_ts_priv *ts = __ts;

	if (likely(ts)) {
		mutex_lock(&ts->lock);
	} else {
		printk(KERN_ERR DRV_NAME ": not initialized yet\n");
	}

	return ts;
}

static char *mmc_ts_find(struct mmc_ts_priv *ts, const char *key, size_t key_len)
{
	char *s = ts->cache.data;
	while (*s) {
		if (!strncmp(s, key, key_len)) {
			if (s[key_len] == '=')
				return s;
		}

		s += strlen(s) + 1;
	}
	return NULL;
}

static inline u32 mmc_ts_check_header(const struct flash_ts *cache)
{
	if (cache->magic == FLASH_TS_MAGIC &&
	    cache->version &&
	    cache->len && cache->len <= sizeof(cache->data) &&
	    /* check correct null-termination */
	    !cache->data[cache->len - 1] &&
	    (cache->len == 1 || !cache->data[cache->len - 2])) {
		/* all is good */
		return cache->version;
	}

	return 0;
}

static int mmc_ts_read(int dev, struct mmc_ts_priv *ts, loff_t off, void *buf, size_t size, u64 part_start_offset)
{
	ulong start_blk;
	struct mmc *mmc = ts->mmc;
	void *addr_byte, *addr = buf, *addr_tmp;
	u64 cnt = 0, sz_byte = 0, n = 0, blk = 0;
	mmc = find_mmc_device(dev);
	if (!mmc) {
		return 1;
	}
	mmc_init(mmc);

	/* blk shift : normal is 9 */
	int blk_shift = 0;
	blk_shift =  ffs(mmc->read_bl_len) - 1;

	/* start blk offset */
	blk = (part_start_offset + off) >> blk_shift;

	/* seziof(ts->cache_tmp_verify) = cnt * ts->chunk + sz_byte */
	cnt = size >> blk_shift;
	sz_byte = size - (cnt << blk_shift);

	/* read cnt* ts->chunk bytes */
	n = blk_dread(mmc_get_blk_desc(mmc), blk, cnt, addr);

	/* read sz_byte bytes */
	if ((n == cnt) && (sz_byte != 0)) {
		//printf("sz_byte=%#llx bytes\n",sz_byte);
		addr_tmp = malloc(mmc->read_bl_len);
		addr_byte = (void *)(addr+cnt*(mmc->read_bl_len));
		start_blk = blk+cnt;

		if (addr_tmp == NULL) {
			printf("mmc read: malloc fail\n");
			free(addr);
			return 1;
		}

		if (blk_dread(mmc_get_blk_desc(mmc), start_blk, 1, addr_tmp) != 1) { // read 1 block
			free(addr_tmp);
			printf("mmc read 1 block fail\n");
			return 1;
		}

		memcpy(addr_byte, addr_tmp, sz_byte);
		free(addr_tmp);
	}

	return 0;
}

static int mmc_is_blank(const void *buf, size_t size)
{
	size_t i;
	const unsigned int *data = (const unsigned int *)buf;
	size /= sizeof(data[0]);

	for (i = 0; i < size; i++)
		if (data[i] != 0xffffffff)
			return 0;
	return 1;
}

static int __init mmc_ts_scan(struct mmc_ts_priv *ts, int dev)
{
	struct mmc *mmc = ts->mmc;
	int res;
	loff_t off = 0;
	struct flash_ts tmp_scan_backup;
	u64 mmc_ts_size = ts->mmc_ts_size;
	u64 part_start_offset = ts->mmc_ts_offset;
	void *scan_addr = (void *) &tmp_scan_backup;

	memset(&tmp_scan_backup, 0, sizeof(tmp_scan_backup));

	mmc_init(mmc);

	do {
		/* read FLASH_TS_MAX_DATA_SIZE  data to ts->cache_tmp_verify */
		res = mmc_ts_read(dev, ts, off, scan_addr, sizeof(tmp_scan_backup), part_start_offset);
		if (!res) {
			/* check data struct */
			u32 version = mmc_ts_check_header(scan_addr);
			if (version > 0) {
				if (version > ts->cache.version) {
					memcpy(&ts->cache, scan_addr, sizeof(tmp_scan_backup));
					ts->offset = off;
				}
				break;
			} else if (0 == version && mmc_is_blank(&tmp_scan_backup, sizeof(tmp_scan_backup))) {
				off = (off + ts->mmc->erase_grp_size) & ~(ts->mmc->erase_grp_size - 1);
			} else {
				off += ts->chunk;
			}

		} else {
			off += ts->chunk;
		}

	} while(off < mmc_ts_size) ;/* while done*/

	return res;
}


static int mmc_write(int dev, struct mmc_ts_priv *ts, loff_t off, void *buf, size_t size, u64 part_start_offset)
{
	ulong start_blk;
	struct mmc *mmc = ts->mmc;
	void *addr_byte, *addr_tmp, *addr, *addr_read;
	u64 cnt = 0, sz_byte = 0, n = 0, blk = 0, res;
	struct flash_ts tmp_write_backup, tmp_read_backup;

	/*********init local data struct**************/
	memset(&tmp_write_backup, 0, sizeof(tmp_write_backup));
	memset(&tmp_read_backup, 0, sizeof(tmp_read_backup));
	memcpy(&tmp_write_backup, buf, size);
	addr = (void *) &tmp_write_backup;
	addr_read = (void *) &tmp_read_backup;
	/********************************************/

	mmc = find_mmc_device(dev);
	if (!mmc) {
		return 1;
	}
	mmc_init(mmc);

	/* blk shift : normal is 9 */
	int blk_shift = 0;
	blk_shift =  ffs(mmc->read_bl_len) - 1;

	/* start blk offset */
	blk = (part_start_offset + off) >> blk_shift;

	/* seziof(ts->cache_tmp_verify) = cnt * ts->chunk + sz_byte */
	cnt = size >> blk_shift;
	sz_byte = size - (cnt << blk_shift);

	n = blk_dwrite(mmc_get_blk_desc(mmc), blk, cnt, addr);
	//write sz_byte bytes
	if ((n == cnt) && (sz_byte != 0)) {
		// printf("sz_byte=%#llx bytes\n",sz_byte);
		addr_tmp = malloc(mmc->write_bl_len);
		addr_byte = (void*)(addr+cnt*(mmc->write_bl_len));
		start_blk = blk+cnt;

		if (addr_tmp == NULL) {
			printf("mmc write: malloc fail\n");
			return 1;
		}

		if (blk_dread(mmc_get_blk_desc(mmc), start_blk, 1, addr_tmp) != 1) { // read 1 block
			free(addr_tmp);
			printf("mmc read 1 block fail\n");
			return 1;
		}

		memcpy(addr_tmp, addr_byte, sz_byte);
		if (blk_dwrite(mmc_get_blk_desc(mmc), blk, cnt, addr) != 1) { // write 1 block
			free(addr_tmp);
			printf("mmc write 1 block fail\n");
			return 1;
		}
		free(addr_tmp);
	}

	res = mmc_ts_read(dev, ts, off, addr_read, sizeof(tmp_read_backup), part_start_offset);

	if (!res) {
		if (! memcmp(&tmp_write_backup, &tmp_read_backup, size)) {
			memcpy(&ts->cache, &tmp_write_backup, size);
			printk("key write successfull!\n");
			return 0;
		} else {
			printk("%s:%d error!\n", __func__, __LINE__);
			return -1;
		}

	} else {
		printk("check key failure!\n");
		printk("%s:%d error!\n", __func__, __LINE__);
		return -2;
	}

	return 0;
}

static int mmc_ts_commit(struct mmc_ts_priv *ts)
{
	int res;
	int dev = ts->dev;
	u64 part_start_offset = ts->mmc_ts_offset;

	res = mmc_write(dev,ts, 0, &ts->cache, sizeof(ts->cache), part_start_offset);

	return res;
}

int mmc_ts_set(const char *key, const char *value)
{
	int res;
	char *p;
	struct mmc_ts_priv *ts;
	size_t klen = strlen(key);
	size_t vlen = strlen(value);

	ts = __mmc_ts_get();
	if (unlikely(!ts))
		return -EINVAL;
	/* save current cache contents so we can restore it on failure */
	memcpy(&ts->cache_tmp_backup, &ts->cache, sizeof(ts->cache_tmp_backup));

	p = mmc_ts_find(ts, key, klen);
	if (p) {
		/* we are replacing existing entry,
		 * empty value (vlen == 0) removes entry completely.
		 */
		size_t cur_len = strlen(p) + 1;
		size_t new_len = vlen ? klen + 1 + vlen + 1 : 0;

		if (cur_len != new_len) {
			/* we need to move stuff around */

			if ((ts->cache.len - cur_len) + new_len >
			     sizeof(ts->cache.data))
				goto no_space;

			memmove(p + new_len, p + cur_len,
				ts->cache.len - (p - ts->cache.data + cur_len));

			ts->cache.len = (ts->cache.len - cur_len) + new_len;
		} else if (!strcmp(p + klen + 1, value)) {
			/* skip update if new value is the same as the old one */
			res = 0;
			goto out;
		}

		if (vlen) {
			p += klen + 1;
			memcpy(p, value, vlen);
			p[vlen] = '\0';
		}
	} else {
		size_t len = klen + 1 + vlen + 1;

		/* don't do anything if value is empty */
		if (!vlen) {
			res = 0;
			goto out;
		}

		if (ts->cache.len + len > sizeof(ts->cache.data))
			goto no_space;

		/* add new entry at the end */
		p = ts->cache.data + ts->cache.len - 1;
		memcpy(p, key, klen);
		p += klen;
		*p++ = '=';
		memcpy(p, value, vlen);
		p += vlen;
		*p++ = '\0';
		*p = '\0';
		ts->cache.len += len;
	}
	++ts->cache.version;
	res = mmc_ts_commit(ts);
	if (unlikely(res))
		memcpy(&ts->cache, &ts->cache_tmp_backup, sizeof(ts->cache));

	goto out;

    no_space:
	printk(KERN_WARNING DRV_NAME ": no space left for '%s=%s'\n",
	       key, value);
	res = -ENOSPC;
    out:
	__mmc_ts_put(ts);

	return res;

}

void mmc_ts_get(const char *key, char *value, unsigned int size)
{
	const char *p;
	struct mmc_ts_priv *ts;
	size_t klen = strlen(key);

	BUG_ON(!size);

	*value = '\0';

	ts = __mmc_ts_get();
	if (unlikely(!ts))
		return;

	p = mmc_ts_find(ts, key, klen);
	if (p)
		strlcpy(value, p + klen + 1, size);

	__mmc_ts_put(ts);

}

/* Round-up to the next power-of-2,
 * from "Hacker's Delight" by Henry S. Warren.
 */
static inline u32 clp2(u32 x)
{
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

int mmc_ts_init(void)
{
	int res,dev;
	struct mmc *mmc;
	struct mmc_ts_priv *ts;
	struct partitions *part_info = NULL;

	// get dev by name
	dev = find_dev_num_by_partition_name(CONFIG_FLASH_TS_PARTITION);
	if (dev < 0) {
		printf("Cannot find dev.\n");
		return 1;
	}
	mmc = find_mmc_device(dev);
	if (!mmc) {
		return 1;
	}

	//init mmc
	mmc_init(mmc);
	//init fts data struct
	ts = malloc(sizeof(*ts));
	if (unlikely(!ts)) {
		res = -ENOMEM;
		printk(KERN_ERR DRV_NAME ": failed to allocate memory\n");
	}

	mutex_init(&ts->lock);
	ts->mmc = mmc;
	ts->mmc_read_write_unit = ts->mmc->read_bl_len;
	ts->chunk = clp2((sizeof(struct flash_ts) + ts->mmc->read_bl_len - 1) &
			~(ts->mmc->read_bl_len - 1));

	/* dev*/
	ts->dev = dev;

	/* get fts part info*/
	part_info = find_mmc_partition_by_name(CONFIG_FLASH_TS_PARTITION);
	if (part_info == NULL) {
		kfree(ts);
		printf("get partition info failed !!\n");
		return -1;
	}

	/* init partiton info to ts data struct*/
	ts->part_info = part_info;
	ts->mmc_ts_offset = ts->part_info->offset;
	ts->mmc_ts_size = ts->part_info->size;

	/* default empty state */
	set_to_default_empty_state(ts);

	/* scan mmc partition for the most recent record */
	res = mmc_ts_scan(ts, dev);
	if (unlikely(res)) {
		goto out_free;
	}

	if (ts->cache.version)
		printk(KERN_INFO DRV_NAME ": v%u loaded from 0x%08llx\n",
		       ts->cache.version, ts->offset);

	res = misc_register(&mmc_ts_miscdev);
	if (unlikely(res)) {
		goto out_free;
	}

	__ts = ts;

out_free:
	kfree(ts);

	return 0;
}
