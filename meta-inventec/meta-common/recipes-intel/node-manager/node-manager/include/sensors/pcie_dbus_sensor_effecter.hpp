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

#include "pcie_dbus_sensor.hpp"

namespace nodemanager
{

static constexpr const auto kPcieSensorEffecterValueIface =
    "xyz.openbmc_project.Effecter.Value";

class PcieDbusSensorEffecter : public PcieDbusSensor
{
  public:
    PcieDbusSensorEffecter() = delete;
    PcieDbusSensorEffecter(const PcieDbusSensorEffecter&) = delete;
    PcieDbusSensorEffecter& operator=(const PcieDbusSensorEffecter&) = delete;
    PcieDbusSensorEffecter(PcieDbusSensorEffecter&&) = delete;
    PcieDbusSensorEffecter& operator=(PcieDbusSensorEffecter&&) = delete;

    PcieDbusSensorEffecter(
        const std::shared_ptr<SensorReadingsManagerIf>&
            sensorReadingsManagerArg,
        const std::shared_ptr<sdbusplus::asio::connection>& busArg,
        const std::shared_ptr<PldmEntityProviderIf>& pldmEntityProviderArg) :
        PcieDbusSensor(sensorReadingsManagerArg, busArg, pldmEntityProviderArg,
                       kPcieSensorEffecterValueIface)
    {
    }

    virtual ~PcieDbusSensorEffecter() = default;

  private:
    std::string getObjectPath(const std::string& tid,
                              const std::string& device) const override
    {
        return "/xyz/openbmc_project/pldm/" + tid +
               "/effecter/power/PCIe_Slot_" + tid + "_" + device + "_PL1";
    }

    std::vector<SensorReadingType> getSensorReadingTypes() const override
    {
        return {SensorReadingType::pciePowerLimitPldm,
                SensorReadingType::pciePowerCapabilitiesMinPldm,
                SensorReadingType::pciePowerCapabilitiesMaxPldm};
    }
};

} // namespace nodemanager
