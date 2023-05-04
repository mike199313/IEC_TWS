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
#include "devices_manager/hwmon_file_provider.hpp"
#include "mocks/gpio_provider_mock.hpp"
#include "mocks/policy_storage_management_mock.hpp"
#include "node_manager.hpp"
#include "stubs/hwmon_file_stub.hpp"
#include "utils/busctl_call.hpp"
#include "utils/dbus_environment.hpp"
#include "utils/dbus_paths.hpp"
#include "utils/policy_config.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
using namespace nodemanager;

class NodeManagerEndToEndMultiComponentTest
    : public ::testing::TestWithParam<DomainId>
{
  public:
    NodeManagerEndToEndMultiComponentTest()
    {
        std::cout << "[   INFO   ] This End-to-end test for power limiting. "
                  << "It may take few seconds..." << std::endl;
        fakeHwmonPath_ = hwmonManager_.getRootPath().c_str();
        hwmonCpuCurrent_ = hwmonManager_.createCpuFile(
            kHwmonPeciCpuBaseAddress, HwmonGroup::cpu, HwmonFileType::current,
            0, 0);
        hwmonCpuLimit_ = hwmonManager_.createCpuFile(
            kHwmonPeciCpuBaseAddress, HwmonGroup::cpu, HwmonFileType::limit, 0,
            0);
        hwmonManager_.createCpuFile(kHwmonPeciCpuBaseAddress, HwmonGroup::cpu,
                                    HwmonFileType::min, 0, 0);
        hwmonManager_.createCpuFile(kHwmonPeciCpuBaseAddress, HwmonGroup::cpu,
                                    HwmonFileType::max, 0, 1000000);

        hwmonMemoryCurrent_ = hwmonManager_.createCpuFile(
            kHwmonPeciCpuBaseAddress, HwmonGroup::dimm, HwmonFileType::current,
            0, 0);
        hwmonMemoryLimit_ = hwmonManager_.createCpuFile(
            kHwmonPeciCpuBaseAddress, HwmonGroup::dimm, HwmonFileType::limit, 0,
            0);
        hwmonManager_.createCpuFile(kHwmonPeciCpuBaseAddress, HwmonGroup::dimm,
                                    HwmonFileType::min, 0, 0);
        hwmonManager_.createCpuFile(kHwmonPeciCpuBaseAddress, HwmonGroup::dimm,
                                    HwmonFileType::max, 0, 1000000);

        hwmonPcieCurrent_ = hwmonManager_.createPvcFile(
            kHwmonSmbusPvcBaseBus, kHwmonPeciPvcBaseAddress, HwmonGroup::pvc,
            HwmonFileType::pciePower, 0, 1000000);
        hwmonPcieLimit_ = hwmonManager_.createPvcFile(
            kHwmonSmbusPvcBaseBus, kHwmonPeciPvcBaseAddress, HwmonGroup::pvc,
            HwmonFileType::limit, 0, 0);

        hwmonDcCurrent_ = hwmonManager_.createCpuFile(
            kHwmonPeciCpuBaseAddress, HwmonGroup::platform,
            HwmonFileType::current, 0, 0);
        hwmonDcLimit_ = hwmonManager_.createCpuFile(kHwmonPeciCpuBaseAddress,
                                                    HwmonGroup::platform,
                                                    HwmonFileType::limit, 0, 0);
        hwmonManager_.createCpuFile(kHwmonPeciCpuBaseAddress,
                                    HwmonGroup::platform, HwmonFileType::min, 0,
                                    0);
        hwmonManager_.createCpuFile(kHwmonPeciCpuBaseAddress,
                                    HwmonGroup::platform, HwmonFileType::max, 0,
                                    1000000);

        sut_ = std::make_unique<FakeNodeManager>(
            DbusEnvironment::getIoc(), DbusEnvironment::getBus(),
            std::string(nodemanager::kRootObjectPath), fakeHwmonPath_);
    }
    virtual ~NodeManagerEndToEndMultiComponentTest()
    {
        DbusEnvironment::synchronizeIoc();
    }
    void SetUp() override
    {
        // TODO: Remove below Capabilities manual setup when reading these from
        //       hwmon files will be supported
        BusctlCall::SetProperty(DbusPaths::domain(DomainId::DcTotalPower),
                                kDomainCapabilitiesInterface, "Min", 10.0);
        BusctlCall::SetProperty(DbusPaths::domain(DomainId::DcTotalPower),
                                kDomainCapabilitiesInterface, "Max", 123.0);
    }
    void TearDown() override
    {
        std::filesystem::remove_all(std::filesystem::temp_directory_path() /
                                    "nm-hwmon-ut");
    }
    /**
     * The purpose of creating fake NodeManager is that the original object
     * calls the run() method with some fixed period relying on async
     * timeouts. In the test we would like to controll the run() calls
     * without modifying the original class e.g. by passing the timeout window
     * value to constructor.
     */
    class FakeNodeManager : public RunnerIf, DbusEnableIf
    {
      public:
        FakeNodeManager() = delete;
        FakeNodeManager(boost::asio::io_context& iocArg,
                        std::shared_ptr<sdbusplus::asio::connection> busArg,
                        std::string const& objectPathArg,
                        std::string fakeHwmonParhArg) :
            ioc_(iocArg),
            bus_(busArg),
            objectServer_(
                std::make_shared<sdbusplus::asio::object_server>(bus_)),
            objectPath_(objectPathArg)
        {
            sensorReadingsManager_ = std::make_shared<SensorReadingsManager>();
            gpioProvider_ =
                std::make_shared<testing::NiceMock<GpioProviderMock>>();
            devicesManager_ = std::make_shared<DevicesManager>(
                bus_, sensorReadingsManager_,
                std::make_shared<HwmonFileProvider>(bus_,
                                                    fakeHwmonParhArg.c_str()),
                gpioProvider_, std::make_shared<PldmEntityProvider>(bus_));
            efficiencyControl_ =
                std::make_shared<EfficiencyControl>(devicesManager_);
            budgeting_ = makeBudgeting(devicesManager_);
            triggersManager_ = std::make_shared<TriggersManager>(
                objectServer_, objectPath_, gpioProvider_);
            ptam_ = std::make_unique<Ptam>(
                bus_, objectServer_, objectPath_, devicesManager_,
                gpioProvider_, budgeting_, efficiencyControl_, triggersManager_,
                policyStorageManagement_);
            statisticsProvider_ =
                std::make_unique<StatisticsProvider>(devicesManager_);
            statisticsProvider_->addStatistics(
                std::make_shared<Statistic>(
                    kStatGlobalInletTemp,
                    std::make_shared<GlobalAccumulator>()),
                ReadingType::inletTemperature);
            statisticsProvider_->addStatistics(
                std::make_shared<Statistic>(
                    kStatGlobalOutletTemp,
                    std::make_shared<GlobalAccumulator>()),
                ReadingType::outletTemperature);
            statisticsProvider_->addStatistics(
                std::make_shared<Statistic>(
                    kStatGlobalVolumetricAirflow,
                    std::make_shared<GlobalAccumulator>()),
                ReadingType::volumetricAirflow);
            statisticsProvider_->addStatistics(
                std::make_shared<Statistic>(
                    kStatGlobalChassisPower,
                    std::make_shared<GlobalAccumulator>()),
                ReadingType::totalChassisPower);
            statisticsProvider_->initializeDbusInterfaces(dbusInterfaces_);
            initializeDbusInterfaces();
            DbusEnableIf::initializeDbusInterfaces(dbusInterfaces_);
            DbusEnableIf::setParentRunning(true);
        }
        void run() override
        {
            devicesManager_->run();
            ptam_->run();
            budgeting_->run();
            control_->run();
            ptam_->postRun();
        }
        virtual void onStateChanged()
        {
            Logger::log<LogLevel::info>("NM is running: %b",
                                        isEnabledOnDbus() && isParentEnabled());
            ptam_->setParentRunning(isEnabledOnDbus() && isParentEnabled());
        }
        std::shared_ptr<DevicesManager> devicesManager_;
        std::shared_ptr<Control> control_;
        std::shared_ptr<EfficiencyControl> efficiencyControl_;
        std::shared_ptr<Budgeting> budgeting_;
        std::shared_ptr<TriggersManager> triggersManager_;
        std::unique_ptr<Ptam> ptam_;

      private:
        boost::asio::io_context& ioc_;
        std::shared_ptr<sdbusplus::asio::connection> bus_;
        std::shared_ptr<sdbusplus::asio::object_server> objectServer_;
        std::string const& objectPath_;
        std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManager_;
        PropertyPtr<std::underlying_type_t<NmHealth>> health_;
        DbusInterfaces dbusInterfaces_{objectPath_, objectServer_};
        std::unique_ptr<StatisticsProvider> statisticsProvider_;
        std::shared_ptr<PolicyStorageManagementMock> policyStorageManagement_ =
            std::make_shared<testing::NiceMock<PolicyStorageManagementMock>>();
        std::shared_ptr<GpioProviderIf> gpioProvider_;
        std::shared_ptr<Budgeting>
            makeBudgeting(std::shared_ptr<DevicesManagerIf> devicesManagerArg)
        {
            SimpleDomainDistributors simpleDomainDistributors_;

            for (const auto& config : kSimpleDomainDistributorsConfig)
            {
                simpleDomainDistributors_.emplace_back(
                    config.raplDomainId,
                    std::make_unique<SimpleDomainBudgeting>(
                        std::move(std::make_unique<RegulatorP>(
                            devicesManagerArg, config.regulatorPCoeff,
                            config.regulatorFeedbackReading)),
                        std::move(std::make_unique<EfficiencyHelper>(
                            devicesManagerArg, config.efficiencyReading,
                            config.efficiencyAveragingPeriod)),
                        config.budgetCorrection));
            }

            auto compoundBudgeting_ = std::make_unique<CompoundDomainBudgeting>(
                std::move(simpleDomainDistributors_));

            control_ = std::make_shared<Control>(devicesManagerArg);

            return std::make_shared<Budgeting>(
                devicesManagerArg, std::move(compoundBudgeting_), control_);
        }
        void initializeDbusInterfaces()
        {
            constexpr const char* versionInitValue = "1.0";
            dbusInterfaces_.addInterface(
                "xyz.openbmc_project.NodeManager.NodeManager",
                [this, &versionInitValue](auto& objectNodeManagerInterface) {
                    dbusInterfaces_.make_property_r(objectNodeManagerInterface,
                                                    "Version",
                                                    versionInitValue);
                    health_ = dbusInterfaces_.make_property_r(
                        objectNodeManagerInterface, "Health",
                        std::underlying_type_t<NmHealth>(NmHealth::ok));
                    dbusInterfaces_.make_property_r(objectNodeManagerInterface,
                                                    "MaxNumberOfPolicies",
                                                    kNodeManagerMaxPolicies);
                });
        }
    };

    unsigned readPowerCapForDomain(DomainId domainId)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{1500});

        switch (domainId)
        {
            case DomainId::CpuSubsystem:
                return std::stoi(hwmonManager_.readFile(hwmonCpuLimit_));
            case DomainId::MemorySubsystem:
                return std::stoi(hwmonManager_.readFile(hwmonMemoryLimit_));
            case DomainId::Pcie:
                return std::stoi(hwmonManager_.readFile(hwmonPcieLimit_));
            case DomainId::DcTotalPower:
                return std::stoi(hwmonManager_.readFile(hwmonDcLimit_));
            default:
                return -1; // unsupported domain
        }
    }

  protected:
    std::filesystem::path hwmonCpuCurrent_;
    std::filesystem::path hwmonCpuLimit_;
    std::filesystem::path hwmonAcCurrent_;
    std::filesystem::path hwmonAcLimit_;
    std::filesystem::path hwmonMemoryCurrent_;
    std::filesystem::path hwmonMemoryLimit_;
    std::filesystem::path hwmonPcieCurrent_;
    std::filesystem::path hwmonPcieLimit_;
    std::filesystem::path hwmonDcCurrent_;
    std::filesystem::path hwmonDcLimit_;

    HwmonFileManager hwmonManager_;
    std::string fakeHwmonPath_;
    std::unique_ptr<FakeNodeManager> sut_;
};
INSTANTIATE_TEST_SUITE_P(DomainId, NodeManagerEndToEndMultiComponentTest,
                         ::testing::Values(DomainId::CpuSubsystem,
                                           DomainId::MemorySubsystem,
                                           DomainId::Pcie,
                                           DomainId::DcTotalPower));
TEST_P(NodeManagerEndToEndMultiComponentTest,
       DISABLED_PolicyCRUDoperationsForDifferentDomains)
{
    /* becase hwmon sensors are read asynchronously with postponed result fetch
     * need to call run() twice. Readings are required for policy creation. */
    ASSERT_NO_THROW(sut_->run());
    ASSERT_NO_THROW(sut_->run());
    PolicyConfig params;
    const auto domainId = GetParam();
    const unsigned policyId = 0;
    unsigned limitInWatts = 60;
    boost::system::error_code ec;
    sdbusplus::message::object_path response;
    bool isEnabled = true;
    /* verify that at the beginging no policy is selected */
    std::tie(ec, response) = BusctlCall::GetSelectedPolicyId(domainId);
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, "/");

    /* create policy with ID 0 and limit 10W */
    std::tie(ec, response) = BusctlCall::CreatePolicyWithId(
        domainId, std::to_string(policyId), params.limit(limitInWatts));
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, DbusPaths::policy(domainId, policyId));
    ASSERT_NO_THROW(sut_->run());
    /* verify that created policy is disabled by default */
    std::tie(ec, isEnabled) =
        BusctlCall::GetProperty<bool>(DbusPaths::policy(domainId, policyId),
                                      kObjectEnabledInterface, "Enabled");
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_FALSE(isEnabled);
    ASSERT_NO_THROW(sut_->run());
    /* enable the policy with ID 0 */
    ec = BusctlCall::SetProperty(DbusPaths::policy(domainId, policyId),
                                 kObjectEnabledInterface, "Enabled", true);
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the policy is enabled now */
    std::tie(ec, isEnabled) =
        BusctlCall::GetProperty<bool>(DbusPaths::policy(domainId, policyId),
                                      kObjectEnabledInterface, "Enabled");
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_TRUE(isEnabled);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the policy with ID 0 is selected */
    std::tie(ec, response) = BusctlCall::GetSelectedPolicyId(domainId);
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, DbusPaths::policy(domainId, policyId));
    ASSERT_NO_THROW(sut_->run());

    /* verify that limit in mW was written to hwmon cpu power cap file */
    ASSERT_EQ(readPowerCapForDomain(domainId), limitInWatts * 1000U);
    /* disable the policy with ID 0 */
    ec = BusctlCall::SetProperty(DbusPaths::policy(domainId, policyId),
                                 kObjectEnabledInterface, "Enabled", false);
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the policy is disabled now */
    std::tie(ec, isEnabled) =
        BusctlCall::GetProperty<bool>(DbusPaths::policy(domainId, policyId),
                                      kObjectEnabledInterface, "Enabled");
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_FALSE(isEnabled);
    /* verify that limit in mW was reset to 0 in hwmon cpu power cap file */
    ASSERT_EQ(readPowerCapForDomain(domainId), 0U);
    /* verify that no policy is selected */
    std::tie(ec, response) = BusctlCall::GetSelectedPolicyId(domainId);
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, "/");
    ASSERT_NO_THROW(sut_->run());
    /* update disabled policy with ID 0 with new limit 20W */
    limitInWatts = 20;
    ec = BusctlCall::UpdatePolicy(domainId, policyId,
                                  params.limit(limitInWatts));
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_NO_THROW(sut_->run());
    /* enable again the policy with ID 0 */
    ec = BusctlCall::SetProperty(DbusPaths::policy(domainId, policyId),
                                 kObjectEnabledInterface, "Enabled", true);
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the policy is enabled now */
    std::tie(ec, isEnabled) =
        BusctlCall::GetProperty<bool>(DbusPaths::policy(domainId, policyId),
                                      kObjectEnabledInterface, "Enabled");
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_TRUE(isEnabled);
    ASSERT_NO_THROW(sut_->run());
    /* verify that new limit in mW was written to hwmon cpu power cap file */
    ASSERT_EQ(readPowerCapForDomain(domainId), limitInWatts * 1000U);

    /* update enabled policy with ID 0 with new limit 15W */
    limitInWatts = 15;
    ec = BusctlCall::UpdatePolicy(domainId, policyId,
                                  params.limit(limitInWatts));
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_NO_THROW(sut_->run());
    /* verify that new limit in mW was written to hwmon cpu power cap file */
    ASSERT_EQ(readPowerCapForDomain(domainId), limitInWatts * 1000U);
    ASSERT_NO_THROW(sut_->run());
    /* delete policy with ID 0 */
    ec = BusctlCall::DeletePolicy(domainId, policyId);
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_NO_THROW(sut_->run());
    /* verify that limit in mW was reset in hwmon cpu power cap file */
    ASSERT_EQ(readPowerCapForDomain(domainId), 0U);
    /* verify that no policy is selected */
    std::tie(ec, response) = BusctlCall::GetSelectedPolicyId(domainId);
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, "/");
    ASSERT_NO_THROW(sut_->run());
}
TEST_P(NodeManagerEndToEndMultiComponentTest,
       DISABLED_CreateThreePoliciesWithTriggerAlwaysOnWithinSameDomain)
{
    /* becase hwmon sensors are read asynchronously with postponed result fetch
     * need to call run() twice. Readings are required for policy creation. */
    ASSERT_NO_THROW(sut_->run());
    ASSERT_NO_THROW(sut_->run());
    PolicyConfig params;
    const auto domainId = GetParam();
    std::vector<unsigned> policyIds = {0, 2, 5};
    std::vector<unsigned> limitInWatts = {30, 10, 20};
    boost::system::error_code ec;
    sdbusplus::message::object_path response;
    bool isEnabled = true;
    /* verify that at the beginging no policy is selected */
    std::tie(ec, response) = BusctlCall::GetSelectedPolicyId(domainId);
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, "/");
    /* create policy with ID 0 and limit 30W */
    std::tie(ec, response) = BusctlCall::CreatePolicyWithId(
        domainId, std::to_string(policyIds[0]), params.limit(limitInWatts[0]));
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, DbusPaths::policy(domainId, policyIds[0]));
    ASSERT_NO_THROW(sut_->run());
    /* verify that created policy is disabled by default */
    std::tie(ec, isEnabled) =
        BusctlCall::GetProperty<bool>(DbusPaths::policy(domainId, policyIds[0]),
                                      kObjectEnabledInterface, "Enabled");
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_FALSE(isEnabled);
    ASSERT_NO_THROW(sut_->run());
    /* enable the policy with ID 0 */
    ec = BusctlCall::SetProperty(DbusPaths::policy(domainId, policyIds[0]),
                                 kObjectEnabledInterface, "Enabled", true);
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the policy is enabled now */
    std::tie(ec, isEnabled) =
        BusctlCall::GetProperty<bool>(DbusPaths::policy(domainId, policyIds[0]),
                                      kObjectEnabledInterface, "Enabled");
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_TRUE(isEnabled);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the policy with ID 0 is selected */
    std::tie(ec, response) = BusctlCall::GetSelectedPolicyId(domainId);
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, DbusPaths::policy(domainId, policyIds[0]));
    ASSERT_NO_THROW(sut_->run());
    /* verify that limit in mW was written to hwmon cpu power cap file */
    ASSERT_EQ(readPowerCapForDomain(domainId), limitInWatts[0] * 1000U);
    /* create policy with ID 2 and limit 10W */
    std::tie(ec, response) = BusctlCall::CreatePolicyWithId(
        domainId, std::to_string(policyIds[1]), params.limit(limitInWatts[1]));
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, DbusPaths::policy(domainId, policyIds[1]));
    ASSERT_NO_THROW(sut_->run());
    /* verify that new limit in mW was NOT written to hwmon cpu power cap file
       since the new Policy is not enabled yet */
    ASSERT_EQ(readPowerCapForDomain(domainId), limitInWatts[0] * 1000U);
    /* verify that the policy with ID 0 is selected */
    std::tie(ec, response) = BusctlCall::GetSelectedPolicyId(domainId);
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, DbusPaths::policy(domainId, policyIds[0]));
    ASSERT_NO_THROW(sut_->run());
    /* enable the policy with ID 2 */
    ec = BusctlCall::SetProperty(DbusPaths::policy(domainId, policyIds[1]),
                                 kObjectEnabledInterface, "Enabled", true);
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the policy with ID 2 is enabled now */
    std::tie(ec, isEnabled) =
        BusctlCall::GetProperty<bool>(DbusPaths::policy(domainId, policyIds[1]),
                                      kObjectEnabledInterface, "Enabled");
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_TRUE(isEnabled);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the policy with ID 2 is selected */
    std::tie(ec, response) = BusctlCall::GetSelectedPolicyId(domainId);
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, DbusPaths::policy(domainId, policyIds[1]));
    ASSERT_NO_THROW(sut_->run());
    /* verify that new limit in mW was written to hwmon cpu power cap file */
    ASSERT_EQ(readPowerCapForDomain(domainId), limitInWatts[1] * 1000U);
    /* create policy with ID 5 and limit 20W */
    std::tie(ec, response) = BusctlCall::CreatePolicyWithId(
        domainId, std::to_string(policyIds[2]), params.limit(limitInWatts[2]));
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, DbusPaths::policy(domainId, policyIds[2]));
    ASSERT_NO_THROW(sut_->run());
    /* verify that new limit in mW was NOT written to hwmon cpu power cap file
       since the new Policy is not enabled yet */
    ASSERT_EQ(readPowerCapForDomain(domainId), limitInWatts[1] * 1000U);
    /* enable the policy with ID 5 */
    ec = BusctlCall::SetProperty(DbusPaths::policy(domainId, policyIds[2]),
                                 kObjectEnabledInterface, "Enabled", true);
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the policy with ID 5 is enabled now */
    std::tie(ec, isEnabled) =
        BusctlCall::GetProperty<bool>(DbusPaths::policy(domainId, policyIds[2]),
                                      kObjectEnabledInterface, "Enabled");
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_TRUE(isEnabled);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the policy with ID 2 is selected as it is most restrictive*/
    std::tie(ec, response) = BusctlCall::GetSelectedPolicyId(domainId);
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, DbusPaths::policy(domainId, policyIds[1]));
    ASSERT_NO_THROW(sut_->run());
    /* verify that new limit in mW was NOT written to hwmon cpu power cap file
       since the new Policy has less restrictive target limit */
    ASSERT_EQ(readPowerCapForDomain(domainId), limitInWatts[1] * 1000U);
    /* disable the policy with ID 2 */
    ec = BusctlCall::SetProperty(DbusPaths::policy(domainId, policyIds[1]),
                                 kObjectEnabledInterface, "Enabled", false);
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the policy with ID 2 is disabled now */
    std::tie(ec, isEnabled) =
        BusctlCall::GetProperty<bool>(DbusPaths::policy(domainId, policyIds[1]),
                                      kObjectEnabledInterface, "Enabled");
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_FALSE(isEnabled);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the policy with ID 5 is selected now */
    std::tie(ec, response) = BusctlCall::GetSelectedPolicyId(domainId);
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, DbusPaths::policy(domainId, policyIds[2]));
    ASSERT_NO_THROW(sut_->run());
    /* verify that new limit in mW was written to hwmon cpu power cap file
       according to current most restrictive and enabled policy with ID 5 */
    ASSERT_EQ(readPowerCapForDomain(domainId), limitInWatts[2] * 1000U);
    /* delete policy with ID 5 */
    ec = BusctlCall::DeletePolicy(domainId, policyIds[2]);
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the policy with ID 0 is selected now */
    std::tie(ec, response) = BusctlCall::GetSelectedPolicyId(domainId);
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, DbusPaths::policy(domainId, policyIds[0]));
    ASSERT_NO_THROW(sut_->run());
    /* verify that limit in mW was written in hwmon cpu power cap file
       according to current most restrictive and enabled policy with ID 0 */
    ASSERT_EQ(readPowerCapForDomain(domainId), limitInWatts[0] * 1000U);
    /* delete policy with ID 0 */
    ec = BusctlCall::DeletePolicy(domainId, policyIds[0]);
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the no policy is selected now */
    std::tie(ec, response) = BusctlCall::GetSelectedPolicyId(domainId);
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, "/");
    ASSERT_NO_THROW(sut_->run());
    /* verify that limit in mW reset in hwmon cpu power cap file
       as there is no policy that is enabled right now */
    ASSERT_EQ(readPowerCapForDomain(domainId), 0U);
    /* enable again the policy with ID 2 */
    ec = BusctlCall::SetProperty(DbusPaths::policy(domainId, policyIds[1]),
                                 kObjectEnabledInterface, "Enabled", true);
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the policy with ID 2 is enabled now */
    std::tie(ec, isEnabled) =
        BusctlCall::GetProperty<bool>(DbusPaths::policy(domainId, policyIds[1]),
                                      kObjectEnabledInterface, "Enabled");
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_TRUE(isEnabled);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the policy with ID 2 is selected now */
    std::tie(ec, response) = BusctlCall::GetSelectedPolicyId(domainId);
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, DbusPaths::policy(domainId, policyIds[1]));
    ASSERT_NO_THROW(sut_->run());
    /* verify that new limit in mW was written to hwmon cpu power cap file
       according to current most restrictive and enabled policy with ID 5 */
    ASSERT_EQ(readPowerCapForDomain(domainId), limitInWatts[1] * 1000U);
    /* delete policy with ID 2 */
    ec = BusctlCall::DeletePolicy(domainId, policyIds[1]);
    EXPECT_EQ(ec, boost::system::errc::success);
    ASSERT_NO_THROW(sut_->run());
    /* verify that the no policy is selected now */
    std::tie(ec, response) = BusctlCall::GetSelectedPolicyId(domainId);
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, "/");
    ASSERT_NO_THROW(sut_->run());
    /* verify that limit in mW reset in hwmon cpu power cap file
       as there is no policy right now */
    ASSERT_EQ(readPowerCapForDomain(domainId), 0U);
}
class NodeManagerEndToEndStatisticMultiComponentTest
    : public NodeManagerEndToEndMultiComponentTest
{
  public:
    void sutRunAndStepTime(unsigned timeStepMs)
    {
        for (unsigned it = 0; it < timeStepMs / 100; it++)
        {
            ASSERT_NO_THROW(sut_->run());
            Clock::stepMs(100);
        }
    }
    double getAveragePower(std::map<std::string, StatValuesMap>& statMap)
    {
        return std::get<double>(statMap["Power"]["Average"]);
    }
    double getCurrentPower(std::map<std::string, StatValuesMap>& statMap)
    {
        return std::get<double>(statMap["Power"]["Current"]);
    }
    double getMaxPower(std::map<std::string, StatValuesMap>& statMap)
    {
        return std::get<double>(statMap["Power"]["Max"]);
    }
    double getMinPower(std::map<std::string, StatValuesMap>& statMap)
    {
        return std::get<double>(statMap["Power"]["Min"]);
    }
    bool getMeasurementState(std::map<std::string, StatValuesMap>& statMap)
    {
        return std::get<bool>(statMap["Power"]["MeasurementState"]);
    }
    void simulateCurrentPowerReadingInMilliWatts(DomainId domain,
                                                 unsigned currPower)
    {
        switch (domain)
        {
            case DomainId::CpuSubsystem:
                hwmonManager_.writeFile(hwmonCpuCurrent_, currPower);
                break;
            case DomainId::MemorySubsystem:
                hwmonManager_.writeFile(hwmonMemoryCurrent_, currPower);
                break;
            case DomainId::Pcie:
                hwmonManager_.writeFile(hwmonPcieCurrent_, currPower);
                break;
            case DomainId::DcTotalPower:
                hwmonManager_.writeFile(hwmonDcCurrent_, currPower);
                break;
            default:
                break;
        }
    }
};
INSTANTIATE_TEST_SUITE_P(
    DomainId, NodeManagerEndToEndStatisticMultiComponentTest,
    ::testing::Values(DomainId::CpuSubsystem, DomainId::MemorySubsystem,
                      DomainId::Pcie, DomainId::DcTotalPower));
TEST_P(NodeManagerEndToEndStatisticMultiComponentTest,
       DISABLED_GetStatisticsAndResetStatisticsForPolicyAndDomain)
{
    /* in case of HistoricalReadings this line will set min to 0 */
    simulateCurrentPowerReadingInMilliWatts(GetParam(), 0);
    /* becase hwmon sensors are read asynchronously with postponed result fetch
     * we need to call the run() twice here and below. */
    ASSERT_NO_THROW(sut_->run());
    ASSERT_NO_THROW(sut_->run());
    /* create policy with ID 0 and limit 30W */
    PolicyConfig params;
    const auto domainId = GetParam();
    unsigned policyId = 0;
    unsigned limitInWatts = 30;
    boost::system::error_code ec;
    sdbusplus::message::object_path response;
    std::map<std::string, StatValuesMap> statMap;
    std::tie(ec, response) = BusctlCall::CreatePolicyWithId(
        domainId, std::to_string(policyId), params.limit(limitInWatts));
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, DbusPaths::policy(domainId, policyId));
    /* simulate time and power changes */
    sutRunAndStepTime(5000);
    simulateCurrentPowerReadingInMilliWatts(domainId, 20000);
    ASSERT_NO_THROW(sut_->run());
    ASSERT_NO_THROW(sut_->run());
    sutRunAndStepTime(5000);
    /* verify policy statistics */
    std::tie(ec, statMap) =
        BusctlCall::GetStatistics(DbusPaths::policy(domainId, policyId));
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_DOUBLE_EQ(getAveragePower(statMap), 10.0);
    EXPECT_DOUBLE_EQ(getCurrentPower(statMap), 20.0);
    EXPECT_DOUBLE_EQ(getMaxPower(statMap), 20.0);
    EXPECT_DOUBLE_EQ(getMinPower(statMap), 0.0);
    EXPECT_TRUE(getMeasurementState(statMap));
    /* reset statistics for domain and policy*/
    ec = BusctlCall::ResetStatistics(DbusPaths::policy(domainId, policyId));
    ec = BusctlCall::ResetStatistics(DbusPaths::domain(domainId));
    EXPECT_EQ(ec, boost::system::errc::success);
    /* simulate time and power changes */
    simulateCurrentPowerReadingInMilliWatts(domainId, 40000);
    sutRunAndStepTime(1500);
    simulateCurrentPowerReadingInMilliWatts(domainId, 20000);
    sutRunAndStepTime(1500);
    /* verify policy and domain statistics after reset*/
    std::tie(ec, statMap) =
        BusctlCall::GetStatistics(DbusPaths::policy(domainId, policyId));
    EXPECT_DOUBLE_EQ(getAveragePower(statMap), 30.0);
    EXPECT_DOUBLE_EQ(getCurrentPower(statMap), 20.0);
    EXPECT_DOUBLE_EQ(getMaxPower(statMap), 40.0);
    EXPECT_DOUBLE_EQ(getMinPower(statMap), 20.0);
    EXPECT_TRUE(getMeasurementState(statMap));
    std::tie(ec, statMap) =
        BusctlCall::GetStatistics(DbusPaths::domain(domainId));
    EXPECT_DOUBLE_EQ(getAveragePower(statMap), 30.0);
    EXPECT_DOUBLE_EQ(getCurrentPower(statMap), 20.0);
    EXPECT_DOUBLE_EQ(getMaxPower(statMap), 40.0);
    EXPECT_DOUBLE_EQ(getMinPower(statMap), 20.0);
    EXPECT_TRUE(getMeasurementState(statMap));
}