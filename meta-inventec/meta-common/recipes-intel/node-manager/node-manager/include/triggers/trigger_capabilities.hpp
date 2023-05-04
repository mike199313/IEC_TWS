/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020 Intel Corporation.
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

#include "trigger_enums.hpp"
#include "utility/enum_to_string.hpp"
#include "utility/ranges.hpp"

#include <cstdint>

namespace nodemanager
{
class TriggerCapabilities
{
  public:
    TriggerCapabilities() = delete;
    TriggerCapabilities(const TriggerCapabilities&) = delete;
    TriggerCapabilities& operator=(const TriggerCapabilities&) = delete;
    TriggerCapabilities(TriggerCapabilities&&) = delete;
    TriggerCapabilities& operator=(TriggerCapabilities&&) = delete;

    virtual ~TriggerCapabilities() = default;

    TriggerCapabilities(
        TriggerType triggerTypeArg, std::string const& nameArg,
        std::string const& unitArg, uint16_t minArg, uint16_t maxArg,
        std::shared_ptr<sdbusplus::asio::object_server> objectServerArg,
        std::string const& objectPathArg) :
        min(minArg),
        max(maxArg), unit(unitArg), name(nameArg),
        objectServer(objectServerArg),
        objectPath(objectPathArg + "/Trigger/" +
                   enumToStr(kTriggerTypeNames, triggerTypeArg))
    {
        initializeDbusInterfaces();
    }

    virtual bool isTriggerLevelValid(const uint16_t triggerLevel) const
    {
        return isInRange(triggerLevel, min, max);
    }

  protected:
    void initializeDbusInterfaces()
    {
        dbusInterfaces.addInterface(
            "xyz.openbmc_project.NodeManager.Trigger", [this](auto& interface) {
                interface.register_property_r(
                    "Max", uint16_t(), sdbusplus::vtable::property_::const_,
                    [this](const auto&) { return max; });
                interface.register_property_r(
                    "Min", uint16_t(), sdbusplus::vtable::property_::const_,
                    [this](const auto&) { return min; });
                interface.register_property_r(
                    "Name", std::string(), sdbusplus::vtable::property_::const_,
                    [this](const auto&) { return name; });
                interface.register_property_r(
                    "Unit", std::string(), sdbusplus::vtable::property_::const_,
                    [this](const auto&) { return unit; });
            });
    }

    const uint16_t min;
    const uint16_t max;
    const std::string unit;
    const std::string name;

    std::shared_ptr<sdbusplus::asio::object_server> objectServer;
    std::string const objectPath;
    DbusInterfaces dbusInterfaces{objectPath, objectServer};
};

} // namespace nodemanager