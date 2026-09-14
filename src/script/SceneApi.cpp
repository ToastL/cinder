#include "script/SceneApi.hpp"

#include "lua/LuaApi.hpp"
#include "platform/Log.hpp"
#include "scene/Attributes.hpp"
#include "scene/Node.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/Scene.hpp"
#include "scene/Transform.hpp"
#include "script/LuaProps.hpp"

#include <lua.hpp>

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cinder::script {
namespace {

using cinder::lua::LuaApi;
using cinder::reflect::PropDef;
using cinder::reflect::PropType;
using cinder::scene::Node;
using cinder::scene::PropValue;
using cinder::scene::Scene;
using cinder::scene::Transform;

Scene& sceneOf(lua_State* state) { return *LuaApi::context<Scene>(state); }

Node* nodeAt(lua_State* state, int index) {
    if (lua_isnoneornil(state, index)) return nullptr;
    return sceneOf(state).byId(static_cast<int>(lua_tointeger(state, index)));
}

Node* nodeOf(lua_State* state) { return nodeAt(state, 1); }

void pushNode(lua_State* state, const Node* node) {
    if (node == nullptr) lua_pushnil(state);
    else lua_pushinteger(state, node->id());
}

int pushIds(lua_State* state, const std::vector<Node*>& nodes) {
    lua_createtable(state, static_cast<int>(nodes.size()), 0);
    lua_Integer index = 0;
    for (Node* entry : nodes) {
        if (entry->destroyed()) continue;
        lua_pushinteger(state, entry->id());
        lua_rawseti(state, -2, ++index);
    }
    return 1;
}

Transform* transformOf(lua_State* state) {
    Node* node = nodeOf(state);
    return node == nullptr ? nullptr : node->transform();
}

const PropDef* propOf(const Node& node, const char* field) {
    if (field == nullptr) return nullptr;
    for (const PropDef& def : node.propList()) {
        if (def.name() == field) return &def;
    }
    return nullptr;
}

int pushVec3(lua_State* state, const glm::vec3& value) {
    lua_pushnumber(state, value.x);
    lua_pushnumber(state, value.y);
    lua_pushnumber(state, value.z);
    return 3;
}

int create(lua_State* state) {
    Scene& scene = sceneOf(state);
    const char* className = lua_tostring(state, 1);
    if (className == nullptr || scene.types().entry(className) == nullptr) {
        return luaL_error(state, "'%s' is not a node class", className != nullptr ? className : "nil");
    }
    pushNode(state, scene.create(className, nodeAt(state, 2)));
    return 1;
}

int className(lua_State* state) {
    Node* node = nodeOf(state);
    if (node == nullptr) return 0;
    const std::string_view type = sceneOf(state).types().nameOf(*node);
    lua_pushlstring(state, type.data(), type.size());
    return 1;
}

int name(lua_State* state) {
    Node* node = nodeOf(state);
    if (node == nullptr) return 0;
    lua_pushstring(state, node->name().c_str());
    return 1;
}

int setName(lua_State* state) {
    Node* node = nodeOf(state);
    const char* text = lua_tostring(state, 2);
    if (node != nullptr && text != nullptr) node->setName(text);
    return 0;
}

int parent(lua_State* state) {
    Node* node = nodeOf(state);
    pushNode(state, node == nullptr ? nullptr : node->parent());
    return 1;
}

int setParent(lua_State* state) {
    Node* node = nodeOf(state);
    if (node == nullptr) return 0;
    Node* next = nodeAt(state, 2);
    if (next == node || node->isAncestorOf(next)) {
        return luaL_error(state, "a node cannot be parented inside itself");
    }
    node->setParent(next);
    return 0;
}

int children(lua_State* state) {
    Node* node = nodeOf(state);
    if (node == nullptr) {
        lua_newtable(state);
        return 1;
    }
    return pushIds(state, node->children());
}

int roots(lua_State* state) { return pushIds(state, sceneOf(state).roots()); }

int find(lua_State* state) {
    const char* text = lua_tostring(state, 1);
    pushNode(state, text == nullptr ? nullptr : sceneOf(state).find(text));
    return 1;
}

int findChild(lua_State* state) {
    Node* node = nodeOf(state);
    const char* text = lua_tostring(state, 2);
    pushNode(state, node == nullptr || text == nullptr ? nullptr : node->findFirstChild(text));
    return 1;
}

int clone(lua_State* state) {
    Node* node = nodeOf(state);
    pushNode(state, node == nullptr ? nullptr : sceneOf(state).clone(*node, node->parent()));
    return 1;
}

int destroy(lua_State* state) {
    sceneOf(state).destroy(nodeOf(state));
    return 0;
}

int valid(lua_State* state) {
    Node* node = nodeOf(state);
    lua_pushboolean(state, node != nullptr && !node->destroyed());
    return 1;
}

int setPosition(lua_State* state) {
    Transform* t = transformOf(state);
    if (t != nullptr) {
        t->setPosition(static_cast<float>(lua_tonumber(state, 2)),
                       static_cast<float>(lua_tonumber(state, 3)),
                       LuaApi::optFloat(state, 4, 0.0f));
    }
    lua_pushboolean(state, t != nullptr);
    return 1;
}

int setRotation(lua_State* state) {
    Transform* t = transformOf(state);
    if (t != nullptr) {
        t->setRotation(static_cast<float>(lua_tonumber(state, 2)),
                       static_cast<float>(lua_tonumber(state, 3)),
                       LuaApi::optFloat(state, 4, 0.0f));
    }
    lua_pushboolean(state, t != nullptr);
    return 1;
}

int setScale(lua_State* state) {
    Transform* t = transformOf(state);
    if (t != nullptr) {
        t->setScale(static_cast<float>(lua_tonumber(state, 2)),
                    static_cast<float>(lua_tonumber(state, 3)),
                    LuaApi::optFloat(state, 4, 1.0f));
    }
    lua_pushboolean(state, t != nullptr);
    return 1;
}

int translate(lua_State* state) {
    Transform* t = transformOf(state);
    if (t != nullptr) {
        t->translate(static_cast<float>(lua_tonumber(state, 2)),
                     static_cast<float>(lua_tonumber(state, 3)),
                     LuaApi::optFloat(state, 4, 0.0f));
    }
    lua_pushboolean(state, t != nullptr);
    return 1;
}

int position(lua_State* state) {
    Transform* t = transformOf(state);
    return t == nullptr ? 0 : pushVec3(state, t->position());
}

int rotation(lua_State* state) {
    Transform* t = transformOf(state);
    return t == nullptr ? 0 : pushVec3(state, t->rotation());
}

int scale(lua_State* state) {
    Transform* t = transformOf(state);
    return t == nullptr ? 0 : pushVec3(state, t->scale());
}

int worldPosition(lua_State* state) {
    Transform* t = transformOf(state);
    return t == nullptr ? 0 : pushVec3(state, t->worldPosition());
}

int forward(lua_State* state) {
    Transform* t = transformOf(state);
    return t == nullptr ? 0 : pushVec3(state, t->forward());
}

int right(lua_State* state) {
    Transform* t = transformOf(state);
    return t == nullptr ? 0 : pushVec3(state, t->right());
}

int up(lua_State* state) {
    Transform* t = transformOf(state);
    return t == nullptr ? 0 : pushVec3(state, t->up());
}

int setProp(lua_State* state) {
    Node* node = nodeOf(state);
    const PropDef* prop = node == nullptr ? nullptr : propOf(*node, lua_tostring(state, 2));
    if (prop == nullptr) {
        lua_pushboolean(state, false);
        return 1;
    }

    void* target = node->propTarget();
    switch (prop->type()) {
        case PropType::Bool:
            prop->writeBool(target, lua_toboolean(state, 3) != 0);
            break;
        case PropType::String:
        case PropType::Enum: {
            const char* text = lua_tostring(state, 3);
            prop->writeText(target, text != nullptr ? text : "");
            break;
        }
        default: {
            float values[4]{};
            for (int i = 0; i < prop->arity(); ++i) {
                values[i] = static_cast<float>(lua_tonumber(state, 3 + i));
            }
            prop->write(target, values);
            break;
        }
    }
    lua_pushboolean(state, true);
    return 1;
}

int getProp(lua_State* state) {
    Node* node = nodeOf(state);
    const PropDef* prop = node == nullptr ? nullptr : propOf(*node, lua_tostring(state, 2));
    if (prop == nullptr) return 0;

    const void* target = node->propTarget();
    switch (prop->type()) {
        case PropType::Bool:
            lua_pushboolean(state, prop->readBool(target));
            return 1;
        case PropType::String:
        case PropType::Enum: {
            const std::string_view text = prop->readText(target);
            lua_pushlstring(state, text.data(), text.size());
            return 1;
        }
        default: {
            float values[4]{};
            prop->read(target, values);
            for (int i = 0; i < prop->arity(); ++i) lua_pushnumber(state, values[i]);
            return prop->arity();
        }
    }
}

enum class AttributeWrite { Done, BadName, BadValue };

AttributeWrite writeAttribute(lua_State* state, Node& node) {
    const char* key = lua_tostring(state, 2);
    if (key == nullptr || !cinder::scene::isAttributeName(key)) return AttributeWrite::BadName;

    if (lua_isnoneornil(state, 3)) {
        node.removeAttribute(key);
        return AttributeWrite::Done;
    }

    std::optional<PropValue> value = readProp(state, 3);
    if (!value || !cinder::scene::isAttributeValue(*value)) return AttributeWrite::BadValue;

    node.setAttribute(key, std::move(*value));
    return AttributeWrite::Done;
}

int setAttribute(lua_State* state) {
    Node* node = nodeOf(state);
    if (node == nullptr) return 0;

    switch (writeAttribute(state, *node)) {
        case AttributeWrite::BadName:
            return luaL_error(state, "'%s' is not an attribute name", luaL_tolstring(state, 2, nullptr));
        case AttributeWrite::BadValue:
            return luaL_error(state, "attribute '%s' cannot hold a %s", lua_tostring(state, 2),
                              luaL_typename(state, 3));
        case AttributeWrite::Done:
            break;
    }
    return 0;
}

int getAttribute(lua_State* state) {
    Node* node = nodeOf(state);
    const char* key = lua_tostring(state, 2);
    if (node == nullptr || key == nullptr) return 0;

    const PropValue* value = node->attribute(key);
    if (value == nullptr) return 0;

    pushProp(state, *value);
    return 1;
}

int getAttributes(lua_State* state) {
    Node* node = nodeOf(state);
    if (node == nullptr) lua_newtable(state);
    else pushRec(state, node->attributes());
    return 1;
}

class LuaObserver final : public cinder::scene::SceneObserver {
public:
    explicit LuaObserver(lua_State* state) : state_(state) {}

    void attributeChanged(Node& node, const std::string& key) override {
        const int top = lua_gettop(state_);
        if (!function(top, "__attributeChanged")) return;
        lua_pushinteger(state_, node.id());
        lua_pushlstring(state_, key.data(), key.size());
        call(top, 2);
    }

    void childAdded(Node& parent, Node& child) override { pair(parent, child, "__childAdded"); }

    void childRemoved(Node& parent, Node& child) override { pair(parent, child, "__childRemoved"); }

    void destroying(Node& node) override {
        const int top = lua_gettop(state_);
        if (!function(top, "__destroying")) return;
        lua_pushinteger(state_, node.id());
        call(top, 1);
    }

private:
    void pair(Node& parent, Node& child, const char* global) {
        const int top = lua_gettop(state_);
        if (!function(top, global)) return;
        lua_pushinteger(state_, parent.id());
        lua_pushinteger(state_, child.id());
        call(top, 2);
    }

    bool function(int top, const char* global) {
        lua_getglobal(state_, global);
        if (lua_isfunction(state_, -1)) return true;
        lua_settop(state_, top);
        return false;
    }

    void call(int top, int args) {
        if (lua_pcall(state_, args, 0, 0) != LUA_OK) {
            cinder::platform::logError("[lua] %s\n", lua_tostring(state_, -1));
        }
        lua_settop(state_, top);
    }

    lua_State* state_;
};

}

void registerSceneApi(cinder::lua::LuaApi& api, Scene& scene) {
    api.bind("create", create, &scene);
    api.bind("className", className, &scene);
    api.bind("name", name, &scene);
    api.bind("setName", setName, &scene);
    api.bind("parent", parent, &scene);
    api.bind("setParent", setParent, &scene);
    api.bind("children", children, &scene);
    api.bind("roots", roots, &scene);
    api.bind("find", find, &scene);
    api.bind("findChild", findChild, &scene);
    api.bind("clone", clone, &scene);
    api.bind("destroy", destroy, &scene);
    api.bind("valid", valid, &scene);

    api.bind("setPosition", setPosition, &scene);
    api.bind("setRotation", setRotation, &scene);
    api.bind("setScale", setScale, &scene);
    api.bind("translate", translate, &scene);
    api.bind("position", position, &scene);
    api.bind("rotation", rotation, &scene);
    api.bind("scale", scale, &scene);
    api.bind("worldPosition", worldPosition, &scene);
    api.bind("forward", forward, &scene);
    api.bind("right", right, &scene);
    api.bind("up", up, &scene);

    api.bind("setProp", setProp, &scene);
    api.bind("getProp", getProp, &scene);

    api.bind("getAttribute", getAttribute, &scene);
    api.bind("setAttribute", setAttribute, &scene);
    api.bind("getAttributes", getAttributes, &scene);
}

std::unique_ptr<cinder::scene::SceneObserver> makeSceneObserver(lua_State* state) {
    return std::make_unique<LuaObserver>(state);
}

}
