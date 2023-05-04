/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2021-2022 Intel Corporation.
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
#include "status_monitor.hpp"

#include <gmock/gmock.h>

using namespace nodemanager;

class StatusMonitorActionMock : public StatusMonitor::ActionIf
{
  public:
    MOCK_METHOD(void, logSensorMissing,
                (std::string sensorReadingTypeName, DeviceIndex deviceIndex),
                (override));
    MOCK_METHOD(void, logSensorReadingMissing,
                (std::string sensorReadingTypeName, DeviceIndex deviceIndex),
                (override));
    MOCK_METHOD(void, logReadingMissing, (std::string readingTypeName),
                (override));
    MOCK_METHOD(void, logNonCriticalReadingMissing,
                (std::string readingTypeName), (override));
};
