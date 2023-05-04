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

#include "statistics/accumulator_if.hpp"

#include <gmock/gmock.h>

using namespace nodemanager;

class AccumulatorMock : public AccumulatorIf
{
  public:
    MOCK_METHOD(void, addSample, (double), (override));
    MOCK_METHOD(double, getAvg, (), (const, override));
    MOCK_METHOD(double, getMin, (), (const, override));
    MOCK_METHOD(double, getMax, (), (const, override));
    MOCK_METHOD(DurationMs, getStatisticsReportingPeriod, (),
                (const, override));
    MOCK_METHOD(void, reset, (), (override));
    MOCK_METHOD(double, getCurrentValue, (), (const, override));
};
