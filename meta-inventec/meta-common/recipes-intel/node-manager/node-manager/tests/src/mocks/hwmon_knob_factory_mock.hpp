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

#include "devices_manager/hwmon_knob_factory.hpp"

#include <gmock/gmock.h>

using namespace nodemanager;

class HwmonKnobFactoryMock : public HwmonKnobFactoryIf
{
  public:
    MOCK_METHOD(std::unique_ptr<KnobIf>, makeHwmonKnob,
                (KnobType, DeviceIndex, std::filesystem::path), (override));
};
