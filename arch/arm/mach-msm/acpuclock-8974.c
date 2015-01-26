/*
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <mach/rpm-regulator-smd.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_bus.h>
#include <mach/socinfo.h>

#include "acpuclock.h"
#include "acpuclock-krait.h"

/* Corner type vreg VDD values */
#define LVL_NONE	RPM_REGULATOR_CORNER_NONE
#define LVL_LOW		RPM_REGULATOR_CORNER_SVS_SOC
#define LVL_NOM		RPM_REGULATOR_CORNER_NORMAL
#define LVL_HIGH	RPM_REGULATOR_CORNER_SUPER_TURBO

static struct hfpll_data hfpll_data __initdata = {
	.mode_offset = 0x00,
	.l_offset = 0x04,
	.m_offset = 0x08,
	.n_offset = 0x0C,
	.has_user_reg = true,
	.user_offset = 0x10,
	.config_offset = 0x14,
	.user_val = 0x8,
	.user_vco_mask = BIT(20),
	.config_val = 0x04D0405D,
	.has_lock_status = true,
	.status_offset = 0x1C,
	.low_vco_l_max = 65,
	.low_vdd_l_max = 52,
	.nom_vdd_l_max = 104,
	.vdd[HFPLL_VDD_NONE] = LVL_NONE,
	.vdd[HFPLL_VDD_LOW]  = LVL_LOW,
	.vdd[HFPLL_VDD_NOM]  = LVL_NOM,
	.vdd[HFPLL_VDD_HIGH] = LVL_HIGH,
};

static struct scalable scalable[] __initdata = {
	[CPU0] = {
		.hfpll_phys_base = 0xF908A000,
		.l2cpmr_iaddr = 0x4501,
		.sec_clk_sel = 2,
		.vreg[VREG_CORE] = { "krait0",     1100000 },
		.vreg[VREG_MEM]  = { "krait0_mem", 1050000 },
		.vreg[VREG_DIG]  = { "krait0_dig", LVL_HIGH },
		.vreg[VREG_HFPLL_A] = { "krait0_hfpll", 1800000 },
	},
	[CPU1] = {
		.hfpll_phys_base = 0xF909A000,
		.l2cpmr_iaddr = 0x5501,
		.sec_clk_sel = 2,
		.vreg[VREG_CORE] = { "krait1",     1100000 },
		.vreg[VREG_MEM]  = { "krait1_mem", 1050000 },
		.vreg[VREG_DIG]  = { "krait1_dig", LVL_HIGH },
		.vreg[VREG_HFPLL_A] = { "krait1_hfpll", 1800000 },
	},
	[CPU2] = {
		.hfpll_phys_base = 0xF90AA000,
		.l2cpmr_iaddr = 0x6501,
		.sec_clk_sel = 2,
		.vreg[VREG_CORE] = { "krait2",     1100000 },
		.vreg[VREG_MEM]  = { "krait2_mem", 1050000 },
		.vreg[VREG_DIG]  = { "krait2_dig", LVL_HIGH },
		.vreg[VREG_HFPLL_A] = { "krait2_hfpll", 1800000 },
	},
	[CPU3] = {
		.hfpll_phys_base = 0xF90BA000,
		.l2cpmr_iaddr = 0x7501,
		.sec_clk_sel = 2,
		.vreg[VREG_CORE] = { "krait3",     1100000 },
		.vreg[VREG_MEM]  = { "krait3_mem", 1050000 },
		.vreg[VREG_DIG]  = { "krait3_dig", LVL_HIGH },
		.vreg[VREG_HFPLL_A] = { "krait3_hfpll", 1800000 },
	},
	[L2] = {
		.hfpll_phys_base = 0xF9016000,
		.l2cpmr_iaddr = 0x0500,
		.sec_clk_sel = 2,
		.vreg[VREG_HFPLL_A] = { "l2_hfpll", 1800000 },
	},
};

static struct msm_bus_paths bw_level_tbl_v1[] __initdata = {
	[0] =  BW_MBPS(600), /* At least  75 MHz on bus. */
	[1] = BW_MBPS(1600), /* At least 200 MHz on bus. */
	[2] = BW_MBPS(3200), /* At least 400 MHz on bus. */
	[3] = BW_MBPS(6400), /* At least 800 MHz on bus. */
};

static struct l2_level l2_freq_tbl_v1[] __initdata = {
	[0]  = { {  300000, PLL_0, 0,   0 }, LVL_LOW,   950000, 0 },
	[1]  = { {  652800, HFPLL, 1,  34 }, LVL_NOM,   950000, 1 },
	[2] = { { 1036800, HFPLL, 1,  54 }, LVL_HIGH, 1050000, 2 },
	[3] = { { 1497600, HFPLL, 1,  78 }, LVL_HIGH, 1050000, 3 },
	{ }
};

static struct acpu_level acpu_freq_tbl_v1_pvs0[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 }, L2(0),   825000,  73 },
	{ 1, {  652800, HFPLL, 1,  34 }, L2(1),   825000, 165 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  880000, 275 },
	{ 0, { 1497600, HFPLL, 1,  78 }, L2(3),  995000, 423 },
	{ 1, { 1728000, HFPLL, 1,  90 }, L2(3), 1050000, 506 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_v1_pvs1[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 }, L2(0),   825000,  73 },
	{ 1, {  652800, HFPLL, 1,  34 }, L2(1),   825000, 165 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  880000, 275 },
	{ 0, { 1497600, HFPLL, 1,  78 }, L2(3),  995000, 423 },
	{ 1, { 1728000, HFPLL, 1,  90 }, L2(3), 1050000, 506 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_v1_pvs2[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 }, L2(0),   825000,  73 },
	{ 1, {  652800, HFPLL, 1,  34 }, L2(1),   825000, 165 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  855000, 275 },
	{ 0, { 1497600, HFPLL, 1,  78 }, L2(3),  960000, 423 },
	{ 1, { 1728000, HFPLL, 1,  90 }, L2(3), 1000000, 506 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_v1_pvs3[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 }, L2(0),   825000,  73 },
	{ 1, {  652800, HFPLL, 1,  34 }, L2(1),   825000, 165 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  855000, 275 },
	{ 0, { 1497600, HFPLL, 1,  78 }, L2(3),  960000, 423 },
	{ 1, { 1728000, HFPLL, 1,  90 }, L2(3), 1000000, 506 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_v1_pvs4[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 }, L2(0),  825000,  73 },
	{ 1, {  652800, HFPLL, 1,  34 }, L2(1),  825000, 165 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2), 825000, 275 },
	{ 0, { 1497600, HFPLL, 1,  78 }, L2(3), 910000, 423 },
	{ 1, { 1728000, HFPLL, 1,  90 }, L2(3), 950000, 506 },
	{ 0, { 0 } }
};

static struct msm_bus_paths bw_level_tbl_v2[] __initdata = {
	[0] =  BW_MBPS(600), /* At least  75 MHz on bus. */
	[1] = BW_MBPS(1600), /* At least 200 MHz on bus. */
	[2] = BW_MBPS(3680), /* At least 460 MHz on bus. */
	[3] = BW_MBPS(8000), /* At least 1000 MHz on bus. */
};

static struct l2_level l2_freq_tbl_v2[] __initdata = {
	[0]  = { {  300000, PLL_0, 0,   0 }, LVL_LOW,   950000, 0 },
	[1]  = { {  652800, HFPLL, 1,  34 }, LVL_NOM,   950000, 1 },
	[2] = { { 1036800, HFPLL, 1,  54 }, LVL_NOM,   950000, 2 },
	[3] = { { 1497600, HFPLL, 1,  78 }, LVL_HIGH, 1050000, 3 },
	[4] = { { 1728000, HFPLL, 1,  90 }, LVL_HIGH, 1050000, 3 },
	{ }
};

static struct acpu_level acpu_freq_tbl_2g_pvs0[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  815000,  73 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  865000, 165 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  925000, 275 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3), 1010000, 423 },
	{ 1, { 1958400, HFPLL, 1, 102 }, L2(3), 1100000, 598 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2g_pvs1[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  800000,  73 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  850000, 165 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  910000, 275 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  990000, 423 },
	{ 1, { 1958400, HFPLL, 1, 102 }, L2(3), 1075000, 598 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2g_pvs2[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  785000,  73 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  835000, 165 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  890000, 275 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  970000, 423 },
	{ 1, { 1958400, HFPLL, 1, 102 }, L2(3), 1050000, 598 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2g_pvs3[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  775000,  73 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  820000, 165 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  875000, 275 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  950000, 423 },
	{ 1, { 1958400, HFPLL, 1, 102 }, L2(3), 1025000, 598 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2g_pvs4[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  775000,  73 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  810000, 165 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  860000, 275 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  930000, 423 },
	{ 1, { 1958400, HFPLL, 1, 102 }, L2(3), 1000000, 598 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2g_pvs5[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  750000,  73 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  800000, 165 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  850000, 275 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  910000, 423 },
	{ 1, { 1958400, HFPLL, 1, 102 }, L2(3),  975000, 598 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2g_pvs6[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  750000,  73 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  790000, 165 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  840000, 275 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  895000, 423 },
	{ 1, { 1958400, HFPLL, 1, 102 }, L2(3),  950000, 598 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2p2g_pvs0[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  800000,  72 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  835000, 161 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  890000, 267 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  965000, 409 },
	{ 1, { 2150400, HFPLL, 1, 112 }, L2(3), 1100000, 656 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2p2g_pvs1[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  800000,  72 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  820000, 161 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  875000, 267 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  945000, 409 },
	{ 1, { 2150400, HFPLL, 1, 112 }, L2(3), 1075000, 656 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2p2g_pvs2[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  775000,  72 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  805000, 161 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  855000, 267 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  925000, 409 },
	{ 1, { 2150400, HFPLL, 1, 112 }, L2(3), 1050000, 656 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2p2g_pvs3[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  775000,  72 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  790000, 161 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  840000, 267 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  910000, 409 },
	{ 1, { 2150400, HFPLL, 1, 112 }, L2(3), 1025000, 656 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2p2g_pvs4[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  775000,  72 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  780000, 161 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  830000, 267 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  895000, 409 },
	{ 1, { 2150400, HFPLL, 1, 112 }, L2(3), 1000000, 656 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2p2g_pvs5[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  750000,  72 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  770000, 161 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  820000, 267 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  880000, 409 },
	{ 1, { 2150400, HFPLL, 1, 112 }, L2(3),  975000, 656 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2p2g_pvs6[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  750000,  72 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  760000, 161 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  810000, 267 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  870000, 409 },
	{ 1, { 2150400, HFPLL, 1, 112 }, L2(3),  950000, 656 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2p3g_pvs0[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  800000,  72 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  825000, 159 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  875000, 264 },
	{ 1, { 1574400, HFPLL, 1,  82 }, L2(3),  965000, 430 },
	{ 1, { 2265600, HFPLL, 1, 118 }, L2(3), 1100000, 691 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2p3g_pvs1[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  800000,  72 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  810000, 159 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  860000, 264 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  930000, 404 },
	{ 1, { 2265600, HFPLL, 1, 118 }, L2(3), 1075000, 691 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2p3g_pvs2[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  775000,  72 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  795000, 159 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  845000, 264 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  910000, 404 },
	{ 1, { 2265600, HFPLL, 1, 118 }, L2(3), 1050000, 691 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2p3g_pvs3[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  775000,  72 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  780000, 159 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  830000, 264 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  895000, 404 },
	{ 1, { 2265600, HFPLL, 1, 118 }, L2(3), 1025000, 691 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2p3g_pvs4[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  775000,  72 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  775000, 159 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  820000, 264 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  880000, 404 },
	{ 1, { 2265600, HFPLL, 1, 118 }, L2(3), 1000000, 691 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2p3g_pvs5[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  750000,  72 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  760000, 159 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  810000, 264 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  870000, 404 },
	{ 1, { 2265600, HFPLL, 1, 118 }, L2(3),  975000, 691 },
	{ 0, { 0 } }
};

static struct acpu_level acpu_freq_tbl_2p3g_pvs6[] __initdata = {
	{ 1, {  300000, PLL_0, 0,   0 },  L2(0),  750000,  72 },
	{ 1, {  652800, HFPLL, 1,  34 },  L2(1),  750000, 159 },
	{ 1, { 1036800, HFPLL, 1,  54 }, L2(2),  800000, 264 },
	{ 1, { 1497600, HFPLL, 1,  78 }, L2(3),  860000, 404 },
	{ 1, { 2265600, HFPLL, 1, 118 }, L2(3),  950000, 691 },
	{ 0, { 0 } }
};

static struct pvs_table pvs_v1[NUM_SPEED_BINS][NUM_PVS] __initdata = {
	/* 8974v1 1.7GHz Parts */
	[0][0] = { acpu_freq_tbl_v1_pvs0, sizeof(acpu_freq_tbl_v1_pvs0) },
	[0][1] = { acpu_freq_tbl_v1_pvs1, sizeof(acpu_freq_tbl_v1_pvs1) },
	[0][2] = { acpu_freq_tbl_v1_pvs2, sizeof(acpu_freq_tbl_v1_pvs2) },
	[0][3] = { acpu_freq_tbl_v1_pvs3, sizeof(acpu_freq_tbl_v1_pvs3) },
	[0][4] = { acpu_freq_tbl_v1_pvs4, sizeof(acpu_freq_tbl_v1_pvs4) },
};

static struct pvs_table pvs_v2[NUM_SPEED_BINS][NUM_PVS] __initdata = {
	/* 8974v2 2.0GHz Parts */
	[0][0] = { acpu_freq_tbl_2g_pvs0, sizeof(acpu_freq_tbl_2g_pvs0) },
	[0][1] = { acpu_freq_tbl_2g_pvs1, sizeof(acpu_freq_tbl_2g_pvs1) },
	[0][2] = { acpu_freq_tbl_2g_pvs2, sizeof(acpu_freq_tbl_2g_pvs2) },
	[0][3] = { acpu_freq_tbl_2g_pvs3, sizeof(acpu_freq_tbl_2g_pvs3) },
	[0][4] = { acpu_freq_tbl_2g_pvs4, sizeof(acpu_freq_tbl_2g_pvs4) },
	[0][5] = { acpu_freq_tbl_2g_pvs5, sizeof(acpu_freq_tbl_2g_pvs5) },
	[0][6] = { acpu_freq_tbl_2g_pvs6, sizeof(acpu_freq_tbl_2g_pvs6) },
	[0][7] = { acpu_freq_tbl_2g_pvs6, sizeof(acpu_freq_tbl_2g_pvs6) },

	/* 8974v2 2.3GHz Parts */
	[1][0] = { acpu_freq_tbl_2p3g_pvs0, sizeof(acpu_freq_tbl_2p3g_pvs0) },
	[1][1] = { acpu_freq_tbl_2p3g_pvs1, sizeof(acpu_freq_tbl_2p3g_pvs1) },
	[1][2] = { acpu_freq_tbl_2p3g_pvs2, sizeof(acpu_freq_tbl_2p3g_pvs2) },
	[1][3] = { acpu_freq_tbl_2p3g_pvs3, sizeof(acpu_freq_tbl_2p3g_pvs3) },
	[1][4] = { acpu_freq_tbl_2p3g_pvs4, sizeof(acpu_freq_tbl_2p3g_pvs4) },
	[1][5] = { acpu_freq_tbl_2p3g_pvs5, sizeof(acpu_freq_tbl_2p3g_pvs5) },
	[1][6] = { acpu_freq_tbl_2p3g_pvs6, sizeof(acpu_freq_tbl_2p3g_pvs6) },
	[1][7] = { acpu_freq_tbl_2p3g_pvs6, sizeof(acpu_freq_tbl_2p3g_pvs6) },

	/* 8974v2 2.0GHz Parts */
	[2][0] = { acpu_freq_tbl_2p2g_pvs0, sizeof(acpu_freq_tbl_2p2g_pvs0) },
	[2][1] = { acpu_freq_tbl_2p2g_pvs1, sizeof(acpu_freq_tbl_2p2g_pvs1) },
	[2][2] = { acpu_freq_tbl_2p2g_pvs2, sizeof(acpu_freq_tbl_2p2g_pvs2) },
	[2][3] = { acpu_freq_tbl_2p2g_pvs3, sizeof(acpu_freq_tbl_2p2g_pvs3) },
	[2][4] = { acpu_freq_tbl_2p2g_pvs4, sizeof(acpu_freq_tbl_2p2g_pvs4) },
	[2][5] = { acpu_freq_tbl_2p2g_pvs5, sizeof(acpu_freq_tbl_2p2g_pvs5) },
	[2][6] = { acpu_freq_tbl_2p2g_pvs6, sizeof(acpu_freq_tbl_2p2g_pvs6) },
	[2][7] = { acpu_freq_tbl_2p2g_pvs6, sizeof(acpu_freq_tbl_2p2g_pvs6) },

};

static struct msm_bus_scale_pdata bus_scale_data __initdata = {
	.usecase = bw_level_tbl_v2,
	.num_usecases = ARRAY_SIZE(bw_level_tbl_v2),
	.active_only = 1,
	.name = "acpuclk-8974",
};

static struct acpuclk_krait_params acpuclk_8974_params __initdata = {
	.scalable = scalable,
	.scalable_size = sizeof(scalable),
	.hfpll_data = &hfpll_data,
	.pvs_tables = pvs_v2,
	.l2_freq_tbl = l2_freq_tbl_v2,
	.l2_freq_tbl_size = sizeof(l2_freq_tbl_v2),
	.bus_scale = &bus_scale_data,
	.pte_efuse_phys = 0xFC4B80B0,
	.get_bin_info = get_krait_bin_format_b,
	.stby_khz = 300000,
};

static void __init apply_v1_l2_workaround(void)
{
	static struct l2_level resticted_l2_tbl[] __initdata = {
		[0] = { {  300000, PLL_0, 0,   0 }, LVL_LOW,  1050000, 0 },
		[1] = { { 1497600, HFPLL, 1,  78 }, LVL_HIGH, 1050000, 7 },
		{ }
	};
	struct acpu_level *l;
	int s, p;

	for (s = 0; s < NUM_SPEED_BINS; s++)
		for (p = 0; p < NUM_PVS; p++)
			for (l = pvs_v1[s][p].table; l && l->speed.khz; l++)
				l->l2_level = l->l2_level > 5 ? 1 : 0;

	acpuclk_8974_params.l2_freq_tbl = resticted_l2_tbl;
	acpuclk_8974_params.l2_freq_tbl_size = sizeof(resticted_l2_tbl);
}

static int __init acpuclk_8974_probe(struct platform_device *pdev)
{
	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 1) {
		acpuclk_8974_params.pvs_tables = pvs_v1;
		acpuclk_8974_params.l2_freq_tbl = l2_freq_tbl_v1;
		bus_scale_data.usecase = bw_level_tbl_v1;
		bus_scale_data.num_usecases = ARRAY_SIZE(bw_level_tbl_v1);
		acpuclk_8974_params.l2_freq_tbl_size = sizeof(l2_freq_tbl_v1);

		/*
		 * 8974 hardware revisions older than v1.2 may experience L2
		 * parity errors when running at some performance points between
		 * 300MHz and 1497.6MHz (non-inclusive), or when vdd_mx is less
		 * than 1.05V. Restrict L2 operation to safe performance points
		 * on these devices.
		 */
		if (SOCINFO_VERSION_MINOR(socinfo_get_version()) < 2)
			apply_v1_l2_workaround();
	}

	return acpuclk_krait_init(&pdev->dev, &acpuclk_8974_params);
}

static struct of_device_id acpuclk_8974_match_table[] = {
	{ .compatible = "qcom,acpuclk-8974" },
	{}
};

static struct platform_driver acpuclk_8974_driver = {
	.driver = {
		.name = "acpuclk-8974",
		.of_match_table = acpuclk_8974_match_table,
		.owner = THIS_MODULE,
	},
};

static int __init acpuclk_8974_init(void)
{
	return platform_driver_probe(&acpuclk_8974_driver,
				     acpuclk_8974_probe);
}
device_initcall(acpuclk_8974_init);
