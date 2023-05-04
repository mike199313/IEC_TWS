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

#include "config/config.hpp"
#include "sensors/sensor_reading_type.hpp"

namespace nodemanager
{

enum class ReadingType
{
    acPlatformPower,
    acPlatformPowerCapabilitiesMax,
    acPlatformPowerLimit,
    cpuAverageFrequency,
    cpuEfficiency,
    cpuEnergy,
    cpuPackageId,
    cpuPackagePower,
    cpuPackagePowerCapabilitiesMax,
    cpuPackagePowerCapabilitiesMin,
    cpuPackagePowerLimit,
    cpuPresence,
    cpuUtilization,
    dcPlatformEnergy,
    dcPlatformPower,
    dcPlatformPowerCapabilitiesMax,
    dcPlatformPowerLimit,
    dcRatedPowerMin,
    dramEnergy,
    dramPackagePowerCapabilitiesMax,
    dramPower,
    dramPowerLimit,
    gpioState,
    hostPower,
    hostReset,
    hwProtectionPlatformPower,
    hwProtectionPowerCapabilitiesMax,
    inletTemperature,
    outletTemperature,
    pciePower,
    pciePowerCapabilitiesMax,
    pciePowerCapabilitiesMin,
    pciePresence,
    platformPowerEfficiency,
    prochotRatioCapabilitiesMax,
    prochotRatioCapabilitiesMin,
    smbalertInterrupt,
    totalChassisPower,
    turboRatioCapabilitiesMax,
    turboRatioCapabilitiesMin,
    volumetricAirflow
};

SensorReadingType mapReadingTypeToSensorReadingType(ReadingType rt)
{
    switch (rt)
    {
        case ReadingType::acPlatformPower:
            return SensorReadingType::acPlatformPower;
        case ReadingType::dcPlatformPower:
            return SensorReadingType::dcPlatformPowerCpu;
        case ReadingType::hwProtectionPlatformPower:
            return SensorReadingType::dcPlatformPowerPsu;
        case ReadingType::cpuPackagePower:
            return SensorReadingType::cpuPackagePower;
        case ReadingType::dramPower:
            return SensorReadingType::dramPower;
        case ReadingType::pciePower: {
            if (Config::getInstance()
                    .getGeneralPresets()
                    .acceleratorsInterface == kPldmInterfaceName)
            {
                return SensorReadingType::pciePowerPldm;
            }
            else
            {
                return SensorReadingType::pciePower;
            }
        }
        case ReadingType::totalChassisPower:
            return SensorReadingType::acPlatformPower;
        case ReadingType::inletTemperature:
            return SensorReadingType::inletTemperature;
        case ReadingType::outletTemperature:
            return SensorReadingType::outletTemperature;
        case ReadingType::volumetricAirflow:
            return SensorReadingType::volumetricAirflow;
        case ReadingType::cpuEfficiency:
            return SensorReadingType::cpuEfficiency;
        case ReadingType::cpuAverageFrequency:
            return SensorReadingType::cpuAverageFrequency;
        case ReadingType::cpuPackagePowerCapabilitiesMin:
            return SensorReadingType::cpuPackagePowerCapabilitiesMin;
        case ReadingType::cpuPackagePowerCapabilitiesMax:
            return SensorReadingType::cpuPackagePowerCapabilitiesMax;
        case ReadingType::dramPackagePowerCapabilitiesMax:
            return SensorReadingType::dramPackagePowerCapabilitiesMax;
        case ReadingType::acPlatformPowerCapabilitiesMax:
            return SensorReadingType::acPlatformPowerCapabilitiesMax;
        case ReadingType::dcPlatformPowerCapabilitiesMax:
            return SensorReadingType::dcPlatformPowerCapabilitiesMaxCpu;
        case ReadingType::hwProtectionPowerCapabilitiesMax:
            return SensorReadingType::dcPlatformPowerCapabilitiesMaxPsu;
        case ReadingType::cpuEnergy:
            return SensorReadingType::cpuEnergy;
        case ReadingType::dramEnergy:
            return SensorReadingType::dramEnergy;
        case ReadingType::dcPlatformEnergy:
            return SensorReadingType::dcPlatformEnergy;
        case ReadingType::hostReset:
            return SensorReadingType::hostReset;
        case ReadingType::hostPower:
            return SensorReadingType::hostPower;
        case ReadingType::cpuUtilization:
            return SensorReadingType::cpuUtilization;
        case ReadingType::cpuPackagePowerLimit:
            return SensorReadingType::cpuPackagePowerLimit;
        case ReadingType::dramPowerLimit:
            return SensorReadingType::dramPowerLimit;
        case ReadingType::dcPlatformPowerLimit:
            return SensorReadingType::dcPlatformPowerLimit;
        case ReadingType::acPlatformPowerLimit:
            return SensorReadingType::dcPlatformPowerLimit;
        case ReadingType::pciePowerCapabilitiesMin:
            return SensorReadingType::pciePowerCapabilitiesMinPldm;
        case ReadingType::pciePowerCapabilitiesMax: {
            if (Config::getInstance()
                    .getGeneralPresets()
                    .acceleratorsInterface == kPldmInterfaceName)
            {
                return SensorReadingType::pciePowerCapabilitiesMaxPldm;
            }
            else
            {
                return SensorReadingType::pciePower;
            }
        }
        case ReadingType::smbalertInterrupt:
            return SensorReadingType::smartStatus;
        case ReadingType::gpioState:
            return SensorReadingType::gpioState;
        case ReadingType::prochotRatioCapabilitiesMin:
            return SensorReadingType::prochotRatioCapabilitiesMin;
        case ReadingType::prochotRatioCapabilitiesMax:
            return SensorReadingType::prochotRatioCapabilitiesMax;
        case ReadingType::dcRatedPowerMin:
            return SensorReadingType::dcPlatformPowerCapabilitiesMaxPsu;
        case ReadingType::turboRatioCapabilitiesMin:
            return SensorReadingType::turboRatioCapabilitiesMin;
        case ReadingType::turboRatioCapabilitiesMax:
            return SensorReadingType::turboRatioCapabilitiesMax;
        case ReadingType::cpuPackageId:
            return SensorReadingType::cpuPackageId;
        default:
            throw std::runtime_error(
                "Unsupported ReadingType:" +
                std::to_string(
                    static_cast<std::underlying_type_t<ReadingType>>(rt)));
    }
}

/**
 * @brief Mapping  Reading Type with its name
 */
static const std::unordered_map<ReadingType, std::string> kReadingTypeNames = {
    {ReadingType::acPlatformPower, "AcPlatformPower"},
    {ReadingType::acPlatformPowerCapabilitiesMax,
     "AcPlatformPowerCapabilitiesMax"},
    {ReadingType::acPlatformPowerLimit, "AcPlatformPowerLimit"},
    {ReadingType::cpuAverageFrequency, "CpuAverageFrequency"},
    {ReadingType::cpuEfficiency, "CpuEfficiency"},
    {ReadingType::cpuEnergy, "CpuEnergy"},
    {ReadingType::cpuPackageId, "cpuPackageId"},
    {ReadingType::cpuPackagePower, "CpuPackagePower"},
    {ReadingType::cpuPackagePowerCapabilitiesMax,
     "CpuPackagePowerCapabilitiesMax"},
    {ReadingType::cpuPackagePowerCapabilitiesMin,
     "CpuPackagePowerCapabilitiesMin"},
    {ReadingType::cpuPackagePowerLimit, "CpuPackagePowerLimit"},
    {ReadingType::cpuPresence, "CpuPresence"},
    {ReadingType::cpuUtilization, "CpuUtilization"},
    {ReadingType::dcPlatformEnergy, "DcPlatformEnergy"},
    {ReadingType::dcPlatformPower, "DcPlatformPower"},
    {ReadingType::dcPlatformPowerCapabilitiesMax,
     "DcPlatformPowerCapabilitiesMax"},
    {ReadingType::dcPlatformPowerLimit, "DcPlatformPowerLimit"},
    {ReadingType::dcRatedPowerMin, "DcRatedPowerMin"},
    {ReadingType::dramEnergy, "DramEnergy"},
    {ReadingType::dramPackagePowerCapabilitiesMax,
     "DramPackagePowerCapabilitiesMax"},
    {ReadingType::dramPower, "DramPower"},
    {ReadingType::dramPowerLimit, "DramPowerLimit"},
    {ReadingType::gpioState, "GpioState"},
    {ReadingType::hostPower, "HostPower"},
    {ReadingType::hostReset, "HostReset"},
    {ReadingType::hwProtectionPlatformPower, "HwProtectionPlatformPower"},
    {ReadingType::hwProtectionPowerCapabilitiesMax,
     "HwProtectionPowerCapabilitiesMax"},
    {ReadingType::inletTemperature, "InletTemperature"},
    {ReadingType::outletTemperature, "OutletTemperature"},
    {ReadingType::pciePower, "PCIePower"},
    {ReadingType::pciePowerCapabilitiesMax, "PCIePowerCapabilitiesMax"},
    {ReadingType::pciePowerCapabilitiesMin, "PCIePowerCapabilitiesMin"},
    {ReadingType::pciePresence, "PCIePresence"},
    {ReadingType::platformPowerEfficiency, "PlatformPowerEfficiency"},
    {ReadingType::prochotRatioCapabilitiesMax, "prochotRatioCapabilitiesMax"},
    {ReadingType::prochotRatioCapabilitiesMin, "prochotRatioCapabilitiesMin"},
    {ReadingType::smbalertInterrupt, "SMBAlertInterrupt"},
    {ReadingType::totalChassisPower, "TotalChassisPower"},
    {ReadingType::turboRatioCapabilitiesMax, "turboRatioCapabilitiesMax"},
    {ReadingType::turboRatioCapabilitiesMin, "turboRatioCapabilitiesMin"},
    {ReadingType::volumetricAirflow, "VolumetricAirflow"},
};
} // namespace nodemanager
