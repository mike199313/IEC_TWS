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

#include "mocks/devices_manager_mock.hpp"
#include "mocks/gpio_provider_mock.hpp"
#include "mocks/policy_storage_management_mock.hpp"
#include "mocks/triggers_manager_mock.hpp"
#include "utils/dbus_environment.hpp"

#include <policies/policy_factory.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace nodemanager;

class PolicyFactoryTest : public ::testing::Test
{
  protected:
    PolicyFactoryTest() = default;
    virtual ~PolicyFactoryTest() = default;

    virtual void SetUp() override
    {
        sut_ = std::make_shared<PolicyFactory>(
            DbusEnvironment::getBus(), DbusEnvironment::getObjServer(),
            triggersManager_, devManMock_, gpioProviderMock_,
            policyStorageManagement_);
    }

    std::shared_ptr<PolicyIf> createPolicy(const PolicyId& id,
                                           PolicyOwner owner = PolicyOwner::bmc)
    {
        return sut_->createPolicy(domainInfoSp, PolicyType::power, id, owner,
                                  5000, deleteCallback_, DbusState::disabled,
                                  PolicyEditable::yes, true, nullptr,
                                  componentCapabilitiesVector_);
    }

    std::shared_ptr<PolicyFactoryIf> sut_;
    DeleteCallback deleteCallback_ = [](const PolicyId policyId) {};
    std::shared_ptr<DevicesManagerMock> devManMock_ =
        std::make_shared<testing::NiceMock<DevicesManagerMock>>();
    std::shared_ptr<GpioProviderMock> gpioProviderMock_ =
        std::make_shared<testing::NiceMock<GpioProviderMock>>();
    std::shared_ptr<TriggersManagerMock> triggersManager_ =
        std::make_shared<testing::NiceMock<TriggersManagerMock>>();
    std::shared_ptr<PolicyStorageManagementMock> policyStorageManagement_ =
        std::make_shared<testing::NiceMock<PolicyStorageManagementMock>>();
    std::shared_ptr<DomainCapabilitiesIf> capabilities_ =
        std::make_shared<DomainCapabilities>(
            ReadingType::acPlatformPower, ReadingType::acPlatformPower,
            devManMock_, 0, nullptr, DomainId::AcTotalPower);
    std::vector<std::shared_ptr<ComponentCapabilitiesIf>>
        componentCapabilitiesVector_;
    DomainId domainId_ = DomainId::CpuSubsystem;
    DomainInfo expectedDomainInfo_ = {
        "/xyz/openbmc_project/NodeManager/Domain/" +
            std::to_string(static_cast<uint8_t>(domainId_)),
        ReadingType::cpuPackagePower,
        capabilities_,
        domainId_,
        std::make_shared<std::vector<DeviceIndex>>(),
        false,
        std::make_shared<std::vector<TriggerType>>(),
        kComponentIdIgnored};
    std::shared_ptr<DomainInfo> domainInfoSp =
        std::make_shared<DomainInfo>(expectedDomainInfo_);
};

TEST_F(PolicyFactoryTest, CreateDifferentPoliciesWithValidIdNoExceptionExpected)
{
    std::vector<std::shared_ptr<PolicyIf>> temp;
    EXPECT_NO_THROW(temp.push_back(createPolicy(PolicyId{"12323123"})));
    EXPECT_NO_THROW(temp.push_back(createPolicy(PolicyId{"_"})));
    EXPECT_NO_THROW(temp.push_back(createPolicy(PolicyId{"abrakadabra"})));
    EXPECT_NO_THROW(temp.push_back(createPolicy(PolicyId{"abrakadabra_"})));
    EXPECT_NO_THROW(temp.push_back(createPolicy(PolicyId{"_abrakadabra"})));
    EXPECT_NO_THROW(temp.push_back(createPolicy(PolicyId{"_abrakadabra_"})));
    EXPECT_NO_THROW(temp.push_back(createPolicy(PolicyId{"_abra_123_bra"})));
}

TEST_F(PolicyFactoryTest, PolicyIdHaveExtraCharsThrowsException)
{
    std::string extraChars = {" /\\!@#$%^&*()-+={}[]?><,.|`~"};
    std::for_each(extraChars.cbegin(), extraChars.cend(),
                  [this](const char& c) {
                      PolicyId id = "Policy_123_";
                      id += c;
                      EXPECT_THROW(createPolicy(id), errors::InvalidPolicyId);
                  });
}

TEST_F(PolicyFactoryTest, PolicyIdCannotDuplicate)
{
    std::vector<std::shared_ptr<PolicyIf>> policies;
    EXPECT_NO_THROW(policies.push_back(createPolicy(PolicyId{"5"})));
    EXPECT_THROW(policies.push_back(createPolicy(PolicyId{"5"})),
                 errors::PoliciesCannotBeCreated);
}

TEST_F(PolicyFactoryTest,
       PolicyFactoryAllowsToReuseIdWhenPreviousdPolicyWasDestroyed)
{
    auto p1 = createPolicy(PolicyId{"a"});
    p1 = nullptr;
    EXPECT_NO_THROW(createPolicy(PolicyId{"a"}));
}

TEST_F(
    PolicyFactoryTest,
    CreatePolicyThrowsExceptionWhenMaxPolicyLimitIsReachedForNotInternalPolicies)
{
    std::vector<std::shared_ptr<PolicyIf>> policies;
    for (uint8_t idx = 0; idx < kNodeManagerMaxPolicies; idx++)
    {
        EXPECT_NO_THROW(policies.push_back(
            createPolicy(PolicyId{"Policy_" + std::to_string(unsigned{idx})},
                         PolicyOwner::bmc)));
    }
    EXPECT_THROW(policies.push_back(createPolicy(
                     PolicyId{"Policy_" + std::to_string(unsigned{
                                              kNodeManagerMaxPolicies})},
                     PolicyOwner::bmc)),
                 errors::PoliciesCannotBeCreated);
}

TEST_F(
    PolicyFactoryTest,
    CreatePolicyAllowsToCreateInternalPoliciesEvenWhenMaxPolicyLimitIsReachedForExternalPolicies)
{
    std::vector<std::shared_ptr<PolicyIf>> policies;
    for (uint8_t idx = 0; idx < kNodeManagerMaxPolicies; idx++)
    {
        EXPECT_NO_THROW(policies.push_back(
            createPolicy(PolicyId{"Policy_" + std::to_string(unsigned{idx})},
                         PolicyOwner::bmc)));
    }
    EXPECT_NO_THROW(createPolicy(
        PolicyId{std::to_string(unsigned{kNodeManagerMaxPolicies})},
        PolicyOwner::internal));
    EXPECT_NO_THROW(createPolicy(
        PolicyId{std::to_string(unsigned{kNodeManagerMaxPolicies})},
        PolicyOwner::ospm));
    EXPECT_NO_THROW(createPolicy(
        PolicyId{std::to_string(unsigned{kNodeManagerMaxPolicies})},
        PolicyOwner::totalBudget));
}
