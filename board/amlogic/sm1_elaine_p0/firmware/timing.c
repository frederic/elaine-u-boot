
/*
 * board/amlogic/sm1_elaine_p0/firmware/timing.c
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <asm/arch/secure_apb.h>
#include <asm/arch/timing.h>
#include <asm/arch/ddr_define.h>

/* board clk defines */
#define DDR_SIZE				0
#define CPU_CLK					1200

/* ddr config support multiple configs for boards which use same bootloader:
 * config steps:
 * 1. add a new data struct in __ddr_setting[]
 * 2. config correct board_id, ddr_type, freq, etc..
 */


/* CAUTION!! */
/* Confirm ddr configs with hardware designer,
 * if you don't know how to config, then don't edit it
 */

/* Key configs */
/*
 * board_id: check hardware adc config
 * dram_rank_config:
 *            #define CONFIG_DDR_CHL_AUTO					0xF
 *            #define CONFIG_DDR0_16BIT_CH0				0x1
 *            #define CONFIG_DDR0_16BIT_RANK01_CH0		0x4
 *            #define CONFIG_DDR0_32BIT_RANK0_CH0			0x2
 *            #define CONFIG_DDR0_32BIT_RANK01_CH01		0x3
 *            #define CONFIG_DDR0_32BIT_16BIT_RANK0_CH0	0x5
 *            #define CONFIG_DDR0_32BIT_16BIT_RANK01_CH0	0x6
 * DramType:
 *            #define CONFIG_DDR_TYPE_DDR3				0
 *            #define CONFIG_DDR_TYPE_DDR4				1
 *            #define CONFIG_DDR_TYPE_LPDDR4				2
 *            #define CONFIG_DDR_TYPE_LPDDR3				3
 * DRAMFreq:
 *            {pstate0, pstate1, pstate2, pstate3} //more than one pstate means use dynamic freq
 *
 */


/* ddr configs */
#define DDR_RFC_TYPE_DDR3_512Mbx1				0
#define DDR_RFC_TYPE_DDR3_512Mbx2				1
#define DDR_RFC_TYPE_DDR3_512Mbx4				2
#define DDR_RFC_TYPE_DDR3_512Mbx8				3
#define DDR_RFC_TYPE_DDR3_512Mbx16				4
#define DDR_RFC_TYPE_DDR4_2Gbx1					5
#define DDR_RFC_TYPE_DDR4_2Gbx2					6
#define DDR_RFC_TYPE_DDR4_2Gbx4					7
#define DDR_RFC_TYPE_DDR4_2Gbx8					8

#define DDR_RFC_TYPE_LPDDR4_2Gbx1				9
#define DDR_RFC_TYPE_LPDDR4_3Gbx1				10
#define DDR_RFC_TYPE_LPDDR4_4Gbx1				11

#if CONFIG_DDR_LOGS_ENABLED
#define LOG_FLAG 0x9
#else
#define LOG_FLAG 0xA
#endif

ddr_set_t __ddr_setting[] = {
	{
	/* g12a skt (u209) ddr3 */
	.board_id				= CONFIG_BOARD_ID_MASK,
	.version				= 1,
	.dram_rank_config		= CONFIG_DDR0_32BIT_RANK01_CH0,
	.DramType				= CONFIG_DDR_TYPE_DDR3,
	.DRAMFreq				= {912, 0, 0, 0},
	.ddr_base_addr			= CFG_DDR_BASE_ADDR,
	.ddr_start_offset		= CFG_DDR_START_OFFSET,
	.imem_load_addr			= 0xFFFC0000, //sram
	.dmem_load_size			= 0x1000, //4K

	.DisabledDbyte			= 0xf0,
	.Is2Ttiming				= 1,
	.HdtCtrl				= 0xa,
	.dram_cs0_size_MB		= 1024,
	.dram_cs1_size_MB		= 512,
	.training_SequenceCtrl	= {0x31f,0}, //ddr3 0x21f 0x31f
	.phy_odt_config_rank	= {0x23,0x13}, //use 0x23 0x13  compatibility with 1rank and 2rank //targeting rank 0. [3:0] is used //for write ODT [7:4] is used for //read ODT
	.dfi_odt_config			= 0x0d0d,  //use 0d0d compatibility with 1rank and 2rank  //0808
	.PllBypassEn			= 0, //bit0-ps0,bit1-ps1
	.ddr_rdbi_wr_enable		= 0,
	.clk_drv_ohm			= 40,
	.cs_drv_ohm				= 40,
	.ac_drv_ohm				= 40,
	.soc_data_drv_ohm_p		= 34,
	.soc_data_drv_ohm_n		= 34,
	.soc_data_odt_ohm_p		= 60, //48,
	.soc_data_odt_ohm_n		= 0,
	.dram_data_drv_ohm		= 34, //ddr4 sdram only 34 or 48, skt board use 34 better
	.dram_data_odt_ohm		= 60,
	.dram_ac_odt_ohm		= 0,
	.soc_clk_slew_rate		= 0x300,
	.soc_cs_slew_rate		= 0x300,
	.soc_ac_slew_rate		= 0x300,
	.soc_data_slew_rate		= 0x200,
	.vref_output_permil		= 500,
	.vref_receiver_permil	= 500, //700,
	.vref_dram_permil		= 500, //700,
//	.vref_reverse			= 0,
	.ac_trace_delay			={32,32,32,32,32,32,32,32,32,32},
	//{00,00},
	.ac_pinmux				= {00,00},
#if 1
	.ddr_dmc_remap			= {
							[0] = ( 5 |  7 << 5 |  8 << 10 |  9 << 15 | 10 << 20 | 11 << 25 ),
							[1] = ( 12|  0 << 5 |  0 << 10 | 14 << 15 | 15 << 20 | 16 << 25 ),
							[2] = ( 17| 18 << 5 | 19 << 10 | 21 << 15 | 22 << 20 | 23 << 25 ),
							[3] = ( 24| 25 << 5 | 26 << 10 | 27 << 15 | 28 << 20 | 29 << 25 ),
							[4] = ( 30| 13 << 5 | 20 << 10 |  6 << 15 |  0 << 20 |  0 << 25 ),
	},
#else
	//16bit
	.ddr_dmc_remap			= {
							[0] = ( 0 |  5 << 5 |  6<< 10 |  7 << 15 | 8 << 20 | 9 << 25 ),
							[1] = ( 10|  0 << 5 |  0 << 10 | 14 << 15 | 15 << 20 | 16 << 25 ),
							[2] = ( 17|( 18 << 5) |( 19 << 10) |( 20 << 15) |( 21 << 20) | (22 << 25 )),
							[3] = ( 23| 24 << 5 | 25 << 10 | 26 << 15 | 27 << 20 | 28 << 25 ),
							[4] = ( 29| 11<< 5 | 12 << 10 |  13<< 15 |  0 << 20 |  0 << 25 ),
	},
#endif
	.ddr_lpddr34_ca_remap	= {00,00},
	.ddr_lpddr34_dq_remap	= {00,00},
	.dram_rtt_nom_wr_park	= {00,00},

	/* pll ssc config:
	 *
	 *   pll_ssc_mode = (1<<20) | (1<<8) | ([strength] << 4) | [mode],
	 *      ppm = strength * 500
	 *      mode: 0=center, 1=up, 2=down
	 *
	 *   eg:
	 *     1. config 1000ppm center ss. then mode=0, strength=2
	 *        .pll_ssc_mode = (1<<20) | (1<<8) | (2 << 4) | 0,
	 *     2. config 3000ppm down ss. then mode=2, strength=6
	 *        .pll_ssc_mode = (1<<20) | (1<<8) | (6 << 4) | 2,
	 */
	.pll_ssc_mode			= (1<<20) | (1<<8) | (2<<4) | 0,//center_ssc_1000ppm
	.ddr_func				= DDR_FUNC,
	.magic					= DRAM_CFG_MAGIC,
},
{
	/* g12a skt (u209) ddr3 */
	.board_id				= CONFIG_BOARD_ID_MASK,
	.version				= 1,
	.dram_rank_config		= CONFIG_DDR0_32BIT_RANK01_CH0,
	.DramType				= CONFIG_DDR_TYPE_DDR3,
	.DRAMFreq				= {912, 0, 0, 0},
	.ddr_base_addr			= CFG_DDR_BASE_ADDR,
	.ddr_start_offset		= CFG_DDR_START_OFFSET,
	.imem_load_addr			= 0xFFFC0000, //sram
	.dmem_load_size			= 0x1000, //4K

	.DisabledDbyte			= 0xf0,
	.Is2Ttiming				= 1,
	.HdtCtrl				= LOG_FLAG,
	.dram_cs0_size_MB		= 1024,
	.dram_cs1_size_MB		= 512,
	.training_SequenceCtrl	= {0x31f,0}, //ddr3 0x21f 0x31f
	.phy_odt_config_rank	= {0x23,0x13}, //use 0x23 0x13  compatibility with 1rank and 2rank //targeting rank 0. [3:0] is used //for write ODT [7:4] is used for //read ODT
	.dfi_odt_config			= 0x0d0d,  //use 0d0d compatibility with 1rank and 2rank  //0808
	.PllBypassEn			= 0, //bit0-ps0,bit1-ps1
	.ddr_rdbi_wr_enable		= 0,
	.pll_ssc_mode			= 0,
	.clk_drv_ohm			= 40,
	.cs_drv_ohm				= 40,
	.ac_drv_ohm				= 40,
	.soc_data_drv_ohm_p		= 34,
	.soc_data_drv_ohm_n		= 34,
	.soc_data_odt_ohm_p		= 60, //48,
	.soc_data_odt_ohm_n		= 0,
	.dram_data_drv_ohm		= 34, //ddr4 sdram only 34 or 48, skt board use 34 better
	.dram_data_odt_ohm		= 60,
	.dram_ac_odt_ohm		= 0,
	.soc_clk_slew_rate		= 0x300,
	.soc_cs_slew_rate		= 0x300,
	.soc_ac_slew_rate		= 0x300,
	.soc_data_slew_rate		= 0x200,
	.vref_output_permil		= 500,
	.vref_receiver_permil	= 500, //700,
	.vref_dram_permil		= 500, //700,
	//{00,00},
	.ac_pinmux				= {00,00},
#if 1
	.ddr_dmc_remap			= {
							[0] = ( 5 |  7 << 5 |  8 << 10 |  9 << 15 | 10 << 20 | 11 << 25 ),
							[1] = ( 12|  0 << 5 |  0 << 10 | 14 << 15 | 15 << 20 | 16 << 25 ),
							[2] = ( 17| 18 << 5 | 19 << 10 | 21 << 15 | 22 << 20 | 23 << 25 ),
							[3] = ( 24| 25 << 5 | 26 << 10 | 27 << 15 | 28 << 20 | 29 << 25 ),
							[4] = ( 30| 13 << 5 | 20 << 10 |  6 << 15 |  0 << 20 |  0 << 25 ),
	},
#else
	//16bit
	.ddr_dmc_remap			= {
							[0] = ( 0 |  5 << 5 |  6<< 10 |  7 << 15 | 8 << 20 | 9 << 25 ),
							[1] = ( 10|  0 << 5 |  0 << 10 | 14 << 15 | 15 << 20 | 16 << 25 ),
							[2] = ( 17|( 18 << 5) |( 19 << 10) |( 20< 15) |( 21 << 20) | (22 << 25 )),
							[3] = ( 23| 24 << 5 | 25 << 10 | 26 << 15 | 27 << 20 | 28 << 25 ),
							[4] = ( 29| 11<< 5 | 12 << 10 |  13<< 15 |  0 << 20 |  0 << 25 ),
	},
#endif
	.ddr_lpddr34_ca_remap	= {00,00},
	.ddr_lpddr34_dq_remap	= {00,00},
	.dram_rtt_nom_wr_park	= {00,00},

	/* pll ssc config:
	 *
	 *   pll_ssc_mode = (1<<20) | (1<<8) | ([strength] << 4) | [mode],
	 *      ppm = strength * 500
	 *      mode: 0=center, 1=up, 2=down
	 *
	 *   eg:
	 *     1. config 1000ppm center ss. then mode=0, strength=2
	 *        .pll_ssc_mode = (1<<20) | (1<<8) | (2 << 4) | 0,
	 *     2. config 3000ppm down ss. then mode=2, strength=6
	 *        .pll_ssc_mode = (1<<20) | (1<<8) | (6 << 4) | 2,
	 */
	.pll_ssc_mode			= (1<<20) | (1<<8) | (2<<4) | 0,//center_ssc_1000ppm
	.ddr_func				= DDR_FUNC,
	.magic					= DRAM_CFG_MAGIC,
	.slt_test_function		= {DMC_TEST_SLT_ENABLE_DDR_SKIP_TRAINING, 0},
	.dfi_hwtmrl	= 4,

	.ac_trace_delay	= {
		32, 32, 32, 32,
		32,	32, 32, 32,
		32, 32,
	},

	.write_dqs_delay = {
		163, 163, 163, 163,
		167, 167, 165, 165,
		159, 159, 163, 163,
		165, 165, 161, 161,
	},

	.read_dqs_delay	= {
		13, 13, 14, 14,
		14, 14, 15, 14,
		14, 14, 14, 15,
		14, 13, 14, 14,
	},

	.write_dq_bit_delay	= {
		20, 21, 19, 21,
		21, 21, 22, 20,
		22, 21, 20, 20,
		19, 21, 18, 21,
		20, 20, 24, 20,
		22, 25, 24, 25,
		26, 26, 23, 20,
		21, 20, 23, 21,
		21, 23, 23, 20,
		48, 49, 48, 50,
		50, 50, 51, 49,
		49, 21, 21, 19,
		20, 21, 19, 20,
		21, 20, 22, 18,
		20, 23, 23, 24,
		24, 24, 22,	17,
		18, 17,	20,	18,
		18,	21,	20,	19,
	},

	.read_dq_bit_delay = {
		15, 15,	15,	23,
		31,	31,	31,	29,
		0 ,	23,	19,	11,
		17,	35,	23,	31,
		31,	0 ,	15,	3 ,
		9 ,	15,	27,	27,
		35,	35,	0 ,	13,
		15,	11,	19,	25,
		19,	31,	31,	0 ,
		15,	17,	15,	19,
		31,	33,	31,	27,
		0 ,	19,	19,	15,
		13,	31,	23,	27,
		31,	0 ,	13,	3 ,
		5 ,	11,	23,	27,
		31,	27,	0 ,	13,
		13,	9 ,	19,	23,
		23,	27,	27,	0 ,
	},

	.read_dqs_gate_delay = {
		225, 227, 223, 225,
		223, 225, 223, 227,
		223, 225, 223, 225,
		225, 227, 227, 229,
	},

},
};

pll_set_t __pll_setting = {
	.cpu_clk				= CPU_CLK / 24 * 24,
#ifdef CONFIG_PXP_EMULATOR
	.pxp					= 1,
#else
	.pxp					= 0,
#endif
	.spi_ctrl				= 0,
	.lCustomerID			= AML_CUSTOMER_ID,
#ifdef CONFIG_DEBUG_MODE
	.debug_mode				= CONFIG_DEBUG_MODE,
	.ddr_clk_debug			= CONFIG_DDR_CLK_DEBUG,
	.cpu_clk_debug			= CONFIG_CPU_CLK_DEBUG,
#endif
};

ddr_reg_t __ddr_reg[] = {
	/* demo, user defined override register */
	{0xaabbccdd, 0, 0, 0, 0, 0},
	{0x11223344, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
};

#define VCCK_VAL				AML_VCCK_INIT_VOLTAGE
#define VDDEE_VAL				AML_VDDEE_INIT_VOLTAGE
/* VCCK PWM table */
#if   (VCCK_VAL == 800)
	#define VCCK_VAL_REG	0x00150007
#elif (VCCK_VAL == 810)
	#define VCCK_VAL_REG	0x00140008
#elif (VCCK_VAL == 820)
	#define VCCK_VAL_REG	0x00130009
#elif (VCCK_VAL == 830)
	#define VCCK_VAL_REG	0x0012000a
#elif (VCCK_VAL == 840)
	#define VCCK_VAL_REG	0x0011000b
#elif (VCCK_VAL == 850)
	#define VCCK_VAL_REG	0x0010000c
#elif (VCCK_VAL == 860)
	#define VCCK_VAL_REG	0x000f000d
#elif (VCCK_VAL == 870)
	#define VCCK_VAL_REG	0x000e000e
#elif (VCCK_VAL == 880)
	#define VCCK_VAL_REG	0x000d000f
#elif (VCCK_VAL == 890)
	#define VCCK_VAL_REG	0x000c0010
#elif (VCCK_VAL == 900)
	#define VCCK_VAL_REG	0x000b0011
#elif (VCCK_VAL == 910)
	#define VCCK_VAL_REG	0x000a0012
#elif (VCCK_VAL == 920)
	#define VCCK_VAL_REG	0x00090013
#elif (VCCK_VAL == 930)
	#define VCCK_VAL_REG	0x00080014
#elif (VCCK_VAL == 940)
	#define VCCK_VAL_REG	0x00070015
#elif (VCCK_VAL == 950)
	#define VCCK_VAL_REG	0x00060016
#elif (VCCK_VAL == 960)
	#define VCCK_VAL_REG	0x00050017
#elif (VCCK_VAL == 970)
	#define VCCK_VAL_REG	0x00040018
#elif (VCCK_VAL == 980)
	#define VCCK_VAL_REG	0x00030019
#elif (VCCK_VAL == 990)
	#define VCCK_VAL_REG	0x0002001a
#elif (VCCK_VAL == 1000)
	#define VCCK_VAL_REG	0x0001001b
#elif (VCCK_VAL == 1010)
	#define VCCK_VAL_REG	0x0000001c
#else
	#error "VCCK val out of range\n"
#endif

/* VDDEE PWM table */
#if    (VDDEE_VAL == 800)
	#define VDDEE_VAL_REG	0x0010000c
#elif (VDDEE_VAL == 810)
	#define VDDEE_VAL_REG	0x000f000d
#elif (VDDEE_VAL == 820)
	#define VDDEE_VAL_REG	0x000e000e
#elif (VDDEE_VAL == 830)
	#define VDDEE_VAL_REG	0x000d000f
#elif (VDDEE_VAL == 840)
	#define VDDEE_VAL_REG	0x000c0010
#elif (VDDEE_VAL == 850)
	#define VDDEE_VAL_REG	0x000b0011
#elif (VDDEE_VAL == 860)
	#define VDDEE_VAL_REG	0x000a0012
#elif (VDDEE_VAL == 870)
	#define VDDEE_VAL_REG	0x00090013
#elif (VDDEE_VAL == 880)
	#define VDDEE_VAL_REG	0x00080014
#elif (VDDEE_VAL == 890)
	#define VDDEE_VAL_REG	0x00070015
#elif (VDDEE_VAL == 900)
	#define VDDEE_VAL_REG	0x00060016
#elif (VDDEE_VAL == 910)
	#define VDDEE_VAL_REG	0x00050017
#elif (VDDEE_VAL == 920)
	#define VDDEE_VAL_REG	0x00040018
#elif (VDDEE_VAL == 930)
	#define VDDEE_VAL_REG	0x00030019
#elif (VDDEE_VAL == 940)
	#define VDDEE_VAL_REG	0x0002001a
#elif (VDDEE_VAL == 950)
	#define VDDEE_VAL_REG	0x0001001b
#elif (VDDEE_VAL == 960)
	#define VDDEE_VAL_REG	0x0000001c
#else
	#error "VDDEE val out of range\n"
#endif

/* for PWM use */
/* PWM driver check http://scgit.amlogic.com:8080/#/c/38093/ */
#define GPIO_O_EN_N_REG3	((0xff634400 + (0x19 << 2)))
#define GPIO_O_REG3		((0xff634400 + (0x1a << 2)))
#define GPIO_I_REG3		((0xff634400 + (0x1b << 2)))
#define AO_PIN_MUX_REG0	((0xff800000 + (0x05 << 2)))
#define AO_PIN_MUX_REG1	((0xff800000 + (0x06 << 2)))

bl2_reg_t __bl2_reg[] = {
	/* demo, user defined override register */
	/* eg: PWM init */

	/* PWM_AO_D */
	/* VCCK_VAL_REG: check PWM table */
	{AO_PWM_PWM_D,        VCCK_VAL_REG,            0xffffffff,   0, BL2_INIT_STAGE_1, 0},
	{AO_PWM_MISC_REG_CD,  ((1 << 23) | (1 << 1)),  (0x7f << 16), 0, BL2_INIT_STAGE_1, 0},
	{AO_PIN_MUX_REG1,     (3 << 20),               (0xF << 20),  0, BL2_INIT_STAGE_1, 0},
	/* PWM_AO_B */
	/* VDDEE_VAL_REG: check PWM table */
	{AO_PWM_PWM_B,        VDDEE_VAL_REG,           0xffffffff, 0, BL2_INIT_STAGE_1, 0},
	{AO_PWM_MISC_REG_AB,  ((1 << 23) | (1 << 1)),  (0x7f << 16), 0, BL2_INIT_STAGE_1, 0},
	{AO_PIN_MUX_REG1,     (3 << 16),               (0xF << 16),  0, BL2_INIT_STAGE_1, 0},
	/* Enable 5V_EN */
	{GPIO_O_EN_N_REG3,    (0 << 8),                (1 << 8),     0, BL2_INIT_STAGE_1, 0},
	{GPIO_O_REG3,         (1 << 8),                0xffffffff,   0, BL2_INIT_STAGE_1, 0},
	/* Enable VCCK */
	{AO_SEC_REG0,         (1 << 0),                0xffffffff,   0, BL2_INIT_STAGE_1, 0},
	{AO_GPIO_O,           0x80000000,               0xffffffff,   0, BL2_INIT_STAGE_1, 0},
};
