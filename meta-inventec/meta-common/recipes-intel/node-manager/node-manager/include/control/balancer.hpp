/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020-2021 Intel Corporation.
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
#include "control/scalability/scalability.hpp"
#include "devices_manager/devices_manager.hpp"
#include "flow_control.hpp"

#include <cassert>
#include <iostream>
#include <map>

namespace nodemanager
{

class Balancer : public RunnerIf
{
  public:
    Balancer() = delete;
    Balancer(const Balancer&) = delete;
    Balancer& operator=(const Balancer&) = delete;
    Balancer(Balancer&&) = delete;
    Balancer& operator=(Balancer&&) = delete;

    Balancer(std::shared_ptr<DevicesManagerIf> devicesManagerArg,
             KnobType knobTypeArg,
             std::shared_ptr<ScalabilityIf> scalabilityFactorArg) :
        devicesManager(devicesManagerArg),
        knobType(knobTypeArg), domainPowerBudget(std::nullopt),
        scalabilityFactor(scalabilityFactorArg)
    {
        if (!isCastSafe<DeviceIndex>(scalabilityFactor->getFactors().size()))
        {
            throw std::logic_error("Creating Balancer failed");
        }
        totalComponentsNumber =
            static_cast<DeviceIndex>(scalabilityFactor->getFactors().size());
        hwComponentLimits.resize(
            totalComponentsNumber,
            HwLimit{std::nullopt, false, LimitSource::none});
        externalComponentLimits.resize(totalComponentsNumber, std::nullopt);
    }

    ~Balancer() = default;

    void run() override final
    {
        resetLimits();
        distributeBudget();
        applyLimits();
    }

    void setDomainPowerBudget(const std::optional<Limit>& limit)
    {
        domainPowerBudget = limit;
    }

    void setComponentLimit(DeviceIndex componentId,
                           const std::optional<Limit>& limit)
    {
        if (componentId < externalComponentLimits.size())
        {
            externalComponentLimits[componentId] = limit;
        }
        else
        {
            throw std::runtime_error("ComponentId out of range");
        }
    }

    bool isDomainLimitActive() const
    {
        const std::vector<HwLimit>& v = hwComponentLimits;

        if (std::find_if(v.begin(), v.end(), [](const HwLimit& l) {
                return l.source == LimitSource::domainBudget;
            }) != v.end())
        {
            return true;
        }

        return false;
    }

    bool isComponentLimitActive(DeviceIndex componentId) const
    {
        try
        {
            return hwComponentLimits.at(componentId).source ==
                   LimitSource::componentLimit;
        }
        catch (std::out_of_range& e)
        {
            Logger::log<LogLevel::error>(
                "[Balancer]: componentId [%u] is not supported",
                static_cast<unsigned>(componentId));
        }
        return false;
    }

  protected:
    virtual void applyLimits()
    {
        for (DeviceIndex index = 0; index < totalComponentsNumber; ++index)
        {
            if (scaleFactorsVec[index] != 0)
            {
                auto&& incomingLimit = hwComponentLimits[index].limit;
                attemptToApplyLimit(index, incomingLimit);
            }
        }
    }

  private:
    std::shared_ptr<DevicesManagerIf> devicesManager;
    KnobType knobType;
    DeviceIndex totalComponentsNumber;
    std::optional<Limit> domainPowerBudget;
    enum class LimitSource
    {
        none,
        domainBudget,
        componentLimit
    };
    struct HwLimit
    {
        HwLimit(std::optional<Limit> l, bool f, LimitSource s) :
            limit(l), isFull(f), source(s)
        {
        }
        std::optional<Limit> limit{std::nullopt};
        bool isFull{false};
        LimitSource source{LimitSource::none};
    };
    std::vector<HwLimit> hwComponentLimits;
    std::vector<std::optional<Limit>> externalComponentLimits;
    std::shared_ptr<ScalabilityIf> scalabilityFactor;
    std::vector<double> scaleFactorsVec;
    double unassignedLimits{0.0};

    void attemptToApplyLimit(DeviceIndex index,
                             std::optional<Limit> incomingLimit)
    {
        if (incomingLimit.has_value() && !std::isnan(incomingLimit->value))
        {
            devicesManager->setKnobValue(knobType, index, incomingLimit->value);
        }
        else
        {
            devicesManager->resetKnobValue(knobType, index);
        }
    }

    void resetLimits()
    {
        HwLimit noHwLimit(std::nullopt, false, LimitSource::none);
        std::fill(hwComponentLimits.begin(), hwComponentLimits.end(),
                  noHwLimit);
    }

    void markUnavailableComponentsAsFull()
    {
        for (size_t i = 0; i < scaleFactorsVec.size(); i++)
        {
            if (scaleFactorsVec[i] == 0)
            {
                hwComponentLimits[i].isFull = true;
            }
        }
    }

    std::vector<std::optional<double>> distributeDomainPowerBudget()
    {
        std::vector<std::optional<double>> inputLimits;
        inputLimits.resize(totalComponentsNumber, std::nullopt);

        if (domainPowerBudget.has_value())
        {
            for (DeviceIndex i = 0; i < totalComponentsNumber; i++)
            {
                inputLimits[i] = scaleFactorsVec[i] * domainPowerBudget->value;
            }
        }
        return inputLimits;
    }

    std::vector<std::optional<double>>
        distributeLeftovers(const double leftovers) const
    {
        std::vector<std::optional<double>> newScaledLimits;
        newScaledLimits.resize(totalComponentsNumber, std::nullopt);
        double newScaledLimitsBase = 0.0;

        for (DeviceIndex i = 0; i < totalComponentsNumber; i++)
        {
            newScaledLimitsBase +=
                hwComponentLimits[i].isFull ? 0 : scaleFactorsVec[i];
        }
        for (DeviceIndex i = 0; i < totalComponentsNumber; i++)
        {
            if (hwComponentLimits[i].isFull &&
                hwComponentLimits[i].limit.has_value())
            {
                newScaledLimits[i] = hwComponentLimits[i].limit->value;
            }
            else
            {
                newScaledLimits[i] =
                    leftovers * (scaleFactorsVec[i] / newScaledLimitsBase);
            }
        }
        return newScaledLimits;
    }

    std::pair<DeviceIndex, double> adjustDistributedLimitsToExternalLimits(
        const std::vector<std::optional<double>> inputLimits)
    {
        double leftovers = 0.0;
        DeviceIndex notFullComponentLimitsCntr = totalComponentsNumber;
        for (DeviceIndex i = 0; i < totalComponentsNumber; i++)
        {
            if (!hwComponentLimits[i].isFull)
            {
                if (externalComponentLimits[i].has_value() &&
                    (!inputLimits[i].has_value() ||
                     (inputLimits[i].value() >
                      externalComponentLimits[i]->value)))
                {
                    hwComponentLimits[i].limit = externalComponentLimits[i];
                    hwComponentLimits[i].source = LimitSource::componentLimit;
                    hwComponentLimits[i].isFull = true;
                }
                else
                {
                    if (inputLimits[i].has_value())
                    {
                        hwComponentLimits[i].limit =
                            Limit{inputLimits[i].value(),
                                  domainPowerBudget->strategy};
                        hwComponentLimits[i].source = LimitSource::domainBudget;
                    }
                }
            }
        }

        if (domainPowerBudget.has_value())
        {
            leftovers =
                domainPowerBudget->value -
                std::accumulate(
                    hwComponentLimits.begin(), hwComponentLimits.end(), 0.0,
                    [](double sum, const HwLimit& curr) {
                        return sum + (curr.limit.has_value() ? curr.limit->value
                                                             : 0.0);
                    });
        }

        notFullComponentLimitsCntr = static_cast<DeviceIndex>(
            std::count_if(hwComponentLimits.begin(), hwComponentLimits.end(),
                          [](const HwLimit& curr) { return !curr.isFull; }));

        return std::make_pair(notFullComponentLimitsCntr, leftovers);
    }

    void distributeBudget()
    {
        scaleFactorsVec = scalabilityFactor->getFactors();
        markUnavailableComponentsAsFull();

        std::vector<std::optional<double>> distributedLimits =
            distributeDomainPowerBudget();
        double leftovers{0.0};
        DeviceIndex notFullComponentLimitsCntr = 0;

        do
        {
            std::tie(notFullComponentLimitsCntr, leftovers) =
                adjustDistributedLimitsToExternalLimits(distributedLimits);
            if (notFullComponentLimitsCntr != 0 && leftovers > 0)
            {
                const auto tmp = distributeLeftovers(leftovers);
                for (size_t i = 0; i < distributedLimits.size(); i++)
                {
                    if (tmp[i].has_value())
                    {
                        distributedLimits[i].value() += tmp[i].value();
                    }
                }
            }
        } while (notFullComponentLimitsCntr != 0 && leftovers > 0);
        unassignedLimits = leftovers;
    }
}; // namespace nodemanager

} // namespace nodemanager
