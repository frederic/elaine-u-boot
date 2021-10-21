#ifndef _MMC_TS_H
#define _MMC_TS_H

int mmc_ts_init(void);

/* Get/set value, returns 0 on success */
int mmc_ts_set(const char *key, const char *value);
void mmc_ts_get(const char *key, char *value, unsigned int size);

/* Get value as an integer, if missing/invalid return 'default_value' */
int mmc_ts_get_int(const char *key, int default_value);
#endif  /* _MMC_TS_H */
