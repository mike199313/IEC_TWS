/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2022 Intel Corporation.
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

#include "async_knob.hpp"
#include "devices_manager/hwmon_file_provider.hpp"
#include "utility/async_executor.hpp"
#include "utility/ranges.hpp"
#include "utility/units.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace nodemanager
{

class TurboRatioKnob : public AsyncKnob
{

  public:
    TurboRatioKnob(KnobType typeArg, DeviceIndex deviceIndexArg,
                   std::shared_ptr<PeciCommandsIf> peciIfArg,
                   std::shared_ptr<AsyncKnobExecutor> asyncExecutorArg,
                   const std::shared_ptr<SensorReadingsManagerIf>&
                       sensorReadingsManagerArg) :
        AsyncKnob(typeArg, deviceIndexArg, asyncExecutorArg,
                  sensorReadingsManagerArg),
        peciIf(peciIfArg)
    {
    }

    virtual ~TurboRatioKnob() = default;

    /**
     * @brief Sets value in the knob.
     *
     * @param valueToBeSet Value must be castable to uint8_t since this
     * value is acceptable by PCS#50 that is used to set the turbo ratio limit.
     */
    void setKnob(const double valueToBeSet)
    {
        if (isCastSafe<uint8_t>(valueToBeSet))
        {
            valueToSave = valueToBeSet;
        }
        else
        {
            throw std::logic_error("TurboRatioLimit value out of range, "
                                   "expected max of uint8_t while we have: " +
                                   std::to_string(valueToBeSet));
        }
    }

    /**
     * @brief Resets value of the turbo ration limit by setting max of uint8_t
     */
    void resetKnob()
    {
        auto confValue =
            Config::getInstance().getGeneralPresets().cpuTurboRatioLimit;
        if (confValue != 0)
        {
            Logger::log<LogLevel::debug>(
                "Getting default turbo ratio limit from cfg, value: %d",
                unsigned{confValue});
            valueToSave = confValue;
        }
        else
        {
            valueToSave = std::numeric_limits<uint8_t>::max();
        }
    }

    bool isKnobSet() const override
    {
        return lastSavedValue.has_value() &&
               (*lastSavedValue != std::numeric_limits<uint8_t>::max());
    }

    virtual void reportStatus(nlohmann::json& out) const override
    {
        auto type = enumToStr(knobTypeNames, getKnobType());
        nlohmann::json tmp;
        tmp["Health"] = enumToStr(healthNames, getHealth());
        tmp["Status"] = lastState;
        tmp["DeviceIndex"] = getDeviceIndex();
        tmp["Value"] = optionalToJson(lastSavedValue);
        out["Knobs-peci"][type].push_back(tmp);
    }

  protected:
    virtual AsyncKnobExecutor::Task getTask() const override
    {
        return
            [cpuId = getDeviceIndex(),
             ratioValue = static_cast<uint8_t>(*valueToSave),
             peci = peciIf]() -> std::pair<std::ios_base::iostate, uint32_t> {
                if (peci->setTurboRatio(cpuId, ratioValue))
                {
                    return {std::ios_base::goodbit, uint32_t{ratioValue}};
                }
                return {std::ios_base::failbit, 0};
            };
    }

    virtual AsyncKnobExecutor::TaskCallback getTaskCallback() override
    {
        return [this](std::pair<std::ios_base::iostate, uint32_t> results) {
            const auto [status, savedValue] = results;
            lastState = status;
            lastSavedValue = (lastState == std::ios_base::goodbit)
                                 ? std::make_optional(savedValue)
                                 : std::nullopt;
        };
    }

  private:
    std::shared_ptr<PeciCommandsIf> peciIf;
};

} // namespace nodemanager