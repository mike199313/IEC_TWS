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

#include "base/loadFactors.hpp"
#include "peci/metrics/types.hpp"
#include "utils/log.hpp"

#include <boost/asio/steady_timer.hpp>
#include <boost/container/flat_map.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <variant>
#include <vector>

namespace cups
{

namespace base
{

class CupsReadings : public std::enable_shared_from_this<CupsReadings>
{
    // Prevents constructor from being called externally
    struct ctor_lock
    {};

    // default value of core/memory/iio load factor, must sum to
    // 100% and it is used as weight for calculation of cupsIndexUtilization
    static constexpr double defaultLoadFactor = 33.3;

  public:
    enum class Type
    {
        Core,
        Iio,
        Memory,
        CupsIndex,
        AverageCore,
        AverageIio,
        AverageMemory,
        AverageCupsIndex
    };

    CupsReadings(
        ctor_lock, boost::asio::io_context& iocArg,
        std::shared_ptr<sdbusplus::asio::connection> busArg,
        std::function<void(std::array<std::optional<peci::metrics::Cpu>,
                                      peci::cpu::limit>&)>&& cpuRetriever) :
        ioc(iocArg),
        bus(busArg),
        timer(iocArg, interval), getCpuData{std::move(cpuRetriever)}
    {
        if (getCpuData == nullptr)
        {
            throw std::runtime_error("Null cpuRetriever");
        }
    }

    static std::shared_ptr<CupsReadings>
        make(boost::asio::io_context& ioc,
             std::shared_ptr<sdbusplus::asio::connection> bus,
             std::function<void(std::array<std::optional<peci::metrics::Cpu>,
                                           peci::cpu::limit>&)>&& cpuRetriever)
    {
        auto readings = std::make_shared<CupsReadings>(ctor_lock{}, ioc, bus,
                                                       std::move(cpuRetriever));
        readings->initHostStateMonitor();
        readings->tick();

        return readings;
    }

    void monitor(const Type& type, const std::shared_ptr<Sensor>& sensor)
    {
        sensors[type] = sensor;
    }

    std::chrono::milliseconds getInterval()
    {
        return interval;
    }

    void setInterval(const std::chrono::milliseconds& newInterval)
    {
        if (interval != newInterval)
        {
            interval = newInterval;
            updateSampleCount();
        }
    }

    std::chrono::milliseconds getAveragingPeriod()
    {
        return averagingPeriod;
    }

    void setAveragingPeriod(const std::chrono::milliseconds& newAveragingPeriod)
    {
        if (averagingPeriod != newAveragingPeriod)
        {
            averagingPeriod = newAveragingPeriod;
            updateSampleCount();
        }
    }

    const LoadFactorCfg& getLoadFactorCfg() const
    {
        return loadFactorCfg;
    }

    const LoadFactors& getStaticLoadFactors() const
    {
        return staticLoadFactors;
    }

    const LoadFactors& getDynamicLoadFactors() const
    {
        return dynamicLoadFactors;
    }

    const LoadFactors& getActiveLoadFactorCfg() const
    {
        return loadFactorCfg == LoadFactorCfg::dynamicMode ? dynamicLoadFactors
                                                           : staticLoadFactors;
    }

    void setLoadFactorCfg(const LoadFactorCfg& newLoadFactorCfg)
    {
        loadFactorCfg = newLoadFactorCfg;
    }

    void setStaticLoadFactors(const double coreLoadFactor,
                              const double iioLoadFactor,
                              const double memoryLoadFactor)
    {
        staticLoadFactors.coreLoadFactor = coreLoadFactor;
        staticLoadFactors.iioLoadFactor = iioLoadFactor;
        staticLoadFactors.memoryLoadFactor = memoryLoadFactor;
    }

  private:
    std::reference_wrapper<boost::asio::io_context> ioc;
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::unique_ptr<sdbusplus::bus::match_t> hostStateMonitor;

    std::chrono::milliseconds interval = std::chrono::seconds(1);
    std::chrono::milliseconds averagingPeriod = std::chrono::seconds(1);
    boost::asio::steady_timer timer;

    CupsReadings(const CupsReadings&) = delete;
    CupsReadings& operator=(const CupsReadings&) = delete;

    std::function<void(
        std::array<std::optional<peci::metrics::Cpu>, peci::cpu::limit>&)>
        getCpuData;
    std::array<std::optional<peci::metrics::Cpu>, peci::cpu::limit> cpus;
    std::array<std::optional<peci::metrics::Utilization>, peci::cpu::limit>
        utilization;

    // [TODO] consider changing to std::array
    boost::container::flat_map<Type, std::shared_ptr<Sensor>> sensors;
    LoadFactorCfg loadFactorCfg = LoadFactorCfg::dynamicMode;
    LoadFactors staticLoadFactors;
    LoadFactors dynamicLoadFactors;
    std::string hostState;

    void initHostStateMonitor()
    {
        sdbusplus::asio::getProperty<std::string>(
            *bus, "xyz.openbmc_project.State.Host",
            "/xyz/openbmc_project/state/host0",
            "xyz.openbmc_project.State.Host", "CurrentHostState",
            [self = shared_from_this()](const boost::system::error_code ec,
                                        const std::string& initialHostState) {
                if (ec)
                {
                    LOG_ERROR << "Couldn't get host state: " << ec;
                }
                else
                {
                    self->hostState = initialHostState;
                    LOG_DEBUG << "Initial host state: " << self->hostState;
                }
            });

        constexpr auto matchParam =
            "type='signal',member='PropertiesChanged',path='/xyz/"
            "openbmc_project/state/"
            "host0',arg0='xyz.openbmc_project.State.Host'";

        hostStateMonitor = std::make_unique<sdbusplus::bus::match_t>(
            *bus, matchParam,
            [weakSelf =
                 weak_from_this()](sdbusplus::message::message& message) {
                if (auto self = weakSelf.lock())
                {
                    std::string iface;
                    boost::container::flat_map<
                        std::string, std::variant<std::monostate, std::string>>
                        changedProperties;
                    std::vector<std::string> invalidatedProperties;

                    message.read(iface, changedProperties,
                                 invalidatedProperties);

                    if (iface == "xyz.openbmc_project.State.Host")
                    {
                        const auto it =
                            changedProperties.find("CurrentHostState");
                        if (it != changedProperties.end())
                        {
                            if (auto val =
                                    std::get_if<std::string>(&it->second))
                            {
                                LOG_DEBUG << "New host state: " << *val;
                                self->hostState = *val;
                            }
                        }
                    }
                }
            });
    }

    void updateSampleCount()
    {
        auto numSamples =
            static_cast<unsigned>(averagingPeriod.count() / interval.count());

        for (const auto& sensor : sensors)
        {
            sensor.second->configChanged(numSamples);
        }
    }

    void tick()
    {
        updateCpuState();
        if ((hostState == "xyz.openbmc_project.State.Host.HostState.Off") &&
            (countCpus() > 0))
        {
            updateUtilizationForHostOff();
        }
        else
        {
            updateUtilization();
        }
        updateCupsIndex();

        timer.expires_after(interval);
        timer.async_wait(
            [self = shared_from_this()](const boost::system::error_code& e) {
                LOG_DEBUG << "CupsReadings::tick()";

                if (e)
                {
                    LOG_ERROR << "Timer failed with error : " << e;
                    return;
                }

                self->tick();
            });
    }

    void updateCpuState()
    {
        size_t idx = 0;

        getCpuData(cpus);

        // Clear data related to non-existent cpus
        for (auto& cpu : cpus)
        {
            auto& util = utilization[idx];
            if (!cpu)
            {
                util.reset();
            }
            else if (!util)
            {
                util.emplace(*cpu);
            }
            idx++;
        }
    }

    int countCpus()
    {
        const auto isCpuPresent =
            [](const std::optional<peci::metrics::Cpu>& cpu) {
                return cpu != std::nullopt;
            };

        return std::count_if(cpus.begin(), cpus.end(), isCpuPresent);
    }

    void updateUtilization()
    {
        auto core = sensors.find(Type::Core);
        if (core != sensors.end())
        {
            updateAggregateSensor(core->second,
                                  &peci::metrics::Utilization::core);
        }

        auto memory = sensors.find(Type::Memory);
        if (memory != sensors.end())
        {
            updateAggregateSensor(memory->second,
                                  &peci::metrics::Utilization::memory);
        }

        auto iio = sensors.find(Type::Iio);
        if (iio != sensors.end())
        {
            updateAggregateSensor(iio->second,
                                  &peci::metrics::Utilization::iio);
        }
    }

    template <typename T, typename U>
    void updateAggregateSensor(std::shared_ptr<Sensor>& sensor, T U::*counter)
    {
        try
        {
            if (auto value = aggregateUtilization(counter))
            {
                constexpr auto e = boost::system::errc::success;
                sensor->update(boost::system::errc::make_error_code(e), *value);
            }
        }
        catch (const peci::Exception& e)
        {
            LOG_ERROR << "Aggregation failed for sensor: " << sensor->getName()
                      << ", " << e.what();

            auto value = std::numeric_limits<double>::quiet_NaN();
            constexpr auto err = boost::system::errc::io_error;
            sensor->update(boost::system::errc::make_error_code(err), value);
        }
    }

    template <typename T, typename U>
    std::optional<double> aggregateUtilization(T U::*counter)
    {
        double totalDelta = 0;
        double totalMax = 0;

        for (auto& util : utilization)
        {
            if (util)
            {
                const auto& ret = (*util.*counter).delta();
                if (!ret)
                {
                    return std::nullopt;
                }

                const auto& [delta, maxUtil] = *ret;
                totalDelta += delta;
                totalMax += maxUtil;
            }
        }
        LOG_DEBUG_T("Total")
            << "Utilization = " << T::to_string(totalDelta, totalMax);

        return peci::metrics::convertToPercent(totalDelta, totalMax);
    }

    void updateUtilizationForHostOff()
    {
        auto core = sensors.find(Type::Core);
        if (core != sensors.end())
        {
            updateSensorDown(core->second);
        }

        auto memory = sensors.find(Type::Memory);
        if (memory != sensors.end())
        {
            updateSensorDown(memory->second);
        }

        auto iio = sensors.find(Type::Iio);
        if (memory != sensors.end())
        {
            updateSensorDown(iio->second);
        }
    }

    void updateSensorDown(std::shared_ptr<Sensor> sensor)
    {
        auto value = std::numeric_limits<double>::quiet_NaN();
        constexpr auto err = boost::system::errc::success;
        sensor->update(boost::system::errc::make_error_code(err), value);
    }

    void updateCupsIndex()
    {
        auto cupsIndex = sensors.find(Type::CupsIndex);
        if (cupsIndex != sensors.end())
        {
            constexpr auto e = boost::system::errc::success;

            updateDynamicLoadFactors();

            cupsIndex->second->update(
                boost::system::errc::make_error_code(e),
                cupsIndexUtilization(getActiveLoadFactorCfg())
                    .value_or(std::numeric_limits<double>::quiet_NaN()));
        }
    }

    std::optional<double> cupsIndexUtilization(const LoadFactors& loadFactors)
    {
        auto coreUtilization = getSensorValue(Type::Core);
        auto iioUtilization = getSensorValue(Type::Iio);
        auto memoryUtilization = getSensorValue(Type::Memory);
        if (!coreUtilization || !iioUtilization || !memoryUtilization)
        {
            return std::nullopt;
        }

        double cupsIndexUtilization;

        cupsIndexUtilization =
            ((*coreUtilization * loadFactors.coreLoadFactor) +
             (*iioUtilization * loadFactors.iioLoadFactor) +
             (*memoryUtilization * loadFactors.memoryLoadFactor)) /
            100;

        return cupsIndexUtilization;
    }

    void updateDynamicLoadFactors()
    {
        auto coreAverage = getSensorValue(Type::AverageCore);
        auto iioAverage = getSensorValue(Type::AverageIio);
        auto memoryAverage = getSensorValue(Type::AverageMemory);
        if (!coreAverage || !iioAverage || !memoryAverage)
        {
            dynamicLoadFactors = {};
            return;
        }

        auto divider = *coreAverage + *iioAverage + *memoryAverage;

        dynamicLoadFactors.coreLoadFactor =
            peci::metrics::convertToPercent(*coreAverage, divider);
        dynamicLoadFactors.iioLoadFactor =
            peci::metrics::convertToPercent(*iioAverage, divider);
        dynamicLoadFactors.memoryLoadFactor =
            peci::metrics::convertToPercent(*memoryAverage, divider);
    }

    std::optional<double> getSensorValue(const Type& type)
    {
        auto sensor = sensors.find(type);
        if (sensor != sensors.end())
        {
            return sensor->second->getValue();
        }

        return std::nullopt;
    }
};

} // namespace base

} // namespace cups
