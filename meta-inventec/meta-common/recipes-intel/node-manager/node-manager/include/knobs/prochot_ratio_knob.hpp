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
#include "config/config.hpp"
#include "devices_manager/hwmon_file_provider.hpp"
#include "sensors/peci/peci_commands.hpp"
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

static constexpr const uint8_t kDefaultProchotValue = 0;

class ProchotRatioKnob : public AsyncKnob
{

  public:
    ProchotRatioKnob(KnobType typeArg, DeviceIndex deviceIndexArg,
                     std::shared_ptr<PeciCommandsIf> peciIfArg,
                     std::shared_ptr<AsyncKnobExecutor> asyncExecutorArg,
                     const std::shared_ptr<SensorReadingsManagerIf>&
                         sensorReadingsManagerArg) :
        AsyncKnob(typeArg, deviceIndexArg, asyncExecutorArg,
                  sensorReadingsManagerArg),
        peciIf(peciIfArg)
    {
    }

    virtual ~ProchotRatioKnob() = default;

    void setKnob(const double valueToBeSet)
    {
        if (isCastSafe<uint8_t>(valueToBeSet))
        {
            valueToSave = valueToBeSet;
        }
        else
        {
            throw std::logic_error("ProchotRatio value out of range, "
                                   "expected max of uint8_t while we have: " +
                                   std::to_string(valueToBeSet));
        }
    }

    void resetKnob()
    {
        if (defaultValue)
        {
            valueToSave = *defaultValue;
        }
        else
        {
            auto confValue =
                Config::getInstance().getGeneralPresets().prochotAssertionRatio;
            if (confValue != 0)
            {
                Logger::log<LogLevel::debug>(
                    "Getting default prochot assert ratio from cfg, value: %d",
                    unsigned{confValue});
                defaultValue = confValue;
                valueToSave = *defaultValue;
                return;
            }
            auto readTask = [cpuIndex = getDeviceIndex(), peci = peciIf]()
                -> std::pair<std::ios_base::iostate, uint32_t> {
                if (const auto cpuId = peci->getCpuId(cpuIndex))
                {
                    if (auto efficiencyRatio =
                            peci->getMaxEfficiencyRatio(cpuIndex, *cpuId))
                    {
                        return {std::ios_base::goodbit,
                                uint32_t{*efficiencyRatio}};
                    }
                }
                return {std::ios_base::badbit, uint32_t{kDefaultProchotValue}};
            };
            auto readTaskCallback =
                [this](std::pair<std::ios_base::iostate, uint32_t> results) {
                    const auto [status, value] = results;
                    if (status == std::ios_base::goodbit)
                    {
                        defaultValue = static_cast<uint8_t>(value);
                        valueToSave = *defaultValue;
                    }
                    else
                    {
                        valueToSave = uint32_t{kDefaultProchotValue};
                    }
                };
            asyncExecutor->schedule({getKnobType(), getDeviceIndex()},
                                    std::move(readTask), readTaskCallback);
        }
    }

    bool isKnobSet() const override
    {
        return lastSavedValue.has_value() &&
               (!defaultValue.has_value() ||
                (defaultValue.has_value() &&
                 (*lastSavedValue != *defaultValue)));
    }

    virtual void reportStatus(nlohmann::json& out) const override
    {
        auto type = enumToStr(knobTypeNames, getKnobType());
        nlohmann::json tmp;
        tmp["Health"] = enumToStr(healthNames, getHealth());
        tmp["Status"] = lastState;
        tmp["DeviceIndex"] = getDeviceIndex();
        tmp["Value"] = optionalToJson(lastSavedValue);
        tmp["DefaultValue"] = optionalToJson(defaultValue);
        out["Knobs-peci"][type].push_back(tmp);
    }

  protected:
    virtual AsyncKnobExecutor::Task getTask() const override
    {
        return
            [cpuId = getDeviceIndex(),
             ratioValue = static_cast<uint8_t>(*valueToSave),
             peci = peciIf]() -> std::pair<std::ios_base::iostate, uint32_t> {
                if (peci->setProchotRatio(cpuId, ratioValue))
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
    std::optional<uint8_t> defaultValue = std::nullopt;
};

} // namespace nodemanager