/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2022 Intel Corporation.
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

#include "domain_test.hpp"

namespace nodemanager
{

class DomainHwProtectionTest : public DomainTestSimple<DomainHwProtection>
{
  public:
    virtual ~DomainHwProtectionTest() = default;

    virtual void SetUp() override
    {
        ON_CALL(*this->devicesManager_,
                registerReadingConsumerHelper(
                    testing::_, ReadingType::dcRatedPowerMin, kAllDevices))
            .WillByDefault(
                testing::SaveArg<0>(&(this->dcRatedMinReadingConsumer_)));

        DomainTestSimple::SetUp();
        ON_CALL(*this->capabilities_, getMin())
            .WillByDefault(testing::Return(capabilitiesMin));
        ON_CALL(*this->capabilities_, getMax())
            .WillByDefault(testing::Return(capabilitiesMax));
        ON_CALL(*this->capabilities_, getMaxRated())
            .WillByDefault(testing::Return(capabilitiesMaxRated));
    }

    std::shared_ptr<ReadingConsumer> dcRatedMinReadingConsumer_ = nullptr;
    double capabilitiesMin = 7;
    double capabilitiesMax = 180;
    double capabilitiesMaxRated = 200;
};

TEST_F(DomainHwProtectionTest, ReadingSourcePsuLimitSetCorrectly)
{
    setReadingSource(SensorReadingType::dcPlatformPowerPsu);
    EXPECT_CALL(*this->hwProtectionPolicy_, setLimit(123));
    dcRatedMinReadingConsumer_->updateValue(123);
}

TEST_F(DomainHwProtectionTest, ReadingSourceOtherLimitSetCorrectly)
{
    setReadingSource(SensorReadingType::dcPlatformPowerCpu);
    EXPECT_CALL(*this->hwProtectionPolicy_, setLimit(capabilitiesMax));
    this->capabilitiesChangeCallback();
}

TEST_F(DomainHwProtectionTest,
       ReadingSourcePsuCapabilitiesChangeLimitDoNotChange)
{
    setReadingSource(SensorReadingType::dcPlatformPowerPsu);
    EXPECT_CALL(*this->hwProtectionPolicy_, setLimit(testing::_)).Times(0);
    this->capabilitiesChangeCallback();
}

TEST_F(DomainHwProtectionTest,
       ReadingSourceOtherDcRatedReadingChangesLimitDoNotChange)
{
    setReadingSource(SensorReadingType::dcPlatformPowerCpu);
    EXPECT_CALL(*this->hwProtectionPolicy_, setLimit(testing::_)).Times(0);
    dcRatedMinReadingConsumer_->updateValue(123);
}

TEST_F(DomainHwProtectionTest, ReadingSourcePsuCapabilitiesChangeBlocked)
{
    setReadingSource(SensorReadingType::dcPlatformPowerPsu);
    EXPECT_CALL(*this->capabilities_, setMax(testing::_)).Times(0);
    EXPECT_CALL(*this->capabilities_, setMin(testing::_)).Times(0);
    this->dbusSetProperty(kDomainCapabilitiesInterface, "Max", 123);
    this->dbusSetProperty(kDomainCapabilitiesInterface, "Min", 34);
}

TEST_F(DomainHwProtectionTest, ReadingSourceOtherCapabilitiesChangeAllowed)
{
    setReadingSource(SensorReadingType::dcPlatformPowerCpu);
    EXPECT_CALL(*this->capabilities_, setMax(123));
    EXPECT_CALL(*this->capabilities_, setMin(34));
    this->dbusSetProperty(kDomainCapabilitiesInterface, "Max", 123);
    this->dbusSetProperty(kDomainCapabilitiesInterface, "Min", 34);
}

TEST_F(DomainHwProtectionTest,
       ReadingSourceOtherCapabilityMaxOverRatedChangeBlocked)
{
    setReadingSource(SensorReadingType::dcPlatformPowerCpu);
    EXPECT_CALL(*this->capabilities_, setMax(testing::_)).Times(0);
    EXPECT_THAT(
        this->dbusSetProperty(kDomainCapabilitiesInterface, "Max", 200.1),
        testing::Ne(boost::system::errc::success));
}

} // namespace nodemanager