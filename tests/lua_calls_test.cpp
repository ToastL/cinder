#include <doctest/doctest.h>

#include "lua/LuaApi.hpp"
#include "lua/LuaCalls.hpp"
#include "script/RuntimeApi.hpp"

#include <functional>

namespace {

struct State {
    lua_State* value = luaL_newstate();

    State() { luaL_openlibs(value); }
    ~State() { lua_close(value); }

    State(const State&) = delete;
    State& operator=(const State&) = delete;
};

void install(lua_State* state, cinder::script::RuntimeContext& context) {
    cinder::lua::LuaApi api(state, nullptr);
    cinder::script::registerRuntimeApi(api, context);
    api.install("engine");
}

}

TEST_CASE("protected Lua calls preserve results and restore the surrounding stack") {
    State state;
    lua_pushinteger(state.value, 73);
    REQUIRE(cinder::lua::runChunk(state.value,
            "function sum(a, b) return a + b, a - b end\n"
            "function fail() error('expected failure') end", "=calls"));

    {
        cinder::lua::StackRestore stack(state.value);
        REQUIRE(cinder::lua::pushFunction(state.value, "sum"));
        lua_pushinteger(state.value, 9);
        lua_pushinteger(state.value, 4);
        REQUIRE(cinder::lua::protectedCall(state.value, 2, 2));
        CHECK(lua_tointeger(state.value, -2) == 13);
        CHECK(lua_tointeger(state.value, -1) == 5);
    }
    CHECK(lua_gettop(state.value) == 1);
    CHECK(lua_tointeger(state.value, 1) == 73);

    for (int i = 0; i < 3; ++i) {
        cinder::lua::callGlobal(state.value, "missing");
        cinder::lua::callGlobal(state.value, "fail");
        CHECK(lua_gettop(state.value) == 1);
    }

    REQUIRE(cinder::lua::pushFunction(state.value, "fail"));
    CHECK_FALSE(cinder::lua::protectedCall(state.value, 0, 1));
    CHECK(lua_gettop(state.value) == 1);
    CHECK(lua_tointeger(state.value, 1) == 73);
}

TEST_CASE("Lua method calls clean up their receiver after success missing methods and errors") {
    State state;
    REQUIRE(cinder::lua::runChunk(state.value,
            "object = { value = 0 }\n"
            "function object:add(value) self.value = self.value + value end\n"
            "function object:fail() error('expected failure') end", "=methods"));
    lua_getglobal(state.value, "object");
    const int ref = luaL_ref(state.value, LUA_REGISTRYINDEX);
    lua_pushinteger(state.value, 73);

    cinder::lua::callMethod(state.value, ref, "add", 4.0);
    cinder::lua::callMethod(state.value, ref, "missing");
    cinder::lua::callMethod(state.value, ref, "fail");
    CHECK(lua_gettop(state.value) == 1);
    CHECK(lua_tointeger(state.value, 1) == 73);
    REQUIRE(cinder::lua::runChunk(state.value, "assert(object.value == 4)", "=check"));
    luaL_unref(state.value, LUA_REGISTRYINDEX, ref);
}

TEST_CASE("runtime bindings keep their own context across independent Lua states and registration") {
    State first;
    State second;
    int firstCalls = 0;
    int secondCalls = 0;
    std::function<void()> firstQuit = [&] { ++firstCalls; };
    std::function<void()> secondQuit = [&] { ++secondCalls; };
    cinder::script::RuntimeContext firstContext{nullptr, nullptr, &firstQuit};
    cinder::script::RuntimeContext secondContext{nullptr, nullptr, &secondQuit};
    install(first.value, firstContext);
    install(second.value, secondContext);

    REQUIRE(cinder::lua::runChunk(first.value, "engine.quit()", "=first"));
    CHECK(firstCalls == 1);
    CHECK(secondCalls == 0);
    REQUIRE(cinder::lua::runChunk(second.value, "engine.quit()", "=second"));
    CHECK(firstCalls == 1);
    CHECK(secondCalls == 1);

    install(second.value, secondContext);
    REQUIRE(cinder::lua::runChunk(first.value, "engine.quit()", "=first again"));
    CHECK(firstCalls == 2);
    CHECK(secondCalls == 1);
    CHECK(lua_gettop(first.value) == 0);
    CHECK(lua_gettop(second.value) == 0);
}
