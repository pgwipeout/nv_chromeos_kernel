/*
 * drivers/thermal/pid_thermal_gov.c
 *
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.

 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.

 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/thermal.h>

#include "thermal_core.h"

#define DRV_NAME	"pid_thermal_gov"

#define MAX_ERR_TEMP_DEFAULT	10000	/* in mC */
#define MAX_ERR_GAIN_DEFAULT	1000
#define GAIN_P_DEFAULT		1000
#define GAIN_D_DEFAULT		0

struct pid_thermal_gov_attribute {
	struct attribute attr;
	ssize_t (*show)(struct kobject *kobj, struct attribute *attr,
			char *buf);
	ssize_t (*store)(struct kobject *kobj, struct attribute *attr,
			 const char *buf, size_t count);
};

struct pid_thermal_governor {
	struct kobject kobj;

	int max_err_temp; /* max error temperature in mC */
	int max_err_gain; /* max error gain */

	int gain_p; /* proportional gain */
	int gain_i; /* integral gain */
	int gain_d; /* derivative gain */
};

#define tz_to_gov(t)		\
	(t->governor_data)

#define kobj_to_gov(k)		\
	container_of(k, struct pid_thermal_governor, kobj)

#define attr_to_gov_attr(a)	\
	container_of(a, struct pid_thermal_gov_attribute, attr)

static ssize_t max_err_temp_show(struct kobject *kobj, struct attribute *attr,
				 char *buf)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);

	if (!gov)
		return -ENODEV;

	return sprintf(buf, "%d\n", gov->max_err_temp);
}

static ssize_t max_err_temp_store(struct kobject *kobj, struct attribute *attr,
				  const char *buf, size_t count)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);
	int val;

	if (!gov)
		return -ENODEV;

	if (!sscanf(buf, "%d\n", &val))
		return -EINVAL;

	gov->max_err_temp = val;
	return count;
}

static struct pid_thermal_gov_attribute max_err_temp_attr =
	__ATTR(max_err_temp, 0644, max_err_temp_show, max_err_temp_store);

static ssize_t max_err_gain_show(struct kobject *kobj, struct attribute *attr,
				 char *buf)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);

	if (!gov)
		return -ENODEV;

	return sprintf(buf, "%d\n", gov->max_err_gain);
}

static ssize_t max_err_gain_store(struct kobject *kobj, struct attribute *attr,
				  const char *buf, size_t count)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);
	int val;

	if (!gov)
		return -ENODEV;

	if (!sscanf(buf, "%d\n", &val))
		return -EINVAL;

	gov->max_err_gain = val;
	return count;
}

static struct pid_thermal_gov_attribute max_err_gain_attr =
	__ATTR(max_err_gain, 0644, max_err_gain_show, max_err_gain_store);

static ssize_t gain_p_show(struct kobject *kobj, struct attribute *attr,
			   char *buf)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);

	if (!gov)
		return -ENODEV;

	return sprintf(buf, "%d\n", gov->gain_p);
}

static ssize_t gain_p_store(struct kobject *kobj, struct attribute *attr,
			    const char *buf, size_t count)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);
	int val;

	if (!gov)
		return -ENODEV;

	if (!sscanf(buf, "%d\n", &val))
		return -EINVAL;

	gov->gain_p = val;
	return count;
}

static struct pid_thermal_gov_attribute gain_p_attr =
	__ATTR(gain_p, 0644, gain_p_show, gain_p_store);

static ssize_t gain_d_show(struct kobject *kobj, struct attribute *attr,
			   char *buf)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);

	if (!gov)
		return -ENODEV;

	return sprintf(buf, "%d\n", gov->gain_d);
}

static ssize_t gain_d_store(struct kobject *kobj, struct attribute *attr,
			    const char *buf, size_t count)
{
	struct pid_thermal_governor *gov = kobj_to_gov(kobj);
	int val;

	if (!gov)
		return -ENODEV;

	if (!sscanf(buf, "%d\n", &val))
		return -EINVAL;

	gov->gain_d = val;
	return count;
}

static struct pid_thermal_gov_attribute gain_d_attr =
	__ATTR(gain_d, 0644, gain_d_show, gain_d_store);

static struct attribute *pid_thermal_gov_default_attrs[] = {
	&max_err_temp_attr.attr,
	&max_err_gain_attr.attr,
	&gain_p_attr.attr,
	&gain_d_attr.attr,
	NULL,
};

static ssize_t pid_thermal_gov_show(struct kobject *kobj,
				    struct attribute *attr, char *buf)
{
	struct pid_thermal_gov_attribute *gov_attr = attr_to_gov_attr(attr);

	if (!gov_attr->show)
		return -EIO;

	return gov_attr->show(kobj, attr, buf);
}

static ssize_t pid_thermal_gov_store(struct kobject *kobj,
				     struct attribute *attr, const char *buf,
				     size_t len)
{
	struct pid_thermal_gov_attribute *gov_attr = attr_to_gov_attr(attr);

	if (!gov_attr->store)
		return -EIO;

	return gov_attr->store(kobj, attr, buf, len);
}

static const struct sysfs_ops pid_thermal_gov_sysfs_ops = {
	.show	= pid_thermal_gov_show,
	.store	= pid_thermal_gov_store,
};

static struct kobj_type pid_thermal_gov_ktype = {
	.default_attrs	= pid_thermal_gov_default_attrs,
	.sysfs_ops	= &pid_thermal_gov_sysfs_ops,
};

static int pid_thermal_gov_start(struct thermal_zone_device *tz)
{
	struct pid_thermal_governor *gov;
	int ret;

	gov = kzalloc(sizeof(struct pid_thermal_governor), GFP_KERNEL);
	if (!gov) {
		dev_err(&tz->device, "%s: Can't alloc governor data\n",
			DRV_NAME);
		return -ENOMEM;
	}

	ret = kobject_init_and_add(&gov->kobj, &pid_thermal_gov_ktype,
				   &tz->device.kobj, DRV_NAME);
	if (ret) {
		dev_err(&tz->device, "%s: Can't init kobject\n", DRV_NAME);
		kobject_put(&gov->kobj);
		kfree(gov);
		return ret;
	}

	gov->max_err_temp = MAX_ERR_TEMP_DEFAULT;
	gov->max_err_gain = MAX_ERR_GAIN_DEFAULT;
	gov->gain_p = GAIN_P_DEFAULT;
	gov->gain_d = GAIN_D_DEFAULT;
	tz->governor_data = gov;

	return 0;
}

static void pid_thermal_gov_stop(struct thermal_zone_device *tz)
{
	struct pid_thermal_governor *gov = tz_to_gov(tz);

	if (!gov)
		return;

	kobject_put(&gov->kobj);
	kfree(gov);
}

static void pid_thermal_gov_update_passive(struct thermal_zone_device *tz,
					   enum thermal_trip_type trip_type,
					   unsigned long old_target,
					   unsigned long target)
{
	if ((trip_type != THERMAL_TRIP_PASSIVE) &&
			(trip_type != THERMAL_TRIPS_NONE))
		return;

	if ((!old_target || old_target == THERMAL_NO_TARGET) &&
			(target && target != THERMAL_NO_TARGET))
		tz->passive++;
	else if ((old_target && old_target != THERMAL_NO_TARGET) &&
			(!target || target == THERMAL_NO_TARGET))
		tz->passive--;
}

static unsigned long
pid_thermal_gov_get_target(struct thermal_zone_device *tz,
			   struct thermal_cooling_device *cdev,
			   enum thermal_trip_type trip_type,
			   unsigned long trip_temp)
{
	struct pid_thermal_governor *gov = tz_to_gov(tz);
	int last_temperature = tz->passive ? tz->last_temperature : trip_temp;
	int passive_delay = tz->passive ? tz->passive_delay : MSEC_PER_SEC;
	s64 proportional, derivative, sum_err, max_err;
	unsigned long max_state;

	if (cdev->ops->get_max_state(cdev, &max_state) < 0)
		return 0;

	/* Calculate proportional term */
	proportional = (s64)tz->temperature - (s64)trip_temp;
	proportional *= gov->gain_p;

	/* Calculate derivative term */
	derivative = (s64)tz->temperature - (s64)last_temperature;
	derivative *= gov->gain_d;
	derivative *= MSEC_PER_SEC;
	derivative = div64_s64(derivative, passive_delay);

	max_err = (s64)gov->max_err_temp * (s64)gov->max_err_gain;
	sum_err = proportional + derivative;
	sum_err = max_t(s64, sum_err, 0);
	if (sum_err == 0)
		return 0;

	sum_err = min_t(s64, sum_err, max_err);
	if (sum_err == max_err)
		return max_state;

	sum_err = sum_err * max_state + max_err - 1;
	sum_err = div64_s64(sum_err, max_err);

	return (unsigned long)sum_err;
}

static int pid_thermal_gov_throttle(struct thermal_zone_device *tz, int trip)
{
	struct thermal_instance *instance;
	enum thermal_trip_type trip_type;
	unsigned long trip_temp, target;

	tz->ops->get_trip_type(tz, trip, &trip_type);
	tz->ops->get_trip_temp(tz, trip, &trip_temp);

	mutex_lock(&tz->lock);

	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		if (instance->trip != trip)
			continue;

		target = pid_thermal_gov_get_target(tz, instance->cdev,
						    trip_type, trip_temp);
		if (target >= instance->upper)
			target = instance->upper;
		else if (target < instance->lower)
			target = instance->lower;

		if (tz->temperature < trip_temp) {
			if ((target == instance->lower) &&
					(instance->target == instance->lower ||
					instance->target == THERMAL_NO_TARGET))
				target = THERMAL_NO_TARGET;
		}

		if (instance->target == target)
			continue;

		pid_thermal_gov_update_passive(tz, trip_type, instance->target,
					       target);
		instance->target = target;
		instance->cdev->updated = false;
	}

	list_for_each_entry(instance, &tz->thermal_instances, tz_node)
		thermal_cdev_update(instance->cdev);

	mutex_unlock(&tz->lock);

	return 0;
}

static struct thermal_governor pid_thermal_gov = {
	.name		= DRV_NAME,
	.start		= pid_thermal_gov_start,
	.stop		= pid_thermal_gov_stop,
	.throttle	= pid_thermal_gov_throttle,
	.owner		= THIS_MODULE,
};

static int __init pid_thermal_gov_init(void)
{
	return thermal_register_governor(&pid_thermal_gov);
}
fs_initcall(pid_thermal_gov_init);

static void __exit pid_thermal_gov_exit(void)
{
	thermal_unregister_governor(&pid_thermal_gov);
}
module_exit(pid_thermal_gov_exit);

MODULE_AUTHOR("Jinyoung Park <jinyoungp@nvidia.com>");
MODULE_DESCRIPTION("PID thermal governor");
MODULE_LICENSE("GPL");
