#include <common.h>
#include <bootm.h>
#include <command.h>
#include <image.h>
#include <malloc.h>
#include <asm/arch/io.h>
#include <asm/arch/secure_apb.h>

//#define AML_DT_DEBUG
#ifdef AML_DT_DEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...) ((void)0)
#endif

#define AML_DT_IND_LENGTH_V1		4	/*fixed*/
#define AML_DT_IND_LENGTH_V2		16	/*fixed*/

#define AML_DT_IND_LENGTH			16
#define AML_DT_ID_VARI_TOTAL		3	//Total 3 strings
#define AML_DT_EACH_ID_INT			(AML_DT_IND_LENGTH / 4)

/*Latest version: v2*/
#define AML_DT_VERSION_OFFSET		4
#define AML_DT_TOTAL_DTB_OFFSET		8
#define AML_DT_FIRST_DTB_OFFSET		12
//#define AML_DT_DTB_HEADER_SIZE	(8+(AML_DT_IND_LENGTH * AML_DT_ID_VARI_TOTAL))
#define AML_DT_DTB_DT_INFO_OFFSET	0
//#define AML_DT_DTB_OFFSET_OFFSET	(AML_DT_IND_LENGTH * AML_DT_ID_VARI_TOTAL)
//#define AML_DT_DTB_SIZE_OFFSET	16

#define AML_DT_UBOOT_ENV	"aml_dt"
#define DT_HEADER_MAGIC		0xedfe0dd0	/*header of dtb file*/
#define AML_DT_HEADER_MAGIC	0x5f4c4d41	/*"AML_", multi dtbs supported*/

#define IS_GZIP_FORMAT(data)		((data & (0x0000FFFF)) == (0x00008B1F))
#define GUNZIP_BUF_SIZE				(0x500000) /* 5MB */
#define DTB_MAX_SIZE				(0x40000) /* 256KB */

//#define readl(addr) (*(volatile unsigned int*)(addr))
extern int checkhw(char * name);

unsigned long __attribute__((unused))
	get_multi_dt_entry(unsigned long fdt_addr){
	unsigned int dt_magic = readl(fdt_addr);
	unsigned int dt_total = 0;
	unsigned int dt_tool_version = 0;
	unsigned int gzip_format = 0;
	void * gzip_buf = NULL;
	unsigned long dt_entry = fdt_addr;

	printf("      Amlogic multi-dtb tool\n");

	/* first check the file header, support GZIP format */
	gzip_format = IS_GZIP_FORMAT(dt_magic);
	if (gzip_format) {
		printf("      GZIP format, decompress...\n");
		gzip_buf = malloc(GUNZIP_BUF_SIZE);
		memset(gzip_buf, 0, GUNZIP_BUF_SIZE);
		unsigned long unzip_size = GUNZIP_BUF_SIZE;
		gunzip(gzip_buf, GUNZIP_BUF_SIZE, (void *)fdt_addr, &unzip_size);
		dbg_printf("      DBG: unzip_size: 0x%x\n", (unsigned int)unzip_size);
		if (unzip_size > GUNZIP_BUF_SIZE) {
			printf("      Warning! GUNZIP overflow...\n");
		}
		fdt_addr = (unsigned long)gzip_buf;
		dt_magic = readl(fdt_addr);
	}

	dbg_printf("      DBG: fdt_addr: 0x%x\n", (unsigned int)fdt_addr);
	dbg_printf("      DBG: dt_magic: 0x%x\n", (unsigned int)dt_magic);
	dbg_printf("      DBG: gzip_format: %d\n", gzip_format);

	/*printf("      Process device tree. dt magic: %x\n", dt_magic);*/
	if (dt_magic == DT_HEADER_MAGIC) {/*normal dtb*/
		printf("      Single dtb detected\n");
		if (gzip_format) {
			memcpy((void *)dt_entry, (void *)fdt_addr, DTB_MAX_SIZE);
			fdt_addr = dt_entry;
			if (gzip_buf)
				free(gzip_buf);
		}
		return fdt_addr;
	}
	else if (dt_magic == AML_DT_HEADER_MAGIC) {/*multi dtb*/
		printf("      Multi dtb detected\n");
		/* check and set aml_dt */
		int i = 0;
		char aml_dt_buf[64] = {0};

		/* update 2016.07.27, checkhw and setenv everytime,
		or else aml_dt will set only once if it is reserved */
#if 1
		checkhw(aml_dt_buf);
#else
		char *aml_dt = getenv(AML_DT_UBOOT_ENV);
		/* if aml_dt not exist or env not ready, get correct dtb by name */
		if (NULL == aml_dt)
			checkhw(aml_dt_buf);
		else
			memcpy(aml_dt_buf, aml_dt, (strlen(aml_dt)>64?64:(strlen(aml_dt)+1)));
#endif

		unsigned int aml_dt_len = strlen(aml_dt_buf);
		if (aml_dt_len <= 0) {
			printf("      Get env aml_dt failed!\n");
			return fdt_addr;
		}

		/*version control, compatible with v1*/
		dt_tool_version = readl(fdt_addr + AML_DT_VERSION_OFFSET);
		unsigned int aml_each_id_length=0;
		unsigned int aml_dtb_offset_offset;
		unsigned int aml_dtb_header_size;
		if (dt_tool_version == 1)
			aml_each_id_length = 4;
		else if(dt_tool_version == 2)
			aml_each_id_length = 16;

		aml_dtb_offset_offset = aml_each_id_length * AML_DT_ID_VARI_TOTAL;
		aml_dtb_header_size = 8+(aml_each_id_length * AML_DT_ID_VARI_TOTAL);
		printf("      Multi dtb tool version: v%d .\n", dt_tool_version);

		/*fdt_addr + 0x8: num of dtbs*/
		dt_total = readl(fdt_addr + AML_DT_TOTAL_DTB_OFFSET);
		printf("      Support %d dtbs.\n", dt_total);

		/* split aml_dt to 3 strings */
		char *tokens[3] = {NULL, NULL, NULL};
		char *sep_start = aml_dt_buf;
		for (i = 0; i < AML_DT_ID_VARI_TOTAL; i++) {
			tokens[i] = strsep(&sep_start, "_");
		}
		printf("        aml_dt soc: %s platform: %s variant: %s\n", tokens[0],
		       tokens[1], tokens[2]);

		/*match and print result*/
		char **dt_info;
		dt_info = (char **)malloc(sizeof(char *)*AML_DT_ID_VARI_TOTAL);
		for (i = 0; i < AML_DT_ID_VARI_TOTAL; i++)
			dt_info[i] = (char *)malloc(sizeof(char)*aml_each_id_length);
		unsigned int dtb_match_num = 0xffff;
		unsigned int x = 0, y = 0, z = 0; //loop counter
		unsigned int read_data;
		for (i = 0; i < dt_total; i++) {
			for (x = 0; x < AML_DT_ID_VARI_TOTAL; x++) {
				for (y = 0; y < aml_each_id_length; y+=4) {
					read_data = readl(fdt_addr + AML_DT_FIRST_DTB_OFFSET + \
						 i * aml_dtb_header_size + AML_DT_DTB_DT_INFO_OFFSET + \
						 (x * aml_each_id_length) + y);
					dt_info[x][y+0] = (read_data >> 24) & 0xff;
					dt_info[x][y+1] = (read_data >> 16) & 0xff;
					dt_info[x][y+2] = (read_data >> 8) & 0xff;
					dt_info[x][y+3] = (read_data >> 0) & 0xff;
				}
				for (z=0; z<aml_each_id_length; z++) {
					/*fix string with \0*/
					if (0x20 == (uint)dt_info[x][z]) {
						dt_info[x][z] = '\0';
					}
				}
				//printf("dt_info[x]: %s\n", dt_info[x]);
				//printf("strlen(dt_info[x]): %d\n", strlen(dt_info[x]));
			}
			if (dt_tool_version == 1)
				printf("        dtb %d soc: %.4s   plat: %.4s   vari: %.4s\n", i, (char *)(dt_info[0]), (char *)(dt_info[1]), (char *)(dt_info[2]));
			else if(dt_tool_version == 2)
				printf("        dtb %d soc: %.16s   plat: %.16s   vari: %.16s\n", i, (char *)(dt_info[0]), (char *)(dt_info[1]), (char *)(dt_info[2]));
			uint match_str_counter = 0;
			for (z=0; z<AML_DT_ID_VARI_TOTAL; z++) {
				/*must match 3 strings*/
				if (!strncmp(tokens[z], (char *)(dt_info[z]), strlen(tokens[z])) && \
					(strlen(tokens[z]) == strlen(dt_info[z])))
					match_str_counter++;
			}
			if (match_str_counter == AML_DT_ID_VARI_TOTAL) {
				//printf("Find match dtb\n");
				dtb_match_num = i;
			}
			for (z=0; z<AML_DT_ID_VARI_TOTAL; z++) {
				/*clear data for next loop*/
				memset(dt_info[z], 0, sizeof(aml_each_id_length));
			}
		}
		/*clean malloc memory*/
		for (i = 0; i < AML_DT_ID_VARI_TOTAL; i++) {
			if (dt_info[i])
				free(dt_info[i]);
		}
		if (dt_info)
			free(dt_info);

		/*if find match dtb, return address, or else return main entrance address*/
		if (0xffff != dtb_match_num) {
			printf("      Find match dtb: %d\n", dtb_match_num);
			/*this offset is based on dtb image package, so should add on base address*/
			fdt_addr = (fdt_addr + readl(fdt_addr + AML_DT_FIRST_DTB_OFFSET + \
				dtb_match_num * aml_dtb_header_size + aml_dtb_offset_offset));
			if (gzip_format) {
				memcpy((void *)dt_entry, (void *)fdt_addr, DTB_MAX_SIZE);
				fdt_addr = dt_entry;
				if (gzip_buf)
					free(gzip_buf);
			}
			return fdt_addr;
		}
		else{
			printf("      Not match any dtb.\n");
			if (gzip_buf)
				free(gzip_buf);
			return fdt_addr;
		}
	}
	else {
		printf("      Cannot find legal dtb!\n");
		return fdt_addr;
	}
	return 0;
}
