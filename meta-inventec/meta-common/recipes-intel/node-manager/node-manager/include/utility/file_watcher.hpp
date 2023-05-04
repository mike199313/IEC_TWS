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

#include <sys/inotify.h>

namespace nodemanager
{

using FileWatcherCallback =
    std::function<void(const std::vector<std::string>&)>;

class FileWatcher
{
  public:
    FileWatcher(const FileWatcher&) = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;
    FileWatcher(FileWatcher&&) = delete;
    FileWatcher& operator=(FileWatcher&&) = delete;
    FileWatcher() = delete;

    FileWatcher(boost::asio::io_context& ioc,
                const std::filesystem::path& filePathArg,
                const FileWatcherCallback& callbackArg) :
        filePath(filePathArg),
        callback(callbackArg)
    {
        cacheFile();
        startMonitor(ioc);
    }

    ~FileWatcher()
    {
        if (dirWatchDesc != -1)
        {
            inotify_rm_watch(inotifyFd, dirWatchDesc);
        }
        if (fileWatchDesc != -1)
        {
            inotify_rm_watch(inotifyFd, fileWatchDesc);
        }
        inotifyConn->close();
    }

  private:
    std::filesystem::path filePath;
    FileWatcherCallback callback;

    std::optional<boost::asio::posix::stream_descriptor> inotifyConn;
    int inotifyFd = -1;
    int dirWatchDesc = -1;
    int fileWatchDesc = -1;
    std::streampos filePosition{0};

    void startMonitor(boost::asio::io_context& ioc)
    {
        inotifyConn.emplace(ioc);
        inotifyFd = inotify_init1(IN_NONBLOCK);
        if (inotifyFd == -1)
        {
            Logger::log<LogLevel::error>("inotify_init1 failed");
            return;
        }

        dirWatchDesc =
            inotify_add_watch(inotifyFd, filePath.parent_path().c_str(),
                              IN_CREATE | IN_MOVED_TO | IN_DELETE);
        if (dirWatchDesc == -1)
        {
            Logger::log<LogLevel::error>(
                "inotify_add_watch failed for directory %s",
                filePath.parent_path().string());
            return;
        }

        fileWatchDesc =
            inotify_add_watch(inotifyFd, filePath.c_str(), IN_MODIFY);
        if (fileWatchDesc == -1)
        {
            Logger::log<LogLevel::error>("inotify_add_watch failed for file %s",
                                         filePath.string());
            // Don't return error if file does not exist.
            // Watch on directory will handle create/delete of file.
        }

        inotifyConn->assign(inotifyFd);
        watchFile();
    }

    void resetFilePosition()
    {
        filePosition = 0;
    }

    void cacheFile()
    {
        std::ifstream fileStream(filePath);
        if (!fileStream.good())
        {
            Logger::log<LogLevel::error>("File open failed for %s",
                                         filePath.string());
            return;
        }
        std::string logEntry;
        while (std::getline(fileStream, logEntry))
        {
            filePosition = fileStream.tellg();
        }
    }

    void readFromFile()
    {
        std::ifstream fileStream(filePath);
        if (!fileStream.good())
        {
            Logger::log<LogLevel::error>("File open failed for %s",
                                         filePath.string());
            return;
        }

        fileStream.seekg(filePosition);

        std::string line;
        std::vector<std::string> lines;

        while (std::getline(fileStream, line))
        {
            filePosition = fileStream.tellg();
            lines.push_back(line);
        }
        callback(lines);
    }

    void watchFile()
    {
        if (!inotifyConn)
        {
            return;
        }

        static std::array<char, 1024> readBuffer;

        inotifyConn->async_read_some(
            boost::asio::buffer(readBuffer),
            [&](const boost::system::error_code& ec,
                const std::size_t& bytesTransferred) {
                if (ec)
                {
                    Logger::log<LogLevel::error>(
                        "File watcher callback error: %s", ec.message());
                    return;
                }
                processNotifyEvents(readBuffer, bytesTransferred);

                watchFile();
            });
    }

    void processNotifyEvents(const std::array<char, 1024>& readBuffer,
                             const std::size_t& bytesTransferred)
    {
        std::size_t index = 0;
        const std::size_t& eventSize = sizeof(inotify_event);
        while ((index + eventSize) <= bytesTransferred)
        {
            struct inotify_event event
            {
            };
            std::memcpy(&event, &readBuffer[index], eventSize);
            if (event.wd == dirWatchDesc)
            {
                if ((event.len == 0) ||
                    (index + eventSize + event.len > bytesTransferred))
                {
                    index += (eventSize + event.len);
                    continue;
                }

                std::string fileName(&readBuffer[index + eventSize], event.len);
                fileName.erase(
                    std::find(fileName.begin(), fileName.end(), '\0'),
                    fileName.end());

                if (fileName != filePath.filename().string())
                {
                    index += (eventSize + event.len);
                    continue;
                }

                Logger::log<LogLevel::debug>(
                    "File created/deleted. event.name: %s", fileName);
                if (event.mask == IN_CREATE)
                {
                    if (fileWatchDesc != -1)
                    {
                        Logger::log<LogLevel::debug>(
                            "Remove and Add inotify watcher on file");
                        inotify_rm_watch(inotifyFd, fileWatchDesc);
                        fileWatchDesc = -1;
                    }

                    fileWatchDesc = inotify_add_watch(
                        inotifyFd, filePath.c_str(), IN_MODIFY);
                    if (fileWatchDesc == -1)
                    {
                        Logger::log<LogLevel::error>(
                            "inotify_add_watch failed for file %s",
                            filePath.string());
                        return;
                    }

                    resetFilePosition();
                    readFromFile();
                }
                else if ((event.mask == IN_DELETE) ||
                         (event.mask == IN_MOVED_TO))
                {
                    if (fileWatchDesc != -1)
                    {
                        inotify_rm_watch(inotifyFd, fileWatchDesc);
                        fileWatchDesc = -1;
                    }
                }
            }
            else if (event.wd == fileWatchDesc)
            {
                if (event.mask == IN_MODIFY)
                {
                    readFromFile();
                }
            }
            index += (eventSize + event.len);
        }
    }
};
} // namespace nodemanager
