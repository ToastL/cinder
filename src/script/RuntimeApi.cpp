#include "script/RuntimeApi.hpp"

#include "gfx/Renderer.hpp"
#include "lua/LuaApi.hpp"
#include "platform/Assets.hpp"
#include "platform/Glfw.hpp"
#include "platform/Input.hpp"
#include "platform/Log.hpp"

#include <exception>

namespace cinder::script {
namespace {

using cinder::lua::LuaApi;

RuntimeContext& host(lua_State* state) { return *LuaApi::context<RuntimeContext>(state); }

int time(lua_State* state) {
    lua_pushnumber(state, cinder::platform::Glfw::time());
    return 1;
}

int quit(lua_State* state) {
    (*host(state).quit)();
    return 0;
}

int logMessage(lua_State* state) {
    const char* message = luaL_tolstring(state, 1, nullptr);
    cinder::platform::logInfo("[game] %s\n", message != nullptr ? message : "nil");
    lua_pop(state, 1);
    return 0;
}

int loadTexture(lua_State* state) {
    const char* name = luaL_checkstring(state, 1);

    int handle = -1;
    try {
        handle = host(state).renderer->assets().load(cinder::platform::contentPath(name).string());
    } catch (const std::exception& e) {
        luaL_where(state, 1);
        lua_pushstring(state, e.what());
        lua_concat(state, 2);
    }
    if (handle < 0) return lua_error(state);

    lua_pushinteger(state, handle);
    return 1;
}

int textureSize(lua_State* state) {
    const auto& texture = host(state).renderer->assets().get(
            static_cast<int>(lua_tointeger(state, 1)));
    lua_pushinteger(state, texture.width());
    lua_pushinteger(state, texture.height());
    return 2;
}

int keyDown(lua_State* state) {
    lua_pushboolean(state, host(state).input->keyDown(lua_tostring(state, 1)));
    return 1;
}

int keyPressed(lua_State* state) {
    lua_pushboolean(state, host(state).input->keyPressed(lua_tostring(state, 1)));
    return 1;
}

int keyReleased(lua_State* state) {
    lua_pushboolean(state, host(state).input->keyReleased(lua_tostring(state, 1)));
    return 1;
}

int mouseDown(lua_State* state) {
    lua_pushboolean(state, host(state).input->mouseDown(lua_tostring(state, 1)));
    return 1;
}

int mousePressed(lua_State* state) {
    lua_pushboolean(state, host(state).input->mousePressed(lua_tostring(state, 1)));
    return 1;
}

int mouseReleased(lua_State* state) {
    lua_pushboolean(state, host(state).input->mouseReleased(lua_tostring(state, 1)));
    return 1;
}

int mousePosition(lua_State* state) {
    lua_pushnumber(state, host(state).input->mouseX());
    lua_pushnumber(state, host(state).input->mouseY());
    return 2;
}

int mouseDelta(lua_State* state) {
    lua_pushnumber(state, host(state).input->mouseDeltaX());
    lua_pushnumber(state, host(state).input->mouseDeltaY());
    return 2;
}

int scroll(lua_State* state) {
    lua_pushnumber(state, host(state).input->scrollX());
    lua_pushnumber(state, host(state).input->scrollY());
    return 2;
}

int setCursorLocked(lua_State* state) {
    host(state).input->setCursorLocked(lua_toboolean(state, 1) != 0);
    return 0;
}

int cursorLocked(lua_State* state) {
    lua_pushboolean(state, host(state).input->cursorLocked());
    return 1;
}

}

void registerRuntimeApi(cinder::lua::LuaApi& api, RuntimeContext& context) {
    api.bind("time", time, &context);
    api.bind("quit", quit, &context);
    api.bind("log", logMessage, &context);
    api.bind("loadTexture", loadTexture, &context);
    api.bind("textureSize", textureSize, &context);

    api.bind("keyDown", keyDown, &context);
    api.bind("keyPressed", keyPressed, &context);
    api.bind("keyReleased", keyReleased, &context);

    api.bind("mouseDown", mouseDown, &context);
    api.bind("mousePressed", mousePressed, &context);
    api.bind("mouseReleased", mouseReleased, &context);

    api.bind("mousePosition", mousePosition, &context);
    api.bind("mouseDelta", mouseDelta, &context);
    api.bind("scroll", scroll, &context);
    api.bind("setCursorLocked", setCursorLocked, &context);
    api.bind("cursorLocked", cursorLocked, &context);
}

}
