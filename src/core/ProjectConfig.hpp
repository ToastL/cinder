#pragma once

#include <filesystem>
#include <string>

namespace cinder::core {

struct ProjectConfig {
    std::string title = "Cinder";
    int width = 1280;
    int height = 720;
    std::string scene = "scenes/main.scene";
    int fixedHz = 60;

    ProjectConfig() = default;
    ProjectConfig(std::string title, int width, int height, std::string scene, int fixedHz);

    static ProjectConfig load(const std::filesystem::path& path);
};

}
