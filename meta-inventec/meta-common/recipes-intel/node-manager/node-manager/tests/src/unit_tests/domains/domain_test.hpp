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
#include "common_types.hpp"
#include "domains/domain.hpp"
#include "domains/domain_ac_total_power.hpp"
#include "domains/domain_cpu_subsystem.hpp"
#include "domains/domain_dc_total_power.hpp"
#include "domains/domain_memory_subsystem.hpp"
#include "domains/domain_pcie.hpp"
#include "domains/domain_types.hpp"
#include "mocks/budgeting_mock.hpp"
#include "mocks/capabilities_factory_mock.hpp"
#include "mocks/component_capabilities_mock.hpp"
#include "mocks/devices_manager_mock.hpp"
#include "mocks/domain_capabilities_mock.hpp"
#include "mocks/gpio_provider_mock.hpp"
#include "mocks/policy_factory_mock.hpp"
#include "mocks/policy_mock.hpp"
#include "mocks/reading_mock.hpp"
#include "mocks/triggers_manager_mock.hpp"
#include "policies/policy.hpp"
#include "readings/reading_type.hpp"
#include "triggers/trigger_enums.hpp"
#include "utils/dbus_environment.hpp"
#include "utils/domain_params.hpp"
#include "utils/policy_config.hpp"

#include <bitset>
#include <boost/system/error_code.hpp>
#include <iostream>
#include <sdbusplus/asio/property.hpp>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gtest/internal/gtest-death-test-internal.h>

namespace nodemanager
{

static const unsigned int kPoliciesCreatedNum = 3;
static const uint32_t kMaxCorrectionTimeInMs = 60000;
static const uint32_t kMinCorrectionTimeInMs = 1000;
static const uint16_t kMaxStatisticsReportingPeriod = 3600;
static const uint16_t kMinStatisticsReportingPeriod = 1;
static const unsigned int kMaxPoliciesNum = 64;

static constexpr const auto kPolicyManagerInterface =
    "xyz.openbmc_project.NodeManager.PolicyManager";
static constexpr const auto kDomainCapabilitiesInterface =
    "xyz.openbmc_project.NodeManager.Capabilities";
static constexpr const auto kDomainAttributesInterface =
    "xyz.openbmc_project.NodeManager.DomainAttributes";
static constexpr const auto kObjectDeleteInterface =
    "xyz.openbmc_project.Object.Delete";
static const std::string kDomainObjectPath =
    std::string("/xyz/openbmc_project/NodeManager/Domain/");

static std::unordered_map<std::type_index, DomainId> domainClassToDomainId = {
    {typeid(DomainAcTotalPower), DomainId(0)},
    {typeid(DomainCpuSubsystem), DomainId(1)},
    {typeid(DomainMemorySubsystem), DomainId(2)},
    {typeid(DomainHwProtection), DomainId(3)},
    {typeid(DomainPcie), DomainId(4)},
    {typeid(DomainDcTotalPower), DomainId(5)},
};

static std::unordered_map<std::type_index, ReadingType>
    domainClassToControlledParameter = {
        {typeid(DomainAcTotalPower), ReadingType::acPlatformPower},
        {typeid(DomainCpuSubsystem), ReadingType::cpuPackagePower},
        {typeid(DomainMemorySubsystem), ReadingType::dramPower},
        {typeid(DomainHwProtection), ReadingType::hwProtectionPlatformPower},
        {typeid(DomainPcie), ReadingType::pciePower},
        {typeid(DomainDcTotalPower), ReadingType::dcPlatformPower},
};

static std::unordered_map<std::type_index, DeviceIndex>
    domainClassToMaxComponentsNumber = {
        {typeid(DomainAcTotalPower), kComponentIdIgnored},
        {typeid(DomainCpuSubsystem), kMaxCpuNumber},
        {typeid(DomainMemorySubsystem), kMaxCpuNumber},
        {typeid(DomainHwProtection), kComponentIdIgnored},
        {typeid(DomainPcie), kMaxPcieNumber},
        {typeid(DomainDcTotalPower), kComponentIdIgnored},
};

static std::unordered_map<std::type_index,
                          std::shared_ptr<std::vector<TriggerType>>>
    domainClassToTriggerTypes = {
        {typeid(DomainAcTotalPower),
         std::make_shared<std::vector<TriggerType>>(std::vector<TriggerType>{
             TriggerType::always, TriggerType::inletTemperature,
             TriggerType::gpio, TriggerType::cpuUtilization,
             TriggerType::hostReset, TriggerType::smbalertInterrupt})},
        {typeid(DomainCpuSubsystem),
         std::make_shared<std::vector<TriggerType>>(std::vector<TriggerType>{
             TriggerType::always, TriggerType::inletTemperature,
             TriggerType::gpio, TriggerType::cpuUtilization,
             TriggerType::hostReset, TriggerType::smbalertInterrupt})},
        {typeid(DomainMemorySubsystem),
         std::make_shared<std::vector<TriggerType>>(std::vector<TriggerType>{
             TriggerType::always, TriggerType::inletTemperature,
             TriggerType::gpio, TriggerType::hostReset,
             TriggerType::smbalertInterrupt})},
        {typeid(DomainHwProtection),
         std::make_shared<std::vector<TriggerType>>(
             std::vector<TriggerType>{TriggerType::always, TriggerType::gpio})},
        {typeid(DomainPcie),
         std::make_shared<std::vector<TriggerType>>(std::vector<TriggerType>{
             TriggerType::always, TriggerType::inletTemperature,
             TriggerType::gpio, TriggerType::hostReset,
             TriggerType::smbalertInterrupt})},
        {typeid(DomainDcTotalPower),
         std::make_shared<std::vector<TriggerType>>(std::vector<TriggerType>{
             TriggerType::always, TriggerType::inletTemperature,
             TriggerType::gpio, TriggerType::cpuUtilization,
             TriggerType::hostReset, TriggerType::smbalertInterrupt})}};

struct PolicySetupConfig
{
    PolicyState state;
    uint16_t limit;
    BudgetingStrategy strategy;
    uint8_t componentId;
};

template <typename T>
class DomainTestSimple : public testing::Test
{
  public:
    virtual ~DomainTestSimple() = default;

    virtual void SetUp() override
    {
        ON_CALL(*this->devicesManager_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::cpuPresence, kAllDevices))
            .WillByDefault(testing::SaveArg<0>(
                &(this->registerAvailableComponentsHandler_)));

        ON_CALL(*this->devicesManager_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::hostPower, kAllDevices))
            .WillByDefault(
                testing::SaveArg<0>(&(this->hostPowerOnEventHandler_)));

        ON_CALL(*this->devicesManager_,
                registerReadingConsumerHelper(
                    testing::_, domainClassToControlledParameter.at(typeid(T)),
                    kAllDevices))
            .WillByDefault(
                testing::SaveArg<0>(&(this->requiredReadingsEventsHandler_)));

        ON_CALL(*this->capabilitiesFactory_,
                createDomainCapabilities(
                    domainClassToMinCapability.at(typeid(T)),
                    domainClassToMaxCapability.at(typeid(T)),
                    domainClassToMinCorrectionTime.at(typeid(T)), testing::_,
                    testing::_))
            .WillByDefault(
                testing::DoAll(testing::SaveArg<3>(&capabilitiesChangeCallback),
                               testing::Return(this->capabilities_)));

        ON_CALL(*this->capabilitiesFactory_,
                createComponentCapabilities(
                    domainClassToMinCapability.at(typeid(T)),
                    domainClassToMaxCapability.at(typeid(T)), testing::_))
            .WillByDefault(testing::Return(this->compCapabilities_));

        setupInternalPolicies();

        sut_ = std::make_shared<T>(
            DbusEnvironment::getBus(), DbusEnvironment::getObjServer(),
            "/xyz/openbmc_project/NodeManager", devicesManager_, gpioProvider_,
            triggersManager_, budgeting_, policyFactory_, capabilitiesFactory_,
            DbusState::enabled);

        sut_->createDefaultPolicies();

        this->setAvailableComponents(1.0);

        domainId_ = domainClassToDomainId.at(typeid(*(this->sut_)));

        expectedDomainInfo_ = DomainInfo{
            kDomainObjectPath + enumToStr(kDomainIdNames, domainId_),
            domainClassToControlledParameter.at(typeid(*(this->sut_))),
            capabilities_,
            domainId_,
            expectedAvailableComponents_,
            false,
            domainClassToTriggerTypes.at(typeid(*(this->sut_))),
            domainClassToMaxComponentsNumber.at(typeid(*(this->sut_)))};

        for (unsigned int policyIndex = 0; policyIndex < kPoliciesCreatedNum;
             policyIndex++)
        {
            auto policy = std::make_shared<testing::NiceMock<PolicyMock>>();
            policies_.push_back(policy);

            ON_CALL(*policy, getId())
                .WillByDefault(testing::Return(std::to_string(policyIndex)));

            auto objectPath =
                kDomainObjectPath + enumToStr(kDomainIdNames, domainId_) +
                std::string{"/Policy/"} + std::to_string(policyIndex);
            policiesPaths_[policyIndex] = objectPath;
            ON_CALL(testing::Const(*policy), getObjectPath())
                .WillByDefault(testing::ReturnRef(policiesPaths_[policyIndex]));
        }

        setReadingAvailable(domainClassToControlledParameter.at(typeid(T)));

        ON_CALL(*this->devicesManager_,
                findReading(
                    domainClassToControlledParameter.at(typeid(*(this->sut_)))))
            .WillByDefault(testing::Return(this->hwProtectionReading_));
    }

    void setHostPowerOnReading(bool isHostPowerOn)
    {
        assert(hostPowerOnEventHandler_ != nullptr);
        hostPowerOnEventHandler_->updateValue(
            static_cast<double>(isHostPowerOn));
    }
    void setAvailableComponents(double value)
    {
        if (nullptr != this->registerAvailableComponentsHandler_)
        {
            std::bitset<8> componentsPresenceMap =
                static_cast<unsigned long>(value);
            for (DeviceIndex componentId = 0;
                 componentId < componentsPresenceMap.size(); componentId++)
            {
                if (componentsPresenceMap[componentId])
                {
                    expectedAvailableComponents_->push_back(componentId);
                }
            }
            this->registerAvailableComponentsHandler_->updateValue(value);
        }
    }

    void setReadingAvailable(ReadingType readingType,
                             DeviceIndex deviceIndex = kAllDevices)
    {
        if (nullptr != this->requiredReadingsEventsHandler_)
        {
            this->requiredReadingsEventsHandler_->reportEvent(
                ReadingEventType::readingAvailable, {readingType, deviceIndex});
        }
    }

    void setReadingSource(SensorReadingType source)
    {
        ON_CALL(*this->hwProtectionReading_, getReadingSource())
            .WillByDefault(testing::Return(source));
    }

    std::tuple<boost::system::error_code,
               std::map<std::string, CapabilitiesValuesMap>>
        dbusGetAllLimitsCapabilities()
    {
        std::promise<std::tuple<boost::system::error_code,
                                std::map<std::string, CapabilitiesValuesMap>>>
            promise;
        DbusEnvironment::getBus()->async_method_call(
            [&promise](
                boost::system::error_code ec,
                const std::map<std::string, CapabilitiesValuesMap>& map) {
                promise.set_value(std::make_tuple(ec, map));
            },
            DbusEnvironment::serviceName(),
            kDomainObjectPath + enumToStr(kDomainIdNames, domainId_),
            kDomainCapabilitiesInterface, "GetAllLimitsCapabilities");

        return DbusEnvironment::waitForFuture(promise.get_future());
    }

    boost::system::error_code dbusSetProperty(std::string interface,
                                              std::string name, double value)
    {
        std::promise<boost::system::error_code> promise;
        sdbusplus::asio::setProperty(
            *(DbusEnvironment::getBus()), DbusEnvironment::serviceName(),
            kDomainObjectPath + enumToStr(kDomainIdNames, domainId_), interface,
            name, value, [&promise](boost::system::error_code ec) {
                promise.set_value(ec);
            });

        return DbusEnvironment::waitForFuture(promise.get_future());
    }

    template <typename PropertyType>
    std::tuple<boost::system::error_code, PropertyType>
        dbusGetProperty(std::string interface, std::string name)
    {
        std::promise<std::tuple<boost::system::error_code, PropertyType>>
            promise;
        sdbusplus::asio::getProperty<PropertyType>(
            *(DbusEnvironment::getBus()), DbusEnvironment::serviceName(),
            kDomainObjectPath + enumToStr(kDomainIdNames, domainId_), interface,
            name, [&promise](boost::system::error_code ec, PropertyType data) {
                promise.set_value(std::make_tuple(ec, data));
            });

        return DbusEnvironment::waitForFuture(promise.get_future());
    }

    void setupInternalPolicies()
    {
        this->dmtfPolicy_ = std::make_shared<testing::NiceMock<PolicyMock>>();
        ON_CALL(*this->policyFactory_,
                createPolicy(
                    testing::_, testing::_, testing::HasSubstr("DmtfPower"),
                    testing::_, testing::_, testing::_, testing::_,
                    PolicyEditable::yes, false, testing::_, testing::_))
            .WillByDefault(testing::Return(this->dmtfPolicy_));

        this->smbalertPolicy_ =
            std::make_shared<testing::NiceMock<PolicyMock>>();
        ON_CALL(
            *this->policyFactory_,
            createPolicy(testing::_, testing::_, testing::HasSubstr("SMBAlert"),
                         testing::_, testing::_, testing::_, testing::_,
                         PolicyEditable::yes, false, testing::_, testing::_))
            .WillByDefault(testing::Return(this->smbalertPolicy_));

        this->hwProtectionPolicy_ =
            std::make_shared<testing::NiceMock<PolicyMock>>();
        ON_CALL(*this->policyFactory_,
                createPolicy(testing::_, testing::_,
                             testing::HasSubstr("HwProtectionAlwaysOn"),
                             testing::_, testing::_, testing::_, testing::_,
                             PolicyEditable::no, false, testing::_, testing::_))
            .WillByDefault(testing::Return(this->hwProtectionPolicy_));
    }

    std::shared_ptr<DevicesManagerMock> devicesManager_ =
        std::make_shared<testing::NiceMock<DevicesManagerMock>>();
    std::shared_ptr<GpioProviderMock> gpioProvider_ =
        std::make_shared<testing::NiceMock<GpioProviderMock>>();
    std::shared_ptr<TriggersManagerMock> triggersManager_ =
        std::make_shared<testing::NiceMock<TriggersManagerMock>>();
    std::shared_ptr<BudgetingMock> budgeting_ =
        std::make_shared<testing::NiceMock<BudgetingMock>>();
    std::shared_ptr<PolicyFactoryMock> policyFactory_ =
        std::make_shared<testing::NiceMock<PolicyFactoryMock>>();
    std::shared_ptr<CapabilitiesFactoryMock> capabilitiesFactory_ =
        std::make_shared<testing::NiceMock<CapabilitiesFactoryMock>>();
    std::vector<std::shared_ptr<PolicyMock>> policies_;
    std::shared_ptr<PolicyMock> dmtfPolicy_;
    std::shared_ptr<PolicyMock> smbalertPolicy_;
    std::shared_ptr<PolicyMock> hwProtectionPolicy_;
    std::string policiesPaths_[kPoliciesCreatedNum];
    std::shared_ptr<DomainCapabilitiesMock> capabilities_ =
        std::make_shared<testing::NiceMock<DomainCapabilitiesMock>>();
    std::shared_ptr<ComponentCapabilitiesMock> compCapabilities_ =
        std::make_shared<testing::NiceMock<ComponentCapabilitiesMock>>();
    std::shared_ptr<Domain> sut_;
    DomainId domainId_;
    std::shared_ptr<ReadingConsumer> hostPowerOnEventHandler_ = nullptr;
    std::shared_ptr<ReadingConsumer> registerAvailableComponentsHandler_ =
        nullptr;
    DeleteCallback lastSavedDeleteCallback_;
    DomainInfo expectedDomainInfo_;
    std::shared_ptr<std::vector<DeviceIndex>> expectedAvailableComponents_ =
        std::make_shared<std::vector<DeviceIndex>>();
    OnCapabilitiesChangeCallback capabilitiesChangeCallback;
    std::shared_ptr<ReadingConsumer> requiredReadingsEventsHandler_ = nullptr;
    std::shared_ptr<ReadingMock> hwProtectionReading_ =
        std::make_shared<testing::NiceMock<ReadingMock>>();
};

typedef testing::Types<DomainAcTotalPower, DomainCpuSubsystem,
                       DomainMemorySubsystem, DomainHwProtection, DomainPcie,
                       DomainDcTotalPower>
    SimpleImplementations;
TYPED_TEST_SUITE(DomainTestSimple, SimpleImplementations);

template <typename T>
class DomainTestWithPolicies : public DomainTestSimple<T>
{
  public:
    std::tuple<boost::system::error_code, sdbusplus::message::object_path>
        dbusGetSelectedPolicyId()
    {
        std::promise<std::tuple<boost::system::error_code,
                                sdbusplus::message::object_path>>
            promise;
        DbusEnvironment::getBus()->async_method_call(
            [&promise](
                boost::system::error_code ec,
                const sdbusplus::message::object_path& limitingPolicyId) {
                promise.set_value({ec, limitingPolicyId});
            },
            DbusEnvironment::serviceName(),
            kDomainObjectPath + enumToStr(kDomainIdNames, this->domainId_),
            kPolicyManagerInterface, "GetSelectedPolicyId");

        return DbusEnvironment::waitForFuture(promise.get_future());
    }

    std::tuple<boost::system::error_code, sdbusplus::message::object_path>
        dbusCreatePolicyWithId(PolicyId id, PolicyConfig& params,
                               std::shared_ptr<PolicyIf> returnObject)
    {
        std::promise<std::tuple<boost::system::error_code,
                                sdbusplus::message::object_path>>
            promise;
        DbusEnvironment::getBus()->async_method_call(
            [&promise](boost::system::error_code ec,
                       const sdbusplus::message::object_path& path) {
                promise.set_value(std::make_tuple(ec, path));
            },
            DbusEnvironment::serviceName(),
            kDomainObjectPath + enumToStr(kDomainIdNames, this->domainId_),
            kPolicyManagerInterface, "CreateWithId", id, params._getTuple());

        return DbusEnvironment::waitForFuture(promise.get_future());
    }

    std::tuple<boost::system::error_code, sdbusplus::message::object_path>
        dbusCreatePolicyForTotalBudget(PolicyId id, PolicyConfig& params)
    {
        std::promise<std::tuple<boost::system::error_code,
                                sdbusplus::message::object_path>>
            promise;
        DbusEnvironment::getBus()->async_method_call(
            [&promise](boost::system::error_code ec,
                       const sdbusplus::message::object_path& path) {
                promise.set_value(std::make_tuple(ec, path));
            },
            DbusEnvironment::serviceName(),
            kDomainObjectPath + enumToStr(kDomainIdNames, this->domainId_),
            kPolicyManagerInterface, "CreateForTotalBudget", id,
            params._getTuple());

        return DbusEnvironment::waitForFuture(promise.get_future());
    }

    void setupPolicies(std::vector<PolicySetupConfig> config,
                       double capabilitiesMin, double capabilitiesMax)
    {
        PolicyConfig params;
        boost::system::error_code ec;
        sdbusplus::message::object_path response;

        for (unsigned int i = 0; i < config.size(); i++)
        {
            PolicySetupConfig configPerPolicy = config.at(i);

            ON_CALL(*this->policyFactory_,
                    createPolicy(testing::Pointee(this->expectedDomainInfo_),
                                 PolicyType::power, std::to_string(i),
                                 PolicyOwner::bmc, params.statReportingPeriod(),
                                 testing::_, testing::_, PolicyEditable::yes,
                                 true, testing::_, testing::_))
                .WillByDefault([this, i](const auto&, const auto&, const auto&,
                                         const auto&, const auto&,
                                         const auto& callback, const auto&,
                                         const auto&, const auto&, const auto&,
                                         const auto&) {
                    this->lastSavedDeleteCallback_ = callback;
                    return this->policies_.at(i);
                });

            std::tie(ec, response) = this->dbusCreatePolicyWithId(
                std::to_string(i), params, nullptr);
            EXPECT_EQ(ec, boost::system::errc::success);

            ON_CALL(*this->policies_.at(i), getState())
                .WillByDefault(testing::Return(configPerPolicy.state));
            ON_CALL(*this->policies_.at(i), getLimit())
                .WillByDefault(testing::Return(configPerPolicy.limit));
            ON_CALL(*this->policies_.at(i), getStrategy())
                .WillByDefault(testing::Return(configPerPolicy.strategy));
            ON_CALL(*this->policies_.at(i), getInternalComponentId())
                .WillByDefault(testing::Return(configPerPolicy.componentId));
        }

        ON_CALL(*this->capabilities_, getMin())
            .WillByDefault(testing::Return(capabilitiesMin));
        ON_CALL(*this->capabilities_, getMax())
            .WillByDefault(testing::Return(capabilitiesMax));

        ON_CALL(*this->compCapabilities_, getMin())
            .WillByDefault(testing::Return(capabilitiesMin));
        ON_CALL(*this->compCapabilities_, getMax())
            .WillByDefault(testing::Return(capabilitiesMax));
    }
};

typedef testing::Types<DomainAcTotalPower, DomainCpuSubsystem,
                       DomainMemorySubsystem, DomainPcie, DomainDcTotalPower>
    WithPoliciesImplementations;

TYPED_TEST_SUITE(DomainTestWithPolicies, WithPoliciesImplementations);

TYPED_TEST(DomainTestWithPolicies, RunTriggersRunLoopsInPolicies)
{
    PolicyConfig params;

    ON_CALL(*this->policyFactory_,
            createPolicy(testing::_, PolicyType::power, PolicyId("0"),
                         PolicyOwner::bmc, params.statReportingPeriod(),
                         testing::_, testing::_, PolicyEditable::yes, true,
                         testing::_, testing::_))
        .WillByDefault(testing::Return(this->policies_.at(0)));
    auto [ec, policyId] = this->dbusCreatePolicyWithId("0", params, nullptr);
    EXPECT_EQ(ec, boost::system::errc::success);

    ON_CALL(*this->policyFactory_,
            createPolicy(testing::_, PolicyType::power, PolicyId("1"),
                         PolicyOwner::bmc, params.statReportingPeriod(),
                         testing::_, testing::_, PolicyEditable::yes, true,
                         testing::_, testing::_))
        .WillByDefault(testing::Return(this->policies_.at(1)));
    std::tie(ec, policyId) = this->dbusCreatePolicyWithId("1", params, nullptr);
    EXPECT_EQ(ec, boost::system::errc::success);

    EXPECT_CALL(*this->policies_.at(0), run()).Times(1);
    EXPECT_CALL(*this->policies_.at(1), run()).Times(1);
    (this->sut_)->run();
}

TYPED_TEST(DomainTestSimple,
           SetMaxDbusPropertyExpectCapabilitiesStructReflectChange)
{
    ON_CALL(*this->capabilities_, getMaxRated())
        .WillByDefault(testing::Return(200.0));
    this->setReadingSource(SensorReadingType::dcPlatformPowerCpu);
    EXPECT_CALL(*this->capabilities_, setMax(123));
    EXPECT_THAT(this->dbusSetProperty(kDomainCapabilitiesInterface, "Max", 123),
                testing::Eq(boost::system::errc::success));
}

TYPED_TEST(DomainTestSimple,
           SetMinDbusPropertyExpectCapabilitiesStructReflectChange)
{
    this->setReadingSource(SensorReadingType::dcPlatformPowerCpu);
    EXPECT_CALL(*this->capabilities_, setMin(123));
    EXPECT_THAT(this->dbusSetProperty(kDomainCapabilitiesInterface, "Min", 123),
                testing::Eq(boost::system::errc::success));
}

TYPED_TEST(DomainTestSimple, SetMaxDbusPropertyRangeVerification)
{
    ON_CALL(*this->capabilities_, getMaxRated())
        .WillByDefault(testing::Return(2 * kMaxSupportedCapabilityValue));
    EXPECT_TRUE(this->dbusSetProperty(kDomainCapabilitiesInterface, "Max",
                                      kMinSupportedCapabilityValue - 1)
                    .failed());
    EXPECT_TRUE(this->dbusSetProperty(kDomainCapabilitiesInterface, "Max",
                                      kMaxSupportedCapabilityValue + 1)
                    .failed());
    EXPECT_FALSE(this->dbusSetProperty(kDomainCapabilitiesInterface, "Max",
                                       kMinSupportedCapabilityValue)
                     .failed());
    EXPECT_FALSE(this->dbusSetProperty(kDomainCapabilitiesInterface, "Max",
                                       kMaxSupportedCapabilityValue)
                     .failed());
}

TYPED_TEST(DomainTestSimple, SetMinDbusPropertyRangeVerification)
{
    EXPECT_TRUE(this->dbusSetProperty(kDomainCapabilitiesInterface, "Min",
                                      kMinSupportedCapabilityValue - 1)
                    .failed());
    EXPECT_TRUE(this->dbusSetProperty(kDomainCapabilitiesInterface, "Min",
                                      kMaxSupportedCapabilityValue + 1)
                    .failed());
    EXPECT_FALSE(this->dbusSetProperty(kDomainCapabilitiesInterface, "Min",
                                       kMinSupportedCapabilityValue)
                     .failed());
    EXPECT_FALSE(this->dbusSetProperty(kDomainCapabilitiesInterface, "Min",
                                       kMaxSupportedCapabilityValue)
                     .failed());
}

TYPED_TEST(DomainTestWithPolicies,
           CapabilitiesChangeCallbackExpectPoliciesToValidateParams)
{
    PolicyConfig params;

    ON_CALL(*this->policyFactory_,
            createPolicy(testing::Pointee(this->expectedDomainInfo_),
                         PolicyType::power, PolicyId("0"), PolicyOwner::bmc,
                         params.statReportingPeriod(), testing::_, testing::_,
                         PolicyEditable::yes, true, testing::_, testing::_))
        .WillByDefault(testing::Return(this->policies_.at(0)));
    auto [ec, policyId] = this->dbusCreatePolicyWithId("0", params, nullptr);
    EXPECT_EQ(ec, boost::system::errc::success);

    ON_CALL(*this->policyFactory_,
            createPolicy(testing::Pointee(this->expectedDomainInfo_),
                         PolicyType::power, PolicyId("1"), PolicyOwner::bmc,
                         params.statReportingPeriod(), testing::_, testing::_,
                         PolicyEditable::yes, true, testing::_, testing::_))
        .WillByDefault(testing::Return(this->policies_.at(1)));
    std::tie(ec, policyId) = this->dbusCreatePolicyWithId("1", params, nullptr);
    EXPECT_EQ(ec, boost::system::errc::success);

    EXPECT_CALL(*this->policies_.at(0), validateParameters());
    EXPECT_CALL(*this->policies_.at(1), validateParameters());
    this->capabilitiesChangeCallback();
}

TYPED_TEST(DomainTestSimple, ExpectCorrectMaxCorrectionTimeOnDbus)
{
    EXPECT_CALL(*this->capabilities_, getMaxCorrectionTimeInMs());
    auto [ec, response] = this->template dbusGetProperty<uint32_t>(
        kDomainCapabilitiesInterface, "MaxCorrectionTimeInMs");
    EXPECT_EQ(ec, boost::system::errc::success);
}

TYPED_TEST(DomainTestSimple, ExpectCorrectMinCorrectionTimeOnDbus)
{
    EXPECT_CALL(*this->capabilities_, getMinCorrectionTimeInMs());
    auto [ec, response] = this->template dbusGetProperty<uint32_t>(
        kDomainCapabilitiesInterface, "MinCorrectionTimeInMs");
    EXPECT_EQ(ec, boost::system::errc::success);
}

TYPED_TEST(DomainTestSimple, ExpectCorrectMaxStatisticsReportingPeriodOnDbus)
{

    EXPECT_CALL(*this->capabilities_, getMaxStatReportingPeriod());
    auto [ec, response] = this->template dbusGetProperty<uint16_t>(
        kDomainCapabilitiesInterface, "MaxStatisticsReportingPeriod");
    EXPECT_EQ(ec, boost::system::errc::success);
}

TYPED_TEST(DomainTestSimple, ExpectCorrectMinStatisticsReportingPeriodOnDbus)
{
    EXPECT_CALL(*this->capabilities_, getMinStatReportingPeriod());
    auto [ec, response] = this->template dbusGetProperty<uint16_t>(
        kDomainCapabilitiesInterface, "MinStatisticsReportingPeriod");
    EXPECT_EQ(ec, boost::system::errc::success);
}

TYPED_TEST(DomainTestSimple, ExpectCorrectAvailableTriggersListedOnDbus)
{
    std::vector<std::string> expectedTriggerNames{};

    for (auto trigger : *(this->expectedDomainInfo_.triggers))
    {
        expectedTriggerNames.push_back(kTriggerTypeNames.at(trigger));
    }

    ON_CALL(*this->triggersManager_, isTriggerAvailable(testing::_))
        .WillByDefault(testing::Return(true));

    auto [ec, response] =
        this->template dbusGetProperty<std::vector<std::string>>(
            kDomainAttributesInterface, "AvailableTriggers");
    EXPECT_EQ(ec, boost::system::errc::success);

    ASSERT_THAT(response, testing::ElementsAreArray(expectedTriggerNames));
}

TYPED_TEST(DomainTestSimple, ExpectNoAvailableTriggersListedOnDbus)
{
    std::vector<std::string> expectedTriggerNames{};

    ON_CALL(*this->triggersManager_, isTriggerAvailable(testing::_))
        .WillByDefault(testing::Return(false));

    auto [ec, response] =
        this->template dbusGetProperty<std::vector<std::string>>(
            kDomainAttributesInterface, "AvailableTriggers");
    EXPECT_EQ(ec, boost::system::errc::success);

    ASSERT_THAT(response, testing::ElementsAreArray(expectedTriggerNames));
}

TYPED_TEST(DomainTestSimple, ExpectGpioTriggerNotListedOnDbus)
{
    const TriggerType notAvailableTrigger = TriggerType::gpio;
    std::vector<std::string> expectedTriggerNames{};

    for (auto trigger : *(this->expectedDomainInfo_.triggers))
    {
        if (trigger != notAvailableTrigger)
        {
            ON_CALL(*this->triggersManager_, isTriggerAvailable(trigger))
                .WillByDefault(testing::Return(true));

            expectedTriggerNames.push_back(kTriggerTypeNames.at(trigger));
        }
        else
        {
            ON_CALL(*this->triggersManager_, isTriggerAvailable(trigger))
                .WillByDefault(testing::Return(false));
        }
    }

    auto [ec, response] =
        this->template dbusGetProperty<std::vector<std::string>>(
            kDomainAttributesInterface, "AvailableTriggers");
    EXPECT_EQ(ec, boost::system::errc::success);

    ASSERT_THAT(response, testing::ElementsAreArray(expectedTriggerNames));
}

TYPED_TEST(DomainTestSimple, ExpectCorrectLimitCapabilitiesMapOnDbus)
{
    boost::system::error_code ec;
    std::map<std::string, CapabilitiesValuesMap> response;

    std::string domainName = "domain";
    std::string componentName = "component";
    CapabilitiesValuesMap domainMap = {{"min", 11.0}, {"max", 330.0}};
    CapabilitiesValuesMap componentMap = {{"min", 1.0}, {"max", 420.0}};
    std::map<std::string, CapabilitiesValuesMap> expectedCapabilitiesMap;

    if (domainClassToMaxComponentsNumber.at(typeid(*(this->sut_))) ==
        kComponentIdIgnored)
    {
        expectedCapabilitiesMap = {{domainName, domainMap}};
    }
    else
    {
        expectedCapabilitiesMap = {{domainName, domainMap},
                                   {componentName, componentMap}};
    }

    ON_CALL(*this->capabilities_, getName())
        .WillByDefault(testing::Return(domainName));
    ON_CALL(*this->compCapabilities_, getName())
        .WillByDefault(testing::Return(componentName));
    ON_CALL(*this->capabilities_, getValuesMap())
        .WillByDefault(testing::Return(domainMap));
    ON_CALL(*this->compCapabilities_, getValuesMap())
        .WillByDefault(testing::Return(componentMap));

    std::tie(ec, response) = this->dbusGetAllLimitsCapabilities();
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(response, expectedCapabilitiesMap);
}

TYPED_TEST(
    DomainTestWithPolicies,
    CreatePolicyOneAfterAnotherExpectNulloptPolicyIdPassedToPolicyFactory)
{
    PolicyConfig params;

    EXPECT_CALL(*this->policyFactory_,
                createPolicy(testing::Pointee(this->expectedDomainInfo_),
                             PolicyType::power, PolicyId("0"), PolicyOwner::bmc,
                             params.statReportingPeriod(), testing::_,
                             DbusState::disabled, PolicyEditable::yes, true,
                             testing::_, testing::_))
        .WillOnce(testing::Return(this->policies_.at(0)));
    auto [ec, response] = this->dbusCreatePolicyWithId("0", params, nullptr);
    EXPECT_EQ(ec, boost::system::errc::success);

    EXPECT_CALL(*this->policyFactory_,
                createPolicy(testing::Pointee(this->expectedDomainInfo_),
                             PolicyType::power, PolicyId("1"), PolicyOwner::bmc,
                             params.statReportingPeriod(), testing::_,
                             DbusState::disabled, PolicyEditable::yes, true,
                             testing::_, testing::_))
        .WillOnce(testing::Return(this->policies_.at(1)));
    std::tie(ec, response) = this->dbusCreatePolicyWithId("1", params, nullptr);
    EXPECT_EQ(ec, boost::system::errc::success);
}

TYPED_TEST(
    DomainTestWithPolicies,
    CreatePolicyWithIdWhenNoPolicyExistsExpectPolicyFactoryCallWithCorrectId)
{
    PolicyConfig params;

    EXPECT_CALL(*this->policyFactory_,
                createPolicy(testing::Pointee(this->expectedDomainInfo_),
                             PolicyType::power, PolicyId("123"),
                             PolicyOwner::bmc, params.statReportingPeriod(),
                             testing::_, DbusState::disabled,
                             PolicyEditable::yes, true, testing::_, testing::_))
        .WillOnce(testing::Return(this->policies_.at(0)));
    auto [ec, response] =
        this->dbusCreatePolicyWithId("123", params, this->policies_.at(0));
    EXPECT_EQ(ec, boost::system::errc::success);
}

TYPED_TEST(DomainTestWithPolicies,
           CreatePolicyWithIdExpectCorrectIdReturnedOnDbus)
{
    PolicyConfig params;

    ON_CALL(*this->policyFactory_,
            createPolicy(testing::Pointee(this->expectedDomainInfo_),
                         PolicyType::power, PolicyId("1"), PolicyOwner::bmc,
                         params.statReportingPeriod(), testing::_,
                         DbusState::disabled, PolicyEditable::yes, true,
                         testing::_, testing::_))
        .WillByDefault(testing::Return(this->policies_.at(1)));
    auto [ec, response] =
        this->dbusCreatePolicyWithId("1", params, this->policies_.at(1));
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(std::filesystem::path{std::string{response}}.filename(), "1");
}

TYPED_TEST(DomainTestWithPolicies, CreatePolicExpectCorrectIdReturnedOnDbus)
{
    PolicyConfig params;

    ON_CALL(*this->policyFactory_,
            createPolicy(testing::Pointee(this->expectedDomainInfo_),
                         PolicyType::power, PolicyId("2"), PolicyOwner::bmc,
                         params.statReportingPeriod(), testing::_,
                         DbusState::disabled, PolicyEditable::yes, true,
                         testing::_, testing::_))
        .WillByDefault(testing::Return(this->policies_.at(2)));
    auto [ec, response] = this->dbusCreatePolicyWithId("2", params, nullptr);
    EXPECT_EQ(std::filesystem::path{std::string{response}}.filename(), "2");
}

TYPED_TEST(DomainTestWithPolicies,
           CreatePolicyForTotalBudgetExpectTotalBudgetPolicyOwner)
{
    PolicyConfig params;

    EXPECT_CALL(*this->policyFactory_,
                createPolicy(testing::Pointee(this->expectedDomainInfo_),
                             PolicyType::power, PolicyId("0"),
                             PolicyOwner::totalBudget,
                             params.statReportingPeriod(), testing::_,
                             DbusState::enabled, PolicyEditable::yes, true,
                             testing::_, testing::_))
        .WillOnce(testing::Return(this->policies_.at(0)));
    auto [ec, response] = this->dbusCreatePolicyForTotalBudget("0", params);
    EXPECT_EQ(ec, boost::system::errc::success);
}

TYPED_TEST(
    DomainTestWithPolicies,
    GetTriggeredPoliciesWithSameComponentIdExpectSetLimitWithLowestValueFound)
{
    this->setupPolicies({{PolicyState::ready, 10, BudgetingStrategy::aggressive,
                          kComponentIdAll},
                         {PolicyState::triggered, 20,
                          BudgetingStrategy::aggressive, kComponentIdAll},
                         {PolicyState::triggered, 30,
                          BudgetingStrategy::aggressive, kComponentIdAll}},
                        10, 30);

    this->setHostPowerOnReading(true);

    EXPECT_CALL(*(this->budgeting_),
                setLimit(this->domainId_, kComponentIdAll, 20,
                         BudgetingStrategy::aggressive));
    (this->sut_)->run();
}

TYPED_TEST(DomainTestWithPolicies,
           GetTriggeredPoliciesWithDifferentComponentIdExpectSetLimitForAll)
{
    this->setupPolicies(
        {{PolicyState::ready, 10, BudgetingStrategy::aggressive, 1},
         {PolicyState::triggered, 20, BudgetingStrategy::aggressive, 2},
         {PolicyState::triggered, 30, BudgetingStrategy::aggressive, 3}},
        10, 30);

    this->setHostPowerOnReading(true);

    EXPECT_CALL(*(this->budgeting_), setLimit(this->domainId_, 2, 20,
                                              BudgetingStrategy::aggressive));
    EXPECT_CALL(*(this->budgeting_), setLimit(this->domainId_, 3, 30,
                                              BudgetingStrategy::aggressive));
    (this->sut_)->run();
}

TYPED_TEST(DomainTestWithPolicies,
           GetTriggeredPoliciesWithDifferentStrategiesExpectSetLimitForAll)
{
    this->setupPolicies(
        {{PolicyState::triggered, 10, BudgetingStrategy::aggressive, 1},
         {PolicyState::triggered, 20, BudgetingStrategy::immediate, 2},
         {PolicyState::triggered, 30, BudgetingStrategy::nonAggressive, 3}},
        10, 30);

    this->setHostPowerOnReading(true);

    EXPECT_CALL(*(this->budgeting_), setLimit(this->domainId_, 1, 10,
                                              BudgetingStrategy::aggressive));
    EXPECT_CALL(*(this->budgeting_),
                setLimit(this->domainId_, 2, 20, BudgetingStrategy::immediate));
    EXPECT_CALL(
        *(this->budgeting_),
        setLimit(this->domainId_, 3, 30, BudgetingStrategy::nonAggressive));
    (this->sut_)->run();
}

TYPED_TEST(DomainTestWithPolicies,
           UpdateLimitsSameAsPreviousRunExpectSetLimitCall)
{
    this->setupPolicies(
        {
            {PolicyState::triggered, 10, BudgetingStrategy::aggressive, 1},
        },
        10, 30);

    this->setHostPowerOnReading(true);

    (this->sut_)->run();
    EXPECT_CALL(*(this->budgeting_),
                setLimit(testing::_, testing::_, testing::_, testing::_))
        .Times(1);
    (this->sut_)->run();
}

TYPED_TEST(DomainTestWithPolicies,
           UpdateLimitsWhenHostPowerIsOffExpectNoSetLimitCall)
{
    this->setupPolicies(
        {
            {PolicyState::triggered, 10, BudgetingStrategy::aggressive,
             kComponentIdAll},
        },
        10, 30);

    this->setHostPowerOnReading(false);

    EXPECT_CALL(*(this->budgeting_),
                setLimit(testing::_, testing::_, testing::_, testing::_))
        .Times(0);
    (this->sut_)->run();
}

TYPED_TEST(DomainTestWithPolicies, ApplyBiasExpectSetLimit)
{
    this->setupPolicies(
        {
            {PolicyState::triggered, 10, BudgetingStrategy::aggressive, 1},
        },
        0, 30);

    this->setHostPowerOnReading(true);

    (this->sut_)->run();

    this->dbusSetProperty(kDomainAttributesInterface, "LimitBiasAbsolute", 1);
    EXPECT_CALL(*(this->budgeting_), setLimit(this->domainId_, 1, 10 + 1,
                                              BudgetingStrategy::aggressive));
    (this->sut_)->run();

    this->dbusSetProperty(kDomainAttributesInterface, "LimitBiasRelative", 0.5);
    EXPECT_CALL(*(this->budgeting_),
                setLimit(this->domainId_, 1, (0.5 * 10) + 1,
                         BudgetingStrategy::aggressive));
    (this->sut_)->run();
}

TYPED_TEST(DomainTestWithPolicies,
           ApplyBiasAboveMinMaxCapabilitiesExpectSetLimitWithClampedValue)
{
    this->setupPolicies(
        {
            {PolicyState::triggered, 10, BudgetingStrategy::aggressive, 1},
        },
        10.2, 30.75);

    this->setHostPowerOnReading(true);

    this->dbusSetProperty(kDomainAttributesInterface, "LimitBiasAbsolute", 100);
    EXPECT_CALL(*(this->budgeting_), setLimit(this->domainId_, 1, 30.75,
                                              BudgetingStrategy::aggressive));
    (this->sut_)->run();

    this->dbusSetProperty(kDomainAttributesInterface, "LimitBiasRelative", 0.1);
    this->dbusSetProperty(kDomainAttributesInterface, "LimitBiasAbsolute", 0);
    EXPECT_CALL(*(this->budgeting_), setLimit(this->domainId_, 1, 10.2,
                                              BudgetingStrategy::aggressive));
    (this->sut_)->run();
}

TYPED_TEST(DomainTestWithPolicies, DeletePolicyExpectDeleteCallbackExecuted)
{
    this->setupPolicies(
        {{PolicyState::triggered, 10, BudgetingStrategy::aggressive, 1}}, 10,
        30);

    long pointerCounter = (this->policies_.at(0)).use_count();
    EXPECT_THAT(pointerCounter, testing::Gt(0));
    this->lastSavedDeleteCallback_(PolicyId("0"));
    EXPECT_EQ((this->policies_.at(0)).use_count(), pointerCounter - 1);
}

TYPED_TEST(DomainTestWithPolicies,
           GetSelectedPolicyNoPolicyCreatedExpectFalseActiveLimitFlag)
{
    auto [ec, policyId] = this->dbusGetSelectedPolicyId();
    EXPECT_EQ(ec, boost::system::errc::success);
}

TYPED_TEST(DomainTestWithPolicies,
           GetSelectedInactivePolicyExpectFalseActiveFlagLimit)
{
    this->setupPolicies(
        {{PolicyState::ready, 10, BudgetingStrategy::aggressive, 255}}, 10, 30);
    auto [ec, response] = this->dbusGetSelectedPolicyId();
    auto [policyId] = response;
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(std::filesystem::path{std::string{policyId}}.has_filename(),
              false);
}

TYPED_TEST(DomainTestWithPolicies,
           GetSelectedActivePolicyExpectTrueActiveFlagLimitAndCorrectPolicyId)
{
    this->setupPolicies(
        {{PolicyState::ready, 10, BudgetingStrategy::aggressive, 255},
         {PolicyState::triggered, 10, BudgetingStrategy::aggressive, 255},
         {PolicyState::selected, 30, BudgetingStrategy::aggressive, 255}},
        10, 30);
    auto [ec, response] = this->dbusGetSelectedPolicyId();
    auto [policyPath] = response;
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_EQ(std::filesystem::path{std::string{policyPath}}.filename(), "2");
}

TYPED_TEST(DomainTestWithPolicies, TwoPoliciesSelectedExpecNoErrorInLogger)
{
    this->setupPolicies(
        {{PolicyState::selected, 10, BudgetingStrategy::aggressive, 255},
         {PolicyState::selected, 20, BudgetingStrategy::aggressive, 255}},
        10, 30);
    this->setHostPowerOnReading(true);
    EXPECT_NO_THROW((this->sut_)->run());
    EXPECT_NO_THROW((this->sut_)->postRun());
}

TYPED_TEST(
    DomainTestWithPolicies,
    MatchPolicyWithSelectedLimitPolicyPerComponentExpectSetLimitSelectedTrue)
{
    this->setupPolicies(
        {{PolicyState::selected, 10, BudgetingStrategy::aggressive, 255},
         {PolicyState::triggered, 10, BudgetingStrategy::aggressive, 0},
         {PolicyState::selected, 30, BudgetingStrategy::aggressive, 2}},
        10, 30);

    ON_CALL(*this->budgeting_, isActive(this->domainId_, testing::_,
                                        BudgetingStrategy::aggressive))
        .WillByDefault(testing::Return(true));

    EXPECT_CALL(*this->policies_.at(0), setLimitSelected(true));
    EXPECT_CALL(*this->policies_.at(1), setLimitSelected(true));
    EXPECT_CALL(*this->policies_.at(2), setLimitSelected(true));

    this->setHostPowerOnReading(true);
    (this->sut_)->run();
    (this->sut_)->postRun();
}

TYPED_TEST(
    DomainTestWithPolicies,
    MatchPolicyWithSelectedLimitComponentIdAllPolicyNoMoreUsedToLimitExpectSetLimitSelectedFalse)
{
    this->setupPolicies(
        {{PolicyState::triggered, 10, BudgetingStrategy::aggressive, 255},
         {PolicyState::triggered, 20, BudgetingStrategy::aggressive, 255}},
        10, 30);

    ON_CALL(*this->budgeting_, isActive(this->domainId_, kComponentIdAll,
                                        BudgetingStrategy::aggressive))
        .WillByDefault(testing::Return(true));
    this->setHostPowerOnReading(true);
    (this->sut_)->run();
    (this->sut_)->postRun();

    ON_CALL(*this->policies_.at(0), getLimit())
        .WillByDefault(testing::Return(30));
    EXPECT_CALL(*this->policies_.at(0), setLimitSelected(false));
    EXPECT_CALL(*this->policies_.at(1), setLimitSelected(true));
    (this->sut_)->run();
    (this->sut_)->postRun();
}

TYPED_TEST(
    DomainTestWithPolicies,
    PolicyWithLowerLimitTriggeredExpectCorrectStateTransitionsInCurrentlyLimitingPolicyAndTheNewOne)
{
    this->setHostPowerOnReading(true);

    this->setupPolicies(
        {{PolicyState::ready, 540, BudgetingStrategy::aggressive, 255},
         {PolicyState::ready, 0, BudgetingStrategy::aggressive, 255},
         {PolicyState::triggered, 270, BudgetingStrategy::aggressive, 255}},
        0, 600);

    (this->sut_)->run();
    ON_CALL(*this->budgeting_, isActive(this->domainId_, kComponentIdAll,
                                        BudgetingStrategy::aggressive))
        .WillByDefault(testing::Return(true));
    EXPECT_CALL(*this->policies_.at(2), setLimitSelected(true));
    (this->sut_)->postRun();

    this->setupPolicies(
        {{PolicyState::ready, 540, BudgetingStrategy::aggressive, 255},
         {PolicyState::triggered, 0, BudgetingStrategy::aggressive, 255},
         {PolicyState::selected, 270, BudgetingStrategy::aggressive, 255}},
        0, 600);

    EXPECT_CALL(*this->policies_.at(1), setLimitSelected(true));
    EXPECT_CALL(*this->policies_.at(2), setLimitSelected(false));
    (this->sut_)->run();
    (this->sut_)->postRun();
}

TYPED_TEST(
    DomainTestWithPolicies,
    SelectedPolicyAgainChosenToLimitExpectNoStateTransitionFromSelectedState)
{
    this->setHostPowerOnReading(true);

    this->setupPolicies(
        {{PolicyState::triggered, 0, BudgetingStrategy::aggressive, 255}}, 0,
        600);

    (this->sut_)->run();
    ON_CALL(*this->budgeting_, isActive(this->domainId_, kComponentIdAll,
                                        BudgetingStrategy::aggressive))
        .WillByDefault(testing::Return(true));
    EXPECT_CALL(*this->policies_.at(0), setLimitSelected(true))
        .RetiresOnSaturation();
    (this->sut_)->postRun();

    this->setupPolicies(
        {{PolicyState::selected, 0, BudgetingStrategy::aggressive, 255}}, 0,
        600);

    EXPECT_CALL(*this->policies_.at(0), setLimitSelected(true))
        .Times(testing::AnyNumber());
    EXPECT_CALL(*this->policies_.at(0), setLimitSelected(false)).Times(0);
    (this->sut_)->run();
    (this->sut_)->postRun();
}

TYPED_TEST(DomainTestWithPolicies,
           ControlledParameterAvailabilityChangeExpectPoliciesToValidateParams)
{
    this->setupPolicies(
        {
            {PolicyState::disabled, 10, BudgetingStrategy::aggressive, 1},
            {PolicyState::triggered, 10, BudgetingStrategy::aggressive, 1},
        },
        10, 30);

    EXPECT_CALL(*this->policies_.at(0), validateParameters());
    EXPECT_CALL(*this->policies_.at(1), validateParameters());
    this->requiredReadingsEventsHandler_->reportEvent(
        ReadingEventType::readingUnavailable,
        {domainClassToControlledParameter.at(typeid(*(this->sut_))),
         kAllDevices});
}

template <typename T>
class WithCpuComponentDomainsTest : public DomainTestWithPolicies<T>
{
};

typedef testing::Types<DomainCpuSubsystem, DomainMemorySubsystem>
    WithCpuComponentImplementations;
TYPED_TEST_SUITE(WithCpuComponentDomainsTest, WithCpuComponentImplementations);

TYPED_TEST(WithCpuComponentDomainsTest,
           AvailableComponentChangeExpectPoliciesToValidateParams)
{
    this->setupPolicies(
        {
            {PolicyState::disabled, 10, BudgetingStrategy::aggressive, 1},
            {PolicyState::triggered, 10, BudgetingStrategy::aggressive, 1},
        },
        10, 30);

    EXPECT_CALL(*this->policies_.at(0), validateParameters());
    EXPECT_CALL(*this->policies_.at(1), validateParameters());
    this->setAvailableComponents(2.0);
}

TYPED_TEST(WithCpuComponentDomainsTest,
           AvailableComponentSetTwiceExpectPoliciesToValidateParamsOnce)
{
    this->setupPolicies(
        {
            {PolicyState::disabled, 10, BudgetingStrategy::aggressive, 1},
            {PolicyState::triggered, 10, BudgetingStrategy::aggressive, 1},
        },
        10, 30);

    EXPECT_CALL(*this->policies_.at(0), validateParameters());
    EXPECT_CALL(*this->policies_.at(1), validateParameters());
    this->setAvailableComponents(2.0);
    this->setAvailableComponents(2.0);
}

} // namespace nodemanager
