/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2022 Intel Corporation.
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

#include "common_types.hpp"
#include "devices_manager/pldm_entity_provider.hpp"
#include "loggers/log.hpp"
#include "utility/dbus_common.hpp"

namespace nodemanager
{

static constexpr const auto kPcieDbusKnobIface =
    "xyz.openbmc_project.Effecter.SetNumericEffecter";
static constexpr const auto kPcieKnobBusName = "xyz.openbmc_project.pldm";

static constexpr const auto kPowerLimitLongTimeWindow = 0.125;
static constexpr const auto kPowerLimitShortTimeWindow = 0.01;
static constexpr const auto kPowerLimitShortMultplier = 1.2;

struct PcieDbusKnobConfig
{
    std::string name;
    std::string type;
    std::optional<std::string> dbusPath;
};

struct PcieDbusKnobState
{
    std::ios_base::iostate state;
    std::optional<uint32_t> keyValue;
    bool isPresent;

    bool isValueSaved(uint32_t keyValueToSave)
    {
        return (state == std::ios_base::goodbit) && (keyValue.has_value()) &&
               (keyValue == keyValueToSave);
    }
};

enum class PcieKnobType
{
    PL1,
    PL2,
    PL1Tau,
    PL2Tau
};

/**
 * @brief This class implements Control Knob for PLDM PCIe devices
 *
 */
class PcieDbusKnob : public Knob
{
  public:
    PcieDbusKnob(KnobType typeArg, DeviceIndex deviceIndexArg,
                 std::shared_ptr<PldmEntityProviderIf> pldmEntityProviderArg,
                 const std::shared_ptr<sdbusplus::asio::connection>& busArg,
                 const std::shared_ptr<SensorReadingsManagerIf>&
                     sensorReadingsManagerArg,
                 const char* dbusServiceNameArg = kPcieKnobBusName) :
        Knob(typeArg, deviceIndexArg, sensorReadingsManagerArg),
        pldmEntityProvider(pldmEntityProviderArg), bus(busArg),
        dbusServiceName(dbusServiceNameArg)
    {
        installKnobs();
        pldmEntityProvider->registerDiscoveryDataChangeCallback(
            pldmDiscoveryDataChanged);
    }

    virtual ~PcieDbusKnob()
    {
        pldmEntityProvider->unregisterDiscoveryDataChangeCallback(
            pldmDiscoveryDataChanged);
    }

    void setKnob(const double valueToBeSet) override final
    {
        valueToSave = valueToBeSet;
    }
    void resetKnob() override final
    {
        if (!maxValue.has_value())
        {
            auto pcieIndex = getDeviceIndex();
            if (auto sensorReadingCapMax =
                    sensorReadingsManager
                        ->getAvailableAndValueValidSensorReading(
                            SensorReadingType::pciePowerCapabilitiesMaxPldm,
                            pcieIndex))
            {
                if (std::holds_alternative<double>(
                        sensorReadingCapMax->getValue()))
                {
                    valueToSave = maxValue =
                        std::get<double>(sensorReadingCapMax->getValue());
                }
            }
        }
        else
        {
            valueToSave = *maxValue;
        }
    }

    bool isKnobSet() const override
    {
        return lastSavedValue.has_value() &&
               (!maxValue.has_value() || (*lastSavedValue != *maxValue));
    }

    virtual void reportStatus(nlohmann::json& out) const override
    {
        auto type = enumToStr(knobTypeNames, getKnobType());
        nlohmann::json tmp;
        tmp["Health"] = enumToStr(healthNames, getHealth());
        tmp["DbusPathPL1"] =
            optionalToJson(pcieDbusKnobsConfig.at(PcieKnobType::PL1).dbusPath);
        tmp["DbusPathPL2"] =
            optionalToJson(pcieDbusKnobsConfig.at(PcieKnobType::PL2).dbusPath);
        tmp["DbusPathPL1Tau"] = optionalToJson(
            pcieDbusKnobsConfig.at(PcieKnobType::PL1Tau).dbusPath);
        tmp["DbusPathPL2Tau"] = optionalToJson(
            pcieDbusKnobsConfig.at(PcieKnobType::PL2Tau).dbusPath);
        tmp["Status"] = lastState;
        tmp["StatusPL1"] = isPL1Available();
        tmp["StatusPL2"] = isPL2Available();
        tmp["DeviceIndex"] = getDeviceIndex();
        tmp["Value"] = optionalToJson(lastSavedValue);
        out["Knobs-pci-dbus"][type].push_back(tmp);
    }

  private:
    std::shared_ptr<PldmEntityProviderIf> pldmEntityProvider;
    std::shared_ptr<sdbusplus::asio::connection> bus;
    const char* dbusServiceName;
    std::optional<uint32_t> maxValue = std::nullopt;
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>>
        operationalStatusMatches;
    std::unordered_map<PcieKnobType, PcieDbusKnobState> pcieKnobsState = {
        {PcieKnobType::PL1, {std::ios_base::goodbit, std::nullopt, false}},
        {PcieKnobType::PL2, {std::ios_base::goodbit, std::nullopt, false}},
        {PcieKnobType::PL1Tau, {std::ios_base::goodbit, std::nullopt, false}},
        {PcieKnobType::PL2Tau, {std::ios_base::goodbit, std::nullopt, false}}};

    std::unordered_map<PcieKnobType, PcieDbusKnobConfig> pcieDbusKnobsConfig = {
        {PcieKnobType::PL1, {"PL1", "power", std::nullopt}},
        {PcieKnobType::PL2, {"PL2", "power", std::nullopt}},
        {PcieKnobType::PL1Tau, {"Tau_PL1", "time", std::nullopt}},
        {PcieKnobType::PL2Tau, {"Tau_PL2", "time", std::nullopt}}};

    bool isPcieKnobAvailable(PcieKnobType pcieKnobType) const
    {
        return pcieDbusKnobsConfig.at(pcieKnobType).dbusPath.has_value() &&
               pcieKnobsState.at(pcieKnobType).isPresent;
    }

    bool isPL1Available() const
    {
        return isPcieKnobAvailable(PcieKnobType::PL1) &&
               isPcieKnobAvailable(PcieKnobType::PL1Tau);
    }

    bool isPL2Available() const
    {
        return isPcieKnobAvailable(PcieKnobType::PL2) &&
               isPcieKnobAvailable(PcieKnobType::PL2Tau);
    }

    bool isKnobEndpointAvailable() const
    {
        return isPL1Available() || isPL2Available();
    }

    virtual void writeValue() override
    {
        auto value = valueToSave.value();

        if (isPL1Available())
        {
            bus->async_method_call(
                getCallback(PcieKnobType::PL1, value), dbusServiceName,
                pcieDbusKnobsConfig[PcieKnobType::PL1].dbusPath.value(),
                kPcieDbusKnobIface, "SetEffecter", static_cast<double>(value));
            bus->async_method_call(
                getCallback(PcieKnobType::PL1Tau, value), dbusServiceName,
                pcieDbusKnobsConfig[PcieKnobType::PL1Tau].dbusPath.value(),
                kPcieDbusKnobIface, "SetEffecter", kPowerLimitLongTimeWindow);
        }

        if (isPL2Available())
        {
            bus->async_method_call(
                getCallback(PcieKnobType::PL2, value), dbusServiceName,
                pcieDbusKnobsConfig[PcieKnobType::PL2].dbusPath.value(),
                kPcieDbusKnobIface, "SetEffecter",
                std::clamp(value * kPowerLimitShortMultplier, 0.0,
                           static_cast<double>(
                               maxValue.value_or(kMaxPowerLimitWatts))));
            bus->async_method_call(
                getCallback(PcieKnobType::PL2Tau, value), dbusServiceName,
                pcieDbusKnobsConfig[PcieKnobType::PL2Tau].dbusPath.value(),
                kPcieDbusKnobIface, "SetEffecter", kPowerLimitShortTimeWindow);
        }
    }

    void setObjectPath(PcieDbusKnobConfig& knobConfig, const std::string& tid,
                       const std::string& device)
    {
        knobConfig.dbusPath = "/xyz/openbmc_project/pldm/" + tid +
                              "/effecter/" + knobConfig.type + "/PCIe_Slot_" +
                              std::to_string(getDeviceIndex() + 1) + "_" +
                              device + "_" + knobConfig.name;
    }

    std::function<void(const boost::system::error_code& err)>
        getCallback(PcieKnobType pcieKnobType, uint32_t keyValue)
    {
        return [this, pcieKnobType, keyValue](boost::system::error_code ec) {
            if (ec)
            {
                pcieKnobsState[pcieKnobType].state = std::ios_base::failbit;
                pcieKnobsState[pcieKnobType].keyValue = std::nullopt;
                lastState = std::ios_base::failbit;
                lastSavedValue = std::nullopt;
                Logger::log<LogLevel::error>(
                    "[PcieDbusKnob]: value %d for knob %s.%d has not been"
                    "saved, err=%d",
                    keyValue, pcieDbusKnobsConfig[pcieKnobType].name,
                    getDeviceIndex(), ec.value());
                return;
            }

            pcieKnobsState[pcieKnobType].state = std::ios_base::goodbit;
            pcieKnobsState[pcieKnobType].keyValue = keyValue;

            if (isPL1Available() &&
                !(pcieKnobsState[PcieKnobType::PL1].isValueSaved(keyValue) &&
                  pcieKnobsState[PcieKnobType::PL1Tau].isValueSaved(keyValue)))
            {
                return;
            }

            if (isPL2Available() &&
                !(pcieKnobsState[PcieKnobType::PL2].isValueSaved(keyValue) &&
                  pcieKnobsState[PcieKnobType::PL2Tau].isValueSaved(keyValue)))
            {
                return;
            }

            lastState = std::ios_base::goodbit;
            lastSavedValue = keyValue;
        };
    }

    void registerForOperationalStatus(PcieKnobType pcieKnobType)
    {
        if (!pcieDbusKnobsConfig[pcieKnobType].dbusPath.has_value())
        {
            return;
        }

        operationalStatusMatches.push_back(
            std::make_unique<sdbusplus::bus::match::match>(
                static_cast<sdbusplus::bus::bus&>(*bus),
                sdbusplus::bus::match::rules::type::signal() +
                    sdbusplus::bus::match::rules::member("PropertiesChanged") +
                    sdbusplus::bus::match::rules::path_namespace(
                        pcieDbusKnobsConfig[pcieKnobType].dbusPath.value()) +
                    sdbusplus::bus::match::rules::arg0namespace(
                        kOperationalStatusInterface),
                [this, pcieKnobType](sdbusplus::message::message& message) {
                    std::string iface;
                    boost::container::flat_map<std::string, DBusValue>
                        changedProperties;
                    std::vector<std::string> invalidatedProperties;

                    message.read(iface, changedProperties,
                                 invalidatedProperties);
                    if (iface.compare(kOperationalStatusInterface) == 0)
                    {
                        const auto it =
                            changedProperties.find(kOperationalStatusProperty);
                        if (it != changedProperties.end())
                        {
                            auto value = std::get_if<bool>(&it->second);
                            if (value)
                            {
                                pcieKnobsState[pcieKnobType].isPresent = *value;
                            }
                        }
                    }
                }));
    }

    void updateOperationalStatus(PcieKnobType pcieKnobType)
    {
        if (!pcieDbusKnobsConfig[pcieKnobType].dbusPath.has_value())
        {
            pcieKnobsState[pcieKnobType].isPresent = false;
            return;
        }

        sdbusplus::asio::getProperty<bool>(
            *bus.get(), dbusServiceName,
            pcieDbusKnobsConfig[pcieKnobType].dbusPath.value(),
            kOperationalStatusInterface, kOperationalStatusProperty,
            [this, pcieKnobType](boost::system::error_code ec, bool status) {
                if (ec)
                {
                    pcieKnobsState[pcieKnobType].isPresent = false;
                    Logger::log<LogLevel::error>(
                        "[PcieDbusKnob]: operational status for knob %s-%d has"
                        "not been read, err=%d",
                        pcieDbusKnobsConfig[pcieKnobType].name,
                        getDeviceIndex(), ec);
                    return;
                }
                pcieKnobsState[pcieKnobType].isPresent = status;
            });
    }

    void installKnobs()
    {
        const auto& tid = pldmEntityProvider->getTid(getDeviceIndex());
        const auto& device =
            pldmEntityProvider->getDeviceName(getDeviceIndex());
        if (tid && device)
        {
            for (auto& [pcieKnobType, knobConfig] : pcieDbusKnobsConfig)
            {
                setObjectPath(knobConfig, *tid, *device);
                registerForOperationalStatus(pcieKnobType);
                updateOperationalStatus(pcieKnobType);
            }
        }
    }

    std::shared_ptr<std::function<void(void)>> pldmDiscoveryDataChanged =
        std::make_shared<std::function<void(void)>>([this]() {
            for (auto& [pcieKnobType, knobState] : pcieKnobsState)
            {
                knobState = {std::ios_base::goodbit, std::nullopt, false};
            }
            for (auto& [pcieKnobType, knobConfig] : pcieDbusKnobsConfig)
            {
                knobConfig.dbusPath = std::nullopt;
            }

            operationalStatusMatches.clear();
            lastSavedValue = std::nullopt;
            maxValue = std::nullopt;

            installKnobs();
        });
};
} // namespace nodemanager
