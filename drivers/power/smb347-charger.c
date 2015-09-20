/*
 * Summit Microelectronics SMB347 Battery Charger Driver
 *
 * Copyright (C) 2011, Intel Corporation
 *
 * Authors: Bruce E. Robertson <bruce.e.robertson@intel.com>
 *          Mika Westerberg <mika.westerberg@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/power_supply.h>
#include <linux/power/smb347-charger.h>
<<<<<<< HEAD
#include <linux/seq_file.h>
#include <linux/delay.h>
=======
#include <linux/regmap.h>
>>>>>>> v3.10.88

/*
 * Configuration registers. These are mirrored to volatile RAM and can be
 * written once %CMD_A_ALLOW_WRITE is set in %CMD_A register. They will be
 * reloaded from non-volatile registers after POR.
 */
#define CFG_CHARGE_CURRENT			0x00
#define CFG_CHARGE_CURRENT_FCC_MASK		0xe0
#define CFG_CHARGE_CURRENT_FCC_SHIFT		5
#define CFG_CHARGE_CURRENT_PCC_MASK		0x18
#define CFG_CHARGE_CURRENT_PCC_SHIFT		3
#define CFG_CHARGE_CURRENT_TC_MASK		0x07
#define CFG_CURRENT_LIMIT			0x01
#define CFG_CURRENT_LIMIT_DC_MASK		0xf0
#define CFG_CURRENT_LIMIT_DC_SHIFT		4
#define CFG_CURRENT_LIMIT_USB_MASK		0x0f
#define CFG_VARIOUS_FUNCTION                    0x02
#define CFG_INPUT_SOURCE_PRIORITY               BIT(2)
#define CFG_FLOAT_VOLTAGE			0x03
#define CFG_FLOAT_VOLTAGE_FLOAT_MASK		0x3f
#define CFG_FLOAT_VOLTAGE_THRESHOLD_MASK	0xc0
#define CFG_FLOAT_VOLTAGE_MASK			0x3F
#define CFG_FLOAT_VOLTAGE_THRESHOLD_SHIFT	6
#define CFG_CHARGE_CONTROL			0x04
#define CFG_AUTOMATIC_RECHARGE_DISABLE		BIT(7)
#define CFG_STAT				0x05
#define CFG_STAT_DISABLED			BIT(5)
#define CFG_STAT_ACTIVE_HIGH			BIT(7)
#define CFG_PIN					0x06
#define CFG_PIN_EN_CTRL_MASK			0x60
#define CFG_PIN_USB_MODE_CTRL			BIT(4)
#define CFG_PIN_EN_CTRL_ACTIVE_HIGH		0x40
#define CFG_PIN_EN_CTRL_ACTIVE_LOW		0x60
#define CFG_PIN_EN_APSD_IRQ			BIT(1)
#define CFG_PIN_EN_CHARGER_ERROR		BIT(2)
#define CFG_THERM				0x07
#define CFG_THERM_SOFT_HOT_COMPENSATION_MASK	0x03
#define CFG_THERM_SOFT_HOT_COMPENSATION_SHIFT	0
#define CFG_THERM_SOFT_COLD_COMPENSATION_MASK	0x0c
#define CFG_THERM_SOFT_COLD_COMPENSATION_SHIFT	2
#define CFG_THERM_MONITOR_DISABLED		BIT(4)
#define CFG_SYSOK				0x08
#define CFG_SYSOK_SUSPEND_HARD_LIMIT_DISABLED	BIT(2)
#define CFG_OTHER				0x09
#define CFG_OTHER_RID_MASK			0xc0
#define CFG_OTHER_RID_ENABLED_AUTO_OTG		0xc0
#define CFG_OTG					0x0a
#define CFG_OTG_TEMP_THRESHOLD_MASK		0x30
#define CFG_OTG_TEMP_THRESHOLD_SHIFT		4
#define CFG_OTG_CC_COMPENSATION_MASK		0xc0
#define CFG_OTG_CC_COMPENSATION_SHIFT		6
#define CFG_TEMP_LIMIT				0x0b
#define CFG_TEMP_LIMIT_SOFT_HOT_MASK		0x03
#define CFG_TEMP_LIMIT_SOFT_HOT_SHIFT		0
#define CFG_TEMP_LIMIT_SOFT_COLD_MASK		0x0c
#define CFG_TEMP_LIMIT_SOFT_COLD_SHIFT		2
#define CFG_TEMP_LIMIT_HARD_HOT_MASK		0x30
#define CFG_TEMP_LIMIT_HARD_HOT_SHIFT		4
#define CFG_TEMP_LIMIT_HARD_COLD_MASK		0xc0
#define CFG_TEMP_LIMIT_HARD_COLD_SHIFT		6
#define CFG_FAULT_IRQ				0x0c
#define CFG_FAULT_IRQ_DCIN_UV			BIT(2)
#define CFG_STATUS_IRQ				0x0d
#define CFG_STATUS_IRQ_TERMINATION_OR_TAPER	BIT(4)
#define CFG_STATUS_IRQ_CHARGE_TIMEOUT		BIT(7)
#define CFG_ADDRESS				0x0e

/* Command registers */
#define CMD_A					0x30
#define CMD_A_CHG_ENABLED			BIT(1)
#define CMD_A_SUSPEND_ENABLED			BIT(2)
#define CMD_A_OTG_ENABLE			BIT(4)
#define CMD_A_ALLOW_WRITE			BIT(7)
#define CMD_B					0x31
#define CMD_B_POR				BIT(7)
#define CMD_B_USB59_MODE			BIT(1)
#define CMD_B_HC_MODE				BIT(0)
#define CMD_C					0x33

/* Interrupt Status registers */
#define IRQSTAT_A				0x35
#define IRQSTAT_C				0x37
#define IRQSTAT_C_TERMINATION_STAT		BIT(0)
#define IRQSTAT_C_TERMINATION_IRQ		BIT(1)
#define IRQSTAT_C_TAPER_IRQ			BIT(3)
#define IRQSTAT_D				0x38
#define IRQSTAT_D_CHARGE_TIMEOUT_STAT		BIT(2)
#define IRQSTAT_D_CHARGE_TIMEOUT_IRQ		BIT(3)
#define IRQSTAT_E				0x39
#define IRQSTAT_E_USBIN_UV_STAT			BIT(0)
#define IRQSTAT_E_USBIN_UV_IRQ			BIT(1)
#define IRQSTAT_E_DCIN_UV_STAT			BIT(4)
#define IRQSTAT_E_DCIN_UV_IRQ			BIT(5)
#define IRQSTAT_F				0x3a

/* Status registers */
#define STAT_A					0x3b
#define STAT_A_FLOAT_VOLTAGE_MASK		0x3f
#define STAT_B					0x3c
#define STAT_C					0x3d
#define STAT_C_CHG_ENABLED			BIT(0)
<<<<<<< HEAD
#define STAT_C_CHG_STATUS			BIT(5)
=======
#define STAT_C_HOLDOFF_STAT			BIT(3)
>>>>>>> v3.10.88
#define STAT_C_CHG_MASK				0x06
#define STAT_C_CHG_SHIFT			1
#define STAT_C_CHG_TERM				BIT(5)
#define STAT_C_CHARGER_ERROR			BIT(6)
#define STAT_E					0x3f

#define SMB347_MAX_REGISTER			0x3f

/**
 * struct smb347_charger - smb347 charger instance
 * @lock: protects concurrent access to online variables
 * @dev: pointer to device
 * @regmap: pointer to driver regmap
 * @mains: power_supply instance for AC/DC power
 * @usb: power_supply instance for USB power
 * @battery: power_supply instance for battery
 * @mains_online: is AC/DC input connected
 * @usb_online: is USB input connected
 * @charging_enabled: is charging enabled
 * @pdata: pointer to platform data
 */
struct smb347_charger {
	struct mutex		lock;
	struct device		*dev;
	struct regmap		*regmap;
	struct power_supply	mains;
	struct power_supply	usb;
	struct power_supply	battery;
	bool			mains_online;
	bool			usb_online;
	bool			charging_enabled;
<<<<<<< HEAD
	unsigned int		mains_current_limit;
	bool			usb_hc_mode;
	bool			usb_otg_enabled;
	bool			is_fully_charged;
	int			en_gpio;
	struct dentry		*dentry;
=======
>>>>>>> v3.10.88
	const struct smb347_charger_platform_data *pdata;
};

/* Fast charge current in uA */
static const unsigned int fcc_tbl[] = {
	700000,
	900000,
	1200000,
	1500000,
	1800000,
	2000000,
	2200000,
	2500000,
};

/* Pre-charge current in uA */
static const unsigned int pcc_tbl[] = {
	100000,
	150000,
	200000,
	250000,
};

/* Termination current in uA */
static const unsigned int tc_tbl[] = {
	37500,
	50000,
	100000,
	150000,
	200000,
	250000,
	500000,
	600000,
};

/* Input current limit in uA */
static const unsigned int icl_tbl[] = {
	300000,
	500000,
	700000,
	900000,
	1200000,
	1500000,
	1800000,
	2000000,
	2200000,
	2500000,
};

/* Charge current compensation in uA */
static const unsigned int ccc_tbl[] = {
	250000,
	700000,
	900000,
	1200000,
};

/* Convert register value to current using lookup table */
static int hw_to_current(const unsigned int *tbl, size_t size, unsigned int val)
{
	if (val >= size)
		return -EINVAL;
	return tbl[val];
}

/* Convert current to register value using lookup table */
static int current_to_hw(const unsigned int *tbl, size_t size, unsigned int val)
{
	size_t i;

	for (i = 0; i < size; i++)
		if (val < tbl[i])
			break;
	return i > 0 ? i - 1 : -EINVAL;
}

/**
 * smb347_update_ps_status - refreshes the power source status
 * @smb: pointer to smb347 charger instance
 *
 * Function checks whether any power source is connected to the charger and
 * updates internal state accordingly. If there is a change to previous state
 * function returns %1, otherwise %0 and negative errno in case of errror.
 */
static int smb347_update_ps_status(struct smb347_charger *smb)
{
	bool usb = false;
	bool dc = false;
	unsigned int val;
	int ret;

	ret = regmap_read(smb->regmap, IRQSTAT_E, &val);
	if (ret < 0)
		return ret;

	/*
	 * Dc and usb are set depending on whether they are enabled in
	 * platform data _and_ whether corresponding undervoltage is set.
	 */
	if (smb->pdata->use_mains)
		dc = !(val & IRQSTAT_E_DCIN_UV_STAT);
	if (smb->pdata->use_usb)
		usb = !(val & IRQSTAT_E_USBIN_UV_STAT);

	mutex_lock(&smb->lock);
	ret = smb->mains_online != dc || smb->usb_online != usb;
	smb->mains_online = dc;
	smb->usb_online = usb;
	mutex_unlock(&smb->lock);

	return ret;
}

/*
 * smb347_is_ps_online - returns whether input power source is connected
 * @smb: pointer to smb347 charger instance
 *
 * Returns %true if input power source is connected. Note that this is
 * dependent on what platform has configured for usable power sources. For
 * example if USB is disabled, this will return %false even if the USB cable
 * is connected.
 */
static bool smb347_is_ps_online(struct smb347_charger *smb)
{
	bool ret;

	mutex_lock(&smb->lock);
	ret = smb->usb_online || smb->mains_online;
	mutex_unlock(&smb->lock);

	return ret;
}

/**
 * smb347_charging_status - returns status of charging
 * @smb: pointer to smb347 charger instance
 *
 * Function returns charging status. %0 means no charging is in progress,
 * %1 means pre-charging, %2 fast-charging and %3 taper-charging.
 */
static int smb347_charging_status(struct smb347_charger *smb)
{
	unsigned int val;
	int ret;

	if (!smb347_is_ps_online(smb))
		return 0;

	ret = regmap_read(smb->regmap, STAT_C, &val);
	if (ret < 0)
		return 0;

	return (val & STAT_C_CHG_MASK) >> STAT_C_CHG_SHIFT;
}

static int smb347_charging_set(struct smb347_charger *smb, bool enable)
{
	int ret = 0;

	if (enable && !smb->charging_enabled)
		smb->is_fully_charged = false;

	if (smb->pdata->enable_control != SMB347_CHG_ENABLE_SW) {
<<<<<<< HEAD
		smb->charging_enabled = enable;

		if (smb->en_gpio)
			gpio_set_value(
				smb->en_gpio,
				(smb->pdata->enable_control ==
				 SMB347_CHG_ENABLE_PIN_ACTIVE_LOW) ^ enable);
=======
		dev_dbg(smb->dev, "charging enable/disable in SW disabled\n");
>>>>>>> v3.10.88
		return 0;
	}

	mutex_lock(&smb->lock);
	if (smb->charging_enabled != enable) {
		ret = regmap_update_bits(smb->regmap, CMD_A, CMD_A_CHG_ENABLED,
					 enable ? CMD_A_CHG_ENABLED : 0);
		if (!ret)
			smb->charging_enabled = enable;
	}
	mutex_unlock(&smb->lock);
	return ret;
}

static inline int smb347_charging_enable(struct smb347_charger *smb)
{
	return smb347_charging_set(smb, true);
}

static inline int smb347_charging_disable(struct smb347_charger *smb)
{
	return smb347_charging_set(smb, false);
}

static int smb347_start_stop_charging(struct smb347_charger *smb)
{
	int ret;

	/*
	 * Depending on whether valid power source is connected or not, we
	 * disable or enable the charging. We do it manually because it
	 * depends on how the platform has configured the valid inputs.
	 */
	if (smb347_is_ps_online(smb)) {
		ret = smb347_charging_enable(smb);
		if (ret < 0)
			dev_err(smb->dev, "failed to enable charging\n");
	} else {
		ret = smb347_charging_disable(smb);
		if (ret < 0)
			dev_err(smb->dev, "failed to disable charging\n");
	}

	return ret;
}

static int smb347_set_charge_current(struct smb347_charger *smb)
{
	int ret;

	if (smb->pdata->max_charge_current) {
		ret = current_to_hw(fcc_tbl, ARRAY_SIZE(fcc_tbl),
				    smb->pdata->max_charge_current);
		if (ret < 0)
			return ret;

		ret = regmap_update_bits(smb->regmap, CFG_CHARGE_CURRENT,
					 CFG_CHARGE_CURRENT_FCC_MASK,
					 ret << CFG_CHARGE_CURRENT_FCC_SHIFT);
		if (ret < 0)
			return ret;
	}

	if (smb->pdata->pre_charge_current) {
		ret = current_to_hw(pcc_tbl, ARRAY_SIZE(pcc_tbl),
				    smb->pdata->pre_charge_current);
		if (ret < 0)
			return ret;

		ret = regmap_update_bits(smb->regmap, CFG_CHARGE_CURRENT,
					 CFG_CHARGE_CURRENT_PCC_MASK,
					 ret << CFG_CHARGE_CURRENT_PCC_SHIFT);
		if (ret < 0)
			return ret;
	}

	if (smb->pdata->termination_current) {
		ret = current_to_hw(tc_tbl, ARRAY_SIZE(tc_tbl),
				    smb->pdata->termination_current);
		if (ret < 0)
			return ret;

		ret = regmap_update_bits(smb->regmap, CFG_CHARGE_CURRENT,
					 CFG_CHARGE_CURRENT_TC_MASK, ret);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int smb347_set_current_limits(struct smb347_charger *smb)
{
	int ret;

<<<<<<< HEAD
	if (smb->mains_current_limit) {
		val = current_to_hw(icl_tbl, ARRAY_SIZE(icl_tbl),
				    smb->mains_current_limit);
		if (val < 0)
			return val;
=======
	if (smb->pdata->mains_current_limit) {
		ret = current_to_hw(icl_tbl, ARRAY_SIZE(icl_tbl),
				    smb->pdata->mains_current_limit);
		if (ret < 0)
			return ret;
>>>>>>> v3.10.88

		ret = regmap_update_bits(smb->regmap, CFG_CURRENT_LIMIT,
					 CFG_CURRENT_LIMIT_DC_MASK,
					 ret << CFG_CURRENT_LIMIT_DC_SHIFT);
		if (ret < 0)
			return ret;
	}

	if (smb->pdata->usb_hc_current_limit) {
		ret = current_to_hw(icl_tbl, ARRAY_SIZE(icl_tbl),
				    smb->pdata->usb_hc_current_limit);
		if (ret < 0)
			return ret;

		ret = regmap_update_bits(smb->regmap, CFG_CURRENT_LIMIT,
					 CFG_CURRENT_LIMIT_USB_MASK, ret);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int smb347_set_voltage_limits(struct smb347_charger *smb)
{
	int ret;

	if (smb->pdata->pre_to_fast_voltage) {
		ret = smb->pdata->pre_to_fast_voltage;

		/* uV */
		ret = clamp_val(ret, 2400000, 3000000) - 2400000;
		ret /= 200000;

		ret = regmap_update_bits(smb->regmap, CFG_FLOAT_VOLTAGE,
				CFG_FLOAT_VOLTAGE_THRESHOLD_MASK,
				ret << CFG_FLOAT_VOLTAGE_THRESHOLD_SHIFT);
		if (ret < 0)
			return ret;
	}

	if (smb->pdata->max_charge_voltage) {
		ret = smb->pdata->max_charge_voltage;

		/* uV */
		ret = clamp_val(ret, 3500000, 4500000) - 3500000;
		ret /= 20000;

<<<<<<< HEAD
		ret &= ~CFG_FLOAT_VOLTAGE_MASK;
		ret |= val;
=======
		ret = regmap_update_bits(smb->regmap, CFG_FLOAT_VOLTAGE,
					 CFG_FLOAT_VOLTAGE_FLOAT_MASK, ret);
		if (ret < 0)
			return ret;
>>>>>>> v3.10.88
	}

	return 0;
}

static int smb347_set_temp_limits(struct smb347_charger *smb)
{
	bool enable_therm_monitor = false;
	int ret = 0;
	int val;

	if (smb->pdata->chip_temp_threshold) {
		val = smb->pdata->chip_temp_threshold;

		/* degree C */
		val = clamp_val(val, 100, 130) - 100;
		val /= 10;

		ret = regmap_update_bits(smb->regmap, CFG_OTG,
					 CFG_OTG_TEMP_THRESHOLD_MASK,
					 val << CFG_OTG_TEMP_THRESHOLD_SHIFT);
		if (ret < 0)
			return ret;
	}

	if (smb->pdata->soft_cold_temp_limit != SMB347_TEMP_USE_DEFAULT) {
		val = smb->pdata->soft_cold_temp_limit;

		val = clamp_val(val, 0, 15);
		val /= 5;
		/* this goes from higher to lower so invert the value */
		val = ~val & 0x3;

		ret = regmap_update_bits(smb->regmap, CFG_TEMP_LIMIT,
					 CFG_TEMP_LIMIT_SOFT_COLD_MASK,
					 val << CFG_TEMP_LIMIT_SOFT_COLD_SHIFT);
		if (ret < 0)
			return ret;

		enable_therm_monitor = true;
	}

	if (smb->pdata->soft_hot_temp_limit != SMB347_TEMP_USE_DEFAULT) {
		val = smb->pdata->soft_hot_temp_limit;

		val = clamp_val(val, 40, 55) - 40;
		val /= 5;

		ret = regmap_update_bits(smb->regmap, CFG_TEMP_LIMIT,
					 CFG_TEMP_LIMIT_SOFT_HOT_MASK,
					 val << CFG_TEMP_LIMIT_SOFT_HOT_SHIFT);
		if (ret < 0)
			return ret;

		enable_therm_monitor = true;
	}

	if (smb->pdata->hard_cold_temp_limit != SMB347_TEMP_USE_DEFAULT) {
		val = smb->pdata->hard_cold_temp_limit;

		val = clamp_val(val, -5, 10) + 5;
		val /= 5;
		/* this goes from higher to lower so invert the value */
		val = ~val & 0x3;

		ret = regmap_update_bits(smb->regmap, CFG_TEMP_LIMIT,
					 CFG_TEMP_LIMIT_HARD_COLD_MASK,
					 val << CFG_TEMP_LIMIT_HARD_COLD_SHIFT);
		if (ret < 0)
			return ret;

		enable_therm_monitor = true;
	}

	if (smb->pdata->hard_hot_temp_limit != SMB347_TEMP_USE_DEFAULT) {
		val = smb->pdata->hard_hot_temp_limit;

		val = clamp_val(val, 50, 65) - 50;
		val /= 5;

		ret = regmap_update_bits(smb->regmap, CFG_TEMP_LIMIT,
					 CFG_TEMP_LIMIT_HARD_HOT_MASK,
					 val << CFG_TEMP_LIMIT_HARD_HOT_SHIFT);
		if (ret < 0)
			return ret;

		enable_therm_monitor = true;
	}

	/*
	 * If any of the temperature limits are set, we also enable the
	 * thermistor monitoring.
	 *
	 * When soft limits are hit, the device will start to compensate
	 * current and/or voltage depending on the configuration.
	 *
	 * When hard limit is hit, the device will suspend charging
	 * depending on the configuration.
	 */
	if (enable_therm_monitor) {
		ret = regmap_update_bits(smb->regmap, CFG_THERM,
					 CFG_THERM_MONITOR_DISABLED, 0);
		if (ret < 0)
			return ret;
	}

	if (smb->pdata->suspend_on_hard_temp_limit) {
		ret = regmap_update_bits(smb->regmap, CFG_SYSOK,
				 CFG_SYSOK_SUSPEND_HARD_LIMIT_DISABLED, 0);
		if (ret < 0)
			return ret;
	}

	if (smb->pdata->soft_temp_limit_compensation !=
	    SMB347_SOFT_TEMP_COMPENSATE_DEFAULT) {
		val = smb->pdata->soft_temp_limit_compensation & 0x3;

		ret = regmap_update_bits(smb->regmap, CFG_THERM,
				 CFG_THERM_SOFT_HOT_COMPENSATION_MASK,
				 val << CFG_THERM_SOFT_HOT_COMPENSATION_SHIFT);
		if (ret < 0)
			return ret;

		ret = regmap_update_bits(smb->regmap, CFG_THERM,
				 CFG_THERM_SOFT_COLD_COMPENSATION_MASK,
				 val << CFG_THERM_SOFT_COLD_COMPENSATION_SHIFT);
		if (ret < 0)
			return ret;
	}

	if (smb->pdata->charge_current_compensation) {
		val = current_to_hw(ccc_tbl, ARRAY_SIZE(ccc_tbl),
				    smb->pdata->charge_current_compensation);
		if (val < 0)
			return val;

		ret = regmap_update_bits(smb->regmap, CFG_OTG,
				CFG_OTG_CC_COMPENSATION_MASK,
				(val & 0x3) << CFG_OTG_CC_COMPENSATION_SHIFT);
		if (ret < 0)
			return ret;
	}

	return ret;
}

/*
 * smb347_set_writable - enables/disables writing to non-volatile registers
 * @smb: pointer to smb347 charger instance
 *
 * You can enable/disable writing to the non-volatile configuration
 * registers by calling this function.
 *
 * Returns %0 on success and negative errno in case of failure.
 */
static int smb347_set_writable(struct smb347_charger *smb, bool writable)
{
	return regmap_update_bits(smb->regmap, CMD_A, CMD_A_ALLOW_WRITE,
				  writable ? CMD_A_ALLOW_WRITE : 0);
}

static int smb347_irq_set(struct smb347_charger *smb, bool enable)
{
	int ret;

	ret = smb347_set_writable(smb, true);
	if (ret < 0)
		return ret;

	/*
	 * Enable/disable interrupts for:
	 *	- under voltage
	 *	- termination current reached
	 *	- charger error
	 */
	if (enable) {
		ret = smb347_write(smb, CFG_FAULT_IRQ, CFG_FAULT_IRQ_DCIN_UV);
		if (ret < 0)
			goto fail;

		ret = smb347_write(smb, CFG_STATUS_IRQ,
				   CFG_STATUS_IRQ_TERMINATION_OR_TAPER);
		if (ret < 0)
			goto fail;

		ret = smb347_read(smb, CFG_PIN);
		if (ret < 0)
			goto fail;

		ret |= CFG_PIN_EN_CHARGER_ERROR;

		ret = smb347_write(smb, CFG_PIN, ret);
	} else {
		ret = smb347_write(smb, CFG_FAULT_IRQ, 0);
		if (ret < 0)
			goto fail;

		ret = smb347_write(smb, CFG_STATUS_IRQ, 0);
		if (ret < 0)
			goto fail;

		ret = smb347_read(smb, CFG_PIN);
		if (ret < 0)
			goto fail;

		ret &= ~CFG_PIN_EN_CHARGER_ERROR;

		ret = smb347_write(smb, CFG_PIN, ret);
	}

fail:
	smb347_set_writable(smb, false);
	return ret;
}

static inline int smb347_irq_enable(struct smb347_charger *smb)
{
	return smb347_irq_set(smb, true);
}

static inline int smb347_irq_disable(struct smb347_charger *smb)
{
	return smb347_irq_set(smb, false);
}

static irqreturn_t smb347_interrupt(int irq, void *data)
{
	struct smb347_charger *smb = data;
	int stat_c, t;
	u8 irqstat[6];
	irqreturn_t ret = IRQ_NONE;

	t = i2c_smbus_read_i2c_block_data(smb->client, IRQSTAT_A, 6, irqstat);
	if (t < 0) {
		dev_warn(&smb->client->dev,
			 "reading IRQSTAT registers failed\n");
		return IRQ_NONE;
	}

	stat_c = smb347_read(smb, STAT_C);
	if (stat_c < 0) {
		dev_warn(&smb->client->dev, "reading STAT_C failed\n");
		return IRQ_NONE;
	}

	pr_debug("%s: stat c=%x irq a=%x b=%x c=%x d=%x e=%x f=%x\n",
		 __func__, stat_c, irqstat[0], irqstat[1], irqstat[2],
		 irqstat[3], irqstat[4], irqstat[5]);

	/*
	 * If we get charger error we report the error back to user and
	 * disable charging.
	 */
	if (stat_c & STAT_C_CHARGER_ERROR) {
		dev_err(&smb->client->dev,
			"error in charger, disabling charging\n");

		smb347_charging_disable(smb);
		power_supply_changed(&smb->battery);

		ret = IRQ_HANDLED;
	} else if (((stat_c & STAT_C_CHG_STATUS) ||
		    (irqstat[2] & (IRQSTAT_C_TERMINATION_IRQ |
				   IRQSTAT_C_TERMINATION_STAT))) &&
		   !smb->is_fully_charged) {
		dev_info(&smb->client->dev, "charge terminated\n");
		smb->is_fully_charged = true;
		smb347_charging_disable(smb);
		power_supply_changed(&smb->battery);
		ret = IRQ_HANDLED;
	}

	if (irqstat[2] & IRQSTAT_C_TAPER_IRQ)
		ret = IRQ_HANDLED;

	/*
	 * If we got an under voltage interrupt it means that AC/USB input
	 * was disconnected.
	 */
	if (irqstat[4] & (IRQSTAT_E_USBIN_UV_IRQ | IRQSTAT_E_DCIN_UV_IRQ))
		ret = IRQ_HANDLED;

	if (smb347_update_status(smb) > 0) {
		smb347_update_online(smb);
		power_supply_changed(&smb->mains);
		power_supply_changed(&smb->usb);
		ret = IRQ_HANDLED;
	}

	return ret;
}

static int smb347_irq_init(struct smb347_charger *smb)
{
	const struct smb347_charger_platform_data *pdata = smb->pdata;
	int ret, irq = gpio_to_irq(pdata->irq_gpio);

	ret = gpio_request_one(pdata->irq_gpio, GPIOF_IN, smb->client->name);
	if (ret < 0)
		goto fail;

	ret = request_threaded_irq(irq, NULL, smb347_interrupt,
				   pdata->disable_stat_interrupts ?
				   IRQF_TRIGGER_RISING | IRQF_ONESHOT :
				   IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				   smb->client->name, smb);
	if (ret < 0)
		goto fail_gpio;

	ret = enable_irq_wake(irq);
	if (ret)
		pr_err("%s: failed to enable wake on irq %d\n", __func__, irq);

	smb->client->irq = irq;
	return 0;

fail_gpio:
	gpio_free(pdata->irq_gpio);
fail:
	smb->client->irq = 0;
	return ret;
}

static int smb347_hw_init(struct smb347_charger *smb)
{
	unsigned int val;
	int ret;

	ret = smb347_set_writable(smb, true);
	if (ret < 0)
		return ret;

	/*
	 * Program the platform specific configuration values to the device
	 * first.
	 */
	ret = smb347_set_charge_current(smb);
	if (ret < 0)
		goto fail;

	ret = smb347_set_current_limits(smb);
	if (ret < 0)
		goto fail;

	ret = smb347_set_voltage_limits(smb);
	if (ret < 0)
		goto fail;

// HACK for Manta pre-alpha 0.2, TH_BATTERY not connected properly
#if 0 // HACK
	ret = smb347_set_temp_limits(smb);
	if (ret < 0)
		goto fail;
#endif // HACK

	/* If USB charging is disabled we put the USB in suspend mode */
	if (!smb->pdata->use_usb) {
		ret = regmap_update_bits(smb->regmap, CMD_A,
					 CMD_A_SUSPEND_ENABLED,
					 CMD_A_SUSPEND_ENABLED);
		if (ret < 0)
			goto fail;
	}

	/*
	 * If configured by platform data, we enable hardware Auto-OTG
	 * support for driving VBUS. Otherwise we disable it.
	 */
<<<<<<< HEAD
	ret &= ~CFG_OTHER_RID_MASK;
	if (smb->pdata->use_usb_otg)
		ret |= CFG_OTHER_RID_ENABLED_AUTO_OTG;

	ret = smb347_write(smb, CFG_OTHER, ret);
	if (ret < 0)
		goto fail;

	/* If configured by platform data, disable AUTOMATIC RECHARGE */
	if (smb->pdata->disable_automatic_recharge) {
		ret = smb347_read(smb, CFG_CHARGE_CONTROL);
		if (ret < 0)
			goto fail;

		ret |= CFG_AUTOMATIC_RECHARGE_DISABLE;

		ret = smb347_write(smb, CFG_CHARGE_CONTROL, ret);
		if (ret < 0)
			goto fail;
	}

	ret = smb347_read(smb, CFG_PIN);
=======
	ret = regmap_update_bits(smb->regmap, CFG_OTHER, CFG_OTHER_RID_MASK,
		smb->pdata->use_usb_otg ? CFG_OTHER_RID_ENABLED_AUTO_OTG : 0);
>>>>>>> v3.10.88
	if (ret < 0)
		goto fail;

	/*
	 * Make the charging functionality controllable by a write to the
	 * command register unless pin control is specified in the platform
	 * data.
	 */
<<<<<<< HEAD
	ret &= ~(CFG_PIN_EN_CTRL_MASK | CFG_PIN_USB_MODE_CTRL);

=======
>>>>>>> v3.10.88
	switch (smb->pdata->enable_control) {
	case SMB347_CHG_ENABLE_PIN_ACTIVE_LOW:
		val = CFG_PIN_EN_CTRL_ACTIVE_LOW;
		break;
	case SMB347_CHG_ENABLE_PIN_ACTIVE_HIGH:
		val = CFG_PIN_EN_CTRL_ACTIVE_HIGH;
		break;
	default:
		val = 0;
		break;
	}

<<<<<<< HEAD
	if (smb->pdata->usb_mode_pin_ctrl)
		ret |= CFG_PIN_USB_MODE_CTRL;

	/* Disable Automatic Power Source Detection (APSD) interrupt. */
	ret &= ~CFG_PIN_EN_APSD_IRQ;
=======
	ret = regmap_update_bits(smb->regmap, CFG_PIN, CFG_PIN_EN_CTRL_MASK,
				 val);
	if (ret < 0)
		goto fail;
>>>>>>> v3.10.88

	/* Disable Automatic Power Source Detection (APSD) interrupt. */
	ret = regmap_update_bits(smb->regmap, CFG_PIN, CFG_PIN_EN_APSD_IRQ, 0);
	if (ret < 0)
		goto fail;

	ret = smb347_update_ps_status(smb);
	if (ret < 0)
		goto fail;

	ret = smb347_start_stop_charging(smb);

<<<<<<< HEAD
	if ((smb->pdata->irq_gpio >= 0) &&
	    !smb->pdata->disable_stat_interrupts) {
		/*
		 * Configure the STAT output to be suitable for interrupts:
		 * disable all other output (except interrupts) and make it
		 * active low.
		 */
		ret = smb347_read(smb, CFG_STAT);
		if (ret < 0)
			goto fail;

		ret &= ~CFG_STAT_ACTIVE_HIGH;
		ret |= CFG_STAT_DISABLED;

		ret = smb347_write(smb, CFG_STAT, ret);
		if (ret < 0)
			goto fail;

		ret = smb347_irq_enable(smb);
		if (ret < 0)
			goto fail;
	}
=======
fail:
	smb347_set_writable(smb, false);
	return ret;
}

static irqreturn_t smb347_interrupt(int irq, void *data)
{
	struct smb347_charger *smb = data;
	unsigned int stat_c, irqstat_c, irqstat_d, irqstat_e;
	bool handled = false;
	int ret;

	ret = regmap_read(smb->regmap, STAT_C, &stat_c);
	if (ret < 0) {
		dev_warn(smb->dev, "reading STAT_C failed\n");
		return IRQ_NONE;
	}

	ret = regmap_read(smb->regmap, IRQSTAT_C, &irqstat_c);
	if (ret < 0) {
		dev_warn(smb->dev, "reading IRQSTAT_C failed\n");
		return IRQ_NONE;
	}

	ret = regmap_read(smb->regmap, IRQSTAT_D, &irqstat_d);
	if (ret < 0) {
		dev_warn(smb->dev, "reading IRQSTAT_D failed\n");
		return IRQ_NONE;
	}

	ret = regmap_read(smb->regmap, IRQSTAT_E, &irqstat_e);
	if (ret < 0) {
		dev_warn(smb->dev, "reading IRQSTAT_E failed\n");
		return IRQ_NONE;
	}

	/*
	 * If we get charger error we report the error back to user.
	 * If the error is recovered charging will resume again.
	 */
	if (stat_c & STAT_C_CHARGER_ERROR) {
		dev_err(smb->dev, "charging stopped due to charger error\n");
		power_supply_changed(&smb->battery);
		handled = true;
	}

	/*
	 * If we reached the termination current the battery is charged and
	 * we can update the status now. Charging is automatically
	 * disabled by the hardware.
	 */
	if (irqstat_c & (IRQSTAT_C_TERMINATION_IRQ | IRQSTAT_C_TAPER_IRQ)) {
		if (irqstat_c & IRQSTAT_C_TERMINATION_STAT)
			power_supply_changed(&smb->battery);
		dev_dbg(smb->dev, "going to HW maintenance mode\n");
		handled = true;
	}

	/*
	 * If we got a charger timeout INT that means the charge
	 * full is not detected with in charge timeout value.
	 */
	if (irqstat_d & IRQSTAT_D_CHARGE_TIMEOUT_IRQ) {
		dev_dbg(smb->dev, "total Charge Timeout INT received\n");

		if (irqstat_d & IRQSTAT_D_CHARGE_TIMEOUT_STAT)
			dev_warn(smb->dev, "charging stopped due to timeout\n");
		power_supply_changed(&smb->battery);
		handled = true;
	}

	/*
	 * If we got an under voltage interrupt it means that AC/USB input
	 * was connected or disconnected.
	 */
	if (irqstat_e & (IRQSTAT_E_USBIN_UV_IRQ | IRQSTAT_E_DCIN_UV_IRQ)) {
		if (smb347_update_ps_status(smb) > 0) {
			smb347_start_stop_charging(smb);
			if (smb->pdata->use_mains)
				power_supply_changed(&smb->mains);
			if (smb->pdata->use_usb)
				power_supply_changed(&smb->usb);
		}
		handled = true;
	}

	return handled ? IRQ_HANDLED : IRQ_NONE;
}

static int smb347_irq_set(struct smb347_charger *smb, bool enable)
{
	int ret;

	ret = smb347_set_writable(smb, true);
	if (ret < 0)
		return ret;

	/*
	 * Enable/disable interrupts for:
	 *	- under voltage
	 *	- termination current reached
	 *	- charger timeout
	 *	- charger error
	 */
	ret = regmap_update_bits(smb->regmap, CFG_FAULT_IRQ, 0xff,
				 enable ? CFG_FAULT_IRQ_DCIN_UV : 0);
	if (ret < 0)
		goto fail;

	ret = regmap_update_bits(smb->regmap, CFG_STATUS_IRQ, 0xff,
			enable ? (CFG_STATUS_IRQ_TERMINATION_OR_TAPER |
					CFG_STATUS_IRQ_CHARGE_TIMEOUT) : 0);
	if (ret < 0)
		goto fail;
>>>>>>> v3.10.88

	ret = regmap_update_bits(smb->regmap, CFG_PIN, CFG_PIN_EN_CHARGER_ERROR,
				 enable ? CFG_PIN_EN_CHARGER_ERROR : 0);
fail:
	smb347_set_writable(smb, false);
	return ret;
}

static int smb347_mains_get_property(struct power_supply *psy,
				     enum power_supply_property prop,
				     union power_supply_propval *val)
{
	struct smb347_charger *smb =
		container_of(psy, struct smb347_charger, mains);

	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = smb->mains_online;
		return 0;

	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = smb->mains_current_limit;
		return 0;

	default:
		return -EINVAL;
	}
	return -EINVAL;
}

<<<<<<< HEAD
static int smb347_mains_set_property(struct power_supply *psy,
				     enum power_supply_property prop,
				     const union power_supply_propval *val)
=======
static int smb347_irq_init(struct smb347_charger *smb,
			   struct i2c_client *client)
>>>>>>> v3.10.88
{
	struct smb347_charger *smb =
		container_of(psy, struct smb347_charger, mains);
	int ret;
	bool oldval;

<<<<<<< HEAD
	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		oldval = smb->mains_online;

		smb->mains_online = val->intval;
=======
	ret = gpio_request_one(pdata->irq_gpio, GPIOF_IN, client->name);
	if (ret < 0)
		goto fail;

	ret = request_threaded_irq(irq, NULL, smb347_interrupt,
				   IRQF_TRIGGER_FALLING, client->name, smb);
	if (ret < 0)
		goto fail_gpio;
>>>>>>> v3.10.88

		smb347_set_writable(smb, true);

<<<<<<< HEAD
		ret = smb347_read(smb, CMD_A);
		if (ret < 0)
			return -EINVAL;

		ret &= ~CMD_A_SUSPEND_ENABLED;
		if (val->intval)
			ret |= CMD_A_SUSPEND_ENABLED;

		ret = smb347_write(smb, CMD_A, ret);

		smb347_hw_init(smb);

		smb347_set_writable(smb, false);

		if (smb->mains_online != oldval)
			power_supply_changed(psy);
		return 0;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		smb->mains_current_limit = val->intval;
		smb347_hw_init(smb);
		return 0;

	default:
		return -EINVAL;
	}

	return -EINVAL;
}

static int smb347_mains_property_is_writeable(struct power_supply *psy,
					     enum power_supply_property prop)
{
	switch (prop) {
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		return 1;
	default:
		break;
=======
	/*
	 * Configure the STAT output to be suitable for interrupts: disable
	 * all other output (except interrupts) and make it active low.
	 */
	ret = regmap_update_bits(smb->regmap, CFG_STAT,
				 CFG_STAT_ACTIVE_HIGH | CFG_STAT_DISABLED,
				 CFG_STAT_DISABLED);
	if (ret < 0)
		goto fail_readonly;

	smb347_set_writable(smb, false);
	client->irq = irq;
	return 0;

fail_readonly:
	smb347_set_writable(smb, false);
fail_irq:
	free_irq(irq, smb);
fail_gpio:
	gpio_free(pdata->irq_gpio);
fail:
	client->irq = 0;
	return ret;
}

/*
 * Returns the constant charge current programmed
 * into the charger in uA.
 */
static int get_const_charge_current(struct smb347_charger *smb)
{
	int ret, intval;
	unsigned int v;

	if (!smb347_is_ps_online(smb))
		return -ENODATA;

	ret = regmap_read(smb->regmap, STAT_B, &v);
	if (ret < 0)
		return ret;

	/*
	 * The current value is composition of FCC and PCC values
	 * and we can detect which table to use from bit 5.
	 */
	if (v & 0x20) {
		intval = hw_to_current(fcc_tbl, ARRAY_SIZE(fcc_tbl), v & 7);
	} else {
		v >>= 3;
		intval = hw_to_current(pcc_tbl, ARRAY_SIZE(pcc_tbl), v & 7);
	}

	return intval;
}

/*
 * Returns the constant charge voltage programmed
 * into the charger in uV.
 */
static int get_const_charge_voltage(struct smb347_charger *smb)
{
	int ret, intval;
	unsigned int v;

	if (!smb347_is_ps_online(smb))
		return -ENODATA;

	ret = regmap_read(smb->regmap, STAT_A, &v);
	if (ret < 0)
		return ret;

	v &= STAT_A_FLOAT_VOLTAGE_MASK;
	if (v > 0x3d)
		v = 0x3d;

	intval = 3500000 + v * 20000;

	return intval;
}

static int smb347_mains_get_property(struct power_supply *psy,
				     enum power_supply_property prop,
				     union power_supply_propval *val)
{
	struct smb347_charger *smb =
		container_of(psy, struct smb347_charger, mains);
	int ret;

	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = smb->mains_online;
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		ret = get_const_charge_voltage(smb);
		if (ret < 0)
			return ret;
		else
			val->intval = ret;
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = get_const_charge_current(smb);
		if (ret < 0)
			return ret;
		else
			val->intval = ret;
		break;

	default:
		return -EINVAL;
>>>>>>> v3.10.88
	}

	return 0;
}

static enum power_supply_property smb347_mains_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
<<<<<<< HEAD
	POWER_SUPPLY_PROP_CURRENT_MAX,
=======
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
>>>>>>> v3.10.88
};

static int smb347_usb_get_property(struct power_supply *psy,
				   enum power_supply_property prop,
				   union power_supply_propval *val)
{
	struct smb347_charger *smb =
		container_of(psy, struct smb347_charger, usb);
	int ret;

	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = smb->usb_online;
<<<<<<< HEAD
		return 0;

	case POWER_SUPPLY_PROP_USB_HC:
		val->intval = smb->usb_hc_mode;
		return 0;

	case POWER_SUPPLY_PROP_USB_OTG:
		val->intval = smb->usb_otg_enabled;
		return 0;

	default:
		break;
=======
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		ret = get_const_charge_voltage(smb);
		if (ret < 0)
			return ret;
		else
			val->intval = ret;
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = get_const_charge_current(smb);
		if (ret < 0)
			return ret;
		else
			val->intval = ret;
		break;

	default:
		return -EINVAL;
>>>>>>> v3.10.88
	}

	return 0;
}

static int smb347_usb_set_property(struct power_supply *psy,
				   enum power_supply_property prop,
				   const union power_supply_propval *val)
{
	int ret = -EINVAL;
	struct smb347_charger *smb =
		container_of(psy, struct smb347_charger, usb);
	bool oldval;

	switch (prop) {
	case POWER_SUPPLY_PROP_ONLINE:
		oldval = smb->usb_online;
		smb->usb_online = val->intval;

		if (smb->usb_online != oldval)
			power_supply_changed(psy);
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_USB_HC:
		smb347_set_writable(smb, true);
		ret = smb347_write(smb, CMD_B, val->intval ?
				   CMD_B_HC_MODE : CMD_B_USB59_MODE);
		smb347_set_writable(smb, false);
		smb->usb_hc_mode = val->intval;
		break;

	case POWER_SUPPLY_PROP_USB_OTG:
		ret = smb347_read(smb, CMD_A);

		if (ret < 0)
			return ret;

		if (val->intval)
			ret |= CMD_A_OTG_ENABLE;
		else
			ret &= ~CMD_A_OTG_ENABLE;

		ret = smb347_write(smb, CMD_A, ret);

		if (ret >= 0)
			smb->usb_otg_enabled = val->intval;

		break;

	default:
		break;
	}

	return ret;
}

static int smb347_usb_property_is_writeable(struct power_supply *psy,
					    enum power_supply_property prop)
{
	switch (prop) {
	case POWER_SUPPLY_PROP_USB_HC:
	case POWER_SUPPLY_PROP_USB_OTG:
		return 1;
	default:
		break;
	}

	return 0;
}

static enum power_supply_property smb347_usb_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
<<<<<<< HEAD
	POWER_SUPPLY_PROP_USB_HC,
	POWER_SUPPLY_PROP_USB_OTG,
=======
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
>>>>>>> v3.10.88
};

static int smb347_get_charging_status(struct smb347_charger *smb)
{
	int ret, status;
	unsigned int val;

	if (!smb347_is_ps_online(smb))
		return POWER_SUPPLY_STATUS_DISCHARGING;

	ret = regmap_read(smb->regmap, STAT_C, &val);
	if (ret < 0)
		return ret;

	if ((val & STAT_C_CHARGER_ERROR) ||
			(val & STAT_C_HOLDOFF_STAT)) {
		/*
		 * set to NOT CHARGING upon charger error
		 * or charging has stopped.
		 */
		status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	} else {
		if ((val & STAT_C_CHG_MASK) >> STAT_C_CHG_SHIFT) {
			/*
			 * set to charging if battery is in pre-charge,
			 * fast charge or taper charging mode.
			 */
			status = POWER_SUPPLY_STATUS_CHARGING;
		} else if (val & STAT_C_CHG_TERM) {
			/*
			 * set the status to FULL if battery is not in pre
			 * charge, fast charge or taper charging mode AND
			 * charging is terminated at least once.
			 */
			status = POWER_SUPPLY_STATUS_FULL;
		} else {
			/*
			 * in this case no charger error or termination
			 * occured but charging is not in progress!!!
			 */
			status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		}
	}

	return status;
}

static int smb347_battery_get_property(struct power_supply *psy,
				       enum power_supply_property prop,
				       union power_supply_propval *val)
{
	struct smb347_charger *smb =
			container_of(psy, struct smb347_charger, battery);
	const struct smb347_charger_platform_data *pdata = smb->pdata;
	int ret;

	ret = smb347_update_ps_status(smb);
	if (ret < 0)
		return ret;

	if (ret > 0) {
		smb347_update_online(smb);
		power_supply_changed(&smb->mains);
		power_supply_changed(&smb->usb);
	}

	switch (prop) {
	case POWER_SUPPLY_PROP_STATUS:
<<<<<<< HEAD
		if (!smb347_is_online(smb)) {
			smb->is_fully_charged = false;
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
			break;
		}
		if (smb347_charging_status(smb))
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else
			val->intval = smb->is_fully_charged ?
					POWER_SUPPLY_STATUS_FULL :
					POWER_SUPPLY_STATUS_NOT_CHARGING;
=======
		ret = smb347_get_charging_status(smb);
		if (ret < 0)
			return ret;
		val->intval = ret;
>>>>>>> v3.10.88
		break;

	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		if (!smb347_is_ps_online(smb))
			return -ENODATA;

		/*
		 * We handle trickle and pre-charging the same, and taper
		 * and none the same.
		 */
		switch (smb347_charging_status(smb)) {
		case 1:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
			break;
		case 2:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
			break;
		default:
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
			break;
		}
		break;

	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = pdata->battery_info.technology;
		break;

	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = pdata->battery_info.voltage_min_design;
		break;

	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = pdata->battery_info.voltage_max_design;
		break;

	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->intval = pdata->battery_info.charge_full_design;
		break;

	case POWER_SUPPLY_PROP_CHARGE_ENABLED:
		val->intval = smb->charging_enabled;
		break;

	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = pdata->battery_info.name;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int smb347_battery_set_property(struct power_supply *psy,
				       enum power_supply_property prop,
				       const union power_supply_propval *val)
{
	int ret = -EINVAL;
	struct smb347_charger *smb =
		container_of(psy, struct smb347_charger, battery);

	switch (prop) {
	case POWER_SUPPLY_PROP_CHARGE_ENABLED:
		ret = smb347_charging_set(smb, val->intval);
		break;

	default:
		break;
	}

	return ret;
}

static int smb347_battery_property_is_writeable(struct power_supply *psy,
						enum power_supply_property prop)
{
	switch (prop) {
	case POWER_SUPPLY_PROP_CHARGE_ENABLED:
		return 1;
	default:
		break;
	}

	return 0;
}

static enum power_supply_property smb347_battery_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_ENABLED,
	POWER_SUPPLY_PROP_MODEL_NAME,
};

static bool smb347_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case IRQSTAT_A:
	case IRQSTAT_C:
	case IRQSTAT_E:
	case IRQSTAT_F:
	case STAT_A:
	case STAT_B:
	case STAT_C:
	case STAT_E:
		return true;
	}

	return false;
}

static bool smb347_readable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case CFG_CHARGE_CURRENT:
	case CFG_CURRENT_LIMIT:
	case CFG_FLOAT_VOLTAGE:
	case CFG_STAT:
	case CFG_PIN:
	case CFG_THERM:
	case CFG_SYSOK:
	case CFG_OTHER:
	case CFG_OTG:
	case CFG_TEMP_LIMIT:
	case CFG_FAULT_IRQ:
	case CFG_STATUS_IRQ:
	case CFG_ADDRESS:
	case CMD_A:
	case CMD_B:
	case CMD_C:
		return true;
	}

	return smb347_volatile_reg(dev, reg);
}

static const struct regmap_config smb347_regmap = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.max_register	= SMB347_MAX_REGISTER,
	.volatile_reg	= smb347_volatile_reg,
	.readable_reg	= smb347_readable_reg,
};

static int smb347_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	static char *battery[] = { "smb347-battery" };
	const struct smb347_charger_platform_data *pdata;
	struct device *dev = &client->dev;
	struct smb347_charger *smb;
	int ret;

	pdata = dev->platform_data;
	if (!pdata)
		return -EINVAL;

	if (!pdata->use_mains && !pdata->use_usb)
		return -EINVAL;

	smb = devm_kzalloc(dev, sizeof(*smb), GFP_KERNEL);
	if (!smb)
		return -ENOMEM;

	i2c_set_clientdata(client, smb);

	mutex_init(&smb->lock);
	smb->dev = &client->dev;
	smb->pdata = pdata;

<<<<<<< HEAD
	smb->mains_current_limit = smb->pdata->mains_current_limit;

	if (pdata->en_gpio) {
		ret = gpio_request_one(
			pdata->en_gpio,
			smb->pdata->enable_control ==
			SMB347_CHG_ENABLE_PIN_ACTIVE_LOW ?
			GPIOF_OUT_INIT_HIGH : GPIOF_OUT_INIT_LOW,
			smb->client->name);
		if (ret < 0)
			dev_warn(dev, "failed to claim EN GPIO: %d\n", ret);
		else
			smb->en_gpio = pdata->en_gpio;
	}

	ret = smb347_write(smb, CMD_B, CMD_B_POR);
	if (ret < 0)
		return ret;

	msleep(20);

	ret = smb347_read(smb, CMD_B);
	if (ret < 0) {
		dev_err(dev, "failed read after reset\n");
		return ret;
	}
=======
	smb->regmap = devm_regmap_init_i2c(client, &smb347_regmap);
	if (IS_ERR(smb->regmap))
		return PTR_ERR(smb->regmap);
>>>>>>> v3.10.88

	ret = smb347_hw_init(smb);
	if (ret < 0)
		return ret;

<<<<<<< HEAD
	smb->mains.name = "smb347-mains";
	smb->mains.type = POWER_SUPPLY_TYPE_MAINS;
	smb->mains.get_property = smb347_mains_get_property;
	smb->mains.set_property = smb347_mains_set_property;
	smb->mains.property_is_writeable = smb347_mains_property_is_writeable;
	smb->mains.properties = smb347_mains_properties;
	smb->mains.num_properties = ARRAY_SIZE(smb347_mains_properties);
	smb->mains.supplied_to = battery;
	smb->mains.num_supplicants = ARRAY_SIZE(battery);

	smb->usb.name = "smb347-usb";
	smb->usb.type = POWER_SUPPLY_TYPE_USB;
	smb->usb.get_property = smb347_usb_get_property;
	smb->usb.set_property = smb347_usb_set_property;
	smb->usb.property_is_writeable = smb347_usb_property_is_writeable;
	smb->usb.properties = smb347_usb_properties;
	smb->usb.num_properties = ARRAY_SIZE(smb347_usb_properties);
	smb->usb.supplied_to = battery;
	smb->usb.num_supplicants = ARRAY_SIZE(battery);
=======
	if (smb->pdata->use_mains) {
		smb->mains.name = "smb347-mains";
		smb->mains.type = POWER_SUPPLY_TYPE_MAINS;
		smb->mains.get_property = smb347_mains_get_property;
		smb->mains.properties = smb347_mains_properties;
		smb->mains.num_properties = ARRAY_SIZE(smb347_mains_properties);
		smb->mains.supplied_to = battery;
		smb->mains.num_supplicants = ARRAY_SIZE(battery);
		ret = power_supply_register(dev, &smb->mains);
		if (ret < 0)
			return ret;
	}

	if (smb->pdata->use_usb) {
		smb->usb.name = "smb347-usb";
		smb->usb.type = POWER_SUPPLY_TYPE_USB;
		smb->usb.get_property = smb347_usb_get_property;
		smb->usb.properties = smb347_usb_properties;
		smb->usb.num_properties = ARRAY_SIZE(smb347_usb_properties);
		smb->usb.supplied_to = battery;
		smb->usb.num_supplicants = ARRAY_SIZE(battery);
		ret = power_supply_register(dev, &smb->usb);
		if (ret < 0) {
			if (smb->pdata->use_mains)
				power_supply_unregister(&smb->mains);
			return ret;
		}
	}
>>>>>>> v3.10.88

	smb->battery.name = "smb347-battery";
	smb->battery.type = POWER_SUPPLY_TYPE_BATTERY;
	smb->battery.get_property = smb347_battery_get_property;
	smb->battery.set_property = smb347_battery_set_property;
	smb->battery.property_is_writeable = smb347_battery_property_is_writeable;
	smb->battery.properties = smb347_battery_properties;
	smb->battery.num_properties = ARRAY_SIZE(smb347_battery_properties);

<<<<<<< HEAD
	if (smb->pdata->supplied_to) {
		smb->battery.supplied_to = smb->pdata->supplied_to;
		smb->battery.num_supplicants = smb->pdata->num_supplicants;
		smb->battery.external_power_changed = power_supply_changed;
	}

	ret = power_supply_register(dev, &smb->mains);
	if (ret < 0)
		return ret;

	ret = power_supply_register(dev, &smb->usb);
	if (ret < 0) {
		power_supply_unregister(&smb->mains);
		return ret;
	}
=======
>>>>>>> v3.10.88

	ret = power_supply_register(dev, &smb->battery);
	if (ret < 0) {
		if (smb->pdata->use_usb)
			power_supply_unregister(&smb->usb);
		if (smb->pdata->use_mains)
			power_supply_unregister(&smb->mains);
		return ret;
	}

	/*
	 * Interrupt pin is optional. If it is connected, we setup the
	 * interrupt support here.
	 */
	if (pdata->irq_gpio >= 0) {
		ret = smb347_irq_init(smb, client);
		if (ret < 0) {
			dev_warn(dev, "failed to initialize IRQ: %d\n", ret);
			dev_warn(dev, "disabling IRQ support\n");
		} else {
			smb347_irq_enable(smb);
		}
	}

	return 0;
}

static int smb347_remove(struct i2c_client *client)
{
	struct smb347_charger *smb = i2c_get_clientdata(client);

	if (client->irq) {
		smb347_irq_disable(smb);
		disable_irq_wake(client->irq);
		free_irq(client->irq, smb);
		gpio_free(smb->pdata->irq_gpio);
	}

	power_supply_unregister(&smb->battery);
	if (smb->pdata->use_usb)
		power_supply_unregister(&smb->usb);
	if (smb->pdata->use_mains)
		power_supply_unregister(&smb->mains);
	return 0;
}

static int smb347_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);

	if (client->irq)
		disable_irq(client->irq);
	return 0;
}

static int smb347_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);

	if (client->irq)
		enable_irq(client->irq);
	return 0;
}

static const struct dev_pm_ops smb347_pm_ops = {
	.suspend = smb347_suspend,
	.resume = smb347_resume,
};

static const struct i2c_device_id smb347_id[] = {
	{ "smb347", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, smb347_id);

static struct i2c_driver smb347_driver = {
	.driver = {
		.name = "smb347",
		.pm = &smb347_pm_ops,
	},
	.probe        = smb347_probe,
	.remove       = smb347_remove,
	.id_table     = smb347_id,
};

module_i2c_driver(smb347_driver);

MODULE_AUTHOR("Bruce E. Robertson <bruce.e.robertson@intel.com>");
MODULE_AUTHOR("Mika Westerberg <mika.westerberg@linux.intel.com>");
MODULE_DESCRIPTION("SMB347 battery charger driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("i2c:smb347");
