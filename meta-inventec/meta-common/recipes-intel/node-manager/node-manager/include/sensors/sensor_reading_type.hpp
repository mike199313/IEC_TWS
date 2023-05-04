/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020-2022 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you ("License"). Unless the License provides otherwise,
 * you may not use, modify, copy, publish, distribute, disclose or transmit
 * this software or the related documents without Intel's prior written
 * permission.
 *
 * This software and the related documents are provided as is, with
 * no express or implied warranties, other than those that are expressly
 * stated in the License.
 */

#pragma once

namespace nodemanager
{

enum class SensorReadingType
{
    acPlatformPower,
    dcPlatformPowerCpu,
    dcPlatformPowerPsu,
    cpuPackagePower,
    dramPower,
    pciePower,
    inletTemperature,
    outletTemperature,
    volumetricAirflow,
    cpuEfficiency,
    cpuAverageFrequency,
    cpuPackagePowerCapabilitiesMin,
    cpuPackagePowerCapabilitiesMax,
    dramPackagePowerCapabilitiesMax,
    acPlatformPowerCapabilitiesMax,
    dcPlatformPowerCapabilitiesMaxCpu,
    dcPlatformPowerCapabilitiesMaxPsu,
    cpuEnergy,
    dramEnergy,
    dcPlatformEnergy,
    hostReset,
    hostPower,
    cpuUtilization,
    cpuPackagePowerLimit,
    dramPowerLimit,
    dcPlatformPowerLimit,
    powerState,
    gpuPowerState,
    smartStatus,
    gpioState,
    prochotRatioCapabilitiesMin,
    prochotRatioCapabilitiesMax,
    turboRatioCapabilitiesMin,
    turboRatioCapabilitiesMax,
    cpuPackageId,
    pciePowerPldm,
    pciePowerLimitPldm,
    pciePowerCapabilitiesMaxPldm,
    pciePowerCapabilitiesMinPldm,
    cpuDieMask,
};

enum class SmartStatusType
{
    uninitialized,
    noGpio,
    idle,
    interruptHandling
};

/**
 * @brief Mapping Sensor Reading Type with its name
 */
static const std::unordered_map<SensorReadingType, std::string>
    kSensorReadingTypeNames = {
        {SensorReadingType::acPlatformPower, "AcPlatformPower"},
        {SensorReadingType::dcPlatformPowerCpu, "DcPlatformPowerCpu"},
        {SensorReadingType::dcPlatformPowerPsu, "DcPlatformPowerPsu"},
        {SensorReadingType::cpuPackagePower, "CpuPackagePower"},
        {SensorReadingType::dramPower, "DramPower"},
        {SensorReadingType::pciePower, "PciePower"},
        {SensorReadingType::inletTemperature, "InletTemperature"},
        {SensorReadingType::outletTemperature, "OutletTemperature"},
        {SensorReadingType::volumetricAirflow, "VolumetricAirflow"},
        {SensorReadingType::cpuEfficiency, "CpuEfficiency"},
        {SensorReadingType::cpuAverageFrequency, "CpuAverageFrequency"},
        {SensorReadingType::cpuPackagePowerCapabilitiesMin,
         "CpuPackagePowerCapabilitiesMin"},
        {SensorReadingType::cpuPackagePowerCapabilitiesMax,
         "CpuPackagePowerCapabilitiesMax"},
        {SensorReadingType::dramPackagePowerCapabilitiesMax,
         "DramPackagePowerCapabilitiesMax"},
        {SensorReadingType::acPlatformPowerCapabilitiesMax,
         "AcPlatformPowerCapabilitiesMax"},
        {SensorReadingType::dcPlatformPowerCapabilitiesMaxCpu,
         "DcPlatformPowerCapabilitiesMaxCpu"},
        {SensorReadingType::dcPlatformPowerCapabilitiesMaxPsu,
         "DcPlatformPowerCapabilitiesMaxPsu"},
        {SensorReadingType::cpuEnergy, "CpuEnergy"},
        {SensorReadingType::dramEnergy, "DramEnergy"},
        {SensorReadingType::dcPlatformEnergy, "DcPlatformEnergy"},
        {SensorReadingType::hostReset, "HostReset"},
        {SensorReadingType::hostPower, "HostPower"},
        {SensorReadingType::cpuUtilization, "CpuUtilization"},
        {SensorReadingType::cpuPackagePowerLimit, "CpuPackagePowerLimit"},
        {SensorReadingType::dramPowerLimit, "DramPowerLimit"},
        {SensorReadingType::dcPlatformPowerLimit, "PlatformRaplPowerLimit"},
        {SensorReadingType::powerState, "PlatformPowerState"},
        {SensorReadingType::gpuPowerState, "GpuPowerState"},
        {SensorReadingType::smartStatus, "SmaRTStatus"},
        {SensorReadingType::gpioState, "GpioState"},
        {SensorReadingType::prochotRatioCapabilitiesMin,
         "ProchotRatioCapabilitiesMin"},
        {SensorReadingType::prochotRatioCapabilitiesMax,
         "ProchotRatioCapabilitiesMax"},
        {SensorReadingType::turboRatioCapabilitiesMin,
         "TurboRatioCapabilitiesMin"},
        {SensorReadingType::turboRatioCapabilitiesMax,
         "TurboRatioCapabilitiesMax"},
        {SensorReadingType::cpuPackageId, "CpuPackageId"},
        {SensorReadingType::pciePowerPldm, "PciePowerPldm"},
        {SensorReadingType::pciePowerLimitPldm, "PciePowerLimitPldm"},
        {SensorReadingType::pciePowerCapabilitiesMaxPldm,
         "PciePowerCapabilitiesMaxPldm"},
        {SensorReadingType::pciePowerCapabilitiesMinPldm,
         "PciePowerCapabilitiesMinPldm"},
        {SensorReadingType::cpuDieMask, "CpuDieMask"}};

enum class PowerStateType
{
    s0,
    s1,
    s2,
    s3,
    s4,
    s5,
    g3,
    unknown
};

enum class GpuPowerState
{
    on,
    off
};

} // namespace nodemanager
