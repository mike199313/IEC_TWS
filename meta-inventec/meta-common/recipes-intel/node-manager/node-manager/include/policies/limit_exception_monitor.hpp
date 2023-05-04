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
#include "devices_manager/devices_manager.hpp"
#include "flow_control.hpp"
#include "limit_exception_handler.hpp"
#include "policy_if.hpp"
#include "readings/reading_event.hpp"
#include "statistics/statistic.hpp"
#include "utility/property_wrapper.hpp"

namespace nodemanager
{

class LimitExceptionMonitor : public RunnerIf
{
  public:
    LimitExceptionMonitor() = delete;
    LimitExceptionMonitor(const LimitExceptionMonitor&) = delete;
    LimitExceptionMonitor& operator=(const LimitExceptionMonitor&) = delete;
    LimitExceptionMonitor(LimitExceptionMonitor&&) = delete;
    LimitExceptionMonitor& operator=(LimitExceptionMonitor&&) = delete;

    LimitExceptionMonitor(
        std::weak_ptr<PolicyIf> policyArg,
        std::shared_ptr<LimitExceptionHandlerIf> actionArg,
        std::shared_ptr<DevicesManagerIf> devicesManagerArg,
        ReadingType readingTypeArg, const PropertyWrapper<uint16_t>& limitArg,
        const PropertyWrapper<uint32_t>& correctionTimeArg,
        const PropertyWrapper<LimitException>& limitExceptionArg) :
        policy(policyArg),
        actionHandler(actionArg), devicesManager(devicesManagerArg),
        readingType(readingTypeArg), limit(limitArg),
        correctionTime(correctionTimeArg), limitException(limitExceptionArg)
    {
        reset();
        readingEvent = std::make_shared<ReadingEvent>(
            [this](double value) { monitoredValue = value; });
        devicesManager->registerReadingConsumer(readingEvent, readingType);
    }

    ~LimitExceptionMonitor()
    {
        devicesManager->unregisterReadingConsumer(readingEvent);
    };

    void reset()
    {
        timestamp = std::nullopt;
        isActionCalled = false;
        isActionFinished = true;
    }

    void run() override final
    {
        if (!devicesManager || !actionHandler)
        {
            return;
        }

        if (auto policySp = policy.lock())
        {
            if (policySp->getState() != PolicyState::selected)
            {
                timestamp = std::nullopt;
                isActionCalled = false;
                return;
            }
        }
        else
        {
            return;
        }

        if (std::isnan(monitoredValue))
        {
            return;
        }

        double limitOffset = getLimitOffset();
        if (!timestamp && (monitoredValue > limitOffset))
        {
            timestamp = Clock::now();
        }
        else if (timestamp && monitoredValue <= limitOffset)
        {
            timestamp = std::nullopt;
            isActionCalled = false;
        }

        if (!isActionCalled && isActionFinished && isTimeExceeded())
        {
            actionHandler->doAction(
                limitException.get(),
                [this](const boost::system::errc::errc_t err) {
                    if (err)
                    {
                        Logger::log<LogLevel::warning>(
                            "LimitExceptionAction finished with failure: %s",
                            boost::system::errc::make_error_code(err)
                                .message());
                    }
                    isActionFinished = true;
                });
            isActionFinished = false;
            isActionCalled = true;
        }
    }

  private:
    bool isTimeExceeded()
    {
        if (timestamp)
        {
            std::chrono::milliseconds correctionWindow =
                std::chrono::milliseconds{correctionTime.get()};
            return (Clock::now() - *timestamp) > correctionWindow;
        }
        return false;
    }

    double getLimitOffset()
    {
        constexpr double limitOffsetCoeff = 1.05;
        constexpr double limitOffsetMinValue = 2.0;
        return std::max(limitOffsetCoeff * limit.get(), limitOffsetMinValue);
    }

    std::weak_ptr<PolicyIf> policy;
    std::shared_ptr<LimitExceptionHandlerIf> actionHandler;
    std::shared_ptr<DevicesManagerIf> devicesManager;
    ReadingType readingType;
    const PropertyWrapper<uint16_t>& limit;
    const PropertyWrapper<uint32_t>& correctionTime;
    const PropertyWrapper<LimitException>& limitException;
    std::optional<Clock::time_point> timestamp;
    bool isActionCalled;
    bool isActionFinished;
    double monitoredValue;
    std::shared_ptr<ReadingEvent> readingEvent;
};

} // namespace nodemanager
