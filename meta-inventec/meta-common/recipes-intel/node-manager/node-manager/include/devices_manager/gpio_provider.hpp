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

#include "loggers/log.hpp"
#include "utility/devices_configuration.hpp"
#include "utility/ranges.hpp"

#include <gpiod.hpp>
#include <map>

namespace nodemanager
{

enum class GpioState
{
    low = 0,
    high = 1,
};

class GpioProviderIf
{
  public:
    virtual ~GpioProviderIf() = default;
    virtual DeviceIndex getGpioLinesCount() const = 0;
    virtual std::string getLineName(DeviceIndex) const = 0;
    virtual std::optional<DeviceIndex> getGpioLine(std::string) const = 0;
    virtual std::string getFormattedLineName(DeviceIndex) const = 0;
    virtual std::optional<GpioState> getState(DeviceIndex index) const = 0;
    virtual bool reserveGpio(DeviceIndex) = 0;
    virtual void freeGpio(DeviceIndex) = 0;
    virtual bool isGpioReserved(DeviceIndex) const = 0;
};

/**
 * @brief This class scans GPIOs and provides easy access to them
 *
 */
class GpioProvider : public GpioProviderIf
{
  public:
    GpioProvider()
    {
        discoverGpioLines();
    }

    virtual ~GpioProvider()
    {
        for (const auto& gpio : gpioMapping)
        {
            if (gpio.second->is_requested())
            {
                gpio.second->release();
            }
        }
    }

    virtual DeviceIndex getGpioLinesCount() const override
    {
        return safeCast<DeviceIndex>(gpioMapping.size(), kMaxGpioNumber);
    }

    virtual std::string getLineName(DeviceIndex index) const override
    {
        auto it = gpioMapping.find(index);
        if (it != gpioMapping.end())
        {
            return it->second->name();
        }
        return std::string();
    }

    virtual std::optional<DeviceIndex>
        getGpioLine(std::string lineName) const override
    {
        for (DeviceIndex index = 0; index < getGpioLinesCount(); index++)
        {
            if (getLineName(index) == lineName)
            {
                return index;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief Get the Formatted Line Name string
     * Removes NM prefix, removes underscores and makes the string lowercase and
     * leaves only the first letter of each word as an uppercase char.
     *
     * @param index
     * @return std::string formatted name of GPIO line with given index
     */
    virtual std::string getFormattedLineName(DeviceIndex index) const override
    {
        std::string lineName = getLineName(index);
        lineName = lineName.substr(strlen(kGpioNamePrefix));
        unsigned char prev = '_';
        std::transform(lineName.begin(), lineName.end(), lineName.begin(),
                       [&prev](unsigned char c) {
                           if (prev == '_')
                           {
                               prev = c;
                               return std::toupper(c);
                           }
                           else
                           {
                               prev = c;
                               return std::tolower(c);
                           }
                       });
        lineName.erase(std::remove(lineName.begin(), lineName.end(), '_'),
                       lineName.end());
        return std::to_string(index) + "_" + lineName;
    }

    virtual std::optional<GpioState> getState(DeviceIndex index) const override
    {
        std::optional<GpioState> state = std::nullopt;
        auto it = gpioMapping.find(index);
        if (it != gpioMapping.end())
        {
            int value = it->second->get_value();
            if (value != -1)
            {
                state = (value == kGpioHighState) ? GpioState::high
                                                  : GpioState::low;
            }
        }
        return state;
    }

    virtual bool reserveGpio(DeviceIndex index) override
    {
        if (gpioMapping.count(index) != 0)
        {
            reservedGpios.insert(index);
            return true;
        }
        return false;
    }

    virtual void freeGpio(DeviceIndex index) override
    {
        reservedGpios.erase(index);
    }

    virtual bool isGpioReserved(DeviceIndex index) const override
    {
        return (reservedGpios.count(index) != 0);
    }

  private:
    static constexpr const auto kGpioHighState = 1;
    static constexpr const auto kGpioNamePrefix = "NM_GPIO_";

    void discoverGpioLines()
    {
        DeviceIndex index = 0;

        for (const gpiod::chip& chip : gpiod::make_chip_iter())
        {
            for (const gpiod::line& line : gpiod::line_iter(chip))
            {
                if (isNodeManagerLine(line))
                {
                    if (saveLine(line, index))
                    {
                        index++;
                    }
                }
            }
        }
    }

    bool isNodeManagerLine(const gpiod::line& line) const
    {
        return (line.name().rfind(kGpioNamePrefix, 0) == 0);
    }

    bool saveLine(const gpiod::line& line, const DeviceIndex& index)
    {
        static const gpiod::line_request nmRequest = {
            "node-manager", gpiod::line_request::DIRECTION_INPUT, 0};

        if (index < kMaxGpioNumber)
        {
            try
            {
                line.request(nmRequest);
                if (line.is_requested())
                {
                    Logger::log<LogLevel::debug>(
                        "Discovered GPIO line %s. Assigned index: "
                        "%d",
                        line.name(), index);
                    gpioMapping.emplace(index,
                                        std::make_unique<gpiod::line>(line));
                    return true;
                }
                else
                {
                    Logger::log<LogLevel::error>(
                        "Tried to request GPIO line %s as input, but it did "
                        "not get requested",
                        line.name());
                }
            }
            catch (std::exception const& e)
            {
                Logger::log<LogLevel::error>(
                    "Failed to request GPIO line %s as input. %s", line.name(),
                    e.what());
            }
        }
        else
        {
            Logger::log<LogLevel::warning>(
                "Failed to discover GPIO line %s. Maximum number "
                "of NM GPIO lines reached",
                line.name());
        }
        return false;
    }

    std::unordered_map<DeviceIndex, std::unique_ptr<gpiod::line>> gpioMapping;
    std::set<DeviceIndex> reservedGpios;
};

} // namespace nodemanager
