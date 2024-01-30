#!/bin/sh

echo "host-power-off start"

# bind peci driver
echo 0-30 > /sys/bus/peci/drivers/intel_peci_client/unbind
echo 0-31 > /sys/bus/peci/drivers/intel_peci_client/unbind

# Re-start cpusensor service
# systemctl restart xyz.openbmc_project.cpusensor.service

# Re-start phosphor-pid-control
# systemctl stop phosphor-pid-control.service

