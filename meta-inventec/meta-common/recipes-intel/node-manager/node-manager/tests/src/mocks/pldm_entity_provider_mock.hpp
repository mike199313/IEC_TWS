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

#include "devices_manager/pldm_entity_provider.hpp"

#include <gmock/gmock.h>

using namespace nodemanager;

class PldmEntityProviderMock : public PldmEntityProviderIf
{
  public:
    MOCK_METHOD(std::optional<std::string>, getTid, (DeviceIndex),
                (const, override));
    MOCK_METHOD(std::optional<std::string>, getDeviceName, (DeviceIndex),
                (const, override));
    MOCK_METHOD(void, registerDiscoveryDataChangeCallback,
                (const std::shared_ptr<std::function<void(void)>>&),
                (override));
    MOCK_METHOD(void, unregisterDiscoveryDataChangeCallback,
                (const std::shared_ptr<std::function<void(void)>>&),
                (override));
};