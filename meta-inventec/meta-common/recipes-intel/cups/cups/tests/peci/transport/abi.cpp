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

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace ::cups;

TEST(AbiTest, MHzToHz_converst_properly)
{
    constexpr unsigned one_million = 1'000'000;
    EXPECT_EQ(one_million, peci::abi::MHzToHz(1));
    EXPECT_EQ(2 * one_million, peci::abi::MHzToHz(2));
    EXPECT_EQ(1000 * one_million, peci::abi::MHzToHz(1000));
}
