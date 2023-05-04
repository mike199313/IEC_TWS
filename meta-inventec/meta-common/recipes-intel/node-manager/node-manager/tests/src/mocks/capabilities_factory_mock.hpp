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

#include "domains/capabilities/capabilities_factory.hpp"

#include <gmock/gmock.h>

using namespace nodemanager;

class CapabilitiesFactoryMock : public CapabilitiesFactoryIf
{
  public:
    MOCK_METHOD(std::shared_ptr<DomainCapabilitiesIf>, createDomainCapabilities,
                (std::optional<ReadingType> minReadingType,
                 std::optional<ReadingType> maxReadingTyp,
                 uint32_t minCorrectionTime,
                 OnCapabilitiesChangeCallback callback, DomainId domainId),
                (override));
    MOCK_METHOD(std::shared_ptr<ComponentCapabilitiesIf>,
                createComponentCapabilities,
                (std::optional<ReadingType> minReadingType,
                 std::optional<ReadingType> maxReadingType,
                 DeviceIndex deviceIndex),
                (override));
    MOCK_METHOD(std::shared_ptr<KnobCapabilitiesIf>, createKnobCapabilities,
                (std::optional<ReadingType> minReadingTypeArg,
                 std::optional<ReadingType> maxReadingTypeArg,
                 KnobType knobType,
                 OnCapabilitiesChangeCallback capabilitiesChangeCallback),
                (override));
    MOCK_METHOD(std::shared_ptr<KnobCapabilitiesIf>, createKnobCapabilities,
                (double minArg, double maxArg, KnobType knobType), (override));
};