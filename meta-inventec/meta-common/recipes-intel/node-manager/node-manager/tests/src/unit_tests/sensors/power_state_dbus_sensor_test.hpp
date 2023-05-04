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

#include "mocks/sensor_reading_mock.hpp"
#include "mocks/sensor_readings_manager_mock.hpp"
#include "sensors/power_state_dbus_sensor.hpp"
#include "stubs/arbitrary_dbus_stub.hpp"
#include "utils/busctl_call.hpp"
#include "utils/dbus_environment.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class PowerStateDbusSensorTest : public ::testing::Test
{
  protected:
    virtual void SetUp() override
    {
        ON_CALL(*sensorReadingsManager_,
                createSensorReading(SensorReadingType::powerState, 0))
            .WillByDefault(testing::Return(sensorReading_));
        ON_CALL(*sensorReading_, getDeviceIndex())
            .WillByDefault(testing::Return(DeviceIndex(0)));
        ON_CALL(*sensorReading_, getSensorReadingType())
            .WillByDefault(testing::Return(SensorReadingType::powerState));

        sut_ = std::make_shared<PowerStateDbusSensor>(
            sensorReadingsManager_, DbusEnvironment::getBus(),
            DbusEnvironment::serviceName());
    }

    auto setPromise(const std::string& tag)
    {
        return testing::InvokeWithoutArgs(DbusEnvironment::setPromise(tag));
    }

    std::unique_ptr<ArbitraryDbusStub<std::string>> dbusAcpiPowerStateStub_ =
        std::make_unique<ArbitraryDbusStub<std::string>>(
            DbusEnvironment::getIoc(), DbusEnvironment::getBus(),
            DbusEnvironment::getObjServer(), kPowerStateObjectPath,
            std::unordered_map<std::string, std::vector<std::string>>{
                {kPowerStateDbusInterface, {kPowerStatePropertyName}}});
    std::shared_ptr<SensorReadingsManagerMock> sensorReadingsManager_ =
        std::make_shared<testing::NiceMock<SensorReadingsManagerMock>>();
    std::shared_ptr<SensorReadingMock> sensorReading_ =
        std::make_shared<testing::NiceMock<SensorReadingMock>>();
    std::shared_ptr<PowerStateDbusSensor> sut_;
};

TEST_F(PowerStateDbusSensorTest, ReadingSetToUnavailableWhenThereIsNoDbusObject)
{
    EXPECT_CALL(*sensorReading_, setStatus(SensorReadingStatus::unavailable))
        .WillOnce(setPromise("wait"));
    dbusAcpiPowerStateStub_.reset();
    sut_->initialize();
    DbusEnvironment::waitForAllFutures();
}

TEST_F(PowerStateDbusSensorTest,
       ReadingSetToInvalidWhenInitializedWithUnexpectedSensorValue)
{
    EXPECT_CALL(*sensorReading_, setStatus(SensorReadingStatus::invalid))
        .WillOnce(setPromise("status"));
    BusctlCall::SetProperty(kPowerStateObjectPath, kPowerStateDbusInterface,
                            kPowerStatePropertyName, "XXX");
    sut_->initialize();
    DbusEnvironment::waitForAllFutures();
}

TEST_F(PowerStateDbusSensorTest,
       ReadingSetWithValueAndStatusValidWhenInitializedWithCorrectSensorValue)
{
    EXPECT_CALL(*sensorReading_, setStatus(SensorReadingStatus::valid))
        .WillOnce(setPromise("status"));
    EXPECT_CALL(*sensorReading_, updateValue(ValueType{PowerStateType::s0}))
        .WillOnce(setPromise("value"));
    BusctlCall::SetProperty(
        kPowerStateObjectPath, kPowerStateDbusInterface,
        kPowerStatePropertyName,
        "xyz.openbmc_project.Control.Power.ACPIPowerState.ACPI.S0_G0_D0");
    sut_->initialize();
    DbusEnvironment::waitForAllFutures();
}

TEST_F(PowerStateDbusSensorTest,
       ReadingSetWithValueAndStatusValidWhenSensorChanges)
{
    testing::Sequence seq;
    EXPECT_CALL(*sensorReading_, setStatus(testing::_))
        .InSequence(seq)
        .WillOnce(setPromise("status-on-init"));
    sut_->initialize();
    DbusEnvironment::waitForAllFutures();

    EXPECT_CALL(*sensorReading_, setStatus(SensorReadingStatus::valid))
        .InSequence(seq)
        .WillOnce(setPromise("status"));
    EXPECT_CALL(*sensorReading_, updateValue(ValueType{PowerStateType::s0}))
        .WillOnce(setPromise("value"));
    BusctlCall::SetProperty(
        kPowerStateObjectPath, kPowerStateDbusInterface,
        kPowerStatePropertyName,
        "xyz.openbmc_project.Control.Power.ACPIPowerState.ACPI.S0_G0_D0");
    DbusEnvironment::waitForAllFutures();
}

TEST_F(PowerStateDbusSensorTest,
       ReadingSetToInvalidWhenSensorChangesToUnexpected)
{
    testing::Sequence seq;
    EXPECT_CALL(*sensorReading_, setStatus(testing::_))
        .InSequence(seq)
        .WillOnce(setPromise("status-on-init"));
    sut_->initialize();
    DbusEnvironment::waitForAllFutures();

    EXPECT_CALL(*sensorReading_, setStatus(SensorReadingStatus::invalid))
        .InSequence(seq)
        .WillOnce(setPromise("status"));
    BusctlCall::SetProperty(kPowerStateObjectPath, kPowerStateDbusInterface,
                            kPowerStatePropertyName, "XXX");
    DbusEnvironment::waitForAllFutures();
}