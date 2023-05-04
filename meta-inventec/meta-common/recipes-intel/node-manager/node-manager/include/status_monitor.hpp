/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020-2022 Intel Corporation.
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
#include "readings/reading_event.hpp"

#include <functional>
#include <iostream>
#include <memory>

namespace nodemanager
{

constexpr std::array kCriticalReadingTypes{
    ReadingType::acPlatformPower,  ReadingType::dcPlatformPower,
    ReadingType::cpuPackagePower,  ReadingType::dramPower,
    ReadingType::inletTemperature, ReadingType::hostReset,
    ReadingType::cpuUtilization};

class StatusMonitor : public RunnerIf
{
  public:
    class ActionIf
    {
      public:
        virtual ~ActionIf() = default;
        virtual void logSensorMissing(std::string sensorReadingTypeName,
                                      DeviceIndex deviceIndex) = 0;
        virtual void logSensorReadingMissing(std::string sensorReadingTypeName,
                                             DeviceIndex deviceIndex) = 0;
        virtual void logReadingMissing(std::string readingTypeName) = 0;
        virtual void
            logNonCriticalReadingMissing(std::string readingTypeName) = 0;
    };

    StatusMonitor(const StatusMonitor&) = delete;
    StatusMonitor& operator=(const StatusMonitor&) = delete;
    StatusMonitor(StatusMonitor&&) = delete;
    StatusMonitor& operator=(StatusMonitor&&) = delete;

    StatusMonitor(std::shared_ptr<DevicesManagerIf> devicesManagerArg,
                  std::shared_ptr<ActionIf> actionHandlerArg) :
        devicesManager(devicesManagerArg),
        actionHandler(actionHandlerArg)
    {
        installReadings();
    }

    ~StatusMonitor()
    {
        unregisterReadings();
    }

    virtual void run() override final
    {
        auto perf =
            Perf("StatusMonitor-run-duration", std::chrono::milliseconds{20});
        if (!actionHandler)
        {
            return;
        }
        for (auto& [key, context] : sensorPresence)
        {
            if (context.everAppeared && !context.actionWasTriggered &&
                isTimeWindowReached(context.timestamp))
            {
                context.actionWasTriggered = true;
                actionHandler->logSensorMissing(
                    enumToStr(kSensorReadingTypeNames, key.first), key.second);
            }
        }

        for (auto& [key, context] : sensorReadingPresence)
        {
            if (context.everAppeared && !context.actionWasTriggered &&
                context.timestamp)
            {
                context.actionWasTriggered = true;
                actionHandler->logSensorReadingMissing(
                    enumToStr(kSensorReadingTypeNames, key.first), key.second);
            }
        }

        for (auto& [key, context] : readingPresence)
        {
            if (context.everAppeared && !context.actionWasTriggered &&
                isTimeWindowReached(context.timestamp))
            {
                context.actionWasTriggered = true;
                if (std::end(kCriticalReadingTypes) ==
                    std::find(std::begin(kCriticalReadingTypes),
                              std::end(kCriticalReadingTypes), key))
                {
                    actionHandler->logNonCriticalReadingMissing(
                        enumToStr(kReadingTypeNames, key));
                }
                else
                {

                    actionHandler->logReadingMissing(
                        enumToStr(kReadingTypeNames, key));
                }
            }
        }
    }

  private:
    struct EventContext
    {
        std::optional<Clock::time_point> timestamp;
        bool actionWasTriggered;
        bool everAppeared;
    };

    bool isTimeWindowReached(const std::optional<Clock::time_point>& timestamp)
    {
        return timestamp && (Clock::now() - *timestamp > monitoringWindow);
    }

    void unregisterReadings()
    {
        for (const auto& reading : readings)
        {
            devicesManager->unregisterReadingConsumer(reading);
        }
    }

    /**
     * @brief Function creates all required ReadingEvent and installs them
     * in DeviceManager.
     */
    void installReadings(void)
    {
        EventCallback sensorEventCallback =
            [this](SensorEventType eventType, const SensorContext& sensorCtx,
                   const ReadingContext& readingCtx) {
                auto key =
                    std::make_pair(sensorCtx.type, sensorCtx.deviceIndex);

                if (eventType == SensorEventType::sensorDisappear)
                {
                    bool everLogged =
                        sensorPresence.find(key) != sensorPresence.end();
                    EventContext& context = sensorPresence[key];
                    if (!context.timestamp)
                    {
                        context = {Clock::now(), false,
                                   everLogged && context.everAppeared};
                    }
                }
                else if (eventType == SensorEventType::sensorAppear)
                {
                    sensorPresence[std::make_pair(
                        sensorCtx.type, sensorCtx.deviceIndex)] = {std::nullopt,
                                                                   false, true};
                }

                if (eventType == SensorEventType::readingMissing)
                {
                    bool everLogged = sensorReadingPresence.find(key) !=
                                      sensorReadingPresence.end();
                    EventContext& context = sensorReadingPresence[key];
                    if (!context.timestamp)
                    {
                        context = {Clock::now(), false,
                                   everLogged && context.everAppeared};
                    }
                }
                else if (eventType == SensorEventType::readingAvailable)
                {
                    sensorReadingPresence[std::make_pair(
                        sensorCtx.type, sensorCtx.deviceIndex)] = {std::nullopt,
                                                                   false, true};
                }
            };

        ReadingEventCallback readingEventCallback = [this](
                                                        const ReadingEventType
                                                            eventType,
                                                        const ReadingContext&
                                                            readingCtx) {
            if (readingCtx.deviceIndex != kAllDevices)
            {
                throw std::logic_error("Invalid reading event in SM");
            }

            if (eventType == ReadingEventType::readingUnavailable)
            {
                if (!readingPresence[readingCtx.type].timestamp)
                {
                    readingPresence[readingCtx.type] = {
                        Clock::now(), false,
                        readingPresence[readingCtx.type].everAppeared};
                }
            }
            else if (eventType == ReadingEventType::readingAvailable)
            {
                readingPresence[readingCtx.type] = {std::nullopt, false, true};
            }
        };

        constexpr std::array readingTypes{
            ReadingType::acPlatformPower,   ReadingType::dcPlatformPower,
            ReadingType::cpuPackagePower,   ReadingType::dramPower,
            ReadingType::pciePower,         ReadingType::totalChassisPower,
            ReadingType::inletTemperature,  ReadingType::outletTemperature,
            ReadingType::volumetricAirflow, ReadingType::hostReset,
            ReadingType::cpuUtilization};

        for (const auto readingType : readingTypes)
        {
            devicesManager->registerReadingConsumer(
                readings.emplace_back(std::make_shared<ReadingEvent>(
                    [](double value) {}, sensorEventCallback,
                    readingEventCallback)),
                readingType);
        }
    }

    std::shared_ptr<DevicesManagerIf> devicesManager;
    std::vector<std::shared_ptr<ReadingEvent>> readings;
    std::shared_ptr<ActionIf> actionHandler;
    std::chrono::milliseconds monitoringWindow{20000};
    std::map<std::pair<SensorReadingType, DeviceIndex>, EventContext>
        sensorPresence;
    std::map<std::pair<SensorReadingType, DeviceIndex>, EventContext>
        sensorReadingPresence;
    std::map<ReadingType, EventContext> readingPresence;
};

class StatusMonitorActions : public StatusMonitor::ActionIf
{
  public:
    StatusMonitorActions(const StatusMonitorActions&) = delete;
    StatusMonitorActions& operator=(const StatusMonitorActions&) = delete;
    StatusMonitorActions(StatusMonitorActions&&) = delete;
    StatusMonitorActions& operator=(StatusMonitorActions&&) = delete;

    StatusMonitorActions() = default;
    virtual ~StatusMonitorActions() = default;
    virtual void logSensorMissing(std::string sensorReadingTypeName,
                                  DeviceIndex deviceIndex) final
    {
        RedfishLogger::logSensorMissing(sensorReadingTypeName, deviceIndex);
        Logger::log<LogLevel::warning>(
            "[StatusMonitor]: Sensor %s-%d is missing", sensorReadingTypeName,
            unsigned{deviceIndex});
    }

    virtual void logSensorReadingMissing(std::string sensorReadingTypeName,
                                         DeviceIndex deviceIndex) final
    {
        Logger::log<LogLevel::warning>(
            "[StatusMonitor]: Sensor %s-%d value is invalid",
            sensorReadingTypeName, unsigned{deviceIndex});
    }

    virtual void logReadingMissing(std::string readingTypeName) final
    {
        RedfishLogger::logReadingMissing(readingTypeName);
        Logger::log<LogLevel::critical>(
            "[StatusMonitor]: All %s readings disappeared", readingTypeName);
    }

    virtual void logNonCriticalReadingMissing(std::string readingTypeName) final
    {
        RedfishLogger::logNonCriticalReadingMissing(readingTypeName);
        Logger::log<LogLevel::warning>(
            "[StatusMonitor]: All %s readings disappeared", readingTypeName);
    }
};
} // namespace nodemanager
