#pragma once

#include "dev/GameBuild.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace cinder::dev {

enum class Platform : std::uint8_t { MacOS, Linux, Windows, IOS, Android };
enum class Configuration : std::uint8_t { Development, Shipping };

std::span<const Platform> platforms();
std::string_view platformName(Platform platform);
std::optional<Platform> platformNamed(std::string_view name);
Platform hostPlatform();
bool platformSupported(Platform platform);
std::string_view configurationName(Configuration configuration);
std::optional<Configuration> configurationNamed(std::string_view name);
std::string quote(const std::filesystem::path& path);

struct StageRequest {
    std::filesystem::path project;
    Platform platform = Platform::MacOS;
    Configuration configuration = Configuration::Development;
    std::filesystem::path out;
    std::filesystem::path player;
    std::filesystem::path engineDir;
    std::filesystem::path shaderDir;
    std::optional<GameBuild> build;
};

void stage(const StageRequest& request);

}
