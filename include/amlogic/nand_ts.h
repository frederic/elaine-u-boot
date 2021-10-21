#ifndef _NAND_TS_H
#define _NAND_TS_H

int nand_ts_init(void);

/* Get/set value, returns 0 on success */
int nand_ts_set(const char *key, const char *value);
void nand_ts_get(const char *key, char *value, unsigned int size);

/* Get value as an integer, if missing/invalid return 'default_value' */
int nand_ts_get_int(const char *key, int default_value);

#endif  /* _NAND_TS_H */
