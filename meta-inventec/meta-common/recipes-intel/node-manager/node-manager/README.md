# Intel Node Manager in OpenBMC
Node Manager (NM) is an application designed to extend server's management capabilities by providing a clear out-of-band (i.e. without using host CPU/OS) interface for server's power and performance management.

NM can be used to monitor server's operating conditions. Users are able to create policies that define desired level of power consumption, temperature, airflow, or performance. Based on obtained readings, Node Manager acts on these policies by controlling power capping mechanisms of other server components (mainly host CPUs and memories).

# Building
    
    bitbake node-manager

# Configuration

The NM configuration file is located here: `/var/lib/node-manager/general.conf.json`

NM reads the file at start so to apply any changes NM restart is needed.

    systemctl restart xyz.openbmc_project.NodeManager.service

# D-Bus interface

NM provides D-Bus interface, it can be accessed under this root name `xyz.openbmc_project.NodeManager`

# IPMI interface

Supported IPMI commands:
Code|	IPMI Command
-|-
C0h|	Enable/Disable Node Manager Policy Control
C1h|	Set Node Manager Policy|
C2h|	Get Node Manager Policy
C7h|	Reset Node Manager Statistics
C8h|	Get Node Manager Statistics
C9h|	Get Node Manager Capabilities
CAh|	Get Node Manager Version
CBh|	Set Node Manager Power Draw Range
D0h|	Set Total Power Budget
D1h|	Get Total Power Budget
F2h|	Get Limiting Policy ID

# Redfish interface

> Redfish endpoints are available only when proper bmcweb patches were applied!

Redfish endpoint: `/​redfish/​v1/​Managers/​{ManagerId}/​NodeManager`

## NM watchdog
NM is working as a standard systemd service and is using service unit configuration to define watchdog.

To disable the watchdog edit NM's service configuration file,

    systemctl edit --full xyz.openbmc_project.NodeManager.service

set property *WatchdogSec=0* save the file and restart the service.

    systemctl restart xyz.openbmc_project.NodeManager.service

## Dump diagnostic data
NM can provide usefull information via D-Bus diagnostic interface, for example use this command to print the data to journal log.

    busctl call xyz.openbmc_project.NodeManager /xyz/openbmc_project/NodeManager/Diagnostics xyz.openbmc_project.NodeManager.Status DumpToLog

## Develop and test ipmi

Make the /usr folder writable and disable the openbmc watchdog.

    mkdir -p /tmp/persist/usr
    mkdir -p /tmp/persist/work/usr
    mount -t overlay -o lowerdir=/usr,upperdir=/tmp/persist/usr,workdir=/tmp/persist/work/usr overlay /usr
    touch /tmp/nowatchdog

Copy ```libzintelnmipmicmds.so.0.1.0``` to destination machine under folder ```/usr/lib/ipmid-providers``` and restart ``` phosphor-ipmi-host.service``` service.

    mv libzintelnmipmicmds.so.0.1.0 /usr/lib/ipmid-providers && systemctl restart phosphor-ipmi-host.service

IPMI logs can be displayed using this command:

    journalctl -f -u phosphor-ipmi-host -u phosphor-ipmi-host.service -o export

# Docs

743558 Intel® OpenBMC Extensions Development Guide for Birch Stream

# License
[License file](LICENSE)
