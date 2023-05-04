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

#include "devices_manager/gpio_provider.hpp"
#include "sensor.hpp"

namespace nodemanager
{

class GpioSensor : public Sensor
{
  public:
    GpioSensor(const GpioSensor&) = delete;
    GpioSensor(GpioSensor&&) = delete;
    GpioSensor& operator=(GpioSensor&&) = delete;
    GpioSensor& operator=(const GpioSensor&) = delete;

    GpioSensor(const std::shared_ptr<SensorReadingsManagerIf>&
                   sensorReadingsManagerArg,
               const std::shared_ptr<GpioProviderIf> gpioProviderArg) :
        Sensor(sensorReadingsManagerArg),
        gpioProvider(gpioProviderArg)
    {
        installSensorReadings();
    }

    virtual ~GpioSensor() = default;

    virtual void reportStatus(nlohmann::json& out) const override
    {
        for (std::shared_ptr<SensorReadingIf> sensorReading : readings)
        {
            nlohmann::json tmp;
            tmp["Status"] =
                enumToStr(sensorReadingStatusNames, sensorReading->getStatus());
            tmp["Health"] = enumToStr(healthNames, sensorReading->getHealth());
            auto type = enumToStr(kSensorReadingTypeNames,
                                  sensorReading->getSensorReadingType());
            DeviceIndex index = sensorReading->getDeviceIndex();
            tmp["DeviceIndex"] = index;
            std::visit([&tmp](auto&& value) { tmp["Value"] = value; },
                       sensorReading->getValue());
            tmp["GpioName"] = gpioProvider->getLineName(index);
            out["Sensors-gpio"][type].push_back(tmp);
        }
    }

    void run() override final
    {
        auto perf =
            Perf("SensorSet-Gpio-run-duration", std::chrono::milliseconds{10});
        for (const auto& sensorReading : readings)
        {
            DeviceIndex deviceIndex = sensorReading->getDeviceIndex();

            if (auto state = gpioProvider->getState(deviceIndex))
            {
                sensorReading->setStatus(SensorReadingStatus::valid);
                sensorReading->updateValue(static_cast<double>(
                    std::underlying_type_t<GpioState>(*state)));
            }
            else
            {
                sensorReading->setStatus(SensorReadingStatus::unavailable);
            }
        }
    }

  private:
    std::shared_ptr<GpioProviderIf> gpioProvider;

    void installSensorReadings()
    {
        for (DeviceIndex index = 0; index < gpioProvider->getGpioLinesCount();
             index++)
        {
            readings.emplace_back(sensorReadingsManager->createSensorReading(
                SensorReadingType::gpioState, index));
        }
    }
};

} // namespace nodemanager