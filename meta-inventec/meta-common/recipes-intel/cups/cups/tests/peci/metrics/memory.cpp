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

#include "peci/metrics/impl/memory_factory.hpp"
#include "peci/mocks.hpp"

#include <cstdint>
#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace ::testing;
using namespace ::cups;

class MemoryTest : public ::testing::Test
{
  public:
    using MemoryFactory = peci::metrics::impl::MemoryFactory<Adapter>;
    static constexpr unsigned cpuAddr = 0x32;
    static constexpr unsigned cpuId = 0x606A0; // ICX LCC
    static constexpr unsigned busNumber = 0xFF;

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

static void createDefaultFakeMemory()
{
    ON_CALL(*Adapter::mock, isDIMMPopulated(_, _, _, _, _, _, _))
        .WillByDefault(DoAll(SetArgReferee<6>(true), Return(true)));
    ON_CALL(*Adapter::mock, getMemoryFreq(_, _)).WillByDefault(Return(true));
}

TEST_F(MemoryTest, detectSucceeds_onAllSuccessfulCalls)
{
    createDefaultFakeMemory();

    auto memory = MemoryFactory::detect(cpuAddr, cpuId, busNumber);
    EXPECT_TRUE(memory);
}

TEST_F(MemoryTest, detectFails_onFailedFrequencyRatio)
{
    createDefaultFakeMemory();
    ON_CALL(*Adapter::mock, getMemoryFreq(_, _)).WillByDefault(Return(false));

    auto memory = MemoryFactory::detect(cpuAddr, cpuId, busNumber);
    EXPECT_FALSE(memory);
}

TEST_F(MemoryTest, dimmCountCalculationNoDimms)
{
    constexpr unsigned expectedDimmCount = 0;
    constexpr unsigned expectedChannelCount = 0;

    createDefaultFakeMemory();
    ON_CALL(*Adapter::mock, isDIMMPopulated(_, _, _, _, _, _, _))
        .WillByDefault(DoAll(SetArgReferee<6>(false), Return(true)));

    auto memory = MemoryFactory::detect(cpuAddr, cpuId, busNumber);
    ASSERT_TRUE(memory);
    EXPECT_EQ(memory->getChannelCount(), expectedChannelCount);
    EXPECT_EQ(memory->getDimmCount(), expectedDimmCount);
}

TEST_F(MemoryTest, dimmCountCalculationAllDimms)
{
    constexpr unsigned expectedDimmCount = 16;
    constexpr unsigned expectedChannelCount = expectedDimmCount / 2;

    createDefaultFakeMemory();
    ON_CALL(*Adapter::mock, isDIMMPopulated(_, _, _, _, _, _, _))
        .WillByDefault(DoAll(SetArgReferee<6>(true), Return(true)));

    auto memory = MemoryFactory::detect(cpuAddr, cpuId, busNumber);
    ASSERT_TRUE(memory);
    EXPECT_EQ(memory->getChannelCount(), expectedChannelCount);
    EXPECT_EQ(memory->getDimmCount(), expectedDimmCount);
}

TEST_F(MemoryTest, utilCalculation)
{
    constexpr uint32_t memoryFrequency = 50 * 133;
    constexpr uint8_t channelCount = 8;
    uint64_t expectedMaxUtil =
        peci::abi::MHzToHz(channelCount * memoryFrequency *
                           peci::abi::memory::channelWidth) /
        peci::abi::memory::counterResolution;

    createDefaultFakeMemory();
    ON_CALL(*Adapter::mock, getMemoryFreq(_, _))
        .WillByDefault(DoAll(SetArgReferee<1>(memoryFrequency), Return(true)));

    auto memory = MemoryFactory::detect(cpuAddr, cpuId, busNumber);
    ASSERT_TRUE(memory);
    EXPECT_EQ(memory->getMaxUtil(), expectedMaxUtil);
}
