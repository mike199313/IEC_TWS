#!/bin/sh

# Init Fan setting
FAN_INIT_SH="/usr/sbin/superion-fan-init.sh"
bash $FAN_INIT_SH

# Init GPIO setting
gpioset `gpiofind BMC_READY`=0
echo BMC ready !!
gpioset `gpiofind RST_BMC_SGPIO`=1
echo Release reset SGPIO !!
