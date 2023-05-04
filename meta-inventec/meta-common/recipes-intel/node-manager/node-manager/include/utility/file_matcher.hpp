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
#include <regex>

namespace nodemanager
{
template <class Results>
class FileMatcher
{
  public:
    using PathDecompositionMap = std::map<std::filesystem::path, Results>;
    FileMatcher() = delete;
    FileMatcher(const FileMatcher&) = default;
    FileMatcher& operator=(const FileMatcher&) = default;
    FileMatcher(FileMatcher&&) = default;
    FileMatcher& operator=(FileMatcher&&) = default;
    FileMatcher(const std::filesystem::path rootPathArg,
                const std::regex pathRegexArg,
                std::function<Results(const std::smatch&)> extractResultsArg) :
        rootPath(rootPathArg),
        pathRegex(pathRegexArg), extractResults(extractResultsArg){};
    virtual ~FileMatcher() = default;

    void findFiles(PathDecompositionMap& foundPaths,
                   unsigned int symlinkDepth = 0U) const
    {
        if (!std::filesystem::exists(rootPath))
        {
            return;
        }
        findFiles(rootPath, foundPaths, symlinkDepth);
    }

  private:
    inline void findFiles(const std::filesystem::path& dirPath,
                          PathDecompositionMap& foundPaths,
                          unsigned int symlinkDepth = 0U) const
    {
        for (auto& p : std::filesystem::recursive_directory_iterator(dirPath))
        {
            if (p.is_symlink() && symlinkDepth)
            {
                findFiles(p.path(), foundPaths, symlinkDepth - 1U);
            }
            else if (!p.is_directory())
            {
                if (const auto& matchResults = isMatching(p.path()))
                {
                    foundPaths[p.path()] = *matchResults;
                }
            }
        }
    }

    inline std::optional<Results>
        isMatching(const std::filesystem::path& p) const
    {
        std::smatch match;
        std::string pa = p.string();
        if (std::regex_search(pa, match, pathRegex))
        {
            return extractResults(match);
        }
        else
        {
            return std::nullopt;
        }
    }

    std::filesystem::path rootPath;
    std::regex pathRegex;
    std::function<Results(const std::smatch&)> extractResults;
};

} // namespace nodemanager
