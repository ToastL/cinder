#include "script/SceneObservers.hpp"

#include "lua/LuaCalls.hpp"
#include "physics/World.hpp"
#include "scene/Node.hpp"
#include "scene/Scene.hpp"

namespace cinder::script {
namespace {

using cinder::scene::Node;

void luaPair(lua_State* state, const char* global, const Node& first, const Node& second) {
    cinder::lua::StackRestore stack(state);
    if (!cinder::lua::pushFunction(state, global)) return;
    lua_pushinteger(state, first.id());
    lua_pushinteger(state, second.id());
    cinder::lua::protectedCall(state, 2, 0);
}

class LuaObserver final : public cinder::scene::SceneObserver {
public:
    explicit LuaObserver(lua_State* state) : state_(state) {}

    void attributeChanged(Node& node, const std::string& key) override {
        cinder::lua::StackRestore stack(state_);
        if (!cinder::lua::pushFunction(state_, "__attributeChanged")) return;
        lua_pushinteger(state_, node.id());
        lua_pushlstring(state_, key.data(), key.size());
        cinder::lua::protectedCall(state_, 2, 0);
    }

    void childAdded(Node& parent, Node& child) override {
        luaPair(state_, "__childAdded", parent, child);
    }

    void childRemoved(Node& parent, Node& child) override {
        luaPair(state_, "__childRemoved", parent, child);
    }

    void destroying(Node& node) override {
        cinder::lua::StackRestore stack(state_);
        if (!cinder::lua::pushFunction(state_, "__destroying")) return;
        lua_pushinteger(state_, node.id());
        cinder::lua::protectedCall(state_, 1, 0);
    }

private:
    lua_State* state_;
};

class LuaContacts final : public cinder::physics::ContactObserver {
public:
    explicit LuaContacts(lua_State* state) : state_(state) {}

    void touched(Node& a, Node& b) override { luaPair(state_, "__touched", a, b); }

    void touchEnded(Node& a, Node& b) override { luaPair(state_, "__touchEnded", a, b); }

private:
    lua_State* state_;
};

}

std::unique_ptr<cinder::scene::SceneObserver> makeSceneObserver(lua_State* state) {
    return std::make_unique<LuaObserver>(state);
}

std::unique_ptr<cinder::physics::ContactObserver> makeContactObserver(lua_State* state) {
    return std::make_unique<LuaContacts>(state);
}

}
