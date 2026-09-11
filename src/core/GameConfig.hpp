#pragma once

#include <filesystem>
#include <string>

namespace cinder::core {

struct GameConfig {
    std::string title = "Cinder";
    int width = 1280;
    int height = 720;
    std::string scene = "assets/scenes/main.scene";
    int fixedHz = 60;

    GameConfig() = default;
    GameConfig(std::string title, int width, int height, std::string scene, int fixedHz);

    static GameConfig load(const std::filesystem::path& path);
};

}
