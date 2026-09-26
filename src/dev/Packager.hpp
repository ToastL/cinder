#pragma once

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

enum class Platform : std::uint8_t { MacOS, Linux, Windows, IOS, Android };

std::span<const Platform> platforms();
std::string_view platformName(Platform platform);
Platform hostPlatform();
bool platformSupported(Platform platform);

struct PackageTools {
    std::filesystem::path cmake;
    std::filesystem::path buildDir;
    std::filesystem::path player;
    std::filesystem::path script;
    std::filesystem::path engineDir;
    std::filesystem::path shaderDir;

    static PackageTools fromBuild();
};

std::string packageCommand(const PackageTools& tools, Platform platform, const std::filesystem::path& project,
                           const std::filesystem::path& output);

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

    bool busy() const { return running_.load(); }
    std::optional<Platform> current() const { return busy() ? current_ : std::nullopt; }

    bool start(Platform platform);
    void update();

private:
    void finish();

    std::filesystem::path project_;
    std::vector<std::string> targetPlatforms_;
    PackageTools tools_;
    std::string name_;
    std::thread worker_;
    std::atomic<bool> running_ = false;
    std::optional<Platform> current_;
    std::mutex mutex_;
    std::deque<std::string> lines_;
    int status_ = 0;
    bool reported_ = true;
};

}
