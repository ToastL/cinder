#include "physics/PhysicsApi.hpp"

#include "lua/LuaApi.hpp"
#include "physics/Body.hpp"
#include "physics/World.hpp"
#include "scene/Scene.hpp"

namespace cinder::physics {
namespace {

World& hostOf(lua_State* state) { return *cinder::lua::LuaApi::context<World>(state); }

glm::vec3 vectorAt(lua_State* state, int index) {
    return glm::vec3(static_cast<float>(lua_tonumber(state, index)),
                     static_cast<float>(lua_tonumber(state, index + 1)),
                     static_cast<float>(lua_tonumber(state, index + 2)));
}

Body* bodyAt(lua_State* state, int index) {
    const int id = static_cast<int>(lua_tointeger(state, index));
    return dynamic_cast<Body*>(hostOf(state).scene().byId(id));
}

int applyImpulse(lua_State* state) {
    Body* body = bodyAt(state, 1);
    if (body == nullptr) return luaL_error(state, "applyImpulse: the node is not a Body");

    if (lua_gettop(state) >= 7) body->applyImpulse(vectorAt(state, 2), vectorAt(state, 5));
    else body->applyImpulse(vectorAt(state, 2));
    return 0;
}

int applyForce(lua_State* state) {
    Body* body = bodyAt(state, 1);
    if (body == nullptr) return luaL_error(state, "applyForce: the node is not a Body");

    if (lua_gettop(state) >= 7) body->applyForce(vectorAt(state, 2), vectorAt(state, 5));
    else body->applyForce(vectorAt(state, 2));
    return 0;
}

int applyTorque(lua_State* state) {
    Body* body = bodyAt(state, 1);
    if (body == nullptr) return luaL_error(state, "applyTorque: the node is not a Body");

    body->applyTorque(vectorAt(state, 2));
    return 0;
}

int castRay(lua_State* state) {
    RayHit hit;
    const float reach = cinder::lua::LuaApi::optFloat(state, 7, 1000.0f);
    if (!hostOf(state).raycast(vectorAt(state, 1), vectorAt(state, 4), reach, hit)) return 0;

    lua_pushinteger(state, hit.node->id());
    lua_pushnumber(state, hit.position.x);
    lua_pushnumber(state, hit.position.y);
    lua_pushnumber(state, hit.position.z);
    lua_pushnumber(state, hit.normal.x);
    lua_pushnumber(state, hit.normal.y);
    lua_pushnumber(state, hit.normal.z);
    lua_pushnumber(state, hit.distance);
    return 8;
}

int readGravity(lua_State* state) {
    const glm::vec3& gravity = hostOf(state).gravity();
    lua_pushnumber(state, gravity.x);
    lua_pushnumber(state, gravity.y);
    lua_pushnumber(state, gravity.z);
    return 3;
}

int writeGravity(lua_State* state) {
    hostOf(state).setGravity(vectorAt(state, 1));
    return 0;
}

}

void registerPhysicsApi(cinder::lua::LuaApi& api, World& world) {
    api.bind("applyImpulse", applyImpulse, &world);
    api.bind("applyForce", applyForce, &world);
    api.bind("applyTorque", applyTorque, &world);
    api.bind("raycast", castRay, &world);
    api.bind("gravity", readGravity, &world);
    api.bind("setGravity", writeGravity, &world);
}

}
