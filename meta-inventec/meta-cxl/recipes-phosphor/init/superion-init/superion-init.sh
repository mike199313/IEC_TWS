#!/bin/sh

# Init Fan setting
FAN_INIT_SH="/usr/sbin/superion-fan-init.sh"
bash $FAN_INIT_SH

# Init GPIO setting
GPIO_INIT_SH="/usr/sbin/superion-gpio-init.sh"
bash $GPIO_INIT_SH

# Read cpld verion
CPLD_CHECK="/usr/sbin/superion-cpld-init.sh"
bash $CPLD_CHECK

