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

#include "trigger_capabilities.hpp"

namespace nodemanager
{
class TriggerCapabilitiesGpio : public TriggerCapabilities
{
  private:
    class TriggerCapabilitiesGpioLine
    {
      public:
        TriggerCapabilitiesGpioLine(
            TriggerCapabilitiesGpio& triggerCapabilitiesGpioArg,
            std::string const& nameArg) :
            objectPathGpioLine(triggerCapabilitiesGpioArg.objectPath + "/" +
                               nameArg),
            dbusInterfacesGpioLine(objectPathGpioLine,
                                   triggerCapabilitiesGpioArg.objectServer)
        {
            initializeDbusInterfaces();
        }

      private:
        void initializeDbusInterfaces()
        {
            dbusInterfacesGpioLine.addInterface(
                "xyz.openbmc_project.NodeManager.Trigger.GPIO",
                [this](auto& ifce) {});
        }
        std::string const objectPathGpioLine;
        DbusInterfaces dbusInterfacesGpioLine;
    };

  public:
    TriggerCapabilitiesGpio() = delete;
    TriggerCapabilitiesGpio(const TriggerCapabilitiesGpio&) = delete;
    TriggerCapabilitiesGpio& operator=(const TriggerCapabilitiesGpio&) = delete;
    TriggerCapabilitiesGpio(TriggerCapabilitiesGpio&&) = delete;
    TriggerCapabilitiesGpio& operator=(TriggerCapabilitiesGpio&&) = delete;

    virtual ~TriggerCapabilitiesGpio() = default;

    TriggerCapabilitiesGpio(
        TriggerType triggerTypeArg, std::string const& nameArg,
        std::string const& unitArg, uint16_t minArg, uint16_t maxArg,
        std::shared_ptr<sdbusplus::asio::object_server> objectServerArg,
        std::string const& objectPathArg,
        std::shared_ptr<GpioProviderIf> gpioProviderArg) :
        TriggerCapabilities(triggerTypeArg, nameArg, unitArg, minArg, maxArg,
                            objectServerArg, objectPathArg),
        gpioProvider(gpioProviderArg)
    {
        for (DeviceIndex i = 0; i < gpioProvider->getGpioLinesCount(); i++)
        {
            triggerCapabilitiesGpioLines.emplace_back(
                std::make_shared<TriggerCapabilitiesGpioLine>(
                    *this, gpioProvider->getFormattedLineName(i)));
        }
    }

    bool isTriggerLevelValid(uint16_t const triggerLevel) const override
    {
        uint16_t gpioIndex = triggerLevel & 0x7FFF;
        if (gpioIndex >= kAllDevices)
        {
            return false;
        }
        if (gpioProvider->isGpioReserved(static_cast<DeviceIndex>(gpioIndex)))
        {
            return false;
        }
        return TriggerCapabilities::isTriggerLevelValid(gpioIndex);
    }

  private:
    std::shared_ptr<GpioProviderIf> gpioProvider;
    std::vector<std::shared_ptr<TriggerCapabilitiesGpioLine>>
        triggerCapabilitiesGpioLines;
};

} // namespace nodemanager