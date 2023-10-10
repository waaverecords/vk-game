#include "utilities.hpp"
#include <fstream>
#include <ios>

std::vector<char> Utilities::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("Faile to open file.");

    auto fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

Utilities::FileWatcher::FileWatcher(
    const std::vector<std::string>& filenames,
    std::function<void(const std::string&)> onChangedCallback
) {
    filenames_ = filenames;
    
    for (const auto& filename : filenames)
        lastModifiedTimes_[filename] = std::filesystem::last_write_time(filename);

    onChangedCallback_ = onChangedCallback;
}

Utilities::FileWatcher::~FileWatcher() {
    stop();
    watchingThread_.join();
}

void Utilities::FileWatcher::start() {
    watching_ = true;
    watchingThread_ = std::thread(&watch_, this);
}

void Utilities::FileWatcher::stop() {
    watching_ = false;
}

void Utilities::FileWatcher::watch_() {
    while (watching_) {
        for (const auto& filename : filenames_) {
            auto currentModifiedTime = std::filesystem::last_write_time(filename);
            if (lastModifiedTimes_[filename] != currentModifiedTime) {
                lastModifiedTimes_[filename] = currentModifiedTime;
                onChangedCallback_(filename);
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}