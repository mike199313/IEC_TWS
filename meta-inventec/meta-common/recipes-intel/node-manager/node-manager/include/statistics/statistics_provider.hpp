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
#include "devices_manager/devices_manager.hpp"
#include "statistics/statistic_if.hpp"
#include "utility/dbus_interfaces.hpp"

#include <memory>

namespace nodemanager
{

class StatisticsProvider
{
  public:
    StatisticsProvider(std::shared_ptr<DevicesManagerIf> devicesManagerArg) :
        devicesManager(std::move(devicesManagerArg)){};

    virtual ~StatisticsProvider()
    {
        removeAllStatistics();
    }

  private:
    std::vector<std::shared_ptr<StatisticIf>> statistics;
    std::shared_ptr<DevicesManagerIf> devicesManager;

  public:
    void addStatistics(std::shared_ptr<StatisticIf> newStat, ReadingType type,
                       DeviceIndex index = kAllDevices)
    {
        Logger::log<LogLevel::debug>("Registering statistics: %s",
                                     newStat->getName());
        devicesManager->registerReadingConsumer(newStat, type, index);
        statistics.push_back(std::move(newStat));
    }

    void removeAllStatistics()
    {
        Logger::log<LogLevel::debug>("Unregistering statistics");
        for (const auto& stat : statistics)
        {
            devicesManager->unregisterReadingConsumer(stat);
        }
        statistics.clear();
    }

    void initializeDbusInterfaces(DbusInterfaces& dbusInterfaces)
    {
        dbusInterfaces.addInterface(
            "xyz.openbmc_project.NodeManager.Statistics", [this](auto& iface) {
                iface.register_method("ResetStatistics", [this]() {
                    for (const std::shared_ptr<StatisticIf>& stat : statistics)
                    {
                        stat->reset();
                    }
                });
                iface.register_method("GetStatistics", [this]() {
                    std::map<std::string, StatValuesMap> retMap;
                    for (const std::shared_ptr<StatisticIf>& stat : statistics)
                    {
                        retMap.emplace(stat->getName(), stat->getValuesMap());
                    }
                    return retMap;
                });
            });
    }

    void enableStatisticsCalculation()
    {
        for (const std::shared_ptr<StatisticIf>& stat : statistics)
        {
            stat->enableStatisticCalculation();
        }
    }

    void disableStatisticsCalculation()
    {
        for (const std::shared_ptr<StatisticIf>& stat : statistics)
        {
            stat->disableStatisticCalculation();
        }
    }
};

} // namespace nodemanager