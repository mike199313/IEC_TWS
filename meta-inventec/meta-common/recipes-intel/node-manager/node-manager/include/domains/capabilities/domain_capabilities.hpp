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

#include "config/config.hpp"
#include "limit_capabilities.hpp"
#include "utility/latch_property.hpp"

#include <map>
#include <memory>

namespace nodemanager
{

static const double kNoOverwrittenValue = 0;

class DomainCapabilitiesIf : public LimitCapabilitiesIf
{
  public:
    virtual ~DomainCapabilitiesIf() = default;
    virtual void setMax(double value) = 0;
    virtual void setMin(double value) = 0;
    virtual double getMaxRated() const = 0;
    virtual uint32_t getMaxCorrectionTimeInMs() const = 0;
    virtual uint32_t getMinCorrectionTimeInMs() const = 0;
    virtual uint16_t getMaxStatReportingPeriod() const = 0;
    virtual uint16_t getMinStatReportingPeriod() const = 0;
};

class DomainCapabilities : public DomainCapabilitiesIf, public LimitCapabilities
{
  public:
    DomainCapabilities(const DomainCapabilities&) = delete;
    DomainCapabilities& operator=(const DomainCapabilities&) = delete;
    DomainCapabilities(DomainCapabilities&&) = delete;
    DomainCapabilities& operator=(DomainCapabilities&&) = delete;

    DomainCapabilities(
        const std::optional<ReadingType>& minReadingTypeArg,
        const std::optional<ReadingType>& maxReadingTypeArg,
        const std::shared_ptr<DevicesManagerIf>& devicesManagerArg,
        uint32_t minCorrectionTimeArg,
        OnCapabilitiesChangeCallback capabilitiesChangeCallbackArg,
        DomainId domainIdArg) :
        LimitCapabilities(
            minReadingTypeArg, maxReadingTypeArg,
            std::make_shared<ReadingEvent>([this](double incomingMinValue) {
                if (!std::isnan(incomingMinValue))
                {
                    auto previousValue = getMin();
                    lastMin = incomingMinValue;
                    min.set(incomingMinValue);
                    if ((getMin() != previousValue) &&
                        capabilitiesChangeCallback)
                    {
                        capabilitiesChangeCallback();
                    }
                }
            }),
            std::make_shared<ReadingEvent>([this](double incomingMaxValue) {
                if (!std::isnan(incomingMaxValue))
                {
                    auto previousValue = getMax();
                    lastMax = incomingMaxValue;
                    max.set(incomingMaxValue);
                    maxRated.set(incomingMaxValue);
                    if ((getMax() != previousValue) &&
                        capabilitiesChangeCallback)
                    {
                        capabilitiesChangeCallback();
                    }
                }
            }),
            devicesManagerArg),
        minCorrectionTime(minCorrectionTimeArg),
        capabilitiesChangeCallback(capabilitiesChangeCallbackArg),
        minReadingType(minReadingTypeArg), maxReadingType(maxReadingTypeArg),
        domainId(domainIdArg)
    {
        readConfig();
    };

    virtual ~DomainCapabilities() = default;

    double getMaxRated() const override
    {
        return maxRated.get();
    }
    double getMax() const override
    {
        return max.get();
    }
    void setMax(double value) override
    {
        auto previousValue = getMax();
        if (value == 0 && maxReadingType)
        {
            max.setAndUnlock(lastMax);
            updateConfigMax(kNoOverwrittenValue);
        }
        else
        {
            max.setAndLock(value);
            updateConfigMax(value);
        }

        if (getMax() != previousValue && capabilitiesChangeCallback)
        {
            capabilitiesChangeCallback();
        }
    }
    double getMin() const override
    {
        return min.get();
    }
    void setMin(double value) override
    {
        auto previousValue = getMin();
        if (value == 0 && minReadingType)
        {
            min.setAndUnlock(lastMin);
            updateConfigMin(kNoOverwrittenValue);
        }
        else
        {
            min.setAndLock(value);
            updateConfigMin(value);
        }

        if (getMin() != previousValue && capabilitiesChangeCallback)
        {
            capabilitiesChangeCallback();
        }
    }
    uint32_t getMaxCorrectionTimeInMs() const override
    {
        return 60000;
    };
    uint32_t getMinCorrectionTimeInMs() const override
    {
        return minCorrectionTime;
    };
    uint16_t getMaxStatReportingPeriod() const override
    {
        return 3600;
    };
    uint16_t getMinStatReportingPeriod() const override
    {
        return 1;
    };

    std::string getName() const override
    {
        return "Domain";
    }

    CapabilitiesValuesMap getValuesMap() const override
    {
        CapabilitiesValuesMap capabilitiesMap = {{"Min", min.get()},
                                                 {"Max", max.get()}};

        return capabilitiesMap;
    }

  private:
    LatchProperty<double> maxRated{kUnknownMaxPowerLimitInWatts};
    LatchProperty<double> max{kUnknownMaxPowerLimitInWatts};
    LatchProperty<double> min{0.0};
    uint32_t minCorrectionTime;
    OnCapabilitiesChangeCallback capabilitiesChangeCallback;
    std::optional<ReadingType> minReadingType;
    std::optional<ReadingType> maxReadingType;
    double lastMin, lastMax;
    DomainId domainId;

    void updateConfigMin(double value)
    {
        PowerRange config = Config::getInstance().getPowerRange();

        switch (domainId)
        {
            case DomainId::AcTotalPower:
                config.acMin = value;
                break;
            case DomainId::CpuSubsystem:
                config.cpuMin = value;
                break;
            case DomainId::MemorySubsystem:
                config.memoryMin = value;
                break;
            case DomainId::Pcie:
                config.pcieMin = value;
                break;
            case DomainId::DcTotalPower:
                config.dcMin = value;
                break;
            default:
                // Other domains do not support that
                // Silently discard
                return;
        }

        Config::getInstance().update(config);
    }

    void updateConfigMax(double value)
    {
        PowerRange config = Config::getInstance().getPowerRange();

        switch (domainId)
        {
            case DomainId::AcTotalPower:
                config.acMax = value;
                break;
            case DomainId::CpuSubsystem:
                config.cpuMax = value;
                break;
            case DomainId::MemorySubsystem:
                config.memoryMax = value;
                break;
            case DomainId::Pcie:
                config.pcieMax = value;
                break;
            case DomainId::DcTotalPower:
                config.dcMax = value;
                break;
            default:
                // Other domains do not support that
                // Silently discard
                return;
        }

        Config::getInstance().update(config);
    }

    void readConfig()
    {
        PowerRange config = Config::getInstance().getPowerRange();
        double configMin = 0, configMax = 0;

        switch (domainId)
        {
            case DomainId::AcTotalPower:
                configMin = config.acMin;
                configMax = config.acMax;
                break;
            case DomainId::CpuSubsystem:
                configMin = config.cpuMin;
                configMax = config.cpuMax;
                break;
            case DomainId::MemorySubsystem:
                configMin = config.memoryMin;
                configMax = config.memoryMax;
                break;
            case DomainId::Pcie:
                configMin = config.pcieMin;
                configMax = config.pcieMax;
                break;
            case DomainId::DcTotalPower:
                configMin = config.dcMin;
                configMax = config.dcMax;
                break;
            default:
                // Other domains do not support that
                // Silently discard
                return;
        }

        if (configMax > 0)
        {
            max.setAndLock(configMax);
        }

        if (configMin > 0)
        {
            min.setAndLock(configMin);
        }
    }
};
} // namespace nodemanager