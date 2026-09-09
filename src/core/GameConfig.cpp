#include "core/GameConfig.hpp"

#include "core/GameLoop.hpp"
#include "lua/LuaTable.hpp"

#include <utility>

namespace cinder::core {

GameConfig::GameConfig(std::string title, int width, int height, std::string script, int fixedHz)
    : title(std::move(title)), width(width), height(height), script(std::move(script)),
      fixedHz(fixedHz) {
    GameLoop::requirePositiveHz(fixedHz, "game.fixed_hz");
}

GameConfig GameConfig::load(const std::filesystem::path& path) {
    cinder::lua::LuaTable game = cinder::lua::LuaTable::fromFile(path, "game");
    return GameConfig(game.str("title", "Cinder"),
                      game.num("width", 1280),
                      game.num("height", 720),
                      game.str("script", "assets/scripts/main.lua"),
                      game.num("fixed_hz", 60));
}

}
