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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

TEST(CpuModel, Spr)
{
    using namespace cups::peci;

    // XCC-A0
    EXPECT_EQ(cpu::model::spr, cpu::toModel(0x806F0));

    // XCC-B0
    EXPECT_EQ(cpu::model::spr, cpu::toModel(0x806f1));

    // XCC-C0
    EXPECT_EQ(cpu::model::spr, cpu::toModel(0x806f2));

    // XCC-D0, MCC-D0, LCC-D0, UCC-D0
    EXPECT_EQ(cpu::model::spr, cpu::toModel(0x806f3));

    // XCC-E0, MCC-E0, LCC-E0, UCC-E0
    EXPECT_EQ(cpu::model::spr, cpu::toModel(0x806f4));

    // HBM-A0
    EXPECT_EQ(cpu::model::spr, cpu::toModel(0x806f1));
}

TEST(CpuModel, Invalid_Throws)
{
    using namespace cups::peci;

    EXPECT_ANY_THROW(cpu::toModel(0x11111));
}