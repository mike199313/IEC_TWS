#!/bin/sh

# Init fan setting
FAN_DRIVER_NAME=max31790
FAN_BUS_NUM=21
FAN_ADDR1=20
FAN_ADDR2=21
FAN_ENABLE_NUM=6
FAN_PWM_NUM=3
# duty: 80/255 ~= 30%
PWM_INITIAL_VALUE=80

# Enable the fan_tach register 0x02 to 0x07
function enable_fan_tach() {
    for (( fun_i=1; fun_i <= $FAN_ENABLE_NUM; fun_i++))
    do
        i2cset -y -f $FAN_BUS_NUM 0x$FAN_ADDR1 0x0$((fun_i+1)) 0x48
        i2cset -y -f $FAN_BUS_NUM 0x$FAN_ADDR2 0x0$((fun_i+1)) 0x48
    done
    echo Fan enable setting is done !!
}

# Init fan pwm
FAN1_DRIVER_PATH=/sys/bus/i2c/drivers/$FAN_DRIVER_NAME/$FAN_BUS_NUM-00$FAN_ADDR1
FAN2_DRIVER_PATH=/sys/bus/i2c/drivers/$FAN_DRIVER_NAME/$FAN_BUS_NUM-00$FAN_ADDR2
function init_fan_pwm() {
    for (( fun_i=1; fun_i <= $FAN_PWM_NUM ; fun_i++))
    do
        echo $PWM_INITIAL_VALUE > $FAN1_DRIVER_PATH/hwmon/hwmon*/pwm$fun_i
        echo $PWM_INITIAL_VALUE > $FAN2_DRIVER_PATH/hwmon/hwmon*/pwm$fun_i
    done
    echo Fan pwm initial value setting is done !!
}

# Wait for driver binding
RETRY_TIMES=10
for (( i=0; i < $RETRY_TIMES; i++))
do
    if [ -d "$FAN1_DRIVER_PATH" ] && [ -d "$FAN2_DRIVER_PATH" ]
    then
        enable_fan_tach
        init_fan_pwm
        echo Fan initial setting is done !!
        break
    else
        sleep 1
    fi
done
