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

#pragma once

#include "peci/metrics/average.hpp"
#include "utils/log.hpp"

#include <boost/container/flat_set.hpp>
#include <boost/system/error_code.hpp>

#include <cmath>
#include <functional>

namespace cups
{

namespace base
{

class Sensor
{
  public:
    using ReadCallback = std::function<void(const boost::system::error_code& e,
                                            const double value)>;
    using SensorFailureCallback = std::function<void()>;

    Sensor(const std::string& nameArg, SensorFailureCallback&& callback) :
        name(nameArg), notifySensorFailure(std::move(callback))
    {
        LOG_DEBUG_T(name) << "base::Sensor Constructor";
    }

    const std::string& getName() const
    {
        return name;
    }

    std::optional<double> getValue() const
    {
        return value;
    }

    bool operator<(const Sensor& rhs) const
    {
        return name < rhs.name;
    }

    void registerObserver(ReadCallback&& callback)
    {
        observers.emplace_back(std::move(callback));
    }

    virtual void update(const boost::system::error_code& e,
                        const double valueArg)
    {
        if (e)
        {
            errorCounter++;
            if (errorCounter < sensorErrorLimit)
            {
                return;
            }

            LOG_ERROR << name << " sensor reading failed " << sensorErrorLimit
                      << " times in a row, rescheduling discovery";
            value = std::numeric_limits<double>::quiet_NaN();
            notifySensorFailure();
        }
        else
        {
            value = valueArg;
            errorCounter = 0;
        }

        for (const auto& callback : observers)
        {
            callback(e, *value);
        }
    }

    virtual void configChanged(unsigned numSamples)
    {}

    virtual ~Sensor()
    {
        LOG_DEBUG_T(name) << "base::Sensor ~Sensor";
    }

  protected:
    const std::string name;
    SensorFailureCallback notifySensorFailure;
    std::vector<base::Sensor::ReadCallback> observers;
    std::optional<double> value;
    static constexpr int sensorErrorLimit = 5;
    int errorCounter = 0;
};

class AverageSensor : public Sensor
{
  public:
    AverageSensor(const std::string& nameArg,
                  SensorFailureCallback&& callback) :
        Sensor(nameArg, std::move(callback))
    {
        LOG_DEBUG_T(name) << "base::AverageSensor Constructor";
    }

    static std::shared_ptr<AverageSensor> make(const std::string& nameArg,
                                               Sensor& sensor,
                                               SensorFailureCallback&& callback)
    {
        auto averageSensor =
            std::make_shared<AverageSensor>(nameArg, std::move(callback));

        sensor.registerObserver(
            [weakAverageSensor(std::weak_ptr<AverageSensor>(averageSensor))](
                const boost::system::error_code& e, const double value) {
                if (auto averageSensor = weakAverageSensor.lock())
                {
                    averageSensor->update(e, value);
                }
            });

        return averageSensor;
    }

    void update(const boost::system::error_code& e,
                const double valueArg) override
    {
        double val;

        if (std::isnan(valueArg))
        {
            val = valueArg;
        }
        else
        {
            val = average.updateAverage(valueArg);
        }

        Sensor::update(e, val);
    }

    void configChanged(unsigned numSamples) override
    {
        average = cups::peci::metrics::AverageCounter(numSamples);
    }

    ~AverageSensor()
    {
        LOG_DEBUG_T(name) << "base::AverageSensor ~AverageSensor";
    }

  private:
    cups::peci::metrics::AverageCounter average;
};

} // namespace base

std::ostream& operator<<(std::ostream& o, const base::Sensor& s)
{
    o << s.getName();
    return o;
}

template <class S>
typename std::enable_if<std::is_base_of<base::Sensor, S>::value,
                        std::ostream&>::type
    operator<<(std::ostream& o, const std::shared_ptr<const S>& s)
{
    if (s)
    {
        o << *s;
    }
    else
    {
        o << s;
    }

    return o;
}

} // namespace cups
