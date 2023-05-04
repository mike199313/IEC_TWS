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

#include "common_types.hpp"
#include "mocks/peci_commands_mock.hpp"
#include "mocks/sensor_reading_mock.hpp"
#include "mocks/sensor_readings_manager_mock.hpp"
#include "sensors/cpu_efficiency_sensor.hpp"
#include "utils/common_test_settings.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

static constexpr auto kEpiDefault = 100;

class CpuEfficiencySensorTest : public ::testing::Test
{
    struct CpuEfficiencySensorWaitForTasks : public CpuEfficiencySensor
    {
        CpuEfficiencySensorWaitForTasks(
            std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManagerArg,
            std::shared_ptr<PeciCommandsIf> peciCommandsArg,
            unsigned int maxCpuNumberArg) :
            CpuEfficiencySensor(sensorReadingsManagerArg, peciCommandsArg,
                                maxCpuNumberArg)
        {
        }
        ~CpuEfficiencySensorWaitForTasks() = default;

        void waitForAllTasks(const std::chrono::milliseconds& timeout)
        {
            auto exit_time = std::chrono::steady_clock::now() + timeout;
            for (const auto& [key, future] : futureSamples)
            {
                if (future.valid() &&
                    future.wait_until(exit_time) != std::future_status::ready)
                {
                    throw std::logic_error("Timeout was hit!");
                }
            }
        }
    };

  protected:
    std::shared_ptr<PeciCommandsMock> peciCommands_ =
        std::make_shared<testing::NiceMock<PeciCommandsMock>>();
    std::shared_ptr<SensorReadingsManagerMock> sensorReadingsManager_ =
        std::make_shared<testing::NiceMock<SensorReadingsManagerMock>>();
    std::shared_ptr<CpuEfficiencySensorWaitForTasks> sut_;
    std::map<SensorReadingType, std::vector<std::shared_ptr<SensorReadingMock>>>
        sensorReadings_;

    void SetUp() override
    {
        for (unsigned int deviceIndex = 0; deviceIndex < kSimulatedCpusNumber;
             deviceIndex++)
        {
            sensorReadings_[SensorReadingType::cpuEfficiency].push_back(
                std::make_shared<testing::NiceMock<SensorReadingMock>>());

            ON_CALL(*((sensorReadings_[SensorReadingType::cpuEfficiency])
                          .at(deviceIndex)),
                    getDeviceIndex())
                .WillByDefault(testing::Return(DeviceIndex(deviceIndex)));

            ON_CALL(*sensorReadingsManager_,
                    createSensorReading(SensorReadingType::cpuEfficiency,
                                        deviceIndex))
                .WillByDefault(testing::Return(
                    sensorReadings_[SensorReadingType::cpuEfficiency].at(
                        deviceIndex)));

            ON_CALL(
                *sensorReadingsManager_,
                getSensorReading(SensorReadingType::cpuEfficiency, deviceIndex))
                .WillByDefault(testing::Return(
                    sensorReadings_[SensorReadingType::cpuEfficiency].at(
                        deviceIndex)));
        }
        ON_CALL(*sensorReadingsManager_, isCpuAvailable(kCpuOnIndex))
            .WillByDefault(testing::Return(true));

        ON_CALL(*sensorReadingsManager_, isCpuAvailable(kCpuOffIndex))
            .WillByDefault(testing::Return(false));

        ON_CALL(*sensorReadingsManager_, isPowerStateOn())
            .WillByDefault(testing::Return(true));

        ON_CALL(*peciCommands_,
                getEpiCounterSensor(testing::Lt(kSimulatedCpusNumber)))
            .WillByDefault(testing::Return(kEpiDefault));

        sut_ = std::make_shared<CpuEfficiencySensorWaitForTasks>(
            sensorReadingsManager_, peciCommands_, kSimulatedCpusNumber);
    }
};

TEST_F(CpuEfficiencySensorTest, CpuEfficiencySensorForCpuOnShallBeAvailable)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuEfficiency].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::invalid));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuEfficiencySensorTest, CpuEfficiencySensorForCpuOffShallBeNotAvailable)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuEfficiency].at(kCpuOffIndex),
        setStatus(SensorReadingStatus::unavailable));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuEfficiencySensorTest, EpiReadFailedOverPeciSensorValueInvalidExpected)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuEfficiency].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::invalid));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);
    ON_CALL(*peciCommands_, getEpiCounterSensor(testing::_))
        .WillByDefault(testing::Return(std::nullopt));
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuEfficiencySensorTest, EpiReadOverPeciSensorValueValidExpected)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuEfficiency].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::invalid));
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuEfficiency].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::valid));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuEfficiencySensorTest, NoDiffInEpiUpdateSensorWithZeroValueExpected)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuEfficiency].at(kCpuOnIndex),
        updateValue(ValueType{0.0}));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuEfficiencySensorTest, NoDiffInTimeValueInvalidExpected)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuEfficiency].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::invalid))
        .Times(2);

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuEfficiencySensorTest, VerifySensorReadingValues)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuEfficiency].at(kCpuOnIndex),
        updateValue(ValueType{static_cast<double>(400) /
                              kNodeManagerSyncTimeIntervalUs}));

    ON_CALL(*peciCommands_, getEpiCounterSensor(kCpuOnIndex))
        .WillByDefault(testing::Return(100));
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    ON_CALL(*peciCommands_, getEpiCounterSensor(kCpuOnIndex))
        .WillByDefault(testing::Return(500));
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuEfficiencySensorTest, DoNotRequestDataOverPeciWhenCpuIsUnavailable)
{
    EXPECT_CALL(*peciCommands_, getEpiCounterSensor(kCpuOnIndex)).Times(1);
    EXPECT_CALL(*peciCommands_, getEpiCounterSensor(kCpuOffIndex)).Times(0);

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuEfficiencySensorTest, DoNotRequestDataOverPeciWhenPowerStateOff)
{
    ON_CALL(*sensorReadingsManager_, isPowerStateOn())
        .WillByDefault(testing::Return(false));
    EXPECT_CALL(*peciCommands_, getEpiCounterSensor(kCpuOnIndex)).Times(0);
    EXPECT_CALL(*peciCommands_, getEpiCounterSensor(kCpuOffIndex)).Times(0);

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuEfficiencySensorTest, SensorIsUnavailableWhenPowerStateOff)
{
    ON_CALL(*sensorReadingsManager_, isPowerStateOn())
        .WillByDefault(testing::Return(false));
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuEfficiency].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::unavailable))
        .Times(1);
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuEfficiencySensorTest,
       InvalidDeviceCpuIndexExpectReadingValidSetToFalse)
{
    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuEfficiency].at(0),
                setStatus(SensorReadingStatus::invalid));

    for (unsigned int deviceIndex = 2; deviceIndex < 4; deviceIndex++)
    {
        sensorReadings_[SensorReadingType::cpuEfficiency].push_back(
            std::make_shared<testing::NiceMock<SensorReadingMock>>());

        ON_CALL(*((sensorReadings_[SensorReadingType::cpuEfficiency])
                      .at(deviceIndex)),
                getDeviceIndex())
            .WillByDefault(testing::Return(DeviceIndex(deviceIndex)));

        ON_CALL(*sensorReadingsManager_,
                getSensorReading(SensorReadingType::cpuEfficiency, deviceIndex))
            .WillByDefault(testing::Return(
                sensorReadings_[SensorReadingType::cpuEfficiency].at(
                    deviceIndex)));

        ON_CALL(*sensorReadingsManager_, isCpuAvailable(deviceIndex))
            .WillByDefault(testing::Return(true));
    }

    ON_CALL(*((sensorReadings_[SensorReadingType::cpuEfficiency]).at(0)),
            getDeviceIndex())
        .WillByDefault(testing::Return(DeviceIndex(3)));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}
