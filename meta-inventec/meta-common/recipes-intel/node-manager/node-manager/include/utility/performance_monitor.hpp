/*
 * INTEL CONFIDENTIAL
 *
 * Copyright2022 Intel Corporation.
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

#include "clock.hpp"
#include "loggers/log.hpp"
#include "utility/dbus_interfaces.hpp"
#include "utility/status_provider_if.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace nodemanager
{

class Performance;
class PerformanceCollector;

/**
 * @brief A class that represents all performace measurements.
 *
 * It allows to get a value difference between start and stop methods, including
 * max value that ever occured.
 * This class also exposes on dbus its values. Units of: Value, ValueMax are
 * microseconds. The hitCounter attribute holds value of how many times this
 * measure was hit.
 */
class Measure
{

  public:
    Measure(const std::string nameArg, const Clock::duration thresholdArg) :
        name(nameArg), value(Clock::duration::zero()),
        value_max(Clock::duration::min()), hitCounter(0),
        valueAccumulator(Clock::duration::zero()), isMeasuring(false),
        threshold(thresholdArg){};

    void startMeasure(void)
    {
        if (!isMeasuring)
        {
            isMeasuring = true;
            start = Clock::now();
        }
        else
        {
            throw std::logic_error("Performance measuring cannot be started "
                                   "because it is already in progress, name: " +
                                   name);
        }
    }
    void stopMeasure(void)
    {
        if (isMeasuring)
        {
            if (std::numeric_limits<uint32_t>::max() == hitCounter ||
                ((Clock::duration::max() - valueAccumulator) < value))
            {
                resetStatistics();
            }
            else
            {
                value = Clock::now() - start;
                value_max = std::max(value_max, value);
                valueAccumulator += value;
                hitCounter++;
            }
            isMeasuring = false;
        }
        else
        {
            throw std::logic_error("Performance measuring cannot be stopped "
                                   "because it was not started, name: " +
                                   name);
        }
    }

    NmHealth getHealth() const
    {
        if (threshold != Clock::duration::zero() && hitCounter != 0 &&
            (valueAccumulator / hitCounter) > threshold)
        {
            return NmHealth::warning;
        }
        return NmHealth::ok;
    }

    std::string name; // @brief unique name identifing this mesurement
    Clock::duration
        value; // @brief a distance measured from start to stop calls
    Clock::duration value_max;
    uint32_t hitCounter; // @brief value in this attributes points how many
                         // times the Measure was hit
    Clock::duration valueAccumulator;

  private:
    Clock::time_point start = Clock::now();
    bool isMeasuring; // @brief flag that indicates that measuring is in
                      // progress
    Clock::duration threshold; // @brief if valueAccumulator exceeds threshold
                               // value then counter reports warning.

    void resetStatistics()
    {
        value_max = std::numeric_limits<Clock::duration>::min();
        hitCounter = 0;
        valueAccumulator = Clock::duration{0};
    }
};

void to_json(nlohmann::json& j, const Measure& measure)
{
    if (measure.hitCounter > 0)
    {
        j = nlohmann::json{
            {"HitCounter", measure.hitCounter},
            {"Value[us]", std::chrono::duration_cast<std::chrono::microseconds>(
                              measure.value)
                              .count()},
            {"ValueMax[us]",
             std::chrono::duration_cast<std::chrono::microseconds>(
                 measure.value_max)
                 .count()},
            {"ValueAverage[us]",
             std::chrono::duration_cast<std::chrono::microseconds>(
                 measure.valueAccumulator)
                     .count() /
                 measure.hitCounter},
            {"Health", enumToStr(healthNames, measure.getHealth())}};
    }
    else
    {
        j = nlohmann::json{
            {"HitCounter", measure.hitCounter},
            {"Value[us]", std::numeric_limits<double>::quiet_NaN()},
            {"ValueMax[us]", std::numeric_limits<double>::quiet_NaN()},
            {"ValueAverage[us]", std::numeric_limits<double>::quiet_NaN()},
            {"Health", enumToStr(healthNames, measure.getHealth())}};
    }
}

/**
 * @brief This class holds all mesuremenets.
 */
class PerformanceCollector : public StatusProviderIf
{
  public:
    PerformanceCollector() = default;
    PerformanceCollector(const PerformanceCollector&) = delete;
    PerformanceCollector& operator=(const PerformanceCollector&) = delete;
    PerformanceCollector(PerformanceCollector&&) = delete;
    PerformanceCollector& operator=(PerformanceCollector&&) = delete;

    std::shared_ptr<Measure> getMeasure(const std::string& nameArg,
                                        const Clock::duration thresholdArg)
    {
        auto& ret = measureMap[nameArg];
        if (!ret)
        {
            ret = std::make_shared<Measure>(nameArg, thresholdArg);
        }
        return ret;
    }

    void reportStatus(nlohmann::json& out) const final
    {
        auto& perfJson = out["Performance"];
        for (auto const& [key, measure] : measureMap)
        {
            perfJson[key] = *measure;
        }
        perfJson["Health"] = enumToStr(healthNames, getHealth());
    }

    NmHealth getHealth() const final
    {
        std::set<NmHealth> allHealth;
        for (auto const& [key, measure] : measureMap)
        {
            allHealth.insert(measure->getHealth());
        }
        return getMostRestrictiveHealth(allHealth);
    }

    void reset()
    {
        measureMap.clear();
    }

  private:
    std::unordered_map<std::string, std::shared_ptr<Measure>> measureMap;
};

std::weak_ptr<PerformanceCollector> performanceCollectorWp;

/**
 * @brief Purpose of this class is to start and stop the selected measurement
 * within the opbject's life scope.
 */
class MeasureWrapper
{
  public:
    MeasureWrapper() = default;
    MeasureWrapper(const MeasureWrapper&) = delete;
    MeasureWrapper& operator=(const MeasureWrapper&) = delete;
    MeasureWrapper(MeasureWrapper&& other) noexcept :
        measure(std::move(other.measure))
    {
        other.measure = nullptr;
    }
    MeasureWrapper& operator=(MeasureWrapper&& other)
    {
        if (measure)
        {
            measure->stopMeasure();
        }
        measure = nullptr;
        std::swap(measure, other.measure);
        return *this;
    }

    /**
     * @param nameArg It must ba a valid dbus object name.
     * The name should follow this naming convention:
     * <component>-<method>-<time measure approach>
     * @param thresholdArg - when the measurement exceeds this value then health
     * status will be reported as warning.
     */
    MeasureWrapper(const std::string& nameArg,
                   const Clock::duration thresholdArg = Clock::duration::zero())
    {
        if (auto spt = performanceCollectorWp.lock())
        {
            measure = spt->getMeasure(nameArg, thresholdArg);
            measure->startMeasure();
        }
    }

    ~MeasureWrapper()
    {
        stopMeasure();
    }

    void stopMeasure(void)
    {
        if (measure)
        {
            measure->stopMeasure();
            measure = nullptr;
        }
    }

  private:
    std::shared_ptr<Measure> measure;
};

using Perf = MeasureWrapper;

} // namespace nodemanager
