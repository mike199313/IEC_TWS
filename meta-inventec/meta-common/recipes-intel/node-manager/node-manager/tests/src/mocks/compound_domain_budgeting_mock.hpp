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

#include "budgeting/compound_domain_budgeting.hpp"

#include <gmock/gmock.h>

using namespace nodemanager;

class CompoundDomainBudgetingMock : public CompoundDomainBudgetingIf
{
  public:
    MOCK_METHOD((std::vector<std::pair<RaplDomainId, double>>),
                distributeBudget, (double), (const, override));
    MOCK_METHOD(void, updateDistributors, (const SimpleDomainCapabilities&),
                (override));
};
