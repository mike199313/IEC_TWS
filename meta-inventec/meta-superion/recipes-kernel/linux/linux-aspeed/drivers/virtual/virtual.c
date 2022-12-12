/*
*  Inventec virtual hwmon driver
*
*/


#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/regmap.h>
#include <linux/util_macros.h>
#include "virtual.h"


static virtual_hwmon_temp_params_t device_params_temp = {
	.value1 = 0,
	.value2 = 0,
	.value3 = 0,
	.value4 = 0,
	.value5 = 0,
	.value6 = 0,
	.value7 = 0,
	.value8 = 0,
	.value9 = 0,
};

static virtual_hwmon_eeprom_params_t device_params_eeprom = {
	.data = {
			0x01, 0x00, 0x01, 0x04, 0x0c, 0x12, 0x00, 0xdc, 0x01, 0x03, 0x1d, 0xc3, 0x4e, 0x2f, 0x41, 0xc3,
			0x4e, 0x2f, 0x41, 0xc3, 0x4e, 0x2f, 0x41, 0xc3, 0x4e, 0x2f, 0x41, 0xc1, 0x00, 0x00, 0x00, 0x1a,
			0x01, 0x08, 0x19, 0xb0, 0xb2, 0xc1, 0xca, 0x49, 0x6e, 0x76, 0x65, 0x6e, 0x74, 0x65, 0x63, 0x20,
			0x20, 0xc5, 0x53, 0x43, 0x4d, 0x20, 0x20, 0xca, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
			0x20, 0x20, 0xcc, 0x31, 0x33, 0x39, 0x35, 0x41, 0x33, 0x32, 0x30, 0x39, 0x33, 0x30, 0x31, 0xc9,
			0x46, 0x52, 0x55, 0x20, 0x76, 0x30, 0x2e, 0x30, 0x39, 0xc4, 0x41, 0x20, 0x20, 0x20, 0xc1, 0x09,
			0x01, 0x06, 0x19, 0xc9, 0x49, 0x6e, 0x76, 0x65, 0x6e, 0x74, 0x65, 0x63, 0x20, 0xc3, 0x53, 0x43,
			0x4d, 0xc3, 0x4e, 0x2f, 0x41, 0xc3, 0x4e, 0x2f, 0x41, 0xc3, 0x4e, 0x2f, 0x41, 0xc3, 0x4e, 0x2f,
			0x41, 0xc2, 0x30, 0x20, 0xc3, 0x4e, 0x2f, 0x41, 0xc3, 0x4e, 0x2f, 0x41, 0xc1, 0x00, 0x00, 0x3c,	
		}
};

static virtual_hwmon_psu_params_t device_params_psu = {
	.in1_input = 0,
	.in2_input = 0,
	.curr1_input = 0,
	.curr2_input = 0,
	.power1_input = 0,
	.power2_input = 0,
	.fan1_input = 0,
	.fan2_input = 0,
	.pwm1_input = 0,
};

static virtual_hwmon_fan_params_t device_params_fan = {
	.fan1_input = 0,
	.fan2_input = 0,
	.fan3_input = 0,
	.fan4_input = 0,
	.fan1_target = 0,
	.fan2_target = 0,
	.fan3_target = 0,
	.fan4_target = 0,
	.fan1_fault = 0,
	.fan2_fault = 0,
	.fan3_fault = 0,
	.fan4_fault = 0,
	.pwm1_input = 0,
	.pwm2_input = 0,
	.pwm1_enable = 0,
	.pwm2_enable = 0,
};


static const struct i2c_device_id virtual_hwmon_ids[] = {
	{ "virtual_temp", VIRTUAL_TEMP, },
	{ "virtual_eeprom", VIRTUAL_EEPROM, },
	{ "virtual_psu", VIRTUAL_PSU, },
	{ "virtual_fan", VIRTUAL_FAN, },
	{ /* LIST END */ }
};
MODULE_DEVICE_TABLE(i2c, virtual_hwmon_ids);

static const struct of_device_id __maybe_unused virtual_hwmon_of_match[] = {
	{
		.compatible = "inventec,virtual_temp",
		.data = (void *)VIRTUAL_TEMP
	},
	{
		.compatible = "inventec,virtual_eeprom",
		.data = (void *)VIRTUAL_EEPROM
	},
	{
		.compatible = "inventec,virtual_psu",
		.data = (void *)VIRTUAL_PSU
	},
	{
		.compatible = "inventec,virtual_fan",
		.data = (void *)VIRTUAL_FAN
	},
	{ },
};
MODULE_DEVICE_TABLE(of, virtual_hwmon_of_match);




static umode_t virtual_hwmon_is_visible(const void *data, enum hwmon_sensor_types type,
			       u32 attr, int channel)
{
	switch (type) {
	case hwmon_temp:
		switch (attr) {
		case hwmon_temp_input:
			return 0644;
		}
		break;
	case hwmon_in:
		switch (attr) {
		case hwmon_in_input:
			return 0644;
		case hwmon_in_label:
			return 0444;
		}
		break;
	case hwmon_curr:
		switch (attr) {
		case hwmon_curr_input:
			return 0644;
		case hwmon_curr_label:
			return 0444;
		}
		break;
	case hwmon_power:
		switch (attr) {
		case hwmon_power_input:
			return 0644;
		case hwmon_power_label:
			return 0444;
		}
		break;
	case hwmon_fan:
		switch (attr) {
		case hwmon_fan_input:
		case hwmon_fan_label:
			return 0444;
		case hwmon_fan_target:
		case hwmon_fan_fault:
			return 0644;
		}
		break;
	case hwmon_pwm:
		switch (attr) {
		case hwmon_pwm_input:
		case hwmon_pwm_enable:
			return 0644;
		}
		break;
	default:
		break;
	}
	return 0;
}


static int virtual_hwmon_temp_read(struct device *dev, enum hwmon_sensor_types type,
		     u32 attr, int channel, long *val)
{
	virtual_hwmon_data_t *data = dev_get_drvdata(dev);
	virtual_hwmon_temp_params_t *para = data->params;

	switch (attr) {
	case hwmon_temp_input:
		switch(channel) {
		case 0:
			*val = para->value1;
			break;
		case 1:
			*val = para->value2;
			break;
		case 2:
			*val = para->value3;
			break;
		case 3:
			*val = para->value4;
			break;
		case 4:
			*val = para->value5;
			break;
		case 5:
			*val = para->value6;
			break;
		case 6:
			*val = para->value7;
			break;
		case 7:
			*val = para->value8;
			break;
		case 8:
			*val = para->value9;
			break;
		default:
			dev_info(dev, "hwmon temp not support channel %d\n", channel);
			return -EINVAL;
		}
		break;
	default:
		dev_info(dev, "hwmon temp not support attr %d\n", attr);
		return -EINVAL;
	}

	return 0;
}


static int virtual_hwmon_temp_write(struct device *dev, enum hwmon_sensor_types type,
		      u32 attr, int channel, long val)
{
	virtual_hwmon_data_t *data = dev_get_drvdata(dev);
	virtual_hwmon_temp_params_t *para = data->params;


	switch (attr) {
	case hwmon_temp_input:
		switch(channel) {
		case 0:
			para->value1 = val;
			break;
		case 1:
			para->value2 = val;
			break;
		case 2:
			para->value3 = val;
			break;
		case 3:
			para->value4 = val;
			break;
		case 4:
			para->value5 = val;
			break;
		case 5:
			para->value6 = val;
			break;
		case 6:
			para->value7 = val;
			break;
		case 7:
			para->value8 = val;
			break;
		case 8:
			para->value9 = val;
			break;
		default:
			dev_info(dev, "hwmon temp not support channel %d\n", channel);
			return -EINVAL;
		}
		break;
	default:
		dev_info(dev, "hwmon temp not support attr %d\n", attr);
		return -EINVAL;
	}
	return 0;
}


static ssize_t virtual_eeprom_read(struct file *filp, struct kobject *kobj,
			   struct bin_attribute *bin_attr,
			   char *buf, loff_t off, size_t count)
{
	struct i2c_client *client = to_i2c_client(kobj_to_dev(kobj));
	virtual_hwmon_data_t *data = i2c_get_clientdata(client);
	virtual_hwmon_eeprom_params_t *para = data->params;

	memcpy(buf, &para->data[off], count);

	return count;
}


static ssize_t virtual_eeprom_write(struct file *filp, struct kobject *kobj,
			   struct bin_attribute *bin_attr,
			   char *buf, loff_t off, size_t count)
{
	struct i2c_client *client = to_i2c_client(kobj_to_dev(kobj));
	virtual_hwmon_data_t *data = i2c_get_clientdata(client);
	virtual_hwmon_eeprom_params_t *para = data->params;

	memcpy(&para->data[off],buf, count);

	return count;
}

static int virtual_pwm_to_main_fan_input(u8 pwm, long *main_input)
{
	if(pwm == 0) {
		*main_input = 0;
	} else {
		*main_input = 1000 + pwm * 100;
	}
	return 0;
}

static int virtual_pwm_to_sub_fan_input(u8 pwm, long *sub_input)
{
	if(pwm == 0) {
		*sub_input = 0;
	} else {
		*sub_input = 500 + pwm * 60;
	}
	return 0;
}

static int virtual_hwmon_psu_read(struct device *dev, enum hwmon_sensor_types type,
		     u32 attr, int channel, long *val)
{
	virtual_hwmon_data_t *data = dev_get_drvdata(dev);
	virtual_hwmon_psu_params_t *para = data->params;

	switch (type) {
	case hwmon_in:
		switch (attr) {
		case hwmon_in_input:
			switch(channel) {
			case 0:
				*val = para->in1_input;
				break;
			case 1:
				*val = para->in2_input;
				break;
			case 2:
				*val = para->in3_input;
				break;
			default:
				dev_info(dev, "hwmon psu not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		default:
			dev_info(dev, "hwmon psu not support attr %d\n", attr);
			return -EINVAL;
		}
		break;
	case hwmon_curr:
		switch (attr) {
		case hwmon_curr_input:
			switch(channel) {
			case 0:
				*val = para->curr1_input;
				break;
			case 1:
				*val = para->curr2_input;
				break;
			default:
				dev_info(dev, "hwmon psu not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		default:
			dev_info(dev, "hwmon psu not support attr %d\n", attr);
			return -EINVAL;
		}
		break;
	case hwmon_power:
		switch (attr) {
		case hwmon_power_input:
			switch(channel) {
			case 0:
				*val = para->power1_input;
				break;
			case 1:
				*val = para->power2_input;
				break;
			default:
				dev_info(dev, "hwmon psu not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		default:
			dev_info(dev, "hwmon psu not support attr %d\n", attr);
			return -EINVAL;
		}
		break;
	case hwmon_fan:
		switch (attr) {
		case hwmon_fan_input:
			switch(channel) {
			case 0:
				return virtual_pwm_to_main_fan_input( para->pwm1_input, val);
			case 1:
				return virtual_pwm_to_sub_fan_input( para->pwm1_input, val);
			default:
				dev_info(dev, "hwmon psu not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		default:
			dev_info(dev, "hwmon psu not support attr %d\n", attr);
			return -EINVAL;
		}
		break;
	case hwmon_pwm:
		switch (attr) {
		case hwmon_pwm_input:
			switch(channel) {
			case 0:
				*val = para->pwm1_input;
				break;
			default:
				dev_info(dev, "hwmon psu not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		default:
			dev_info(dev, "hwmon psu not support attr %d\n", attr);
			return -EINVAL;
		}
		break;
	case hwmon_temp:
		switch (attr) {
		case hwmon_temp_input:
			switch(channel) {
			case 0:
				*val = para->temp1_input;
				break;
			default:
				dev_info(dev, "hwmon psu not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		default:
			dev_info(dev, "hwmon psu not support attr %d\n", attr);
			return -EINVAL;
		}
		break;
	default:
		dev_info(dev, "hwmon psu not support type %d\n", type);
		return -EINVAL;
	}

	return 0;
}




static const char *const virtual_hwmon_psu_in_labels[] = {
	"pending",
	"vin",
	"vout1",
};

static const char *const virtual_hwmon_psu_curr_labels[] = {
	"iout1",
	"pending",
};

static const char *const virtual_hwmon_psu_power_labels[] = {
	"pin",
	"pending",
};

static int virtual_hwmon_psu_read_string(struct device *dev, enum hwmon_sensor_types type, u32 attr,
		    int channel, const char **str)
{
	switch (type) {
	case hwmon_in:
		switch (attr) {
		case hwmon_in_label:
			*str = virtual_hwmon_psu_in_labels[channel];
			return 0;
		default:
			return -EOPNOTSUPP;
		}
		break;
	case hwmon_curr:
		switch (attr) {
		case hwmon_curr_label:
			*str = virtual_hwmon_psu_curr_labels[channel];
			return 0;
		default:
			return -EOPNOTSUPP;
		}
		break;
	case hwmon_power:
		switch (attr) {
		case hwmon_power_label:
			*str = virtual_hwmon_psu_power_labels[channel];
			return 0;
		default:
			return -EOPNOTSUPP;
		}
		break;
	default:
		return -EOPNOTSUPP;
	}

	return -EOPNOTSUPP;
}


static int virtual_hwmon_psu_write(struct device *dev, enum hwmon_sensor_types type,
		      u32 attr, int channel, long val)
{
	virtual_hwmon_data_t *data = dev_get_drvdata(dev);
	virtual_hwmon_psu_params_t *para = data->params;

	switch (type) {
	case hwmon_in:
		switch (attr) {
		case hwmon_in_input:
			switch(channel) {
			case 0:
				para->in1_input = val;
				break;
			case 1:
				para->in2_input = val;
				break;
			case 2:
				para->in3_input = val;
				break;
			default:
				dev_info(dev, "hwmon psu not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		default:
			dev_info(dev, "hwmon psu not support attr %d\n", attr);
			return -EINVAL;
		}
		break;
	case hwmon_curr:
		switch (attr) {
		case hwmon_curr_input:
			switch(channel) {
			case 0:
				para->curr1_input = val;
				break;
			case 1:
				para->curr2_input = val;
				break;
			default:
				dev_info(dev, "hwmon psu not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		default:
			dev_info(dev, "hwmon psu not support attr %d\n", attr);
			return -EINVAL;
		}
		break;
	case hwmon_power:
		switch (attr) {
		case hwmon_power_input:
			switch(channel) {
			case 0:
				para->power1_input = val;
				break;
			case 1:
				para->power2_input = val;
				break;
			default:
				dev_info(dev, "hwmon psu not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		default:
			dev_info(dev, "hwmon psu not support attr %d\n", attr);
			return -EINVAL;
		}
		break;
	case hwmon_fan:
		switch (attr) {
		case hwmon_fan_input:
			dev_info(dev, "Please use pwm to set fan input\n");
			break;
		default:
			dev_info(dev, "hwmon psu not support attr %d\n", attr);
			return -EINVAL;
		}
		break;
	case hwmon_pwm:
		switch (attr) {
		case hwmon_pwm_input:
			switch(channel) {
			case 0:
				para->pwm1_input = val;
				break;
			default:
				dev_info(dev, "hwmon psu not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		default:
			dev_info(dev, "hwmon psu not support attr %d\n", attr);
			return -EINVAL;
		}
		break;
	case hwmon_temp:
		switch (attr) {
		case hwmon_temp_input:
			switch(channel) {
			case 0:
				para->temp1_input = val;
				break;
			default:
				dev_info(dev, "hwmon psu not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		default:
			dev_info(dev, "hwmon psu not support attr %d\n", attr);
			return -EINVAL;
		}
		break;
	default:
		dev_info(dev, "hwmon psu not support type %d\n", type);
		return -EINVAL;
	}

	return 0;
}


static int virtual_hwmon_fan_read(struct device *dev, enum hwmon_sensor_types type,
		     u32 attr, int channel, long *val)
{
	virtual_hwmon_data_t *data = dev_get_drvdata(dev);
	virtual_hwmon_fan_params_t *para = data->params;

	switch (type) {
	case hwmon_fan:
		switch (attr) {
		case hwmon_fan_input:
			switch(channel) {
			case 0:
				return virtual_pwm_to_main_fan_input( para->pwm1_input, val);
			case 1:
				return virtual_pwm_to_sub_fan_input( para->pwm1_input, val);
			case 2:
				return virtual_pwm_to_main_fan_input( para->pwm2_input, val);
			case 3:
				return virtual_pwm_to_sub_fan_input( para->pwm2_input, val);
			default:
				dev_info(dev, "hwmon fan not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		case hwmon_fan_target:
			switch(channel) {
			case 0:
				*val = para->fan1_target;
				break;
			case 1:
				*val = para->fan2_target;
				break;
			case 2:
				*val = para->fan3_target;
				break;
			case 3:
				*val = para->fan4_target;
				break;
			default:
				dev_info(dev, "hwmon fan not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		case hwmon_fan_fault:
			switch(channel) {
			case 0:
				*val = para->fan1_fault;
				break;
			case 1:
				*val = para->fan2_fault;
				break;
			case 2:
				*val = para->fan3_fault;
				break;
			case 3:
				*val = para->fan4_fault;
				break;
			default:
				dev_info(dev, "hwmon fan not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		default:
			dev_info(dev, "hwmon fan not support attr %d\n", attr);
			return -EINVAL;
		}
		break;
	case hwmon_pwm:
		switch (attr) {
		case hwmon_pwm_input:
			switch(channel) {
			case 0:
				*val = para->pwm1_input;
				break;
			case 1:
				*val = para->pwm2_input;
				break;
			default:
				dev_info(dev, "hwmon fan not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		case hwmon_pwm_enable:
			switch(channel) {
			case 0:
				*val = para->pwm1_enable;
				break;
			case 1:
				*val = para->pwm2_enable;
				break;
			default:
				dev_info(dev, "hwmon fan not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		default:
			dev_info(dev, "hwmon fan not support attr %d\n", attr);
			return -EINVAL;
		}
		break;
	default:
		dev_info(dev, "hwmon fan not support type %d\n", type);
		return -EINVAL;
	}

	return 0;
}

static int virtual_hwmon_fan_write(struct device *dev, enum hwmon_sensor_types type,
		      u32 attr, int channel, long val)
{
	virtual_hwmon_data_t *data = dev_get_drvdata(dev);
	virtual_hwmon_fan_params_t *para = data->params;

	switch (type) {
	case hwmon_fan:
		switch (attr) {
		case hwmon_fan_input:
			dev_info(dev, "Please use pwm to set fan input\n");
			break;
		case hwmon_fan_target:
			switch(channel) {
			case 0:
				para->fan1_target = val;
				break;
			case 1:
				para->fan2_target = val;
				break;
			case 2:
				para->fan3_target = val;
				break;
			case 3:
				para->fan4_target = val;
				break;
			default:
				dev_info(dev, "hwmon fan not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		case hwmon_fan_fault:
			switch(channel) {
			case 0:
				para->fan1_fault = val;
				break;
			case 1:
				para->fan2_fault = val;
				break;
			case 2:
				para->fan3_fault = val;
				break;
			case 3:
				para->fan4_fault = val;
				break;
			default:
				dev_info(dev, "hwmon fan not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		default:
			dev_info(dev, "hwmon fan not support attr %d\n", attr);
			return -EINVAL;
		}
		break;
	case hwmon_pwm:
		switch (attr) {
		case hwmon_pwm_input:
			switch(channel) {
			case 0:
				para->pwm1_input = val;
				break;
			case 1:
				para->pwm2_input = val;
				break;
			default:
				dev_info(dev, "hwmon psu not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		case hwmon_pwm_enable:
			switch(channel) {
			case 0:
				para->pwm1_enable = val;
				break;
			case 1:
				para->pwm2_enable = val;
				break;
			default:
				dev_info(dev, "hwmon psu not support channel %d\n", channel);
				return -EINVAL;
			}
			break;
		default:
			dev_info(dev, "hwmon psu not support attr %d\n", attr);
			return -EINVAL;
		}
		break;
	default:
		dev_info(dev, "hwmon psu not support type %d\n", type);
		return -EINVAL;
	}

	return 0;
}

/*************************/

static const struct hwmon_channel_info *virtual_hwmon_temp_info[] = {
	HWMON_CHANNEL_INFO(temp,
			   HWMON_T_INPUT,
			   HWMON_T_INPUT,
			   HWMON_T_INPUT,
			   HWMON_T_INPUT,
			   HWMON_T_INPUT,
			   HWMON_T_INPUT,
			   HWMON_T_INPUT,
			   HWMON_T_INPUT,
			   HWMON_T_INPUT),
	NULL
};

static const struct hwmon_ops virtual_hwmon_temp_ops = {
	.is_visible = virtual_hwmon_is_visible,
	.read = virtual_hwmon_temp_read,
	.write = virtual_hwmon_temp_write,
};

static const struct hwmon_chip_info virtual_hwmon_temp_chip_info = {
	.ops = &virtual_hwmon_temp_ops,
	.info = virtual_hwmon_temp_info,
};


static const struct bin_attribute virtual_eeprom_attr = {
	.attr = {
		.name = "eeprom",
		.mode = S_IRUGO|S_IWUGO ,
	},
	.size = VIRTUAL_EEPROM_SIZE,
	.read = virtual_eeprom_read,
	.write = virtual_eeprom_write,
};


static const struct hwmon_channel_info *virtual_hwmon_psu_info[] = {
	HWMON_CHANNEL_INFO(in,
			   HWMON_I_INPUT|HWMON_I_LABEL,
			   HWMON_I_INPUT|HWMON_I_LABEL,
			   HWMON_I_INPUT|HWMON_I_LABEL),
	HWMON_CHANNEL_INFO(curr,
			   HWMON_C_INPUT|HWMON_C_LABEL,
			   HWMON_C_INPUT|HWMON_C_LABEL),
	HWMON_CHANNEL_INFO(power,
			   HWMON_P_INPUT|HWMON_P_LABEL,
			   HWMON_P_INPUT|HWMON_P_LABEL),
	HWMON_CHANNEL_INFO(fan,
			   HWMON_F_INPUT,
			   HWMON_F_INPUT),
	HWMON_CHANNEL_INFO(pwm,
			   HWMON_PWM_INPUT),
	HWMON_CHANNEL_INFO(temp,
			   HWMON_PWM_INPUT),
	NULL
};

static const struct hwmon_ops virtual_hwmon_psu_ops = {
	.is_visible = virtual_hwmon_is_visible,
	.read = virtual_hwmon_psu_read,
	.read_string = virtual_hwmon_psu_read_string,
	.write = virtual_hwmon_psu_write,
};

static const struct hwmon_chip_info virtual_hwmon_psu_chip_info = {
	.ops = &virtual_hwmon_psu_ops,
	.info = virtual_hwmon_psu_info,
};

static const struct hwmon_channel_info *virtual_hwmon_fan_info[] = {
	HWMON_CHANNEL_INFO(fan,
			   HWMON_F_INPUT|HWMON_F_TARGET|HWMON_F_FAULT,
			   HWMON_F_INPUT|HWMON_F_TARGET|HWMON_F_FAULT,
			   HWMON_F_INPUT|HWMON_F_TARGET|HWMON_F_FAULT,
			   HWMON_F_INPUT|HWMON_F_TARGET|HWMON_F_FAULT),
	HWMON_CHANNEL_INFO(pwm,
			   HWMON_PWM_INPUT|HWMON_PWM_ENABLE,
			   HWMON_PWM_INPUT|HWMON_PWM_ENABLE),
	NULL
};

static const struct hwmon_ops virtual_hwmon_fan_ops = {
	.is_visible = virtual_hwmon_is_visible,
	.read = virtual_hwmon_fan_read,
	.write = virtual_hwmon_fan_write,
};

static const struct hwmon_chip_info virtual_hwmon_fan_chip_info = {
	.ops = &virtual_hwmon_fan_ops,
	.info = virtual_hwmon_fan_info,
};


/*******
  Remove
*******/

static int virtual_hwmon_remove(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	virtual_hwmon_data_t *data = dev_get_drvdata(dev);
	int err = 0;

	switch(data->kind)
	{
	case VIRTUAL_TEMP:
		break;
	case VIRTUAL_EEPROM:
		sysfs_remove_bin_file(&client->dev.kobj, &virtual_eeprom_attr);
		break;
	case VIRTUAL_PSU:
		break;
	case VIRTUAL_FAN:
		break;
	default:
		dev_info(dev, "sensor '%s' not support kind %d\n", client->name, data->kind);
		err = -EINVAL;
		break;
	}

	dev_info(dev, "sensor '%s'\n", client->name);

	return err;
}


/*******
  Probe
*******/

static int
virtual_temp_probe(struct i2c_client *client )
{
	struct device *dev = &client->dev;
	struct device *hwmon_dev;
	virtual_hwmon_data_t *data;
	int err;

	err = 0;

	data = devm_kzalloc(dev, sizeof(virtual_hwmon_data_t), GFP_KERNEL);
	if (!data) {
		return -ENOMEM;
	}

	data->client = client;
	data->kind = VIRTUAL_TEMP;
	data->params = &device_params_temp;

	hwmon_dev = devm_hwmon_device_register_with_info(dev, client->name,
							 data, &virtual_hwmon_temp_chip_info,
							 NULL);

	if (IS_ERR(hwmon_dev))
		return PTR_ERR(hwmon_dev);

	dev_info(dev, "%s: sensor '%s'\n", dev_name(hwmon_dev), client->name);

	return err;
}

static int
virtual_eeprom_probe(struct i2c_client *client )
{
	struct device *dev = &client->dev;
	virtual_hwmon_data_t *data;

	data = devm_kzalloc(dev, sizeof(virtual_hwmon_data_t), GFP_KERNEL);
	if (!data) {
		return -ENOMEM;
	}

	data->client = client;
	data->kind = VIRTUAL_EEPROM;
	data->params = &device_params_eeprom;

	i2c_set_clientdata(client, data);

	/* create the sysfs eeprom file */
	return sysfs_create_bin_file(&client->dev.kobj, &virtual_eeprom_attr);
}


static int
virtual_psu_probe(struct i2c_client *client )
{
	struct device *dev = &client->dev;
	struct device *hwmon_dev;
	virtual_hwmon_data_t *data;
	int err;

	err = 0;

	data = devm_kzalloc(dev, sizeof(virtual_hwmon_data_t), GFP_KERNEL);
	if (!data) {
		return -ENOMEM;
	}

	data->client = client;
	data->kind = VIRTUAL_PSU;
	data->params = &device_params_psu;

	hwmon_dev = devm_hwmon_device_register_with_info(dev, client->name,
							 data, &virtual_hwmon_psu_chip_info,
							 NULL);

	if (IS_ERR(hwmon_dev))
		return PTR_ERR(hwmon_dev);

	dev_info(dev, "%s: sensor '%s'\n", dev_name(hwmon_dev), client->name);

	return err;
}

static int
virtual_fan_probe(struct i2c_client *client )
{
	struct device *dev = &client->dev;
	struct device *hwmon_dev;
	virtual_hwmon_data_t *data;
	int err;

	err = 0;

	data = devm_kzalloc(dev, sizeof(virtual_hwmon_data_t), GFP_KERNEL);
	if (!data) {
		return -ENOMEM;
	}

	data->client = client;
	data->kind = VIRTUAL_FAN;
	data->params = &device_params_fan;

	hwmon_dev = devm_hwmon_device_register_with_info(dev, client->name,
							 data, &virtual_hwmon_fan_chip_info,
							 NULL);

	if (IS_ERR(hwmon_dev))
		return PTR_ERR(hwmon_dev);

	dev_info(dev, "%s: sensor '%s'\n", dev_name(hwmon_dev), client->name);

	return err;
}


static int
virtual_hwmon_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	virtual_hwmon_type_enum kind;
	int err;

	err = 0;

	if (client->dev.of_node)
		kind = (virtual_hwmon_type_enum)of_device_get_match_data(&client->dev);
	else
		kind = id->driver_data;

	switch (kind) {
	case VIRTUAL_TEMP:
		err = virtual_temp_probe(client);
		break;
	case VIRTUAL_EEPROM:
		err = virtual_eeprom_probe(client);
		break;
	case VIRTUAL_PSU:
		err = virtual_psu_probe(client);
		break;
	case VIRTUAL_FAN:
		err = virtual_fan_probe(client);
		break;
	default:
		dev_info(dev, "sensor '%s' not support kind %d\n", client->name, kind);
		err = -EINVAL;
		break;
	}

	return err;
}


static struct i2c_driver virtual_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "virtual",
		.of_match_table = of_match_ptr(virtual_hwmon_of_match),
	},
	.probe		= virtual_hwmon_probe,
	.remove     = virtual_hwmon_remove,
	.id_table	= virtual_hwmon_ids,
};

module_i2c_driver(virtual_driver);


MODULE_AUTHOR("Inventec");
MODULE_DESCRIPTION("virtual hwmon driver");
MODULE_LICENSE("GPL");

