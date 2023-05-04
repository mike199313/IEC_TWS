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

#include "devices_manager/devices_manager.hpp"
#include "readings/reading_event.hpp"
#include "scalability.hpp"

#include <boost/range/adaptors.hpp>

namespace nodemanager
{

template <DeviceIndex maxDevices, ReadingType readingType>
class ProportionalScalability : public ScalabilityIf
{
  public:
    ProportionalScalability() = default;
    ProportionalScalability(const ProportionalScalability&) = delete;
    ProportionalScalability& operator=(const ProportionalScalability&) = delete;
    ProportionalScalability(ProportionalScalability&&) = delete;
    ProportionalScalability& operator=(ProportionalScalability&&) = delete;

    ProportionalScalability(
        std::shared_ptr<DevicesManagerIf> devicesManagerArg) :
        devicesManager(devicesManagerArg)
    {
        readingEvent = std::make_shared<ReadingEvent>(
            std::bind(&ProportionalScalability::onPresenceReading, this,
                      std::placeholders::_1));
        devicesManager->registerReadingConsumer(readingEvent, readingType);
    }

    ~ProportionalScalability()
    {
        devicesManager->unregisterReadingConsumer(readingEvent);
    }

    virtual std::vector<double> getFactors() override
    {
        double factor = 1.0 / static_cast<double>(presenceMap.count());
        std::vector<double> ret(presenceMap.size());
        for (size_t i = 0; i < presenceMap.size(); ++i)
        {
            ret[i] = presenceMap[i] ? factor : 0;
        }
        return ret;
    }

  protected:
    std::shared_ptr<DevicesManagerIf> devicesManager;
    std::bitset<maxDevices> presenceMap = {};

    virtual void onPresenceReading(double value)
    {
        if (!std::isnan(value))
        {
            presenceMap = static_cast<unsigned long>(value);
        }
        else
        {
            presenceMap = 0;
        }
    }

  private:
    std::shared_ptr<ReadingEvent> readingEvent;
};

} // namespace nodemanager
