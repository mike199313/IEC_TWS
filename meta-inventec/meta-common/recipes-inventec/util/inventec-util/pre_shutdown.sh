#!/bin/sh

SHUTDOWN_FILE="/var/log/SHUTDOWN"

if [ ! -f $SHUTDOWN_FILE ]; then
    touch $SHUTDOWN_FILE
fi
