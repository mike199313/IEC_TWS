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

#include "peci/transport/adapter.hpp"

#include "peci/mocks.hpp"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace ::cups;
using namespace ::testing;

class AdapterTest : public ::testing::Test
{
  public:
    using Adapter = peci::transport::Adapter<Driver::commandHandler>;

  protected:
    void SetUp() override
    {
        Driver::mock = std::make_unique<::testing::NiceMock<Driver::Mock>>();
    }

    void TearDown() override
    {
        Driver::mock.reset();
    }
};

TEST_F(AdapterTest, CpuId_Positive)
{
    constexpr unsigned CpuAddr = 0x32;
    constexpr unsigned FakeCpuId = 0xDEADBEEF;

    auto injectFakeCpuId =
        [FakeCpuId](const uint8_t, const uint8_t*, const size_t, uint8_t* pRsp,
                    const size_t, const bool, const std::string&) {
            auto rsp = reinterpret_cast<peci::abi::response::GetCpuId*>(pRsp);
            *rsp = {{}, FakeCpuId};
            return true;
        };

    ON_CALL(*Driver::mock,
            commandHandler(CpuAddr, _, sizeof(peci::abi::request::GetCpuId), _,
                           sizeof(peci::abi::response::GetCpuId), _, _))
        .WillByDefault(Invoke(injectFakeCpuId));

    uint32_t CpuId = 0;
    ASSERT_TRUE(Adapter::getCpuId(CpuAddr, CpuId));
    EXPECT_EQ(CpuId, FakeCpuId);
}

TEST_F(AdapterTest, CpuId_Negative)
{
    constexpr unsigned CpuAddr = 0x32;

    ON_CALL(*Driver::mock,
            commandHandler(CpuAddr, _, sizeof(peci::abi::request::GetCpuId), _,
                           sizeof(peci::abi::response::GetCpuId), _, _))
        .WillByDefault(Return(false));

    uint32_t CpuId = 0;
    ASSERT_FALSE(Adapter::getCpuId(CpuAddr, CpuId));
    EXPECT_EQ(CpuId, 0);
}
