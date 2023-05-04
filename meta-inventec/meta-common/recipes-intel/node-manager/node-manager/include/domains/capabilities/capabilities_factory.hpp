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

#include "component_capabilities.hpp"
#include "domain_capabilities.hpp"
#include "knob_capabilities.hpp"

#include <memory>

namespace nodemanager
{

class CapabilitiesFactoryIf
{
  public:
    virtual ~CapabilitiesFactoryIf() = default;

    virtual std::shared_ptr<DomainCapabilitiesIf> createDomainCapabilities(
        std::optional<ReadingType> minReadingType,
        std::optional<ReadingType> maxReadingType, uint32_t minCorrectionTime,
        OnCapabilitiesChangeCallback capabilitiesChangeCallback,
        DomainId domainId) = 0;

    virtual std::shared_ptr<ComponentCapabilitiesIf>
        createComponentCapabilities(std::optional<ReadingType> minReadingType,
                                    std::optional<ReadingType> maxReadingType,
                                    DeviceIndex deviceIndex) = 0;

    virtual std::shared_ptr<KnobCapabilitiesIf> createKnobCapabilities(
        std::optional<ReadingType> minReadingTypeArg,
        std::optional<ReadingType> maxReadingTypeArg, KnobType knobType,
        OnCapabilitiesChangeCallback capabilitiesChangeCallback) = 0;

    virtual std::shared_ptr<KnobCapabilitiesIf>
        createKnobCapabilities(double minArg, double maxArg,
                               KnobType knobType) = 0;
};

class CapabilitiesFactory : public CapabilitiesFactoryIf
{
  public:
    CapabilitiesFactory(const CapabilitiesFactory&) = delete;
    CapabilitiesFactory& operator=(const CapabilitiesFactory&) = delete;
    CapabilitiesFactory(CapabilitiesFactory&&) = delete;
    CapabilitiesFactory& operator=(CapabilitiesFactory&&) = delete;
    CapabilitiesFactory(
        const std::shared_ptr<DevicesManagerIf>& devicesManagerArg) :
        devicesManager(devicesManagerArg)
    {
    }

    std::shared_ptr<DomainCapabilitiesIf> createDomainCapabilities(
        std::optional<ReadingType> minReadingType,
        std::optional<ReadingType> maxReadingType, uint32_t minCorrectionTime,
        OnCapabilitiesChangeCallback capabilitiesChangeCallback,
        DomainId domainId) override
    {
        return std::make_shared<DomainCapabilities>(
            minReadingType, maxReadingType, devicesManager, minCorrectionTime,
            capabilitiesChangeCallback, domainId);
    }

    std::shared_ptr<ComponentCapabilitiesIf>
        createComponentCapabilities(std::optional<ReadingType> minReadingType,
                                    std::optional<ReadingType> maxReadingType,
                                    DeviceIndex deviceIndex) override
    {
        return std::make_shared<ComponentCapabilities>(
            minReadingType, maxReadingType, deviceIndex, devicesManager);
    }

    std::shared_ptr<KnobCapabilitiesIf> createKnobCapabilities(
        std::optional<ReadingType> minReadingTypeArg,
        std::optional<ReadingType> maxReadingTypeArg, KnobType knobType,
        OnCapabilitiesChangeCallback capabilitiesChangeCallback) override
    {
        return std::make_shared<KnobCapabilities>(
            minReadingTypeArg, maxReadingTypeArg, knobType, devicesManager,
            capabilitiesChangeCallback);
    }

    std::shared_ptr<KnobCapabilitiesIf>
        createKnobCapabilities(double minArg, double maxArg,
                               KnobType knobType) override
    {
        return std::make_shared<KnobCapabilities>(minArg, maxArg, knobType,
                                                  devicesManager);
    }

  private:
    std::shared_ptr<DevicesManagerIf> devicesManager;
};

} // namespace nodemanager