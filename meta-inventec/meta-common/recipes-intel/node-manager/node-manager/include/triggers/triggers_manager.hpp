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

#include "actions/action.hpp"
#include "actions/action_binary.hpp"
#include "actions/action_cpu_utilization.hpp"
#include "actions/action_gpio.hpp"
#include "statistics/moving_average.hpp"
#include "trigger.hpp"
#include "trigger_capabilities.hpp"
#include "trigger_capabilities_gpio.hpp"
#include "utility/ranges.hpp"

#include <array>

namespace nodemanager
{

class TriggersManagerIf
{
  public:
    virtual ~TriggersManagerIf() = default;
    virtual std::shared_ptr<Trigger>
        createTrigger(TriggerType triggerType, uint16_t triggerLevel,
                      TriggerCallback callback) = 0;
    virtual std::shared_ptr<TriggerCapabilities>
        getTriggerCapabilities(TriggerType triggerType) = 0;
    virtual bool isTriggerAvailable(TriggerType triggerType) const = 0;
};

struct TriggersConfig
{
    TriggerType type;
    std::string name;
    std::string unit;
    uint16_t min;
    uint16_t max;
};

/**
 * @brief Setups trigger capabilities on DBus interface. On user's request,
 * creates Trigger objectt and related to it Action object. Action object is
 * created based on the action-to-trigger-type mapping.
 */
class TriggersManager : public TriggersManagerIf
{
  public:
    TriggersManager() = delete;
    TriggersManager(const TriggersManager&) = delete;
    TriggersManager(TriggersManager&&) = delete;
    TriggersManager& operator=(TriggersManager&&) = delete;

    virtual ~TriggersManager() = default;

    TriggersManager(
        std::shared_ptr<sdbusplus::asio::object_server> objectServerArg,
        std::string const& objectPathArg,
        std::shared_ptr<GpioProviderIf> gpioProviderArg) :
        objectServer(objectServerArg),
        objectPath(objectPathArg), gpioProvider(gpioProviderArg)
    {
        installTriggersCapabilities();
    }

    /**
     * @brief Create a Trigger object
     *
     * @param triggerType   Trigger's type to be created. Based on that
     * parameter a proper Action object is created for the Trigger.
     * @param triggerLevel  For Triggers which are configured by a user, this is
     *                      a threshold value. That threshold works without
     * hysteresis.
     * @param callback      Optional callback function. Will be called with a
     * proper ActionType value when reading crosses user defined threshold (by
     * the triggerLevel parameter) or internal hardocded threshold value.
     * @return              Returns shared pointer to the Trigger object.
     */
    virtual std::shared_ptr<Trigger>
        createTrigger(TriggerType triggerType, uint16_t triggerLevel,
                      TriggerCallback callback) final
    {
        return std::make_shared<Trigger>(makeAction(triggerType, triggerLevel),
                                         callback);
    }

    /**
     * @brief Get the Trigger Capabilities object. This object defines Name,
     * Unit, Max and Min parameters for each available Trigger type.
     *
     * @param triggerType Trigger's type. If issued with a trigger type which is
     * not supported, then throws the errors::UnsupportedPolicyTriggerType
     * exception.
     * @return            Returns shared pointer to the TriggerCapabilities
     * object.
     */
    virtual std::shared_ptr<TriggerCapabilities>
        getTriggerCapabilities(TriggerType triggerType) final
    {
        try
        {
            return triggersCapabilities.at(triggerType);
        }
        catch (const std::out_of_range& e)
        {
            throw errors::UnsupportedPolicyTriggerType();
        }
    }

    virtual bool isTriggerAvailable(TriggerType triggerType) const final
    {
        return triggersCapabilities.find(triggerType) !=
               triggersCapabilities.end();
    }

  private:
    std::shared_ptr<sdbusplus::asio::object_server> objectServer;
    std::string const& objectPath;
    std::shared_ptr<GpioProviderIf> gpioProvider;
    std::unordered_map<TriggerType, std::shared_ptr<TriggerCapabilities>>
        triggersCapabilities;

    /**
     * @brief Installs all available Triggers Capabilities on DBus interface.
     */
    void installTriggersCapabilities()
    {
        std::array<TriggersConfig, 5> const triggersConfig{
            {{TriggerType::inletTemperature, "Inlet Temperature",
              "Degree Celsius", 0, 100},
             {TriggerType::hostReset, "Host Reset", "Boot(0)/HostReset(1)", 0,
              0},
             {TriggerType::cpuUtilization, "C0 Residency", "Percentage", 0,
              100},
             {TriggerType::always, "Always On", "N/A", 0, 0},
             {TriggerType::smbalertInterrupt, "SMBAlert",
              "Interrupt(0)/Idle(1)", 0, 0}}};

        for (const auto& config : triggersConfig)
        {
            triggersCapabilities.emplace(
                config.type,
                std::make_shared<TriggerCapabilities>(
                    config.type, config.name, config.unit, config.min,
                    config.max, objectServer, objectPath));
        }

        auto gpioLinesCount = gpioProvider->getGpioLinesCount();

        if (gpioLinesCount > 0)
        {
            triggersCapabilities.emplace(
                TriggerType::gpio,
                std::make_shared<TriggerCapabilitiesGpio>(
                    TriggerType::gpio, "GPIO", "GpioIndex", 0,
                    static_cast<uint16_t>(gpioLinesCount - 1), objectServer,
                    objectPath, gpioProvider));
        }
    }

  private:
    /**
     * @brief Creates Action object based on provided Trigger's type.
     *
     * @param tt    Trigger's type. If issued with a trigger type not supported,
     *              then throws the errors::UnsupportedPolicyTriggerType
     * exception.
     * @param value Setups threshold value in the Action object. For some types
     * of Action objects also setups init value.
     * @return      Returns shared pointer to the Action object.
     */
    std::shared_ptr<ActionIf> makeAction(const TriggerType tt,
                                         const uint16_t value)
    {
        static constexpr std::chrono::milliseconds correctionTime{2000};

        switch (tt)
        {
            case TriggerType::inletTemperature:
                return std::make_shared<Action>(value);
            case TriggerType::missingReadingsTimeout:
                throw errors::UnsupportedPolicyTriggerType();
                break;
            case TriggerType::gpio: {
                bool triggerOnRisingEdge = value & (1 << 15);
                return std::make_shared<ActionGpio>(triggerOnRisingEdge);
            }
            case TriggerType::cpuUtilization:
                return std::make_shared<ActionCpuUtilization>(
                    value, std::make_shared<MovingAverage>(correctionTime));
            case TriggerType::hostReset:
            case TriggerType::smbalertInterrupt:
                return std::make_shared<ActionBinary>(value);
            default:
                throw errors::UnsupportedPolicyTriggerType();
        }
    }
};

} // namespace nodemanager