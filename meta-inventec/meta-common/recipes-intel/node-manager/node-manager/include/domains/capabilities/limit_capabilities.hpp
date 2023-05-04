/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2021 Intel Corporation.
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
#include "devices_manager/devices_manager.hpp"
#include "readings/reading_event.hpp"

namespace nodemanager
{

using CapabilitiesValuesMap = std::map<std::string, double>;
using OnCapabilitiesChangeCallback = std::function<void(void)>;

static constexpr const double kUnknownMaxPowerLimitInWatts = 0x7fff;

class LimitCapabilitiesIf
{
  public:
    virtual ~LimitCapabilitiesIf() = default;
    virtual std::string getName() const = 0;
    virtual CapabilitiesValuesMap getValuesMap() const = 0;
    virtual double getMin() const = 0;
    virtual double getMax() const = 0;
};

class LimitCapabilities : public LimitCapabilitiesIf
{
  public:
    LimitCapabilities(const LimitCapabilities&) = delete;
    LimitCapabilities& operator=(const LimitCapabilities&) = delete;
    LimitCapabilities(LimitCapabilities&&) = delete;
    LimitCapabilities& operator=(LimitCapabilities&&) = delete;

    LimitCapabilities(
        std::optional<ReadingType> minReadingTypeArg,
        std::optional<ReadingType> maxReadingTypeArg, DeviceIndex componentIdx,
        const std::shared_ptr<ReadingEvent> capabReadingMinValueArg,
        const std::shared_ptr<ReadingEvent> capabReadingMaxValueArg,
        const std::shared_ptr<DevicesManagerIf>& devicesManagerArg) :
        devicesManager(devicesManagerArg),
        capabReadingMinValue(capabReadingMinValueArg),
        capabReadingMaxValue(capabReadingMaxValueArg)
    {
        if (minReadingTypeArg)
        {
            devicesManager->registerReadingConsumer(
                capabReadingMinValue, *minReadingTypeArg, componentIdx);
        }
        if (maxReadingTypeArg)
        {
            devicesManager->registerReadingConsumer(
                capabReadingMaxValue, *maxReadingTypeArg, componentIdx);
        }
    };

    LimitCapabilities(
        std::optional<ReadingType> minReadingTypeArg,
        std::optional<ReadingType> maxReadingTypeArg,
        const std::shared_ptr<ReadingEvent> capabReadingMinValueArg,
        const std::shared_ptr<ReadingEvent> capabReadingMaxValueArg,
        const std::shared_ptr<DevicesManagerIf>& devicesManagerArg) :
        devicesManager(devicesManagerArg),
        capabReadingMinValue(capabReadingMinValueArg),
        capabReadingMaxValue(capabReadingMaxValueArg)
    {
        if (minReadingTypeArg)
        {
            devicesManager->registerReadingConsumer(capabReadingMinValue,
                                                    *minReadingTypeArg);
        }

        if (maxReadingTypeArg)
        {
            devicesManager->registerReadingConsumer(capabReadingMaxValue,
                                                    *maxReadingTypeArg);
        }
    };

    virtual ~LimitCapabilities()
    {
        devicesManager->unregisterReadingConsumer(capabReadingMinValue);
        devicesManager->unregisterReadingConsumer(capabReadingMaxValue);
    }

  private:
    std::shared_ptr<DevicesManagerIf> devicesManager;
    std::shared_ptr<ReadingEvent> capabReadingMinValue;
    std::shared_ptr<ReadingEvent> capabReadingMaxValue;
};

} // namespace nodemanager