#!/bin/sh
set -e

NETWORK_INTERFACE="eth0"


post_mac_conf() {
    NETWORK_FILE="00-bmc-$1.network"
    OVERRIDE_PATH="/run/systemd/network/$NETWORK_FILE.d"

    mkdir -p $OVERRIDE_PATH
    MAC_STR=`/usr/bin/mac_util r $1`

    MAC_STR=`echo ${MAC_STR//0x/}`
    MAC_STR=`echo ${MAC_STR// /:}`

    printf '[Link]\nMACAddress=%s\n' "$MAC_STR" >"$OVERRIDE_PATH"/50-mac.conf

}


for INTERFACE in $NETWORK_INTERFACE
do
    post_mac_conf $INTERFACE
done
