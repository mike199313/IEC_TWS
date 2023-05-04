/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020-2021 Intel Corporation.
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
#include "config_defaults.hpp"
#include "config_values.hpp"
#include "persistent_storage.hpp"
#include "utility/ranges.hpp"

#include <functional>
#include <optional>
#include <variant>

namespace nodemanager
{

void to_json(nlohmann::json& j, const GeneralPresets& data)
{
    j = nlohmann::json{
        {"AcTotalPowerDomainPresent", data.acTotalPowerDomainPresent},
        {"AcTotalPowerDomainEnabled", data.acTotalPowerDomainEnabled},
        {"CpuSubsystemDomainPresent", data.cpuSubsystemDomainPresent},
        {"CpuSubsystemDomainEnabled", data.cpuSubsystemDomainEnabled},
        {"MemorySubsystemDomainPresent", data.memorySubsystemDomainPresent},
        {"MemorySubsystemDomainEnabled", data.memorySubsystemDomainEnabled},
        {"HwProtectionDomainPresent", data.hwProtectionDomainPresent},
        {"HwProtectionDomainEnabled", data.hwProtectionDomainEnabled},
        {"PcieDomainPresent", data.pcieDomainPresent},
        {"PcieDomainEnabled", data.pcieDomainEnabled},
        {"DcTotalPowerDomainPresent", data.dcTotalPowerDomainPresent},
        {"DcTotalPowerDomainEnabled", data.dcTotalPowerDomainEnabled},
        {"PerformanceDomainPresent", data.performanceDomainPresent},
        {"PerformanceDomainEnabled", data.performanceDomainEnabled},
        {"PolicyControlEnabled", data.policyControlEnabled},
        {"CpuPerformanceOptimization", data.cpuPerformanceOptimization},
        {"ProchotAssertionRatio", data.prochotAssertionRatio},
        {"NmInitializationMode", data.nmInitializationMode},
        {"AcceleratorsInterface", data.acceleratorsInterface},
        {"CpuTurboRatioLimit", data.cpuTurboRatioLimit}};
}

void to_json(nlohmann::json& j, const Gpio& data)
{
    j = nlohmann::json{
        {"HwProtectionPolicyTriggerGpio", data.hwProtectionPolicyTriggerGpio}};
}

void to_json(nlohmann::json& j, const Smart& data)
{
    j = nlohmann::json{
        {"PsuPollingIntervalMs", data.psuPollingIntervalMs},
        {"OvertemperatureThrottlingTimeMs",
         data.overtemperatureThrottlingTimeMs},
        {"OvercurrentThrottlingTimeMs", data.overcurrentThrottlingTimeMs},
        {"UndervoltageThrottlingTimeMs", data.undervoltageThrottlingTimeMs},
        {"MaxUndervoltageTimeTimeMs", data.maxUndervoltageTimeTimeMs},
        {"MaxOvertemperatureTimeMs", data.maxOvertemperatureTimeMs},
        {"PowergoodPollingIntervalTimeMs", data.powergoodPollingIntervalTimeMs},
        {"I2cAddrMax", data.i2cAddrsMax},
        {"I2cAddrMin", data.i2cAddrsMin},
        {"ForceSmbalertMaskIntervalTimeMs",
         data.forceSmbalertMaskIntervalTimeMs},
        {"RedundancyEnabled", data.redundancyEnabled},
        {"SmartEnabled", data.smartEnabled},
    };
}

void to_json(nlohmann::json& j, const PowerRange& data)
{
    j = nlohmann::json{{"AcMinimumPower", data.acMin},
                       {"AcMaximumPower", data.acMax},
                       {"CpuMinimumPower", data.cpuMin},
                       {"CpuMaximumPower", data.cpuMax},
                       {"MemoryMinimumPower", data.memoryMin},
                       {"MemoryMaximumPower", data.memoryMax},
                       {"PcieMinimumPower", data.pcieMin},
                       {"PcieMaximumPower", data.pcieMax},
                       {"DcMinimumPower", data.dcMin},
                       {"DcMaximumPower", data.dcMax}};
}

/**
 * @brief Class used to hanlde the NM configuration.
 * Class is designed as a signleton.
 */
class Config
{
  public:
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config(Config&&) = delete;
    Config& operator=(Config&&) = delete;

    static Config& getInstance()
    {
        static Config instance; // Guaranteed to be destroyed.
                                // Instantiated on first use.
        return instance;
    }

    const GeneralPresets& getGeneralPresets()
    {
        return generalPresets;
    }

    const Gpio& getGpio()
    {
        return gpio;
    }

    const Smart& getSmart()
    {
        return smart;
    }

    const PowerRange& getPowerRange()
    {
        return powerRange;
    }

    bool update(const Gpio& newValue)
    {
        gpio = newValue;
        return flush();
    }

    bool update(const GeneralPresets& newValue)
    {
        generalPresets = newValue;
        return flush();
    }

    bool update(const Smart& newValue)
    {
        smart = newValue;
        return flush();
    }

    bool update(const PowerRange& newValue)
    {
        powerRange = newValue;
        return flush();
    }

    const nlohmann::json toJson() const
    {
        nlohmann::json jsonCfg;
        jsonCfg[kGeneralPresets] = generalPresets;
        jsonCfg[kGpio] = gpio;
        jsonCfg[kSmart] = smart;
        jsonCfg[kPowerRange] = powerRange;
        return jsonCfg;
    }

  private:
    static constexpr const auto kGeneralConfJson =
        "/var/lib/node-manager/general.conf.json";
    static constexpr const auto kGeneralPresets = "GeneralPresets";
    static constexpr const auto kPowerRange = "PowerRange";
    static constexpr const auto kGpio = "Gpio";
    static constexpr const auto kSmart = "Smart";

    template <class T>
    auto getRangeValidator(T min, T max)
    {
        return [min, max](const T& value) -> bool {
            return isInRange(value, min, max);
        };
    }

    /**
     * @brief Get the Params used to map json fields to this class structures
     *
     * @return tuple of tuples that describe the json mapping.
     */
    auto getParamsToRead()
    {
        const auto validate = [](const auto& value) -> bool { return true; };
        const auto validatePowerRange = [](const double& value) -> bool {
            return value >= 0;
        };
        return std::make_tuple(
            std::make_tuple(kGeneralPresets, "AcTotalPowerDomainPresent",
                            std::ref(generalPresets.acTotalPowerDomainPresent),
                            validate),
            std::make_tuple(kGeneralPresets, "AcTotalPowerDomainEnabled",
                            std::ref(generalPresets.acTotalPowerDomainEnabled),
                            validate),
            std::make_tuple(kGeneralPresets, "CpuSubsystemDomainPresent",
                            std::ref(generalPresets.cpuSubsystemDomainPresent),
                            validate),
            std::make_tuple(kGeneralPresets, "CpuSubsystemDomainEnabled",
                            std::ref(generalPresets.cpuSubsystemDomainEnabled),
                            validate),
            std::make_tuple(
                kGeneralPresets, "MemorySubsystemDomainPresent",
                std::ref(generalPresets.memorySubsystemDomainPresent),
                validate),
            std::make_tuple(
                kGeneralPresets, "MemorySubsystemDomainEnabled",
                std::ref(generalPresets.memorySubsystemDomainEnabled),
                validate),
            std::make_tuple(kGeneralPresets, "HwProtectionDomainPresent",
                            std::ref(generalPresets.hwProtectionDomainPresent),
                            validate),
            std::make_tuple(kGeneralPresets, "HwProtectionDomainEnabled",
                            std::ref(generalPresets.hwProtectionDomainEnabled),
                            validate),
            std::make_tuple(kGeneralPresets, "PcieDomainPresent",
                            std::ref(generalPresets.pcieDomainPresent),
                            validate),
            std::make_tuple(kGeneralPresets, "PcieDomainEnabled",
                            std::ref(generalPresets.pcieDomainEnabled),
                            validate),
            std::make_tuple(kGeneralPresets, "DcTotalPowerDomainPresent",
                            std::ref(generalPresets.dcTotalPowerDomainPresent),
                            validate),
            std::make_tuple(kGeneralPresets, "DcTotalPowerDomainEnabled",
                            std::ref(generalPresets.dcTotalPowerDomainEnabled),
                            validate),
            std::make_tuple(kGeneralPresets, "PerformanceDomainPresent",
                            std::ref(generalPresets.performanceDomainPresent),
                            validate),
            std::make_tuple(kGeneralPresets, "PerformanceDomainEnabled",
                            std::ref(generalPresets.performanceDomainEnabled),
                            validate),
            std::make_tuple(kGeneralPresets, "PolicyControlEnabled",
                            std::ref(generalPresets.policyControlEnabled),
                            validate),
            std::make_tuple(kGeneralPresets, "CpuPerformanceOptimization",
                            std::ref(generalPresets.cpuPerformanceOptimization),
                            validate),
            std::make_tuple(kGeneralPresets, "ProchotAssertionRatio",
                            std::ref(generalPresets.prochotAssertionRatio),
                            validate),
            std::make_tuple(kGeneralPresets, "NmInitializationMode",
                            std::ref(generalPresets.nmInitializationMode),
                            [](const uint8_t& value) -> bool {
                                return isEnumCastSafe<InitializationMode>(
                                    kInitializationModeSet, value);
                            }),
            std::make_tuple(kGeneralPresets, "AcceleratorsInterface",
                            std::ref(generalPresets.acceleratorsInterface),
                            [](const std::string& value) -> bool {
                                return value == kPldmInterfaceName ||
                                       value == kPeciInterfaceName;
                            }),
            std::make_tuple(kGeneralPresets, "CpuTurboRatioLimit",
                            std::ref(generalPresets.cpuTurboRatioLimit),
                            validate),
            std::make_tuple(kGpio, "HwProtectionPolicyTriggerGpio",
                            std::ref(gpio.hwProtectionPolicyTriggerGpio),
                            validate),
            std::make_tuple(kSmart, "PsuPollingIntervalMs",
                            std::ref(smart.psuPollingIntervalMs),
                            getRangeValidator<uint32_t>(10, 10000)),
            std::make_tuple(kSmart, "OvertemperatureThrottlingTimeMs",
                            std::ref(smart.overtemperatureThrottlingTimeMs),
                            getRangeValidator<uint32_t>(100, 10000)),
            std::make_tuple(kSmart, "OvercurrentThrottlingTimeMs",
                            std::ref(smart.overcurrentThrottlingTimeMs),
                            getRangeValidator<uint32_t>(100, 10000)),
            std::make_tuple(kSmart, "UndervoltageThrottlingTimeMs",
                            std::ref(smart.undervoltageThrottlingTimeMs),
                            getRangeValidator<uint32_t>(100, 10000)),
            std::make_tuple(kSmart, "MaxUndervoltageTimeTimeMs",
                            std::ref(smart.maxUndervoltageTimeTimeMs),
                            getRangeValidator<uint32_t>(100, 2000)),
            std::make_tuple(kSmart, "MaxOvertemperatureTimeMs",
                            std::ref(smart.maxOvertemperatureTimeMs),
                            getRangeValidator<uint32_t>(100, 2000)),
            std::make_tuple(kSmart, "PowergoodPollingIntervalTimeMs",
                            std::ref(smart.powergoodPollingIntervalTimeMs),
                            getRangeValidator<uint32_t>(10, 10000)),
            std::make_tuple(kSmart, "I2cAddrMax", std::ref(smart.i2cAddrsMax),
                            getRangeValidator<uint32_t>(0x00, 0xFF)),
            std::make_tuple(kSmart, "I2cAddrMin", std::ref(smart.i2cAddrsMin),
                            getRangeValidator<uint32_t>(0x00, 0xFF)),
            std::make_tuple(kSmart, "ForceSmbalertMaskIntervalTimeMs",
                            std::ref(smart.forceSmbalertMaskIntervalTimeMs),
                            getRangeValidator<uint32_t>(1000, 1000000)),
            std::make_tuple(kSmart, "RedundancyEnabled",
                            std::ref(smart.redundancyEnabled), validate),
            std::make_tuple(kSmart, "SmartEnabled",
                            std::ref(smart.smartEnabled), validate),
            std::make_tuple(kPowerRange, "AcMinimumPower",
                            std::ref(powerRange.acMin), validatePowerRange),
            std::make_tuple(kPowerRange, "AcMaximumPower",
                            std::ref(powerRange.acMax), validatePowerRange),
            std::make_tuple(kPowerRange, "CpuMinimumPower",
                            std::ref(powerRange.cpuMin), validatePowerRange),
            std::make_tuple(kPowerRange, "CpuMaximumPower",
                            std::ref(powerRange.cpuMax), validatePowerRange),
            std::make_tuple(kPowerRange, "MemoryMinimumPower",
                            std::ref(powerRange.memoryMin), validatePowerRange),
            std::make_tuple(kPowerRange, "MemoryMaximumPower",
                            std::ref(powerRange.memoryMax), validatePowerRange),
            std::make_tuple(kPowerRange, "PcieMinimumPower",
                            std::ref(powerRange.pcieMin), validatePowerRange),
            std::make_tuple(kPowerRange, "PcieMaximumPower",
                            std::ref(powerRange.pcieMax), validatePowerRange),
            std::make_tuple(kPowerRange, "DcMinimumPower",
                            std::ref(powerRange.dcMin), validatePowerRange),
            std::make_tuple(kPowerRange, "DcMaximumPower",
                            std::ref(powerRange.dcMax), validatePowerRange));
    }

    Config()
    {
        if (!isConfig() && !createCfg())
        {
            Logger::log<LogLevel::critical>(
                "Cannot create configuration. Using defaults");
            return;
        }
        storage.readJsonFile(kGeneralConfJson, getParamsToRead());
    };

    /**
     * @brief Returns true if config file exists
     *
     * @return true if config exists
     * @return false if config dosn't exists.
     */
    bool isConfig()
    {
        return storage.exists(kGeneralConfJson);
    }

    /**
     * @brief Creates a configuration file and fills it with provisioned values.
     *
     * @return true on success
     * @return false
     */
    bool createCfg()
    {
        Logger::log<LogLevel::info>(
            "Creating provisioned configuration at: %1%", kGeneralConfJson);
        return flush();
    }

    /**
     * @brief Dumps configuration hold by this class into configuration file.
     *
     * @return true on success
     * @return false
     */
    bool flush()
    {
        return storage.store(kGeneralConfJson, toJson());
    }

    GeneralPresets generalPresets = {};
    Gpio gpio = {};
    PersistentStorage storage = {};
    Smart smart = {};
    PowerRange powerRange = {};
};

} // namespace nodemanager
