#!/bin/sh

SHUTDOWN_FILE="/var/log/SHUTDOWN"

if [ -f $SHUTDOWN_FILE ]; then
    echo "^-- Reboot --"
    rm $SHUTDOWN_FILE
else
    echo "Abnormal shutdown detected."
fi
