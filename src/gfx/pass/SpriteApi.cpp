#include "gfx/pass/SpriteApi.hpp"

#include "gfx/pass/SpritePass.hpp"
#include "lua/LuaApi.hpp"
#include "scene/DrawList.hpp"

namespace cinder::gfx::pass {
namespace {

SpritePass& self(lua_State* state) {
    return *cinder::lua::LuaApi::context<SpritePass>(state);
}

int drawSprite(lua_State* state) {
    using cinder::lua::LuaApi;
    self(state).draw(static_cast<int>(lua_tointeger(state, 1)),
                     static_cast<float>(lua_tonumber(state, 2)),
                     static_cast<float>(lua_tonumber(state, 3)),
                     static_cast<float>(lua_tonumber(state, 4)),
                     static_cast<float>(lua_tonumber(state, 5)),
                     LuaApi::optFloat(state, 6, 0.0f),
                     LuaApi::optFloat(state, 7, 1.0f),
                     LuaApi::optFloat(state, 8, 1.0f),
                     LuaApi::optFloat(state, 9, 1.0f),
                     LuaApi::optFloat(state, 10, 1.0f));
    return 0;
}

int drawSpriteRegion(lua_State* state) {
    using cinder::lua::LuaApi;
    self(state).drawRegion(static_cast<int>(lua_tointeger(state, 1)),
                           static_cast<float>(lua_tonumber(state, 2)),
                           static_cast<float>(lua_tonumber(state, 3)),
                           static_cast<float>(lua_tonumber(state, 4)),
                           static_cast<float>(lua_tonumber(state, 5)),
                           static_cast<float>(lua_tonumber(state, 6)),
                           static_cast<float>(lua_tonumber(state, 7)),
                           static_cast<float>(lua_tonumber(state, 8)),
                           static_cast<float>(lua_tonumber(state, 9)),
                           LuaApi::optFloat(state, 10, 0.0f),
                           LuaApi::optFloat(state, 11, 1.0f),
                           LuaApi::optFloat(state, 12, 1.0f),
                           LuaApi::optFloat(state, 13, 1.0f),
                           LuaApi::optFloat(state, 14, 1.0f));
    return 0;
}

int drawRect(lua_State* state) {
    using cinder::lua::LuaApi;
    self(state).draw(cinder::scene::DrawList::WHITE,
                     static_cast<float>(lua_tonumber(state, 1)),
                     static_cast<float>(lua_tonumber(state, 2)),
                     static_cast<float>(lua_tonumber(state, 3)),
                     static_cast<float>(lua_tonumber(state, 4)),
                     0.0f,
                     LuaApi::optFloat(state, 5, 1.0f),
                     LuaApi::optFloat(state, 6, 1.0f),
                     LuaApi::optFloat(state, 7, 1.0f),
                     LuaApi::optFloat(state, 8, 1.0f));
    return 0;
}

int screenSize(lua_State* state) {
    OrthographicCamera& camera = self(state).camera();
    lua_pushnumber(state, camera.virtualWidth());
    lua_pushnumber(state, camera.virtualHeight());
    return 2;
}

int screenToWorld(lua_State* state) {
    const glm::vec3 world = self(state).screenToWorld(static_cast<float>(lua_tonumber(state, 1)),
                                                      static_cast<float>(lua_tonumber(state, 2)));
    lua_pushnumber(state, world.x);
    lua_pushnumber(state, world.y);
    return 2;
}

}

void registerSpriteApi(cinder::lua::LuaApi& api, SpritePass& pass) {
    api.bind("drawSprite", drawSprite, &pass);
    api.bind("drawSpriteRegion", drawSpriteRegion, &pass);
    api.bind("drawRect", drawRect, &pass);
    api.bind("screenSize", screenSize, &pass);
    api.bind("screenToWorld", screenToWorld, &pass);
}

}
