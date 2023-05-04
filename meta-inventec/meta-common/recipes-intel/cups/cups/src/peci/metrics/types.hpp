/*
 *  INTEL CONFIDENTIAL
 *
 *  Copyright 2020 Intel Corporation
 *
 *  This software and the related documents are Intel copyrighted materials,
 *  and your use of them is governed by the express license under which they
 *  were provided to you (License). Unless the License provides otherwise,
 *  you may not use, modify, copy, publish, distribute, disclose or
 *  transmit this software or the related documents without
 *  Intel's prior written permission.
 *
 *  This software and the related documents are provided as is,
 *  with no express or implied warranties, other than those
 *  that are expressly stated in the License.
 */

#pragma once

#include "peci/metrics/core_factory.hpp"
#include "peci/metrics/iio_factory.hpp"
#include "peci/metrics/memory_factory.hpp"
#include "peci/metrics/utilization.hpp"
#include "peci/transport/adapter.hpp"
#include "utils/traits.hpp"

#include <optional>

namespace cups
{

namespace peci
{

namespace metrics
{

struct Cpu
{
    Core core;
    Memory memory;
    Iio iio;

    Cpu(Core&& core, Memory&& memory, Iio&& iio) :
        core{std::move(core)}, memory{std::move(memory)}, iio{std::move(iio)}
    {}

    static std::optional<Cpu>
        detect(std::shared_ptr<transport::Adapter> peciAdapter,
               const uint8_t address)
    {
        auto coreFactory = CoreFactory(peciAdapter);
        auto cpu = coreFactory.detect(address);
        if (!cpu)
        {
            return std::nullopt;
        }

        auto iioFactory = IioFactory(peciAdapter);
        auto iio = iioFactory.detect(cpu->getAddress(), cpu->getCpuId());
        if (!iio)
        {
            return std::nullopt;
        }

        auto memoryFactory = MemoryFactory(peciAdapter);
        auto memory = memoryFactory.detect(cpu->getAddress(), cpu->getCpuId(),
                                           cpu->getBusNumber());
        if (!memory)
        {
            return std::nullopt;
        }

        return Cpu(std::move(*cpu), std::move(*memory), std::move(*iio));
    }
};

struct Utilization
{
    UtilizationDelta<Core> core;
    UtilizationDelta<Memory> memory;
    UtilizationDelta<Iio> iio;

    Utilization(Cpu& cpu) : core(cpu.core), memory(cpu.memory), iio(cpu.iio)
    {}
};

} // namespace metrics

} // namespace peci

} // namespace cups
