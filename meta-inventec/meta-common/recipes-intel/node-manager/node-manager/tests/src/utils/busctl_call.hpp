/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2021 Intel Corporation.
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
#include "statistics/statistic_if.hpp"
#include "utils/dbus_environment.hpp"
#include "utils/dbus_paths.hpp"
#include "utils/policy_config.hpp"

using namespace nodemanager;

static constexpr const auto kPolicyManagerInterface =
    "xyz.openbmc_project.NodeManager.PolicyManager";
static constexpr const auto kDomainCapabilitiesInterface =
    "xyz.openbmc_project.NodeManager.Capabilities";
static constexpr const auto kObjectEnabledInterface =
    "xyz.openbmc_project.Object.Enable";
static constexpr const auto kObjectDeleteInterface =
    "xyz.openbmc_project.Object.Delete";
static constexpr const auto kPolicyAttributesInterface =
    "xyz.openbmc_project.NodeManager.PolicyAttributes";
static constexpr const auto kStatisticsInterface =
    "xyz.openbmc_project.NodeManager.Statistics";

class BusctlCall
{
  public:
    static std::tuple<boost::system::error_code,
                      sdbusplus::message::object_path>
        CreatePolicyWithId(DomainId domainId, PolicyId policyId,
                           PolicyConfig& params)
    {
        std::promise<std::tuple<boost::system::error_code,
                                sdbusplus::message::object_path>>
            promise;
        DbusEnvironment::getBus()->async_method_call(
            [&promise](boost::system::error_code ec,
                       const sdbusplus::message::object_path& path) {
                promise.set_value(std::make_tuple(ec, path));
            },
            DbusEnvironment::serviceName(), DbusPaths::domain(domainId),
            kPolicyManagerInterface, "CreateWithId", policyId,
            params._getTuple());

        return DbusEnvironment::waitForFuture(promise.get_future());
    }

    static boost::system::error_code
        UpdatePolicy(DomainId domainId, unsigned policyId, PolicyConfig& params)
    {
        std::promise<boost::system::error_code> promise;
        DbusEnvironment::getBus()->async_method_call(
            [&promise](boost::system::error_code ec) { promise.set_value(ec); },
            DbusEnvironment::serviceName(),
            DbusPaths::policy(domainId, policyId), kPolicyAttributesInterface,
            "Update", params._getTuple());

        return DbusEnvironment::waitForFuture(promise.get_future());
    }

    static boost::system::error_code DeletePolicy(DomainId domainId,
                                                  unsigned policyId)
    {
        std::promise<boost::system::error_code> promise;
        DbusEnvironment::getBus()->async_method_call(
            [&promise](boost::system::error_code ec) { promise.set_value(ec); },
            DbusEnvironment::serviceName(),
            DbusPaths::policy(domainId, policyId), kObjectDeleteInterface,
            "Delete");

        return DbusEnvironment::waitForFuture(promise.get_future());
    }

    static std::tuple<boost::system::error_code,
                      sdbusplus::message::object_path>
        GetSelectedPolicyId(DomainId domainId)
    {
        std::promise<std::tuple<boost::system::error_code,
                                sdbusplus::message::object_path>>
            promise;
        DbusEnvironment::getBus()->async_method_call(
            [&promise](
                boost::system::error_code ec,
                const sdbusplus::message::object_path& limitingPolicyId) {
                promise.set_value({ec, limitingPolicyId});
            },
            DbusEnvironment::serviceName(), DbusPaths::domain(domainId),
            kPolicyManagerInterface, "GetSelectedPolicyId");

        return DbusEnvironment::waitForFuture(promise.get_future());
    }

    template <class ValueType>
    static boost::system::error_code
        SetProperty(std::string path, std::string interface, std::string name,
                    ValueType&& value)
    {
        std::promise<boost::system::error_code> promise;
        sdbusplus::asio::setProperty(
            *(DbusEnvironment::getBus()), DbusEnvironment::serviceName(), path,
            interface, name, value, [&promise](boost::system::error_code ec) {
                promise.set_value(ec);
            });

        return DbusEnvironment::waitForFuture(promise.get_future());
    }

    template <typename PropertyType>
    static std::tuple<boost::system::error_code, PropertyType>
        GetProperty(std::string path, std::string interface, std::string name)
    {
        std::promise<std::tuple<boost::system::error_code, PropertyType>>
            promise;
        sdbusplus::asio::getProperty<PropertyType>(
            *(DbusEnvironment::getBus()), DbusEnvironment::serviceName(), path,
            interface, name,
            [&promise](boost::system::error_code ec, PropertyType data) {
                promise.set_value(std::make_tuple(ec, data));
            });

        return DbusEnvironment::waitForFuture(promise.get_future());
    }

    static std::tuple<boost::system::error_code,
                      std::map<std::string, StatValuesMap>>
        GetStatistics(const std::string& objectPath)
    {
        std::promise<std::tuple<boost::system::error_code,
                                std::map<std::string, StatValuesMap>>>
            promise;
        DbusEnvironment::getBus()->async_method_call(
            [&promise](boost::system::error_code ec,
                       const std::map<std::string, StatValuesMap>& map) {
                promise.set_value(std::make_tuple(ec, map));
            },
            DbusEnvironment::serviceName(), objectPath, kStatisticsInterface,
            "GetStatistics");

        return DbusEnvironment::waitForFuture(promise.get_future());
    }

    static boost::system::error_code
        ResetStatistics(const std::string& objectPath)
    {
        std::promise<boost::system::error_code> promise;
        DbusEnvironment::getBus()->async_method_call(
            [&promise](boost::system::error_code ec) { promise.set_value(ec); },
            DbusEnvironment::serviceName(), objectPath, kStatisticsInterface,
            "ResetStatistics");

        return DbusEnvironment::waitForFuture(promise.get_future());
    }
};