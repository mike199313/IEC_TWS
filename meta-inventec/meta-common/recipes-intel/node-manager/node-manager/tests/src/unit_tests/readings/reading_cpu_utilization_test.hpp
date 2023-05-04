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

#include "mocks/reading_consumer_mock.hpp"
#include "mocks/sensor_reading_mock.hpp"
#include "mocks/sensor_readings_manager_mock.hpp"
#include "readings/reading_cpu_utilization.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

static constexpr unsigned int kAllDevicesNum = 2;
static constexpr double kMaxReadingValue = 10.5;

struct SensorCpuUtilizationConfiguration
{
    CpuUtilizationType cpuValues;
    bool isValidAndAvailable;
};

class ReadingCpuUtilizationTest : public ::testing::Test
{
  public:
    std::map<DeviceIndex, std::shared_ptr<SensorReadingMock>> sensors_;
    std::shared_ptr<ReadingConsumerMock> readingConsumer_ =
        std::make_shared<testing::NiceMock<ReadingConsumerMock>>();
    std::shared_ptr<SensorReadingsManagerMock> sensorReadingsManager_ =
        std::make_shared<testing::NiceMock<SensorReadingsManagerMock>>();
    std::shared_ptr<ReadingCpuUtilization> sut_ =
        std::make_shared<ReadingCpuUtilization>(sensorReadingsManager_);

    void SetUp() override
    {
        for (DeviceIndex cpuIndex = 0; cpuIndex < 3; ++cpuIndex)
        {
            sensors_[cpuIndex] =
                std::make_shared<testing::NiceMock<SensorReadingMock>>();
        }
        ON_CALL(*sensorReadingsManager_,
                forEachSensorReading(SensorReadingType::cpuUtilization,
                                     testing::_, testing::_))
            .WillByDefault(testing::Invoke(
                [this](auto type, auto deviceIndex, auto action) {
                    if (deviceIndex == kAllDevices)
                    {
                        for (auto& [idx, sensor] : sensors_)
                        {
                            action(*sensor);
                        }
                    }
                    else
                    {
                        action(*sensors_[deviceIndex]);
                    }
                    return true;
                }));

        sut_ = std::make_shared<ReadingCpuUtilization>(sensorReadingsManager_);
        sut_->registerReadingConsumer(readingConsumer_,
                                      ReadingType::cpuUtilization,
                                      DeviceIndex{kAllDevices});
    }

    void setupSensor(DeviceIndex idx, bool isValid, CpuUtilizationType value)
    {
        ON_CALL(*sensors_[idx], getValue).WillByDefault(testing::Return(value));
        ON_CALL(*sensors_[idx], isGood).WillByDefault(testing::Return(isValid));
    }
};

TEST_F(ReadingCpuUtilizationTest, SingleSensorInvalidReadingReturnsNaN)
{
    setupSensor(DeviceIndex{0}, false, CpuUtilizationType{});
    setupSensor(DeviceIndex{1}, false, CpuUtilizationType{});
    setupSensor(DeviceIndex{2}, false, CpuUtilizationType{});
    EXPECT_CALL(*readingConsumer_,
                updateValue(testing::NanSensitiveDoubleEq(
                    std::numeric_limits<double>::quiet_NaN())));
    sut_->run();
}

TEST_F(ReadingCpuUtilizationTest, SingleSensorValidButDeltaZeroGeneratesNaN)
{
    setupSensor(
        DeviceIndex{0}, true,
        CpuUtilizationType{20, std::chrono::microseconds{0}, MHz{10}.count()});
    setupSensor(DeviceIndex{1}, false, CpuUtilizationType{});
    setupSensor(DeviceIndex{2}, false, CpuUtilizationType{});
    EXPECT_CALL(*readingConsumer_,
                updateValue(testing::NanSensitiveDoubleEq(
                    std::numeric_limits<double>::quiet_NaN())));
    sut_->run();
}

TEST_F(ReadingCpuUtilizationTest, SingleSensorValidButCmaxZeroGeneratesNaN)
{
    setupSensor(
        DeviceIndex{0}, true,
        CpuUtilizationType{20, std::chrono::microseconds{10}, MHz{0}.count()});
    setupSensor(DeviceIndex{1}, false, CpuUtilizationType{});
    setupSensor(DeviceIndex{2}, false, CpuUtilizationType{});
    EXPECT_CALL(*readingConsumer_,
                updateValue(testing::NanSensitiveDoubleEq(
                    std::numeric_limits<double>::quiet_NaN())));
    sut_->run();
}

TEST_F(ReadingCpuUtilizationTest, SingleSensorValidGeneratesProperResults)
{
    setupSensor(
        DeviceIndex{0}, true,
        CpuUtilizationType{20, std::chrono::microseconds{10}, MHz{10}.count()});
    setupSensor(DeviceIndex{1}, false, CpuUtilizationType{});
    setupSensor(DeviceIndex{2}, false, CpuUtilizationType{});
    EXPECT_CALL(*readingConsumer_, updateValue(testing::DoubleEq(20.0)));
    EXPECT_CALL(
        *readingConsumer_,
        reportEvent(ReadingEventType::readingAvailable,
                    testing::AllOf(testing::Field(&ReadingContext::type,
                                                  ReadingType::cpuUtilization),
                                   testing::Field(&ReadingContext::deviceIndex,
                                                  kAllDevices))));
    sut_->run();
}

TEST_F(ReadingCpuUtilizationTest,
       ManyCpuSensorsValidGeneratesOverallProperResults)
{
    setupSensor(DeviceIndex{0}, true,
                CpuUtilizationType{10, std::chrono::microseconds{500},
                                   MHz{21}.count()});
    setupSensor(DeviceIndex{1}, true,
                CpuUtilizationType{20, std::chrono::microseconds{600},
                                   MHz{22}.count()});
    setupSensor(DeviceIndex{2}, true,
                CpuUtilizationType{30, std::chrono::microseconds{700},
                                   MHz{23}.count()});
    EXPECT_CALL(
        *readingConsumer_,
        updateValue(testing::DoubleEq(
            100.0 * (10.0 / 500 + 20.0 / 600 + 30.0 / 700) / (21 + 22 + 23))));
    EXPECT_CALL(
        *readingConsumer_,
        reportEvent(ReadingEventType::readingAvailable,
                    testing::AllOf(testing::Field(&ReadingContext::type,
                                                  ReadingType::cpuUtilization),
                                   testing::Field(&ReadingContext::deviceIndex,
                                                  kAllDevices))));
    sut_->run();
}
