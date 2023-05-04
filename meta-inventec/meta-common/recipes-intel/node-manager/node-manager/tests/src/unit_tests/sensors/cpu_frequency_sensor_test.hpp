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

#include "common_types.hpp"
#include "mocks/peci_commands_mock.hpp"
#include "mocks/sensor_reading_mock.hpp"
#include "mocks/sensor_readings_manager_mock.hpp"
#include "sensors/cpu_frequency_sensor.hpp"
#include "utils/common_test_settings.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

static constexpr uint8_t kEfficiencyRatioDefult = 8;
static constexpr uint8_t kMinOperatingRatioDefult = 5;
static constexpr uint64_t kC0Delta = 100;

class CpuFrequencySensorTest : public ::testing::Test
{
    struct CpuFrequencySensorWaitForTasks : public CpuFrequencySensor
    {
        CpuFrequencySensorWaitForTasks(
            std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManagerArg,
            std::shared_ptr<PeciCommandsIf> peciCommandsArg,
            unsigned int maxCpuNumberArg) :
            CpuFrequencySensor(sensorReadingsManagerArg, peciCommandsArg,
                               maxCpuNumberArg)
        {
        }
        ~CpuFrequencySensorWaitForTasks() = default;

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
            for (const auto& [key, future] : futuresNeededRatios)
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
    std::shared_ptr<CpuFrequencySensorWaitForTasks> sut_;
    std::map<SensorReadingType, std::vector<std::shared_ptr<SensorReadingMock>>>
        sensorReadings_;

    void SetUp() override
    {
        for (unsigned int deviceIndex = 0; deviceIndex < kSimulatedCpusNumber;
             deviceIndex++)
        {
            sensorReadings_[SensorReadingType::cpuAverageFrequency].push_back(
                std::make_shared<testing::NiceMock<SensorReadingMock>>());

            ON_CALL(*((sensorReadings_[SensorReadingType::cpuAverageFrequency])
                          .at(deviceIndex)),
                    getDeviceIndex())
                .WillByDefault(testing::Return(DeviceIndex(deviceIndex)));

            ON_CALL(*sensorReadingsManager_,
                    createSensorReading(SensorReadingType::cpuAverageFrequency,
                                        deviceIndex))
                .WillByDefault(testing::Return(
                    sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                        deviceIndex)));

            ON_CALL(*sensorReadingsManager_,
                    getSensorReading(SensorReadingType::cpuAverageFrequency,
                                     deviceIndex))
                .WillByDefault(testing::Return(
                    sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
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

        ON_CALL(*peciCommands_,
                getC0CounterSensor(testing::Lt(kSimulatedCpusNumber)))
            .WillByDefault(testing::Return(kC0Delta));
        ON_CALL(*peciCommands_,
                detectCores(testing::Lt(kSimulatedCpusNumber), kCpuIdDefault))
            .WillByDefault(testing::Return(kCoresNumber));
        ON_CALL(*peciCommands_,
                getMaxEfficiencyRatio(testing::Lt(kSimulatedCpusNumber),
                                      kCpuIdDefault))
            .WillByDefault(testing::Return(kEfficiencyRatioDefult));
        ON_CALL(*peciCommands_,
                getMinOperatingRatio(testing::Lt(kSimulatedCpusNumber),
                                     kCpuIdDefault))
            .WillByDefault(testing::Return(kMinOperatingRatioDefult));

        sut_ = std::make_shared<CpuFrequencySensorWaitForTasks>(
            sensorReadingsManager_, peciCommands_, kSimulatedCpusNumber);
    }
};

TEST_F(CpuFrequencySensorTest, CpuFrequencySensorForCpuOnShallBeAvailable)
{
    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOnIndex),
                setStatus(SensorReadingStatus::invalid));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuFrequencySensorTest, CpuFrequencySensorUnavailableWhenPowerStateOff)
{
    ON_CALL(*sensorReadingsManager_, isPowerStateOn())
        .WillByDefault(testing::Return(false));
    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOnIndex),
                setStatus(SensorReadingStatus::unavailable));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuFrequencySensorTest, CpuFrequencySensorForCpuOffShallBeNotAvailable)
{
    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOffIndex),
                setStatus(SensorReadingStatus::unavailable));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuFrequencySensorTest,
       CpuIdSensorNotAvailableSensorValueInvalidExpected)
{
    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOnIndex),
                setStatus(SensorReadingStatus::invalid));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);
    ON_CALL(*sensorReadingsManager_,
            getAvailableAndValueValidSensorReading(
                SensorReadingType::cpuPackageId, kCpuOnIndex))
        .WillByDefault(testing::Return(nullptr));
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuFrequencySensorTest,
       MinOperatingRatioReadFailedOverPeciSensorValueValidExpected)
{
    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOnIndex),
                setStatus(SensorReadingStatus::invalid));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);
    ON_CALL(*peciCommands_, getMinOperatingRatio(testing::_, kCpuIdDefault))
        .WillByDefault(testing::Return(std::nullopt));
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuFrequencySensorTest,
       MaxEfficiencyRatioReadFailedOverPeciSensorValueValidExpected)
{
    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOnIndex),
                setStatus(SensorReadingStatus::invalid));

    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOnIndex),
                setStatus(SensorReadingStatus::valid));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);
    ON_CALL(*peciCommands_, getMaxEfficiencyRatio(testing::_, kCpuIdDefault))
        .WillByDefault(testing::Return(std::nullopt));
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuFrequencySensorTest,
       CoreCountReadFailedOverPeciSensorValueInvalidExpected)
{
    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOnIndex),
                setStatus(SensorReadingStatus::invalid));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);
    ON_CALL(*peciCommands_, detectCores(testing::_, testing::_))
        .WillByDefault(testing::Return(std::nullopt));
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuFrequencySensorTest,
       C0ResidencyReadFailedOverPeciSensorValueInvalidExpected)
{
    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOnIndex),
                setStatus(SensorReadingStatus::invalid));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);
    ON_CALL(*peciCommands_, getC0CounterSensor(testing::_))
        .WillByDefault(testing::Return(std::nullopt));
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuFrequencySensorTest, C0ResidencyReadOverPeciSensorValueValidExpected)
{
    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOnIndex),
                setStatus(SensorReadingStatus::invalid));
    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOnIndex),
                setStatus(SensorReadingStatus::valid));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuFrequencySensorTest, NoDiffInTimeValueInvalidExpected)
{
    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOnIndex),
                setStatus(SensorReadingStatus::invalid))
        .Times(2);

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuFrequencySensorTest, ZeroValueCoreCountInvalidExpected)
{
    constexpr auto kCoresNum = 0;
    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOnIndex),
                setStatus(SensorReadingStatus::invalid))
        .Times(2);

    ON_CALL(*peciCommands_, detectCores(testing::_, testing::_))
        .WillByDefault(testing::Return(uint8_t(kCoresNum)));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuFrequencySensorTest, CorrectCalculationOfCpuFrequencyWithLoad)
{
    constexpr auto kCoresNum = 2;
    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOnIndex),
                updateValue(ValueType{1000.0}));

    ON_CALL(*peciCommands_, detectCores(testing::_, testing::_))
        .WillByDefault(testing::Return(uint8_t(kCoresNum)));
    ON_CALL(*peciCommands_, getC0CounterSensor(testing::_))
        .WillByDefault(testing::Return(0));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);

    ON_CALL(*peciCommands_, getC0CounterSensor(testing::_))
        .WillByDefault(testing::Return(200000000));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuFrequencySensorTest, CorrectCalculationOfCpuFrequencyBetweenPmPn)
{
    constexpr auto kCoresNum = 1;

    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOnIndex),
                updateValue(ValueType{400.0}));

    ON_CALL(*peciCommands_, detectCores(testing::_, testing::_))
        .WillByDefault(testing::Return(uint8_t(kCoresNum)));
    ON_CALL(*peciCommands_, getC0CounterSensor(testing::_))
        .WillByDefault(testing::Return(0));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);

    ON_CALL(*peciCommands_, getC0CounterSensor(testing::_))
        .WillByDefault(testing::Return(40000000));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuFrequencySensorTest, CorrectCalculationOfCpuFrequencyInIdle)
{
    constexpr auto kCoresNum = 1;

    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOnIndex),
                updateValue(ValueType{800.0}));

    ON_CALL(*peciCommands_, detectCores(testing::_, testing::_))
        .WillByDefault(testing::Return(uint8_t(kCoresNum)));
    ON_CALL(*peciCommands_, getC0CounterSensor(testing::_))
        .WillByDefault(testing::Return(0));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);

    ON_CALL(*peciCommands_, getC0CounterSensor(testing::_))
        .WillByDefault(testing::Return(39000000));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuFrequencySensorTest,
       NoDiffInC0ResidencyUpdateSensorWithPnValueExpected)
{
    EXPECT_CALL(*sensorReadings_[SensorReadingType::cpuAverageFrequency].at(
                    kCpuOnIndex),
                updateValue(ValueType{800.0}));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuFrequencySensorTest, DoNotRequestDataOverPeciWhenCpuIsUnavailable)
{
    EXPECT_CALL(*peciCommands_, getC0CounterSensor(kCpuOnIndex)).Times(1);
    EXPECT_CALL(*peciCommands_, getC0CounterSensor(kCpuOffIndex)).Times(0);

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(CpuFrequencySensorTest, DoNotRequestDataOverPeciWhenPowerStateOff)
{
    ON_CALL(*sensorReadingsManager_, isPowerStateOn())
        .WillByDefault(testing::Return(false));

    EXPECT_CALL(*peciCommands_, getC0CounterSensor(kCpuOnIndex)).Times(0);
    EXPECT_CALL(*peciCommands_, getC0CounterSensor(kCpuOffIndex)).Times(0);

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}