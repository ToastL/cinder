#pragma once

#include "dev/Staging.hpp"

#include <atomic>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace cinder::dev {

struct PackageTools {
    std::filesystem::path packager;

    static PackageTools fromBuild();
};

std::string packageCommand(const PackageTools& tools, Platform platform, Configuration configuration,
                           const std::filesystem::path& project, const std::filesystem::path& output);

class Packager {
public:
    Packager(std::filesystem::path project, std::vector<std::string> targetPlatforms,
             PackageTools tools = PackageTools::fromBuild());
    ~Packager();

    Packager(const Packager&) = delete;
    Packager& operator=(const Packager&) = delete;

    bool targets(Platform platform) const;
    bool canPackage(Platform platform) const;
    std::string unavailableReason(Platform platform) const;
    std::filesystem::path outputDir(Platform platform) const;

    Configuration configuration() const { return configuration_; }
    void setConfiguration(Configuration configuration) { configuration_ = configuration; }

    bool busy() const { return running_.load(); }
    std::optional<Platform> current() const { return busy() ? current_ : std::nullopt; }

    bool start(Platform platform);
    void cancel();
    void update();

private:
    void finish();

    std::filesystem::path project_;
    std::vector<std::string> targetPlatforms_;
    PackageTools tools_;
    std::string name_;
    Configuration configuration_ = Configuration::Development;
    std::thread worker_;
    int pid_ = 0;
    std::atomic<bool> cancelled_ = false;
    std::atomic<bool> running_ = false;
    std::optional<Platform> current_;
    std::mutex mutex_;
    std::deque<std::string> lines_;
    int status_ = 0;
    bool reported_ = true;
};

}
