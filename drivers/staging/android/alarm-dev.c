/* drivers/rtc/alarm-dev.c
 *
 * Copyright (C) 2007-2009 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/time.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/alarmtimer.h>
<<<<<<< HEAD
#include <linux/wakelock.h>
=======
>>>>>>> v3.10.88
#include "android_alarm.h"
#include <linux/xlog.h>

<<<<<<< HEAD
#include <mach/mtk_rtc.h>
#define XLOG_MYTAG	"Power/Alarm"

=======
>>>>>>> v3.10.88
#define ANDROID_ALARM_PRINT_INFO (1U << 0)
#define ANDROID_ALARM_PRINT_IO (1U << 1)
#define ANDROID_ALARM_PRINT_INT (1U << 2)

<<<<<<< HEAD
static int debug_mask = ANDROID_ALARM_PRINT_INFO | \
						ANDROID_ALARM_PRINT_IO | \
						ANDROID_ALARM_PRINT_INT;
module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);
/*
#define pr_alarm(debug_level_mask, args...) \
	do { \
		if (debug_mask & ANDROID_ALARM_PRINT_##debug_level_mask) { \
			pr_info(args); \
		} \
	} while (0)*/
#define pr_alarm(debug_level_mask, fmt, args...) \
	do { \
		if (debug_mask & ANDROID_ALARM_PRINT_##debug_level_mask) { \
			xlog_printk(ANDROID_LOG_INFO, XLOG_MYTAG, fmt, ##args); \
		} \
	} while (0)
=======
static int debug_mask = ANDROID_ALARM_PRINT_INFO;
module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

#define alarm_dbg(debug_level_mask, fmt, ...)				\
do {									\
	if (debug_mask & ANDROID_ALARM_PRINT_##debug_level_mask)	\
		pr_info(fmt, ##__VA_ARGS__);				\
} while (0)
>>>>>>> v3.10.88

#define ANDROID_ALARM_WAKEUP_MASK ( \
	ANDROID_ALARM_RTC_WAKEUP_MASK | \
	ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP_MASK)

static int alarm_opened;
static DEFINE_SPINLOCK(alarm_slock);
static struct wakeup_source alarm_wake_lock;
static DECLARE_WAIT_QUEUE_HEAD(alarm_wait_queue);
static uint32_t alarm_pending;
static uint32_t alarm_enabled;
static uint32_t wait_pending;

struct devalarm {
	union {
		struct hrtimer hrt;
		struct alarm alrm;
	} u;
	enum android_alarm_type type;
};
<<<<<<< HEAD

static struct devalarm alarms[ANDROID_ALARM_TYPE_COUNT];

#if 0
void alarm_set_power_on(struct timespec new_pwron_time, bool logo)
{
	unsigned long pwron_time;
	struct rtc_wkalrm alm;
	struct rtc_device *alarm_rtc_dev;
	
	pr_alarm(INFO, "alarm set power on\n");
	
#ifdef RTC_PWRON_SEC
	/* round down the second */
	new_pwron_time.tv_sec = (new_pwron_time.tv_sec / 60) * 60;
#endif
	if (new_pwron_time.tv_sec > 0) {
		pwron_time = new_pwron_time.tv_sec;
#ifdef RTC_PWRON_SEC
		pwron_time += RTC_PWRON_SEC;
#endif
		alm.enabled = (logo ? 3 : 2);
	} else {
		pwron_time = 0;
		alm.enabled = 4;
	}
	alarm_rtc_dev = alarmtimer_get_rtcdev();
	rtc_time_to_tm(pwron_time, &alm.time);
	
	rtc_set_alarm_poweron(alarm_rtc_dev, &alm);
}

void alarm_get_power_on(struct rtc_wkalrm *alm)
{
	if (!alm)
		return;

	memset(alm, 0, sizeof(struct rtc_wkalrm));
	rtc_read_pwron_alarm(alm);
}
#endif
static int is_wakeup(enum android_alarm_type type)
{
	if (type == ANDROID_ALARM_RTC_WAKEUP ||
			type == ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP)
		return 1;
	return 0;
}


static void devalarm_start(struct devalarm *alrm, ktime_t exp)
{
	if (is_wakeup(alrm->type))
		alarm_start(&alrm->u.alrm, exp);
	else
		hrtimer_start(&alrm->u.hrt, exp, HRTIMER_MODE_ABS);
}


static int devalarm_try_to_cancel(struct devalarm *alrm)
{
	int ret;
	if (is_wakeup(alrm->type))
		ret = alarm_try_to_cancel(&alrm->u.alrm);
	else
		ret = hrtimer_try_to_cancel(&alrm->u.hrt);
	return ret;
}

static void devalarm_cancel(struct devalarm *alrm)
{
	if (is_wakeup(alrm->type))
		alarm_cancel(&alrm->u.alrm);
	else
		hrtimer_cancel(&alrm->u.hrt);
}

=======
>>>>>>> v3.10.88

static struct devalarm alarms[ANDROID_ALARM_TYPE_COUNT];


static int is_wakeup(enum android_alarm_type type)
{
	return (type == ANDROID_ALARM_RTC_WAKEUP ||
		type == ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP);
}


static void devalarm_start(struct devalarm *alrm, ktime_t exp)
{
	if (is_wakeup(alrm->type))
		alarm_start(&alrm->u.alrm, exp);
	else
		hrtimer_start(&alrm->u.hrt, exp, HRTIMER_MODE_ABS);
}


static int devalarm_try_to_cancel(struct devalarm *alrm)
{
	if (is_wakeup(alrm->type))
		return alarm_try_to_cancel(&alrm->u.alrm);
	return hrtimer_try_to_cancel(&alrm->u.hrt);
}

static void devalarm_cancel(struct devalarm *alrm)
{
	if (is_wakeup(alrm->type))
		alarm_cancel(&alrm->u.alrm);
	else
		hrtimer_cancel(&alrm->u.hrt);
}

static void alarm_clear(enum android_alarm_type alarm_type)
{
	uint32_t alarm_type_mask = 1U << alarm_type;
	unsigned long flags;

	spin_lock_irqsave(&alarm_slock, flags);
	alarm_dbg(IO, "alarm %d clear\n", alarm_type);
	devalarm_try_to_cancel(&alarms[alarm_type]);
	if (alarm_pending) {
		alarm_pending &= ~alarm_type_mask;
		if (!alarm_pending && !wait_pending)
			__pm_relax(&alarm_wake_lock);
	}
	alarm_enabled &= ~alarm_type_mask;
	spin_unlock_irqrestore(&alarm_slock, flags);

}

static void alarm_set(enum android_alarm_type alarm_type,
							struct timespec *ts)
{
	uint32_t alarm_type_mask = 1U << alarm_type;
	unsigned long flags;

	spin_lock_irqsave(&alarm_slock, flags);
	alarm_dbg(IO, "alarm %d set %ld.%09ld\n",
			alarm_type, ts->tv_sec, ts->tv_nsec);
	alarm_enabled |= alarm_type_mask;
	devalarm_start(&alarms[alarm_type], timespec_to_ktime(*ts));
	spin_unlock_irqrestore(&alarm_slock, flags);
}

static int alarm_wait(void)
{
	unsigned long flags;
	int rv = 0;

	spin_lock_irqsave(&alarm_slock, flags);
	alarm_dbg(IO, "alarm wait\n");
	if (!alarm_pending && wait_pending) {
		__pm_relax(&alarm_wake_lock);
		wait_pending = 0;
	}
	spin_unlock_irqrestore(&alarm_slock, flags);

	rv = wait_event_interruptible(alarm_wait_queue, alarm_pending);
	if (rv)
		return rv;

	spin_lock_irqsave(&alarm_slock, flags);
	rv = alarm_pending;
	wait_pending = 1;
	alarm_pending = 0;
	spin_unlock_irqrestore(&alarm_slock, flags);

	return rv;
}

static int alarm_set_rtc(struct timespec *ts)
{
	struct rtc_time new_rtc_tm;
	struct rtc_device *rtc_dev;
	unsigned long flags;
	int rv = 0;

	rtc_time_to_tm(ts->tv_sec, &new_rtc_tm);
	rtc_dev = alarmtimer_get_rtcdev();
	rv = do_settimeofday(ts);
	if (rv < 0)
		return rv;
	if (rtc_dev)
		rv = rtc_set_time(rtc_dev, &new_rtc_tm);

	spin_lock_irqsave(&alarm_slock, flags);
	alarm_pending |= ANDROID_ALARM_TIME_CHANGE_MASK;
	wake_up(&alarm_wait_queue);
	spin_unlock_irqrestore(&alarm_slock, flags);

	return rv;
}

static int alarm_get_time(enum android_alarm_type alarm_type,
							struct timespec *ts)
{
	int rv = 0;

	switch (alarm_type) {
	case ANDROID_ALARM_RTC_WAKEUP:
	case ANDROID_ALARM_RTC:
		getnstimeofday(ts);
		break;
	case ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP:
	case ANDROID_ALARM_ELAPSED_REALTIME:
		get_monotonic_boottime(ts);
		break;
	case ANDROID_ALARM_SYSTEMTIME:
		ktime_get_ts(ts);
		break;
	default:
		rv = -EINVAL;
	}
	return rv;
}

static long alarm_do_ioctl(struct file *file, unsigned int cmd,
							struct timespec *ts)
{
	int rv = 0;
	unsigned long flags;
<<<<<<< HEAD
	struct timespec new_alarm_time;
	struct timespec new_rtc_time;
	struct timespec tmp_time;
	struct rtc_time new_rtc_tm;
	struct rtc_device *rtc_dev;
	struct rtc_wkalrm pwron_alm;
=======
>>>>>>> v3.10.88
	enum android_alarm_type alarm_type = ANDROID_ALARM_IOCTL_TO_TYPE(cmd);

 	if (alarm_type >= ANDROID_ALARM_TYPE_COUNT &&
	    alarm_type != ANDROID_ALARM_POWER_ON &&
	    alarm_type != ANDROID_ALARM_POWER_ON_LOGO) {
		return -EINVAL;
	}
	
	if (ANDROID_ALARM_BASE_CMD(cmd) != ANDROID_ALARM_GET_TIME(0)) {
		if ((file->f_flags & O_ACCMODE) == O_RDONLY)
			return -EPERM;
		if (file->private_data == NULL &&
		    cmd != ANDROID_ALARM_SET_RTC) {
			spin_lock_irqsave(&alarm_slock, flags);
			if (alarm_opened) {
				spin_unlock_irqrestore(&alarm_slock, flags);
				return -EBUSY;
			}
			alarm_opened = 1;
			file->private_data = (void *)1;
			spin_unlock_irqrestore(&alarm_slock, flags);
		}
	}

	switch (ANDROID_ALARM_BASE_CMD(cmd)) {
	case ANDROID_ALARM_CLEAR(0):
<<<<<<< HEAD
		pr_alarm(IO, "alarm %d clear\n", alarm_type);
		if (alarm_type == ANDROID_ALARM_POWER_ON ||
			alarm_type == ANDROID_ALARM_POWER_ON_LOGO) {
			new_alarm_time.tv_sec = 0;
			alarm_set_power_on(new_alarm_time, false);
			break;
		}
		spin_lock_irqsave(&alarm_slock, flags);
		pr_alarm(IO, "alarm %d clear\n", alarm_type);
		devalarm_try_to_cancel(&alarms[alarm_type]);
		if (alarm_pending) {
			alarm_pending &= ~alarm_type_mask;
			if (!alarm_pending && !wait_pending)
				wake_unlock(&alarm_wake_lock);
		}
		alarm_enabled &= ~alarm_type_mask;
		spin_unlock_irqrestore(&alarm_slock, flags);
=======
		alarm_clear(alarm_type);
>>>>>>> v3.10.88
		break;
	case ANDROID_ALARM_SET(0):
<<<<<<< HEAD
		if (copy_from_user(&new_alarm_time, (void __user *)arg,
		    sizeof(new_alarm_time))) {
			rv = -EFAULT;
			goto err1;
		}
from_old_alarm_set:
		pr_alarm(IO, "alarm %d set %ld.%09ld\n", alarm_type,
			new_alarm_time.tv_sec, new_alarm_time.tv_nsec);

		if (alarm_type == ANDROID_ALARM_POWER_ON) {
			alarm_set_power_on(new_alarm_time, false);
			break;
		}
		if (alarm_type == ANDROID_ALARM_POWER_ON_LOGO) {
			alarm_set_power_on(new_alarm_time, true);
			break;
		}
		spin_lock_irqsave(&alarm_slock, flags);
	
		alarm_enabled |= alarm_type_mask;
		devalarm_start(&alarms[alarm_type],
			timespec_to_ktime(new_alarm_time));
		spin_unlock_irqrestore(&alarm_slock, flags);
		if (ANDROID_ALARM_BASE_CMD(cmd) != ANDROID_ALARM_SET_AND_WAIT(0)
		    && cmd != ANDROID_ALARM_SET_AND_WAIT_OLD)
			break;
=======
		alarm_set(alarm_type, ts);
		break;
	case ANDROID_ALARM_SET_AND_WAIT(0):
		alarm_set(alarm_type, ts);
>>>>>>> v3.10.88
		/* fall though */
	case ANDROID_ALARM_WAIT:
		rv = alarm_wait();
		break;
	case ANDROID_ALARM_SET_RTC:
<<<<<<< HEAD
		if (copy_from_user(&new_rtc_time, (void __user *)arg,
		    sizeof(new_rtc_time))) {
			rv = -EFAULT;
			goto err1;
		}
		rtc_time_to_tm(new_rtc_time.tv_sec, &new_rtc_tm);
		
		pr_alarm(IO, "set rtc %ld %ld - rtc %02d:%02d:%02d %02d/%02d/%04d\n",
		new_rtc_time.tv_sec, new_rtc_time.tv_nsec,
		new_rtc_tm.tm_hour, new_rtc_tm.tm_min,
		new_rtc_tm.tm_sec, new_rtc_tm.tm_mon + 1,
		new_rtc_tm.tm_mday,
		new_rtc_tm.tm_year + 1900);

		rtc_dev = alarmtimer_get_rtcdev();
		rv = do_settimeofday(&new_rtc_time);
		if (rv < 0)
		{	
			goto err1;
		}
		if (rtc_dev)
		{
			rv = rtc_set_time(rtc_dev, &new_rtc_tm);
		}
		spin_lock_irqsave(&alarm_slock, flags);
		alarm_pending |= ANDROID_ALARM_TIME_CHANGE_MASK;
		wake_up(&alarm_wait_queue);
		spin_unlock_irqrestore(&alarm_slock, flags);
		if (rv < 0)
			goto err1;
		break;
	case ANDROID_ALARM_GET_TIME(0):
		switch (alarm_type) {
		case ANDROID_ALARM_RTC_WAKEUP:
		case ANDROID_ALARM_RTC:
			getnstimeofday(&tmp_time);
			break;
		case ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP:
		case ANDROID_ALARM_ELAPSED_REALTIME:
			get_monotonic_boottime(&tmp_time);
			break;
		case ANDROID_ALARM_TYPE_COUNT:
		case ANDROID_ALARM_SYSTEMTIME:
			ktime_get_ts(&tmp_time);
			break;
		case ANDROID_ALARM_POWER_ON:
		case ANDROID_ALARM_POWER_ON_LOGO:
			break;		
		}
		if (copy_to_user((void __user *)arg, &tmp_time,
		    sizeof(tmp_time))) {
			rv = -EFAULT;
			goto err1;
		}
=======
		rv = alarm_set_rtc(ts);
		break;
	case ANDROID_ALARM_GET_TIME(0):
		rv = alarm_get_time(alarm_type, ts);
>>>>>>> v3.10.88
		break;
	case ANDROID_ALARM_GET_POWER_ON:
		alarm_get_power_on(&pwron_alm);
		if (copy_to_user((void __user *)arg, &pwron_alm,
		    sizeof(struct rtc_wkalrm))) {
			rv = -EFAULT;
			goto err1;
		}
		break;

	default:
		rv = -EINVAL;
	}
	return rv;
}

static long alarm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	struct timespec ts;
	int rv;

	switch (ANDROID_ALARM_BASE_CMD(cmd)) {
	case ANDROID_ALARM_SET_AND_WAIT(0):
	case ANDROID_ALARM_SET(0):
	case ANDROID_ALARM_SET_RTC:
		if (copy_from_user(&ts, (void __user *)arg, sizeof(ts)))
			return -EFAULT;
		break;
	}

	rv = alarm_do_ioctl(file, cmd, &ts);
	if (rv)
		return rv;

	switch (ANDROID_ALARM_BASE_CMD(cmd)) {
	case ANDROID_ALARM_GET_TIME(0):
		if (copy_to_user((void __user *)arg, &ts, sizeof(ts)))
			return -EFAULT;
		break;
	}

	return 0;
}
#ifdef CONFIG_COMPAT
static long alarm_compat_ioctl(struct file *file, unsigned int cmd,
							unsigned long arg)
{

	struct timespec ts;
	int rv;

	switch (ANDROID_ALARM_BASE_CMD(cmd)) {
	case ANDROID_ALARM_SET_AND_WAIT_COMPAT(0):
	case ANDROID_ALARM_SET_COMPAT(0):
	case ANDROID_ALARM_SET_RTC_COMPAT:
		if (compat_get_timespec(&ts, (void __user *)arg))
			return -EFAULT;
		/* fall through */
	case ANDROID_ALARM_GET_TIME_COMPAT(0):
		cmd = ANDROID_ALARM_COMPAT_TO_NORM(cmd);
		break;
	}

	rv = alarm_do_ioctl(file, cmd, &ts);
	if (rv)
		return rv;

	switch (ANDROID_ALARM_BASE_CMD(cmd)) {
	case ANDROID_ALARM_GET_TIME(0): /* NOTE: we modified cmd above */
		if (compat_put_timespec(&ts, (void __user *)arg))
			return -EFAULT;
		break;
	}

	return 0;
}
#endif

static int alarm_open(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	pr_alarm(INFO, "alarm_open (%d:%d)\n", current->tgid, current->pid);
	return 0;
}

static int alarm_release(struct inode *inode, struct file *file)
{
	int i;
	unsigned long flags;

	spin_lock_irqsave(&alarm_slock, flags);
	if (file->private_data) {
		for (i = 0; i < ANDROID_ALARM_TYPE_COUNT; i++) {
			uint32_t alarm_type_mask = 1U << i;
			if (alarm_enabled & alarm_type_mask) {
				alarm_dbg(INFO,
					  "%s: clear alarm, pending %d\n",
					  __func__,
					  !!(alarm_pending & alarm_type_mask));
				alarm_enabled &= ~alarm_type_mask;
			}
			spin_unlock_irqrestore(&alarm_slock, flags);
			devalarm_cancel(&alarms[i]);
			spin_lock_irqsave(&alarm_slock, flags);
		}
		if (alarm_pending | wait_pending) {
			if (alarm_pending)
				alarm_dbg(INFO, "%s: clear pending alarms %x\n",
					  __func__, alarm_pending);
			__pm_relax(&alarm_wake_lock);
			wait_pending = 0;
			alarm_pending = 0;
		}
		alarm_opened = 0;
	}
	spin_unlock_irqrestore(&alarm_slock, flags);
	pr_alarm(INFO, "alarm_release (%d:%d)(%d)\n",
	current->tgid, current->pid, (int)file->private_data);
	return 0;
}

static void devalarm_triggered(struct devalarm *alarm)
{
	unsigned long flags;
	uint32_t alarm_type_mask = 1U << alarm->type;

<<<<<<< HEAD
	//pr_alarm(INT, "devalarm_triggered type %d\n", alarm->type);
	xlog_printk(ANDROID_LOG_DEBUG, "Power/Alarm", "devalarm_triggered type %d\n", alarm->type);
	
=======
	alarm_dbg(INT, "%s: type %d\n", __func__, alarm->type);
>>>>>>> v3.10.88
	spin_lock_irqsave(&alarm_slock, flags);
	if (alarm_enabled & alarm_type_mask) {
		__pm_wakeup_event(&alarm_wake_lock, 5000); /* 5secs */
		alarm_enabled &= ~alarm_type_mask;
		alarm_pending |= alarm_type_mask;
		wake_up(&alarm_wait_queue);
	}
	spin_unlock_irqrestore(&alarm_slock, flags);
}


static enum hrtimer_restart devalarm_hrthandler(struct hrtimer *hrt)
{
	struct devalarm *devalrm = container_of(hrt, struct devalarm, u.hrt);

<<<<<<< HEAD
	//pr_alarm(INT, "devalarm_hrthandler\n");
	xlog_printk(ANDROID_LOG_DEBUG, "Power/Alarm", "devalarm_hrthandler\n");
	
=======
>>>>>>> v3.10.88
	devalarm_triggered(devalrm);
	return HRTIMER_NORESTART;
}

static enum alarmtimer_restart devalarm_alarmhandler(struct alarm *alrm,
							ktime_t now)
{
	struct devalarm *devalrm = container_of(alrm, struct devalarm, u.alrm);

<<<<<<< HEAD
	//pr_alarm(INT, "devalarm_alarmhandler\n");
	xlog_printk(ANDROID_LOG_DEBUG, "Power/Alarm", "devalarm_alarmhandler\n");
	
=======
>>>>>>> v3.10.88
	devalarm_triggered(devalrm);
	return ALARMTIMER_NORESTART;
}


static const struct file_operations alarm_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = alarm_ioctl,
	.open = alarm_open,
	.release = alarm_release,
#ifdef CONFIG_COMPAT
	.compat_ioctl = alarm_compat_ioctl,
#endif
};

static struct miscdevice alarm_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "alarm",
	.fops = &alarm_fops,
};

static int __init alarm_dev_init(void)
{
	int err;
	int i;

	err = misc_register(&alarm_device);
	if (err)
		return err;

	alarm_init(&alarms[ANDROID_ALARM_RTC_WAKEUP].u.alrm,
			ALARM_REALTIME, devalarm_alarmhandler);
	hrtimer_init(&alarms[ANDROID_ALARM_RTC].u.hrt,
			CLOCK_REALTIME, HRTIMER_MODE_ABS);
	alarm_init(&alarms[ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP].u.alrm,
			ALARM_BOOTTIME, devalarm_alarmhandler);
	hrtimer_init(&alarms[ANDROID_ALARM_ELAPSED_REALTIME].u.hrt,
			CLOCK_BOOTTIME, HRTIMER_MODE_ABS);
	hrtimer_init(&alarms[ANDROID_ALARM_SYSTEMTIME].u.hrt,
			CLOCK_MONOTONIC, HRTIMER_MODE_ABS);

	for (i = 0; i < ANDROID_ALARM_TYPE_COUNT; i++) {
		alarms[i].type = i;
		if (!is_wakeup(i))
			alarms[i].u.hrt.function = devalarm_hrthandler;
	}
<<<<<<< HEAD

	wake_lock_init(&alarm_wake_lock, WAKE_LOCK_SUSPEND, "alarm");
=======
>>>>>>> v3.10.88

	wakeup_source_init(&alarm_wake_lock, "alarm");
	return 0;
}

static void  __exit alarm_dev_exit(void)
{
	misc_deregister(&alarm_device);
	wakeup_source_trash(&alarm_wake_lock);
}

module_init(alarm_dev_init);
module_exit(alarm_dev_exit);

