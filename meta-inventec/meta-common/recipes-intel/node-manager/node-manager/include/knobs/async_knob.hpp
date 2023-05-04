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

#include "flow_control.hpp"
#include "knob.hpp"
#include "utility/async_executor.hpp"

namespace nodemanager
{
using AsyncKnobExecutor =
    AsyncExecutorIf<std::pair<KnobType, DeviceIndex>,
                    std::pair<std::ios_base::iostate, uint32_t>>;

class AsyncKnob : public Knob
{
  public:
    AsyncKnob() = delete;
    AsyncKnob(const AsyncKnob&) = delete;
    AsyncKnob& operator=(const AsyncKnob&) = delete;
    AsyncKnob(AsyncKnob&&) = delete;
    AsyncKnob& operator=(AsyncKnob&&) = delete;

    AsyncKnob(KnobType typeArg, DeviceIndex deviceIndexArg,
              std::shared_ptr<AsyncKnobExecutor> asyncExecutorArg,
              const std::shared_ptr<SensorReadingsManagerIf>&
                  sensorReadingsManagerArg) :
        Knob(typeArg, deviceIndexArg, sensorReadingsManagerArg),
        asyncExecutor(asyncExecutorArg)
    {
    }

    virtual ~AsyncKnob()
    {
    }

    virtual void writeValue() override
    {
        asyncExecutor->schedule({getKnobType(), getDeviceIndex()}, getTask(),
                                getTaskCallback());
    }

  protected:
    virtual AsyncKnobExecutor::Task getTask() const = 0;
    virtual AsyncKnobExecutor::TaskCallback getTaskCallback() = 0;
    std::shared_ptr<AsyncKnobExecutor> asyncExecutor;
};
} // namespace nodemanager