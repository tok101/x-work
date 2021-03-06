/*
 * drivers/regulator/ricoh618-regulator.c
 *
 * Regulator driver for RICOH R5T618 power management chip.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* #define DEBUG			1 */
/*#define VERBOSE_DEBUG		1*/
#include <config.h>
#include <common.h>
#include <linux/err.h>
#include <linux/list.h>
#include <regulator.h>
#include <ingenic_soft_i2c.h>
#ifndef CONFIG_SPL_BUILD
#include <power/ricoh618.h>
#include <power/ricoh618-regulator.h>
#endif

#define RICOH618_I2C_ADDR    0x32

#define RICOH618_DC1  0x36
#define RICOH618_DC2  0x37
#define RICOH618_DC3  0x38
#define	RICOH618_LDO1 0x4c
#define	RICOH618_LDO2 0x4d
#define	RICOH618_LDO3 0x4e
#define	RICOH618_LDO4 0x4f
#define	RICOH618_LDO5 0x50
#define	RICOH618_LDORTC1 0x56
#define	RICOH618_LDORTC2 0x57

#ifndef CONFIG_SPL_BUILD
struct ricoh618_regulator {
	int		id;
	int		sleep_id;
	/* Regulator register address.*/
	u8		reg_en_reg;
	u8		en_bit;
	u8		reg_disc_reg;
	u8		disc_bit;
	u8		vout_reg;
	u8		vout_mask;
	u8		vout_reg_cache;
	u8		sleep_reg;

	/* chip constraints on regulator behavior */
	int			min_uV;
	int			max_uV;
	int			step_uV;
	int			nsteps;

	/* regulator specific turn-on delay */
	u16			delay;

	/* used by regulator core */
	struct regulator	desc;

	/* Device */
	struct device		*dev;
};

enum {
	POWER_SUPPLY_STATUS_UNKNOWN = 0,
	POWER_SUPPLY_STATUS_CHARGING,
	POWER_SUPPLY_STATUS_DISCHARGING,
	POWER_SUPPLY_STATUS_NOT_CHARGING,
	POWER_SUPPLY_STATUS_FULL,
};

enum int_type {
	ADC_INT  = 0x08,
	GPIO_INT = 0x10,
	CHG_INT  = 0x40,
};

static int bank_num = 1;

enum regulator_type {
	REGULATOR_VOLTAGE,
	REGULATOR_CURRENT,
};

struct ricoh618_soca_info {
	int OCV100_min;
	int OCV100_max;
	int ready_fg;
	int Vbat_old;
	int Rbat;
	int cc_cap_offset;
	int init_pswr;
	int rsoc_ready_flag;
	int glb_cap;
	int chg_status;         /* chg_status */
};

struct ricoh618_battery_info {
	struct device      *dev;
	int             chg_ctr;
	int             chg_stat1;
	unsigned        present:1;
	u16             delay;
	struct          ricoh618_soca_info *soca;
	bool            entry_factory_mode;
	int             ch_vfchg;
	int             ch_vrchg;
	int             ch_vbatovset;
	int             ch_ichg;
	int             ch_ilim_adp;
	int             ch_ilim_usb;
	int             ch_icchg;
	int             fg_target_vsys;
	int             fg_target_ibat;
	int             fg_poff_vbat;
	int             fg_rsense_val;
	int             jt_en;
	int             jt_hw_sw;
	int             jt_temp_h;
	int             jt_temp_l;
	int             jt_vfchg_h;
	int             jt_vfchg_l;
	int             jt_ichg_h;
	int             jt_ichg_l;
	uint8_t         adp_current_val;
	uint8_t         usb_current_val;

	int             num;
};
#endif

static struct i2c ricoh618_i2c;
static struct i2c *i2c;

static uint8_t battery_curve_para[32] = {0};
static struct ricoh618_battery_info *info = NULL;

static int ricoh618_write_reg(u8 reg, u8 *val)
{
	unsigned int  ret;
	ret = i2c_write(i2c, RICOH618_I2C_ADDR, reg, 1, val, 1);
	if(ret) {
		debug("ricoh618 write register error\n");
		return -EIO;
	}
	return 0;
}

#ifndef CONFIG_SPL_BUILD
int ricoh618_read_reg(u8 reg, u8 *val, u32 len)
{
	int ret;
	ret = i2c_read(i2c, RICOH618_I2C_ADDR, reg, 1, val, len);
	if(ret) {
		printf("ricoh618 read register error\n");
		return -EIO;
	}
	return 0;
}
void *rdev_get_drvdata(struct regulator *rdev)
{
	return rdev->reg_data;
}

int rdev_set_drvdata(struct regulator *rdev, void *data)
{

	rdev->reg_data = data;
	return 0;
}

int ricoh618_set_bits(u8 reg, uint8_t bit_mask)
{
	uint8_t reg_val;
	int ret = 0;

	ret = ricoh618_read_reg( reg, &reg_val,1);
	if (ret){
		debug("ricoh618 read error\n");
		return 1;
	}

	if ((reg_val & bit_mask) != bit_mask) {
		reg_val |= bit_mask;
		ret = ricoh618_write_reg(reg,&reg_val);
		if(ret){
			debug("ricoh618 write error\n");
			return 1;
		}
	}
	return 0;
}

int ricoh618_clr_bits(u8 reg, uint8_t bit_mask)
{
	uint8_t reg_val;
	int ret = 0;

	ret = ricoh618_read_reg( reg, &reg_val,1);
	if (ret){
		debug("ricoh618 read error\n");
		return 1;
	}

	if (reg_val & bit_mask) {
		reg_val &= ~bit_mask;
		ret = ricoh618_write_reg(reg,&reg_val);
		if(ret){
			debug("ricoh618 write error\n");
			return 1;
		}

	}
	return 0 ;
}

int ricoh618_power_off(void)
{
	int ret;
	uint8_t reg_val;

	printf("WARNNING : system will power off!\n");

	/* Clear RICOH618_FG_CTRL 0x01 bit */
	ret = ricoh618_read_reg(RICOH618_FG_CTRL, &reg_val,1);
	if (ret < 0)
		printf("Error in reading FG_CTRL\n");
	else if (reg_val & 0x01) {
		reg_val &= ~0x01;
		ret = ricoh618_write_reg(RICOH618_FG_CTRL, &reg_val);
	}

	/* set rapid timer 300 min */
	ret = ricoh618_read_reg(TIMSET_REG, &reg_val,1);
	reg_val |= 0x03;
	ret = ricoh618_write_reg(TIMSET_REG, &reg_val);
	if (ret < 0)
		printf("Error in writing the TIMSET_Reg\n");

	/* Disable all Interrupt */
	reg_val = 0;
	ricoh618_write_reg(RICOH618_INTC_INTEN, &reg_val);

	/* Not repeat power ON after power off(Power Off/N_OE) */
	reg_val = 0x0;
	ricoh618_write_reg(RICOH618_PWR_REP_CNT, &reg_val);

	/* Power OFF */
	reg_val = 0x1;
	ricoh618_write_reg(RICOH618_PWR_SLP_CNT, &reg_val);

	return 0;
}

static int ricoh618_reg_enable(struct regulator *rdev)
{
	struct ricoh618_regulator *ri = rdev_get_drvdata(rdev);
	int ret;

	ret = ricoh618_set_bits(ri->reg_en_reg, (1 << ri->en_bit));
	if(ret){
		printf("ricoh618 set bit is error\n");
	}
	return ret;
}

static int ricoh618_reg_disable(struct regulator *rdev)
{
	struct ricoh618_regulator *ri = rdev_get_drvdata(rdev);
	int ret;

	ret = ricoh618_clr_bits(ri->reg_en_reg, (1 << ri->en_bit));
	if(ret){
		printf("ricoh618 set clr is error\n");
	}

	return ret;
}


static int __ricoh618_set_voltage( struct ricoh618_regulator *ri,
		int min_uV, int max_uV, unsigned *selector)
{
	int vsel;
	int ret;
	uint8_t vout_val;

	if ((min_uV < ri->min_uV) || (max_uV > ri->max_uV))
		return -EDOM;

	vsel = (min_uV - ri->min_uV + ri->step_uV - 1)/ri->step_uV;
	if (vsel > ri->nsteps)
		return -EDOM;

	if (selector)
		*selector = vsel;


	vout_val =  (vsel & ri->vout_mask);
	ret = ricoh618_write_reg(ri->vout_reg, &vout_val);
	if (ret < 0)
		printf("Error in writing the Voltage register\n");

	return ret;
}

static int ricoh618_set_voltage(struct regulator *rdev,
		int min_uV, int max_uV, unsigned *selector)
{
	struct ricoh618_regulator *ri = rdev_get_drvdata(rdev);
	return __ricoh618_set_voltage(ri, min_uV, max_uV, NULL);
}

void test_richo(void)
{
	uint8_t val = 0;
	ricoh618_read_reg(0x2c, &val, 1);
	printf("val:%d\n", val);
	ricoh618_read_reg(0xbd, &val, 1);
	printf("val:%d\n", val);
}

int ricoh618_reg_charge_status(u8 reg, uint8_t bit_status)
{
	uint8_t reg_val;
	ricoh618_read_reg( reg, &reg_val,1);
	return (((reg_val >> bit_status) & 1) == 1);
}

static int ricoh618_reg_is_enabled(struct regulator *regulator)
{
	uint8_t control;
	struct ricoh618_regulator *ri = rdev_get_drvdata(regulator);

	ricoh618_read_reg(ri->reg_en_reg, &control,1);
	return (((control >> ri->en_bit) & 1) == 1);
}

static struct regulator_ops ricoh618_ops = {

	.disable = ricoh618_reg_disable,
	.enable = ricoh618_reg_enable,
	.set_voltage = ricoh618_set_voltage,
	.is_enabled	= ricoh618_reg_is_enabled,

#if 0
	.list_voltage	= ricoh618_list_voltage,
	.set_voltage	= ricoh618_set_voltage,
	.get_voltage	= ricoh618_get_voltage,
	.enable		= ricoh618_reg_enable,
	.disable	= ricoh618_reg_disable,
	.enable_time	= ricoh618_regulator_enable_time,
#endif

};

#define RICOH618_REG(_id, _en_reg, _en_bit, _disc_reg, _disc_bit, _vout_reg, \
		_vout_mask, _ds_reg, _min_mv, _max_mv, _step_uV, _nsteps,    \
		_ops, _delay)					\
{								\
	.reg_en_reg	= _en_reg,				\
	.en_bit		= _en_bit,				\
	.reg_disc_reg	= _disc_reg,				\
	.disc_bit	= _disc_bit,				\
	.vout_reg	= _vout_reg,				\
	.vout_mask	= _vout_mask,				\
	.sleep_reg	= _ds_reg,				\
	.step_uV	= _step_uV,				\
	.nsteps		= _nsteps,				\
	.delay		= _delay,				\
	.id		= RICOH618_ID_##_id,			\
	.sleep_id	= RICOH618_DS_##_id,			\
	.min_uV		= _min_mv * 1000,			\
	.max_uV		= _max_mv * 1000,			\
	.desc = {						\
		.name = ricoh618_rails(_id),			\
		.id = RICOH618_ID_##_id,			\
		.min_uV		= _min_mv * 1000,			\
		.max_uV		= _max_mv * 1000,			\
		.n_voltages = _nsteps,				\
		.ops = &_ops,					\
	},							\
}

static struct ricoh618_regulator ricoh618_regulator[] = {
	RICOH618_REG(DC1, 0x2C, 0, 0x2C, 1, 0x36,
			0xFF, 0x3B, 600, 3500, 12500, 0xE8,
			ricoh618_ops, 500),

	RICOH618_REG(DC2, 0x2E, 0, 0x2E, 1, 0x37,
			0xFF, 0x3C, 600, 3500, 12500, 0xE8,
			ricoh618_ops, 500),

	RICOH618_REG(DC3, 0x30, 0, 0x30, 1, 0x38,
			0xFF, 0x3D, 600, 3500, 12500, 0xE8,
			ricoh618_ops, 500),

	RICOH618_REG(LDO1, 0x44, 0, 0x46, 0, 0x4C,
			0x7F, 0x58, 900, 3500, 25000, 0x68,
			ricoh618_ops, 500),

	RICOH618_REG(LDO2, 0x44, 1, 0x46, 1, 0x4D,
			0x7F, 0x59, 900, 3500, 25000, 0x68,
			ricoh618_ops, 500),

	RICOH618_REG(LDO3, 0x44, 2, 0x46, 2, 0x4E,
			0x7F, 0x5A, 600, 3500, 25000, 0x74,
			ricoh618_ops, 500),

	RICOH618_REG(LDO4, 0x44, 3, 0x46, 3, 0x4F,
			0x7F, 0x5B, 900, 3500, 25000, 0x68,
			ricoh618_ops, 500),

	RICOH618_REG(LDO5, 0x44, 4, 0x46, 4, 0x50,
			0x7F, 0x5C, 900, 3500, 25000, 0x68,
			ricoh618_ops, 500),

	RICOH618_REG(LDORTC1, 0x45, 4, 0x00, 0, 0x56,
			0x7F, 0x00, 1700, 3500, 25000, 0x48,
			ricoh618_ops, 500),

	RICOH618_REG(LDORTC2, 0x45, 5, 0x00, 0, 0x57,
			0x7F, 0x00, 900, 3500, 25000, 0x68,
			ricoh618_ops, 500),
};

static int get_OCV_init_Data(struct ricoh618_battery_info *info, int index)
{
	int ret = 0;
	ret = (battery_curve_para[index * 2] << 8) | (battery_curve_para[index * 2 + 1]);
	return ret;
}


static int __ricoh618_write(u8 reg, int val)
{
	int ret;
	u8 val1 = val;

	ret = ricoh618_write_reg(reg, &val1);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int __ricoh618_read(u8 reg, uint8_t *val)
{
	int ret;
	ret = ricoh618_read_reg(reg, val, 1);
	if (ret < 0) {
		printf("failed reading at 0x%02x\n", reg);
		return ret;
	}
	return 0;
}

static int set_bank_ricoh618(int bank)
{
	int ret;

	if (bank != (bank & 1))
		return -EINVAL;
	if (bank == bank_num)
		return 0;
	ret = __ricoh618_write(RICOH618_REG_BANKSEL, bank);
	if (!ret)
		bank_num = bank;

	return ret;
}

static int ricoh618_write(u8 reg, uint8_t val)
{
	int ret = 0;

	ret = set_bank_ricoh618(0);
	if (!ret)
		ret = __ricoh618_write(reg, val);

	return ret;
}

static int ricoh618_read(u8 reg, uint8_t *val)
{
	int ret = 0;

	ret = set_bank_ricoh618(0);
	if (!ret)
		ret = __ricoh618_read(reg, val);

	return ret;
}

static inline int __ricoh618_bulk_writes(u8 reg, int len, uint8_t *val)
{
	int ret;
	int i;

	for (i = 0; i < len; i++) {
		printf("ricoh618: reg write  reg=%x, val=%x\n", reg + i, *(val + i));
	}

	ret = i2c_write(i2c, RICOH618_I2C_ADDR, reg, 1 , val, len);
	if (ret < 0) {
		printf("failed writings to 0x%02x\n", reg);
		return ret;
	}

	return 0;
}

static int ricoh618_bulk_writes(u8 reg, u8 len, uint8_t *val)
{
	int ret = 0;
	ret = set_bank_ricoh618(0);
	if (!ret)
		ret = __ricoh618_bulk_writes(reg, len, val);

	return ret;
}

static inline int __ricoh618_bulk_reads(u8 reg, int len, uint8_t *val)
{
	int ret;

	ret = i2c_read(i2c, RICOH618_I2C_ADDR, reg, 1 , val, len);
	if (ret < 0) {
		printf("failed reading from 0x%02x\n", reg);
		return ret;
	}
	return 0;
}

static int ricoh618_bulk_reads(u8 reg, u8 len, uint8_t *val)
{
	int ret = 0;

	ret = set_bank_ricoh618(0);
	if (!ret)
		ret = __ricoh618_bulk_reads(reg, len, val);

	return ret;
}

static int set_cc_sum_back(struct ricoh618_battery_info *info, int *val)
{
	uint8_t         cc_sum_reg[4];
	uint8_t         fa_cap_reg[2];
	uint16_t        fa_cap;
	uint32_t        cc_sum;
	int err;
	int value = *val;

	err = ricoh618_bulk_reads(FA_CAP_H_REG, 2, fa_cap_reg);
	if (err < 0)
		return -1;

	/* fa_cap = *(uint16_t*)fa_cap_reg & 0x7fff; */
	fa_cap = (fa_cap_reg[0] << 8 | fa_cap_reg[1]) & 0x7fff;

	if (fa_cap == 0)
		return -1;
	else if (value >= 0) {
		cc_sum = value * 9 * fa_cap/25;
	} else {
		cc_sum = -value * 9 * fa_cap /25;
		cc_sum = cc_sum - 0x01;
		cc_sum = cc_sum^0xffffffff;
	}
	//printf("Ross's CC_SUM is %d \n",cc_sum);
	//printf("VALis %d \n",value);

	cc_sum_reg[3]= cc_sum & 0xff;
	cc_sum_reg[2]= (cc_sum & 0xff00)>> 8;
	cc_sum_reg[1]= (cc_sum & 0xff0000) >> 16;
	cc_sum_reg[0]= (cc_sum & 0xff000000) >> 24;

	//printf("reg0~3 is %x %x %x %x \n",cc_sum_reg[0],cc_sum_reg[1],cc_sum_reg[2],cc_sum_reg[3]);

	err = ricoh618_set_bits(CC_CTRL_REG, 0x01);
	if (err < 0)
		return -1;

	err = ricoh618_bulk_writes(CC_SUMREG3_REG, 4, cc_sum_reg);
	if (err < 0)
		return -1;

	/* CC_pause exit */
	err = ricoh618_clr_bits(CC_CTRL_REG, 0x01);
	if (err < 0)
		return -1;

	return 0;
}

static int get_OCV_voltage(struct ricoh618_battery_info *info, int index)
{
	int ret = 0;
	ret =  get_OCV_init_Data(info, index);
	/* conversion unit 1 Unit is 1.22mv (5000/4095 mv) */
	ret = ret * 50000 / 4095;
	/* return unit should be 1uV */
	ret = ret * 100;
	// printf("the ret is:%d\n",ret);
	return ret;
}

static int get_check_fuel_gauge_reg(struct ricoh618_battery_info *info,
		int Reg_h, int Reg_l, int enable_bit)
{
	uint8_t get_data_h, get_data_l;
	int old_data, current_data;
	int i;
	int ret = 0;

	old_data = 0;

	for (i = 0; i < 5 ; i++) {
		ret = ricoh618_read(Reg_h, &get_data_h);
		if (ret < 0) {
			printf("Error in reading the control register\n");
			return ret;
		}

		ret = ricoh618_read(Reg_l, &get_data_l);
		if (ret < 0) {
			printf("Error in reading the control register\n");
			return ret;
		}

		current_data = ((get_data_h & 0xff) << 8) | (get_data_l & 0xff);
		current_data = (current_data & enable_bit);

		if (current_data == old_data)
			return current_data;
		else
			old_data = current_data;
	}

	return current_data;
}

/* battery voltage is get from Fuel gauge */
static int measure_vbatt_FG(struct ricoh618_battery_info *info, int *data)
{
	int ret = 0;

	ret = get_check_fuel_gauge_reg(info, VOLTAGE_1_REG, VOLTAGE_2_REG, 0x0fff);
	if (ret < 0) {
		printf("Error in reading the fuel gauge control register\n");
		return ret;
	}

	*data = ret;
	/* conversion unit 1 Unit is 1.22mv (5000/4095 mv) */
	*data = *data * 50000 / 4095;
	/* return unit should be 1uV */
	*data = *data * 100;

	return ret;
}

static int measure_Ibatt_FG(struct ricoh618_battery_info *info, int *data)
{
	int ret = 0;

	ret =  get_check_fuel_gauge_reg(info, CC_AVERAGE1_REG, CC_AVERAGE0_REG, 0x3fff);
	if (ret < 0) {
		printf("Error in reading the fuel gauge control register\n");
		return ret;
	}

	*data = (ret > 0x1fff) ? (ret - 0x4000) : ret;
	return ret;
}

static int calc_ocv(struct ricoh618_battery_info *info)
{
	int Vbat = 0;
	int Ibat = 0;
	int ocv;

	measure_vbatt_FG(info, &Vbat);
	measure_Ibatt_FG(info, &Ibat);

	ocv = Vbat - Ibat * info->soca->Rbat;
	printf("[ RICOH618 ] Vbat: %d, Ibat: %d\n", Vbat, Ibat);
	return ocv;
}

static int calc_capacity_in_period(struct ricoh618_battery_info *info,
		int *cc_cap, bool *is_charging, bool cc_rst)
{
	int err;
	uint8_t         cc_sum_reg[4];
	uint8_t         cc_clr[4] = {0, 0, 0, 0};
	uint8_t         fa_cap_reg[2];
	uint16_t        fa_cap;
	uint32_t        cc_sum;
	int             cc_stop_flag;
	uint8_t         status;
	uint8_t         charge_state;
	int             Ocv;
	uint32_t        cc_cap_temp;
	uint32_t        cc_cap_min;
	int             cc_cap_res;

	*is_charging = true;    /* currrent state initialize -> charging */

	/* get  power supply status */
	err = ricoh618_read(CHGSTATE_REG, &status);
	if (err < 0)
		goto out;
	charge_state = (status & 0x1F);
	Ocv = calc_ocv(info);
	if (charge_state == CHG_STATE_CHG_COMPLETE) {
		/* Check CHG status is complete or not */
		cc_stop_flag = 0;
	} else if (Ocv < get_OCV_voltage(info, 9)) {
		/* Check VBAT is high level or not */
		cc_stop_flag = 0;
	} else {
		cc_stop_flag = 1;
	}

	if (cc_stop_flag == 1)
	{
		/* Disable Charging/Completion Interrupt */
		err = ricoh618_set_bits(RICOH618_INT_MSK_CHGSTS1, 0x01);
		if (err < 0)
			goto out;

		/* disable charging */
		err = ricoh618_clr_bits( RICOH618_CHG_CTL1, 0x03);
		if (err < 0)
			goto out;
	}

	/* CC_pause enter */
	err = ricoh618_write(CC_CTRL_REG, 0x01);
	if (err < 0)
		goto out;

	/* Read CC_SUM */
	err = ricoh618_bulk_reads(CC_SUMREG3_REG, 4, cc_sum_reg);
	if (err < 0)
		goto out;

	if (cc_rst == true) {
		/* CC_SUM <- 0 */
		err = ricoh618_bulk_writes(CC_SUMREG3_REG, 4, cc_clr);
		if (err < 0)
			goto out;
	}

	/* CC_pause exist */
	err = ricoh618_write( CC_CTRL_REG, 0);
	if (err < 0)
		goto out;
	if (cc_stop_flag == 1)
	{
		/* Enable charging */
		err = ricoh618_set_bits( RICOH618_CHG_CTL1, 0x03);
		if (err < 0)
			goto out;

		udelay(1000);

		/* Clear Charging Interrupt status */
		err = ricoh618_clr_bits(RICOH618_INT_IR_CHGSTS1, 0x01);
		if (err < 0)
			goto out;

		/* Enable Charging Interrupt */
		err = ricoh618_clr_bits(RICOH618_INT_MSK_CHGSTS1, 0x01);
		if (err < 0)
			goto out;
	}
	/* Read FA_CAP */
	err = ricoh618_bulk_reads(FA_CAP_H_REG, 2, fa_cap_reg);
	if (err < 0)
		goto out;

	/* fa_cap = *(uint16_t*)fa_cap_reg & 0x7fff; */
	fa_cap = ((fa_cap_reg[0] << 8 | fa_cap_reg[1]) & 0x7fff);

	/* cc_sum = *(uint32_t*)cc_sum_reg; */
	cc_sum = cc_sum_reg[0] << 24 | cc_sum_reg[1] << 16 |
		cc_sum_reg[2] << 8 | cc_sum_reg[3];

	/* calculation  two's complement of CC_SUM */
	if (cc_sum & 0x80000000) {
		cc_sum = (cc_sum^0xffffffff)+0x01;
		*is_charging = false;           /* discharge */
	}
	/* (CC_SUM x 10000)/3600/FA_CAP */

	if(fa_cap == 0)
		goto out;
	else
		*cc_cap = cc_sum*25/9/fa_cap;           /* unit is 0.01% */

	cc_cap_min = fa_cap*3600/100/100/100;   /* Unit is 0.0001% */

	if(cc_cap_min == 0)
		goto out;
	else
		cc_cap_temp = cc_sum / cc_cap_min;

	cc_cap_res = cc_cap_temp % 100;

	if(*is_charging) {
		info->soca->cc_cap_offset += cc_cap_res;
		if (info->soca->cc_cap_offset >= 100) {
			*cc_cap += 1;
			info->soca->cc_cap_offset %= 100;
		}
	}

	return 0;
out:
	printf("Error !!-----\n");
	return err;
}

int ricoh618_clr_left(void)
{
	int ret = 0;
	int err = 0;
	int cc_cap = 0;
	int cc_left = 0;
	bool is_charging = true;

	ret = calc_capacity_in_period(info, &cc_cap, &is_charging, false);
	if(ret < 0){
		return ret;
	}

	if(cc_cap > 100){
		cc_left = cc_cap%100;
		cc_left = (is_charging == true)?cc_left:-cc_left;
		//printf("cc_left is %d\n",cc_left);
		err = set_cc_sum_back(info,&cc_left);
		if(err<0)
			printf("set error!\n");
	}
	return 0;
}

int ricoh618_set_pswr(uint8_t value)
{
	int ret;

	value &= 0x7F;
	ret = ricoh618_write_reg(RICOH618_PSWR, &value);
	if (ret < 0){
		printf("Error in writing PSWR_REG %d\n", value);
		return ret;
	}
	return 0;
}

int ricoh618_clear_pswr(void)
{
	int err = 0;
	ricoh618_write(RICOH618_PSWR,0);
	if (err < 0){
		printf("Error in writing RICOH618_PSWR %d\n",err);
		return err;
	}
	return 0;
}

static int get_power_supply_status(struct ricoh618_battery_info *info)
{
	uint8_t status;
	uint8_t supply_state;
	uint8_t charge_state;
	int ret = 0;

	/* get  power supply status */
	ret = ricoh618_read(CHGSTATE_REG, &status);
	if (ret < 0) {
		printf(  "Error in reading the control register\n");
		return ret;
	}

	charge_state = (status & 0x1F);
	supply_state = ((status & 0xC0) >> 6);

	if (info->entry_factory_mode)
		return POWER_SUPPLY_STATUS_NOT_CHARGING;

	if (supply_state == SUPPLY_STATE_BAT) {
		info->soca->chg_status = POWER_SUPPLY_STATUS_DISCHARGING;
	} else {
		switch (charge_state) {
			case    CHG_STATE_CHG_OFF:
				info->soca->chg_status
					= POWER_SUPPLY_STATUS_DISCHARGING;
				break;
			case    CHG_STATE_CHG_READY_VADP:
				info->soca->chg_status
					= POWER_SUPPLY_STATUS_NOT_CHARGING;
				break;
			case    CHG_STATE_CHG_TRICKLE:
				info->soca->chg_status
					= POWER_SUPPLY_STATUS_CHARGING;
				break;
			case    CHG_STATE_CHG_RAPID:
				info->soca->chg_status
					= POWER_SUPPLY_STATUS_CHARGING;
				break;
			case    CHG_STATE_CHG_COMPLETE:
				info->soca->chg_status
					= POWER_SUPPLY_STATUS_FULL;
				break;
			case    CHG_STATE_SUSPEND:
				info->soca->chg_status
					= POWER_SUPPLY_STATUS_DISCHARGING;
				break;
			case    CHG_STATE_VCHG_OVER_VOL:
				info->soca->chg_status
					= POWER_SUPPLY_STATUS_DISCHARGING;
				break;
			case    CHG_STATE_BAT_ERROR:
				info->soca->chg_status
					= POWER_SUPPLY_STATUS_NOT_CHARGING;
				break;
			case    CHG_STATE_NO_BAT:
				info->soca->chg_status
					= POWER_SUPPLY_STATUS_NOT_CHARGING;
				break;
			case    CHG_STATE_BAT_OVER_VOL:
				info->soca->chg_status
					= POWER_SUPPLY_STATUS_NOT_CHARGING;
				break;
			case    CHG_STATE_BAT_TEMP_ERR:
				info->soca->chg_status
					= POWER_SUPPLY_STATUS_NOT_CHARGING;
				break;
			case    CHG_STATE_DIE_ERR:
				info->soca->chg_status
					= POWER_SUPPLY_STATUS_NOT_CHARGING;
				break;
			case    CHG_STATE_DIE_SHUTDOWN:
				info->soca->chg_status
					= POWER_SUPPLY_STATUS_DISCHARGING;
				break;
			case    CHG_STATE_NO_BAT2:
				info->soca->chg_status
					= POWER_SUPPLY_STATUS_NOT_CHARGING;
				break;
			case    CHG_STATE_CHG_READY_VUSB:
				info->soca->chg_status
					= POWER_SUPPLY_STATUS_NOT_CHARGING;
				break;
			default:
				info->soca->chg_status
					= POWER_SUPPLY_STATUS_UNKNOWN;
				break;
		}
	}

	return info->soca->chg_status;
}

static int calc_capacity_2(void)
{
	uint8_t val;
	int capacity;
	int re_cap, fa_cap;
	int ret = 0;

	re_cap = get_check_fuel_gauge_reg(info, RE_CAP_H_REG, RE_CAP_L_REG,
			0x7fff);
	fa_cap = get_check_fuel_gauge_reg(info, FA_CAP_H_REG, FA_CAP_L_REG,
			0x7fff);

	if (fa_cap != 0) {
		capacity = re_cap * 100 / fa_cap;
		printf("re_cap:%d,fa_cap:%d,capacity:%d\n",re_cap,fa_cap,capacity);
		capacity = min(100, (int)capacity);
		capacity = max(0, (int)capacity);
	} else {
		ret = ricoh618_read(SOC_REG, &val);
		if (ret < 0) {
			printf("PMU:Error in reading the control register\n");
			return ret;
		}
		capacity = val;
	}
	return capacity;
}

//extern unsigned int read_battery_voltage(void);
int calc_battery_capacity(void)
{
	uint8_t capacity;
	uint8_t val;
	uint8_t rsoc;
	int voltage;
	int ret = 0;
	int err = 0;
	int cc_cap = 0;
	bool is_charging = true;

	if (info == NULL) {
		printf("[ ricoh618 ] Init PMU first!\n");
		return -1;
	}
	ret = ricoh618_read(RICOH618_PSWR, &val);
	if (ret < 0) {
		printf("Error in reading PSWR_REG %d\n", ret);
		return ret;
	}

	ricoh618_write(RICOH618_PSWR,0x6c);
//	udelay(1000 * 1000);
	ret = ricoh618_read(RICOH618_PSWR, &val);
	if (ret < 0) {
		printf("Error in reading PSWR_REG %d\n", ret);
		return ret;
	}

	info->soca->init_pswr = val & 0x7f;
	if(val == 0){
		info->soca->rsoc_ready_flag = 1;
		printf("first power on\n");
		return -1;
	}else{
		info->soca->rsoc_ready_flag = 0;
		ret = calc_capacity_in_period(info, &cc_cap, &is_charging, false);
		if(ret < 0){
			return ret;
		}

		capacity = info->soca->glb_cap + (cc_cap / 100);

		ret = ricoh618_read(SOC_REG, &rsoc);
		if(ret < 0){
			return ret;
		}
		ret = ricoh618_read(CHGSTATE_REG, &val);
		if(ret < 0){
			return ret;
		}
//		voltage = read_battery_voltage();

//		if(((val & 0x1F) == 0x04) && (voltage > (CONFIG_BATTERY_VOLTAGE_MAX - 50)) && (rsoc < 100)){
		if((val & 0x1F) == 0x04){
			capacity = 100;
			err = ricoh618_write(RICOH618_FG_CTRL,0x51);
			if(err < 0){
				return err;
			}
			err = ricoh618_set_pswr(capacity);
			if(err < 0){
				return err;
			}
			info->soca->glb_cap = capacity;
			err = ricoh618_clr_left();
			if(err < 0){
				return err;
			}
			calc_capacity_in_period(info, &cc_cap, &is_charging, false);
			printf("battery is full!! info->soca->glb_cap:%d,cc_cap:%d\n",info->soca->glb_cap,cc_cap);
			err = ricoh618_clr_bits(RICOH618_INT_IR_CHGSTS1, 0x02);
			if (err < 0){
				return err;
			}
		}

		capacity = min(100, capacity);
		capacity = max(0, capacity);
		printf("[ ricoh618 ] capacity %d, info->soca->glb_cap:%d, cc_cap %d\n",
				capacity, info->soca->glb_cap, cc_cap);
	}

	ret = ricoh618_set_pswr(capacity);
	if(ret < 0){
		return ret;
	}

	return capacity;
}

/* Initial setting of charger */
static int ricoh618_init_charger(struct ricoh618_battery_info *info)
{
	int err;
	uint8_t val;
	uint8_t val2;
	uint8_t val3;
	uint8_t rsoc;
	int     charge_status;
	int     vfchg_val;
	int     icchg_val;
	int     rbat;
	int     temp;
	int     cap_tmp;

	info->chg_ctr = 0;
	info->chg_stat1 = 0;
	err = ricoh618_set_bits(RICOH618_PWR_FUNC, 0x20);
	if (err < 0) {
		printf(  "Error in writing the PWR FUNC register\n");
		goto free_device;
	}

	err = ricoh618_read(RICOH618_PWR_ON_HISORY, &val);
	if (err < 0) {
		printf(  "Error in reading the PWR ON HISORY register\n");
		goto free_device;
	}
	printf("[ RICOH618 ] power on history is: %d\n", val);

	err = ricoh618_read(RICOH618_PWR_OFF_HISORY, &val);
	if (err < 0) {
		printf(  "Error in reading the PWR OFF HISORY register\n");
		goto free_device;
	}
	printf("[ RICOH618 ] power off history is: %d\n", val);

	err = ricoh618_write(RICOH618_FG_CTRL,0x11);
	if (err < 0) {
		printf(  "Error in writing the FG CTRL register\n");
		goto free_device;
	}
	/* ricoh618_read(RICOH618_FG_CTRL, &val); */
	/* printf("[ RICOH618 ] e0 register val is: %d\n", val); */

	char *str = getenv ("first_boot_complete");
	if(str == NULL){
		setenv("first_boot_complete", "1");
		saveenv();

		ricoh618_write(RICOH618_PSWR,0);

		ricoh618_read(RICOH618_PSWR, &val);

		printf("first boot completed! val is:%d\n",val);
	}

	int ret = ricoh618_read(RICOH618_PSWR, &val);
	if (ret < 0) {
		printf("Charger init Error in reading PSWR_REG %d\n", ret);
		goto free_device;
	}
	info->soca->glb_cap = val & 0x7f;

	cap_tmp = calc_capacity_2();
	if(cap_tmp < 0){
		printf("Charger init Error in calc capacity %d\n", ret);
		goto free_device;
	}

	if((cap_tmp - info->soca->glb_cap) > 30 || (info->soca->glb_cap - cap_tmp) > 30){
		printf("To calibrate the battery,clear pswr\n");
		err = ricoh618_clear_pswr();
		if(err < 0){
			goto free_device;
		}
	}else{
		ret = ricoh618_read(SOC_REG, &rsoc);
		if(ret < 0){
			goto free_device;
		}
		printf("rsoc:%d\n",rsoc);
		if(rsoc > 0){
			info->soca->glb_cap = rsoc;
			//ricoh618_set_pswr(info->soca->glb_cap);
		}
	}

	charge_status = get_power_supply_status(info);

	if (charge_status != POWER_SUPPLY_STATUS_FULL)
	{
		/* Disable charging */
		err = ricoh618_clr_bits(CHGCTL1_REG, 0x03);
		if (err < 0) {
			printf(  "Error in writing the control register\n");
			goto free_device;
		}
	}

	/* REGISET1:(0xB6) setting */
	if ((info->ch_ilim_adp != 0xFF) || (info->ch_ilim_adp <= 0x1D)) {
		val = info->ch_ilim_adp;

		err = ricoh618_write(REGISET1_REG,val);
		if (err < 0) {
			printf(  "Error in writing REGISET1_REG %d\n",
					err);
			goto free_device;
		}
		info->adp_current_val = val;
	}
	else info->adp_current_val = 0xff;

	/* REGISET2:(0xB7) setting */
	err = ricoh618_read(REGISET2_REG, &val);
	if (err < 0) {
		printf(
				"Error in read REGISET2_REG %d\n", err);
		goto free_device;
	}

	if ((info->ch_ilim_usb != 0xFF) || (info->ch_ilim_usb <= 0x1D)) {
		val2 = info->ch_ilim_usb;
	} else {/* Keep OTP value */
		val2 = (val & 0x1F);
	}

	/* keep bit 5-7 */
	val &= 0xE0;

	val = val + val2;
	info->usb_current_val = val;
	err = ricoh618_write(REGISET2_REG,val);
	if (err < 0) {
		printf(  "Error in writing REGISET2_REG %d\n", err);
		goto free_device;
	}

	err = ricoh618_read(CHGISET_REG, &val);
	if (err < 0) {
		printf("Error in read CHGISET_REG %d\n", err);
		goto free_device;
	}

	/* Define Current settings value for charging (bit 4~0)*/
	if ((info->ch_ichg != 0xFF) || (info->ch_ichg <= 0x1D)) {
		val2 = info->ch_ichg;
	} else { /* Keep OTP value */
		val2 = (val & 0x1F);
	}

	/* Define Current settings at the charge completion (bit 7~6)*/
	if ((info->ch_icchg != 0xFF) || (info->ch_icchg <= 0x03)) {
		val3 = info->ch_icchg << 6;
	} else { /* Keep OTP value */
		val3 = (val & 0xC0);
	}

	val = val2 + val3;

	err = ricoh618_write(CHGISET_REG, val);
	if (err < 0) {
		printf(  "Error in writing CHGISET_REG %d\n",
				err);
		goto free_device;
	}

	//debug messeage
	err = ricoh618_read(CHGISET_REG,&val);

	//debug messeage
	err = ricoh618_read(BATSET1_REG,&val);

	/* BATSET1_REG(0xBA) setting */
	err = ricoh618_read(BATSET1_REG, &val);
	if (err < 0) {
		printf("Error in read BATSET1 register %d\n", err);
		goto free_device;
	}

	/* Define Battery overvoltage  (bit 4)*/
	if ((info->ch_vbatovset != 0xFF) || (info->ch_vbatovset <= 0x1)) {
		val2 = info->ch_vbatovset;
		val2 = val2 << 4;
	} else { /* Keep OTP value */
		val2 = (val & 0x10);
	}

	/* keep bit 0-3 and bit 5-7 */
	val = (val & 0xEF);

	val = val + val2;

	err = ricoh618_write(BATSET1_REG, val);
	if (err < 0) {
		printf(  "Error in writing BAT1_REG %d\n", err);
		goto free_device;
	}
	//debug messeage
	err = ricoh618_read(BATSET1_REG,&val);

	//debug messeage
	err = ricoh618_read(BATSET2_REG,&val);


	/* BATSET2_REG(0xBB) setting */
	err = ricoh618_read(BATSET2_REG, &val);
	if (err < 0) {
		printf("Error in read BATSET2 register %d\n", err);
		goto free_device;
	}

	/* Define Re-charging voltage (bit 2~0)*/
	if ((info->ch_vrchg != 0xFF) || (info->ch_vrchg <= 0x04)) {
		val2 = info->ch_vrchg;
	} else { /* Keep OTP value */
		val2 = (val & 0x07);
	}

	/* Define FULL charging voltage (bit 6~4)*/
	if ((info->ch_vfchg != 0xFF) || (info->ch_vfchg <= 0x04)) {
		val3 = info->ch_vfchg;
		val3 = val3 << 4;
	} else {        /* Keep OTP value */
		val3 = (val & 0x70);
	}

	/* keep bit 3 and bit 7 */
	val = (val & 0x88);

	val = val + val2 + val3;

	err = ricoh618_write(BATSET2_REG, val);
	if (err < 0) {
		printf(  "Error in writing RICOH618_RE_CHARGE_VOLTAGE %d\n", err);
		goto free_device;
	}

	/* Set rising edge setting ([1:0]=01b)for INT in charging */
	/*  and rising edge setting ([3:2]=01b)for charge completion */
	err = ricoh618_read(RICOH618_CHG_STAT_DETMOD1, &val);
	if (err < 0) {
		printf(  "Error in reading CHG_STAT_DETMOD1 %d\n", err);
		goto free_device;
	}
	val &= 0xf0;
	val |= 0x05;
	err = ricoh618_write(RICOH618_CHG_STAT_DETMOD1, val);
	if (err < 0) {
		printf(  "Error in writing CHG_STAT_DETMOD1 %d\n", err);
		goto free_device;
	}

	/* Unmask In charging/charge completion */
	err = ricoh618_write(RICOH618_INT_MSK_CHGSTS1, 0xfc);
	if (err < 0) {
		printf(  "Error in writing INT_MSK_CHGSTS1 %d\n", err);
		goto free_device;
	}

	/* Set both edge for VUSB([3:2]=11b)/VADP([1:0]=11b) detect */
	err = ricoh618_read(RICOH618_CHG_CTRL_DETMOD1, &val);
	if (err < 0) {
		printf(  "Error in reading CHG_CTRL_DETMOD1 %d\n", err);
		goto free_device;
	}
	val &= 0xf0;
	val |= 0x0f;
	err = ricoh618_write(RICOH618_CHG_CTRL_DETMOD1, val);
	if (err < 0) {
		printf(  "Error in writing CHG_CTRL_DETMOD1 %d\n", err);
		goto free_device;
	}

	/* Unmask In VUSB/VADP completion */
	err = ricoh618_write(RICOH618_INT_MSK_CHGCTR, 0xfc);
	if (err < 0) {
		printf(  "Error in writing INT_MSK_CHGSTS1 %d\n", err);
		goto free_device;
	}

	if (charge_status != POWER_SUPPLY_STATUS_FULL)
	{
		/* Enable charging */
		err = ricoh618_set_bits(CHGCTL1_REG, 0x03);
		if (err < 0) {
			printf(  "Error in writing the control register\n");
			goto free_device;
		}
	}
	/* get OCV100_min, OCV100_min*/
	temp = get_OCV_init_Data(info, 12);
	rbat = temp * 1000 / 512 * 5000 / 4095;

	/* get vfchg value */
	err = ricoh618_read(BATSET2_REG, &val);
	if (err < 0) {
		printf(  "Error in reading the batset2reg\n");
		goto free_device;
	}
	val &= 0x70;
	val2 = val >> 4;
	if (val2 <= 3) {
		vfchg_val = 4050 + val2 * 50;
	} else {
		vfchg_val = 4350;
	}

	/* get  value */
	err = ricoh618_read(CHGISET_REG, &val);
	if (err < 0) {
		printf(  "Error in reading the chgisetreg\n");
		goto free_device;
	}
	val &= 0xC0;
	val2 = val >> 6;
	icchg_val = 50 + val2 * 50;

	info->soca->OCV100_min = ( vfchg_val * 99 / 100 - (icchg_val * (rbat +20))/1000 - 20 ) * 1000;
	info->soca->OCV100_max = ( vfchg_val * 101 / 100 - (icchg_val * (rbat +20))/1000 + 20 ) * 1000;

	/* Set reset time to 10s */
	{
		int error = 0;
		uint8_t value = 0;
		error = ricoh618_read(RICOH618_PWR_ON_TIMSET, &value);
		if (error < 0){
			printf("Read RICOH618_PWR_ON_TIMSET error\n");
			goto free_device;
		}else{
			debug("Read RICOH618_PWR_ON_TIMSET value %d\n", value);
		}
		value &= 0x0F;
		value |= 0x60;
		error = ricoh618_write(RICOH618_PWR_ON_TIMSET, value);
		debug("Read RICOH618_PWR_ON_TIMSET value %d\n", value);
		if (error < 0){
			printf("Write RICOH618_PWR_ON_TIMSET error\n");
			goto free_device;
		}
	}

free_device:
	return err;
}

int ricoh618_battery_init(struct ricoh618_battery_type_data * pdata,
		uint8_t curve_table[32])
{
	int temp =  0;
	int ret = 0;

	info = malloc(sizeof(struct ricoh618_battery_info));
	if (!info){
		printf("[ RICOH618 ] Malloc info fail in ricoh618 battery init\n");
		return -ENOMEM;
	}
	info->soca = malloc(sizeof(struct ricoh618_soca_info));
	if (!info->soca){
		printf("[ RICOH618 ] Malloc info->soca fail in ricoh618 battery init\n");
		free(info);
		return -ENOMEM;
	}

	info->ch_vfchg = pdata->ch_vfchg;
	info->ch_vrchg = pdata->ch_vrchg;
	info->ch_vbatovset = pdata->ch_vbatovset;
	info->ch_ichg = pdata->ch_ichg;
	info->ch_ilim_adp = pdata->ch_ilim_adp;
	info->ch_ilim_usb = pdata->ch_ilim_usb;
	info->ch_icchg = pdata->ch_icchg;
	if ( pdata->fg_rsense_val == 0 )
		info->fg_rsense_val = 100;
	else
		info->fg_rsense_val = pdata->fg_rsense_val;
	info->fg_target_vsys = pdata->fg_target_vsys;
	info->fg_target_ibat = pdata->fg_target_ibat * info->fg_rsense_val / 20;
	info->fg_poff_vbat = pdata->fg_poff_vbat;
	info->jt_en = pdata->jt_en;
	info->jt_hw_sw = pdata->jt_hw_sw;
	info->jt_temp_h = pdata->jt_temp_h;
	info->jt_temp_l = pdata->jt_temp_l;
	info->jt_vfchg_h = pdata->jt_vfchg_h;
	info->jt_vfchg_l = pdata->jt_vfchg_l;
	info->jt_ichg_h = pdata->jt_ichg_h;
	info->jt_ichg_l = pdata->jt_ichg_l;

	memcpy(battery_curve_para, curve_table, 32);
	temp = get_OCV_init_Data(info, 11) * info->fg_rsense_val / 20;
	battery_curve_para[22] = (temp >> 8);
	battery_curve_para[23] = (temp & 0xff);
	temp = get_OCV_init_Data(info, 12) * 20 / info->fg_rsense_val;
	battery_curve_para[24] = (temp >> 8);
	battery_curve_para[25] = (temp & 0xff);

	ricoh618_clr_bits(RICOH618_INTC_INTEN, CHG_INT | ADC_INT);

	ret = ricoh618_init_charger(info);
	if (ret < 0) {
		printf("[ RICOH618 ] Fail to init ricoh618_init_charger\n");
		free(info->soca);
		free(info);
		return -1;
	}

	ricoh618_set_bits(RICOH618_INTC_INTEN, CHG_INT | ADC_INT);

	return 0;
}

int ricoh618_regulator_init(void)
{
	int ret;
	ricoh618_i2c.scl = CONFIG_RICOH618_I2C_SCL;
	ricoh618_i2c.sda = CONFIG_RICOH618_I2C_SDA;
	i2c = &ricoh618_i2c;
	i2c_init(i2c);

	ret = i2c_probe(i2c, RICOH618_I2C_ADDR);

	if(ret) {
		printf("probe richo618 error, i2c addr ox%x\n", RICOH618_I2C_ADDR);
		return -EIO;
	}
#if 0
	for (i = 0; i < ARRAY_SIZE(ricoh618_regulator); i++) {
		ret = regulator_register(&ricoh618_regulator[i].desc, NULL);
		rdev_set_drvdata(&ricoh618_regulator[i].desc,&ricoh618_regulator[i]);
		if(ret)
			printf("%s regulator_register error\n",
					ricoh618_regulator[i].desc.name);
	}
#endif
	return ret;
}
#endif

#ifdef CONFIG_SPL_BUILD
int spl_regulator_init(void)
{
	int ret;

	ricoh618_i2c.scl = CONFIG_RICOH618_I2C_SCL;
	ricoh618_i2c.sda = CONFIG_RICOH618_I2C_SDA;
	i2c = &ricoh618_i2c;
	i2c_init(i2c);

	ret = i2c_probe(i2c, RICOH618_I2C_ADDR);

	return ret;
}
int spl_regulator_set_voltage(enum regulator_outnum outnum, int vol_mv)
{
	char reg;
	u8 regvalue;

	switch(outnum) {
		case REGULATOR_CORE:
			reg = RICOH618_DC1;
			if ((vol_mv < 1000) || (vol_mv >1300)) {
				debug("voltage for core is out of range\n");
				return -EINVAL;
			}
			break;
		case REGULATOR_MEM:
			reg = RICOH618_DC2;
			break;
		case REGULATOR_IO:
			reg = RICOH618_DC3;
			break;
		default:return -EINVAL;
	}

	if ((vol_mv < 600) || (vol_mv > 3500)) {
		debug("unsupported voltage\n");
		return -EINVAL;
	} else {
		regvalue = ((vol_mv - 600) * 10)/ 125;
	}

	return ricoh618_write_reg(reg, &regvalue);
}
#endif
