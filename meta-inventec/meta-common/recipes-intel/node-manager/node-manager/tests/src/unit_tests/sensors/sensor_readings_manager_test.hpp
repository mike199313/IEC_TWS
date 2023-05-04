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
#include "mocks/reading_consumer_mock.hpp"
#include "sensors/sensor_reading.hpp"
#include "sensors/sensor_reading_type.hpp"
#include "sensors/sensor_readings_manager.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

constexpr static const DeviceIndex device0{0};

class SensorReadingsManagerFixture
{
  protected:
    std::shared_ptr<SensorReadingsManager> sut_ =
        std::make_shared<SensorReadingsManager>();
    testing::NiceMock<testing::MockFunction<void(SensorReadingIf&)>> callback_;
};

class SensorReadingsManagerTest : public SensorReadingsManagerFixture,
                                  public ::testing::Test
{
};

TEST_F(SensorReadingsManagerTest,
       isGpuPowerStateOnReturnsFalseWhenGpuPowerStateNotPresent)
{
    EXPECT_FALSE(sut_->isGpuPowerStateOn());
}

TEST_F(SensorReadingsManagerTest,
       isGpuPowerStateOnReturnsFalseWhenGpuPowerStateUnavailable)
{
    std::shared_ptr<SensorReadingIf> s =
        sut_->createSensorReading(SensorReadingType::gpuPowerState, device0);

    s->setStatus(SensorReadingStatus::unavailable);
    EXPECT_FALSE(sut_->isGpuPowerStateOn());
}

TEST_F(SensorReadingsManagerTest,
       isGpuPowerStateOnReturnsFalseWhenGpuPowerStateInvalid)
{
    std::shared_ptr<SensorReadingIf> s =
        sut_->createSensorReading(SensorReadingType::gpuPowerState, device0);

    s->setStatus(SensorReadingStatus::invalid);
    EXPECT_FALSE(sut_->isGpuPowerStateOn());
}

TEST_F(SensorReadingsManagerTest,
       isGpuPowerStateOnReturnsFlaseWhenGpuPowerStateInvalidButOn)
{
    std::shared_ptr<SensorReadingIf> s =
        sut_->createSensorReading(SensorReadingType::gpuPowerState, device0);

    s->setStatus(SensorReadingStatus::invalid);
    s->updateValue(GpuPowerState::on);
    EXPECT_FALSE(sut_->isGpuPowerStateOn());
}

TEST_F(SensorReadingsManagerTest,
       isGpuPowerStateOnReturnsFalseWhenGpuPowerStateValidButOff)
{
    std::shared_ptr<SensorReadingIf> s =
        sut_->createSensorReading(SensorReadingType::gpuPowerState, device0);

    s->setStatus(SensorReadingStatus::valid);
    s->updateValue(GpuPowerState::off);
    EXPECT_FALSE(sut_->isGpuPowerStateOn());
}

TEST_F(SensorReadingsManagerTest,
       isGpuPowerStateOnReturnsTrueWhenGpuPowerStateValidAndOn)
{
    std::shared_ptr<SensorReadingIf> s =
        sut_->createSensorReading(SensorReadingType::gpuPowerState, device0);

    s->setStatus(SensorReadingStatus::valid);
    s->updateValue(GpuPowerState::on);
    EXPECT_TRUE(sut_->isGpuPowerStateOn());
}

TEST_F(SensorReadingsManagerTest,
       isGpuPowerStateOnThrowsWhenGpuPowerStateHasUnexpectedType)
{
    std::shared_ptr<SensorReadingIf> s =
        sut_->createSensorReading(SensorReadingType::gpuPowerState, device0);

    s->setStatus(SensorReadingStatus::valid);
    s->updateValue(1.23);
    EXPECT_THROW(sut_->isGpuPowerStateOn(), std::logic_error);
}

TEST_F(SensorReadingsManagerTest,
       isPowerStateOnReturnsFalseWhenPowerStateNotPresent)
{
    EXPECT_FALSE(sut_->isPowerStateOn());
}

TEST_F(SensorReadingsManagerTest,
       isPowerStateOnReturnsFalseWhenPowerStateUnavailable)
{
    std::shared_ptr<SensorReadingIf> s =
        sut_->createSensorReading(SensorReadingType::powerState, device0);

    s->setStatus(SensorReadingStatus::unavailable);
    EXPECT_FALSE(sut_->isPowerStateOn());
}

TEST_F(SensorReadingsManagerTest,
       isPowerStateOnReturnsFalseWhenPowerStateInvalid)
{
    std::shared_ptr<SensorReadingIf> s =
        sut_->createSensorReading(SensorReadingType::powerState, device0);

    s->setStatus(SensorReadingStatus::invalid);
    EXPECT_FALSE(sut_->isPowerStateOn());
}

TEST_F(SensorReadingsManagerTest,
       isPowerStateOnReturnsFalseWhenPowerStateInvalidButS0)
{
    std::shared_ptr<SensorReadingIf> s =
        sut_->createSensorReading(SensorReadingType::powerState, device0);

    s->setStatus(SensorReadingStatus::invalid);
    s->updateValue(PowerStateType::s0);
    EXPECT_FALSE(sut_->isPowerStateOn());
}

TEST_F(SensorReadingsManagerTest,
       isPowerStateOnReturnsFalseWhenPowerStateValidButS1)
{
    std::shared_ptr<SensorReadingIf> s =
        sut_->createSensorReading(SensorReadingType::powerState, device0);

    s->setStatus(SensorReadingStatus::valid);
    s->updateValue(PowerStateType::s1);
    EXPECT_FALSE(sut_->isPowerStateOn());
}

TEST_F(SensorReadingsManagerTest,
       isPowerStateOnReturnsTrueWhenPowerStateValidAndS0)
{
    std::shared_ptr<SensorReadingIf> s =
        sut_->createSensorReading(SensorReadingType::powerState, device0);

    s->setStatus(SensorReadingStatus::valid);
    s->updateValue(PowerStateType::s0);
    EXPECT_TRUE(sut_->isPowerStateOn());
}

TEST_F(SensorReadingsManagerTest,
       isPowerStateOnReturnsFalseWhenPowerStateValidAndNotS0)
{
    std::shared_ptr<SensorReadingIf> s =
        sut_->createSensorReading(SensorReadingType::powerState, device0);

    s->setStatus(SensorReadingStatus::valid);
    s->updateValue(PowerStateType::unknown);
    EXPECT_FALSE(sut_->isPowerStateOn());
}

TEST_F(SensorReadingsManagerTest,
       isPowerStateOnThrowsWhenPowerStateHasUnexpectedType)
{
    std::shared_ptr<SensorReadingIf> s =
        sut_->createSensorReading(SensorReadingType::powerState, device0);

    s->setStatus(SensorReadingStatus::valid);
    s->updateValue(1.23);
    EXPECT_THROW(sut_->isPowerStateOn(), std::logic_error);
}

TEST_F(SensorReadingsManagerTest,
       isCpuAvailableReturnsFalseWhenCPUSensorIsNotPresent)
{
    EXPECT_FALSE(sut_->isCpuAvailable(device0));
}

TEST_F(SensorReadingsManagerTest,
       isCpuAvailableReturnsFalseWhenCPUSensorNotAvailable)
{
    std::shared_ptr<SensorReadingIf> s =
        sut_->createSensorReading(SensorReadingType::cpuPackagePower, device0);

    s->setStatus(SensorReadingStatus::unavailable);
    EXPECT_FALSE(sut_->isCpuAvailable(device0));
}

TEST_F(SensorReadingsManagerTest, isCpuAvailableReturnsTrueWhenCPUSensorInvalid)
{
    std::shared_ptr<SensorReadingIf> s =
        sut_->createSensorReading(SensorReadingType::cpuPackagePower, device0);

    s->setStatus(SensorReadingStatus::invalid);
    EXPECT_TRUE(sut_->isCpuAvailable(device0));
}

TEST_F(SensorReadingsManagerTest, isCpuAvailableReturnsTrueWhenCPUSensorValid)
{
    std::shared_ptr<SensorReadingIf> s =
        sut_->createSensorReading(SensorReadingType::cpuPackagePower, device0);

    s->setStatus(SensorReadingStatus::valid);
    EXPECT_TRUE(sut_->isCpuAvailable(device0));
}
class SensorReadingsManagerTestCreateSensorReading
    : public SensorReadingsManagerFixture,
      public ::testing::Test
{
};

TEST_F(SensorReadingsManagerTestCreateSensorReading, LowestDeviceIndex)
{
    EXPECT_TRUE(sut_->createSensorReading(SensorReadingType::acPlatformPower,
                                          0) != nullptr);
}

TEST_F(SensorReadingsManagerTestCreateSensorReading, HighestDeviceIndex)
{
    EXPECT_TRUE(sut_->createSensorReading(SensorReadingType::acPlatformPower,
                                          kAllDevices - 1) != nullptr);
}

TEST_F(SensorReadingsManagerTestCreateSensorReading, AllDevicesIndex)
{
    ASSERT_ANY_THROW(sut_->createSensorReading(
        SensorReadingType::acPlatformPower, kAllDevices));
}

TEST_F(SensorReadingsManagerTestCreateSensorReading, ExisitingDeviceIndex)
{
    std::shared_ptr<SensorReadingIf> sensorReading = nullptr;
    sut_->createSensorReading(SensorReadingType::acPlatformPower, 0);
    ASSERT_ANY_THROW(
        sut_->createSensorReading(SensorReadingType::acPlatformPower, 0));
}

TEST_F(SensorReadingsManagerTestCreateSensorReading,
       ExisitingDeviceIndexDifferentType)
{
    std::shared_ptr<SensorReadingIf> sensorReading = nullptr;
    sut_->createSensorReading(SensorReadingType::acPlatformPower, 0);
    EXPECT_TRUE(sut_->createSensorReading(SensorReadingType::dcPlatformPowerCpu,
                                          0) != nullptr);
}

class SensorReadingsManagerTestFindSingleReading
    : public SensorReadingsManagerFixture,
      public ::testing::Test
{
};

TEST_F(SensorReadingsManagerTestFindSingleReading,
       GetSensorReadingReturnsProperObjectWhenExists)
{
    std::shared_ptr<SensorReadingIf> s1 =
        sut_->createSensorReading(SensorReadingType::acPlatformPower, 1);

    std::shared_ptr<SensorReadingIf> s2 =
        sut_->getSensorReading(SensorReadingType::acPlatformPower, 1);
    EXPECT_EQ(s1.get(), s2.get());
}

TEST_F(SensorReadingsManagerTestFindSingleReading,
       GetSensorReadingReturnsNullWhenSensorDoesNotExists)
{
    std::shared_ptr<SensorReadingIf> s1 =
        sut_->createSensorReading(SensorReadingType::dcPlatformPowerCpu, 0);

    std::shared_ptr<SensorReadingIf> s2 =
        sut_->getSensorReading(SensorReadingType::acPlatformPower, 1);
    EXPECT_TRUE(s2 == nullptr);
}

TEST_F(SensorReadingsManagerTestFindSingleReading,
       ExpectProperObjectPassedToCallback)
{
    sut_->createSensorReading(SensorReadingType::acPlatformPower, 0);
    std::shared_ptr<SensorReadingIf> sensorReading =
        sut_->createSensorReading(SensorReadingType::acPlatformPower, 1);

    sut_->forEachSensorReading(SensorReadingType::acPlatformPower, 1,
                               [sensorReading](auto& sensorReadingFound) {
                                   EXPECT_EQ(&sensorReadingFound,
                                             sensorReading.get());
                               });
}

TEST_F(SensorReadingsManagerTestFindSingleReading, ExpectTrueWhenReadingFound)
{
    sut_->createSensorReading(SensorReadingType::acPlatformPower, 0);
    EXPECT_EQ(sut_->forEachSensorReading(SensorReadingType::acPlatformPower, 0,
                                         callback_.AsStdFunction()),
              true);
}

TEST_F(SensorReadingsManagerTestFindSingleReading,
       ExpectFalseWhenReadingNotFound)
{
    sut_->createSensorReading(SensorReadingType::acPlatformPower, 0);
    EXPECT_EQ(sut_->forEachSensorReading(SensorReadingType::acPlatformPower, 1,
                                         callback_.AsStdFunction()),
              false);
}

TEST_F(SensorReadingsManagerTestFindSingleReading, ExpectCallbackExecutedOnce)
{
    EXPECT_CALL(callback_, Call(testing::_)).Times(1);
    sut_->createSensorReading(SensorReadingType::acPlatformPower, 0);
    sut_->createSensorReading(SensorReadingType::acPlatformPower, 1);
    sut_->forEachSensorReading(SensorReadingType::acPlatformPower, 1,
                               callback_.AsStdFunction());
}

class SensorReadingsManagerTestFindAllReadings
    : public SensorReadingsManagerFixture,
      public ::testing::Test
{
};

TEST_F(SensorReadingsManagerTestFindAllReadings,
       ExpectProperObjectsPassedToCallback)
{
    std::vector<std::shared_ptr<SensorReadingIf>> sensorReadings = {
        sut_->createSensorReading(SensorReadingType::acPlatformPower, 0),
        sut_->createSensorReading(SensorReadingType::acPlatformPower, 1)};

    sut_->createSensorReading(SensorReadingType::dcPlatformPowerCpu, 0);

    sut_->forEachSensorReading(
        SensorReadingType::acPlatformPower, kAllDevices,
        [&](auto& sensorReadingFound) {
            EXPECT_EQ(
                sensorReadings.at(sensorReadingFound.getDeviceIndex()).get(),
                &sensorReadingFound);
        });
}

TEST_F(SensorReadingsManagerTestFindAllReadings,
       ExpectCallbackCalledOncePerEachObject)
{
    std::vector<std::shared_ptr<SensorReadingIf>> sensorReadings = {
        sut_->createSensorReading(SensorReadingType::acPlatformPower, 0),
        sut_->createSensorReading(SensorReadingType::acPlatformPower, 1)};

    sut_->createSensorReading(SensorReadingType::dcPlatformPowerCpu, 0);

    EXPECT_CALL(callback_, Call(testing::_)).Times(sensorReadings.size());
    sut_->forEachSensorReading(SensorReadingType::acPlatformPower, kAllDevices,
                               callback_.AsStdFunction());
}

TEST_F(SensorReadingsManagerTestFindAllReadings, ExpectTrueWhenReadingsFound)
{
    std::vector<std::shared_ptr<SensorReadingIf>> sensorReadings = {
        sut_->createSensorReading(SensorReadingType::acPlatformPower, 0),
        sut_->createSensorReading(SensorReadingType::acPlatformPower, 1)};

    sut_->createSensorReading(SensorReadingType::dcPlatformPowerCpu, 0);

    EXPECT_EQ(sut_->forEachSensorReading(SensorReadingType::acPlatformPower,
                                         kAllDevices,
                                         callback_.AsStdFunction()),
              true);
}

TEST_F(SensorReadingsManagerTestFindAllReadings,
       ExpectFalseWhenReadingsNotFound)
{
    std::vector<std::shared_ptr<SensorReadingIf>> sensorReadings = {
        sut_->createSensorReading(SensorReadingType::acPlatformPower, 0),
        sut_->createSensorReading(SensorReadingType::acPlatformPower, 1)};

    sut_->createSensorReading(SensorReadingType::dcPlatformPowerCpu, 0);

    EXPECT_EQ(sut_->forEachSensorReading(SensorReadingType::dcPlatformPowerPsu,
                                         kAllDevices,
                                         callback_.AsStdFunction()),
              false);
}

class SensorReadingsManagerTestCheckIfAvailableAndValid
    : public SensorReadingsManagerFixture,
      public ::testing::Test
{
  public:
    SensorReadingsManagerTestCheckIfAvailableAndValid()
    {
        sensorReading_ =
            sut_->createSensorReading(SensorReadingType::acPlatformPower, 0);
        sut_->createSensorReading(SensorReadingType::acPlatformPower, 1);
        sut_->createSensorReading(SensorReadingType::dcPlatformPowerPsu, 0);
    }

  protected:
    std::shared_ptr<SensorReadingIf> sensorReading_;
};

TEST_F(SensorReadingsManagerTestCheckIfAvailableAndValid,
       ManagerEmptyExpectNullopt)
{
    sut_ = std::make_shared<SensorReadingsManager>();
    EXPECT_EQ(sut_->getAvailableAndValueValidSensorReading(
                  SensorReadingType::acPlatformPower, 0),
              nullptr);
}

TEST_F(SensorReadingsManagerTestCheckIfAvailableAndValid,
       ReadingAvailableAndValidExpectCorrectHandle)
{
    sensorReading_->setStatus(SensorReadingStatus::valid);
    auto readingFound = sut_->getAvailableAndValueValidSensorReading(
        SensorReadingType::acPlatformPower, 0);
    EXPECT_EQ(readingFound.get(), sensorReading_.get());
}

TEST_F(SensorReadingsManagerTestCheckIfAvailableAndValid,
       ReadingNotAvailableExpectNullopt)
{
    sensorReading_->setStatus(SensorReadingStatus::unavailable);
    EXPECT_EQ(sut_->getAvailableAndValueValidSensorReading(
                  SensorReadingType::acPlatformPower, 0),
              nullptr);
}

TEST_F(SensorReadingsManagerTestCheckIfAvailableAndValid,
       ReadingNotValidExpectNullopt)
{
    sensorReading_->setStatus(SensorReadingStatus::invalid);
    EXPECT_EQ(sut_->getAvailableAndValueValidSensorReading(
                  SensorReadingType::acPlatformPower, 0),
              nullptr);
}

TEST_F(SensorReadingsManagerTestCheckIfAvailableAndValid,
       AllDevicesRequestedExpectNullopt)
{
    EXPECT_TRUE(sut_->getAvailableAndValueValidSensorReading(
                    SensorReadingType::acPlatformPower, kAllDevices) ==
                nullptr);
}

class SensorReadingsManagerTestCheckReadingEventDispatching
    : public SensorReadingsManagerFixture,
      public ::testing::Test
{
  public:
    SensorReadingsManagerTestCheckReadingEventDispatching()
    {
        sut_->registerReadingConsumer(readingConsumerMock_0_,
                                      ReadingType::acPlatformPower, 0);
        sut_->registerReadingConsumer(readingConsumerMock_1_,
                                      ReadingType::acPlatformPower, 1);
        sut_->registerReadingConsumer(readingConsumerMock_All_,
                                      ReadingType::acPlatformPower,
                                      kAllDevices);
        acPlatformPowerState_0_ =
            sut_->createSensorReading(SensorReadingType::acPlatformPower, 0);
        acPlatformPowerState_1_ =
            sut_->createSensorReading(SensorReadingType::acPlatformPower, 1);
        sensorReading_ =
            sut_->createSensorReading(SensorReadingType::cpuEfficiency, 0);
    }

  protected:
    std::shared_ptr<SensorReadingIf> sensorReading_;
    std::shared_ptr<SensorReadingIf> acPlatformPowerState_0_;
    std::shared_ptr<SensorReadingIf> acPlatformPowerState_1_;
    std::shared_ptr<ReadingConsumerMock> readingConsumerMock_0_ =
        std::make_shared<testing::NiceMock<ReadingConsumerMock>>();
    std::shared_ptr<ReadingConsumerMock> readingConsumerMock_1_ =
        std::make_shared<testing::NiceMock<ReadingConsumerMock>>();
    std::shared_ptr<ReadingConsumerMock> readingConsumerMock_All_ =
        std::make_shared<testing::NiceMock<ReadingConsumerMock>>();
};

TEST_F(SensorReadingsManagerTestCheckReadingEventDispatching,
       NoEventWhenNotRegisteredType)
{
    EXPECT_CALL(*readingConsumerMock_0_,
                reportEvent(testing::_, testing::_, testing::_))
        .Times(0);
    EXPECT_CALL(*readingConsumerMock_1_,
                reportEvent(testing::_, testing::_, testing::_))
        .Times(0);
    EXPECT_CALL(*readingConsumerMock_All_,
                reportEvent(testing::_, testing::_, testing::_))
        .Times(0);
    sensorReading_->setStatus(SensorReadingStatus::valid);
}

TEST_F(SensorReadingsManagerTestCheckReadingEventDispatching,
       EventDispatchedWhenSensorReadingAvailable)
{
    acPlatformPowerState_0_->setStatus(SensorReadingStatus::invalid);
    EXPECT_CALL(
        *readingConsumerMock_0_,
        reportEvent(
            SensorEventType::readingAvailable,
            testing::AllOf(testing::Field(&SensorContext::type,
                                          SensorReadingType::acPlatformPower),
                           testing::Field(&SensorContext::deviceIndex, 0)),
            testing::AllOf(testing::Field(&ReadingContext::type,
                                          ReadingType::acPlatformPower),
                           testing::Field(&ReadingContext::deviceIndex, 0))))
        .Times(1);
    EXPECT_CALL(*readingConsumerMock_1_,
                reportEvent(testing::_, testing::_, testing::_))
        .Times(0);
    EXPECT_CALL(
        *readingConsumerMock_All_,
        reportEvent(
            SensorEventType::readingAvailable,
            testing::AllOf(testing::Field(&SensorContext::type,
                                          SensorReadingType::acPlatformPower),
                           testing::Field(&SensorContext::deviceIndex, 0)),
            testing::AllOf(
                testing::Field(&ReadingContext::type,
                               ReadingType::acPlatformPower),
                testing::Field(&ReadingContext::deviceIndex, kAllDevices))))
        .Times(1);
    acPlatformPowerState_0_->setStatus(SensorReadingStatus::valid);
}

TEST_F(SensorReadingsManagerTestCheckReadingEventDispatching,
       EventDispatchedWhenSensorReadingAvailableDevice1)
{
    acPlatformPowerState_1_->setStatus(SensorReadingStatus::valid);
    EXPECT_CALL(*readingConsumerMock_0_,
                reportEvent(testing::_, testing::_, testing::_))
        .Times(0);
    EXPECT_CALL(
        *readingConsumerMock_1_,
        reportEvent(
            SensorEventType::readingMissing,
            testing::AllOf(testing::Field(&SensorContext::type,
                                          SensorReadingType::acPlatformPower),
                           testing::Field(&SensorContext::deviceIndex, 1)),
            testing::AllOf(testing::Field(&ReadingContext::type,
                                          ReadingType::acPlatformPower),
                           testing::Field(&ReadingContext::deviceIndex, 1))))
        .Times(1);

    EXPECT_CALL(
        *readingConsumerMock_All_,
        reportEvent(
            SensorEventType::readingMissing,
            testing::AllOf(testing::Field(&SensorContext::type,
                                          SensorReadingType::acPlatformPower),
                           testing::Field(&SensorContext::deviceIndex, 1)),
            testing::AllOf(
                testing::Field(&ReadingContext::type,
                               ReadingType::acPlatformPower),
                testing::Field(&ReadingContext::deviceIndex, kAllDevices))))
        .Times(1);
    acPlatformPowerState_1_->setStatus(SensorReadingStatus::invalid);
}