#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace cinder::dev {

struct GameBuild {
    std::filesystem::path cbt;
    std::filesystem::path output;
    std::string config;
    std::string backend;

    static std::optional<GameBuild> locate();
    static std::string_view defaultBackend();

    std::filesystem::path binaries() const;
    std::vector<std::string> game(const std::filesystem::path& project, const std::filesystem::path& library = {}) const;
    std::vector<std::string> build(const std::string& target) const;
};

}
