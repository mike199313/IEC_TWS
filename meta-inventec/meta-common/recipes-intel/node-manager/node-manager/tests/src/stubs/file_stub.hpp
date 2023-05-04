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

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

template <typename T>
class FileStub
{
  public:
    FileStub() : rootPath(std::filesystem::temp_directory_path())
    {
    }

    std::filesystem::path rootPath;

    std::filesystem::path createFile(const std::string& name, T value)
    {
        std::ofstream(rootPath / name) << value << "\n";
        return rootPath / name;
    }

    T readFileContent(const std::string& name)
    {
        if (!std::filesystem::exists(rootPath / name))
        {
            std::ostringstream msg;
            msg << "The file " << name << " does not exist!";
            throw std::runtime_error(msg.str());
        }

        std::string value;
        std::ifstream file(rootPath / name);
        file >> value;
        return value;
    }

    void writeFileContent(const std::string& name, const T& value)
    {
        if (!std::filesystem::exists(rootPath / name))
        {
            std::ostringstream msg;
            msg << "The file " << name << " does not exist!";
            throw std::runtime_error(msg.str());
        }

        std::ofstream file(rootPath / name);
        file << value << "\n";
    }

    std::filesystem::path getFilePath(const std::string& name)
    {
        std::filesystem::path path(rootPath / name);
        if (!std::filesystem::exists(path))
        {
            std::ostringstream msg;
            msg << "The file " << name << " does not exist!";
            throw std::runtime_error(msg.str());
        }

        return path;
    }
};