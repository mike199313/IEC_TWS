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
#include "devices_manager/hwmon_file_provider.hpp"
#include "stubs/hwmon_file_stub.hpp"
#include "utils/dbus_environment.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

struct HwmonFileProviderWaitForTasks : public HwmonFileProvider
{
    HwmonFileProviderWaitForTasks(
        std::shared_ptr<sdbusplus::asio::connection> busArg,
        std::filesystem::path rootPathArg) :
        HwmonFileProvider(busArg, rootPathArg, false)
    {
    }
    ~HwmonFileProviderWaitForTasks() = default;

    void waitForAllTasks(const std::chrono::milliseconds& timeout)
    {
        auto exit_time = std::chrono::steady_clock::now() + timeout;
        if (future && future->valid() &&
            future->wait_until(exit_time) != std::future_status::ready)
        {
            throw std::logic_error("Timeout was reached.");
        }
    }
};

class HwmonFileProviderTest : public ::testing::Test
{
  public:
    virtual ~HwmonFileProviderTest()
    {
        DbusEnvironment::synchronizeIoc();
    }
    virtual void SetUp() override
    {
        HwmonFileManager hwmon;
        sut_ = std::make_shared<HwmonFileProviderWaitForTasks>(
            DbusEnvironment::getBus(), hwmon.getRootPath());
    }

  protected:
    std::shared_ptr<HwmonFileProviderWaitForTasks> sut_;
};

TEST_F(HwmonFileProviderTest, NoFileDetectedWhenPcieAddressIsBelowExpectedRange)
{
    HwmonFileManager hwmon;
    auto invalidAddress = kHwmonPeciPvcBaseAddress - 1;
    hwmon.createPvcFile(kHwmonSmbusPvcBaseBus, invalidAddress, HwmonGroup::pvc,
                        HwmonFileType::pciePower);

    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    for (DeviceIndex idx = 0; idx < kMaxPcieNumber; idx++)
    {
        ASSERT_TRUE(sut_->getFile(SensorReadingType::pciePower, idx).empty());
    }
}

TEST_F(HwmonFileProviderTest, NoFileDetectedWhenPcieAddressIsAboveExpectedRange)
{
    HwmonFileManager hwmon;
    auto invalidAddress = kHwmonPeciPvcBaseAddress + kMaxPcieNumber;
    hwmon.createPvcFile(kHwmonSmbusPvcBaseBus, invalidAddress, HwmonGroup::pvc,
                        HwmonFileType::pciePower);

    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    for (DeviceIndex idx = 0; idx < kMaxPcieNumber; idx++)
    {
        ASSERT_TRUE(sut_->getFile(SensorReadingType::pciePower, idx).empty());
    }
}

TEST_F(HwmonFileProviderTest, NoFileDetectedWhenCpuAddressIsBelowExpectedRange)
{
    HwmonFileManager hwmon;
    auto invalidAddress = kHwmonPeciCpuBaseAddress - 1;

    hwmon.createCpuFile(invalidAddress, HwmonGroup::cpu, HwmonFileType::limit);

    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    for (DeviceIndex idx = 0; idx < kMaxPcieNumber; idx++)
    {
        ASSERT_TRUE(sut_->getFile(KnobType::CpuPackagePower, idx).empty());
    }
}

TEST_F(HwmonFileProviderTest, NoFileDetectedWhenCpuAddressIsAboveExpectedRange)
{
    HwmonFileManager hwmon;
    auto invalidAddress = kHwmonPeciCpuBaseAddress + kMaxCpuNumber;
    hwmon.createCpuFile(invalidAddress, HwmonGroup::cpu, HwmonFileType::limit);

    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    for (DeviceIndex idx = 0; idx < kMaxPcieNumber; idx++)
    {
        ASSERT_TRUE(sut_->getFile(KnobType::CpuPackagePower, idx).empty());
    }
}

TEST_F(HwmonFileProviderTest, NoPcieFileDetectedWhenFilenameInvalid)
{
    HwmonFileManager hwmon;
    auto invalidAddress = kHwmonPeciPvcBaseAddress;
    hwmon.createPvcFile(kHwmonSmbusPvcBaseBus, invalidAddress, HwmonGroup::pvc,
                        std::nullopt);

    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    ASSERT_TRUE(
        sut_->getFile(SensorReadingType::pciePower, DeviceIndex{0}).empty());
}

TEST_F(HwmonFileProviderTest, PeciPowerFileDetected)
{
    HwmonFileManager hwmon;
    auto createdFile =
        hwmon.createPvcFile(kHwmonSmbusPvcBaseBus, kHwmonPeciPvcBaseAddress,
                            HwmonGroup::pvc, HwmonFileType::pciePower);

    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    ASSERT_STREQ(
        sut_->getFile(SensorReadingType::pciePower, DeviceIndex{0}).c_str(),
        createdFile.c_str());
}

TEST_F(HwmonFileProviderTest, KnobFileDetected)
{
    HwmonFileManager hwmon;
    auto createdFile =
        hwmon.createPvcFile(kHwmonSmbusPvcBaseBus, kHwmonPeciPvcBaseAddress,
                            HwmonGroup::pvc, HwmonFileType::limit);

    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    ASSERT_STREQ(sut_->getFile(KnobType::PciePower, DeviceIndex{0}).c_str(),
                 createdFile.c_str());
}

TEST_F(HwmonFileProviderTest, KnobDramFileExistsThenFileDetected)
{
    HwmonFileManager hwmon;
    auto createdFile = hwmon.createCpuFile(
        kHwmonPeciCpuBaseAddress + 1, HwmonGroup::dimm, HwmonFileType::limit);

    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    ASSERT_STREQ(sut_->getFile(KnobType::DramPower, DeviceIndex{1}).c_str(),
                 createdFile.c_str());
}

TEST_F(HwmonFileProviderTest, KnobCpuFileExistsThenFileDetected)
{
    HwmonFileManager hwmon;
    auto createdFile = hwmon.createCpuFile(
        kHwmonPeciCpuBaseAddress + 2, HwmonGroup::cpu, HwmonFileType::limit);

    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    ASSERT_STREQ(
        sut_->getFile(KnobType::CpuPackagePower, DeviceIndex{2}).c_str(),
        createdFile.c_str());
}

TEST_F(HwmonFileProviderTest, KnobPlatfromFileExistsThenFileDetected)
{
    HwmonFileManager hwmon;
    auto createdFile = hwmon.createCpuFile(
        kHwmonPeciCpuBaseAddress, HwmonGroup::platform, HwmonFileType::limit);

    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    ASSERT_STREQ(
        sut_->getFile(KnobType::DcPlatformPower, DeviceIndex{0}).c_str(),
        createdFile.c_str());
}

TEST_F(HwmonFileProviderTest,
       KnobPlatfromFileExistsButDifferentDeviceIdThenFileNotDetected)
{
    HwmonFileManager hwmon;
    auto createdFile = hwmon.createCpuFile(
        kHwmonPeciCpuBaseAddress + 5, HwmonGroup::cpu, HwmonFileType::limit);

    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    ASSERT_TRUE(
        sut_->getFile(KnobType::CpuPackagePower, DeviceIndex{7}).empty());
}

TEST_F(HwmonFileProviderTest,
       HwomFileDetectedThenRemovedProviderReturnsEmptyPath)
{
    HwmonFileManager hwmon;
    auto createdFile = hwmon.createCpuFile(
        kHwmonPeciCpuBaseAddress, HwmonGroup::platform, HwmonFileType::limit);

    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    ASSERT_STREQ(
        sut_->getFile(KnobType::DcPlatformPower, DeviceIndex{0}).c_str(),
        createdFile.c_str());

    std::filesystem::remove_all(createdFile);
    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    ASSERT_TRUE(
        sut_->getFile(KnobType::DcPlatformPower, DeviceIndex{0}).empty());
}

TEST_F(HwmonFileProviderTest, HwomFileNameChangedThenProviderReturnsNewName)
{
    HwmonFileManager hwmon;
    auto createdFile =
        hwmon.createCpuFile(kHwmonPeciCpuBaseAddress, HwmonGroup::platform,
                            HwmonFileType::limit, 3);

    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    ASSERT_STREQ(
        sut_->getFile(KnobType::DcPlatformPower, DeviceIndex{0}).c_str(),
        createdFile.c_str());

    std::filesystem::remove_all(createdFile);
    createdFile =
        hwmon.createCpuFile(kHwmonPeciCpuBaseAddress, HwmonGroup::platform,
                            HwmonFileType::limit, 7);

    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    ASSERT_STREQ(
        sut_->getFile(KnobType::DcPlatformPower, DeviceIndex{0}).c_str(),
        createdFile.c_str());
}

TEST_F(HwmonFileProviderTest, TwoKnobFilesDetectedThenOneReplaceAndNewOneAdded)
{
    HwmonFileManager hwmon;
    auto createdFile_0 = hwmon.createCpuFile(
        kHwmonPeciCpuBaseAddress + 0, HwmonGroup::cpu, HwmonFileType::limit);
    auto createdFile_1 = hwmon.createCpuFile(
        kHwmonPeciCpuBaseAddress + 1, HwmonGroup::cpu, HwmonFileType::limit);
    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    ASSERT_STREQ(
        sut_->getFile(KnobType::CpuPackagePower, DeviceIndex{0}).c_str(),
        createdFile_0.c_str());
    ASSERT_STREQ(
        sut_->getFile(KnobType::CpuPackagePower, DeviceIndex{1}).c_str(),
        createdFile_1.c_str());

    std::filesystem::remove_all(createdFile_1);

    createdFile_1 = hwmon.createCpuFile(
        kHwmonPeciCpuBaseAddress + 1, HwmonGroup::cpu, HwmonFileType::limit, 9);
    auto createdFile_2 = hwmon.createCpuFile(
        kHwmonPeciCpuBaseAddress + 2, HwmonGroup::cpu, HwmonFileType::limit);

    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();

    ASSERT_STREQ(
        sut_->getFile(KnobType::CpuPackagePower, DeviceIndex{1}).c_str(),
        createdFile_1.c_str());
    ASSERT_STREQ(
        sut_->getFile(KnobType::CpuPackagePower, DeviceIndex{2}).c_str(),
        createdFile_2.c_str());
}

TEST_F(HwmonFileProviderTest, PsuPowerFileDetected)
{
    HwmonFileManager hwmon;
    auto createdFile = hwmon.createPsuFile(7, 0x58, HwmonGroup::psu,
                                           HwmonFileType::psuAcPower);

    sut_->discoverFiles();
    sut_->waitForAllTasks(std::chrono::seconds{5});
    sut_->discoverFiles();
    ASSERT_STREQ(
        sut_->getFile(SensorReadingType::acPlatformPower, DeviceIndex{0})
            .c_str(),
        createdFile.c_str());
}