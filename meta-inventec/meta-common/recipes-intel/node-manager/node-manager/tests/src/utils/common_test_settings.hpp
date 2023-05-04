/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2021-2022 Intel Corporation.
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

static constexpr uint32_t kCpuIdDefault = 12345;
static constexpr const unsigned int kSimulatedCpusNumber = 2;
static constexpr uint8_t kCoresNumber = 1;
static constexpr auto kCpuOnIndex = 0;
static constexpr auto kCpuOffIndex = 1;
static constexpr auto kNodeManagerSyncTimeIntervalMs = 100;
static constexpr auto kNodeManagerSyncTimeIntervalUs =
    (kNodeManagerSyncTimeIntervalMs * 1000);

} // namespace nodemanager