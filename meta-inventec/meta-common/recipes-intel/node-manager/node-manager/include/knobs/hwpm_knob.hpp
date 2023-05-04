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

static constexpr uint32_t kPreferenceReservedFieldsMask = 0x00FF0000;
static constexpr uint32_t kBiasReservedFieldsMask = 0xFFFFFFF0;
static constexpr uint32_t kOverrideReservedFieldsMask = 0xFFFFFFF8;

class HwpmKnob : public AsyncKnob
{

  public:
    HwpmKnob(KnobType typeArg, DeviceIndex deviceIndexArg,
             uint32_t defaultValueArg,
             std::shared_ptr<PeciCommandsIf> peciIfArg,
             std::shared_ptr<AsyncKnobExecutor> asyncExecutorArg,
             const std::shared_ptr<SensorReadingsManagerIf>&
                 sensorReadingsManagerArg) :
        AsyncKnob(typeArg, deviceIndexArg, asyncExecutorArg,
                  sensorReadingsManagerArg),
        defaultValue(defaultValueArg), peciIf(peciIfArg)
    {
    }

    virtual ~HwpmKnob() = default;

    /**
     * @brief Sets value in the knob.
     *
     * @param valueToBeSet Value to be set in hardware by @operation
     *        lambda function passed in ctor. Value should be castable
     *        to uint32_t. All lambdas passed to ctor should accept uint32_t.
     */
    void setKnob(const double valueToBeSet)
    {
        if (!isCastSafe<uint32_t>(valueToBeSet))
        {
            throw std::logic_error(
                "Value passed to performance knob out of range, "
                "expected max of uint32_t while we have: " +
                std::to_string(valueToBeSet));
        }

        if (!reservedFieldsEmpty(getKnobType(),
                                 static_cast<uint32_t>(valueToBeSet)))
        {
            throw std::logic_error("Reserved fields in the value passed to "
                                   "performance knob are not empty");
        }

        valueToSave = valueToBeSet;
    }

    /**
     * @brief Resets value of the performance knob by setting default value
     *        passed to ctor.
     */
    void resetKnob()
    {
        valueToSave = defaultValue;
    }

    bool isKnobSet() const override
    {
        return lastSavedValue.has_value() && (*lastSavedValue != defaultValue);
    }

    virtual void reportStatus(nlohmann::json& out) const override
    {
        auto type = enumToStr(knobTypeNames, getKnobType());
        nlohmann::json tmp;
        tmp["Health"] = enumToStr(healthNames, getHealth());
        tmp["Status"] = lastState;
        tmp["DeviceIndex"] = getDeviceIndex();
        tmp["Value"] = optionalToJson(lastSavedValue);
        tmp["DefaultValue"] = defaultValue;
        out["Knobs-peci"][type].push_back(tmp);
    }

  protected:
    virtual AsyncKnobExecutor::Task getTask() const override
    {
        return [cpuId = getDeviceIndex(), value = *valueToSave, peci = peciIf,
                type = getKnobType()]()
                   -> std::pair<std::ios_base::iostate, uint32_t> {
            bool ret;
            switch (type)
            {
                case KnobType::HwpmPerfPreference:
                    ret = peci->setHwpmPreference(cpuId, value);
                    break;
                case KnobType::HwpmPerfBias:
                    ret = peci->setHwpmPreferenceBias(cpuId, value);
                    break;
                case KnobType::HwpmPerfPreferenceOverride:
                    ret = peci->setHwpmPreferenceOverride(cpuId, value);
                    break;
                default:
                    throw std::logic_error("Invalid type set in the HwpmKnob");
            }
            if (ret)
            {
                return {std::ios_base::goodbit, uint32_t{value}};
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
    bool reservedFieldsEmpty(KnobType type, uint32_t value)
    {
        switch (type)
        {
            case KnobType::HwpmPerfPreference:
                return (value & kPreferenceReservedFieldsMask) == 0;
            case KnobType::HwpmPerfBias:
                return (value & kBiasReservedFieldsMask) == 0;
            case KnobType::HwpmPerfPreferenceOverride:
                return (value & kOverrideReservedFieldsMask) == 0;
            default:
                throw std::logic_error("Invalid type set in the HwpmKnob");
        }
    }

    uint32_t defaultValue;
    std::shared_ptr<PeciCommandsIf> peciIf;
};

} // namespace nodemanager
