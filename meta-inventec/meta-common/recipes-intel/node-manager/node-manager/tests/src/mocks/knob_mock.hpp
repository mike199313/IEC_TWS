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

#include "knobs/knob.hpp"

#include <gmock/gmock.h>

class KnobMock : public KnobIf
{
  public:
    MOCK_METHOD(void, setKnob, (const double), (override));
    MOCK_METHOD(void, resetKnob, (), (override));
    MOCK_METHOD(bool, isKnobSet, (), (const override));
    MOCK_METHOD(KnobType, getKnobType, (), (const override));
    MOCK_METHOD(DeviceIndex, getDeviceIndex, (), (const override));
    MOCK_METHOD(void, reportStatus, (nlohmann::json & out), (const override));
    MOCK_METHOD(NmHealth, getHealth, (), (const override));
    MOCK_METHOD(void, run, (), (override));
};