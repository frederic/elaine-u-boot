// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2011 Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 */

#include <common.h>
#include <image.h>
#include <android_image.h>
#include <malloc.h>
#include <errno.h>
#define KDTB_MAGIC "KDTB"
#define KDTB_MAGIC_SZ (sizeof(KDTB_MAGIC) - 1)

// The Kernel is packed along with its dtb file
// The format is [header][kernel][dtb file]
// This is the header
typedef struct __attribute__((__packed__)) KDTB_HEADER {
	char magic[KDTB_MAGIC_SZ];
	uint32_t kernel_size;
	uint32_t dtb_size;
} KDTB_HEADER;

typedef struct KDTB_PARSED {
	unsigned char *kernel_addr;
	uint32_t kernel_size;
	unsigned char *dtb_addr;
	uint32_t dtb_size;
} KDTB_PARSED;

static int parse_kern_dtb(unsigned char *kern_dtb, unsigned kern_dtb_size,
		KDTB_PARSED *parsed) {
	KDTB_HEADER header;
	memcpy(&header, kern_dtb, sizeof(header));
	if (memcmp(KDTB_MAGIC, header.magic, KDTB_MAGIC_SZ) != 0) {
		return ANDR_BOOT_KDTB_NOT_FOUND;
	}

	unsigned expected_kernel_size =
		header.kernel_size + header.dtb_size + sizeof(header);
	if (expected_kernel_size != kern_dtb_size) {
		printf("the expected kern-dtb size is: %u\n", expected_kernel_size);
		printf("the actual kern-dtb size is: %u\n", kern_dtb_size);
		// For now, don't error out on this condition. Seems the
		// hdr->kernel_size value is not used in current code, so there is no
		// way to assert this.
	}
	parsed->kernel_addr = kern_dtb + sizeof(header);
	if ((uintptr_t)parsed->kernel_addr & 0x3) {
		printf("Kernel must be 4 byte aligned\n");
		return ANDR_BOOT_KDTB_INVALID;
	}
	parsed->kernel_size = header.kernel_size;
	parsed->dtb_addr = parsed->kernel_addr + header.kernel_size;
	parsed->dtb_size = header.dtb_size;

	return 0;
}
static const unsigned char lzop_magic[] = {
	0x89, 0x4c, 0x5a, 0x4f, 0x00, 0x0d, 0x0a, 0x1a, 0x0a
};

#define ANDROID_IMAGE_DEFAULT_KERNEL_ADDR	0x10008000
static const unsigned char gzip_magic[] = {
	0x1f, 0x8b
};

static char andr_tmp_str[ANDR_BOOT_ARGS_SIZE + 1];

static ulong android_image_get_kernel_addr(const struct andr_img_hdr *hdr)
{
	/*
	 * All the Android tools that generate a boot.img use this
	 * address as the default.
	 *
	 * Even though it doesn't really make a lot of sense, and it
	 * might be valid on some platforms, we treat that adress as
	 * the default value for this field, and try to execute the
	 * kernel in place in such a case.
	 *
	 * Otherwise, we will return the actual value set by the user.
	 */
	if (hdr->kernel_addr == ANDROID_IMAGE_DEFAULT_KERNEL_ADDR)
		return (ulong)hdr + hdr->page_size;

	return hdr->kernel_addr;
}

/**
 * android_image_get_kernel() - processes kernel part of Android boot images
 * @hdr:	Pointer to image header, which is at the start
 *			of the image.
 * @verify:	Checksum verification flag. Currently unimplemented.
 * @os_data:	Pointer to a ulong variable, will hold os data start
 *			address.
 * @os_len:	Pointer to a ulong variable, will hold os data length.
 *
 * This function returns the os image's start address and length. Also,
 * it appends the kernel command line to the bootargs env variable.
 *
 * Return: Zero, os start address and length on success,
 *		otherwise on failure.
 */
int android_image_get_kernel(const struct andr_img_hdr *hdr, int verify,
			     ulong *os_data, ulong *os_len)
{
	u32 kernel_addr = android_image_get_kernel_addr(hdr);

	/*
	 * Not all Android tools use the id field for signing the image with
	 * sha1 (or anything) so we don't check it. It is not obvious that the
	 * string is null terminated so we take care of this.
	 */
	ulong end;
	strncpy(andr_tmp_str, hdr->name, ANDR_BOOT_NAME_SIZE);
	andr_tmp_str[ANDR_BOOT_NAME_SIZE] = '\0';
	if (strlen(andr_tmp_str))
		printf("Android's image name: %s\n", andr_tmp_str);

	printf("Kernel load addr 0x%08x size %u KiB\n",
	       kernel_addr, DIV_ROUND_UP(hdr->kernel_size, 1024));

	int len = 0;
	if (*hdr->cmdline) {
		printf("Kernel command line: %s\n", hdr->cmdline);
		len += strlen(hdr->cmdline);
	}

	char *bootargs = env_get("bootargs");
	if (bootargs)
		len += strlen(bootargs);

	char *newbootargs = malloc(len + 2);
	if (!newbootargs) {
		puts("Error: malloc in android_image_get_kernel failed!\n");
		return -ENOMEM;
	}
	*newbootargs = '\0';

	if (bootargs) {
		strcpy(newbootargs, bootargs);
		strcat(newbootargs, " ");
	}
	if (*hdr->cmdline)
		strcat(newbootargs, hdr->cmdline);

	env_set("bootargs", newbootargs);

	// Kernel or kernel-dtb file exists at this location.
	void *kernel = (unsigned char *)hdr + hdr->page_size;

	KDTB_PARSED parsed = {0};
	int ret = parse_kern_dtb(kernel, hdr->kernel_size, &parsed);

	if (ret == ANDR_BOOT_KDTB_INVALID)
		return ret;

	if (ret == 0) {
		// kernel-dtb file found.
		printf("found kdtb.\n");
		if (os_data) {
			*os_data = (ulong)parsed.kernel_addr;
		}
		if (os_len)
			*os_len = parsed.kernel_size;
		images.ft_len = parsed.dtb_size;
		images.ft_addr = (char *)parsed.dtb_addr;

		end = (ulong)hdr;
		end += hdr->page_size;
		end += ALIGN(hdr->kernel_size, hdr->page_size);
		images.rd_start = end;
		return 0;
	}
	if (os_data) {
		*os_data = (ulong)kernel;
	}
	if (os_len)
		*os_len = hdr->kernel_size;

#if defined(CONFIG_ANDROID_BOOT_IMAGE)
			//ulong end;
			images.ft_len = (ulong)(hdr->second_size);
			end = (ulong)hdr;
			end += hdr->page_size;
			end += ALIGN(hdr->kernel_size, hdr->page_size);
			images.rd_start = end;
			end += ALIGN(hdr->ramdisk_size, hdr->page_size);
			images.ft_addr = (char *)end;
#endif

	return 0;
}

int android_image_check_header(const struct andr_img_hdr *hdr)
{
	return memcmp(ANDR_BOOT_MAGIC, hdr->magic, ANDR_BOOT_MAGIC_SIZE);
}

ulong android_image_get_end(const struct andr_img_hdr *hdr)
{
	ulong end;
	/*
	 * The header takes a full page, the remaining components are aligned
	 * on page boundary
	 */
	end = (ulong)hdr;
	end += hdr->page_size;
	end += ALIGN(hdr->kernel_size, hdr->page_size);
	end += ALIGN(hdr->ramdisk_size, hdr->page_size);
	end += ALIGN(hdr->second_size, hdr->page_size);

	return end;
}

ulong android_image_get_kload(const struct andr_img_hdr *hdr)
{
	return android_image_get_kernel_addr(hdr);
}

int android_image_get_ramdisk(const struct andr_img_hdr *hdr,
			      ulong *rd_data, ulong *rd_len)
{
	if (!hdr->ramdisk_size) {
		*rd_data = *rd_len = 0;
		return -1;
	}

	printf("RAM disk load addr 0x%08x size %u KiB\n",
	       hdr->ramdisk_addr, DIV_ROUND_UP(hdr->ramdisk_size, 1024));

	*rd_data = (unsigned long)hdr;
	*rd_data += hdr->page_size;
	*rd_data += ALIGN(hdr->kernel_size, hdr->page_size);

	*rd_len = hdr->ramdisk_size;
	return 0;
}

int android_image_get_second(const struct andr_img_hdr *hdr,
			      ulong *second_data, ulong *second_len)
{
	if (!hdr->second_size) {
		*second_data = *second_len = 0;
		return -1;
	}

	*second_data = (unsigned long)hdr;
	*second_data += hdr->page_size;
	*second_data += ALIGN(hdr->kernel_size, hdr->page_size);
	*second_data += ALIGN(hdr->ramdisk_size, hdr->page_size);

	printf("second address is 0x%lx\n",*second_data);

	*second_len = hdr->second_size;
	return 0;
}

#if !defined(CONFIG_SPL_BUILD)
/**
 * android_print_contents - prints out the contents of the Android format image
 * @hdr: pointer to the Android format image header
 *
 * android_print_contents() formats a multi line Android image contents
 * description.
 * The routine prints out Android image properties
 *
 * returns:
 *     no returned results
 */
void android_print_contents(const struct andr_img_hdr *hdr)
{
	const char * const p = IMAGE_INDENT_STRING;
	/* os_version = ver << 11 | lvl */
	u32 os_ver = hdr->os_version >> 11;
	u32 os_lvl = hdr->os_version & ((1U << 11) - 1);

	printf("%skernel size:      %x\n", p, hdr->kernel_size);
	printf("%skernel address:   %x\n", p, hdr->kernel_addr);
	printf("%sramdisk size:     %x\n", p, hdr->ramdisk_size);
	printf("%sramdisk addrress: %x\n", p, hdr->ramdisk_addr);
	printf("%ssecond size:      %x\n", p, hdr->second_size);
	printf("%ssecond address:   %x\n", p, hdr->second_addr);
	printf("%stags address:     %x\n", p, hdr->tags_addr);
	printf("%spage size:        %x\n", p, hdr->page_size);
	/* ver = A << 14 | B << 7 | C         (7 bits for each of A, B, C)
	 * lvl = ((Y - 2000) & 127) << 4 | M  (7 bits for Y, 4 bits for M) */
	printf("%sos_version:       %x (ver: %u.%u.%u, level: %u.%u)\n",
	       p, hdr->os_version,
	       (os_ver >> 7) & 0x7F, (os_ver >> 14) & 0x7F, os_ver & 0x7F,
	       (os_lvl >> 4) + 2000, os_lvl & 0x0F);
	printf("%sname:             %s\n", p, hdr->name);
	printf("%scmdline:          %s\n", p, hdr->cmdline);
}
#endif

ulong android_image_get_comp(const struct andr_img_hdr *os_hdr)
{
	int i;
	unsigned char *src = (unsigned char *)os_hdr + os_hdr->page_size;

	KDTB_PARSED parsed = {0};
	if (parse_kern_dtb(src, os_hdr->kernel_size, &parsed) == 0) {
		src = parsed.kernel_addr;
	}
	unsigned char *begin = src;

	/* read magic: 9 first bytes */
	for (i = 0; i < ARRAY_SIZE(lzop_magic); i++) {
		if (*src++ != lzop_magic[i])
			break;
	}
	if (i == ARRAY_SIZE(lzop_magic))
		return IH_COMP_LZO;

	src = begin;
	for (i = 0; i < ARRAY_SIZE(gzip_magic); i++) {
		if (*src++ != gzip_magic[i])
			break;
	}
	if (i == ARRAY_SIZE(gzip_magic))
		return IH_COMP_GZIP;

	return IH_COMP_NONE;
}
int android_image_need_move(ulong *img_addr, const struct andr_img_hdr *hdr)
{
	ulong kernel_load_addr = android_image_get_kload(hdr);
	ulong img_start = *img_addr;
	ulong val = 0;
	if (kernel_load_addr > img_start)
		val = kernel_load_addr - img_start;
	else
		val = img_start - kernel_load_addr;
	if (android_image_get_comp(hdr) == IH_COMP_NONE)
		return 0;
	if (val < 32*1024*1024) {
		ulong total_size = android_image_get_end(hdr)-(ulong)hdr;
		void *reloc_addr = malloc(total_size);
		if (!reloc_addr) {
			puts("Error: malloc in  android_image_need_move failed!\n");
			return -ENOMEM;
		}
		printf("reloc_addr =%lx\n", (ulong)reloc_addr);
		memset(reloc_addr, 0, total_size);
		memmove(reloc_addr, hdr, total_size);
		*img_addr = (ulong)reloc_addr;
		pr_info("copy done\n");
	}
	return 0;
}
