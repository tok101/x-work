/*
 * include/linux/mfd/ricoh618.h
 *
 * Core driver interface to access RICOH R5T618 power management chip.
 *
 * Copyright (C) 2012-2014 RICOH COMPANY,LTD
 *
 * Based on code
 *	Copyright (C) 2011 NVIDIA Corporation
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
 *
 */

#ifndef __LINUX_MFD_RICOH618_H
#define __LINUX_MFD_RICOH618_H


/* Maximum number of main interrupts */
#define MAX_INTERRUPT_MASKS	13
#define MAX_MAIN_INTERRUPT	7
#define MAX_GPEDGE_REG		2

/* Power control register */
#define RICOH618_PWR_ON_HISORY		0x09
#define RICOH618_PWR_OFF_HISORY		0x0A
#define RICOH618_PWR_WD			0x0B
#define RICOH618_PWR_WD_COUNT		0x0C
#define RICOH618_PWR_FUNC		0x0D
#define RICOH618_PWR_SLP_CNT		0x0E
#define RICOH618_PWR_REP_CNT		0x0F
#define RICOH618_PWR_ON_TIMSET		0x10
#define RICOH618_PWR_NOE_TIMSET		0x11
#define RICOH618_PWR_IRSEL		0x15

/* Interrupt enable register */
#define RICOH618_INT_EN_SYS		0x12
#define RICOH618_INT_EN_DCDC		0x40
#define RICOH618_INT_EN_RTC		0xAE
#define RICOH618_INT_EN_ADC1		0x88
#define RICOH618_INT_EN_ADC2		0x89
#define RICOH618_INT_EN_ADC3		0x8A
#define RICOH618_INT_EN_GPIO		0x94
#define RICOH618_INT_EN_GPIO2		0x94 /* dummy */
#define RICOH618_INT_MSK_CHGCTR		0xBE
#define RICOH618_INT_MSK_CHGSTS1	0xBF
#define RICOH618_INT_MSK_CHGSTS2	0xC0
#define RICOH618_INT_MSK_CHGERR		0xC1
#define RICOH618_INT_MSK_CHGEXTIF	0xD1

/* Interrupt select register */
#define RICOH618_CHG_CTRL_DETMOD1	0xCA
#define RICOH618_CHG_CTRL_DETMOD2	0xCB
#define RICOH618_CHG_STAT_DETMOD1	0xCC
#define RICOH618_CHG_STAT_DETMOD2	0xCD
#define RICOH618_CHG_STAT_DETMOD3	0xCE


/* interrupt status registers (monitor regs)*/
#define RICOH618_INTC_INTPOL		0x9C
#define RICOH618_INTC_INTEN		0x9D
#define RICOH618_INTC_INTMON		0x9E

#define RICOH618_INT_MON_SYS		0x14
#define RICOH618_INT_MON_DCDC		0x42
#define RICOH618_INT_MON_RTC		0xAF

#define RICOH618_INT_MON_CHGCTR		0xC6
#define RICOH618_INT_MON_CHGSTS1	0xC7
#define RICOH618_INT_MON_CHGSTS2	0xC8
#define RICOH618_INT_MON_CHGERR		0xC9
#define RICOH618_INT_MON_CHGEXTIF	0xD3

/* interrupt clearing registers */
#define RICOH618_INT_IR_SYS		0x13
#define RICOH618_INT_IR_DCDC		0x41
#define RICOH618_INT_IR_RTC		0xAF
#define RICOH618_INT_IR_ADCL		0x8C
#define RICOH618_INT_IR_ADCH		0x8D
#define RICOH618_INT_IR_ADCEND		0x8E
#define RICOH618_INT_IR_GPIOR		0x95
#define RICOH618_INT_IR_GPIOF		0x96
#define RICOH618_INT_IR_CHGCTR		0xC2
#define RICOH618_INT_IR_CHGSTS1		0xC3
#define RICOH618_INT_IR_CHGSTS2		0xC4
#define RICOH618_INT_IR_CHGERR		0xC5
#define RICOH618_INT_IR_CHGEXTIF	0xD2

/* GPIO register base address */
#define RICOH618_GPIO_IOSEL		0x90
#define RICOH618_GPIO_IOOUT		0x91
#define RICOH618_GPIO_GPEDGE1		0x92
#define RICOH618_GPIO_GPEDGE2		0x93
/* #define RICOH618_GPIO_EN_GPIR	0x94 */
/* #define RICOH618_GPIO_IR_GPR		0x95 */
/* #define RICOH618_GPIO_IR_GPF		0x96 */
#define RICOH618_GPIO_MON_IOIN		0x97
#define RICOH618_GPIO_LED_FUNC		0x98

#define RICOH618_REG_BANKSEL		0xFF

/* Charger Control register */
#define RICOH618_CHG_CTL1		0xB3
#define	TIMSET_REG			0xB9

/* ADC Control register */
#define RICOH618_ADC_CNT1		0x64
#define RICOH618_ADC_CNT2		0x65
#define RICOH618_ADC_CNT3		0x66
#define RICOH618_ADC_VADP_THL		0x7C
#define RICOH618_ADC_VSYS_THL		0x80

#define	RICOH618_FG_CTRL		0xE0
#define	RICOH618_PSWR			0x07

/* RICOH618 IRQ definitions */
enum {
	RICOH618_IRQ_POWER_ON,
	RICOH618_IRQ_EXTIN,
	RICOH618_IRQ_PRE_VINDT,
	RICOH618_IRQ_PREOT,
	RICOH618_IRQ_POWER_OFF,
	RICOH618_IRQ_NOE_OFF,
	RICOH618_IRQ_WD,
	RICOH618_IRQ_CLK_STP,

	RICOH618_IRQ_DC1LIM,
	RICOH618_IRQ_DC2LIM,
	RICOH618_IRQ_DC3LIM,
	RICOH618_IRQ_DC4LIM,
	RICOH618_IRQ_DC5LIM,

	RICOH618_IRQ_ILIMLIR,
	RICOH618_IRQ_VBATLIR,
	RICOH618_IRQ_VADPLIR,
	RICOH618_IRQ_VUSBLIR,
	RICOH618_IRQ_VSYSLIR,
	RICOH618_IRQ_VTHMLIR,
	RICOH618_IRQ_AIN1LIR,
	RICOH618_IRQ_AIN0LIR,

	RICOH618_IRQ_ILIMHIR,
	RICOH618_IRQ_VBATHIR,
	RICOH618_IRQ_VADPHIR,
	RICOH618_IRQ_VUSBHIR,
	RICOH618_IRQ_VSYSHIR,
	RICOH618_IRQ_VTHMHIR,
	RICOH618_IRQ_AIN1HIR,
	RICOH618_IRQ_AIN0HIR,

	RICOH618_IRQ_ADC_ENDIR,

	RICOH618_IRQ_GPIO0,
	RICOH618_IRQ_GPIO1,
	RICOH618_IRQ_GPIO2,
	RICOH618_IRQ_GPIO3,
	RICOH618_IRQ_GPIO4,

	RICOH618_IRQ_CTC,
	RICOH618_IRQ_DALE,

	RICOH618_IRQ_FVADPDETSINT,
	RICOH618_IRQ_FVUSBDETSINT,
	RICOH618_IRQ_FVADPLVSINT,
	RICOH618_IRQ_FVUSBLVSINT,
	RICOH618_IRQ_FWVADPSINT,
	RICOH618_IRQ_FWVUSBSINT,

	RICOH618_IRQ_FONCHGINT,
	RICOH618_IRQ_FCHGCMPINT,
	RICOH618_IRQ_FBATOPENINT,
	RICOH618_IRQ_FSLPMODEINT,
	RICOH618_IRQ_FBTEMPJTA1INT,
	RICOH618_IRQ_FBTEMPJTA2INT,
	RICOH618_IRQ_FBTEMPJTA3INT,
	RICOH618_IRQ_FBTEMPJTA4INT,

	RICOH618_IRQ_FCURTERMINT,
	RICOH618_IRQ_FVOLTERMINT,
	RICOH618_IRQ_FICRVSINT,
	RICOH618_IRQ_FPOOR_CHGCURINT,
	RICOH618_IRQ_FOSCFDETINT1,
	RICOH618_IRQ_FOSCFDETINT2,
	RICOH618_IRQ_FOSCFDETINT3,
	RICOH618_IRQ_FOSCMDETINT,

	RICOH618_IRQ_FDIEOFFINT,
	RICOH618_IRQ_FDIEERRINT,
	RICOH618_IRQ_FBTEMPERRINT,
	RICOH618_IRQ_FVBATOVINT,
	RICOH618_IRQ_FTTIMOVINT,
	RICOH618_IRQ_FRTIMOVINT,
	RICOH618_IRQ_FVADPOVSINT,
	RICOH618_IRQ_FVUSBOVSINT,

	RICOH618_IRQ_FGCDET,
	RICOH618_IRQ_FPCDET,
	RICOH618_IRQ_FWARN_ADP,

	/* Should be last entry */
	RICOH618_NR_IRQS,
};

/* RICOH618 gpio definitions */
enum {
	RICOH618_GPIO0,
	RICOH618_GPIO1,
	RICOH618_GPIO2,
	RICOH618_GPIO3,
	RICOH618_GPIO4,

	RICOH618_NR_GPIO,
};

enum ricoh618_sleep_control_id {
	RICOH618_DS_DC1,
	RICOH618_DS_DC2,
	RICOH618_DS_DC3,
	RICOH618_DS_DC4,
	RICOH618_DS_DC5,
	RICOH618_DS_LDO1,
	RICOH618_DS_LDO2,
	RICOH618_DS_LDO3,
	RICOH618_DS_LDO4,
	RICOH618_DS_LDO5,
	RICOH618_DS_LDO6,
	RICOH618_DS_LDO7,
	RICOH618_DS_LDO8,
	RICOH618_DS_LDO9,
	RICOH618_DS_LDO10,
	RICOH618_DS_LDORTC1,
	RICOH618_DS_LDORTC2,
	RICOH618_DS_PSO0,
	RICOH618_DS_PSO1,
	RICOH618_DS_PSO2,
	RICOH618_DS_PSO3,
	RICOH618_DS_PSO4,
};

/* Defined battery information */
#define	ADC_VDD_MV	2800
#define	MIN_VOLTAGE	3100
#define	MAX_VOLTAGE	4200
#define	B_VALUE		3435


/* RICOH618 Register information */
/* bank 0 */
#define VINDAC_REG		0x03
/* for ADC */
#define	INTEN_REG		0x9D
#define	EN_ADCIR3_REG		0x8A
#define	ADCCNT3_REG		0x66
#define	VBATDATAH_REG		0x6A
#define	VBATDATAL_REG		0x6B

#define VSYSDATAH_REG	0x70
#define VSYSDATAL_REG	0x71

#define CHGCTL1_REG		0xB3
#define	REGISET1_REG	0xB6
#define	REGISET2_REG	0xB7
#define	CHGISET_REG		0xB8
#define	TIMSET_REG		0xB9
#define	BATSET1_REG		0xBA
#define	BATSET2_REG		0xBB

#define CHGSTATE_REG		0xBD

#define	SOC_REG			0xE1
#define	RE_CAP_H_REG		0xE2
#define	RE_CAP_L_REG		0xE3
#define	FA_CAP_H_REG		0xE4
#define	FA_CAP_L_REG		0xE5
#define	TT_EMPTY_H_REG		0xE7
#define	TT_EMPTY_L_REG		0xE8
#define	TT_FULL_H_REG		0xE9
#define	TT_FULL_L_REG		0xEA
#define	VOLTAGE_1_REG		0xEB
#define	VOLTAGE_2_REG		0xEC
#define	TEMP_1_REG		0xED
#define	TEMP_2_REG		0xEE

#define	CC_CTRL_REG		0xEF
#define	CC_SUMREG3_REG		0xF3
#define	CC_SUMREG2_REG		0xF4
#define	CC_SUMREG1_REG		0xF5
#define	CC_SUMREG0_REG		0xF6
#define	CC_AVERAGE1_REG		0xFB
#define	CC_AVERAGE0_REG		0xFC

/* bank 1 */
/* Top address for battery initial setting */
#define	BAT_INIT_TOP_REG	0xBC
#define	TEMP_GAIN_H_REG		0xD6
#define	TEMP_OFF_H_REG		0xD8
#define	BAT_REL_SEL_REG		0xDA
#define	BAT_TA_SEL_REG		0xDB
/* / */

/* detailed status in CHGSTATE (0xBD) */
enum ChargeState {
	CHG_STATE_CHG_OFF = 0,
	CHG_STATE_CHG_READY_VADP,
	CHG_STATE_CHG_TRICKLE,
	CHG_STATE_CHG_RAPID,
	CHG_STATE_CHG_COMPLETE,
	CHG_STATE_SUSPEND,
	CHG_STATE_VCHG_OVER_VOL,
	CHG_STATE_BAT_ERROR,
	CHG_STATE_NO_BAT,
	CHG_STATE_BAT_OVER_VOL,
	CHG_STATE_BAT_TEMP_ERR,
	CHG_STATE_DIE_ERR,
	CHG_STATE_DIE_SHUTDOWN,
	CHG_STATE_NO_BAT2,
	CHG_STATE_CHG_READY_VUSB,
};

enum SupplyState {
	SUPPLY_STATE_BAT = 0,
	SUPPLY_STATE_ADP,
	SUPPLY_STATE_USB,
} ;

struct ricoh618_battery_type_data {
	int	ch_vfchg;
	int	ch_vrchg;
	int	ch_vbatovset;
	int	ch_ichg;
	int	ch_icchg;
	int	ch_ilim_adp;
	int	ch_ilim_usb;
	int	fg_target_vsys;
	int	fg_target_ibat;
	int	fg_poff_vbat;
	int	fg_rsense_val;
	int	jt_en;
	int	jt_hw_sw;
	int	jt_temp_h;
	int	jt_temp_l;
	int	jt_vfchg_h;
	int	jt_vfchg_l;
	int	jt_ichg_h;
	int	jt_ichg_l;
};

#define BATTERY_TYPE_NUM 3
struct ricoh618_battery_platform_data {
	int	alarm_vol_mv;
	int	multiple;
	unsigned long	monitor_time;
	struct ricoh618_battery_type_data type[BATTERY_TYPE_NUM];
};

#endif
