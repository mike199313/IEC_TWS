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

#include "domains/capabilities/component_capabilities.hpp"

#include <gmock/gmock.h>

using namespace nodemanager;

class ComponentCapabilitiesMock : public ComponentCapabilitiesIf
{
  public:
    MOCK_METHOD(std::string, getName, (), (const, override));
    MOCK_METHOD(CapabilitiesValuesMap, getValuesMap, (), (const, override));
    MOCK_METHOD(double, getMin, (), (const, override));
    MOCK_METHOD(double, getMax, (), (const, override));
};