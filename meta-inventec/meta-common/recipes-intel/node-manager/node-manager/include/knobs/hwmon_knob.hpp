/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020 Intel Corporation.
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
#include "loggers/log.hpp"
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

static constexpr const auto kPowerLimitFilename = "power1_cap";

/**
 * @brief HwmonKnob class implements Knob to be used with Hwmon files.
 *
 */
class HwmonKnob : public AsyncKnob
{

  public:
    HwmonKnob(KnobType typeArg, DeviceIndex deviceIndexArg,
              uint32_t minValueArg, uint32_t maxValueArg,
              std::shared_ptr<HwmonFileProviderIf> hwmonProviderArg,
              std::shared_ptr<AsyncKnobExecutor> asyncExecutorArg,
              const std::shared_ptr<SensorReadingsManagerIf>&
                  sensorReadingsManagerArg) :
        AsyncKnob(typeArg, deviceIndexArg, asyncExecutorArg,
                  sensorReadingsManagerArg),
        hwmonProvider(hwmonProviderArg), minValue(minValueArg),
        maxValue(maxValueArg)
    {
    }

    virtual ~HwmonKnob() = default;

    bool isKnobEndpointAvailable() const override
    {
        std::filesystem::path filePath =
            hwmonProvider->getFile(getKnobType(), getDeviceIndex());
        if (getKnobType() == KnobType::PciePower)
        {
            return !filePath.empty() &&
                   sensorReadingsManager->isGpuPowerStateOn();
        }
        else
        {
            return !filePath.empty() && Knob::isKnobEndpointAvailable();
        }
    }

    /**
     * @brief Sets value in the knob.
     *
     * @param valueToBeSet Value to be set, in watts. The minimum available
     * value is 1 mW. The min value here comes from hwmon implementation.
     * Value of 0 is interpreted as removing limit. Thus, we use 1 mW as a
     * minimum here.
     */
    void setKnob(const double valueToBeSet)
    {
        auto milliwatts =
            safeCast<uint32_t>((std::chrono::duration_cast<
                                    std::chrono::duration<double, std::milli>>(
                                    Unity{valueToBeSet}))
                                   .count(),
                               std::numeric_limits<uint32_t>::max());

        auto clampedMilliwatts = std::clamp(milliwatts, minValue, maxValue);
        valueToSave = clampedMilliwatts;
    }

    /**
     * @brief Resets value in a hwmon file by putting 0 value there. Hwmon
     * while receiving 0, disables limiting.
     *
     */
    void resetKnob()
    {
        valueToSave = 0;
    }

    bool isKnobSet() const override
    {
        return lastSavedValue.has_value() && (*lastSavedValue != 0);
    }

    virtual NmHealth getHealth() const override
    {
        std::filesystem::path filePath =
            hwmonProvider->getFile(getKnobType(), getDeviceIndex());
        if (filePath.empty())
        {
            return NmHealth::ok;
        }
        return Knob::getHealth();
    }

    virtual void reportStatus(nlohmann::json& out) const override
    {
        auto type = enumToStr(knobTypeNames, getKnobType());
        nlohmann::json tmp;
        tmp["Health"] = enumToStr(healthNames, getHealth());
        tmp["HwmonPath"] =
            hwmonProvider->getFile(getKnobType(), getDeviceIndex());
        tmp["Status"] = lastState;
        tmp["DeviceIndex"] = getDeviceIndex();
        tmp["Value"] = optionalToJson(lastSavedValue);
        out["Knobs-hwmon"][type].push_back(tmp);
    }

  protected:
    virtual AsyncKnobExecutor::Task getTask() const override
    {
        std::filesystem::path filePath =
            hwmonProvider->getFile(getKnobType(), getDeviceIndex());
        return [filePath, value = *valueToSave]()
                   -> std::pair<std::ios_base::iostate, uint32_t> {
            std::ofstream powerLimitFile(filePath);
            if (!powerLimitFile.good())
            {
                return {powerLimitFile.rdstate(), value};
            }
            powerLimitFile << value;
            if (!powerLimitFile.good())
            {
                return {powerLimitFile.rdstate(), value};
            }
            powerLimitFile.flush();
            if (!powerLimitFile.good())
            {
                return {powerLimitFile.rdstate(), value};
            }
            powerLimitFile.close();
            return {powerLimitFile.rdstate(), value};
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
    std::shared_ptr<HwmonFileProviderIf> hwmonProvider;
    std::filesystem::path devicesDirectoryPath;
    uint32_t minValue;
    uint32_t maxValue;
};

} // namespace nodemanager