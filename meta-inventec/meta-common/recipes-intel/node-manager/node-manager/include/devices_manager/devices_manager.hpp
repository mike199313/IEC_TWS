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
#include "config/config_values.hpp"
#include "flow_control.hpp"
#include "hwmon_file_provider.hpp"
#include "knobs/hwmon_knob.hpp"
#include "knobs/hwpm_knob.hpp"
#include "knobs/knob.hpp"
#include "knobs/pcie_dbus_knob.hpp"
#include "knobs/prochot_ratio_knob.hpp"
#include "knobs/turbo_ratio_knob.hpp"
#include "loggers/log.hpp"
#include "pldm_entity_provider.hpp"
#include "readings/reading.hpp"
#include "readings/reading_ac_platform_limit.hpp"
#include "readings/reading_average.hpp"
#include "readings/reading_consumer.hpp"
#include "readings/reading_cpu_presence.hpp"
#include "readings/reading_cpu_utilization.hpp"
#include "readings/reading_delta.hpp"
#include "readings/reading_historical_max.hpp"
#include "readings/reading_max.hpp"
#include "readings/reading_min.hpp"
#include "readings/reading_multi_source.hpp"
#include "readings/reading_pcie_presence.hpp"
#include "readings/reading_power_efficiency.hpp"
#include "readings/reading_smbalert_interrupt.hpp"
#include "sensors/cpu_efficiency_sensor.hpp"
#include "sensors/cpu_frequency_sensor.hpp"
#include "sensors/cpu_utilization_sensor.hpp"
#include "sensors/gpio_sensor.hpp"
#include "sensors/gpu_power_state_dbus_sensor.hpp"
#include "sensors/host_power_dbus_sensor.hpp"
#include "sensors/host_reset_dbus_sensor.hpp"
#include "sensors/hwmon_sensor.hpp"
#include "sensors/inlet_temp_dbus_sensor.hpp"
#include "sensors/outlet_temp_dbus_sensor.hpp"
#include "sensors/pcie_dbus_sensor_effecter.hpp"
#include "sensors/pcie_dbus_sensor_sensor.hpp"
#include "sensors/peci/peci_commands.hpp"
#include "sensors/peci_sensor.hpp"
#include "sensors/power_state_dbus_sensor.hpp"
#include "sensors/sensor_readings_manager.hpp"
#include "sensors/smart_status_sensor.hpp"
#include "utility/final_callback.hpp"
#include "utility/performance_monitor.hpp"
#include "utility/status_provider_if.hpp"
#include "utility/types.hpp"

#include <iostream>
#include <sdbusplus/asio/object_server.hpp>

namespace nodemanager
{

static constexpr uint32_t kHwpmKnobsDefault = 0;

class DevicesManagerIf : public ReadingEventDispatcherIf
{
  public:
    virtual ~DevicesManagerIf() = default;
    virtual void setKnobValue(KnobType, DeviceIndex, const double) = 0;
    virtual void resetKnobValue(KnobType, DeviceIndex) = 0;
    virtual std::shared_ptr<ReadingIf>
        findReading(ReadingType readingType) const = 0;
    virtual bool isKnobSet(KnobType, DeviceIndex) const = 0;
};

class DevicesManager : public RunnerIf,
                       public DevicesManagerIf,
                       public StatusProviderIf
{
  public:
    DevicesManager() = delete;
    DevicesManager(const DevicesManager&) = delete;
    DevicesManager& operator=(const DevicesManager&) = delete;
    DevicesManager(DevicesManager&&) = delete;
    DevicesManager& operator=(DevicesManager&&) = delete;
    DevicesManager(
        std::shared_ptr<sdbusplus::asio::connection> busArg,
        const std::shared_ptr<SensorReadingsManagerIf>&
            sensorReadingsManagerArg,
        std::shared_ptr<HwmonFileProviderIf> hwmonFileProviderArg,
        std::shared_ptr<GpioProviderIf> gpioProviderArg,
        std::shared_ptr<PldmEntityProviderIf> pldmEntityProviderArg) :

        bus(busArg),
        sensorReadingsManager(sensorReadingsManagerArg),
        hwmonFileProvider(hwmonFileProviderArg), gpioProvider(gpioProviderArg),
        pldmEntityProvider(pldmEntityProviderArg),
        knobExecutor(
            std::make_shared<
                AsyncExecutor<std::pair<KnobType, DeviceIndex>,
                              std::pair<std::ios_base::iostate, uint32_t>>>(
                busArg))
    {
        installSensors();
        installReadings();
        installKnobs();
    }

    virtual ~DevicesManager()
    {
        for (auto&& knob : knobs)
        {
            knob->resetKnob();
            knob->run();
        }
        Logger::log<LogLevel::info>("DevicesManager destroyed with success");
    }

    NmHealth getHealth() const final
    {
        std::set<NmHealth> allHealth;
        for (auto&& knob : knobs)
        {
            allHealth.insert(knob->getHealth());
        }
        for (const auto& sensor : sensorsVec)
        {
            allHealth.insert(sensor->getHealth());
        }
        return getMostRestrictiveHealth(allHealth);
    }

    void reportStatus(nlohmann::json& out) const final
    {
        std::set<NmHealth> allHealth;
        for (auto&& knob : knobs)
        {
            knob->reportStatus(out["Knobs"]);
            allHealth.insert(knob->getHealth());
        }
        out["Knobs"]["Health"] =
            enumToStr(healthNames, getMostRestrictiveHealth(allHealth));
        allHealth.clear();
        for (const auto& sensor : sensorsVec)
        {
            sensor->reportStatus(out["Sensors"]);
            allHealth.insert(sensor->getHealth());
        }
        out["Sensors"]["Health"] =
            enumToStr(healthNames, getMostRestrictiveHealth(allHealth));
    }

    void run() override final
    {
        auto perf1 = Perf("DeviceManager-sensors-run-duration",
                          std::chrono::milliseconds{20});
        for (const auto& sensors : sensorsVec)
        {
            sensors->run();
        }
        perf1.stopMeasure();
        auto perf2 = Perf("DeviceManager-readings-run-duration",
                          std::chrono::milliseconds{20});
        for (const auto& reading : readings)
        {
            reading->run();
        }

        for (auto&& knob : knobs)
        {
            knob->run();
        }
    }

    void registerReadingConsumer(
        std::shared_ptr<ReadingConsumer> readingConsumers,
        ReadingType readingType, DeviceIndex deviceIndex = kAllDevices) final
    {
        const auto& reading = findReading(readingType);
        if (reading)
        {
            reading->registerReadingConsumer(readingConsumers, readingType,
                                             deviceIndex);
        }
        else
        {
            Logger::log<LogLevel::info>(
                "[DevicesManager]: Reading Type [%s] not supported",
                enumToStr(kReadingTypeNames, readingType));
        }
    }

    void unregisterReadingConsumer(
        std::shared_ptr<ReadingConsumer> readingConsumers) final
    {
        for (const auto& reading : readings)
        {
            reading->unregisterReadingConsumer(readingConsumers);
        }
    }

    void setKnobValue(KnobType knobType, DeviceIndex deviceIndex,
                      const double valueToBeSet) override
    {
        executeKnobAction(knobType, deviceIndex,
                          [valueToBeSet](std::unique_ptr<KnobIf>& knob) {
                              knob->setKnob(valueToBeSet);
                          });
    }

    void resetKnobValue(KnobType knobType, DeviceIndex deviceIndex) override
    {
        executeKnobAction(
            knobType, deviceIndex,
            [](std::unique_ptr<KnobIf>& knob) { knob->resetKnob(); });
    }

    virtual std::shared_ptr<ReadingIf>
        findReading(ReadingType readingType) const override
    {
        const auto& readingIt =
            std::find_if(readings.cbegin(), readings.cend(),
                         [&readingType](std::shared_ptr<Reading> reading) {
                             return (reading->getReadingType() == readingType);
                         });

        if (readingIt != readings.cend())
        {
            return *readingIt;
        }
        return nullptr;
    }

    virtual bool isKnobSet(KnobType knobType,
                           DeviceIndex deviceIndex) const override
    {
        const auto& knobIt = std::find_if(
            knobs.cbegin(), knobs.cend(),
            [knobType, deviceIndex](const std::unique_ptr<KnobIf>& knob) {
                return (knob->getKnobType() == knobType) &&
                       (knob->getDeviceIndex() == deviceIndex);
            });
        if (knobIt != knobs.cend())
        {
            return (*knobIt)->isKnobSet();
        }
        return false;
    }

  private:
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<SensorReadingsManagerIf> sensorReadingsManager;
    std::shared_ptr<HwmonFileProviderIf> hwmonFileProvider;
    std::shared_ptr<GpioProviderIf> gpioProvider;
    std::shared_ptr<PldmEntityProviderIf> pldmEntityProvider;
    std::vector<std::shared_ptr<Reading>> readings;
    std::vector<std::shared_ptr<Sensor>> sensorsVec;
    std::vector<std::unique_ptr<KnobIf>> knobs;
    std::shared_ptr<PeciCommands> peciCommands =
        std::make_shared<PeciCommands>();
    std::shared_ptr<AsyncKnobExecutor> knobExecutor;

    void installReadings()
    {
        constexpr std::array readingTypes{
            ReadingType::cpuPackagePower,
            ReadingType::dramPower,
            ReadingType::pciePower,
            ReadingType::acPlatformPower,
            ReadingType::inletTemperature,
            ReadingType::outletTemperature,
            ReadingType::volumetricAirflow,
            ReadingType::totalChassisPower,
            ReadingType::cpuPackagePowerCapabilitiesMin,
            ReadingType::cpuPackagePowerCapabilitiesMax,
            ReadingType::dramPackagePowerCapabilitiesMax,
            ReadingType::acPlatformPowerCapabilitiesMax,
            ReadingType::hwProtectionPowerCapabilitiesMax,
            ReadingType::hostReset,
            ReadingType::hostPower,
            ReadingType::cpuEfficiency,
            ReadingType::dcPlatformPowerLimit,
            ReadingType::cpuPackagePowerLimit,
            ReadingType::dramPowerLimit,
            ReadingType::gpioState,
            ReadingType::cpuPackageId};

        for (const auto readingType : readingTypes)
        {
            readings.emplace_back(
                std::make_shared<Reading>(sensorReadingsManager, readingType));
        }

        readings.emplace_back(std::make_shared<ReadingDelta>(
            sensorReadingsManager, ReadingType::cpuEnergy, kMaxCpuNumber,
            kMaxEnergySensorReadingValue));

        readings.emplace_back(std::make_shared<ReadingDelta>(
            sensorReadingsManager, ReadingType::dramEnergy, kMaxCpuNumber,
            kMaxEnergySensorReadingValue));

        readings.emplace_back(std::make_shared<ReadingDelta>(
            sensorReadingsManager, ReadingType::dcPlatformEnergy, kMaxCpuNumber,
            kMaxEnergySensorReadingValue));

        if (Config::getInstance().getGeneralPresets().acceleratorsInterface ==
            kPldmInterfaceName)
        {
            readings.emplace_back(std::make_shared<Reading>(
                sensorReadingsManager, ReadingType::pciePowerCapabilitiesMax));
            readings.emplace_back(std::make_shared<Reading>(
                sensorReadingsManager, ReadingType::pciePowerCapabilitiesMin));
        }
        else
        {
            readings.emplace_back(std::make_shared<ReadingHistoricalMax>(
                sensorReadingsManager, ReadingType::pciePowerCapabilitiesMax,
                kMaxPcieNumber));
        }

        readings.emplace_back(
            std::make_shared<ReadingSmbalertInterrupt>(sensorReadingsManager));

        readings.emplace_back(std::make_shared<ReadingMax<uint8_t>>(
            sensorReadingsManager, ReadingType::prochotRatioCapabilitiesMin));

        readings.emplace_back(std::make_shared<ReadingMin<uint8_t>>(
            sensorReadingsManager, ReadingType::prochotRatioCapabilitiesMax));

        readings.emplace_back(std::make_shared<ReadingMin<double>>(
            sensorReadingsManager, ReadingType::dcRatedPowerMin));

        readings.emplace_back(std::make_shared<ReadingMax<uint8_t>>(
            sensorReadingsManager, ReadingType::turboRatioCapabilitiesMin));

        readings.emplace_back(std::make_shared<ReadingMin<uint8_t>>(
            sensorReadingsManager, ReadingType::turboRatioCapabilitiesMax));

        readings.emplace_back(std::make_shared<ReadingAverage<double>>(
            sensorReadingsManager, ReadingType::cpuAverageFrequency));

        std::map<int, SensorReadingType> kHwProtectionReadingSources{
            {0, SensorReadingType::dcPlatformPowerPsu},
            {1, SensorReadingType::dcPlatformPowerCpu}};

        readings.emplace_back(std::make_shared<ReadingMultiSource>(
            sensorReadingsManager, ReadingType::hwProtectionPlatformPower,
            kHwProtectionReadingSources));

        std::map<int, SensorReadingType> kDcPlatformPowerReadingSources{
            {0, SensorReadingType::dcPlatformPowerCpu},
            {1, SensorReadingType::dcPlatformPowerPsu}};

        readings.emplace_back(std::make_shared<ReadingMultiSource>(
            sensorReadingsManager, ReadingType::dcPlatformPower,
            kDcPlatformPowerReadingSources));

        std::map<int, SensorReadingType> kDcPlatformPowerCapMaxReadingSources{
            {0, SensorReadingType::dcPlatformPowerCapabilitiesMaxCpu},
            {1, SensorReadingType::dcPlatformPowerCapabilitiesMaxPsu}};

        readings.emplace_back(std::make_shared<ReadingMultiSource>(
            sensorReadingsManager, ReadingType::dcPlatformPowerCapabilitiesMax,
            kDcPlatformPowerCapMaxReadingSources));

        using ReadingClasses =
            utility::Types<ReadingPowerEfficiency, ReadingCpuPresence,
                           ReadingPciePresence, ReadingCpuUtilization>;

        ReadingClasses::for_each([this](auto t) {
            readings.emplace_back(std::make_shared<typename decltype(t)::type>(
                sensorReadingsManager));
        });

        installComplexReadings();
    }

    void installSensors()
    {
        using DbusSensorClasses =
            utility::Types<InletTempDbusSensor, OutletTempDbusSensor,
                           HostResetDbusSensor, HostPowerDbusSensor,
                           PowerStateDbusSensor, GpuPowerStateDbusSensor>;

        DbusSensorClasses::for_each([this](auto t) {
            sensorsVec.push_back(std::make_shared<typename decltype(t)::type>(
                sensorReadingsManager, bus));
        });

        sensorsVec.push_back(std::make_shared<PcieDbusSensorSensor>(
            sensorReadingsManager, bus, pldmEntityProvider));

        sensorsVec.push_back(std::make_shared<PcieDbusSensorEffecter>(
            sensorReadingsManager, bus, pldmEntityProvider));

        using CpuSensorClasses =
            utility::Types<CpuUtilizationSensor, CpuEfficiencySensor,
                           CpuFrequencySensor, PeciSensor>;

        CpuSensorClasses::for_each([this](auto t) {
            sensorsVec.push_back(std::make_shared<typename decltype(t)::type>(
                sensorReadingsManager, peciCommands, kMaxCpuNumber));
        });

        sensorsVec.push_back(std::make_shared<HwmonSensor>(
            sensorReadingsManager, hwmonFileProvider));

        sensorsVec.push_back(
            std::make_shared<SmartStatusSensor>(sensorReadingsManager));

        sensorsVec.push_back(
            std::make_shared<GpioSensor>(sensorReadingsManager, gpioProvider));

        std::for_each(sensorsVec.begin(), sensorsVec.end(),
                      [](auto& sensor) { sensor->initialize(); });
    }

    void installComplexReadings()
    {
        if (auto reading = findReading(ReadingType::platformPowerEfficiency))
        {
            readings.emplace_back(std::make_shared<ReadingAcPlatformLimit>(
                sensorReadingsManager, reading));
        }
        else
        {
            throw std::logic_error(
                "installComplexReadings must be called after installReadings");
        }
    }

    void installKnobs()
    {
        constexpr uint32_t cpuHwmonKnobMinInMilliWatts = 1;
        constexpr uint32_t pcieHwmonKnobMinInMilliWatts = 1000;
        constexpr uint32_t hwmonKnobMaxInMilliWatts =
            std::numeric_limits<uint32_t>::max();

        for (DeviceIndex idx = 0; idx < kMaxCpuNumber; ++idx)
        {
            knobs.push_back(std::move(std::make_unique<HwmonKnob>(
                KnobType::CpuPackagePower, idx, cpuHwmonKnobMinInMilliWatts,
                hwmonKnobMaxInMilliWatts, hwmonFileProvider, knobExecutor,
                sensorReadingsManager)));
            knobs.push_back(std::move(std::make_unique<HwmonKnob>(
                KnobType::DramPower, idx, cpuHwmonKnobMinInMilliWatts,
                hwmonKnobMaxInMilliWatts, hwmonFileProvider, knobExecutor,
                sensorReadingsManager)));

            knobs.push_back(std::move(std::make_unique<TurboRatioKnob>(
                KnobType::TurboRatioLimit, idx, peciCommands, knobExecutor,
                sensorReadingsManager)));

            knobs.push_back(std::move(std::make_unique<HwpmKnob>(
                KnobType::HwpmPerfPreference, idx, kHwpmKnobsDefault,
                peciCommands, knobExecutor, sensorReadingsManager)));

            knobs.push_back(std::move(std::make_unique<HwpmKnob>(
                KnobType::HwpmPerfBias, idx, kHwpmKnobsDefault, peciCommands,
                knobExecutor, sensorReadingsManager)));

            knobs.push_back(std::move(std::make_unique<HwpmKnob>(
                KnobType::HwpmPerfPreferenceOverride, idx, kHwpmKnobsDefault,
                peciCommands, knobExecutor, sensorReadingsManager)));

            knobs.push_back(std::move(std::make_unique<ProchotRatioKnob>(
                KnobType::Prochot, idx, peciCommands, knobExecutor,
                sensorReadingsManager)));
        }
        for (DeviceIndex idx = 0; idx < kMaxPlatformNumber; ++idx)
        {
            knobs.push_back(std::move(std::make_unique<HwmonKnob>(
                KnobType::DcPlatformPower, idx, cpuHwmonKnobMinInMilliWatts,
                hwmonKnobMaxInMilliWatts, hwmonFileProvider, knobExecutor,
                sensorReadingsManager)));
        }

        if (Config::getInstance().getGeneralPresets().acceleratorsInterface ==
            kPldmInterfaceName)
        {
            for (DeviceIndex idx = 0; idx < kMaxPcieNumber; ++idx)
            {
                knobs.push_back(std::move(std::make_unique<PcieDbusKnob>(
                    KnobType::PciePower, idx, pldmEntityProvider, bus,
                    sensorReadingsManager)));
            }
        }
        else
        {
            for (DeviceIndex idx = 0; idx < kMaxPcieNumber; ++idx)
            {
                knobs.push_back(std::move(std::make_unique<HwmonKnob>(
                    KnobType::PciePower, idx, pcieHwmonKnobMinInMilliWatts,
                    hwmonKnobMaxInMilliWatts, hwmonFileProvider, knobExecutor,
                    sensorReadingsManager)));
            }
        }

        for (auto&& knob : knobs)
        {
            knob->resetKnob();
        }
    }

    void executeKnobAction(
        KnobType knobType, DeviceIndex deviceIndex,
        std::function<void(std::unique_ptr<KnobIf>&)> knobAction)
    {
        bool isDeviceFound = false;
        for (auto&& knob : knobs)
        {
            if (((knob->getDeviceIndex() == deviceIndex) ||
                 kAllDevices == deviceIndex) &&
                (knob->getKnobType() == knobType))
            {
                knobAction(knob);
                isDeviceFound = true;
            }
        }
        if (!isDeviceFound)
        {
            Logger::log<LogLevel::error>(
                "[DevicesManager]: Knob Type [%d], with Device Index %d not "
                "supported",
                std::underlying_type_t<KnobType>(knobType),
                unsigned{deviceIndex});
            // TODO: throw exception so that PBC knows that setKnobValue failed
        }
    }
};

} // namespace nodemanager
