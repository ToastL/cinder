#include "core/GameConfig.hpp"

#include "core/GameLoop.hpp"
#include "lua/LuaTable.hpp"

#include <utility>

namespace cinder::core {

GameConfig::GameConfig(std::string title, int width, int height, std::string scene, int fixedHz)
    : title(std::move(title)), width(width), height(height), scene(std::move(scene)),
      fixedHz(fixedHz) {
    GameLoop::requirePositiveHz(fixedHz, "game.fixed_hz");
}

GameConfig GameConfig::load(const std::filesystem::path& path) {
    cinder::lua::LuaTable game = cinder::lua::LuaTable::fromFile(path, "game");
    return GameConfig(game.str("title", "Cinder"),
                      game.num("width", 1280),
                      game.num("height", 720),
                      game.str("scene", "assets/scenes/main.scene"),
                      game.num("fixed_hz", 60));
}

}
