#ifndef __AML_SLCNAND_H_
#define __AML_SLCNAND_H_


#include <linux/mtd/mtd.h>
#include <linux/mtd/rawnand.h>
#include <linux/mtd/nand_ecc.h>
#include <nand.h>
#include <amlogic/aml_nand.h>
#include "aml_hwctrl.h"
#include <partition_table.h>
#include <linux/mtd/partitions.h>

#include <amlogic/aml_rsv.h>
#include <amlogic/aml_mtd.h>

#define NAND_MAX_DEVICE 		4

#define CONFIG_MTD_PARTITIONS 1

/*MAX page list cnt for usrdef mode*/
#define NAND_PAGELIST_CNT 16

/*nand read retry info,max equal to zero,
*that means no need retry*/
struct nand_retry_t {
		unsigned id;
		unsigned max;
		unsigned no_rb;
};

typedef struct _nand_cmd {
		unsigned char type;
		unsigned char val;
}nand_cmd_t;

struct meson_slcnand_platdata {
	u32 reg_base;
	u8 nand_user_mode;
	u8 nand_ran_mode;
};

/**must same with arch/arm/include/asm/nand.h
*ext bits:
*      bit 26: pagelist enable flag,
*      bit 24: a2 cmd enable flag,
*      bit 23: no_rb,
*      bit 22: large. large for what?
*      bit 19: randomizer mode.
*      bit 14-16: ecc mode
*      bit 13: short mode
*      bit 6-12: short page size
*      bit 0-5: ecc pages.
***/
typedef struct nand_setup {
		union {
			uint32_t d32;
			struct {
				unsigned cmd:22;
				unsigned large_page:1;  //22
				unsigned no_rb:1;       //23 from efuse
				unsigned a2:1;          //24
				unsigned reserved25:1;  //25
				unsigned page_list:1;   //26
				unsigned sync_mode:2;   //27 from efuse
				unsigned size:2;        //29 from efuse
				unsigned active:1;      //31
			}b;
		}cfg;
		uint16_t id;
		uint16_t max;  //id:0x100 user,max:0 disable.
} nand_setup_t;

typedef struct _ext_info{
		uint32_t read_info;             //nand_read_info;
		uint32_t new_type;              //new_nand_type;
		uint32_t page_per_blk;  //pages_in_block;
		uint32_t xlc;                   //slc=1,mlc=2,tlc=3;
		uint32_t ce_mask;
		/*copact mode: boot means whole uboot
		it's easy to understand that copies off_type
		bl2 and fip are the same.
		* discrete mode,boot means the fip only*/
		uint32_t boot_num;
		uint32_t each_boot_pages;
		/*for comptible reason*/
		uint32_t bbt_occupy_pages;
		uint32_t bbt_start_block;
} ext_info_t;

#define NAND_FIPMODE_COMPACT (0)
#define NAND_FIPMODE_DISCRETE  (1)

/* if you don't need skip the bad blocks when address
 * partitions,please enable this macro.
 * #define CONFIG_NOT_SKIP_BAD_BLOCK
 */

typedef struct _fip_info {
		uint16_t version; //version
		uint16_t mode;    //compact or discrete
		uint32_t fip_start; //fip start,pages
}fip_info_t;

 typedef struct _nand_page0 {
		nand_setup_t nand_setup;		//8
		unsigned char page_list[16];	//16
		nand_cmd_t retry_usr[32];		//64(32 cmd max I/F)
		ext_info_t ext_info;			//72
		/*added for slc nand in mtd drivers 20170503*/
		fip_info_t fip_info;
 }nand_page0_t;  //384 byte max.


typedef union nand_core_clk {
	/*raw register data*/
	uint32_t d32;
	struct {
		unsigned clk_div:7;
		unsigned reserved0:1;
		unsigned clk_en:1;
		unsigned clk_sel:3;
		unsigned not_used:20;
	}b;
}nand_core_clk_t;

/***************ERROR CODING*******************/
#define NAND_CHIP_ID_ERR			1
#define	NAND_SHIP_BAD_BLOCK_ERR		2
#define NAND_CHIP_REVB_HY_ERR		3

/** Register defination **/
#define NAND_BOOT_NAME	"bootloader"
#define	NAND_NORMAL_NAME	"nandnormal"
#define NAND_RESERVED_NAME	"nandreserved"


#define BOOT_PAGES_PER_COPY	(1024)
#define	BOOT_COPY_NUM	(BOOT_TOTAL_PAGES/BOOT_PAGES_PER_COPY)


#define AML_CHIP_NONE_RB	4
#define AML_INTERLEAVING_MODE	8

#define AML_NAND_CE0	0xe
#define AML_NAND_CE1	0xd
#define AML_NAND_CE2	0xb
#define	AML_NAND_CE3	0x7

#define AML_BADBLK_POS 0

#define NAND_ECC_OPTIONS_MASK 			0x0000000f
#define	NAND_PLANE_OPTIONS_MASK 		0x000000f0
#define NAND_TIMING_OPTIONS_MASK		0x00000f00
#define NAND_BUSW_OPTIONS_MASK			0x0000f000
#define NAND_INTERLEAVING_OPTIONS_MASK	0x000f0000

#define NAND_TWO_PLANE_MODE	0x00000010
#define NAND_TIMING_MODE0	0x00000000
#define NAND_TIMING_MODE1	0x00000100
#define NAND_TIMING_MODE2	0x00000200
#define NAND_TIMING_MODE3	0x00000300
#define NAND_TIMING_MODE4	0x00000400
#define	NAND_TIMING_MODE5	0x00000500
#define NAND_INTERLEAVING_MODE	0x00010000

#define DEFAULT_T_REA	40
#define DEFAULT_T_RHOH	0
#define NAND_DEFAULT_OPTIONS	(NAND_TIMING_MODE5 | NAND_ECC_BCH8_MODE)

#define AML_NAND_BUSY_TIMEOUT	0x40000
#define AML_DMA_BUSY_TIMEOUT	0x100000
#define MAX_ID_LEN	8

#define	NAND_CMD_PLANE2_READ_START 0x06
#define NAND_CMD_TWOPLANE_PREVIOS_READ 0x60
#define NAND_CMD_TWOPLANE_READ1 0x5a
#define NAND_CMD_TWOPLANE_READ2 0xa5
#define NAND_CMD_TWOPLANE_WRITE2_MICRO 0x80
#define NAND_CMD_TWOPLANE_WRITE2 0x81
#define NAND_CMD_DUMMY_PROGRAM 0x11
#define NAND_CMD_ERASE1_END 0xd1
#define NAND_CMD_MULTI_CHIP_STATUS 0x78

#define ONFI_TIMING_ADDR 0x01

#define NAND_STATUS_READY_MULTI 0x20

#define NAND_BLOCK_GOOD 0
#define NAND_BLOCK_BAD	1
#define NAND_FACTORY_BAD 2
#define BAD_BLK_LEVEL 2
#define	FACTORY_BAD_BLOCK_ERROR	159
#define MINI_PART_SIZE	0x100000
#define NAND_MINI_PART_NUM	4
#define MAX_BAD_BLK_NUM	2000
#define MAX_MTD_PART_NUM	16
#define MAX_MTD_PART_NAME_LEN	24

#define NAND_SYS_PART_SIZE	0x8000000

struct aml_nand_flash_dev {
	char *name;
	u8 id[MAX_ID_LEN];
	unsigned pagesize;
	unsigned chipsize;
	unsigned erasesize;
	unsigned oobsize;
	unsigned internal_chipnr;
	unsigned T_REA;
	unsigned T_RHOH;
	u8 onfi_mode;
	unsigned options;
};

struct aml_nand_part_info {
	char mtd_part_magic[4];
	char mtd_part_name[MAX_MTD_PART_NAME_LEN];
	uint64_t size;
	uint64_t offset;
	u_int32_t mask_flags;
};

struct aml_nand_bch_desc {
    char * name;
    unsigned bch_mode;
    unsigned bch_unit_size;
    unsigned bch_bytes;
    unsigned user_byte_mode;
};

struct aml_nand_chip {
	struct nand_chip chip;
	struct hw_controller *controller;

	/* mtd info */
	u8 mfr_type;
	unsigned onfi_mode;
	unsigned T_REA;
	unsigned T_RHOH;
	unsigned options;
	unsigned page_size;
	unsigned block_size;
	unsigned oob_size;
	unsigned virtual_page_size;
	unsigned virtual_block_size;
	u8 plane_num;
	u8 internal_chipnr;
	unsigned internal_page_nums;

	unsigned internal_chip_shift;
	unsigned int ran_mode;
	unsigned int rbpin_mode;
	unsigned int rbpin_detect;
	unsigned int short_pgsz;
	/* bch for infopage on short mode */
	unsigned int bch_info;

	unsigned bch_mode;
	u8 user_byte_mode;
	u8 ops_mode;
	u8 cached_prog_status;
	u8 max_bch_mode;
	unsigned valid_chip[MAX_CHIP_NUM];
	unsigned page_addr;
	unsigned char *aml_nand_data_buf;
	unsigned int *user_info_buf;
	int8_t *block_status;
	unsigned int toggle_mode;
	u8 ecc_cnt_limit;
	u8 ecc_cnt_cur;
	u8 ecc_max;
	unsigned zero_cnt;
	unsigned oob_fill_cnt;
	unsigned boot_oob_fill_cnt;
	/*add property field for key private data*/
	int dtbsize;
	int keysize;
	uint32_t boot_copy_num; /*tell how many bootloader copies*/

	u8 key_protect;
	unsigned char *rsv_data_buf;

	struct meson_rsv_handler_t *rsv;

	struct aml_nand_bch_desc *bch_desc;
	/* platform info */
	struct aml_nand_platform *platform;

	/* device info */
	struct device *device;

	unsigned max_ecc;
	struct ecc_desc_s * ecc;
	// unsigned onfi_mode;
	unsigned err_sts;
	/* plateform operation function*/
	void (*aml_nand_hw_init)(struct aml_nand_chip *aml_chip);
	void (*aml_nand_adjust_timing)(struct aml_nand_chip *aml_chip);
	int (*aml_nand_options_confirm)(struct aml_nand_chip *aml_chip);
	void (*aml_nand_cmd_ctrl)(struct aml_nand_chip *aml_chip,
		int cmd,  unsigned int ctrl);
	void (*aml_nand_select_chip)(struct aml_nand_chip *aml_chip,
		int chipnr);
	void (*aml_nand_write_byte)(struct aml_nand_chip *aml_chip,
		uint8_t data);
	void (*aml_nand_get_user_byte)(struct aml_nand_chip *aml_chip,
		unsigned char *oob_buf, int byte_num);
	void (*aml_nand_set_user_byte)(struct aml_nand_chip *aml_chip,
		unsigned char *oob_buf, int byte_num);
	void (*aml_nand_command)(struct aml_nand_chip *aml_chip,
		unsigned command, int column, int page_addr, int chipnr);
	int (*aml_nand_wait_devready)(struct aml_nand_chip *aml_chip,
		int chipnr);
	int (*aml_nand_dma_read)(struct aml_nand_chip *aml_chip,
		unsigned char *buf, int len, unsigned bch_mode);
	int (*aml_nand_dma_write)(struct aml_nand_chip *aml_chip,
		unsigned char *buf, int len, unsigned bch_mode);
	int (*aml_nand_hwecc_correct)(struct aml_nand_chip *aml_chip,
		unsigned char *buf, unsigned size, unsigned char *oob_buf);
	int (*aml_nand_block_bad_scrub)(struct mtd_info *mtd);
};

struct aml_nand_platform {
	struct aml_nand_flash_dev *nand_flash_dev;
	char *name;
	unsigned chip_enable_pad;
	unsigned ready_busy_pad;

	/* DMA RD/WR delay loop  timing */
	unsigned int T_REA;	/* for dma  wating delay */
	/* not equal of (nandchip->delay, which is for dev ready func)*/
	unsigned int T_RHOH;
	unsigned int ran_mode; 	/*def close, for all part*/
	unsigned int rbpin_mode;	/*may get from romboot*/
	unsigned int rbpin_detect;
	unsigned int short_pgsz;	/*zero means no short*/

	struct aml_nand_chip *aml_chip;
	struct platform_nand_data platform_nand_data;
};

struct aml_nand_device {
	struct aml_nand_platform *aml_nand_platform;
	u8 dev_num;
};

static inline struct aml_nand_chip *mtd_to_nand_chip(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	return container_of(chip, struct aml_nand_chip, chip);
}

#ifdef CONFIG_PARAMETER_PAGE
struct parameter_page {
	/*0~31 byte: Revision information and features block*/
	unsigned char signature[4];
	unsigned short ver;
	unsigned short feature;
	unsigned short opt_commd;
	unsigned short reserve0;
	unsigned short ex_para_page_len;
	unsigned char num_para_page;
	unsigned char reserve1[17];
	/*32~79 byte: Manufacturer information block*/
	unsigned char dev_manu[12];
	unsigned char dev_model[20];
	unsigned char JEDEC_manu_ID;
	unsigned short date_code;
	unsigned char reserve2[13];
	/*80~127 byte: Memory organization block*/
	unsigned int data_bytes_perpage;
	unsigned short spare_bytes_perpage;
	unsigned int data_bytes_perpartial;
	unsigned short spare_bytes_perpartial;
	unsigned int pages_perblk;
	unsigned int blks_perLUN;
	unsigned char num_LUN;
	/* 4-7: column addr cycles; 0-3: row addr cycles*/
	unsigned char num_addr_cycle;
	unsigned char bits_percell;
	unsigned short max_badblk_perLUN;
	unsigned short blk_edurce;
	/*Guaranteed valid blocks at beginning of target*/
	unsigned char g_v_blk_begin;
	unsigned short blk_edurce_g_v_blk;
	unsigned char progm_perpage;
	unsigned char prt_prog_att;//obsolete
	unsigned char bits_ECC_corretable;
	/*0-3: number of interleaved address bits*/
	unsigned char bits_intleav_addr;
	/*6-7 Reserved (0)
	5 1 = lower bit XNOR block address restriction
	4 1 = read cache supported
	3 Address restrictions for cache operations
	2 1 = program cache supported
	1 1 = no block address restrictions
	0 Overlapped / concurrent interleaving support
	*/
	unsigned char intleav_op_attr;
	unsigned char reserve3[13];
	/*128~163 byte: Electrical parameters block*/
	unsigned char max_io_pin;
	/*6-15 Reserved (0)
	5 1 = supports timing mode 5
	4 1 = supports timing mode 4
	3 1 = supports timing mode 3
	2 1 = supports timing mode 2
	1 1 = supports timing mode 1
	0 1 = supports timing mode 0, shall be 1
	*/
	unsigned short asy_time_mode;
	unsigned short asy_prog_cach_time_mode;	/*obsolete*/
	unsigned short Tprog;	/*Maximum page program time (Ts)*/
	unsigned short Tbers;	/*Maximum block erase time (Ts)*/
	unsigned short Tr;	/*Maximum page read time (Ts)*/
	unsigned short Tccs;	/*Minimum change column setup time (ns)*/
	/* 6-15 Reserved (0)
	5 1 = supports timing mode 5
	4 1 = supports timing mode 4
	3 1 = supports timing mode 3
	2 1 = supports timing mode 2
	1 1 = supports timing mode 1
	0 1 = supports timing mode 0
	*/
	unsigned short src_syn_time_mode;
	/*3-7 Reserved (0)
	2 1 = device supports CLK stopped for data input
	1 1 = typical capacitance values present
	0 tCAD value to use
	*/
	unsigned char src_syn_feature;
	unsigned short CLK_input_pin;
	unsigned short IO_pin;
	unsigned short input_pin;
	unsigned char max_input_pin;
	unsigned char dr_strgth;
	/*Maximum interleaved page read time (Ts)*/
	unsigned short Tir;
	/*Program page register clear enhancement tADL value (ns)*/
	unsigned short Tadl;
	unsigned char reserve4[8];
	/*164~255 byte: Vendor block*/
	unsigned short vd_ver;
	unsigned char vd_spec[88];
	unsigned short int_CRC;
	/*256~ byte: Redundant Parameter Pages*/

}__attribute__ ((__packed__));
#endif

int aml_nand_init(struct aml_nand_chip *aml_chip);

int aml_nand_read_page_raw(struct mtd_info *mtd,
	struct nand_chip *chip, uint8_t *buf, int oob_required, int page);

int aml_nand_write_page_raw(struct mtd_info *mtd,
	struct nand_chip *chip, const uint8_t *buf, int oob_required,
	int page);

int aml_nand_read_page_hwecc(struct mtd_info *mtd,
	struct nand_chip *chip, uint8_t *buf, int oob_required, int page);

int aml_nand_write_page_hwecc(struct mtd_info *mtd,
	struct nand_chip *chip, const uint8_t *buf, int oob_required,
	int page);

int aml_nand_read_oob(struct mtd_info *mtd,
	struct nand_chip *chip, int page);

int aml_nand_write_oob(struct mtd_info *mtd, struct nand_chip *chip, int page);

int aml_nand_block_bad(struct mtd_info *mtd, loff_t ofs);

int aml_nand_block_markbad(struct mtd_info *mtd, loff_t ofs);

void aml_nand_dma_read_buf(struct mtd_info *mtd, uint8_t *buf, int len);

void aml_nand_dma_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len);

void aml_platform_cmd_ctrl(struct aml_nand_chip *aml_chip,
	int cmd, unsigned int ctrl);

void aml_platform_write_byte(struct aml_nand_chip *aml_chip, uint8_t data);

int aml_platform_wait_devready(struct aml_nand_chip *aml_chip, int chipnr);

void aml_platform_get_user_byte(struct aml_nand_chip *aml_chip,
	unsigned char *oob_buf, int byte_num);

void aml_platform_set_user_byte(struct aml_nand_chip *aml_chip,
	unsigned char *oob_buf, int byte_num);

void aml_nand_base_command(struct aml_nand_chip *aml_chip,
	unsigned command, int column, int page_addr, int chipnr);

int aml_nand_block_bad_scrub_update_bbt(struct mtd_info *mtd);

int aml_ubootenv_init(struct aml_nand_chip *aml_chip);

int amlnf_dtb_init(struct aml_nand_chip *aml_chip);

int aml_key_init(struct aml_nand_chip *aml_chip);

int aml_nand_erase_key(struct mtd_info *mtd);

int aml_nand_bbt_check(struct mtd_info *mtd);/*fixed by liuxj*/

int aml_nand_scan(struct mtd_info *mtd, int maxchips);

int aml_nand_write_page_raw(struct mtd_info *mtd,
	struct nand_chip *chip, const uint8_t *buf, int oob_required,
	int page);

int aml_nand_write_page(struct mtd_info *mtd,
	struct nand_chip *chip, uint32_t offset,
	int data_len,
	const uint8_t *buf,
	int oob_required, int page, int raw);

void aml_nand_base_command(struct aml_nand_chip *aml_chip,
	unsigned command, int column, int page_addr, int chipnr);

void aml_nand_command(struct mtd_info *mtd,
	unsigned command, int column, int page_addr);

int aml_nand_wait(struct mtd_info *mtd, struct nand_chip *chip);

int aml_nand_erase_cmd(struct mtd_info *mtd, int page);

int m3_nand_boot_erase_cmd(struct mtd_info *mtd, int page);

int m3_nand_boot_read_page_hwecc(struct mtd_info *mtd,
	struct nand_chip *chip, uint8_t *buf, int oob_required, int page);

int m3_nand_boot_write_page_hwecc(struct mtd_info *mtd,
	struct nand_chip *chip, const uint8_t *buf, int oob_required,
	int page);

int m3_nand_boot_write_page(struct mtd_info *mtd, struct nand_chip *chip,
	uint32_t offset, int data_len, const uint8_t *buf,
	int oob_required, int page, int raw);

int aml_nand_get_fbb_issue(void);

void aml_nand_check_fbb_issue(u8 *dev_id);

#endif
