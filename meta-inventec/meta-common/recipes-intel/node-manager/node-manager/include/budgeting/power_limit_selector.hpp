/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020 Intel Corporation.
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

#include "common_types.hpp"
namespace nodemanager
{
class PowerLimitSelector
{
    PowerLimitSelector(const PowerLimitSelector&) = delete;
    PowerLimitSelector& operator=(const PowerLimitSelector&) = delete;
    PowerLimitSelector(PowerLimitSelector&&) = delete;
    PowerLimitSelector& operator=(PowerLimitSelector&&) = delete;

  public:
    PowerLimitSelector()
    {
    }

    ~PowerLimitSelector() = default;

    void updateLimit(const Limit& newLimit, DomainId ptamDomain)
    {
        if (isLimitMoreRestrictive(newLimit))
        {
            plsLimit.emplace(PlsLimit{newLimit, ptamDomain});
        }
    }

    void resetLimit()
    {
        plsLimit.reset();
    }

    std::optional<Limit> getLimit()
    {
        if (!plsLimit.has_value())
        {
            return std::nullopt;
        }
        return plsLimit->limit;
    }

    std::optional<DomainId> getSourceDomain()
    {
        if (!plsLimit.has_value())
        {
            return std::nullopt;
        }
        return plsLimit->sourceDomain;
    }

    bool isActive(DomainId ptamDomain, BudgetingStrategy strategy)
    {
        return ((plsLimit.has_value()) &&
                (ptamDomain == plsLimit->sourceDomain) &&
                (strategy == plsLimit->limit.strategy));
    }

  private:
    struct PlsLimit
    {
        Limit limit;
        DomainId sourceDomain;
    };
    std::optional<PlsLimit> plsLimit;

    bool isLimitMoreRestrictive(const Limit& newLimit)
    {
        if (!plsLimit.has_value())
        {
            return true;
        }
        if (newLimit.value < plsLimit->limit.value)
        {
            return true;
        }
        if (newLimit.value == plsLimit->limit.value)
        {
            return static_cast<unsigned>(newLimit.strategy) <
                   static_cast<unsigned>(plsLimit->limit.strategy);
        }

        return false;
    }
};

} // namespace nodemanager