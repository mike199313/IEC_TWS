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
#include "sensors/peci_sensor.hpp"
#include "utils/common_test_settings.hpp"
#include "utils/dbus_environment.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class PeciSensorTest : public ::testing::Test
{
    struct PeciSensorWaitForTasks : public PeciSensor
    {
        PeciSensorWaitForTasks(
            std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManagerArg,
            std::shared_ptr<PeciCommandsIf> peciCommandsArg,
            unsigned int maxCpuNumberArg) :
            PeciSensor(sensorReadingsManagerArg, peciCommandsArg,
                       maxCpuNumberArg)
        {
        }
        ~PeciSensorWaitForTasks() = default;

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
    std::shared_ptr<PeciSensorWaitForTasks> sut_;
    std::map<SensorReadingType, std::vector<std::shared_ptr<SensorReadingMock>>>
        sensorReadings_;

    void emulateSensor(DeviceIndex deviceIndex, SensorReadingType sensorType)
    {
        sensorReadings_[sensorType].push_back(
            std::make_shared<testing::NiceMock<SensorReadingMock>>());

        ON_CALL(*((sensorReadings_[sensorType]).at(deviceIndex)),
                getDeviceIndex())
            .WillByDefault(testing::Return(DeviceIndex(deviceIndex)));

        ON_CALL(*((sensorReadings_[sensorType]).at(deviceIndex)),
                getSensorReadingType())
            .WillByDefault(testing::Return(sensorType));

        ON_CALL(*sensorReadingsManager_,
                createSensorReading(sensorType, deviceIndex))
            .WillByDefault(
                testing::Return(sensorReadings_[sensorType].at(deviceIndex)));

        ON_CALL(*sensorReadingsManager_,
                getSensorReading(sensorType, deviceIndex))
            .WillByDefault(
                testing::Return(sensorReadings_[sensorType].at(deviceIndex)));
    }

    void SetUp() override
    {
        for (unsigned int deviceIndex = 0; deviceIndex < kSimulatedCpusNumber;
             deviceIndex++)
        {
            emulateSensor(deviceIndex, SensorReadingType::cpuPackageId);
            emulateSensor(deviceIndex,
                          SensorReadingType::prochotRatioCapabilitiesMin);
            emulateSensor(deviceIndex,
                          SensorReadingType::prochotRatioCapabilitiesMax);
            emulateSensor(deviceIndex,
                          SensorReadingType::turboRatioCapabilitiesMin);
            emulateSensor(deviceIndex,
                          SensorReadingType::turboRatioCapabilitiesMax);
            emulateSensor(deviceIndex, SensorReadingType::cpuDieMask);

            ON_CALL(*sensorReadingsManager_,
                    getAvailableAndValueValidSensorReading(
                        SensorReadingType::cpuPackageId, deviceIndex))
                .WillByDefault(testing::Return(
                    sensorReadings_[SensorReadingType::cpuPackageId].at(
                        deviceIndex)));
            ON_CALL(*sensorReadings_[SensorReadingType::cpuPackageId].at(
                        deviceIndex),
                    getValue())
                .WillByDefault(testing::Return(kCpuIdDefault));
        }

        ON_CALL(*sensorReadingsManager_, isCpuAvailable(kCpuOnIndex))
            .WillByDefault(testing::Return(true));

        ON_CALL(*sensorReadingsManager_, isCpuAvailable(kCpuOffIndex))
            .WillByDefault(testing::Return(false));

        ON_CALL(*sensorReadingsManager_, isPowerStateOn())
            .WillByDefault(testing::Return(true));

        ON_CALL(*peciCommands_, getCpuId(testing::Lt(kSimulatedCpusNumber)))
            .WillByDefault(testing::Return(kCpuIdDefault));

        sut_ = std::make_shared<PeciSensorWaitForTasks>(
            sensorReadingsManager_, peciCommands_, kSimulatedCpusNumber);
    }
};

TEST_F(PeciSensorTest, InvalidWhenNoResponseFromPeci)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::prochotRatioCapabilitiesMin].at(
            kCpuOnIndex),
        setStatus(SensorReadingStatus::invalid));
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::turboRatioCapabilitiesMin].at(
            kCpuOnIndex),
        setStatus(SensorReadingStatus::invalid));

    ON_CALL(
        *peciCommands_,
        getMinOperatingRatio(testing::Lt(kSimulatedCpusNumber), kCpuIdDefault))
        .WillByDefault(testing::Return(std::nullopt));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(PeciSensorTest, MinValidWhenPositiveResponseFromPeci)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::prochotRatioCapabilitiesMin].at(
            kCpuOnIndex),
        setStatus(SensorReadingStatus::valid));

    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::prochotRatioCapabilitiesMin].at(
            kCpuOnIndex),
        updateValue(testing::VariantWith<uint8_t>(5)));

    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::turboRatioCapabilitiesMin].at(
            kCpuOnIndex),
        setStatus(SensorReadingStatus::valid));

    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::turboRatioCapabilitiesMin].at(
            kCpuOnIndex),
        updateValue(testing::VariantWith<uint8_t>(5)));

    ON_CALL(
        *peciCommands_,
        getMinOperatingRatio(testing::Lt(kSimulatedCpusNumber), kCpuIdDefault))
        .WillByDefault(testing::Return(std::optional<uint8_t>(uint8_t{5})));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(PeciSensorTest, MaxValidWhenPositiveResponseFromPeci)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::prochotRatioCapabilitiesMax].at(
            kCpuOnIndex),
        setStatus(SensorReadingStatus::valid));

    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::prochotRatioCapabilitiesMax].at(
            kCpuOnIndex),
        updateValue(testing::VariantWith<uint8_t>(5)));

    ON_CALL(
        *peciCommands_,
        getMaxNonTurboRatio(testing::Lt(kSimulatedCpusNumber), kCpuIdDefault))
        .WillByDefault(testing::Return(std::optional<uint8_t>(uint8_t{5})));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(PeciSensorTest, ValueForCpuOffShallBeNotAvailable)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::prochotRatioCapabilitiesMin].at(
            kCpuOffIndex),
        setStatus(SensorReadingStatus::unavailable));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(PeciSensorTest, WhenPowerStateOffSensorIsSetToNotAvailable)
{
    ON_CALL(*sensorReadingsManager_, isPowerStateOn())
        .WillByDefault(testing::Return(false));
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::prochotRatioCapabilitiesMin].at(
            kCpuOnIndex),
        setStatus(SensorReadingStatus::unavailable));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(PeciSensorTest, DoNotRequestDataOverPeciWhenCpuIsUnavailable)
{
    EXPECT_CALL(*peciCommands_,
                getMinOperatingRatio(kCpuOnIndex, kCpuIdDefault))
        .Times(2);
    EXPECT_CALL(*peciCommands_,
                getMinOperatingRatio(kCpuOffIndex, kCpuIdDefault))
        .Times(0);
    EXPECT_CALL(*peciCommands_, getMaxNonTurboRatio(kCpuOnIndex, kCpuIdDefault))
        .Times(1);
    EXPECT_CALL(*peciCommands_,
                getMaxNonTurboRatio(kCpuOffIndex, kCpuIdDefault))
        .Times(0);
    EXPECT_CALL(*peciCommands_, detectMaxTurboRatio(kCpuOnIndex, testing::_))
        .Times(1);
    EXPECT_CALL(*peciCommands_, detectMaxTurboRatio(kCpuOffIndex, testing::_))
        .Times(0);

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(PeciSensorTest, DoNotRequestDataOverPeciWhenPowerStateIsOff)
{
    ON_CALL(*sensorReadingsManager_, isPowerStateOn())
        .WillByDefault(testing::Return(false));

    EXPECT_CALL(*peciCommands_, getCpuId(kCpuOnIndex)).Times(0);
    EXPECT_CALL(*peciCommands_, getCpuId(kCpuOffIndex)).Times(0);
    EXPECT_CALL(*peciCommands_,
                getMinOperatingRatio(kCpuOnIndex, kCpuIdDefault))
        .Times(0);
    EXPECT_CALL(*peciCommands_,
                getMinOperatingRatio(kCpuOffIndex, kCpuIdDefault))
        .Times(0);
    EXPECT_CALL(*peciCommands_, getMaxNonTurboRatio(kCpuOnIndex, kCpuIdDefault))
        .Times(0);
    EXPECT_CALL(*peciCommands_,
                getMaxNonTurboRatio(kCpuOffIndex, kCpuIdDefault))
        .Times(0);
    EXPECT_CALL(*peciCommands_, detectMaxTurboRatio(kCpuOnIndex, testing::_))
        .Times(0);
    EXPECT_CALL(*peciCommands_, detectMaxTurboRatio(kCpuOffIndex, testing::_))
        .Times(0);
    EXPECT_CALL(*peciCommands_, getCpuDieMask(kCpuOffIndex)).Times(0);

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_F(PeciSensorTest, InvalidDeviceCpuIndexExpectReadingValidSetToFalse)
{
    EXPECT_CALL(
        *sensorReadings_[SensorReadingType::prochotRatioCapabilitiesMin].at(
            kCpuOnIndex),
        setStatus(SensorReadingStatus::invalid));

    for (unsigned int deviceIndex = 2; deviceIndex < 4; deviceIndex++)
    {
        sensorReadings_[SensorReadingType::prochotRatioCapabilitiesMin]
            .push_back(
                std::make_shared<testing::NiceMock<SensorReadingMock>>());

        ON_CALL(
            *((sensorReadings_[SensorReadingType::prochotRatioCapabilitiesMin])
                  .at(deviceIndex)),
            getDeviceIndex())
            .WillByDefault(testing::Return(DeviceIndex(deviceIndex)));

        ON_CALL(*sensorReadingsManager_,
                getSensorReading(SensorReadingType::prochotRatioCapabilitiesMin,
                                 deviceIndex))
            .WillByDefault(testing::Return(
                sensorReadings_[SensorReadingType::prochotRatioCapabilitiesMin]
                    .at(deviceIndex)));

        ON_CALL(*sensorReadingsManager_, isCpuAvailable(deviceIndex))
            .WillByDefault(testing::Return(true));
    }

    ON_CALL(*((sensorReadings_[SensorReadingType::prochotRatioCapabilitiesMin])
                  .at(0)),
            getDeviceIndex())
        .WillByDefault(testing::Return(DeviceIndex(3)));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    Clock::stepMs(kNodeManagerSyncTimeIntervalMs);

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}
