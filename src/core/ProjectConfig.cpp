#include "core/ProjectConfig.hpp"

#include "core/GameLoop.hpp"
#include "lua/LuaTable.hpp"

#include <utility>

namespace cinder::core {

ProjectConfig::ProjectConfig(std::string title, int width, int height, std::string scene,
                             int fixedHz)
    : title(std::move(title)), width(width), height(height), scene(std::move(scene)),
      fixedHz(fixedHz) {
    GameLoop::requirePositiveHz(fixedHz, "project.fixed_hz");
}

ProjectConfig ProjectConfig::load(const std::filesystem::path& path) {
    cinder::lua::LuaTable project = cinder::lua::LuaTable::fromFile(path, "project");
    return ProjectConfig(project.str("title", "Cinder"),
                         project.num("width", 1280),
                         project.num("height", 720),
                         project.str("scene", "scenes/main.scene"),
                         project.num("fixed_hz", 60));
}

}
