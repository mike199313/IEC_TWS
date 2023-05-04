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
#include "loggers/log.hpp"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <utility/for_each.hpp>

namespace nodemanager
{

/**
 * @brief A utility class used to create configuration files and folders.
 *
 */
class PersistentStorage
{
  public:
    PersistentStorage() = default;
    PersistentStorage(const PersistentStorage&) = delete;
    PersistentStorage& operator=(const PersistentStorage&) = delete;
    PersistentStorage(PersistentStorage&&) = delete;
    PersistentStorage& operator=(PersistentStorage&&) = delete;
    virtual ~PersistentStorage() = default;

    bool store(const std::filesystem::path& filePath,
               const nlohmann::json& data)
    {
        Logger::log<LogLevel::info>("Storing json data to file: %s",
                                    filePath.c_str());
        try
        {
            std::error_code ec;

            std::filesystem::create_directories(filePath.parent_path(), ec);
            if (ec)
            {
                throw std::runtime_error(
                    "Unable to create directory for file: " +
                    filePath.string() + ", ec=" + std::to_string(ec.value()) +
                    ": " + ec.message());
            }
            std::ofstream file(filePath);
            file << std::setw(4) << data;
            if (!file)
            {
                throw std::runtime_error("Unable to create file: " +
                                         filePath.string());
            }
            file.flush();
            limitPermissions(filePath.parent_path());
            limitPermissions(filePath);
            return true;
        }
        catch (const std::exception& e)
        {
            remove(filePath);
            Logger::log<LogLevel::error>(
                "Storing data to file: %s failed, error: %s", filePath.c_str(),
                e.what());
            return false;
        }
    }

    std::vector<std::string> readLines(const std::filesystem::path& filePath)
    {
        std::vector<std::string> lines;

        std::ifstream file(filePath);
        if (file)
        {
            std::string line;
            while (std::getline(file, line))
            {
                lines.push_back(line);
            }
        }

        return lines;
    }

    /**
     * @brief From json config file tries to read values described by
     * paramsToRead.
     *
     * @tparam JsonMapper
     * @param filePath
     * @param paramsToRead
     */
    template <class JsonMapper>
    bool readJsonFile(const std::filesystem::path& filePath,
                      JsonMapper&& paramsToRead)
    {
        std::ifstream ifs(filePath);
        nlohmann::json j = nlohmann::json::parse(ifs, nullptr, false);
        if (j.is_discarded())
        {
            Logger::log<LogLevel::error>("File %s is invalid JSON format",
                                         filePath);
            return false;
        }
        uint32_t error_counter = 0;
        for_each(
            std::forward<JsonMapper>(paramsToRead),
            [&j, &error_counter](auto&& category, auto&& paramStr,
                                 auto&& paramValue, auto&& validate) {
                std::string category_error_string;
                try
                {
                    const auto defaultParamValue = paramValue;
                    if (category)
                    {
                        category_error_string =
                            (boost::format("[%1%]:") % category).str();
                        j.at(category).at(paramStr).get_to(paramValue);
                    }
                    else
                    {
                        j.at(paramStr).get_to(paramValue);
                    }
                    if (!validate(paramValue))
                    {
                        paramValue = defaultParamValue;
                        throw std::out_of_range("Value out of range");
                    }
                }
                catch (nlohmann::json::out_of_range& e)
                {
                    Logger::log<LogLevel::error>(
                        "Error `%3%` for %1%[%2%]. Using provisioned value",
                        category_error_string, paramStr, e.what());
                    error_counter++;
                }
                catch (std::out_of_range& e)
                {
                    Logger::log<LogLevel::error>(
                        "Error `%3%` for %1%[%2%]. Using provisioned value",
                        category_error_string, paramStr, e.what());
                    error_counter++;
                }
                catch (nlohmann::json::type_error& e)
                {
                    Logger::log<LogLevel::error>(
                        "Error `%3%` for %1%[%2%]. Using provisioned value",
                        category_error_string, paramStr, e.what());
                    error_counter++;
                }
                catch (...)
                {
                    Logger::log<LogLevel::error>(
                        "Error for %1%[%2%]. Using provisioned value",
                        category_error_string, paramStr);
                    error_counter++;
                }
            });
        if (error_counter == 0)
        {
            Logger::log<LogLevel::info>("Json file %1% loaded with success",
                                        filePath);
        }
        else
        {
            Logger::log<LogLevel::info>(
                "Json file %1% loaded with %2% error(s)", filePath,
                error_counter);
        }
        return (error_counter == 0);
    }

    bool exists(const std::filesystem::path& filePath)
    {
        std::error_code ec;
        bool ret = std::filesystem::exists(filePath, ec);
        if (ec)
        {
            Logger::log<LogLevel::error>(
                "Cannot state if file %1% exists. Error: %2%. Assuming it "
                "dosn't exists",
                filePath, ec.message());
            return false;
        }
        return ret;
    }

    bool remove(const std::filesystem::path& path)
    {
        Logger::log<LogLevel::info>("Removing file named %s", path.c_str());
        std::error_code ec;

        auto removed = std::filesystem::remove(path, ec);
        if (!removed)
        {
            Logger::log<LogLevel::error>("Unable to remove file: %s, error: %s",
                                         path.c_str(), ec.message().c_str());
            return false;
        }

        /* removes directory only if it is empty */
        std::filesystem::remove(path.parent_path(), ec);
        return true;
    }

  private:
    void limitPermissions(const std::filesystem::path& path)
    {
        constexpr auto filePerms = std::filesystem::perms::owner_read |
                                   std::filesystem::perms::owner_write;
        constexpr auto dirPerms =
            filePerms | std::filesystem::perms::owner_exec;
        std::filesystem::permissions(
            path, std::filesystem::is_directory(path) ? dirPerms : filePerms,
            std::filesystem::perm_options::replace);
    }
};

} // namespace nodemanager