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

#include "../nm-ipmi-lib/include/nm_dbus_errors.hpp"
#include "common_types.hpp"
#include "domains/domain_types.hpp"
#include "mocks/devices_manager_mock.hpp"
#include "mocks/gpio_provider_mock.hpp"
#include "mocks/policy_storage_management_mock.hpp"
#include "mocks/triggers_manager_mock.hpp"
#include "policies/policy.hpp"
#include "policies/policy_enums.hpp"
#include "policies/policy_types.hpp"
#include "utility/dbus_errors.hpp"
#include "utils/dbus_environment.hpp"
#include "utils/policy_config.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace nodemanager;
using namespace nmipmi;

class PolicyTestable : public Policy
{
  public:
    PolicyTestable(PolicyId id, PolicyOwner owner,
                   std::shared_ptr<DevicesManagerIf> devicesManager,
                   std::shared_ptr<GpioProviderIf> gpioProvider,
                   std::shared_ptr<TriggersManagerIf> triggersManager,
                   std::shared_ptr<PolicyStorageManagementIf> storageManagement,
                   uint16_t statReportingPeriod,
                   std::shared_ptr<sdbusplus::asio::connection> bus,
                   std::shared_ptr<sdbusplus::asio::object_server> objectServer,
                   std::shared_ptr<DomainInfo>& domainInfo,
                   const DeleteCallback deleteCallback, DbusState dbusState,
                   PolicyEditable editable, bool allowDelete) :
        Policy(id, owner, devicesManager, gpioProvider, triggersManager,
               storageManagement, statReportingPeriod, bus, objectServer,
               domainInfo, deleteCallback, dbusState, editable, allowDelete)
    {
    }

  private:
    void verifyLimit(uint16_t limitArg, DeviceIndex componentIdArg,
                     TriggerType triggerTypeArg) const override
    {
    }

    virtual BudgetingStrategy getStrategy() const override
    {
        return BudgetingStrategy::nonAggressive;
    }
};

template <class PolicyType>
class PolicyTest : public ::testing::Test
{
  public:
    const std::string kDomainObjectPath =
        "/xyz/openbmc_project/NodeManager/Domain/0";
    const std::string kPolicyObjectPath =
        "/xyz/openbmc_project/NodeManager/Domain/0/Policy/";
    const std::string kPolicyId = "TestPolicy";
    const std::string kPolicyAttributesIface =
        "xyz.openbmc_project.NodeManager.PolicyAttributes";
    const std::string kPolicyEnableIface = "xyz.openbmc_project.Object.Enable";
    const std::string kObjectDeleteIface = "xyz.openbmc_project.Object.Delete";
    const double kDomainCapabilitiesMax = 32767.0;
    const double kDomainCapabilitiesMin = 2.0;

    PolicyTest()
    {
        static_assert(
            std::is_base_of<Policy, PolicyType>::value,
            "This test template can only be used with Policy class or its "
            "derived class.");
    }
    virtual ~PolicyTest() = default;

    virtual void SetUp() override
    {
        PolicyConfig policyConfig_;
        policyConfig_.limit(static_cast<uint16_t>(kDomainCapabilitiesMin));

        domainInfo_->objectPath = kDomainObjectPath;
        domainInfo_->domainId = DomainId::AcTotalPower;
        domainInfo_->capabilities = std::make_shared<DomainCapabilities>(
            ReadingType::acPlatformPower, ReadingType::acPlatformPower,
            devicesManager_, 6000, nullptr, DomainId::AcTotalPower);
        domainInfo_->controlledParameter = ReadingType::acPlatformPower;
        domainInfo_->availableComponents =
            std::make_shared<std::vector<DeviceIndex>>();
        domainInfo_->triggers = _triggerTypes;
        domainInfo_->maxComponentNumber = kComponentIdIgnored;

        domainInfo_->capabilities->setMax(kDomainCapabilitiesMax);
        domainInfo_->capabilities->setMin(kDomainCapabilitiesMin);

        sd_bus_error_add_map(nmipmi::kNodeManagerDBusErrors);

        if constexpr (std::is_same_v<PolicyType, PowerPolicy>)
        {
            sut_ = std::make_shared<PolicyType>(
                kPolicyId, PolicyOwner::bmc, devicesManager_, gpioProvider_,
                triggersManager_, policyStorageManagement_, 0,
                DbusEnvironment::getBus(), DbusEnvironment::getObjServer(),
                domainInfo_, callback_.AsStdFunction(), DbusState::disabled,
                PolicyEditable::yes, true, componentCapabilitiesVector_);
        }
        else
        {
            sut_ = std::make_shared<PolicyType>(
                kPolicyId, PolicyOwner::bmc, devicesManager_, gpioProvider_,
                triggersManager_, policyStorageManagement_, 0,
                DbusEnvironment::getBus(), DbusEnvironment::getObjServer(),
                domainInfo_, callback_.AsStdFunction(), DbusState::disabled,
                PolicyEditable::yes, true);
        }

        ON_CALL(*this->triggersManager_,
                isTriggerAvailable(policyConfig_.triggerType()))
            .WillByDefault(testing::Return(true));

        sut_->initialize();
        sut_->verifyParameters(policyConfig_._getStruct());
        sut_->updateParams(policyConfig_._getStruct());
    }

    std::shared_ptr<std::vector<TriggerType>> _triggerTypes =
        std::make_shared<std::vector<TriggerType>>(std::vector<TriggerType>{
            TriggerType::always, TriggerType::inletTemperature,
            TriggerType::missingReadingsTimeout,
            TriggerType::timeAfterHostReset, TriggerType::gpio,
            TriggerType::cpuUtilization, TriggerType::hostReset});
    std::shared_ptr<DevicesManagerMock> devicesManager_ =
        std::make_shared<testing::NiceMock<DevicesManagerMock>>();
    std::shared_ptr<GpioProviderMock> gpioProvider_ =
        std::make_shared<testing::NiceMock<GpioProviderMock>>();
    std::shared_ptr<TriggersManagerMock> triggersManager_ =
        std::make_shared<testing::NiceMock<TriggersManagerMock>>();
    std::shared_ptr<PolicyStorageManagementMock> policyStorageManagement_ =
        std::make_shared<testing::NiceMock<PolicyStorageManagementMock>>();
    std::shared_ptr<DomainInfo> domainInfo_ = std::make_shared<DomainInfo>();
    std::vector<std::shared_ptr<ComponentCapabilitiesIf>>
        componentCapabilitiesVector_;

    testing::MockFunction<void(const PolicyId policyId)> callback_;

    std::shared_ptr<PolicyType> sut_;
};

template <class PolicyType>
class PolicyPropertyTest : public PolicyTest<PolicyType>
{
  public:
    PolicyPropertyTest(std::string propNameArg)
    {
        testedPropertyName = propNameArg;
    }

    template <class PropertyType>
    void setAttributeTestBody(PropertyType propertyValue, int expectedErrorCode)
    {
        auto response = DbusEnvironment::setProperty(
            PolicyTest<PolicyType>::kPolicyAttributesIface,
            PolicyTest<PolicyType>::kPolicyObjectPath +
                PolicyTest<PolicyType>::kPolicyId,
            testedPropertyName, propertyValue);

        EXPECT_EQ(response.value(), expectedErrorCode);

        if (expectedErrorCode == 0)
        {
            auto [ec, attribute] = DbusEnvironment::getProperty<
                utility::simple_type_t<PropertyType>>(
                PolicyTest<PolicyType>::kPolicyAttributesIface,
                PolicyTest<PolicyType>::kPolicyObjectPath +
                    PolicyTest<PolicyType>::kPolicyId,
                testedPropertyName);

            EXPECT_EQ(ec.value(), 0);
            EXPECT_EQ(attribute, propertyValue);
        }
    }

    std::string testedPropertyName;
};

using PolicyStorageParams =
    std::tuple<std::underlying_type_t<PolicyStorage>, int>;

class PolicyPropertyStorageTest : public PolicyPropertyTest<PolicyTestable>,
                                  public WithParamInterface<PolicyStorageParams>
{
  public:
    PolicyPropertyStorageTest() : PolicyPropertyTest("PolicyStorage")
    {
    }
};

INSTANTIATE_TEST_SUITE_P(
    PolicyStorageInvalid, PolicyPropertyStorageTest,
    Values(PolicyStorageParams{2,
                               static_cast<std::underlying_type_t<ErrorCodes>>(
                                   ErrorCodes::InvalidPolicyStorage)},
           PolicyStorageParams{100,
                               static_cast<std::underlying_type_t<ErrorCodes>>(
                                   ErrorCodes::InvalidPolicyStorage)}));

INSTANTIATE_TEST_SUITE_P(
    PolicyStorageValid, PolicyPropertyStorageTest,
    Values(
        PolicyStorageParams{static_cast<std::underlying_type_t<PolicyStorage>>(
                                PolicyStorage::persistentStorage),
                            0},
        PolicyStorageParams{static_cast<std::underlying_type_t<PolicyStorage>>(
                                PolicyStorage::volatileStorage),
                            0}));

TEST_P(PolicyPropertyStorageTest, SetAttribute)
{
    InSequence seq;
    auto [propertyValue, expectedErrorCode] = GetParam();
    setAttributeTestBody(propertyValue, expectedErrorCode);
}

class PolicyTestSimple : public PolicyTest<PolicyTestable>
{
};

TEST_F(PolicyTestSimple,
       VerifyParametersThrowsInvalidComponentIdWhenComponentIdAboveMax)
{
    domainInfo_->maxComponentNumber = 1;
    domainInfo_->availableComponents->clear();
    PolicyConfig policyConfig;
    policyConfig.limit(static_cast<uint16_t>(kDomainCapabilitiesMin))
        .componentId(2);
    EXPECT_THROW(sut_->verifyParameters(policyConfig._getStruct()),
                 errors::InvalidComponentId);
}

TEST_F(
    PolicyTestSimple,
    VerifyParametersThrowsInvalidComponentIdWhenComponentIdBeyondAvailableComponents)
{
    domainInfo_->maxComponentNumber = 2;
    domainInfo_->availableComponents->push_back(0);
    domainInfo_->availableComponents->push_back(1);
    PolicyConfig policyConfig;
    policyConfig.limit(static_cast<uint16_t>(kDomainCapabilitiesMin))
        .componentId(2);
    EXPECT_THROW(sut_->verifyParameters(policyConfig._getStruct()),
                 errors::InvalidComponentId);
}

TEST_F(
    PolicyTestSimple,
    VerifyParametersThrowsInvalidComponentIdWhenComponentNotFromAvailableComponents)
{
    domainInfo_->maxComponentNumber = 5;
    domainInfo_->availableComponents->push_back(0);
    domainInfo_->availableComponents->push_back(4);
    PolicyConfig policyConfig;
    policyConfig.limit(static_cast<uint16_t>(kDomainCapabilitiesMin))
        .componentId(2);
    EXPECT_THROW(sut_->verifyParameters(policyConfig._getStruct()),
                 errors::InvalidComponentId);
}

TEST_F(PolicyTestSimple, VerifyParametersNoExceptionWhenInRange)
{
    domainInfo_->maxComponentNumber = 5;
    domainInfo_->availableComponents->push_back(0);
    domainInfo_->availableComponents->push_back(4);
    PolicyConfig policyConfig;
    policyConfig.limit(static_cast<uint16_t>(kDomainCapabilitiesMin))
        .componentId(4);
    EXPECT_NO_THROW(sut_->verifyParameters(policyConfig._getStruct()));
}

TEST_F(PolicyTestSimple,
       VerifyParametersThrowsInvalidTriggerTypeWhenTriggerNotFromDomainTriggers)
{
    *_triggerTypes = {TriggerType::always};
    PolicyConfig policyConfig;
    policyConfig.triggerType(TriggerType::gpio);
    EXPECT_THROW(sut_->verifyParameters(policyConfig._getStruct()),
                 errors::UnsupportedPolicyTriggerType);
}

TEST_F(PolicyTestSimple,
       VerifyParametersThrowsInvalidTriggerTypeWhenTriggerNotAvailable)
{
    const TriggerType notAvailableTrigger{TriggerType::gpio};

    *_triggerTypes = {notAvailableTrigger};
    PolicyConfig policyConfig;
    policyConfig.triggerType(notAvailableTrigger);

    ON_CALL(*this->triggersManager_, isTriggerAvailable(notAvailableTrigger))
        .WillByDefault(testing::Return(false));

    EXPECT_THROW(sut_->verifyParameters(policyConfig._getStruct()),
                 errors::UnsupportedPolicyTriggerType);
}

TEST_F(PolicyTestSimple, ValidateParametersUnsuspendsPolicyWhenParametersValid)
{
    DbusEnvironment::setProperty(
        kPolicyEnableIface, kPolicyObjectPath + kPolicyId, "Enabled", true);
    domainInfo_->maxComponentNumber = 5;
    domainInfo_->availableComponents->push_back(0);
    domainInfo_->availableComponents->push_back(4);
    PolicyConfig policyConfig;
    policyConfig.limit(static_cast<uint16_t>(kDomainCapabilitiesMin))
        .componentId(2);
    sut_->updateParams(policyConfig._getStruct());
    sut_->validateParameters();
    EXPECT_EQ(sut_->getState(), PolicyState::suspended);
    policyConfig.componentId(4);
    sut_->updateParams(policyConfig._getStruct());
    sut_->validateParameters();
    EXPECT_NE(sut_->getState(), PolicyState::suspended);
}

TEST_F(PolicyTestSimple, UpdateTriggerWhenTriggerAlwaysOn)
{
    PolicyConfig policyConfig = PolicyConfig().triggerType(TriggerType::always);
    EXPECT_CALL(*devicesManager_, registerReadingConsumerHelper(
                                      testing::_, testing::_, testing::_))
        .Times(0);

    sut_->updateParams(policyConfig._getStruct());
    sut_->uninstallTrigger();
    sut_->installTrigger();
}

TEST_F(PolicyTestSimple, UpdateTriggerWhenDomainCpuAndTriggerCpuUtilization)
{
    PolicyConfig policyConfig =
        PolicyConfig().componentId(1).triggerType(TriggerType::cpuUtilization);
    domainInfo_->domainId = DomainId::CpuSubsystem;
    EXPECT_CALL(*devicesManager_, registerReadingConsumerHelper(
                                      testing::_, ReadingType::cpuUtilization,
                                      policyConfig.componentId()));

    sut_->updateParams(policyConfig._getStruct());
    sut_->uninstallTrigger();
    sut_->installTrigger();
}

TEST_F(PolicyTestSimple, UpdateTriggerWhenTriggerGpio)
{
    PolicyConfig policyConfig =
        PolicyConfig().triggerLimit(5).triggerType(TriggerType::gpio);
    EXPECT_CALL(*devicesManager_, registerReadingConsumerHelper(
                                      testing::_, ReadingType::gpioState,
                                      policyConfig.triggerLimit()));

    sut_->updateParams(policyConfig._getStruct());
    sut_->uninstallTrigger();
    sut_->installTrigger();
}

TEST_F(PolicyTestSimple, UpdateTriggerWhenTriggerOtherThanCpuAndGpio)
{
    PolicyConfig policyConfig = PolicyConfig().componentId(0).triggerType(
        TriggerType::inletTemperature);
    EXPECT_CALL(*devicesManager_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::inletTemperature, kAllDevices));

    sut_->updateParams(policyConfig._getStruct());
    sut_->uninstallTrigger();
    sut_->installTrigger();
}

class PolicyTestStorePolicy : public PolicyTest<PolicyTestable>
{
  public:
    void setPolicyStorage(PolicyStorage storageOption)
    {
        PolicyConfig policyConfig =
            PolicyConfig()
                .limit(static_cast<uint16_t>(kDomainCapabilitiesMin))
                .policyStorage(storageOption);
        sut_->verifyParameters(policyConfig._getStruct());
        sut_->updateParams(policyConfig._getStruct());
    }
};

TEST_F(PolicyTestStorePolicy, VolatileStorageDoNotStorePolicyDeletePolicyFile)
{
    setPolicyStorage(PolicyStorage::volatileStorage);
    EXPECT_CALL(*policyStorageManagement_, policyWrite(testing::_, testing::_))
        .Times(0);
    EXPECT_CALL(*policyStorageManagement_, policyDelete(kPolicyId));
    sut_->postCreate();
}

TEST_F(PolicyTestStorePolicy, PersistentStorageStorePolicy)
{
    setPolicyStorage(PolicyStorage::persistentStorage);
    EXPECT_CALL(*policyStorageManagement_, policyWrite(kPolicyId, testing::_))
        .Times(1);
    sut_->postCreate();
}

TEST_F(PolicyTestStorePolicy, SetStoragePersistentStorePolicy)
{
    setPolicyStorage(PolicyStorage::volatileStorage);
    EXPECT_CALL(*policyStorageManagement_, policyWrite(kPolicyId, testing::_))
        .Times(1);
    DbusEnvironment::setProperty(
        kPolicyAttributesIface, kPolicyObjectPath + kPolicyId, "PolicyStorage",
        static_cast<std::underlying_type_t<PolicyStorage>>(
            PolicyStorage::persistentStorage));
}

TEST_F(PolicyTestStorePolicy,
       SetStorageVolatileDoNotStorePolicyDeletePolicyFile)
{
    setPolicyStorage(PolicyStorage::persistentStorage);
    EXPECT_CALL(*policyStorageManagement_, policyWrite(testing::_, testing::_))
        .Times(0);
    EXPECT_CALL(*policyStorageManagement_, policyDelete(kPolicyId));

    DbusEnvironment::setProperty(
        kPolicyAttributesIface, kPolicyObjectPath + kPolicyId, "PolicyStorage",
        static_cast<std::underlying_type_t<PolicyStorage>>(
            PolicyStorage::volatileStorage));
}

TEST_F(PolicyTestStorePolicy, SetAttributeStorePolicy)
{
    setPolicyStorage(PolicyStorage::persistentStorage);
    InSequence seq;
    EXPECT_CALL(*policyStorageManagement_, policyWrite(kPolicyId, testing::_))
        .Times(1);

    DbusEnvironment::setProperty(
        kPolicyAttributesIface, kPolicyObjectPath + kPolicyId, "Limit",
        (static_cast<uint16_t>(kDomainCapabilitiesMin + 1)));
}

TEST_F(PolicyTestStorePolicy, SetAttributeSameValueDoNotStorePolicy)
{
    setPolicyStorage(PolicyStorage::persistentStorage);
    EXPECT_CALL(*policyStorageManagement_, policyWrite(testing::_, testing::_))
        .Times(0);

    DbusEnvironment::setProperty(kPolicyAttributesIface,
                                 kPolicyObjectPath + kPolicyId, "ComponentId",
                                 PolicyConfig().componentId());
}

TEST_F(PolicyTestStorePolicy, DeletePolicyDeletePolicyFile)
{
    setPolicyStorage(PolicyStorage::persistentStorage);
    EXPECT_CALL(*policyStorageManagement_, policyDelete(kPolicyId));

    std::promise<
        std::tuple<boost::system::error_code, sdbusplus::message::object_path>>
        promise;
    DbusEnvironment::getBus()->async_method_call(
        [&promise](boost::system::error_code ec,
                   const sdbusplus::message::object_path& path) {
            promise.set_value(std::make_tuple(ec, path));
        },
        DbusEnvironment::serviceName(), kPolicyObjectPath + kPolicyId,
        kObjectDeleteIface, "Delete");

    DbusEnvironment::waitForFuture(promise.get_future());
}
