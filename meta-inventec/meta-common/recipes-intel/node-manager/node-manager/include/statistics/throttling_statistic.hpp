/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2021 Intel Corporation.
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

#include "domains/domain_types.hpp"
#include "readings/reading_event.hpp"
#include "statistic.hpp"

namespace nodemanager
{

const static constexpr double k100percent = 100.0;
const static constexpr double k0percent = 0.0;

class ThrottlingStatistic : public Statistic
{
  public:
    ThrottlingStatistic(const ThrottlingStatistic&) = delete;
    ThrottlingStatistic& operator=(const ThrottlingStatistic&) = delete;
    ThrottlingStatistic(ThrottlingStatistic&&) = delete;
    ThrottlingStatistic& operator=(ThrottlingStatistic&&) = delete;

    ThrottlingStatistic(std::string nameArg,
                        std::shared_ptr<AccumulatorIf> accumulatorArg,
                        std::shared_ptr<DomainCapabilitiesIf> capabilitiesArg) :
        Statistic(std::move(nameArg), std::move(accumulatorArg)),
        capabilities(capabilitiesArg)
    {
    }

    virtual ~ThrottlingStatistic() = default;

    virtual void updateValue(double newValue) override
    {
        auto throttling = CalculateThrotting(newValue);
        Statistic::updateValue(throttling);
    }

  private:
    std::shared_ptr<DomainCapabilitiesIf> capabilities;

    double CalculateThrotting(double limitValue)
    {
        if (capabilities->getMax() <= capabilities->getMin())
        {
            return std::numeric_limits<double>::quiet_NaN();
        }

        double value = (limitValue - capabilities->getMin()) * k100percent;
        value /= capabilities->getMax() - capabilities->getMin();
        value = k100percent - value;
        value = std::clamp(value, k0percent, k100percent);

        return value;
    }
};

} // namespace nodemanager