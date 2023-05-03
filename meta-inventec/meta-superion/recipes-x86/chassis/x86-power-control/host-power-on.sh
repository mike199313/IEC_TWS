#!/bin/sh

echo "host-power-on start"

# Wait for CPU power on
sleep 10

# bind peci driver
echo 0-30 > /sys/bus/peci/drivers/intel_peci_client/bind
echo 0-31 > /sys/bus/peci/drivers/intel_peci_client/bind

# Re-start cpusensor service
systemctl restart xyz.openbmc_project.cpusensor.service

# Re-start phosphor-pid-control
# systemctl restart phosphor-pid-control.service

