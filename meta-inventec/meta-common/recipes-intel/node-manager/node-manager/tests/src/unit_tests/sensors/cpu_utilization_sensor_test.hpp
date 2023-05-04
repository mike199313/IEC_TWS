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

#include "clock.hpp"
#include "common_types.hpp"
#include "mocks/peci_commands_mock.hpp"
#include "mocks/sensor_reading_mock.hpp"
#include "mocks/sensor_readings_manager_mock.hpp"
#include "sensors/cpu_utilization_sensor.hpp"
#include "sensors/peci/peci_commands.hpp"
#include "sensors/peci_sensor.hpp"
#include "sensors/sensor_reading_if.hpp"
#include "sensors/sensor_reading_type.hpp"
#include "sensors/sensor_readings_manager.hpp"
#include "utils/common_test_settings.hpp"
#include "utils/time.hpp"

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-death-test-internal.h>

static constexpr uint32_t kTurboEnabled = 1;
static constexpr uint64_t kC0DeltaDefault = 100;
static constexpr uint8_t kMaxTurboRatioDefault = 10;
static constexpr uint8_t kMaxNonTurboRatioDefault = 10;

using namespace nodemanager;

class CpuUtilizationSensorTest : public ::testing::Test
{
  protected:
    struct CpuUtilizationSensorWaitForTasks : public CpuUtilizationSensor
    {
        CpuUtilizationSensorWaitForTasks(
            std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManagerArg,
            std::shared_ptr<PeciCommandsIf> peciCommandsArg,
            unsigned int maxCpuNumberArg) :
            CpuUtilizationSensor(sensorReadingsManagerArg, peciCommandsArg,
                                 maxCpuNumberArg)
        {
        }
        ~CpuUtilizationSensorWaitForTasks() = default;

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
            for (const auto& [key, future] : futuresMaxCpu)
            {
                if (future.valid() &&
                    future.wait_until(exit_time) != std::future_status::ready)
                {
                    throw std::logic_error("Timeout was hit!");
                }
            }
        }
    };

    void SetUp() override
    {

        for (unsigned int deviceIndex = 0; deviceIndex < kSimulatedCpusNumber;
             deviceIndex++)
        {
            sensorReadings_[SensorReadingType::cpuUtilization].push_back(
                std::make_shared<testing::NiceMock<SensorReadingMock>>());

            ON_CALL(*sensorReadings_[SensorReadingType::cpuUtilization].at(
                        deviceIndex),
                    getDeviceIndex())
                .WillByDefault(testing::Return(deviceIndex));
            ON_CALL(*sensorReadingsManager_,
                    createSensorReading(SensorReadingType::cpuUtilization,
                                        deviceIndex))
                .WillByDefault(testing::Return(
                    sensorReadings_[SensorReadingType::cpuUtilization].at(
                        deviceIndex)));

            ON_CALL(*sensorReadingsManager_,
                    getSensorReading(SensorReadingType::cpuUtilization,
                                     deviceIndex))
                .WillByDefault(testing::Return(
                    sensorReadings_[SensorReadingType::cpuUtilization].at(
                        deviceIndex)));

            sensorReadings_[SensorReadingType::cpuPackageId].push_back(
                std::make_shared<testing::NiceMock<SensorReadingMock>>());

            ON_CALL(*sensorReadingsManager_,
                    getAvailableAndValueValidSensorReading(
                        SensorReadingType::cpuPackageId, deviceIndex))
                .WillByDefault(testing::Return(
                    sensorReadings_[SensorReadingType::cpuPackageId].at(
                        deviceIndex)));
        }
        ON_CALL(*sensorReadingsManager_, isCpuAvailable(kCpuOnIndex))
            .WillByDefault(testing::Return(true));
        ON_CALL(*sensorReadingsManager_, isCpuAvailable(kCpuOffIndex))
            .WillByDefault(testing::Return(false));

        ON_CALL(*sensorReadingsManager_, isPowerStateOn())
            .WillByDefault(testing::Return(true));

        ON_CALL(
            *sensorReadings_[SensorReadingType::cpuPackageId].at(kCpuOnIndex),
            getValue())
            .WillByDefault(testing::Return(kCpuIdDefault));

        for (auto const& [type, sensorReadingsSameType] : sensorReadings_)
        {
            for (unsigned int deviceIndex = 0;
                 deviceIndex < kSimulatedCpusNumber; deviceIndex++)
            {
                ON_CALL(*sensorReadingsSameType.at(deviceIndex),
                        getDeviceIndex())
                    .WillByDefault(testing::Return(DeviceIndex(deviceIndex)));
            }
        }

        ON_CALL(*peciCommands_,
                getC0CounterSensor(testing::Lt(kSimulatedCpusNumber)))
            .WillByDefault(testing::Return(kC0DeltaDefault));
        ON_CALL(
            *peciCommands_,
            isTurboEnabled(testing::Lt(kSimulatedCpusNumber), kCpuIdDefault))
            .WillByDefault(testing::Return(kTurboEnabled));
        ON_CALL(*peciCommands_,
                detectCores(testing::Lt(kSimulatedCpusNumber), kCpuIdDefault))
            .WillByDefault(testing::Return(kCoresNumber));
        ON_CALL(*peciCommands_,
                detectMinTurboRatio(testing::Lt(kSimulatedCpusNumber),
                                    kCpuIdDefault, kCoresNumber))
            .WillByDefault(testing::Return(kMaxTurboRatioDefault));
        ON_CALL(*peciCommands_,
                getMaxNonTurboRatio(testing::Lt(kSimulatedCpusNumber),
                                    kCpuIdDefault))
            .WillByDefault(testing::Return(kMaxNonTurboRatioDefault));

        sut_ = std::make_shared<
            testing::NiceMock<CpuUtilizationSensorWaitForTasks>>(
            sensorReadingsManager_, peciCommands_, kSimulatedCpusNumber);
    }

    std::shared_ptr<PeciCommandsMock> peciCommands_ =
        std::make_shared<testing::NiceMock<PeciCommandsMock>>();
    std::shared_ptr<SensorReadingsManagerMock> sensorReadingsManager_ =
        std::make_shared<testing::NiceMock<SensorReadingsManagerMock>>();
    std::shared_ptr<CpuUtilizationSensorWaitForTasks> sut_;
    std::map<SensorReadingType, std::vector<std::shared_ptr<SensorReadingMock>>>
        sensorReadings_;
};

TEST_F(CpuUtilizationSensorTest, CpuUtilizationSensorForCpuOnShallBeAvailable)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuUtilization].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::invalid));
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuUtilizationSensorTest,
       CpuUtilizationSensorUnavailableWhenPowerStateOff)
{
    ON_CALL(*sensorReadingsManager_, isPowerStateOn())
        .WillByDefault(testing::Return(false));
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuUtilization].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::unavailable));
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuUtilizationSensorTest,
       CpuUtilizationSensorForCpuOffShallBeNotAvailable)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuUtilization].at(kCpuOffIndex),
        setStatus(SensorReadingStatus::unavailable))
        .Times(2);
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuUtilizationSensorTest,
       C0ResidencyReadFailedOverPeciSensorValueInvalidExpected)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuUtilization].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::invalid));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);
    ON_CALL(*peciCommands_, getC0CounterSensor(testing::_))
        .WillByDefault(testing::Return(std::nullopt));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuUtilizationSensorTest,
       C0ResidencyReadOverPeciSensorValueValidExpected)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuUtilization].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::invalid));
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuUtilization].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::valid));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuUtilizationSensorTest,
       NoDiffInC0ResidencyUpdateSensorWithZeroValueExpected)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuUtilization].at(kCpuOnIndex),
        updateValue(ValueType{CpuUtilizationType{
            0, std::chrono::microseconds{kNodeManagerSyncTimeIntervalUs},
            kMaxTurboRatioDefault * 100}}));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuUtilizationSensorTest, NoDiffInTimeValueValidExpected)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuUtilization].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::invalid));
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuUtilization].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::valid));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuUtilizationSensorTest,
       MaxTurboRatioIsFetchedOncePerMaxUtilizationDivider)
{
    EXPECT_CALL(*peciCommands_,
                detectMinTurboRatio(testing::_, testing::_, testing::_))
        .Times(3);
    for (unsigned i = 0; i < kMaxUtilizationDivider * 3; i++)
    {
        sut_->run();
        sut_->waitForAllTasks(std::chrono::seconds{5});
    }
}

TEST_F(
    CpuUtilizationSensorTest,
    CpuIdSensorNotAvailableWhenUpdatingMaxCpuUtilizationSensorValueStillValidExpected)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuUtilization].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::invalid));
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuUtilization].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::valid))
        .Times(100);

    for (unsigned i = 0; i < kMaxUtilizationDivider; i++)
    {
        sut_->run();
        sut_->waitForAllTasks(std::chrono::seconds{5});
        Clock::stepMs(1);
    }

    ON_CALL(*sensorReadingsManager_,
            getAvailableAndValueValidSensorReading(
                SensorReadingType::cpuPackageId, kCpuOnIndex))
        .WillByDefault(testing::Return(nullptr));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(
    CpuUtilizationSensorTest,
    TurboDetectionFailedOverPeciWhenUpdatingMaxCpuUtilizationSensorValueStillValidExpected)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuUtilization].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::invalid));
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuUtilization].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::valid))
        .Times(100);

    for (unsigned i = 0; i < kMaxUtilizationDivider; i++)
    {
        sut_->run();
        sut_->waitForAllTasks(std::chrono::seconds{5});
        Clock::stepMs(1);
    }
    ON_CALL(*peciCommands_, isTurboEnabled(testing::_, kCpuIdDefault))
        .WillByDefault(testing::Return(std::nullopt));
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(
    CpuUtilizationSensorTest,
    CoresDetectionFailedOverPeciWhenUpdatingMaxCpuUtilizationSensorValueStillValidExpected)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuUtilization].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::invalid));
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuUtilization].at(kCpuOnIndex),
        setStatus(SensorReadingStatus::valid))
        .Times(100);

    for (unsigned i = 0; i < kMaxUtilizationDivider; i++)
    {
        sut_->run();
        sut_->waitForAllTasks(std::chrono::seconds{5});
        Clock::stepMs(1);
    }

    ON_CALL(*peciCommands_, detectCores(testing::_, kCpuIdDefault))
        .WillByDefault(testing::Return(std::nullopt));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuUtilizationSensorTest, CorrectCalculationOfCpuUtilization)
{
    constexpr auto kCoresNum = 1;
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::cpuUtilization].at(kCpuOnIndex),
        updateValue(ValueType{CpuUtilizationType{
            20000, std::chrono::microseconds{kNodeManagerSyncTimeIntervalUs},
            kMaxTurboRatioDefault * 100}}));

    ON_CALL(*peciCommands_, detectCores(testing::_, testing::_))
        .WillByDefault(testing::Return(uint8_t(kCoresNum)));
    ON_CALL(*peciCommands_,
            detectMinTurboRatio(testing::_, testing::_, testing::_))
        .WillByDefault(testing::Return(
            uint8_t(std::chrono::duration_cast<HundredMHz>(GHz{1}).count())));
    ON_CALL(*peciCommands_, getC0CounterSensor(testing::_))
        .WillByDefault(testing::Return(0));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);

    ON_CALL(*peciCommands_, getC0CounterSensor(testing::_))
        .WillByDefault(testing::Return(20000));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuUtilizationSensorTest, DoNotRequestDataOverPeciWhenCpuIsUnavailable)
{
    EXPECT_CALL(*peciCommands_, getC0CounterSensor(kCpuOnIndex)).Times(1);
    EXPECT_CALL(*peciCommands_, getC0CounterSensor(kCpuOffIndex)).Times(0);

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuUtilizationSensorTest, DoNotRequestDataOverPeciWhenPowerStateOff)
{
    ON_CALL(*sensorReadingsManager_, isPowerStateOn())
        .WillByDefault(testing::Return(false));
    EXPECT_CALL(*peciCommands_, getC0CounterSensor(kCpuOnIndex)).Times(0);
    EXPECT_CALL(*peciCommands_, getC0CounterSensor(kCpuOffIndex)).Times(0);

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}