#include "script/SceneApi.hpp"

#include "lua/LuaApi.hpp"
#include "platform/Log.hpp"
#include "scene/Actor.hpp"
#include "scene/Attributes.hpp"
#include "scene/Components.hpp"
#include "scene/Scene.hpp"
#include "script/LuaProps.hpp"

#include <lua.hpp>

#include <optional>
#include <string>
#include <utility>

namespace cinder::script {
namespace {

using cinder::lua::LuaApi;
using cinder::reflect::PropDef;
using cinder::reflect::PropType;
using cinder::scene::Actor;
using cinder::scene::Component;
using cinder::scene::PropValue;
using cinder::scene::Scene;
using cinder::scene::Transform;

Scene& sceneOf(lua_State* state) { return *LuaApi::context<Scene>(state); }

Actor* actorOf(lua_State* state) {
    return sceneOf(state).byId(static_cast<int>(lua_tointeger(state, 1)));
}

Transform* transformOf(lua_State* state) {
    Actor* actor = actorOf(state);
    return actor == nullptr ? nullptr : &actor->transform();
}

Component* componentOf(lua_State* state) {
    Actor* actor = actorOf(state);
    if (actor == nullptr) return nullptr;

    const char* name = lua_tostring(state, 2);
    if (name == nullptr) return nullptr;

    const auto* entry = sceneOf(state).types().entry(name);
    return entry == nullptr ? nullptr : actor->get(entry->type);
}

const PropDef* propOf(const Component& component, const char* field) {
    if (field == nullptr) return nullptr;
    for (const PropDef& def : component.propList()) {
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

int spawn(lua_State* state) {
    Scene& scene = sceneOf(state);
    Actor* parent = lua_isnoneornil(state, 2)
            ? nullptr
            : scene.byId(static_cast<int>(lua_tointeger(state, 2)));
    lua_pushinteger(state, scene.spawn(lua_tostring(state, 1), parent)->id());
    return 1;
}

int destroy(lua_State* state) {
    sceneOf(state).destroy(actorOf(state));
    return 0;
}

int find(lua_State* state) {
    Actor* actor = sceneOf(state).find(lua_tostring(state, 1));
    if (actor == nullptr) lua_pushnil(state);
    else lua_pushinteger(state, actor->id());
    return 1;
}

int setParent(lua_State* state) {
    Actor* actor = actorOf(state);
    if (actor != nullptr) {
        actor->setParent(lua_isnoneornil(state, 2)
                                 ? nullptr
                                 : sceneOf(state).byId(static_cast<int>(lua_tointeger(state, 2))));
    }
    return 0;
}

int setActive(lua_State* state) {
    Actor* actor = actorOf(state);
    if (actor != nullptr) actor->setActive(lua_toboolean(state, 2) != 0);
    return 0;
}

int active(lua_State* state) {
    Actor* actor = actorOf(state);
    lua_pushboolean(state, actor != nullptr && actor->activeSelf());
    return 1;
}

int name(lua_State* state) {
    Actor* actor = actorOf(state);
    if (actor == nullptr) lua_pushnil(state);
    else lua_pushstring(state, actor->name().c_str());
    return 1;
}

int setName(lua_State* state) {
    Actor* actor = actorOf(state);
    if (actor != nullptr) actor->setName(lua_tostring(state, 2));
    return 0;
}

int parent(lua_State* state) {
    Actor* actor = actorOf(state);
    if (actor == nullptr || actor->parent() == nullptr) lua_pushnil(state);
    else lua_pushinteger(state, actor->parent()->id());
    return 1;
}

int children(lua_State* state) {
    Actor* actor = actorOf(state);
    const std::size_t count = actor == nullptr ? 0 : actor->children().size();

    lua_createtable(state, static_cast<int>(count), 0);
    for (std::size_t i = 0; i < count; ++i) {
        lua_pushinteger(state, actor->children()[i]->id());
        lua_rawseti(state, -2, static_cast<lua_Integer>(i) + 1);
    }
    return 1;
}

int valid(lua_State* state) {
    lua_pushboolean(state, actorOf(state) != nullptr);
    return 1;
}

int hasComponent(lua_State* state) {
    lua_pushboolean(state, componentOf(state) != nullptr);
    return 1;
}

int componentNames(lua_State* state) {
    const auto& entries = sceneOf(state).types().registered();
    lua_createtable(state, static_cast<int>(entries.size()), 0);
    for (std::size_t i = 0; i < entries.size(); ++i) {
        lua_pushstring(state, entries[i].name.c_str());
        lua_rawseti(state, -2, static_cast<lua_Integer>(i) + 1);
    }
    return 1;
}

int propNames(lua_State* state) {
    const char* type = lua_tostring(state, 1);
    const auto* props = type == nullptr ? nullptr : sceneOf(state).types().propsOf(type);
    const std::size_t count = props == nullptr ? 0 : props->size();

    lua_createtable(state, static_cast<int>(count), 0);
    for (std::size_t i = 0; i < count; ++i) {
        const std::string_view field = (*props)[i].name();
        lua_pushlstring(state, field.data(), field.size());
        lua_rawseti(state, -2, static_cast<lua_Integer>(i) + 1);
    }
    return 1;
}

int setPosition(lua_State* state) {
    Transform* t = transformOf(state);
    if (t != nullptr) {
        t->setPosition(static_cast<float>(lua_tonumber(state, 2)),
                       static_cast<float>(lua_tonumber(state, 3)),
                       LuaApi::optFloat(state, 4, 0.0f));
    }
    return 0;
}

int setRotation(lua_State* state) {
    Transform* t = transformOf(state);
    if (t != nullptr) {
        t->setRotation(static_cast<float>(lua_tonumber(state, 2)),
                       static_cast<float>(lua_tonumber(state, 3)),
                       LuaApi::optFloat(state, 4, 0.0f));
    }
    return 0;
}

int setScale(lua_State* state) {
    Transform* t = transformOf(state);
    if (t != nullptr) {
        t->setScale(static_cast<float>(lua_tonumber(state, 2)),
                    static_cast<float>(lua_tonumber(state, 3)),
                    LuaApi::optFloat(state, 4, 1.0f));
    }
    return 0;
}

int translate(lua_State* state) {
    Transform* t = transformOf(state);
    if (t != nullptr) {
        t->translate(static_cast<float>(lua_tonumber(state, 2)),
                     static_cast<float>(lua_tonumber(state, 3)),
                     LuaApi::optFloat(state, 4, 0.0f));
    }
    return 0;
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

int addComponent(lua_State* state) {
    Actor* actor = actorOf(state);
    if (actor == nullptr) return 0;

    const char* type = lua_tostring(state, 2);
    auto component = type == nullptr ? nullptr : sceneOf(state).types().create(type);
    if (component == nullptr) {
        cinder::platform::logError("[lua] unknown component type: %s\n", type != nullptr ? type : "nil");
        return 0;
    }

    actor->add(std::move(component));
    return 0;
}

enum class AttributeWrite { Done, BadName, BadValue };

AttributeWrite writeAttribute(lua_State* state, Actor& actor) {
    const char* name = lua_tostring(state, 2);
    if (name == nullptr || !cinder::scene::isAttributeName(name)) return AttributeWrite::BadName;

    if (lua_isnoneornil(state, 3)) {
        actor.removeAttribute(name);
        return AttributeWrite::Done;
    }

    std::optional<PropValue> value = readProp(state, 3);
    if (!value || !cinder::scene::isAttributeValue(*value)) return AttributeWrite::BadValue;

    actor.setAttribute(name, std::move(*value));
    return AttributeWrite::Done;
}

int setAttribute(lua_State* state) {
    Actor* actor = actorOf(state);
    if (actor == nullptr) return 0;

    switch (writeAttribute(state, *actor)) {
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
    Actor* actor = actorOf(state);
    const char* name = lua_tostring(state, 2);
    if (actor == nullptr || name == nullptr) return 0;

    const PropValue* value = actor->attribute(name);
    if (value == nullptr) return 0;

    pushProp(state, *value);
    return 1;
}

int getAttributes(lua_State* state) {
    Actor* actor = actorOf(state);
    if (actor == nullptr) lua_newtable(state);
    else pushRec(state, actor->attributes());
    return 1;
}

int setProp(lua_State* state) {
    Component* component = componentOf(state);
    if (component == nullptr) return 0;

    const PropDef* prop = propOf(*component, lua_tostring(state, 3));
    if (prop == nullptr) {
        cinder::platform::logError("[lua] %s has no prop %s\n", lua_tostring(state, 2),
                                   lua_tostring(state, 3));
        return 0;
    }

    void* target = component->propTarget();
    switch (prop->type()) {
        case PropType::Bool:
            prop->writeBool(target, lua_toboolean(state, 4) != 0);
            break;
        case PropType::String:
        case PropType::Enum: {
            const char* text = lua_tostring(state, 4);
            prop->writeText(target, text != nullptr ? text : "");
            break;
        }
        default: {
            float values[4]{};
            for (int i = 0; i < prop->arity(); ++i) {
                values[i] = static_cast<float>(lua_tonumber(state, 4 + i));
            }
            prop->write(target, values);
            break;
        }
    }
    return 0;
}

int getProp(lua_State* state) {
    Component* component = componentOf(state);
    if (component == nullptr) return 0;

    const PropDef* prop = propOf(*component, lua_tostring(state, 3));
    if (prop == nullptr) return 0;

    const void* target = component->propTarget();
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

}

void registerSceneApi(cinder::lua::LuaApi& api, Scene& scene) {
    api.bind("spawn", spawn, &scene);
    api.bind("destroy", destroy, &scene);
    api.bind("find", find, &scene);
    api.bind("setParent", setParent, &scene);
    api.bind("setActive", setActive, &scene);
    api.bind("active", active, &scene);
    api.bind("name", name, &scene);
    api.bind("setName", setName, &scene);
    api.bind("parent", parent, &scene);
    api.bind("children", children, &scene);
    api.bind("valid", valid, &scene);
    api.bind("hasComponent", hasComponent, &scene);
    api.bind("componentNames", componentNames, &scene);
    api.bind("propNames", propNames, &scene);

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

    api.bind("addComponent", addComponent, &scene);
    api.bind("setProp", setProp, &scene);
    api.bind("getProp", getProp, &scene);

    api.bind("getAttribute", getAttribute, &scene);
    api.bind("setAttribute", setAttribute, &scene);
    api.bind("getAttributes", getAttributes, &scene);
}

void listenForAttributes(lua_State* state, Scene& scene) {
    scene.setAttributeListener([state](Actor& actor, const std::string& name) {
        const int top = lua_gettop(state);
        lua_getglobal(state, "__attributeChanged");
        if (!lua_isfunction(state, -1)) {
            lua_settop(state, top);
            return;
        }

        lua_pushinteger(state, actor.id());
        lua_pushlstring(state, name.data(), name.size());
        if (lua_pcall(state, 2, 0, 0) != LUA_OK) {
            cinder::platform::logError("[lua] attributeChanged: %s\n", lua_tostring(state, -1));
        }
        lua_settop(state, top);
    });
}

}
