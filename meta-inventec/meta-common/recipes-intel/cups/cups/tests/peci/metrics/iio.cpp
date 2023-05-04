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
#include "peci/metrics/impl/iio_factory.hpp"
#include "peci/mocks.hpp"

#include <cstdint>
#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace ::testing;
using namespace ::cups;

class IioTest : public ::testing::Test
{
  public:
    using IioFactory = peci::metrics::impl::IioFactory<Adapter>;
    static constexpr unsigned cpuAddr = 0x32;
    static constexpr unsigned cpuId = 0x606A0; // ICX LCC

  protected:
    void SetUp() override
    {
        Adapter::mock = std::make_unique<::testing::NiceMock<Adapter::Mock>>();
        createDefaultFakeIio();
    }

    void TearDown() override
    {
        Adapter::mock.reset();
    }

  private:
    static void createDefaultFakeIio()
    {
        ON_CALL(*Adapter::mock, getLinkStatus(_, _, _, _, _, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*Adapter::mock, getXppMr(_, _, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*Adapter::mock, getXppMer(_, _, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*Adapter::mock, getXppErConf(_, _, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*Adapter::mock, setXppMr(_, _, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*Adapter::mock, setXppMer(_, _, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*Adapter::mock, setXppErConf(_, _, _, _))
            .WillByDefault(Return(true));
    }
};

TEST_F(IioTest, detectSucceeds_onAllSuccessfulCalls)
{
    auto iio = IioFactory::detect(cpuAddr, cpuId);
    EXPECT_TRUE(iio);
}

TEST_F(IioTest, detectFails_onFailedGetXppMr)
{
    ON_CALL(*Adapter::mock, getXppMr(_, _, _, _)).WillByDefault(Return(false));

    auto iio = IioFactory::detect(cpuAddr, cpuId);
    EXPECT_FALSE(iio);
}

TEST_F(IioTest, detectFails_onFailedGetXppMer)
{
    ON_CALL(*Adapter::mock, getXppMer(_, _, _, _)).WillByDefault(Return(false));

    auto iio = IioFactory::detect(cpuAddr, cpuId);
    EXPECT_FALSE(iio);
}

TEST_F(IioTest, detectFails_onFailedGetXppErConf)
{
    ON_CALL(*Adapter::mock, getXppErConf(_, _, _, _))
        .WillByDefault(Return(false));

    auto iio = IioFactory::detect(cpuAddr, cpuId);
    EXPECT_FALSE(iio);
}

TEST_F(IioTest, detectFails_onFailedSetXppMr)
{
    ON_CALL(*Adapter::mock, setXppMr(_, _, _, _)).WillByDefault(Return(false));

    auto iio = IioFactory::detect(cpuAddr, cpuId);
    EXPECT_FALSE(iio);
}

TEST_F(IioTest, detectFails_onFailedSetXppMer)
{
    ON_CALL(*Adapter::mock, setXppMer(_, _, _, _)).WillByDefault(Return(false));

    auto iio = IioFactory::detect(cpuAddr, cpuId);
    EXPECT_FALSE(iio);
}

TEST_F(IioTest, detectFails_onFailedSetXppErConf)
{
    ON_CALL(*Adapter::mock, setXppErConf(_, _, _, _))
        .WillByDefault(Return(false));

    auto iio = IioFactory::detect(cpuAddr, cpuId);
    EXPECT_FALSE(iio);
}

TEST_F(IioTest, utilCalculationNoLinks)
{
    constexpr uint8_t speedId = 1;
    constexpr uint8_t width = 16;
    constexpr bool active = false;
    constexpr uint64_t expectedMaxUtil = 0;

    ON_CALL(*Adapter::mock, getLinkStatus(_, _, _, _, _, _, _))
        .WillByDefault(DoAll(SetArgReferee<4>(speedId), SetArgReferee<5>(width),
                             SetArgReferee<6>(active), Return(true)));

    auto iio = IioFactory::detect(cpuAddr, cpuId);
    ASSERT_TRUE(iio);
    EXPECT_EQ(iio->getMaxUtil(), expectedMaxUtil);
}

TEST_F(IioTest, utilCalculationAllLinks)
{
    constexpr uint8_t speedId = 3;
    constexpr uint8_t speed = 8;
    constexpr uint8_t width = 16;
    constexpr uint8_t numLinks = 17;
    constexpr bool active = true;

    constexpr uint64_t expectedMaxUtil =
        static_cast<uint64_t>(peci::abi::iio::dwordsInGigabyte / 8) * speed *
        width * numLinks * peci::abi::iio::fullDuplex;

    ON_CALL(*Adapter::mock, getLinkStatus(_, _, _, _, _, _, _))
        .WillByDefault(DoAll(SetArgReferee<4>(speedId), SetArgReferee<5>(width),
                             SetArgReferee<6>(active), Return(true)));

    auto iio = IioFactory::detect(cpuAddr, cpuId);
    ASSERT_TRUE(iio);
    EXPECT_EQ(iio->getMaxUtil(), expectedMaxUtil);
}
