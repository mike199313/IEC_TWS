/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2020 Intel Corporation.
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

namespace nmipmi
{
//----------------DBus services
const static constexpr char* kObjectMapperService =
    "xyz.openbmc_project.ObjectMapper";

//----------------DBus interfaces
const static constexpr char* kNodeManagerInterface =
    "xyz.openbmc_project.NodeManager.NodeManager";
const static constexpr char* kPolicyManagerInterface =
    "xyz.openbmc_project.NodeManager.PolicyManager";
const static constexpr char* kObjectDeleteInterface =
    "xyz.openbmc_project.Object.Delete";
const static constexpr char* kObjectEnableInterface =
    "xyz.openbmc_project.Object.Enable";
const static constexpr char* kPolicyAttributesInterface =
    "xyz.openbmc_project.NodeManager.PolicyAttributes";
const static constexpr char* kDomainAttributesInterface =
    "xyz.openbmc_project.NodeManager.DomainAttributes";
const static constexpr char* kStatisticsInterface =
    "xyz.openbmc_project.NodeManager.Statistics";
const static constexpr char* kNmCapabilitiesInterface =
    "xyz.openbmc_project.NodeManager.Capabilities";
const static constexpr char* kNmTriggerInterface =
    "xyz.openbmc_project.NodeManager.Trigger";

const static constexpr char* kObjectMapperInterface =
    "xyz.openbmc_project.ObjectMapper";

} // namespace nmipmi