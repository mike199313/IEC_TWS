# Code overview
This document provides general understanding of telemetry code structure and
internal workflows.

It also provides basic understanding undersanding of classes responsibilities
and expectations. In particular - it lists all features which are still missing
from original design.

Please refer to README.md for links to high level documentation.

## File structure
###  \
*   `.clang-format` - configuration file to clang-format tool, compatible with
        latest OpenBMC standards
*   `xyz.openbmc_project.CupsService.service` - systemd service file,
        responsible for configuring system to start application at boot time
*   `.gitignore` - contains paths and files which should not be commited to
        repository (intermediate files, binaries, generated headers etc.)
*   `CMakeLists.txt` - main Cmake file
*   `README.md` `OVERVIEW.md` `LICENSE` `MAINTAINERS` - project documentation


### \cmake
Contains files used by CMake build system. Those files are logically separated
by specific purpose.
*   `\modules` - helpers for CMake to aid in loading external libraries
*   `dependencies.cmake` - contains information about project dependencies and
        'recipies' to build them alongisde the project
*   `flags.cmake` - specifies compilation flags
*   `ut.cmake` - provides facilities integrating project with GoogleTest unit
        test environment

### \scripts
Contains support tools which can be used to test and evaluate CUPS functionality
on the system.
* `cups.py` - showcasing logic of CPU discovery and performance counters
polling. Can be used as a reference code to implement core CUPS functionality
(monitoring Core, Memory and IIO utilization)

### \src
#### \src\base
Contains core application logic and classes. This is core of the application
with general (abstract) classes where implementation-specific details should
be implemented.
* `discovery.hpp`
  * `class CupsDiscovery` - responsible for monitoring CPU population,
periodically triggers Discovery procedure, detecting changes in CPU
configuration
* `readings.hpp`
  * `class CupsReadings` - responsible for monitoring CPU performance counters,
periodically triggers polling of CPU metrics and updates the sensors
* `sensor.hpp`
  * `class Sensor` - abstracts one of the CPU metrics, is updated by Readings
procedures
* `service.hpp`
  * `class CupsService` - bounds together Discovery and Readings procedures,
spawns Sensors and glues everything together

#### \src\dbus
Contains D-Bus related definitions and wrappers. By principle no extra logic is
added apart from the logic of exposing proper D-Bus APIs.
* `dbus.hpp` - contains basic types, constants and conversion functions
(`base`<->`dbus`)
* `sensor.hpp`
  * `class Sensor` - decorator of `base::Sensor`, exposing it over D-Bus
* `service.hpp`
  * `class CupsService` - decorator of `base::Service`, exposing it over D-Bus

#### \src\peci
Contains PECI realted definitions and functions. Contains logic used to
communicate with CPU and calculate utilization based on various performance
counters.
* `abi.hpp` - contains PECI transport definitions, constants, enumerations and
request/response structures
* `metrics\types.hpp` - defines main types and specializations to be used by
CupsService
* `metrics\utilization.hpp` - generic class responsible for calculating
percentage utilization of CPU resource
* `metrics\impl\*` - generic classes implementing support for CPU communication
  * `class Core<>, Iio<>, Memory<>` - objects encapsulating CPU data and logic
related to retrieving performance counter values. Used by Readings procedures
  * `class CoreFactory<>, IioFactory<>, MemoryFactory<>` - objects ecapsulating
data and logic used to retrieve information about CPU. Used by Discovery
procedures
* `transport\adapter.hpp`
  * `class Adapter<>` - abstract wrapper for PECI driver, provides
convenient getters and setters for various CPU registers. It abstracts higher
level logic from transport layer.
* `transport\adapter.cpp` - implements layer of integration with concrete
PECI driver

#### \src\utils
* `log.hpp`
  * `class logger` - provides simple stream-based logger with varying
loglevels and predefined formatting. Should be utilized to provide consistent
and readable application logs
  * `LOG_* macros` - wrappers for logger class. They come in two forms:
    * `LOG_{level} << "message" << param << (...)` - classic log
    * `LOG_{level}_T({variable/constant}) << "message" << param << (...)`
\- tagged log, might be used to distinguish logs from different objects of the
same class (by providing name/id)
* `configuration.hpp`
  * `class EntityManager` - provides utilities to consume sensor configuration
from EntityManager
  * `class Configuration` - responsible for monitoring and updating sensor
configuration

#### \src\main.cpp
Application entry point. Configures `logger`, instantiates `boost::asio`
and `sdbusplus` objects, loads configuration with `utils::Configuration`, and
spawns `dbus::CupsService`.

### \tests
Directory with unit tests. Compiled using common `CMakeLists.txt` to pin them
into `ctest` - CMake Unit Test engine. General rule of thumb is to implement
separate test files for separate classes.

# Design
## Relations
```ascii
                          +-----------+
                          |CupsService|
                          |           |
+-------------+ [updates] | +-------+ |   [reads]  +------------+
|CupsDiscovery+------------->cpuData<--------------+CupsReadings|
+----+--------+           | +-------+ |            +-------+----+
     |                    |           |                    |
     |                    | +-------+ |                    |
     |[uses]              | |Sensors| |                    |[triggers]
     |                    | +---^---+ |                    |
     |                    |     |     |          +---------v---------+
     |                    +-----------+          |Utilization        |
 +---v-----------+              |                |                   |
 |Core/Iio/Memory|              ^----------------+ +---------------+ |
 |   Factory     |                  [updates]    | |Core/Iio/Memory| |
 +---------+-----+                               | +-----+---------+ |
           |                                     |       |           |
           |                                     +-------------------+
           |                                             |
         +-v---------------------------------------------v-+
         |                Peci::Adapter                    |
         +---------------------+---------------------------+
                               |
                               |
                               v
 +---------------------------- PECI DRIVER --------------------------+
 ```
