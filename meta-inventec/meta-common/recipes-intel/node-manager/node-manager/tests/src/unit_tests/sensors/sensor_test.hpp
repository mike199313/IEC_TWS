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
#include "common_types.hpp"
#include "mocks/gpio_provider_mock.hpp"
#include "mocks/hwmon_file_provider_mock.hpp"
#include "mocks/peci_commands_mock.hpp"
#include "mocks/sensor_reading_mock.hpp"
#include "mocks/sensor_readings_manager_mock.hpp"
#include "sensors/cpu_efficiency_sensor.hpp"
#include "sensors/cpu_utilization_sensor.hpp"
#include "sensors/host_power_dbus_sensor.hpp"
#include "sensors/host_reset_dbus_sensor.hpp"
#include "sensors/hwmon_sensor.hpp"
#include "sensors/inlet_temp_dbus_sensor.hpp"
#include "sensors/outlet_temp_dbus_sensor.hpp"
#include "sensors/peci_sensor.hpp"
#include "sensors/sensor.hpp"
#include "utility/status_provider_if.hpp"

#include <typeindex>
#include <typeinfo>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-death-test-internal.h>

namespace nodemanager
{
static const unsigned int kPeciSensorCpuNum = 1;
static constexpr const auto kPeciSensorPath = "/dummy_path";

template <typename T>
class SenorTestHealth : public testing::Test
{
  public:
    virtual ~SenorTestHealth() = default;

    std::shared_ptr<Sensor> createSensor()
    {
        if constexpr (std::is_base_of_v<DbusSensor<double>, T> ||
                      std::is_base_of_v<DbusSensor<std::string>, T>)
        {
            return std::make_shared<T>(sensorManager_,
                                       DbusEnvironment::getBus());
        }
        else if constexpr (std::is_base_of_v<HwmonSensor, T>)
        {
            return std::make_shared<T>(sensorManager_, hwmonProvider_);
        }
        else if constexpr (std::is_base_of_v<PeciSampleSensor, T>)
        {
            return std::make_shared<T>(sensorManager_, peciCommands_, 1);
        }
        else if constexpr (std::is_base_of_v<PeciSensor, T>)
        {
            return std::make_shared<T>(sensorManager_, peciCommands_, 1);
        }
        else if constexpr (std::is_base_of_v<GpioSensor, T>)
        {
            return std::make_shared<T>(sensorManager_, gpioProvider_);
        }

        return nullptr;
    }

    void setOneSensorInvalidRemainingValid()
    {
        EXPECT_CALL(*(this->sensorReading_), getHealth())
            .WillOnce(testing::Return(NmHealth::warning))
            .WillRepeatedly(testing::Return(NmHealth::ok));
    }

    void setAllSensorsValid()
    {
        EXPECT_CALL(*(this->sensorReading_), getHealth())
            .WillRepeatedly(testing::Return(NmHealth::ok));
    }

    void setAllSensorsWarning()
    {
        EXPECT_CALL(*(this->sensorReading_), getHealth())
            .WillRepeatedly(testing::Return(NmHealth::warning));
    }

    virtual void SetUp() override
    {
        ON_CALL(*gpioProvider_, getGpioLinesCount())
            .WillByDefault(testing::Return(DeviceIndex{3}));
        ON_CALL(*sensorReading_, getHealth())
            .WillByDefault(testing::Return(NmHealth::warning));
        ON_CALL(*sensorManager_, createSensorReading(testing::_, testing::_))
            .WillByDefault(testing::Return(sensorReading_));
        sut_ = createSensor();
    }

    std::shared_ptr<Sensor> sut_;
    std::shared_ptr<SensorReadingMock> sensorReading_ =
        std::make_shared<testing::NiceMock<SensorReadingMock>>();
    std::shared_ptr<SensorReadingsManagerMock> sensorManager_ =
        std::make_shared<testing::NiceMock<SensorReadingsManagerMock>>();
    std::shared_ptr<PeciCommandsMock> peciCommands_ =
        std::make_shared<testing::NiceMock<PeciCommandsMock>>();
    std::shared_ptr<HwmonFileProviderMock> hwmonProvider_ =
        std::make_shared<testing::NiceMock<HwmonFileProviderMock>>();
    std::shared_ptr<GpioProviderMock> gpioProvider_ =
        std::make_shared<testing::NiceMock<GpioProviderMock>>();
};

typedef testing::Types<HostPowerDbusSensor, HostResetDbusSensor,
                       InletTempDbusSensor, OutletTempDbusSensor, HwmonSensor,
                       CpuEfficiencySensor, CpuUtilizationSensor, GpioSensor,
                       PeciSensor, CpuFrequencySensor>
    SensorImplementations;

TYPED_TEST_SUITE(SenorTestHealth, SensorImplementations);

TYPED_TEST(SenorTestHealth, AllSensorReadingsOkGeneratesHealthOk)
{
    this->setAllSensorsValid();
    auto health = (this->sut_)->getHealth();
    EXPECT_EQ(health, NmHealth::ok);
}

TYPED_TEST(SenorTestHealth, AllSensorReadingsInvalidGeneratesHealthWarning)
{
    this->setAllSensorsWarning();
    auto health = (this->sut_)->getHealth();
    EXPECT_EQ(health, NmHealth::warning);
}

TYPED_TEST(SenorTestHealth, SomeSensorsInvalidGeneratesHealthWarning)
{
    this->setOneSensorInvalidRemainingValid();
    auto health = (this->sut_)->getHealth();
    EXPECT_EQ(health, NmHealth::warning);
}

} // namespace nodemanager