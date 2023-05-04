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

#include "config/persistent_storage.hpp"
#include "throttling_event.hpp"
#include "utility/file_watcher.hpp"

namespace nodemanager
{
static const std::filesystem::path kThrottlingLogFilePath =
    "/var/log/nm_events";
static constexpr const auto kMaxThrottlingEvents = 128;
static constexpr const auto kMaxThrottlingLogFiles = 3;

static constexpr const auto kThrottlingLogTagStart = "+";
static constexpr const auto kThrottlingLogTagStop = "-";
static constexpr const auto kThrottlingLogTagRestart = "R";

class ThrottlingLogCollector
{
  public:
    ThrottlingLogCollector(const ThrottlingLogCollector&) = delete;
    ThrottlingLogCollector& operator=(const ThrottlingLogCollector&) = delete;
    ThrottlingLogCollector(ThrottlingLogCollector&&) = delete;
    ThrottlingLogCollector& operator=(ThrottlingLogCollector&&) = delete;

    ThrottlingLogCollector(boost::asio::io_context& ioc) :
        fileWatcher(ioc, kThrottlingLogFilePath,
                    [this](const std::vector<std::string>& lines) {
                        processLogs(lines);
                    })
    {
        const auto& logs = readLogFiles();
        processLogs(logs);
    }

    nlohmann::json getJson() const
    {
        nlohmann::json eventsJson;
        eventsJson["ThrottlingEvents"] = nlohmann::json::array();
        for (const auto& event : throttlingEventBuffer)
        {
            eventsJson["ThrottlingEvents"].push_back(event.toJson());
        }
        return eventsJson;
    }

  private:
    PersistentStorage storage;
    std::list<ThrottlingEvent> throttlingEventBuffer;
    FileWatcher fileWatcher;

    /**
     * @brief Read throttling log files.
     *
     * @return std::vector<std::string> all logs in order from oldest to most
     * recent.
     */
    std::vector<std::string> readLogFiles()
    {
        std::deque<std::filesystem::path> filesToRead({kThrottlingLogFilePath});
        for (auto i = 1; i < kMaxThrottlingLogFiles; ++i)
        {
            std::filesystem::path rotatedFilePath = kThrottlingLogFilePath;
            rotatedFilePath += "." + std::to_string(i);
            if (storage.exists(rotatedFilePath))
            {
                filesToRead.push_front(rotatedFilePath);
            }
        }

        std::vector<std::string> lines;

        for (auto filePath : filesToRead)
        {
            auto newLines = storage.readLines(filePath);
            lines.insert(lines.end(), newLines.begin(), newLines.end());
        }
        return lines;
    }

    void processLogs(const std::vector<std::string>& lines)
    {
        for (const auto& line : lines)
        {
            try
            {
                processLogLine(line);
            }
            catch (std::exception& e)
            {
                Logger::log<LogLevel::error>(
                    "Error processing throttling log: %s : %s", line, e.what());
            }
        }
    }

    /**
     * @brief Checks for free slot in event buffer. If buffer is full removes
     * oldest, finished entry.
     *
     */
    bool freeSlot()
    {
        if (throttlingEventBuffer.size() < kMaxThrottlingEvents)
        {
            return true;
        }
        auto it = std::find_if(
            throttlingEventBuffer.begin(), throttlingEventBuffer.end(),
            [](ThrottlingEvent event) { return event.isFinished(); });
        if (it != throttlingEventBuffer.end())
        {
            throttlingEventBuffer.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief Process throttling log line. Adds throttling event to the
     * end of event buffer. Throws exception on error.
     *
     */
    void processLogLine(const std::string& line)
    {
        auto lineJson = nlohmann::json::parse(line);
        if (lineJson.is_discarded() ||
            !(lineJson.contains("Tag") && lineJson.contains("From") &&
              lineJson.contains("StartTimestamp")))
        {
            throw std::invalid_argument(
                "Invalid json format or some required fields missing");
        }

        std::string tag = lineJson["Tag"];
        std::string from = lineJson["From"];
        std::chrono::seconds timestamp{
            std::stoi(lineJson["StartTimestamp"].get<std::string>())};
        if (tag == kThrottlingLogTagStart)
        {
            std::string id = lineJson.value("Id", from);
            nlohmann::json policyJson =
                lineJson.value("Policy", nlohmann::json());
            std::string reason = lineJson.value("Reason", "");
            if (freeSlot())
            {
                throttlingEventBuffer.emplace_back(timestamp, id, policyJson,
                                                   reason);
            }
            else
            {
                throw std::length_error("Event buffer full");
            }
        }
        else if (tag == kThrottlingLogTagStop)
        {
            std::string id = lineJson.value("Id", from);
            for (auto it = throttlingEventBuffer.rbegin();
                 it != throttlingEventBuffer.rend(); ++it)
            {
                if (!it->isFinished() && it->getId() == id)
                {
                    it->stop(timestamp);
                    break;
                }
            }
        }
        else if (tag == kThrottlingLogTagRestart)
        {
            for (auto& event : throttlingEventBuffer)
            {
                if (!event.isFinished() && event.isNmEvent())
                {
                    event.stop(timestamp);
                }
            }
        }
        else
        {
            throw std::invalid_argument("Unknown tag " + tag);
        }
    }
};

} // namespace nodemanager
