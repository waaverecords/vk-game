#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <filesystem>
#include <thread>
#include <functional>

namespace Utilities {
    std::vector<char> readFile(const std::string& filename);

    class FileWatcher {
    public:

        FileWatcher(const std::vector<std::string>& filenames, std::function<void(const std::string&)> onChangedCallback);
        ~FileWatcher();

        void start();
        void stop();

    private:
        
        std::thread watchingThread_;
        bool watching_;
        void watch_();

        std::vector<std::string> filenames_;
        std::unordered_map<std::string, std::filesystem::file_time_type> lastModifiedTimes_;

        std::function<void(const std::string&)> onChangedCallback_;
    };
}