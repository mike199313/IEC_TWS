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

#include <boost/asio.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

template <typename T>
class PropertyMock
{
  public:
    PropertyMock(T initValue)
    {
        ON_CALL(*this, setValue(testing::_))
            .WillByDefault(testing::Return(true));
        ON_CALL(*this, getValue()).WillByDefault(testing::Return(initValue));
    }
    virtual ~PropertyMock() = default;
    MOCK_METHOD(bool, setValue, (T), ());
    MOCK_METHOD(T, getValue, (), ());
};

class DbusObjectStub
{
  public:
    DbusObjectStub(
        boost::asio::io_context& ioc,
        const std::shared_ptr<sdbusplus::asio::connection>& bus,
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer) :
        ioc(ioc),
        bus(bus), objServer(objServer)
    {
    }
    virtual ~DbusObjectStub() = default;

    virtual const char* path() = 0;

  private:
    boost::asio::io_context& ioc;
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
};
