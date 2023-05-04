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

namespace nodemanager
{

template <typename T>
class LatchProperty
{
  public:
    LatchProperty(T valueArg) : value(valueArg){};
    LatchProperty(const LatchProperty&) = delete;
    LatchProperty(LatchProperty&&) = delete;
    LatchProperty& operator=(const LatchProperty&) = delete;
    LatchProperty& operator=(LatchProperty&&) = delete;

    T get() const
    {
        return value;
    }

    void set(const T& newValue)
    {
        if (!valueLocked)
        {
            value = newValue;
        }
    }

    void setAndLock(const T& newValue)
    {
        valueLocked = true;
        value = newValue;
    }

    void setAndUnlock(const T& newValue)
    {
        valueLocked = false;
        value = newValue;
    }

  private:
    bool valueLocked = false;
    T value;
};

} // namespace nodemanager
