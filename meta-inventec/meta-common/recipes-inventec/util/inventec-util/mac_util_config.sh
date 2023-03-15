#!/bin/sh
name=${1##*_}
mac=${1%%_*}
OVERRIDE_PATH="/run/systemd/network/00-bmc-$name.network.d"

/usr/bin/mac_util w $name $mac
printf '[Link]\nMACAddress=%s\n' "$mac" >| "$OVERRIDE_PATH"/50-mac.conf
