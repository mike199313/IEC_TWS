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

#include "utils/log.hpp"

#include <boost/circular_buffer.hpp>

#include <numeric>

namespace cups
{

namespace peci
{

namespace metrics
{

class AverageCounter
{
  public:
    AverageCounter(unsigned numSamplesArg = 1) : samples(numSamplesArg)
    {}

    double updateAverage(double value)
    {
        samples.push_back(value);

        return calculateAverageUtilization();
    }

  private:
    boost::circular_buffer<double> samples;

    double calculateAverageUtilization()
    {
        return std::accumulate(samples.begin(), samples.end(), double(0)) /
               static_cast<double>(samples.size());
    }
};

} // namespace metrics

} // namespace peci

} // namespace cups
