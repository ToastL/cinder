#pragma once

#include "core/ProjectDescriptor.hpp"

#include <filesystem>
#include <string>

namespace cinder::serial { class Archive; }

namespace cinder::core {

struct ProjectConfig {
    std::string title = "Cinder";
    int width = 1280;
    int height = 720;
    std::string startScene = "Scenes/main.scene";
    int fixedHz = 60;
    ProjectDescriptor descriptor;

    ProjectConfig() = default;
    ProjectConfig(std::string title, int width, int height, std::string startScene, int fixedHz);

    static std::filesystem::path root(const std::filesystem::path& argument);
    static ProjectConfig load(const std::filesystem::path& folder);
    static std::string save(const ProjectConfig& config);

    void walk(cinder::serial::Archive& ar);
};

}
