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

#pragma once

#include "policies/power_policy.hpp"
#include "policy_test.hpp"

class PowerPolicyTestSimple : public PolicyTest<PowerPolicy>
{
};

TEST_F(PowerPolicyTestSimple,
       AdjustCorrectableParametersFixesCorrectionTimeOutOfRange)
{
    PolicyConfig policyConfig;
    policyConfig.limit(static_cast<uint16_t>(kDomainCapabilitiesMin))
        .correctionInMs(domainInfo_->capabilities->getMaxCorrectionTimeInMs() +
                        1);
    sut_->updateParams(policyConfig._getStruct());
    sut_->adjustCorrectableParameters();
    auto [ec, attribute] = DbusEnvironment::getProperty<uint32_t>(
        kPolicyAttributesIface, kPolicyObjectPath + kPolicyId,
        "CorrectionInMs");
    ASSERT_EQ(ec.value(), 0);
    EXPECT_EQ(attribute, domainInfo_->capabilities->getMaxCorrectionTimeInMs());
}

using LimitParams = std::tuple<uint16_t, int>;

class PowerPolicyPropertyLimitTest : public PolicyPropertyTest<PowerPolicy>,
                                     public WithParamInterface<LimitParams>
{
  public:
    PowerPolicyPropertyLimitTest() : PolicyPropertyTest("Limit")
    {
    }
};

INSTANTIATE_TEST_SUITE_P(
    PolicyLimitInvalid, PowerPolicyPropertyLimitTest,
    Values(LimitParams{uint16_t{1},
                       static_cast<std::underlying_type_t<ErrorCodes>>(
                           ErrorCodes::PowerLimitOutOfRange)},
           LimitParams{uint16_t{32768},
                       static_cast<std::underlying_type_t<ErrorCodes>>(
                           ErrorCodes::PowerLimitOutOfRange)}));

INSTANTIATE_TEST_SUITE_P(PolicyLimitValid, PowerPolicyPropertyLimitTest,
                         Values(LimitParams{uint16_t{0}, 0},
                                LimitParams{uint16_t{2}, 0},
                                LimitParams{uint16_t{32767}, 0}));

TEST_P(PowerPolicyPropertyLimitTest, SetAttribute)
{
    InSequence seq;
    auto [propertyValue, expectedErrorCode] = GetParam();
    setAttributeTestBody(propertyValue, expectedErrorCode);
}

using CorrectionTimeParams = std::tuple<uint32_t, int>;

class PowerPolicyPropertyCorrectionTimeTest
    : public PolicyPropertyTest<PowerPolicy>,
      public WithParamInterface<CorrectionTimeParams>
{
  public:
    PowerPolicyPropertyCorrectionTimeTest() :
        PolicyPropertyTest("CorrectionInMs")
    {
    }
};

INSTANTIATE_TEST_SUITE_P(
    PowerPolicyCorrectionTimeInvalid, PowerPolicyPropertyCorrectionTimeTest,
    Values(CorrectionTimeParams{0u,
                                static_cast<std::underlying_type_t<ErrorCodes>>(
                                    ErrorCodes::CorrectionTimeOutOfRange)},
           CorrectionTimeParams{60001u,
                                static_cast<std::underlying_type_t<ErrorCodes>>(
                                    ErrorCodes::CorrectionTimeOutOfRange)}));

INSTANTIATE_TEST_SUITE_P(PowerPolicyCorrectionTimeValid,
                         PowerPolicyPropertyCorrectionTimeTest,
                         Values(CorrectionTimeParams{6000u, 0},
                                CorrectionTimeParams{60000u, 0}));

TEST_P(PowerPolicyPropertyCorrectionTimeTest, SetAttribute)
{
    InSequence seq;
    auto [propertyValue, expectedErrorCode] = GetParam();
    setAttributeTestBody(propertyValue, expectedErrorCode);
}

using LimitExceptionParams =
    std::tuple<std::underlying_type_t<LimitException>, int>;

class PowerPolicyPropertyLimitExceptionTest
    : public PolicyPropertyTest<PowerPolicy>,
      public WithParamInterface<LimitExceptionParams>
{
  public:
    PowerPolicyPropertyLimitExceptionTest() :
        PolicyPropertyTest("LimitException")
    {
    }
};

INSTANTIATE_TEST_SUITE_P(
    PowerPolicyLimitExceptionInvalid, PowerPolicyPropertyLimitExceptionTest,
    Values(LimitExceptionParams{4,
                                static_cast<std::underlying_type_t<ErrorCodes>>(
                                    ErrorCodes::InvalidLimitException)},
           LimitExceptionParams{100,
                                static_cast<std::underlying_type_t<ErrorCodes>>(
                                    ErrorCodes::InvalidLimitException)}));

INSTANTIATE_TEST_SUITE_P(
    PowerPolicyLimitExceptionValid, PowerPolicyPropertyLimitExceptionTest,
    Values(
        LimitExceptionParams{
            static_cast<std::underlying_type_t<LimitException>>(
                LimitException::noAction),
            0},
        LimitExceptionParams{
            static_cast<std::underlying_type_t<LimitException>>(
                LimitException::powerOff),
            0},
        LimitExceptionParams{
            static_cast<std::underlying_type_t<LimitException>>(
                LimitException::logEvent),
            0},
        LimitExceptionParams{
            static_cast<std::underlying_type_t<LimitException>>(
                LimitException::logEventAndPowerOff),
            0}));

TEST_P(PowerPolicyPropertyLimitExceptionTest, SetAttribute)
{
    InSequence seq;
    auto [propertyValue, expectedErrorCode] = GetParam();
    setAttributeTestBody(propertyValue, expectedErrorCode);
}

using PowerCorrectionTypeParams =
    std::tuple<std::underlying_type_t<PowerCorrectionType>, int>;

class PowerPolicyPropertyPowerCorrectionTypeTest
    : public PolicyPropertyTest<PowerPolicy>,
      public WithParamInterface<PowerCorrectionTypeParams>
{
  public:
    PowerPolicyPropertyPowerCorrectionTypeTest() :
        PolicyPropertyTest("PowerCorrectionType")
    {
    }
};

INSTANTIATE_TEST_SUITE_P(
    PowerPolicyPowerCorrectionTypeInvalid,
    PowerPolicyPropertyPowerCorrectionTypeTest,
    Values(
        PowerCorrectionTypeParams{
            3, static_cast<std::underlying_type_t<ErrorCodes>>(
                   ErrorCodes::InvalidPowerCorrectionType)},
        PowerCorrectionTypeParams{
            100, static_cast<std::underlying_type_t<ErrorCodes>>(
                     ErrorCodes::InvalidPowerCorrectionType)}));

INSTANTIATE_TEST_SUITE_P(
    PowerPolicyPowerCorrectionTypeValid,
    PowerPolicyPropertyPowerCorrectionTypeTest,
    Values(
        PowerCorrectionTypeParams{
            static_cast<std::underlying_type_t<PowerCorrectionType>>(
                PowerCorrectionType::automatic),
            0},
        PowerCorrectionTypeParams{
            static_cast<std::underlying_type_t<PowerCorrectionType>>(
                PowerCorrectionType::nonAggressive),
            0},
        PowerCorrectionTypeParams{
            static_cast<std::underlying_type_t<PowerCorrectionType>>(
                PowerCorrectionType::aggressive),
            0}));

TEST_P(PowerPolicyPropertyPowerCorrectionTypeTest, SetAttribute)
{
    InSequence seq;
    auto [propertyValue, expectedErrorCode] = GetParam();
    setAttributeTestBody(propertyValue, expectedErrorCode);
}

using StatisticsTestParams = std::tuple<PolicyConfig, int>;

class PowerPolicyTestStatistics
    : public PolicyTest<PowerPolicy>,
      public WithParamInterface<StatisticsTestParams>
{
};

INSTANTIATE_TEST_SUITE_P(
    UpdateParamsDoNotRegisterStatistics, PowerPolicyTestStatistics,
    Values(
        StatisticsTestParams{PolicyConfig(), 0},
        StatisticsTestParams{
            PolicyConfig().triggerLimit(PolicyConfig().triggerLimit() + 1), 0},
        StatisticsTestParams{
            PolicyConfig().limitException(LimitException::logEvent), 0},
        StatisticsTestParams{
            PolicyConfig().correctionInMs(PolicyConfig().correctionInMs() + 1),
            0}));

INSTANTIATE_TEST_SUITE_P(
    UpdateParamsRegisterStatistics, PowerPolicyTestStatistics,
    Values(StatisticsTestParams{PolicyConfig().componentId(0), 1},
           StatisticsTestParams{PolicyConfig().componentId(4), 1},
           StatisticsTestParams{PolicyConfig().statReportingPeriod(
                                    PolicyConfig().statReportingPeriod() + 1),
                                1}));

TEST_P(PowerPolicyTestStatistics, UpdateParams)
{
    auto [policyConfig, timesRegisteringStatistics] = GetParam();
    policyConfig.limit(static_cast<uint16_t>(kDomainCapabilitiesMin));

    if (policyConfig.componentId() == kAllDevices)
    {
        EXPECT_CALL(*devicesManager_,
                    registerReadingConsumerHelper(testing::_, testing::_,
                                                  policyConfig.componentId()))
            .Times(timesRegisteringStatistics + 1);
    }
    else
    {
        EXPECT_CALL(*devicesManager_,
                    registerReadingConsumerHelper(testing::_, testing::_,
                                                  policyConfig.componentId()))
            .Times(timesRegisteringStatistics);
        // exception monitor call
        EXPECT_CALL(*devicesManager_, registerReadingConsumerHelper(
                                          testing::_, testing::_, kAllDevices))
            .Times(1);
    }

    sut_->verifyParameters(policyConfig._getStruct());
    sut_->updateParams(policyConfig._getStruct());
}