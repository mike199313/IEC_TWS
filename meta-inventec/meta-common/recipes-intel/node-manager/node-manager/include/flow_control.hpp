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

#include "common_types.hpp"

#include <functional>

namespace nodemanager
{

class RunnerIf
{
  public:
    virtual ~RunnerIf() = default;
    virtual void run() = 0;
};

class RunnerExtIf : public RunnerIf
{
  public:
    virtual ~RunnerExtIf() = default;
    virtual void postRun() = 0;
};

} // namespace nodemanager
