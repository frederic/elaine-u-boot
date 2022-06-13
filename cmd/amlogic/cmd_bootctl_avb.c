/*
 * (C) Copyright 2013 Amlogic, Inc
 *
 * This file is used to run commands from misc partition
 * More detail to check the command "run bcb_cmd" usage
 *
 * cheng.wang@amlogic.com,
 * 2015-04-23 @ Shenzhen
 *
 */
#include <common.h>
#include <command.h>
#include <environment.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <config.h>
#include <asm/arch/io.h>
#include <partition_table.h>
#include <libavb.h>
#include <version.h>
#include <amlogic/storage.h>
#include <asm/arch/secure_apb.h>

#ifdef CONFIG_BOOTLOADER_CONTROL_BLOCK

#define AB_METADATA_MISC_PARTITION_OFFSET 2048

#define MISCBUF_SIZE  2080

#ifdef CONFIG_G_AB_SYSTEM
/* offset definition of slot x */
#define SLOT_PRIORITY_OFFSET            0
#define SLOT_TRIES_REMAINING_OFFSET     8
#define SLOT_SUCCESSFUL_BOOT_OFFSET     16
#define SLOT_STICKY_REG_VALID_OFFSET    24
#define SLOT_MISC_CORRUPTION_OFFSET     25
#define SLOT_NEXT_BOOT_OFFSET           24
#define SLOT_CUR_BOOT_OFFSET            28
#endif

/* Magic for the A/B struct when serialized. */
#define AVB_AB_MAGIC "\0AB0"
#define AVB_AB_MAGIC_LEN 4

/* Versioning for the on-disk A/B metadata - keep in sync with avbtool. */
#define AVB_AB_MAJOR_VERSION 1
#define AVB_AB_MINOR_VERSION 0

/* Size of AvbABData struct. */
#define AVB_AB_DATA_SIZE 32

/* Maximum values for slot data */
#define AVB_AB_MAX_PRIORITY 15
#define AVB_AB_MAX_TRIES_REMAINING 7

/* Struct used for recording per-slot metadata.
 *
 * When serialized, data is stored in network byte-order.
 */
typedef struct AvbABSlotData {
  /* Slot priority. Valid values range from 0 to AVB_AB_MAX_PRIORITY,
   * both inclusive with 1 being the lowest and AVB_AB_MAX_PRIORITY
   * being the highest. The special value 0 is used to indicate the
   * slot is unbootable.
   */
  uint8_t priority;

  /* Number of times left attempting to boot this slot ranging from 0
   * to AVB_AB_MAX_TRIES_REMAINING.
   */
  uint8_t tries_remaining;

  /* Non-zero if this slot has booted successfully, 0 otherwise. */
  uint8_t successful_boot;

  /* Reserved for future use. */
  uint8_t reserved[1];
} AvbABSlotData;

/* Struct used for recording A/B metadata.
 *
 * When serialized, data is stored in network byte-order.
 */
typedef struct AvbABData {
  /* Magic number used for identification - see AVB_AB_MAGIC. */
  uint8_t magic[AVB_AB_MAGIC_LEN];

  /* Version of on-disk struct - see AVB_AB_{MAJOR,MINOR}_VERSION. */
  uint8_t version_major;
  uint8_t version_minor;

  /* Padding to ensure |slots| field start eight bytes in. */
  uint8_t reserved1[2];

  /* Per-slot metadata. */
  AvbABSlotData slots[2];

  /* Reserved for future use. */
  uint8_t reserved2[12];

  /* CRC32 of all 28 bytes preceding this field. */
  uint32_t crc32;
}AvbABData;

/*static int clear_misc_partition(char *clearbuf, int size)
{
    char *partition = "misc";

    memset(clearbuf, 0, size);
    if (store_write((unsigned char *)partition,
        0, size, (unsigned char *)clearbuf) < 0) {
        printf("failed to clear %s.\n", partition);
        return -1;
    }

    return 0;
}*/

#ifdef CONFIG_G_AB_SYSTEM
void boot_info_update(AvbABData *info)
{
    unsigned int sticky_reg0_val;
    unsigned int sticky_reg1_val;

    sticky_reg0_val = readl(P_AO_RTI_STICKY_REG0);
    sticky_reg1_val = readl(P_AO_RTI_STICKY_REG1);

    info->slots[0].priority = (sticky_reg0_val >> SLOT_PRIORITY_OFFSET) & 0xff;
    info->slots[0].tries_remaining = (sticky_reg0_val >> SLOT_TRIES_REMAINING_OFFSET) & 0xff;
    info->slots[0].successful_boot = (sticky_reg0_val >> SLOT_SUCCESSFUL_BOOT_OFFSET) & 0xff;
    info->slots[1].priority = (sticky_reg1_val >> SLOT_PRIORITY_OFFSET) & 0xff;
    info->slots[1].tries_remaining = (sticky_reg1_val >> SLOT_TRIES_REMAINING_OFFSET) & 0xff;
    info->slots[1].successful_boot = (sticky_reg1_val >> SLOT_SUCCESSFUL_BOOT_OFFSET) & 0xff;
}

void boot_info_save_to_reg(AvbABData *info, char *slot)
{
    unsigned int sticky_reg0_val = 0;
    unsigned int sticky_reg1_val = 0;
    unsigned int cur_slot_index = 0;
    unsigned int next_boot_slot_index = 0;

    sticky_reg0_val = ((info->slots[0].priority & 0xff) << SLOT_PRIORITY_OFFSET) |
        ((info->slots[0].tries_remaining & 0xff) << SLOT_TRIES_REMAINING_OFFSET) |
        ((info->slots[0].successful_boot & 0xff) << SLOT_SUCCESSFUL_BOOT_OFFSET);
    sticky_reg1_val = ((info->slots[1].priority & 0xff) << SLOT_PRIORITY_OFFSET) |
        ((info->slots[1].tries_remaining & 0xff) << SLOT_TRIES_REMAINING_OFFSET) |
        ((info->slots[1].successful_boot & 0xff) << SLOT_SUCCESSFUL_BOOT_OFFSET);

    /* mark the sticky register is valid */
    sticky_reg0_val |= (1 << SLOT_STICKY_REG_VALID_OFFSET);
    /* set slot_x to cur_slot_index and next_boot_slot_index */
    if (!strcmp(slot, "a")) {
        cur_slot_index = 0;
        next_boot_slot_index = 0;
    }
    else if (!strcmp(slot, "b")) {
        cur_slot_index = 1;
        next_boot_slot_index = 1;
    }
    sticky_reg1_val |= ((cur_slot_index << SLOT_CUR_BOOT_OFFSET) |
        (next_boot_slot_index << SLOT_NEXT_BOOT_OFFSET));

    writel(sticky_reg0_val, P_AO_RTI_STICKY_REG0);
    writel(sticky_reg1_val, P_AO_RTI_STICKY_REG1);
}
#endif

bool boot_info_validate(AvbABData* info)
{
    if (memcmp(info->magic, AVB_AB_MAGIC, AVB_AB_MAGIC_LEN) != 0) {
        printf("Magic %s is incorrect.\n", info->magic);
        return false;
    }
    if (info->version_major > AVB_AB_MAJOR_VERSION) {
        printf("No support for given major version.\n");
        return false;
    }
    return true;
}

void boot_info_reset(AvbABData* info)
{
    memset(info, '\0', sizeof(AvbABData));
    memcpy(info->magic, AVB_AB_MAGIC, AVB_AB_MAGIC_LEN);
    info->version_major = AVB_AB_MAJOR_VERSION;
    info->version_minor = AVB_AB_MINOR_VERSION;
    info->slots[0].priority = AVB_AB_MAX_PRIORITY;
    info->slots[0].tries_remaining = AVB_AB_MAX_TRIES_REMAINING;
    info->slots[0].successful_boot = 0;
    info->slots[1].priority = AVB_AB_MAX_PRIORITY - 1;
    info->slots[1].tries_remaining = AVB_AB_MAX_TRIES_REMAINING;
    info->slots[1].successful_boot = 0;
}

void dump_boot_info(AvbABData* info)
{
    printf("info->magic = %s\n", info->magic);
    printf("info->version_major = %d\n", info->version_major);
    printf("info->version_minor = %d\n", info->version_minor);
    printf("info->slots[0].priority = %d\n", info->slots[0].priority);
    printf("info->slots[0].tries_remaining = %d\n", info->slots[0].tries_remaining);
    printf("info->slots[0].successful_boot = %d\n", info->slots[0].successful_boot);
    printf("info->slots[1].priority = %d\n", info->slots[1].priority);
    printf("info->slots[1].tries_remaining = %d\n", info->slots[1].tries_remaining);
    printf("info->slots[1].successful_boot = %d\n", info->slots[1].successful_boot);

    printf("info->crc32 = %d\n", info->crc32);
}

#ifndef CONFIG_G_AB_SYSTEM
static bool slot_is_bootable(AvbABSlotData* slot) {
  return slot->priority > 0 &&
         (slot->successful_boot || (slot->tries_remaining > 0));
}
#endif

int get_active_slot(AvbABData* info) {
    if (info->slots[0].priority > info->slots[1].priority)
        return 0;
    else
        return 1;
}

#ifdef CONFIG_G_AB_SYSTEM
int boot_info_set_active_slot(AvbABData* info, int slot, int switch_flag)
{
    unsigned int other_slot_number;

    /* Make requested slot top priority, unsuccessful, and with max tries. */
    info->slots[slot].priority = AVB_AB_MAX_PRIORITY;
    info->slots[slot].tries_remaining = AVB_AB_MAX_TRIES_REMAINING;
    info->slots[slot].successful_boot = 0;

    /* Ensure other slot doesn't have as high a priority. */
    other_slot_number = 1 - slot;
    if (switch_flag == 0) {
        /* switch good slot to good slot, reduce the priority of the current slot */
        if (info->slots[other_slot_number].priority == AVB_AB_MAX_PRIORITY) {
            info->slots[other_slot_number].priority = AVB_AB_MAX_PRIORITY - 1;
        }
    }
    else {
        /* switch bad slot to good slot, mark the current slot to bad */
        info->slots[other_slot_number].priority = 0;
        info->slots[other_slot_number].tries_remaining = 0;
        info->slots[other_slot_number].successful_boot = 0;
    }

#ifdef BL33_DEBUG_PRINT
    dump_boot_info(info);
#endif
    return 0;
}

#else
int boot_info_set_active_slot(AvbABData* info, int slot)
{
    unsigned int other_slot_number;

    /* Make requested slot top priority, unsuccessful, and with max tries. */
    info->slots[slot].priority = AVB_AB_MAX_PRIORITY;
    info->slots[slot].tries_remaining = AVB_AB_MAX_TRIES_REMAINING;
    info->slots[slot].successful_boot = 0;

    /* Ensure other slot doesn't have as high a priority. */
    other_slot_number = 1 - slot;
    if (info->slots[other_slot_number].priority == AVB_AB_MAX_PRIORITY) {
        info->slots[other_slot_number].priority = AVB_AB_MAX_PRIORITY - 1;
    }

    dump_boot_info(info);

    return 0;
}
#endif

int boot_info_open_partition(char *miscbuf)
{
    char *partition = "misc";
    //int i;
    printf("Start read %s partition datas!\n", partition);
    if (store_read((unsigned char *)partition,
        0, MISCBUF_SIZE, (unsigned char *)miscbuf) < 0) {
        printf("failed to store read %s.\n", partition);
        return -1;
    }

    /*for (i = AB_METADATA_MISC_PARTITION_OFFSET;i < (AB_METADATA_MISC_PARTITION_OFFSET+AVB_AB_DATA_SIZE);i++)
        printf("buf: %c \n", miscbuf[i]);*/
    return 0;
}

bool boot_info_load(AvbABData *out_info, char *miscbuf)
{
    memcpy(out_info, miscbuf+AB_METADATA_MISC_PARTITION_OFFSET, AVB_AB_DATA_SIZE);
    dump_boot_info(out_info);
    return true;
}

bool boot_info_save(AvbABData *info, char *miscbuf)
{
    // remove dangerous code that can brick device : if AvbABSlotData.tries_remaining reaches 0, BL2 refuses to boot ("A/B system: slot_a and slot_b are unbootalbe")
    return true;
}

static int do_GetValidSlot(
    cmd_tbl_t * cmdtp,
    int flag,
    int argc,
    char * const argv[])
{
#ifdef CONFIG_G_AB_SYSTEM
    unsigned int cur_slot_index;
    unsigned int sticky_reg0_val = 0;
    unsigned int sticky_reg1_val = 0;
    unsigned int misc_corruption_flag = 0;

    sticky_reg1_val = readl(P_AO_RTI_STICKY_REG1);
    sticky_reg0_val = readl(P_AO_RTI_STICKY_REG0);
    cur_slot_index = (sticky_reg1_val >> SLOT_CUR_BOOT_OFFSET) & 0xf;
    misc_corruption_flag = (sticky_reg0_val >> SLOT_MISC_CORRUPTION_OFFSET) & 0x1;

    pr_info("The index of active slot is:0x%x\n", cur_slot_index);
    if (cur_slot_index == 0) {
        env_set("active_slot","_a");
        env_set("boot_part","boot_a");
        env_set("slot-suffixes","a");
    }
    else if (cur_slot_index == 1) {
        env_set("active_slot","_b");
        env_set("boot_part","boot_b");
        env_set("slot-suffixes","b");
    }
    else {
        printf("Invalid slot num\n");
        return -1;
    }
    has_boot_slot = 1;
    has_system_slot = 1;

    /* the misc image is damaged, it's should boot from slot a */
    if ((misc_corruption_flag == 1) && (cur_slot_index == 0)) {
        /* reconstruct a default misc image and write to misc partition */
        char miscbuf[MISCBUF_SIZE] = {0};
        AvbABData info;
        pr_info("reconstruct a default misc image\n");
        boot_info_reset(&info);
        boot_info_save_to_reg(&info, "a");
        boot_info_save(&info, miscbuf);
    }

#else
    char miscbuf[MISCBUF_SIZE] = {0};
    AvbABData info;
    int slot;
    bool bootable_a, bootable_b;

    if (argc != 1) {
        return cmd_usage(cmdtp);
    }

    boot_info_open_partition(miscbuf);
    boot_info_load(&info, miscbuf);

    if (!boot_info_validate(&info)) {
        printf("boot-info is invalid. Resetting.\n");
        boot_info_reset(&info);
        boot_info_save(&info, miscbuf);
    }

    slot = get_active_slot(&info);
    printf("active slot = %d\n", slot);

    bootable_a = slot_is_bootable(&(info.slots[0]));
    bootable_b = slot_is_bootable(&(info.slots[1]));

    if ((slot == 0) && (bootable_a)) {
        if (has_boot_slot == 1) {
            env_set("active_slot","_a");
            env_set("boot_part","boot_a");
            env_set("slot-suffixes","0");
        }
        else {
            env_set("active_slot","normal");
            env_set("boot_part","boot");
        }
        return 0;
    }

    if ((slot == 1) && (bootable_b)) {
        if (has_boot_slot == 1) {
            env_set("active_slot","_b");
            env_set("boot_part","boot_b");
            env_set("slot-suffixes","1");
        }
        else {
            env_set("active_slot","normal");
            env_set("boot_part","boot");
        }
        return 0;
    }
#endif

    return 0;
}

static int do_SetActiveSlot(
    cmd_tbl_t * cmdtp,
    int flag,
    int argc,
    char * const argv[])
{
    char miscbuf[MISCBUF_SIZE] = {0};
    AvbABData info;

#ifdef CONFIG_G_AB_SYSTEM
    if (argc != 3) {
#else
    if (argc != 2) {
#endif
        return cmd_usage(cmdtp);
    }

    if (has_boot_slot == 0) {
        printf("device is not ab mode\n");
        return -1;
    }

#ifdef CONFIG_G_AB_SYSTEM
    unsigned int switch_flag;
    /* get default data from sticky register */
    boot_info_reset(&info);
    boot_info_update(&info);
    switch_flag = (unsigned int)simple_strtoul(argv[2], NULL, 16);
    if ((switch_flag != 0) && (switch_flag != 1)) {
        printf("Invalid flag for switch slot\n");
	return -1;
    }
#else
    boot_info_open_partition(miscbuf);
    boot_info_load(&info, miscbuf);

    if (!boot_info_validate(&info)) {
        printf("boot-info is invalid. Resetting.\n");
        boot_info_reset(&info);
        boot_info_save(&info, miscbuf);
    }
#endif

    if (strcmp(argv[1], "a") == 0) {
        env_set("active_slot","_a");
        env_set("boot_part","boot_a");
        printf("set active slot a \n");
#ifdef CONFIG_G_AB_SYSTEM
        env_set("slot-suffixes","a");
        boot_info_set_active_slot(&info, 0, switch_flag);
#else
        env_set("slot-suffixes","0");
        boot_info_set_active_slot(&info, 0);
#endif
    } else if (strcmp(argv[1], "b") == 0) {
        env_set("active_slot","_b");
        env_set("boot_part","boot_b");
        printf("set active slot b \n");
#ifdef CONFIG_G_AB_SYSTEM
        env_set("slot-suffixes","b");
        boot_info_set_active_slot(&info, 1, switch_flag);
#else
        env_set("slot-suffixes","1");
        boot_info_set_active_slot(&info, 1);
#endif
    } else {
        printf("error input slot\n");
        return -1;
    }

#ifdef CONFIG_G_AB_SYSTEM
    /* save new ab data to sticky register */
    boot_info_save_to_reg(&info, argv[1]);
#endif
    boot_info_save(&info, miscbuf);

    return 0;
}

int do_GetSystemMode (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    char* system;
#ifdef CONFIG_SYSTEM_AS_ROOT
    system = CONFIG_SYSTEM_AS_ROOT;
    strcpy(system, CONFIG_SYSTEM_AS_ROOT);
    printf("CONFIG_SYSTEM_AS_ROOT: %s \n", CONFIG_SYSTEM_AS_ROOT);
    if (strcmp(system, "systemroot") == 0) {
        env_set("system_mode","1");
    }
    else {
        env_set("system_mode","0");
    }
#else
    env_set("system_mode","0");
#endif

    return 0;
}

int do_GetAvbMode (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#ifdef CONFIG_AVB2
    char* avbmode;
    avbmode = CONFIG_AVB2;
    strcpy(avbmode, CONFIG_AVB2);
    printf("CONFIG_AVB2: %s \n", CONFIG_AVB2);
    if (strcmp(avbmode, "avb2") == 0) {
        env_set("avb2","1");
    }
    else {
        env_set("avb2","0");
    }
#else
    env_set("avb2","0");
#endif

    return 0;
}

#ifdef CONFIG_G_AB_SYSTEM
/* write sticky register to misc partitioin */
int do_SyncAvbData(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    char miscbuf[MISCBUF_SIZE] = {0};
    AvbABData info;

    /* init ab data */
    boot_info_reset(&info);
    /* copy ab data from sticky register to info struct */
    boot_info_update(&info);
    /* write ab data to misc partition */
    boot_info_save(&info, miscbuf);
    /* clear stick register */
    writel(0x0, P_AO_RTI_STICKY_REG0);
    writel(0x0, P_AO_RTI_STICKY_REG1);

    return 0;
}

U_BOOT_CMD(
    sync_ab_data, 1,0, do_SyncAvbData,
    "sync_ab_data",
    "\nThis command will write the sticky register to misc partitioin\n"
    "So you can execute command: sync_ab_data"
);

int do_GetSlotState(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    unsigned int sticky_reg_val = 0;
    unsigned int result = 0;
    char cur_retry_count_str[8] = {0};

    if (argc != 3) {
        return cmd_usage(cmdtp);
    }

    if (!strcmp(argv[1], "a")) {
        sticky_reg_val = readl(P_AO_RTI_STICKY_REG0);
    }

    else if (!strcmp(argv[1], "b")) {
        sticky_reg_val = readl(P_AO_RTI_STICKY_REG1);
    }
    else {
        return cmd_usage(cmdtp);
    }

    if (!strcmp(argv[2], "successful")) {
        result = (sticky_reg_val >> SLOT_SUCCESSFUL_BOOT_OFFSET) & 0xff;
    }
    else if (!strcmp(argv[2], "unbootable")) {
        result = (sticky_reg_val >> SLOT_PRIORITY_OFFSET) & 0xff;
    }
    else if (!strcmp(argv[2], "retry-count")) {
        result = (sticky_reg_val >> SLOT_TRIES_REMAINING_OFFSET) & 0xff;
	/* For command: fastboot getvar slot-retry-count:slot_x */
	snprintf(cur_retry_count_str, sizeof(cur_retry_count_str), "%d", result);
	env_set("cur_retry_count", cur_retry_count_str);
    }
    else {
        return cmd_usage(cmdtp);
    }

    printf("slot %s: %s = %d\n", argv[1], argv[2], result);
    return result;
}

U_BOOT_CMD(
    get_slot_state, 3,0, do_GetSlotState,
    "get_slot_state",
    "\nThis command will get the slot status\n"
    "So you can execute command: get_slot_state [a|b] [successful|unbootable|retry-count]"
);
#endif

#endif /* CONFIG_BOOTLOADER_CONTROL_BLOCK */

U_BOOT_CMD(
    get_valid_slot, 2, 0, do_GetValidSlot,
    "get_valid_slot",
    "\nThis command will choose valid slot to boot up which saved in misc\n"
    "partition by mark to decide whether execute command!\n"
    "So you can execute command: get_valid_slot"
);

#ifdef CONFIG_G_AB_SYSTEM
U_BOOT_CMD(
    set_active_slot, 3, 1, do_SetActiveSlot,
    "set_active_slot",
    "\nThis command will set active slot\n"
    "set_active_slot slot_name switch_flag\n"
    "    slot_name: a|b\n"
    "    switch_flag: 0 - switch good slot to good slot\n"
    "                 1 - switch bad slot to good slot"
);
#else
U_BOOT_CMD(
    set_active_slot, 2, 1, do_SetActiveSlot,
    "set_active_slot",
    "\nThis command will set active slot\n"
    "So you can execute command: set_active_slot a"
);
#endif

U_BOOT_CMD(
    get_system_as_root_mode, 1,	0, do_GetSystemMode,
    "get_system_as_root_mode",
    "\nThis command will get system_as_root_mode\n"
    "So you can execute command: get_system_as_root_mode"
);

U_BOOT_CMD(
    get_avb_mode, 1,	0, do_GetAvbMode,
    "get_avb_mode",
    "\nThis command will get avb mode\n"
    "So you can execute command: get_avb_mode"
);

