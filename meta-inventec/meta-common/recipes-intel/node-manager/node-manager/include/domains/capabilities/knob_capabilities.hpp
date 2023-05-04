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

#include "knobs/knob.hpp"
#include "limit_capabilities.hpp"
#include "utility/enum_to_string.hpp"

#include <memory>

namespace nodemanager
{

class KnobCapabilitiesIf : public LimitCapabilitiesIf
{
  public:
    virtual ~KnobCapabilitiesIf() = default;
    virtual KnobType getKnobType() const = 0;
};

class KnobCapabilities : public KnobCapabilitiesIf, public LimitCapabilities
{
  public:
    KnobCapabilities(const KnobCapabilities&) = delete;
    KnobCapabilities& operator=(const KnobCapabilities&) = delete;
    KnobCapabilities(KnobCapabilities&&) = delete;
    KnobCapabilities& operator=(KnobCapabilities&&) = delete;

    KnobCapabilities(
        std::optional<ReadingType> minReadingTypeArg,
        std::optional<ReadingType> maxReadingTypeArg, KnobType knobTypeArg,
        const std::shared_ptr<DevicesManagerIf>& devicesManagerArg,
        OnCapabilitiesChangeCallback capabilitiesChangeCallbackArg) :
        LimitCapabilities(
            minReadingTypeArg, maxReadingTypeArg,
            std::make_shared<ReadingEvent>([this](double incomingMinValue) {
                if ((!std::isnan(incomingMinValue)) &&
                    (incomingMinValue != min))
                {
                    min = incomingMinValue;
                    capabilitiesChangeCallback();
                }
            }),
            std::make_shared<ReadingEvent>([this](double incomingMaxValue) {
                if ((!std::isnan(incomingMaxValue)) &&
                    (incomingMaxValue != max))
                {
                    max = incomingMaxValue;
                    capabilitiesChangeCallback();
                }
            }),
            devicesManagerArg),
        knobType(knobTypeArg),
        capabilitiesChangeCallback(capabilitiesChangeCallbackArg){};

    KnobCapabilities(
        double minReadingTypeArg, double maxReadingTypeArg,
        KnobType knobTypeArg,
        const std::shared_ptr<DevicesManagerIf>& devicesManagerArg) :
        LimitCapabilities(std::nullopt, std::nullopt, nullptr, nullptr,
                          devicesManagerArg),
        knobType(knobTypeArg), min(minReadingTypeArg), max(maxReadingTypeArg){};

    virtual ~KnobCapabilities() = default;

    std::string getName() const override
    {
        return enumToStr(knobTypeNames, knobType);
    }

    double getMin() const override
    {
        return min;
    }

    double getMax() const override
    {
        return max;
    }

    CapabilitiesValuesMap getValuesMap() const override
    {
        CapabilitiesValuesMap capabilitiesMap = {{"Min", getMin()},
                                                 {"Max", getMax()}};

        return capabilitiesMap;
    }

    KnobType getKnobType() const override
    {
        return knobType;
    }

  private:
    KnobType knobType;
    OnCapabilitiesChangeCallback capabilitiesChangeCallback;
    double min{std::numeric_limits<double>::quiet_NaN()};
    double max{std::numeric_limits<double>::quiet_NaN()};

}; // class KnobCapabilities

} // namespace nodemanager