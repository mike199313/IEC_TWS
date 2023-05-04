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

#include "devices_manager/devices_manager.hpp"
#include "readings/reading_type.hpp"

namespace nodemanager
{

class EfficiencyHelperIf
{
  public:
    virtual ~EfficiencyHelperIf() = default;

    virtual double getHint() = 0;
};

class EfficiencyHelper : public EfficiencyHelperIf
{
  public:
    EfficiencyHelper() = delete;
    EfficiencyHelper(const EfficiencyHelper&) = delete;
    EfficiencyHelper& operator=(const EfficiencyHelper&) = delete;
    EfficiencyHelper(EfficiencyHelper&&) = delete;
    EfficiencyHelper& operator=(EfficiencyHelper&&) = delete;

    EfficiencyHelper(const std::shared_ptr<DevicesManagerIf>& devicesManagerArg,
                     const ReadingType& readingType,
                     const std::chrono::milliseconds& averagingWindow) :
        devicesManager(devicesManagerArg),
        efficiencyAverage(averagingWindow)
    {
        devicesManager->registerReadingConsumer(efficiencyReadingEvent,
                                                readingType);
    }

    virtual ~EfficiencyHelper()
    {
        devicesManager->unregisterReadingConsumer(efficiencyReadingEvent);
    }

    double getHint()
    {
        double hint = 0;
        auto average = efficiencyAverage.getAvg();
        if (!std::isnan(average) && !std::isnan(lastAverage))
        {
            auto delta = average - lastAverage;
            if (delta > maxDelta)
            {
                maxDelta = delta;
                hint = 1;
            }
            else
            {
                hint = -1;
            }
        }
        lastAverage = average;
        return hint;
    }

  private:
    std::shared_ptr<DevicesManagerIf> devicesManager;
    MovingAverage efficiencyAverage;
    double lastAverage = std::numeric_limits<double>::quiet_NaN();
    double maxDelta = std::numeric_limits<double>::lowest();
    std::shared_ptr<ReadingEvent> efficiencyReadingEvent =
        std::make_shared<ReadingEvent>(
            [this](double value) { efficiencyAverage.addSample(value); });
};

} // namespace nodemanager