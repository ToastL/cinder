#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace cinder::Dev {

struct ContentActions {
    std::function<void(const std::filesystem::path&)> openScene;
    std::function<void(std::string title, std::string message, std::string confirm, std::function<void()> action)>
            confirm;
    std::function<void(const std::string&)> reveal;
    std::function<std::filesystem::path()> openPath;
    std::function<bool()> enabled;

    bool available() const { return !enabled || enabled(); }
};

}
