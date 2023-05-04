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

#include "flow_control.hpp"
#include "sensors/sensor_readings_manager.hpp"
#include "utility/async_executor.hpp"
#include "utility/status_provider_if.hpp"

#include <memory>

namespace nodemanager
{

enum class KnobType
{
    DcPlatformPower,
    CpuPackagePower,
    DramPower,
    PciePower,
    TurboRatioLimit,
    Prochot,
    HwpmPerfPreference,
    HwpmPerfBias,
    HwpmPerfPreferenceOverride
};

static const std::unordered_map<KnobType, std::string> knobTypeNames = {
    {KnobType::DcPlatformPower, "DcPlatformPower"},
    {KnobType::CpuPackagePower, "CpuPackagePower"},
    {KnobType::DramPower, "DramPower"},
    {KnobType::PciePower, "PciePower"},
    {KnobType::TurboRatioLimit, "TurboRatioLimit"},
    {KnobType::Prochot, "Prochot"},
    {KnobType::HwpmPerfPreference, "HwpmPerfPreference"},
    {KnobType::HwpmPerfBias, "HwpmPerfBias"},
    {KnobType::HwpmPerfPreferenceOverride, "HwpmPerfPreferenceOverride"},
};

class KnobIf : public StatusProviderIf, public RunnerIf
{
  public:
    virtual ~KnobIf() = default;
    virtual void setKnob(const double valueToBeSet) = 0;
    virtual void resetKnob() = 0;
    virtual bool isKnobSet() const = 0;
    virtual KnobType getKnobType() const = 0;
    virtual DeviceIndex getDeviceIndex() const = 0;
};

class Knob : public KnobIf
{
  public:
    Knob() = delete;
    Knob(const Knob&) = delete;
    Knob& operator=(const Knob&) = delete;
    Knob(Knob&&) = delete;
    Knob& operator=(Knob&&) = delete;

    Knob(KnobType typeArg, DeviceIndex deviceIndexArg,
         const std::shared_ptr<SensorReadingsManagerIf>&
             sensorReadingsManagerArg) :
        knobType(typeArg),
        deviceIndex(deviceIndexArg),
        sensorReadingsManager(sensorReadingsManagerArg)
    {
    }

    virtual ~Knob()
    {
    }

    virtual void run() override
    {
        if (!isKnobEndpointAvailable())
        {
            lastState = std::ios_base::goodbit;
            lastSavedValue = std::nullopt;
            return;
        }

        if (isSomethingToWrite())
        {
            writeValue();
        }
    }

    KnobType getKnobType() const override
    {
        return knobType;
    }

    DeviceIndex getDeviceIndex() const override
    {
        return deviceIndex;
    }

    virtual NmHealth getHealth() const override
    {
        if (lastState != std::ios_base::goodbit)
        {
            return NmHealth::warning;
        }
        return NmHealth::ok;
    }

  protected:
    virtual bool isSomethingToWrite() const
    {
        if (valueToSave.has_value() && (valueToSave != lastSavedValue))
        {
            return true;
        }
        return false;
    }

    virtual bool isKnobEndpointAvailable() const
    {
        return sensorReadingsManager->isPowerStateOn() &&
               sensorReadingsManager->isCpuAvailable(getDeviceIndex());
    }

    virtual void writeValue() = 0;

    template <class T>
    nlohmann::json optionalToJson(const std::optional<T>& value) const
    {
        if (value)
        {
            return *value;
        }
        return nullptr;
    }

    KnobType knobType;
    DeviceIndex deviceIndex;
    std::ios_base::iostate lastState = std::ios_base::goodbit;
    std::optional<uint32_t> valueToSave = std::nullopt;
    std::optional<uint32_t> lastSavedValue = std::nullopt;
    std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManager;
};

} // namespace nodemanager