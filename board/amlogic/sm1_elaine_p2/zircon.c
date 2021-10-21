/*
 * Copyright (c) 2018 The Fuchsia Authors
 *
 * SPDX-License-Identifier:	BSD-3-Clause
 */

#include <common.h>
#include <linux/mtd/partitions.h>
#include <nand.h>
#include <part.h>
#include <emmc_storage.h>
#include <zircon/zircon.h>

#define PDEV_VID_GOOGLE             3
#define PDEV_PID_ASTRO              3

#define NVRAM_LENGTH                (8 * 1024)

const char* BOOTLOADER_VERSION = "zircon-bootloader=0.10";

static const zbi_cpu_config_t cpu_config = {
    .cluster_count = 1,
    .clusters = {
        {
            .cpu_count = 4,
        },
    },
};

static const zbi_mem_range_t mem_config[] = {
    {
        .type = ZBI_MEM_RANGE_RAM,
        .length = 0x60000000, // 1.5 GB
    },
    {
        .type = ZBI_MEM_RANGE_PERIPHERAL,
        .paddr = 0xf5800000,
        .length = 0x0a800000,
    },
    // secmon_reserved:linux,secmon
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x05000000,
        .length = 0x2400000,
    },
    // logo_reserved:linux,meson-fb
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x5f800000,
        .length = 0x800000,
    },
};

static const dcfg_simple_t uart_driver = {
    .mmio_phys = 0xff803000,
    .irq = 225,
};

static const dcfg_arm_gicv2_driver_t gicv2_driver = {
    .mmio_phys = 0xffc00000,
    .gicd_offset = 0x1000,
    .gicc_offset = 0x2000,
    .gich_offset = 0x4000,
    .gicv_offset = 0x6000,
    .ipi_base = 5,
};

static const dcfg_arm_psci_driver_t psci_driver = {
    .use_hvc = false,
    .reboot_args = { 1, 0, 0 },
    .reboot_bootloader_args = { 4, 0, 0 },
    .reboot_recovery_args = { 2, 0, 0 },
};

static const dcfg_arm_generic_timer_driver_t timer_driver = {
    .irq_phys = 30,
};

static const zbi_platform_id_t platform_id = {
    .vid = PDEV_VID_GOOGLE,
    .pid = PDEV_PID_ASTRO,
    .board_name = "astro",
};

enum {
    PART_TPL,
    PART_FTS,
    PART_FACTORY,
    PART_ZIRCON_B,
    PART_ZIRCON_A,
    PART_ZIRCON_R,
    PART_FVM,
    PART_SYS_CONFIG,
    PART_MIGRATION,
    PART_COUNT,
};

#define RECOVERY_SIZE   (16 * 1024 * 1024)
#define SYS_CONFIG_SIZE (1 * 1024 * 1024)
#define MIGRATION_SIZE  (3 * 1024 * 1024)

static zbi_partition_map_t partition_map = {
    // .block_count filled in below
    // .block_size filled in below
    .guid = {},
    .partition_count = PART_COUNT,
    .partitions = {
        {
            .type_guid = GUID_BOOTLOADER_VALUE,
            .name = "tpl",
        },
        {
            .name = "fts",
        },
        {
            .name = "factory",
        },
        {
            .type_guid = GUID_ZIRCON_R_VALUE,
            .name = "zircon-r",
        },
        {
            .type_guid = GUID_ZIRCON_A_VALUE,
            .name = "zircon-a",
        },
        {
            .type_guid = GUID_ZIRCON_B_VALUE,
            .name = "zircon-b",
        },
        {
            .type_guid = GUID_FVM_VALUE,
            .name = "fvm",
        },
        {
            .type_guid = GUID_SYS_CONFIG_VALUE,
            .name = "sys-config",
        },
        {
            .name = "migration",
        },
    },
};

extern struct mtd_partition *get_aml_mtd_partition(void);
extern int get_aml_partition_count(void);

static void add_partition_map(zbi_header_t* zbi) {
    struct mtd_partition* tpl_part = NULL;
    struct mtd_partition* fts_part = NULL;
    struct mtd_partition* factory_part = NULL;
    struct mtd_partition* recovery_part = NULL;
    struct mtd_partition* boot_part = NULL;
    struct mtd_partition* system_part = NULL;
    struct mtd_partition* partitions = get_aml_mtd_partition();
    int partition_count = get_aml_partition_count();
    int i;

    for (i = 0; i < partition_count; i++) {
        struct mtd_partition* part = &partitions[i];
        if (!strcmp("tpl", part->name)) {
            tpl_part = part;
        } else if (!strcmp("fts", part->name)) {
            fts_part = part;
        } else if (!strcmp("factory", part->name)) {
            factory_part = part;
        } else if (!strcmp("recovery", part->name)) {
            recovery_part = part;
        } else if (!strcmp("boot", part->name)) {
            boot_part = part;
        } else if (!strcmp("system", part->name)) {
            system_part = part;
        }
    }

    if (!tpl_part) {
        printf("could not find tpl partition\n");
        return;
    }
    if (!fts_part) {
        printf("could not find fts partition\n");
        return;
    }
    if (!factory_part) {
        printf("could not find factory partition\n");
        return;
    }
    if (!recovery_part) {
        printf("could not find recovery partition\n");
        return;
    }
    if (!boot_part) {
        printf("could not find boot partition\n");
        return;
    }
    if (!system_part) {
        printf("could not find system partition\n");
        return;
    }

    uint32_t block_size = nand_info[1].writesize;
    uint64_t total_size = nand_info[1].size;

    partition_map.block_size = block_size;
    partition_map.block_count = total_size / block_size;

    // map tpl partition to BOOTLOADER
    partition_map.partitions[PART_TPL].first_block = tpl_part->offset / block_size;
    partition_map.partitions[PART_TPL].last_block =
                                ((tpl_part->offset + tpl_part->size) / block_size) - 1;
    // map fts partition to "fts"
    partition_map.partitions[PART_FTS].first_block = fts_part->offset / block_size;
    partition_map.partitions[PART_FTS].last_block =
                                ((fts_part->offset + fts_part->size) / block_size) - 1;
    // map factory partition to "factory"
    partition_map.partitions[PART_FACTORY].first_block = factory_part->offset / block_size;
    partition_map.partitions[PART_FACTORY].last_block =
                                ((factory_part->offset + factory_part->size) / block_size) - 1;
    // map recovery partition to ZIRCON_B
    partition_map.partitions[PART_ZIRCON_B].first_block = recovery_part->offset / block_size;
    partition_map.partitions[PART_ZIRCON_B].last_block =
                                ((recovery_part->offset + recovery_part->size) / block_size) - 1;
    // map boot partition to ZIRCON_A
    partition_map.partitions[PART_ZIRCON_A].first_block = boot_part->offset / block_size;
    partition_map.partitions[PART_ZIRCON_A].last_block =
                                ((boot_part->offset + boot_part->size) / block_size) - 1;
   // ZIRCON_R partition at start of system
    partition_map.partitions[PART_ZIRCON_R].first_block = system_part->offset / block_size;
    partition_map.partitions[PART_ZIRCON_R].last_block =
                                partition_map.partitions[PART_ZIRCON_R].first_block +
                                    (RECOVERY_SIZE / block_size) - 1;
    // FVM follows ZIRCON_R
    partition_map.partitions[PART_FVM].first_block =
                                partition_map.partitions[PART_ZIRCON_R].last_block + 1;
    partition_map.partitions[PART_FVM].last_block =
                            ((total_size - SYS_CONFIG_SIZE - MIGRATION_SIZE) / block_size) - 1;
    // SYS_CONFIG follows FVM
    partition_map.partitions[PART_SYS_CONFIG].first_block =
                                partition_map.partitions[PART_FVM].last_block + 1;
    partition_map.partitions[PART_SYS_CONFIG].last_block =
                                partition_map.partitions[PART_SYS_CONFIG].first_block +
                                    (SYS_CONFIG_SIZE / block_size) - 1;
    // MIGRATION follows SYS_CONFIG
    partition_map.partitions[PART_MIGRATION].first_block =
                                partition_map.partitions[PART_SYS_CONFIG].last_block + 1;
    partition_map.partitions[PART_MIGRATION].last_block =
                                partition_map.partitions[PART_MIGRATION].first_block +
                                    (MIGRATION_SIZE / block_size) - 1;

    printf("Zircon partitions:\n");
    for (i = 0; i < PART_COUNT; i++) {
        printf("  0x%016llx - 0x%016llx : %s\n",
                partition_map.partitions[i].first_block * block_size,
                (partition_map.partitions[i].last_block + 1) * block_size,
                partition_map.partitions[i].name);
    }

    zircon_append_boot_item(zbi, ZBI_TYPE_DRV_PARTITION_MAP, 0, &partition_map,
                            sizeof(zbi_partition_map_t) +
                            partition_map.partition_count * sizeof(zbi_partition_t));
}

int zircon_preboot(zbi_header_t* zbi) {
    // add CPU configuration
    zircon_append_boot_item(zbi, ZBI_TYPE_CPU_CONFIG, 0, &cpu_config,
                    sizeof(zbi_cpu_config_t) +
                    sizeof(zbi_cpu_cluster_t) * cpu_config.cluster_count);

    // allocate crashlog save area before 0x5f800000-0x60000000 reserved area
    zbi_nvram_t nvram;
    nvram.base = 0x5f800000 - NVRAM_LENGTH;
    nvram.length = NVRAM_LENGTH;
    zircon_append_boot_item(zbi, ZBI_TYPE_NVRAM, 0, &nvram, sizeof(nvram));

    // add memory configuration
    zircon_append_boot_item(zbi, ZBI_TYPE_MEM_CONFIG, 0, &mem_config, sizeof(mem_config));

    // add kernel drivers
    zircon_append_boot_item(zbi, ZBI_TYPE_KERNEL_DRIVER, KDRV_AMLOGIC_UART, &uart_driver,
                    sizeof(uart_driver));
    zircon_append_boot_item(zbi, ZBI_TYPE_KERNEL_DRIVER, KDRV_ARM_GIC_V2, &gicv2_driver,
                    sizeof(gicv2_driver));
    zircon_append_boot_item(zbi, ZBI_TYPE_KERNEL_DRIVER, KDRV_ARM_PSCI, &psci_driver,
                    sizeof(psci_driver));
    zircon_append_boot_item(zbi, ZBI_TYPE_KERNEL_DRIVER, KDRV_ARM_GENERIC_TIMER, &timer_driver,
                    sizeof(timer_driver));

    zircon_append_boot_item(zbi, ZBI_TYPE_CMDLINE, 0, BOOTLOADER_VERSION, strlen(BOOTLOADER_VERSION) + 1);

    // add platform ID
    zircon_append_boot_item(zbi, ZBI_TYPE_PLATFORM_ID, 0, &platform_id, sizeof(platform_id));

    add_partition_map(zbi);

    return 0;
}
