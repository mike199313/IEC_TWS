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
#include "mocks/hwmon_file_provider_mock.hpp"
#include "mocks/sensor_reading_mock.hpp"
#include "mocks/sensor_readings_manager_mock.hpp"
#include "sensors/hwmon_sensor.hpp"
#include "stubs/hwmon_file_stub.hpp"
#include "utils/dbus_environment.hpp"

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

static constexpr const auto kHwmonPath =
    "/tmp/nm-hwmon-ut/sys/bus/peci/devices/peci-0/0-30/peci-cpupower.0/hwmon/"
    "hwmon1";
static constexpr const DeviceIndex kDeviceIndexNum = 8;
static const std::map<std::string, std::vector<SensorReadingType>>
    kHwmonSensorReadingTypes = {
        {"pvcpower", {SensorReadingType::pciePower}},
        {"cpupower",
         {SensorReadingType::cpuPackagePower,
          SensorReadingType::cpuPackagePowerCapabilitiesMin,
          SensorReadingType::cpuPackagePowerCapabilitiesMax,
          SensorReadingType::cpuPackagePowerLimit,
          SensorReadingType::cpuEnergy}},
        {"dimmpower",
         {SensorReadingType::dramPower,
          SensorReadingType::dramPackagePowerCapabilitiesMax,
          SensorReadingType::dramPowerLimit, SensorReadingType::dramEnergy}},
        {"platformpower",
         {SensorReadingType::dcPlatformPowerCpu,
          SensorReadingType::dcPlatformPowerLimit,
          SensorReadingType::dcPlatformPowerCapabilitiesMaxCpu,
          SensorReadingType::dcPlatformEnergy}},
        {"psu",
         {SensorReadingType::acPlatformPower,
          SensorReadingType::acPlatformPowerCapabilitiesMax,
          SensorReadingType::dcPlatformPowerPsu,
          SensorReadingType::dcPlatformPowerCapabilitiesMaxPsu}},
};

struct SensorReadingParams
{
    SensorReadingType type;
};

struct HwmonSenorWaitForTasks : public HwmonSensor
{
    HwmonSenorWaitForTasks(
        std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManagerArg,
        std::shared_ptr<HwmonFileProviderIf> hwmonProviderArg) :
        HwmonSensor(sensorReadingsManagerArg, hwmonProviderArg)
    {
    }
    ~HwmonSenorWaitForTasks() = default;

    void waitForAllTasks(const std::chrono::milliseconds& timeout)
    {
        auto exit_time = std::chrono::steady_clock::now() + timeout;
        for (const auto& [key, future] : futures)
        {
            if (future.valid() &&
                future.wait_until(exit_time) != std::future_status::ready)
            {
                throw std::logic_error("Timeout was reached.");
            }
        }
    }
};

class HwmonSensorTest : public ::testing::TestWithParam<SensorReadingType>
{
  protected:
    HwmonSensorTest()
    {
        for (auto& [group, readingTypes] : kHwmonSensorReadingTypes)
        {
            for (auto type : readingTypes)
            {
                for (DeviceIndex index = 0; index < kDeviceIndexNum; index++)
                {
                    auto sensor = std::make_shared<
                        testing::NiceMock<SensorReadingMock>>();
                    sensorReadings_.insert({{type, index}, sensor});
                    ON_CALL(*sensor, getSensorReadingType())
                        .WillByDefault(testing::Return(type));
                    ON_CALL(*sensor, getDeviceIndex())
                        .WillByDefault(testing::Return(index));
                    ON_CALL(*sensorReadingsManager_,
                            createSensorReading(testing::Eq(type),
                                                testing::Eq(index)))
                        .WillByDefault(testing::Return(sensor));
                }
            }
        }
        ON_CALL(*sensorReadingsManager_, isPowerStateOn())
            .WillByDefault(testing::Return(true));

        ON_CALL(*sensorReadingsManager_, isCpuAvailable(testing::_))
            .WillByDefault(testing::Return(true));

        group = sensorReadingTypeToGroup(param);
        filetype = sensorReadingTypeToFileType(param);
        sut_ = std::make_shared<HwmonSenorWaitForTasks>(sensorReadingsManager_,
                                                        hwmonFileProvider_);
    }

    HwmonGroup sensorReadingTypeToGroup(SensorReadingType type)
    {
        switch (type)
        {
            case SensorReadingType::cpuPackagePower:
            case SensorReadingType::cpuPackagePowerCapabilitiesMin:
            case SensorReadingType::cpuPackagePowerCapabilitiesMax:
            case SensorReadingType::cpuPackagePowerLimit:
            case SensorReadingType::cpuEnergy:
                return HwmonGroup::cpu;
            case SensorReadingType::pciePower:
                return HwmonGroup::pvc;
            case SensorReadingType::dramPower:
            case SensorReadingType::dramPackagePowerCapabilitiesMax:
            case SensorReadingType::dramPowerLimit:
            case SensorReadingType::dramEnergy:
                return HwmonGroup::dimm;
            case SensorReadingType::dcPlatformPowerCpu:
            case SensorReadingType::dcPlatformPowerLimit:
            case SensorReadingType::dcPlatformPowerCapabilitiesMaxCpu:
            case SensorReadingType::dcPlatformEnergy:
                return HwmonGroup::platform;
            case SensorReadingType::acPlatformPower:
            case SensorReadingType::acPlatformPowerCapabilitiesMax:
            case SensorReadingType::dcPlatformPowerPsu:
            case SensorReadingType::dcPlatformPowerCapabilitiesMaxPsu:
                return HwmonGroup::psu;
            default:
                throw std::logic_error(
                    "Add new reading type as a supported hwmon sensor");
        }
    }

    uint8_t hwmonGroupToBaseAddress(HwmonGroup groupname)
    {
        switch (groupname)
        {
            case HwmonGroup::cpu:
            case HwmonGroup::dimm:
            case HwmonGroup::platform:
                return kHwmonPeciCpuBaseAddress;
            case HwmonGroup::pvc:
                return kHwmonPeciPvcBaseAddress;
            case HwmonGroup::psu:
                return kHwmonI2cPsuBaseAddress;
            default:
                throw std::logic_error("Add new group name to be supported");
        }
    }

    HwmonFileType sensorReadingTypeToFileType(SensorReadingType type)
    {
        switch (type)
        {
            case SensorReadingType::cpuPackagePower:
            case SensorReadingType::dramPower:
            case SensorReadingType::dcPlatformPowerCpu:
                return HwmonFileType::current;
            case SensorReadingType::cpuPackagePowerCapabilitiesMin:
                return HwmonFileType::min;
            case SensorReadingType::cpuPackagePowerCapabilitiesMax:
            case SensorReadingType::dramPackagePowerCapabilitiesMax:
            case SensorReadingType::dcPlatformPowerCapabilitiesMaxCpu:
                return HwmonFileType::max;
            case SensorReadingType::cpuPackagePowerLimit:
            case SensorReadingType::dramPowerLimit:
            case SensorReadingType::dcPlatformPowerLimit:
                return HwmonFileType::limit;
            case SensorReadingType::cpuEnergy:
            case SensorReadingType::dramEnergy:
            case SensorReadingType::dcPlatformEnergy:
                return HwmonFileType::energy;
            case SensorReadingType::acPlatformPower:
                return HwmonFileType::psuAcPower;
            case SensorReadingType::pciePower:
                return HwmonFileType::pciePower;
            case SensorReadingType::acPlatformPowerCapabilitiesMax:
                return HwmonFileType::psuAcPowerMax;
            case SensorReadingType::dcPlatformPowerPsu:
                return HwmonFileType::psuDcPower;
            case SensorReadingType::dcPlatformPowerCapabilitiesMaxPsu:
                return HwmonFileType::psuDcPowerMax;

            default:
                throw std::logic_error(
                    "Add new reading type as a supported hwmon sensor");
        }
    }

    void TearDown()
    {
        sut_->waitForAllTasks(std::chrono::seconds{5});
    }

    std::vector<SensorReadingType> types{SensorReadingType::cpuPackagePower};
    std::shared_ptr<SensorReadingsManagerMock> sensorReadingsManager_ =
        std::make_shared<testing::NiceMock<SensorReadingsManagerMock>>();

    HwmonFileManager hwmonFileManager = HwmonFileManager();
    std::shared_ptr<HwmonFileProviderMock> hwmonFileProvider_ =
        std::make_shared<testing::NiceMock<HwmonFileProviderMock>>();
    std::shared_ptr<HwmonSenorWaitForTasks> sut_;
    std::unordered_map<std::pair<SensorReadingType, DeviceIndex>,
                       std::shared_ptr<SensorReadingMock>,
                       boost::hash<std::pair<SensorReadingType, DeviceIndex>>>
        sensorReadings_;
    std::unordered_map<std::pair<HwmonGroup, DeviceIndex>, std::string,
                       boost::hash<std::pair<HwmonGroup, DeviceIndex>>>
        hwmonPaths_;
    SensorReadingType param = GetParam();
    std::map<std::pair<DeviceIndex, SensorReadingType>, testing::Sequence> seq;
    HwmonGroup group;
    HwmonFileType filetype;
};

INSTANTIATE_TEST_SUITE_P(
    SensorList, HwmonSensorTest,
    ::testing::Values(SensorReadingType::cpuPackagePower,
                      SensorReadingType::cpuPackagePowerCapabilitiesMin,
                      SensorReadingType::cpuPackagePowerCapabilitiesMax,
                      SensorReadingType::cpuPackagePowerLimit,
                      SensorReadingType::cpuEnergy,
                      SensorReadingType::dramPower,
                      SensorReadingType::dramPackagePowerCapabilitiesMax,
                      SensorReadingType::dramPowerLimit,
                      SensorReadingType::dramEnergy,
                      SensorReadingType::dcPlatformPowerCpu,
                      SensorReadingType::dcPlatformPowerLimit,
                      SensorReadingType::dcPlatformEnergy));

TEST_P(HwmonSensorTest, HwmonFilesPresentRunExpectCorrectConsecutiveReadings)
{
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                setStatus(testing::Eq(SensorReadingStatus::valid)))
        .Times(2);
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                updateValue(testing::VariantWith<double>(0.123)));
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                updateValue(testing::VariantWith<double>(0.456)));

    auto path_ = hwmonFileManager.createCpuFile(hwmonGroupToBaseAddress(group),
                                                group, filetype, 0, 123);
    ON_CALL(*hwmonFileProvider_, getFile(param, DeviceIndex{0}))
        .WillByDefault(testing::Return(path_));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    hwmonFileManager.createCpuFile(hwmonGroupToBaseAddress(group), group,
                                   filetype, 0, 456);

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_P(HwmonSensorTest, WhenPowerStateOffSensorIsUnavailable)
{
    ON_CALL(*sensorReadingsManager_, isPowerStateOn())
        .WillByDefault(testing::Return(false));
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                setStatus(testing::Eq(SensorReadingStatus::unavailable)))
        .Times(2);
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                updateValue(testing::VariantWith<double>(0.123)))
        .Times(0);

    auto path_ = hwmonFileManager.createCpuFile(hwmonGroupToBaseAddress(group),
                                                group, filetype, 0, 123);
    ON_CALL(*hwmonFileProvider_, getFile(param, DeviceIndex{0}))
        .WillByDefault(testing::Return(path_));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_P(HwmonSensorTest,
       MissingHwmonDirectoriesRunExpectReadingsUnavailableAndValueInvalid)
{
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                setStatus(testing::Eq(SensorReadingStatus::unavailable)))
        .Times(2);

    ON_CALL(*hwmonFileProvider_, getFile(param, DeviceIndex{0}))
        .WillByDefault(testing::Return(""));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_P(HwmonSensorTest,
       HwmonDirectoryDissapearedExpectReadingsUnavailableAndValueInvalid)
{
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                setStatus(testing::Eq(SensorReadingStatus::valid)));
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                updateValue(testing::VariantWith<double>(0.789)));
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                setStatus(testing::Eq(SensorReadingStatus::unavailable)));

    auto path_ = hwmonFileManager.createCpuFile(hwmonGroupToBaseAddress(group),
                                                group, filetype, 0, 789);

    ON_CALL(*hwmonFileProvider_, getFile(param, DeviceIndex{0}))
        .WillByDefault(testing::Return(path_));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    hwmonFileManager.removeHwmonDirectories();
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    for (int i = 0; i < kHwmonReadRetriesCount; i++)
    {
        sut_->run();
        sut_->waitForAllTasks(std::chrono::seconds{5});
    }
    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

class HwmonSensorTestWithoutPlatformGroup : public HwmonSensorTest
{
  public:
    HwmonSensorTestWithoutPlatformGroup() : HwmonSensorTest()
    {
    }
};

INSTANTIATE_TEST_SUITE_P(
    SensorListWithoutPlatformGroup, HwmonSensorTestWithoutPlatformGroup,
    ::testing::Values(SensorReadingType::cpuPackagePower,
                      SensorReadingType::cpuPackagePowerCapabilitiesMin,
                      SensorReadingType::cpuPackagePowerCapabilitiesMax,
                      SensorReadingType::cpuPackagePowerLimit,
                      SensorReadingType::cpuEnergy,
                      SensorReadingType::dramPower,
                      SensorReadingType::dramPowerLimit,
                      SensorReadingType::dramEnergy));

TEST_P(HwmonSensorTestWithoutPlatformGroup,
       HwmonFilesForTwoDifferentCpuSlotsExpectCorrectReadingsPerDeviceIndex)
{
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                setStatus(testing::Eq(SensorReadingStatus::valid)));
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                updateValue(testing::VariantWith<double>(0.987)));

    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(1)}),
                setStatus(testing::Eq(SensorReadingStatus::valid)));
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(1)}),
                updateValue(testing::VariantWith<double>(0.654)));

    auto path0_ = hwmonFileManager.createCpuFile(
        hwmonGroupToBaseAddress(group) + 0, group, filetype, 0, 987);
    auto path1_ = hwmonFileManager.createCpuFile(
        hwmonGroupToBaseAddress(group) + 1, group, filetype, 0, 654);
    ON_CALL(*hwmonFileProvider_, getFile(param, DeviceIndex{0}))
        .WillByDefault(testing::Return(path0_));

    ON_CALL(*hwmonFileProvider_, getFile(param, DeviceIndex{1}))
        .WillByDefault(testing::Return(path1_));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_P(HwmonSensorTestWithoutPlatformGroup,
       WhenPowerStateOffSensorIsUnavailable)
{
    ON_CALL(*sensorReadingsManager_, isPowerStateOn())
        .WillByDefault(testing::Return(false));
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                setStatus(testing::Eq(SensorReadingStatus::unavailable)))
        .Times(2);
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                updateValue(testing::VariantWith<double>(0.987)))
        .Times(0);

    auto path0_ = hwmonFileManager.createCpuFile(
        hwmonGroupToBaseAddress(group) + 0, group, filetype, 0, 987);
    ON_CALL(*hwmonFileProvider_, getFile(param, DeviceIndex{0}))
        .WillByDefault(testing::Return(path0_));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_P(HwmonSensorTestWithoutPlatformGroup,
       CpuPackagePowerIsNotBlockedByIsCpuAvailable)
{
    if (param == SensorReadingType::cpuPackagePower)
    {
        ON_CALL(*sensorReadingsManager_, isCpuAvailable(testing::_))
            .WillByDefault(testing::Return(false));
    }
    else
    {
        ON_CALL(*sensorReadingsManager_, isCpuAvailable(testing::_))
            .WillByDefault(testing::Return(true));
    }

    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                setStatus(testing::Eq(SensorReadingStatus::valid)))
        .Times(1);
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                updateValue(testing::VariantWith<double>(0.987)))
        .Times(1);

    auto path0_ = hwmonFileManager.createCpuFile(
        hwmonGroupToBaseAddress(group) + 0, group, filetype, 0, 987);
    ON_CALL(*hwmonFileProvider_, getFile(param, DeviceIndex{0}))
        .WillByDefault(testing::Return(path0_));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

class HwmonSensorTestPsuGroup : public HwmonSensorTest
{
  public:
    HwmonSensorTestPsuGroup() : HwmonSensorTest()
    {
    }
};

INSTANTIATE_TEST_SUITE_P(
    SensorListWithoutPlatformGroup, HwmonSensorTestPsuGroup,
    ::testing::Values(SensorReadingType::acPlatformPower,
                      SensorReadingType::acPlatformPowerCapabilitiesMax,
                      SensorReadingType::dcPlatformPowerPsu,
                      SensorReadingType::dcPlatformPowerCapabilitiesMaxPsu));
TEST_P(HwmonSensorTestPsuGroup,
       HwmonFilesForTwoDifferentCpuSlotsExpectCorrectReadingsPerDeviceIndex)
{
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                setStatus(testing::Eq(SensorReadingStatus::valid)));
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                updateValue(testing::VariantWith<double>(0.987)));

    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(1)}),
                setStatus(testing::Eq(SensorReadingStatus::valid)));
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(1)}),
                updateValue(testing::VariantWith<double>(0.654)));

    auto path0_ =
        hwmonFileManager.createPsuFile(7, 0x58, group, filetype, "987000");
    auto path1_ =
        hwmonFileManager.createPsuFile(7, 0x59, group, filetype, "654000");

    ON_CALL(*hwmonFileProvider_, getFile(param, DeviceIndex{0}))
        .WillByDefault(testing::Return(path0_));

    ON_CALL(*hwmonFileProvider_, getFile(param, DeviceIndex{1}))
        .WillByDefault(testing::Return(path1_));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_P(HwmonSensorTestPsuGroup, PsuSensorsAreAvailableWhenPowerStateIsOff)
{
    ON_CALL(*sensorReadingsManager_, isPowerStateOn())
        .WillByDefault(testing::Return(false));

    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                setStatus(testing::Eq(SensorReadingStatus::valid)));
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                updateValue(testing::VariantWith<double>(0.987)));

    auto path0_ =
        hwmonFileManager.createPsuFile(7, 0x58, group, filetype, "987000");

    ON_CALL(*hwmonFileProvider_, getFile(param, DeviceIndex{0}))
        .WillByDefault(testing::Return(path0_));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}

TEST_P(HwmonSensorTestPsuGroup, PsuSensorsAreAvailableWhenCpuIsUnavailable)
{
    ON_CALL(*sensorReadingsManager_, isCpuAvailable(testing::_))
        .WillByDefault(testing::Return(false));

    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                setStatus(testing::Eq(SensorReadingStatus::valid)));
    EXPECT_CALL(*sensorReadings_.at({param, DeviceIndex(0)}),
                updateValue(testing::VariantWith<double>(0.987)));

    auto path0_ =
        hwmonFileManager.createPsuFile(7, 0x58, group, filetype, "987000");

    ON_CALL(*hwmonFileProvider_, getFile(param, DeviceIndex{0}))
        .WillByDefault(testing::Return(path0_));

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});

    sut_->run();
    sut_->waitForAllTasks(std::chrono::seconds{5});
}