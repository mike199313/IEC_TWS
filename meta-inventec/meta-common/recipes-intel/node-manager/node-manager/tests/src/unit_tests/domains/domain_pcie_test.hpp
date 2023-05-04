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
#include "domain_test.hpp"
#include "domains/domain_ac_total_power.hpp"
#include "domains/domain_cpu_subsystem.hpp"
#include "domains/domain_dc_total_power.hpp"
#include "domains/domain_memory_subsystem.hpp"
#include "domains/domain_pcie.hpp"
#include "domains/domain_power.hpp"
#include "domains/domain_types.hpp"
#include "mocks/budgeting_mock.hpp"
#include "mocks/capabilities_factory_mock.hpp"
#include "mocks/devices_manager_mock.hpp"
#include "mocks/policy_factory_mock.hpp"
#include "mocks/policy_mock.hpp"
#include "mocks/triggers_manager_mock.hpp"
#include "policies/policy.hpp"
#include "readings/reading_type.hpp"
#include "utils/dbus_environment.hpp"
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

class DomainPcieTest : public DomainTestWithPolicies<DomainPcie>
{
  public:
    virtual ~DomainPcieTest() = default;

    virtual void SetUp() override
    {
        ON_CALL(*this->devicesManager_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::pciePresence, kAllDevices))
            .WillByDefault(
                testing::SaveArg<0>(&(this->peciPresenceEventHandler_)));
        DomainTestWithPolicies::SetUp();
    }

    void setPeciPresenceOnReading(std::bitset<kMaxPcieNumber> pcieBitSet)
    {
        assert(peciPresenceEventHandler_ != nullptr);
        peciPresenceEventHandler_->updateValue(
            static_cast<double>(pcieBitSet.to_ulong()));
    }

    std::shared_ptr<ReadingConsumer> peciPresenceEventHandler_ = nullptr;
};

TEST_F(DomainPcieTest, AvailableComponentsAreEmpty)
{
    auto [ec, components] = dbusGetProperty<std::vector<DeviceIndex>>(
        kDomainAttributesInterface, "AvailableComponents");
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_THAT(components, testing::ElementsAre());
}

TEST_F(DomainPcieTest, AvailableComponentsWithThreeElements)
{
    setPeciPresenceOnReading(std::bitset<kMaxPcieNumber>(21));

    auto [ec, components] = dbusGetProperty<std::vector<DeviceIndex>>(
        kDomainAttributesInterface, "AvailableComponents");
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_THAT(components, testing::ElementsAre(0, 2, 4));
}

TEST_F(DomainPcieTest, AvailableComponentsDecreasedOnPcieRemoving)
{
    setPeciPresenceOnReading(std::bitset<kMaxPcieNumber>(21));

    auto [ec, components] = dbusGetProperty<std::vector<DeviceIndex>>(
        kDomainAttributesInterface, "AvailableComponents");
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_THAT(components, testing::ElementsAre(0, 2, 4));

    setPeciPresenceOnReading(std::bitset<kMaxPcieNumber>(5));
    auto [ec2, components2] = dbusGetProperty<std::vector<DeviceIndex>>(
        kDomainAttributesInterface, "AvailableComponents");
    EXPECT_EQ(ec2, boost::system::errc::success);
    EXPECT_THAT(components2, testing::ElementsAre(0, 2));
}

TEST_F(DomainPcieTest, AvailableComponentsInreasedOnPcieAdding)
{
    setPeciPresenceOnReading(std::bitset<kMaxPcieNumber>(5));
    auto [ec2, components2] = dbusGetProperty<std::vector<DeviceIndex>>(
        kDomainAttributesInterface, "AvailableComponents");
    EXPECT_EQ(ec2, boost::system::errc::success);
    EXPECT_THAT(components2, testing::ElementsAre(0, 2));

    setPeciPresenceOnReading(std::bitset<kMaxPcieNumber>(21));
    auto [ec, components] = dbusGetProperty<std::vector<DeviceIndex>>(
        kDomainAttributesInterface, "AvailableComponents");
    EXPECT_EQ(ec, boost::system::errc::success);
    EXPECT_THAT(components, testing::ElementsAre(0, 2, 4));
}

TEST_F(DomainPcieTest, ChangePeciPresenceExpectPoliciesToValidateParams)
{
    this->setupPolicies(
        {
            {PolicyState::disabled, 10, BudgetingStrategy::aggressive, 1},
            {PolicyState::triggered, 10, BudgetingStrategy::aggressive, 1},
        },
        10, 30);

    EXPECT_CALL(*this->policies_.at(0), validateParameters());
    EXPECT_CALL(*this->policies_.at(1), validateParameters());
    setPeciPresenceOnReading(std::bitset<kMaxPcieNumber>(5));
}

TEST_F(DomainPcieTest,
       SetPeciPresenceSameValueExpectPoliciesToValidateParamsOnce)
{
    this->setupPolicies(
        {
            {PolicyState::disabled, 10, BudgetingStrategy::aggressive, 1},
            {PolicyState::triggered, 10, BudgetingStrategy::aggressive, 1},
        },
        10, 30);

    EXPECT_CALL(*this->policies_.at(0), validateParameters());
    EXPECT_CALL(*this->policies_.at(1), validateParameters());
    setPeciPresenceOnReading(std::bitset<kMaxPcieNumber>(5));
    setPeciPresenceOnReading(std::bitset<kMaxPcieNumber>(5));
}

} // namespace nodemanager