#!/bin/sh

# Init Fan setting
FAN_INIT_SH="/usr/sbin/superion-fan-init.sh"
bash $FAN_INIT_SH

# Init GPIO setting
GPIO_INIT_SH="/usr/sbin/superion-gpio-init.sh"
bash $GPIO_INIT_SH


# Init VGPIO direction
devmem 0x1E6EE0C0 w 0xf

