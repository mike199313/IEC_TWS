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

namespace nodemanager
{

static constexpr const auto kPldmInterfaceName = "PLDM";
static constexpr const auto kPeciInterfaceName = "PECI_OVER_SMBUS";

enum class InitializationMode
{
    warningStopBmcNm = 0,
    criticalStopBmcNm,
    warningDisableSpsNm,
    stopBmcNmUnconditionally
};

static const std::set<InitializationMode> kInitializationModeSet = {
    InitializationMode::warningStopBmcNm, InitializationMode::criticalStopBmcNm,
    InitializationMode::warningDisableSpsNm,
    InitializationMode::stopBmcNmUnconditionally};

} // namespace nodemanager