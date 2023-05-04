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

#include "devices_manager/devices_manager.hpp"

#include <iostream>

namespace nodemanager
{

class EfficiencyControlIf
{
  public:
    virtual ~EfficiencyControlIf() = default;
    virtual void setValue(KnobType knobType, uint32_t setValue) = 0;
    virtual void resetValue(KnobType knobType) = 0;
};

class EfficiencyControl : public EfficiencyControlIf
{
  public:
    EfficiencyControl() = delete;
    EfficiencyControl(const EfficiencyControl&) = delete;
    EfficiencyControl& operator=(const EfficiencyControl&) = delete;
    EfficiencyControl(EfficiencyControl&&) = delete;
    EfficiencyControl& operator=(EfficiencyControl&&) = delete;

    EfficiencyControl(std::shared_ptr<DevicesManagerIf> devicesManagerArg) :
        devicesManager(devicesManagerArg)
    {
    }

    virtual ~EfficiencyControl() = default;

    void setValue(KnobType knobType, uint32_t setValue)
    {
        devicesManager->setKnobValue(knobType, kAllDevices, setValue);
    }

    void resetValue(KnobType knobType)
    {
        devicesManager->resetKnobValue(knobType, kAllDevices);
    }

  private:
    std::shared_ptr<DevicesManagerIf> devicesManager;

}; // class EfficiencyControl

} // namespace nodemanager
