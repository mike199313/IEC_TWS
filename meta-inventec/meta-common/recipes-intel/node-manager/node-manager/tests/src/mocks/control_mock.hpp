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
#include "control/control.hpp"

#include <gmock/gmock.h>

using namespace nodemanager;

class ControlMock : public ControlIf
{
  public:
    MOCK_METHOD(void, setBudget,
                (RaplDomainId domainId, const std::optional<Limit>& limit),
                (override));
    MOCK_METHOD(void, setComponentBudget,
                (RaplDomainId domainId, DeviceIndex componentId,
                 const std::optional<Limit>& limit),
                (override));
    MOCK_METHOD(bool, isDomainLimitActive, (RaplDomainId domainId),
                (const, override));
    MOCK_METHOD(bool, isComponentLimitActive,
                (RaplDomainId domainId, DeviceIndex componentId),
                (const, override));
};