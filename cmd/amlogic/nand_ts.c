/*
 * MTD-based transactional key-value store
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
#include <linux/mtd/mtd.h>
#include <amlogic/nand_ts.h>

#define DRV_DESC        	"MTD-based key-value storage"

/* Internal state */
struct nand_ts_priv {
	struct mutex lock;
	struct mtd_info *mtd;

	/* chunk size, >= sizeof(struct flash_ts) */
	size_t chunk;

	/* current record offset within MTD device */
	loff_t offset;

	/* in-memory copy of flash content */
	struct flash_ts cache;

	/* temporary buffers
	 *  - one backup for failure rollback
	 *  - another for read-after-write verification
	 */
	struct flash_ts cache_tmp_backup;
	struct flash_ts cache_tmp_verify;
};

static struct nand_ts_priv *__ts;
static int nand_is_blank(const void *buf, size_t size)
{
	size_t i;
	const unsigned int *data = (const unsigned int *)buf;
	size /= sizeof(data[0]);

	for (i = 0; i < size; i++)
		if (data[i] != 0xffffffff)
			return 0;
	return 1;
}

static void nand_erase_callback(struct erase_info *ctx)
{
#ifndef __UBOOT__
	wake_up((wait_queue_head_t*)ctx->priv);
#endif
}

static int nand_erase(struct mtd_info *mtd, loff_t off)
{
	struct erase_info ei = {0};
	int res;

	wait_queue_head_t waitq;
	DECLARE_WAITQUEUE(wait, current);
        init_waitqueue_head(&waitq);

	ei.mtd = mtd;
        ei.len = mtd->erasesize;
	ei.addr = off;
	ei.callback = nand_erase_callback;
        ei.priv = (unsigned long)&waitq;

	/* Yes, this is racy, but safer than just leaving
	 * partition writeable all the time.
	 */
	mtd->flags |= MTD_WRITEABLE;

	res = mtd_erase(mtd, &ei);
        if (!res) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		add_wait_queue(&waitq, &wait);
		if (ei.state != MTD_ERASE_DONE && ei.state != MTD_ERASE_FAILED)
			schedule();
		remove_wait_queue(&waitq, &wait);
		set_current_state(TASK_RUNNING);

		res = ei.state == MTD_ERASE_FAILED ? -EIO : 0;
	}
	mtd->flags &= ~MTD_WRITEABLE;

	if (unlikely(res))
		printk(KERN_ERR DRV_NAME
		       ": nand_erase(0x%08llx) failed, errno %d\n",
		       off, res);
	return res;
}

static int nand_write(struct mtd_info *mtd, loff_t off,
		       const void *buf, size_t size)
{
	int res = 0;

	mtd->flags |= MTD_WRITEABLE;
	while (size) {
		size_t retlen;
		res = mtd_write(mtd, off, size, &retlen, buf);
		if (likely(!res)) {
			off += retlen;
			buf += retlen;
			size -= retlen;
		} else {
			printk(KERN_ERR DRV_NAME
			       ": nand_write(0x%08llx, %zu) failed, errno %d\n",
			       off, size, res);
			break;
		}
	}
	mtd->flags &= ~MTD_WRITEABLE;

	return res;
}

static int nand_read(struct mtd_info *mtd, loff_t off, void *buf, size_t size)
{
	int res = 0;
	while (size) {
		size_t retlen;
		res = mtd_read(mtd, off, size, &retlen, buf);
		if (!res || res == -EUCLEAN) {
			off += retlen;
			buf += retlen;
			size -= retlen;
		} else {
			printk(KERN_WARNING DRV_NAME
			       ": nand_read() failed, errno %d\n", res);
			break;
		}
	}
	return res;
}

static char *nand_ts_find(struct nand_ts_priv *ts, const char *key,
			   size_t key_len)
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

static inline u32 nand_ts_crc(const struct flash_ts *cache)
{
	const unsigned char *p;
	u32 crc = 0;
	size_t len;

	/* skip magic and crc fields */
	len = cache->len + 2 * sizeof(u32);
	p = (const unsigned char*)&cache->len;

	while (len--) {
		int i;

		crc ^= *p++;
		for (i = 0; i < 8; i++)
			crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
	}
	return crc ^ ~0;
}

static void set_to_default_empty_state(struct nand_ts_priv *ts)
{
	ts->offset = ts->mtd->size - ts->chunk;
	ts->cache.magic = FLASH_TS_MAGIC;
	ts->cache.version = 0;
	ts->cache.len = 1;
	ts->cache.data[0] = '\0';
	ts->cache.crc = nand_ts_crc(&ts->cache);
}

/* Verifies cache consistency and locks it */
static struct nand_ts_priv *__nand_ts_get(void)
{
	struct nand_ts_priv *ts = __ts;

	if (likely(ts)) {
		mutex_lock(&ts->lock);
		if (unlikely(ts->cache.crc != nand_ts_crc(&ts->cache))) {
			printk(KERN_CRIT DRV_NAME
			       ": memory corruption detected\n");
			mutex_lock(&ts->lock);
			ts = NULL;
		}
	} else {
		printk(KERN_ERR DRV_NAME ": not initialized yet\n");
	}

	return ts;
}

static inline void __nand_ts_put(struct nand_ts_priv *ts)
{
	mutex_unlock(&ts->lock);
}

static int nand_ts_commit(struct nand_ts_priv *ts)
{
	struct mtd_info *mtd = ts->mtd;
	loff_t off = ts->offset + ts->chunk;
	/* we try to make two passes to handle non-erased blocks
	 * this should only matter for the inital pass over the whole device.
	 */
	int max_iterations = mtd_div_by_eb(mtd->size, mtd) * 2;
	size_t size = ALIGN(FLASH_TS_HDR_SIZE + ts->cache.len, ts->chunk);

	/* fill unused part of data */
	memset(ts->cache.data + ts->cache.len, 0xff,
	       sizeof(ts->cache.data) - ts->cache.len);

	while (max_iterations--) {
		/* wrap around */
		if (off >= mtd->size)
			off = 0;

		/* new block? */
		if (!(off & (mtd->erasesize - 1))) {
			if (mtd_block_isbad(mtd, off)) {
				/* skip this block */
				off += mtd->erasesize;
				continue;
			}

			if (unlikely(nand_erase(mtd, off))) {
				/* skip this block */
				off += mtd->erasesize;
				continue;
			}
		}

		/* write and read back to veryfy */
		if (nand_write(mtd, off, &ts->cache, size) ||
		    nand_read(mtd, off, &ts->cache_tmp_verify, size)) {
			/* hmm, probably unclean block, skip it for now */
			off = (off + mtd->erasesize) & ~(mtd->erasesize - 1);
			continue;
		}

		/* compare */
		if (memcmp(&ts->cache, &ts->cache_tmp_verify, size)) {
			printk(KERN_WARNING DRV_NAME
			       ": record v%u read mismatch @ 0x%08llx\n",
				ts->cache.version, off);
			/* skip this block for now */
			off = (off + mtd->erasesize) & ~(mtd->erasesize - 1);
			continue;
		}

		/* for new block, erase the previous block after write done,
		 * it's to speed up nand_ts_scan
		 */
		if (!(off & (mtd->erasesize - 1))) {
			loff_t pre_block_base = ts->offset & ~(mtd->erasesize - 1);
			loff_t cur_block_base = off & ~(mtd->erasesize - 1);
			if (cur_block_base != pre_block_base)
				nand_erase(mtd, pre_block_base);
		}
		ts->offset = off;
		printk(KERN_DEBUG DRV_NAME ": record v%u commited @ 0x%08llx\n",
		       ts->cache.version, off);
		return 0;
	}

	printk(KERN_ERR DRV_NAME ": commit failure\n");
	return -EIO;
}

int nand_ts_set(const char *key, const char *value)
{
	struct nand_ts_priv *ts;
	size_t klen = strlen(key);
	size_t vlen = strlen(value);
	int res;
	char *p;

	ts = __nand_ts_get();
	if (unlikely(!ts))
		return -EINVAL;

	/* save current cache contents so we can restore it on failure */
	memcpy(&ts->cache_tmp_backup, &ts->cache, sizeof(ts->cache_tmp_backup));

	p = nand_ts_find(ts, key, klen);
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
	ts->cache.crc = nand_ts_crc(&ts->cache);
	res = nand_ts_commit(ts);
	if (unlikely(res))
		memcpy(&ts->cache, &ts->cache_tmp_backup, sizeof(ts->cache));
	goto out;

    no_space:
	printk(KERN_WARNING DRV_NAME ": no space left for '%s=%s'\n",
	       key, value);
	res = -ENOSPC;
    out:
	__nand_ts_put(ts);

	return res;
}

void nand_ts_get(const char *key, char *value, unsigned int size)
{
	size_t klen = strlen(key);
	struct nand_ts_priv *ts;
	const char *p;

	BUG_ON(!size);

	*value = '\0';

	ts = __nand_ts_get();
	if (unlikely(!ts))
		return;

	p = nand_ts_find(ts, key, klen);
	if (p)
		strlcpy(value, p + klen + 1, size);

	__nand_ts_put(ts);
}

static inline u32 nand_ts_check_header(const struct flash_ts *cache)
{
	if (cache->magic == FLASH_TS_MAGIC &&
	    cache->version &&
	    cache->len && cache->len <= sizeof(cache->data) &&
	    cache->crc == nand_ts_crc(cache) &&
	    /* check correct null-termination */
	    !cache->data[cache->len - 1] &&
	    (cache->len == 1 || !cache->data[cache->len - 2])) {
		/* all is good */
		return cache->version;
	}

	return 0;
}

static int __init nand_ts_scan(struct nand_ts_priv *ts)
{
	struct mtd_info *mtd = ts->mtd;
	int res, good_blocks = 0;
	loff_t off = 0;

	do {
		/* new block ? */
		if (!(off & (mtd->erasesize - 1))) {
			if (mtd_block_isbad(mtd, off)) {
				printk(KERN_INFO DRV_NAME
				       ": skipping bad block @ 0x%08llx\n",
				       off);
				off += mtd->erasesize;
				continue;
			} else
				++good_blocks;
		}

		res = nand_read(mtd, off, &ts->cache_tmp_verify,
				 sizeof(ts->cache_tmp_verify));
		if (!res) {
			u32 version =
			    nand_ts_check_header(&ts->cache_tmp_verify);
			if (version > ts->cache.version) {
				memcpy(&ts->cache, &ts->cache_tmp_verify,
				       sizeof(ts->cache));
				ts->offset = off;
			}
			if (0 == version &&
				nand_is_blank(&ts->cache_tmp_verify,
					sizeof(ts->cache_tmp_verify))) {
				/* skip the whole block if chunk is blank */
				off = (off + mtd->erasesize) & ~(mtd->erasesize - 1);
			} else {
				off += ts->chunk;
			}
		} else {
			off += ts->chunk;
		}
	} while (off < mtd->size);

	if (unlikely(!good_blocks)) {
		printk(KERN_ERR DRV_NAME ": no good blocks\n");
		return -ENODEV;
	}

	if (unlikely(good_blocks < 2))
		printk(KERN_WARNING DRV_NAME ": less than 2 good blocks,"
					     " reliability is not guaranteed\n");
	return 0;
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

int nand_ts_init(void)
{
	static bool do_init = false;
        if (do_init)
                return 0;

        do_init = true;
	struct nand_ts_priv *ts;
	struct mtd_info *mtd;
	int res;

	mtd = get_mtd_device_nm(CONFIG_FLASH_TS_PARTITION);
	if (unlikely(IS_ERR(mtd))) {
		printk(KERN_ERR DRV_NAME
		       ": mtd partition '" CONFIG_FLASH_TS_PARTITION
		       "' not found\n");
		return -ENODEV;
	}

	/* we need at least two erase blocks */
	if (unlikely(mtd->size < 2 * mtd->erasesize)) {
		printk(KERN_ERR DRV_NAME ": mtd partition is too small\n");
		res = -ENODEV;
		goto out_put;
	}

	/* make sure both page and block sizes are power-of-2
	 * (this will make chunk size determination simpler).
	 */
	if (unlikely(!is_power_of_2(mtd->writesize) ||
		     !is_power_of_2(mtd->erasesize))) {
		res = -ENODEV;
		printk(KERN_ERR DRV_NAME ": unsupported MTD geometry\n");
		goto out_put;
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (unlikely(!ts)) {
		res = -ENOMEM;
		printk(KERN_ERR DRV_NAME ": failed to allocate memory\n");
		goto out_put;
	}

	mutex_init(&ts->lock);
	ts->mtd = mtd;

	/* determine chunk size so it doesn't cross block boundary,
	 * is multiple of page size and there is no wasted space in a block.
	 * We assume page and block sizes are power-of-2.
	 */
	ts->chunk = clp2((sizeof(struct flash_ts) + mtd->writesize - 1) &
			  ~(mtd->writesize - 1));
	if (unlikely(ts->chunk > mtd->erasesize)) {
		res = -ENODEV;
		printk(KERN_ERR DRV_NAME ": MTD block size is too small\n");
		goto out_free;
	}

	/* default empty state */
	set_to_default_empty_state(ts);

	/* scan flash partition for the most recent record */
	res = nand_ts_scan(ts);
	if (unlikely(res))
		goto out_free;

	if (ts->cache.version)
		printk(KERN_INFO DRV_NAME ": v%u loaded from 0x%08llx\n",
		       ts->cache.version, ts->offset);

	/* "Protect" MTD partition from direct user-space write access */
	mtd->flags &= ~MTD_WRITEABLE;

	res = misc_register(&flash_ts_miscdev);
	if (unlikely(res))
		goto out_free;

	__ts = ts;

	return 0;

    out_free:
	kfree(ts);

    out_put:
	put_mtd_device(mtd);
	return res;
}
