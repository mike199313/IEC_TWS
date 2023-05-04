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

#include "limit_capabilities.hpp"

#include <map>
#include <memory>

namespace nodemanager
{

class ComponentCapabilitiesIf : public LimitCapabilitiesIf
{
  public:
    virtual ~ComponentCapabilitiesIf() = default;
};

class ComponentCapabilities : public ComponentCapabilitiesIf,
                              public LimitCapabilities
{
  public:
    ComponentCapabilities(const ComponentCapabilities&) = delete;
    ComponentCapabilities& operator=(const ComponentCapabilities&) = delete;
    ComponentCapabilities(ComponentCapabilities&&) = delete;
    ComponentCapabilities& operator=(ComponentCapabilities&&) = delete;

    ComponentCapabilities(
        std::optional<ReadingType> minReadingTypeArg,
        std::optional<ReadingType> maxReadingTypeArg,
        DeviceIndex componentIdArg,
        const std::shared_ptr<DevicesManagerIf>& devicesManagerArg) :
        LimitCapabilities(
            minReadingTypeArg, maxReadingTypeArg, componentIdArg,
            std::make_shared<ReadingEvent>([this](double incomingMinValue) {
                if (!std::isnan(incomingMinValue))
                {
                    min = incomingMinValue;
                }
            }),
            std::make_shared<ReadingEvent>([this](double incomingMaxValue) {
                if (!std::isnan(incomingMaxValue))
                {
                    max = incomingMaxValue;
                }
            }),
            devicesManagerArg),
        componentId(componentIdArg){};

    virtual ~ComponentCapabilities() = default;

    double getMin() const override
    {
        return min;
    }

    double getMax() const override
    {
        return max;
    }

    std::string getName() const override
    {
        return "Component_" + std::to_string(componentId);
    }

    CapabilitiesValuesMap getValuesMap() const override
    {
        CapabilitiesValuesMap capabilitiesMap = {{"Min", getMin()},
                                                 {"Max", getMax()}};

        return capabilitiesMap;
    }

  private:
    DeviceIndex componentId;
    double min{0.0};
    double max{kUnknownMaxPowerLimitInWatts};
};

} // namespace nodemanager