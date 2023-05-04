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

#include "average.hpp"
#include "clock.hpp"

#include <algorithm>
#include <boost/circular_buffer.hpp>
#include <chrono>
#include <cmath>
#include <numeric>

namespace nodemanager
{
struct Sample
{
    double acc;
    double max;
    double min;
};

static constexpr int kPtamStatsWindowCount = 30;

class MovingAverage : public Average
{
  public:
    MovingAverage(const DurationMs& period)
    {
        samplingWindow = period / kPtamStatsWindowCount;
    }

    virtual ~MovingAverage() = default;

    void addSample(double sample) override
    {
        isReset = false;
        if (!std::isfinite(sample))
        {
            reset();
            return;
        }

        auto now = Clock::now();
        DurationMs sampleDuration = now - timestamp;
        timestamp = now;

        accMin = std::min(accMin, sample);
        accMax = std::max(accMax, sample);

        if (std::isfinite(lastSample))
        {
            if (isDurationBeyondCurrentWindow(sampleDuration))
            {
                DurationMs leftover =
                    closeCurrentSampling(lastSample, sampleDuration);
                leftover = createNewSamples(lastSample, leftover);
                accReading = calculateSampleValue(lastSample, leftover);
                accTime = leftover;
                accMin = sample;
                accMax = sample;
            }
            else
            {
                accReading += calculateSampleValue(lastSample, sampleDuration);
                accTime += sampleDuration;
            }
        }

        lastSample = sample;
    }

    double getAvg() override
    {
        if (isReset)
        {
            return std::numeric_limits<double>::quiet_NaN();
        }
        addSample(lastSample);
        // obtain accumulators for current period
        double totalAccReading{accReading};
        DurationMs totalTime{accTime};

        // add accumulated values for already averaged periods
        totalAccReading += std::accumulate(
            bufferedSamples.begin(), bufferedSamples.end(), 0.0,
            [this](double sum, const Sample& curr) { return sum + curr.acc; });
        totalTime +=
            samplingWindow * static_cast<double>(bufferedSamples.size());

        return totalTime.count() ? totalAccReading / totalTime.count()
                                 : std::numeric_limits<double>::quiet_NaN();
    };

    DurationMs getStatisticsReportingPeriod() override
    {
        addSample(lastSample);
        DurationMs basePeriod =
            static_cast<double>(bufferedSamples.size()) * samplingWindow;
        return basePeriod + accTime;
    }

    virtual double getMin() override
    {
        if (isReset)
        {
            return std::numeric_limits<double>::quiet_NaN();
        }
        addSample(lastSample);
        auto it = std::min_element(
            bufferedSamples.begin(), bufferedSamples.end(),
            [](const auto& a, const auto& b) { return a.min < b.min; });
        if (it != bufferedSamples.end())
        {
            return std::min(accMin, it->min);
        }
        return accMin;
    }

    virtual double getMax() override
    {
        if (isReset)
        {
            return std::numeric_limits<double>::quiet_NaN();
        }
        addSample(lastSample);
        auto it = std::max_element(
            bufferedSamples.begin(), bufferedSamples.end(),
            [](const auto& a, const auto& b) { return a.max < b.max; });
        if (it != bufferedSamples.end())
        {
            return std::max(accMax, it->max);
        }
        return accMax;
    }

    void reset() override
    {
        Average::reset();
        bufferedSamples.clear();
    }

  private:
    double calculateSampleValue(const double value, const DurationMs delta)
    {
        return value * delta.count();
    }

    /**
     * @brief Calculates sampling value by time needed to close current sampling
     * window. Puts the sampling values into bufferedSamples and returns
     * remaining time.
     *
     * @param sampleValue - sample value
     * @param sampleDuration - sample duration
     * @return DurationMs - remaining time that should be used to calculate next
     * samples.
     */
    DurationMs closeCurrentSampling(const double sampleValue,
                                    const DurationMs& sampleDuration)
    {
        DurationMs durationToCloseSample = samplingWindow - accTime;
        accReading += calculateSampleValue(lastSample, durationToCloseSample);
        bufferedSamples.push_back(Sample{accReading, accMax, accMin});
        return sampleDuration - durationToCloseSample;
    }

    /**
     * @brief Creates as many new samples as possible within given
     * sampleDuration.
     *
     * @param sampleValue - sample value
     * @param sampleDuration - a time by which new samples should be created.
     * @return DurationMs - remaining time that should be used to calculate next
     * sample.
     */
    DurationMs createNewSamples(const double sampleValue,
                                const DurationMs& sampleDuration)
    {
        DurationMs remainingDuration = sampleDuration;
        for (; remainingDuration > samplingWindow;
             remainingDuration -= samplingWindow)
        {
            bufferedSamples.push_back(
                Sample{sampleValue * samplingWindow.count(), sampleValue,
                       sampleValue});
        }
        return remainingDuration;
    }

    bool isDurationBeyondCurrentWindow(const DurationMs& sampleDuration)
    {
        return accTime + sampleDuration >= samplingWindow;
    }

    boost::circular_buffer<Sample> bufferedSamples{kPtamStatsWindowCount};
    DurationMs samplingWindow;
};

} // namespace nodemanager