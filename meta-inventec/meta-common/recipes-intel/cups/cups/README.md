# CupsService
This component implements CUPS Monitoring for OpenBMC* Distribution (Computer 
Usage Per Second) sensors. These sensors provide utilization of CPU resources
as percentage value (0-100%).

## Capabilities
This application is implementation of CupsService proposed in designs
`[1]`.

It's responsible for:
- exposing CUPS Monitoring for OpenBMC* Distribution sensors on D-Bus using 
standard OpenBMC interface - implementing CPU discovery and monitoring for 
accurate utilization calculation
- exposing configuration APIs for sensor scanning interval, averaging and
  event generation on certain thresholds

## Architecture
Application is separated into three layers:
- D-Bus APIs for configuration and values access
- business logic - CPU Discovery and Readings procedures
  - Discovery - periodically checks for CPU configuration change over PECI. CPU
    capabilities and availability might change due to external conditions
    and host configuration.
  - Readings - periodically polling CPU performance counters and calculate the
    utilization
- PECI access - logic encapsulating producing PECI requests, transport layer
  handling

## Quick start
CUPS Monitoring for OpenBMC* Distribution Sensors should be available in the 
system as soon as they're configured in the system. For CupsService D-Bus 
API please refer to references below.

### Control
Possible in BMC console using service file: xyz.openbmc_project.CupsService.service, i.e.:
- systemctl restart xyz.openbmc_project.CupsService

### Redfish API
Redfish API for CUPS Monitoring for OpenBMC* Distribution monitoring and 
configuration is described in detail in referenced design documents. 
Please note, that these schemes are subject to change, as we are currently
in the process of pushing the schema through DMTF Redfish approval process.

It is possible to get CUPS Monitoring for OpenBMC* Distribution configuration
 by GET method and modify it by PATCH method using resource under:
- /redfish/v1/CupsService

```json
    {
        "@odata.id": "/redfish/v1/CupsService",
        "@odata.type": "#CupsService.v1_0_0.CupsService",
        "AveragingPeriod": "PT5.500S",
        "CupsSensors": {
            "@odata.id": "/redfish/v1/CupsService/Sensors"
        },
        "DynamicLoadFactors": {
            "CoreLoadFactor": null,
            "IioLoadFactor": null,
            "MemoryLoadFactor": null
        },
        "Id": "CupsService",
        "Interval": "PT0.750S",
        "LoadFactorConfiguration": "Static",
        "Name": "Cups Service",
        "StaticLoadFactors": {
            "CoreLoadFactor": 33.3,
            "IioLoadFactor": 33.3,
            "MemoryLoadFactor": 33.4
        },
        "Status": {
            "State": "Enabled"
        }
    }
```

It is also possible to get utilization from CUPS Monitoring for OpenBMC* 
Distribution sensors using resources under:
- /redfish/v1/Chassis/AC_Baseboard/Sensors/AverageCupsIndex
- /redfish/v1/Chassis/AC_Baseboard/Sensors/AverageHostMemoryBandwidthUtilization
- /redfish/v1/Chassis/AC_Baseboard/Sensors/AverageHostPciBandwidthUtilization
- /redfish/v1/Chassis/AC_Baseboard/Sensors/AverageHostCpuUtilization
- /redfish/v1/Chassis/AC_Baseboard/Sensors/CupsIndex
- /redfish/v1/Chassis/AC_Baseboard/Sensors/HostMemoryBandwidthUtilization
- /redfish/v1/Chassis/AC_Baseboard/Sensors/HostPciBandwidthUtilization
- /redfish/v1/Chassis/AC_Baseboard/Sensors/HostCpuUtilization

### CUPS Monitoring for OpenBMC* Distribution configuration
Application requires configuration in EntityManager `[3]` for sensor
to be spawned.
This configuration is required for several reasons:
- EntityManager is standarized single source of sensor configuration in OpenBMC
- ability to modify sensor configuration on the fly
- Configuration is tied to specific Inventory item with use of  `Association`
  interface `[2]`

**Please note, that in precompiled OpenBMC binary this configuration should be
present out of the box.**

#### EntityManager configuration
Following entry should be added to EntityManager configuration under chosen
inventory item (usually a Baseboard):
```json
{
    "Name": "CUPS",
    "Polling": {
        "Interval": 1000,
        "AveragingPeriod": 10000,
        "LoadFactorConfiguration": "Dynamic",
        "CoreLoadFactor": 33.3,
        "IioLoadFactor": 33.3,
        "MemoryLoadFactor":33.3
    },
    "Type": "CupsSensor"
}
```
, where both Interval and AveragingPeriod should be provided in milliseconds
and comply with boundary values specified in the design documents.

CupsService monitors EntityManager for configuration updates (through D-Bus
`PropertiesChanged` signal), and applies it at runtime.

#### Example
EntityManager configuration file could contain given entry in following JSON
(limited to core fields)
```json
{
    "Exposes": [
      {
        "Name": "CUPS",
        "Polling": {
            "Interval": 1000,
            "AveragingPeriod": 10000,
            "LoadFactorConfiguration": "Dynamic",
            "CoreLoadFactor": 33.3,
            "IioLoadFactor": 33.3,
            "MemoryLoadFactor": 33.3
        },
        "Type": "CupsSensor"
      }
    ],
    "Name": "MySystem",
    "Type": "Board"
}
```

Based on that configuration EntityManager would expose configuration as follows:
```
root@obmc:~# busctl introspect xyz.openbmc_project.EntityManager /xyz/openbmc_project/inventory/system/board/MySystem/CUPS

NAME                                                 TYPE      SIGNATURE RESULT/VALUE
xyz.openbmc_project.Configuration.CupsSensor         interface -         -
.Name                                                property  s         "CUPS"
.Type                                                property  s         "CupsSensor"
xyz.openbmc_project.Configuration.CupsSensor.Polling interface -         -
.Delete                                              method    -         -
.AveragingPeriod                                     property  d         10000
.Interval                                            property  d         1000
.LoadFactorConfiguration                             property  s         "Dynamic"
.CoreLoadFactor                                      property  d         33.3
.IioLoadFactor                                       property  d         33.3
.MemoryLoadFactor                                    property  d         33.3
```

## References
1. 743558 Intel® OpenBMC Extensions Development Guide for Birch Stream
2. [OpenBMC Sensor architecture](https://github.com/openbmc/docs/blob/master/architecture/sensor-architecture.md)
3. [EntityManager](https://github.com/openbmc/entity-manager/blob/master/docs/my_first_sensors.md)
