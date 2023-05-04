/*
 *  INTEL CONFIDENTIAL
 *
 *  Copyright 2020 Intel Corporation
 *
 *  This software and the related documents are Intel copyrighted materials,
 *  and your use of them is governed by the express license under which they
 *  were provided to you (License). Unless the License provides otherwise,
 *  you may not use, modify, copy, publish, distribute, disclose or
 *  transmit this software or the related documents without
 *  Intel's prior written permission.
 *
 *  This software and the related documents are provided as is,
 *  with no express or implied warranties, other than those
 *  that are expressly stated in the License.
 */

#include "peci/abi.hpp"
#include "peci/metrics/impl/core_factory.hpp"
#include "peci/metrics/utilization.hpp"
#include "peci/mocks.hpp"

#include <bitset>
#include <cstdint>
#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace ::testing;
using namespace ::cups;

class CoreTest : public ::testing::Test
{
  public:
    using CoreFactory = peci::metrics::impl::CoreFactory<Adapter>;
    static constexpr unsigned cpuAddr = 0x32;
    static constexpr unsigned cpuId = 0x606A0; // ICX LCC

  protected:
    void SetUp() override
    {
        Adapter::mock = std::make_unique<::testing::NiceMock<Adapter::Mock>>();
    }

    void TearDown() override
    {
        Adapter::mock.reset();
    }
};

static void createDefaultFakeCpu(uint32_t cpuId)
{
    ON_CALL(*Adapter::mock, getCpuId(_, _))
        .WillByDefault(DoAll(SetArgReferee<1>(cpuId), Return(true)));
    ON_CALL(*Adapter::mock, getCpuBusNumber(_, _, _))
        .WillByDefault(Return(true));
    ON_CALL(*Adapter::mock, getCoreMaskLow(_, _, _))
        .WillByDefault(Return(true));
    ON_CALL(*Adapter::mock, getCoreMaskHigh(_, _, _))
        .WillByDefault(Return(true));
    ON_CALL(*Adapter::mock, getMaxNonTurboRatio(_, _))
        .WillByDefault(Return(true));
    ON_CALL(*Adapter::mock, getMaxTurboRatio(_, _, _, _))
        .WillByDefault(Return(true));
    ON_CALL(*Adapter::mock, isTurboEnabled(_, _)).WillByDefault(Return(true));
}

TEST_F(CoreTest, detectSucceeds_onAllSuccessfulCalls)
{
    createDefaultFakeCpu(cpuId);
    auto cpu = CoreFactory::detect(cpuAddr);
    EXPECT_TRUE(cpu);
}

TEST_F(CoreTest, detectFails_onFailedCpuId)
{
    createDefaultFakeCpu(cpuId);
    ON_CALL(*Adapter::mock, getCpuId(cpuAddr, _)).WillByDefault(Return(false));

    auto cpu = CoreFactory::detect(cpuAddr);
    EXPECT_FALSE(cpu);
}

TEST_F(CoreTest, detectFails_onFailedBusNumber)
{
    createDefaultFakeCpu(cpuId);
    ON_CALL(*Adapter::mock, getCpuBusNumber(cpuAddr, _, _))
        .WillByDefault(Return(false));

    auto cpu = CoreFactory::detect(cpuAddr);
    EXPECT_FALSE(cpu);
}

TEST_F(CoreTest, coreCountCalculation)
{
    std::array<std::tuple<uint32_t, uint32_t, uint8_t>, 16> masks = {
        {{0x00000000, 0x00000000, 0},
         {0x00000001, 0x00000000, 1},
         {0x00000000, 0x00000001, 1},
         {0x01000001, 0x00000000, 2},
         {0x00000000, 0x00010001, 2},
         {0x01010101, 0x00000001, 5},
         {0x01010101, 0x01010101, 8},
         {0xF1010101, 0x01010101, 12},
         {0xF1010101, 0x0101EEEE, 22},
         {0xFFFFFFFF, 0x00000000, 32},
         {0x00000000, 0xFFFFFFFF, 32},
         {0xFFFFFFFF, 0x00000001, 33},
         {0x01000000, 0xFFFFFFFF, 33},
         {0xF9FFFFFF, 0xFAFFFFFF, 60},
         {0xFFFFFFFF, 0xFEFFFFFF, 63},
         {0xFFFFFFFF, 0xFFFFFFFF, 64}}};

    createDefaultFakeCpu(cpuId);

    for (const auto& [maskLow, maskHigh, expectedCoreCount] : masks)
    {
        ON_CALL(*Adapter::mock, getCoreMaskLow(_, _, _))
            .WillByDefault(DoAll(SetArgReferee<2>(maskLow), Return(true)));

        ON_CALL(*Adapter::mock, getCoreMaskHigh(_, _, _))
            .WillByDefault(DoAll(SetArgReferee<2>(maskHigh), Return(true)));

        auto cpu = CoreFactory::detect(cpuAddr);
        ASSERT_TRUE(cpu);
        EXPECT_EQ(cpu->getCoreCount(), expectedCoreCount);
    }
}

TEST_F(CoreTest, maxUtilTurboCalculation)
{
    constexpr uint32_t coreMaskLow = (UINT32_MAX);
    constexpr uint32_t coreMaskHigh = (UINT32_MAX);
    constexpr uint8_t coreCount = 64;
    constexpr uint8_t maxTurboRatio = 10;
    constexpr bool turbo = true;
    constexpr uint64_t expectedMaxUtil = peci::abi::MHzToHz(
        maxTurboRatio * coreCount * peci::abi::cpuFreqRatioMHz);

    createDefaultFakeCpu(cpuId);
    ON_CALL(*Adapter::mock, getCoreMaskLow(_, _, _))
        .WillByDefault(DoAll(SetArgReferee<2>(coreMaskLow), Return(true)));
    ON_CALL(*Adapter::mock, getCoreMaskHigh(_, _, _))
        .WillByDefault(DoAll(SetArgReferee<2>(coreMaskHigh), Return(true)));
    ON_CALL(*Adapter::mock, getMaxTurboRatio(_, _, _, _))
        .WillByDefault(DoAll(SetArgReferee<3>(maxTurboRatio), Return(true)));
    ON_CALL(*Adapter::mock, isTurboEnabled(_, _))
        .WillByDefault(DoAll(SetArgReferee<1>(turbo), Return(true)));

    auto cpu = CoreFactory::detect(cpuAddr);
    ASSERT_TRUE(cpu);
    EXPECT_EQ(cpu->getMaxUtil(), expectedMaxUtil);
}

TEST_F(CoreTest, maxUtilNonTurboCalculation)
{
    constexpr uint32_t coreMaskLow = (UINT32_MAX);
    constexpr uint32_t coreMaskHigh = (UINT32_MAX);
    constexpr uint8_t coreCount = 64;
    constexpr uint8_t maxNonTurboRatio = 10;
    constexpr bool turbo = false;
    constexpr uint64_t expectedMaxUtil = peci::abi::MHzToHz(
        maxNonTurboRatio * coreCount * peci::abi::cpuFreqRatioMHz);

    createDefaultFakeCpu(cpuId);
    ON_CALL(*Adapter::mock, getCoreMaskLow(_, _, _))
        .WillByDefault(DoAll(SetArgReferee<2>(coreMaskLow), Return(true)));
    ON_CALL(*Adapter::mock, getCoreMaskHigh(_, _, _))
        .WillByDefault(DoAll(SetArgReferee<2>(coreMaskHigh), Return(true)));
    ON_CALL(*Adapter::mock, getMaxNonTurboRatio(_, _))
        .WillByDefault(DoAll(SetArgReferee<1>(maxNonTurboRatio), Return(true)));
    ON_CALL(*Adapter::mock, isTurboEnabled(_, _))
        .WillByDefault(DoAll(SetArgReferee<1>(turbo), Return(true)));

    auto cpu = CoreFactory::detect(cpuAddr);
    ASSERT_TRUE(cpu);
    EXPECT_EQ(cpu->getMaxUtil(), expectedMaxUtil);
}
