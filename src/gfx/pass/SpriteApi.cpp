#include "gfx/pass/SpriteApi.hpp"

#include "gfx/pass/SpritePass.hpp"
#include "lua/LuaApi.hpp"
#include "scene/DrawList.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace cinder::gfx::pass {
namespace {

using cinder::lua::LuaApi;

SpritePass& self(lua_State* state) {
    return *LuaApi::context<SpritePass>(state);
}

float number(lua_State* state, int index) {
    return static_cast<float>(lua_tonumber(state, index));
}

glm::vec4 color(lua_State* state, int first) {
    return glm::vec4(LuaApi::optFloat(state, first, 1.0f), LuaApi::optFloat(state, first + 1, 1.0f),
                     LuaApi::optFloat(state, first + 2, 1.0f), LuaApi::optFloat(state, first + 3, 1.0f));
}

glm::mat4 placed(float x, float y, float w, float h, float rot) {
    const glm::mat4 centred =
            glm::translate(glm::mat4(1.0f), glm::vec3(x + w * 0.5f, y + h * 0.5f, 0.0f));
    return glm::rotate(centred, rot, glm::vec3(0.0f, 0.0f, 1.0f));
}

int drawSprite(lua_State* state) {
    const float w = number(state, 4);
    const float h = number(state, 5);
    self(state).draw(static_cast<int>(lua_tointeger(state, 1)),
                     placed(number(state, 2), number(state, 3), w, h, LuaApi::optFloat(state, 6, 0.0f)),
                     glm::vec2(w, h), color(state, 7));
    return 0;
}

int drawSpriteRegion(lua_State* state) {
    const float w = number(state, 4);
    const float h = number(state, 5);
    self(state).drawRegion(static_cast<int>(lua_tointeger(state, 1)),
                           placed(number(state, 2), number(state, 3), w, h,
                                  LuaApi::optFloat(state, 10, 0.0f)),
                           glm::vec2(w, h),
                           glm::vec4(number(state, 6), number(state, 7), number(state, 8),
                                     number(state, 9)),
                           color(state, 11));
    return 0;
}

int drawRect(lua_State* state) {
    const float w = number(state, 3);
    const float h = number(state, 4);
    self(state).draw(cinder::scene::DrawList::WHITE,
                     placed(number(state, 1), number(state, 2), w, h, 0.0f), glm::vec2(w, h),
                     color(state, 5));
    return 0;
}

}

void registerSpriteApi(cinder::lua::LuaApi& api, SpritePass& pass) {
    api.bind("drawSprite", drawSprite, &pass);
    api.bind("drawSpriteRegion", drawSpriteRegion, &pass);
    api.bind("drawRect", drawRect, &pass);
}

}
