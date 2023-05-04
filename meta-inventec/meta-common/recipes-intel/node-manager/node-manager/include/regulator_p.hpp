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

#include "readings/reading_type.hpp"

namespace nodemanager
{

class RegulatorIf
{
  public:
    virtual ~RegulatorIf() = default;

    virtual double calculateControlSignal(double) = 0;
};

class RegulatorP : public RegulatorIf
{
  public:
    RegulatorP() = delete;
    RegulatorP(const RegulatorP&) = delete;
    RegulatorP& operator=(const RegulatorP&) = delete;
    RegulatorP(RegulatorP&&) = delete;
    RegulatorP& operator=(RegulatorP&&) = delete;

    RegulatorP(std::shared_ptr<DevicesManagerIf> devicesManagerArg,
               double pGainArg, ReadingType readSample) :
        devicesManager(devicesManagerArg),
        pGain(pGainArg)
    {
        devicesManager->registerReadingConsumer(readSampleHandler, readSample,
                                                kAllDevices);
    }

    virtual ~RegulatorP()
    {
        devicesManager->unregisterReadingConsumer(readSampleHandler);
    }

    virtual double calculateControlSignal(double setPoint) override final
    {
        return (setPoint - lastReadSample) * pGain;
    }

  private:
    std::shared_ptr<DevicesManagerIf> devicesManager;
    double pGain;
    double lastReadSample{std::numeric_limits<double>::quiet_NaN()};

    std::shared_ptr<ReadingEvent> readSampleHandler =
        std::make_shared<ReadingEvent>(
            [this](double value) { lastReadSample = value; });
};

} // namespace nodemanager