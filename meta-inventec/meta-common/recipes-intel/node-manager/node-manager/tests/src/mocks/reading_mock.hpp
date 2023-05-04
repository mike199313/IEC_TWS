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

#include "readings/reading.hpp"

#include <gmock/gmock.h>

using namespace nodemanager;

class ReadingMock : public ReadingIf
{
  public:
    MOCK_METHOD(ReadingType, getReadingType, (), (const, override));
    MOCK_METHOD(std::optional<SensorReadingType>, getReadingSource, (),
                (const, override));
    MOCK_METHOD(void, registerReadingConsumer,
                (std::shared_ptr<ReadingConsumer> readingConsumers,
                 ReadingType readingType, DeviceIndex deviceIndex),
                (override));
    MOCK_METHOD(void, unregisterReadingConsumer,
                (std::shared_ptr<ReadingConsumer> readingConsumers),
                (override));
    MOCK_METHOD(void, run, (), (override));
};