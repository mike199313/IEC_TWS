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
#include "common_types.hpp"
#include "knobs/hwmon_knob.hpp"
#include "mocks/async_executor_mock.hpp"
#include "mocks/hwmon_file_provider_mock.hpp"
#include "mocks/sensor_readings_manager_mock.hpp"
#include "stubs/hwmon_file_stub.hpp"
#include "utils/dbus_environment.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-death-test-internal.h>

namespace nodemanager
{

constexpr uint32_t minKnobValueInMilliWatts = 1000;
constexpr uint32_t maxKnobValueInMilliWatts = 5000000;

class HwmonKnobTest : public testing::Test
{
  public:
    virtual ~HwmonKnobTest() = default;

    virtual void SetUp() override
    {
        ON_CALL(
            *asyncExecutorMock_,
            schedule(testing::Pair(KnobType::CpuPackagePower, DeviceIndex{0}),
                     testing::_, testing::_))
            .WillByDefault(
                testing::DoAll(testing::SaveArg<1>(&asyncTask_),
                               testing::SaveArg<2>(&asyncTaskCallback_)));

        path_ = hwmonFileManager_.createCpuFile(kHwmonPeciCpuBaseAddress,
                                                HwmonGroup::cpu,
                                                HwmonFileType::limit, 0, 123);

        ON_CALL(*hwmonFileProvider_,
                getFile(KnobType::CpuPackagePower, DeviceIndex{0}))
            .WillByDefault(testing::Return(path_));

        ON_CALL(*sensorReadingsManager_, isCpuAvailable(testing::_))
            .WillByDefault(testing::Return(true));

        ON_CALL(*sensorReadingsManager_, isPowerStateOn())
            .WillByDefault(testing::Return(true));

        sut_ = std::make_shared<HwmonKnob>(
            KnobType::CpuPackagePower, DeviceIndex{0}, minKnobValueInMilliWatts,
            maxKnobValueInMilliWatts, hwmonFileProvider_, asyncExecutorMock_,
            sensorReadingsManager_);
    }

    virtual void TearDown()
    {
        hwmonFileManager_.removeHwmonDirectories();
    }

    using HwmonAsyncExecutorMock =
        AsyncExecutorMock<std::pair<KnobType, DeviceIndex>,
                          std::pair<std::ios_base::iostate, uint32_t>>;

    std::shared_ptr<HwmonKnob> sut_;
    HwmonFileManager hwmonFileManager_ = HwmonFileManager();
    std::shared_ptr<HwmonFileProviderMock> hwmonFileProvider_ =
        std::make_shared<testing::NiceMock<HwmonFileProviderMock>>();
    std::shared_ptr<HwmonAsyncExecutorMock> asyncExecutorMock_ =
        std::make_shared<testing::NiceMock<HwmonAsyncExecutorMock>>();
    std::filesystem::path path_;
    HwmonAsyncExecutorMock::Task asyncTask_;
    HwmonAsyncExecutorMock::TaskCallback asyncTaskCallback_;
    std::shared_ptr<SensorReadingsManagerMock> sensorReadingsManager_ =
        std::make_shared<testing::NiceMock<SensorReadingsManagerMock>>();
};

TEST_F(HwmonKnobTest, NewValueSavedWithSuccess)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::CpuPackagePower, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(1);
    sut_->setKnob(5);
    sut_->run();
    auto ret = asyncTask_();

    auto& [status, savedValue] = ret;
    EXPECT_EQ(status, std::ios_base::goodbit);
    EXPECT_EQ(savedValue, uint32_t{5000});
    asyncTaskCallback_(ret);

    auto value = hwmonFileManager_.readFile(path_);
    EXPECT_EQ(value, "5000");
}

TEST_F(HwmonKnobTest, FailedAttemptRetriggersAnotherAttempt)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::CpuPackagePower, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(2);
    sut_->setKnob(5);
    sut_->run();
    asyncTask_();
    asyncTaskCallback_({std::ios_base::badbit, 0}); // simulate write failure
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    auto value = hwmonFileManager_.readFile(path_);
    EXPECT_EQ(value, "5000");
}

TEST_F(HwmonKnobTest, AnotherAttemptIsMadeWhenNewValueWasSetBeforeCallback)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::CpuPackagePower, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(3);

    sut_->setKnob(5);
    sut_->run();

    auto ret = asyncTask_();

    sut_->setKnob(15);
    sut_->run();

    asyncTaskCallback_(ret);

    sut_->run();
    asyncTaskCallback_(asyncTask_());

    auto value = hwmonFileManager_.readFile(path_);
    EXPECT_EQ(value, "15000");
}

TEST_F(HwmonKnobTest,
       NoAnotherAttemptIsMadeWhenTheSameValueWasSetBeforeCallback)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::CpuPackagePower, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(2);
    testing::Sequence seq;
    sut_->setKnob(5);
    sut_->run();
    auto ret = asyncTask_();
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_(ret);
    asyncTaskCallback_(asyncTask_());

    auto value = hwmonFileManager_.readFile(path_);
    EXPECT_EQ(value, "5000");
}

TEST_F(HwmonKnobTest, MissingHwmonFileDoesNotTriggerAnyTasks)
{
    ON_CALL(*hwmonFileProvider_,
            getFile(KnobType::CpuPackagePower, DeviceIndex{0}))
        .WillByDefault(testing::Return(""));
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::CpuPackagePower, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(0);
    sut_->setKnob(5);
    sut_->run();
}

TEST_F(HwmonKnobTest, TaskNotScheduledWhenS0ReturnsFalse)
{
    ON_CALL(*sensorReadingsManager_, isPowerStateOn())
        .WillByDefault(testing::Return(false));

    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::CpuPackagePower, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(0);
    sut_->setKnob(5);
    sut_->run();
}

TEST_F(HwmonKnobTest, TaskNotScheduledWhenPeciAndGpuOff)
{
    ON_CALL(*sensorReadingsManager_, isGpuPowerStateOn())
        .WillByDefault(testing::Return(false));

    EXPECT_CALL(*asyncExecutorMock_,
                schedule(testing::Pair(KnobType::PciePower, DeviceIndex{0}),
                         testing::_, testing::_))
        .Times(0);

    sut_ = std::make_shared<HwmonKnob>(
        KnobType::PciePower, DeviceIndex{0}, minKnobValueInMilliWatts,
        maxKnobValueInMilliWatts, hwmonFileProvider_, asyncExecutorMock_,
        sensorReadingsManager_);

    sut_->setKnob(5);
    sut_->run();
}

TEST_F(HwmonKnobTest, ResetKnobSetHwmonValueToZero)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::CpuPackagePower, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(2);

    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_(asyncTask_());

    sut_->resetKnob();
    sut_->run();
    auto ret = asyncTask_();
    auto& [status, savedValue] = ret;
    EXPECT_EQ(status, std::ios_base::goodbit);
    EXPECT_EQ(savedValue, uint32_t{0});
    asyncTaskCallback_(ret);

    auto value = hwmonFileManager_.readFile(path_);
    EXPECT_EQ(value, "0");
}

TEST_F(HwmonKnobTest,
       ScheduleWriteOnlyOnceWhenTheSameKnobValueIsSetMultipleTimes)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::CpuPackagePower, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(1);
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_(asyncTask_());
}

TEST_F(HwmonKnobTest, TriggerScheduleOnlyOnceWhenResettingMultipleTimes)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::CpuPackagePower, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(1);
    sut_->resetKnob();
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    sut_->resetKnob();
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    sut_->resetKnob();
    sut_->run();
    asyncTaskCallback_(asyncTask_());
}

TEST_F(HwmonKnobTest, SettingValueZeroIsClampedToMinKnobValue)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::CpuPackagePower, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(1);
    sut_->setKnob(0);
    sut_->run();
    asyncTaskCallback_(asyncTask_());

    auto value = hwmonFileManager_.readFile(path_);
    EXPECT_EQ(value, std::to_string(minKnobValueInMilliWatts));
}

TEST_F(HwmonKnobTest, LowValuesAreClampedToMinKnobValue)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::CpuPackagePower, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(1);
    sut_->setKnob(0.5);
    sut_->run();
    asyncTaskCallback_(asyncTask_());

    auto value = hwmonFileManager_.readFile(path_);
    EXPECT_EQ(value, std::to_string(minKnobValueInMilliWatts));
}

TEST_F(HwmonKnobTest, HugeValuesAreClampedToMaxKnobValue)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::CpuPackagePower, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(1);
    sut_->setKnob(std::numeric_limits<double>::max());
    sut_->run();
    asyncTaskCallback_(asyncTask_());

    auto value = hwmonFileManager_.readFile(path_);
    EXPECT_EQ(value, std::to_string(maxKnobValueInMilliWatts));
}

TEST_F(HwmonKnobTest, HealthIsOk)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::CpuPackagePower, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(1);
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_({std::ios_base::goodbit, 0.0});
    EXPECT_EQ(sut_->getHealth(), NmHealth::ok);
}

TEST_F(HwmonKnobTest, HealthIsWarning)
{
    EXPECT_CALL(
        *asyncExecutorMock_,
        schedule(testing::Pair(KnobType::CpuPackagePower, DeviceIndex{0}),
                 testing::_, testing::_))
        .Times(1);
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_({std::ios_base::badbit, 0.0});
    EXPECT_EQ(sut_->getHealth(), NmHealth::warning);
}

TEST_F(HwmonKnobTest, InittialyHealthIsOk)
{
    sut_->run();
    EXPECT_EQ(sut_->getHealth(), NmHealth::ok);
}

TEST_F(HwmonKnobTest, InitiallyIsKnobSetReturnsFalse)
{
    EXPECT_FALSE(sut_->isKnobSet());
}

TEST_F(HwmonKnobTest, SetKnobIsKnobSetReturnsTrue)
{
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    EXPECT_TRUE(sut_->isKnobSet());
}

TEST_F(HwmonKnobTest, SetKnobFailedIsKnobSetReturnsFalse)
{
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    sut_->setKnob(6);
    sut_->run();
    asyncTaskCallback_({std::ios_base::badbit, 0.0});
    EXPECT_FALSE(sut_->isKnobSet());
}

TEST_F(HwmonKnobTest, ResetKnobIsKnobSetReturnsFalse)
{
    sut_->setKnob(5);
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    sut_->resetKnob();
    sut_->run();
    asyncTaskCallback_(asyncTask_());
    EXPECT_FALSE(sut_->isKnobSet());
}

} // namespace nodemanager