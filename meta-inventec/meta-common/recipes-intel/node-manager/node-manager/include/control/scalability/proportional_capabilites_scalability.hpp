/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2021-2022 Intel Corporation.
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
class ProportionalCapabilitiesScalability : public ScalabilityIf
{
  public:
    ProportionalCapabilitiesScalability() = default;
    ProportionalCapabilitiesScalability(
        const ProportionalCapabilitiesScalability&) = delete;
    ProportionalCapabilitiesScalability&
        operator=(const ProportionalCapabilitiesScalability&) = delete;
    ProportionalCapabilitiesScalability(ProportionalCapabilitiesScalability&&) =
        delete;
    ProportionalCapabilitiesScalability&
        operator=(ProportionalCapabilitiesScalability&&) = delete;

    ProportionalCapabilitiesScalability(
        std::shared_ptr<DevicesManagerIf> devicesManagerArg) :
        devicesManager(devicesManagerArg),
        readingEventsPerDevice(maxDevices, nullptr),
        maxCapabilitiesPerDevice(maxDevices, 0.0)
    {
        for (DeviceIndex i = 0; i < maxDevices; ++i)
        {
            readingEventsPerDevice[i] =
                std::make_shared<ReadingEvent>([this, i](double value) {
                    maxCapabilitiesPerDevice[i] = value;
                });
            devicesManager->registerReadingConsumer(readingEventsPerDevice[i],
                                                    readingType, i);
        }
    }

    ~ProportionalCapabilitiesScalability()
    {
        for (DeviceIndex i = 0; i < maxDevices; ++i)
        {
            devicesManager->unregisterReadingConsumer(
                readingEventsPerDevice[i]);
        }
    }

    virtual std::vector<double> getFactors() override
    {
        double totalCapMax = std::accumulate(
            maxCapabilitiesPerDevice.begin(), maxCapabilitiesPerDevice.end(),
            0.0, [](double sum, const double& curr) {
                return sum + (std::isnan(curr) ? 0.0 : curr);
            });
        std::vector<double> ret(readingEventsPerDevice.size());
        for (DeviceIndex i = 0; i < readingEventsPerDevice.size(); ++i)
        {
            ret[i] = std::isnan(maxCapabilitiesPerDevice[i] / totalCapMax)
                         ? 0.0
                         : maxCapabilitiesPerDevice[i] / totalCapMax;
        }
        return ret;
    }

  private:
    std::shared_ptr<DevicesManagerIf> devicesManager;
    std::vector<std::shared_ptr<ReadingEvent>> readingEventsPerDevice;
    std::vector<double> maxCapabilitiesPerDevice;
};

using ProportionalDramScalability = ProportionalCapabilitiesScalability<
    kMaxCpuNumber, ReadingType::dramPackagePowerCapabilitiesMax>;

using ProportionalPCieScalability =
    ProportionalCapabilitiesScalability<kMaxPcieNumber,
                                        ReadingType::pciePowerCapabilitiesMax>;
} // namespace nodemanager
