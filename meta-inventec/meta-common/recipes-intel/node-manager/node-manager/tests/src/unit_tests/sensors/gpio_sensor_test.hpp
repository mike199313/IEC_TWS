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

#include "mocks/gpio_provider_mock.hpp"
#include "mocks/sensor_reading_mock.hpp"
#include "mocks/sensor_readings_manager_mock.hpp"
#include "sensors/gpio_sensor.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class GpioSensorTest : public ::testing::Test
{
  public:
    virtual ~GpioSensorTest() = default;

    const std::vector<std::optional<GpioState>> gpioStates = {
        std::nullopt, GpioState::high, GpioState::low};
    std::vector<std::shared_ptr<SensorReadingMock>> sensorReadings;

    virtual void SetUp() override
    {
        ON_CALL(*gpioProvider_, getGpioLinesCount)
            .WillByDefault(
                testing::Return(static_cast<DeviceIndex>(gpioStates.size())));
        ON_CALL(*gpioProvider_, getState(testing::_))
            .WillByDefault(testing::Invoke(
                [this](DeviceIndex index) { return gpioStates[index]; }));

        for (DeviceIndex index = 0; index < 3; index++)
        {
            sensorReadings.push_back(
                std::make_shared<testing::NiceMock<SensorReadingMock>>());
            ON_CALL(*sensorReadings.at(index), getDeviceIndex())
                .WillByDefault(testing::Return(index));
            ON_CALL(*sensorManager_,
                    createSensorReading(SensorReadingType::gpioState, index))
                .WillByDefault(testing::Return(sensorReadings.at(index)));
        }

        sut_ = std::make_shared<GpioSensor>(sensorManager_, gpioProvider_);
    }

    std::shared_ptr<GpioSensor> sut_;
    std::shared_ptr<SensorReadingsManagerMock> sensorManager_ =
        std::make_shared<testing::NiceMock<SensorReadingsManagerMock>>();
    std::shared_ptr<GpioProviderMock> gpioProvider_ =
        std::make_shared<testing::NiceMock<GpioProviderMock>>();
};

TEST_F(GpioSensorTest, WhenRunUpdateAllReadings)
{
    for (DeviceIndex i = 0; i < gpioStates.size(); i++)
    {
        if (gpioStates.at(i) == GpioState::low)
        {
            EXPECT_CALL(*sensorReadings.at(i), updateValue(ValueType{0.0}));
            EXPECT_CALL(*sensorReadings.at(i),
                        setStatus(SensorReadingStatus::valid));
        }
        else if (gpioStates.at(i) == GpioState::high)
        {
            EXPECT_CALL(*sensorReadings.at(i), updateValue(ValueType{1.0}));
            EXPECT_CALL(*sensorReadings.at(i),
                        setStatus(SensorReadingStatus::valid));
        }
        else
        {
            EXPECT_CALL(*sensorReadings.at(i), updateValue(testing::_))
                .Times(0);
            EXPECT_CALL(*sensorReadings.at(i),
                        setStatus(SensorReadingStatus::unavailable));
        }
    }
    sut_->run();
}