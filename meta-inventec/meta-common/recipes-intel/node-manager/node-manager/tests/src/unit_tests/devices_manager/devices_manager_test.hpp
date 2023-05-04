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
#include "devices_manager/devices_manager.hpp"
#include "mocks/gpio_provider_mock.hpp"
#include "mocks/hwmon_file_provider_mock.hpp"
#include "mocks/pldm_entity_provider_mock.hpp"
#include "stubs/hwmon_file_stub.hpp"
#include "utils/dbus_environment.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class DevicesManagerTest : public ::testing::Test
{
  public:
    virtual ~DevicesManagerTest()
    {
        DbusEnvironment::synchronizeIoc();
    }
    virtual void SetUp() override
    {
        sut_ = std::make_shared<DevicesManager>(
            DbusEnvironment::getBus(), sensorReadingsManager_,
            hwmonFileProvider_, gpioProvider_, pldmEntityProvider_);
        auto s = sensorReadingsManager_->getSensorReading(
            SensorReadingType::powerState, DeviceIndex{0});
        s->setStatus(SensorReadingStatus::valid);
        s->updateValue(PowerStateType::s0);

        s = sensorReadingsManager_->getSensorReading(
            SensorReadingType::cpuPackagePower, DeviceIndex{0});
        s->setStatus(SensorReadingStatus::valid);
        s = sensorReadingsManager_->getSensorReading(
            SensorReadingType::cpuPackagePower, DeviceIndex{1});
        s->setStatus(SensorReadingStatus::valid);
    }

  protected:
    std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManager_ =
        std::make_shared<SensorReadingsManager>();
    std::shared_ptr<DevicesManager> sut_;
    std::shared_ptr<HwmonFileProviderMock> hwmonFileProvider_ =
        std::make_shared<testing::NiceMock<HwmonFileProviderMock>>();
    std::shared_ptr<GpioProviderMock> gpioProvider_ =
        std::make_shared<testing::NiceMock<GpioProviderMock>>();
    std::shared_ptr<PldmEntityProviderMock> pldmEntityProvider_ =
        std::make_shared<testing::NiceMock<PldmEntityProviderMock>>();
};

TEST_F(DevicesManagerTest, DeviceManagerOnDescructionResetsAllAvailableKnobs)
{
    HwmonFileManager hwmon;
    auto createdFile0 =
        hwmon.createCpuFile(kHwmonPeciCpuBaseAddress, HwmonGroup::platform,
                            HwmonFileType::limit, 0, 666999);
    ON_CALL(*hwmonFileProvider_,
            getFile(KnobType::DcPlatformPower, DeviceIndex{0}))
        .WillByDefault(testing::Return(createdFile0));

    auto createdFile1 =
        hwmon.createCpuFile(kHwmonPeciCpuBaseAddress + 1, HwmonGroup::cpu,
                            HwmonFileType::limit, 0, 9966);
    ON_CALL(*hwmonFileProvider_,
            getFile(KnobType::CpuPackagePower, DeviceIndex{1}))
        .WillByDefault(testing::Return(createdFile1));

    sut_ = nullptr;
    ASSERT_STREQ(hwmon.readFile(createdFile0).c_str(), "0");
    ASSERT_STREQ(hwmon.readFile(createdFile1).c_str(), "0");
}
