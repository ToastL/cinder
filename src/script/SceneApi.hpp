#pragma once

#include <memory>

struct lua_State;

namespace cinder::lua { class LuaApi; }
namespace cinder::physics { class ContactObserver; }
namespace cinder::scene {
class Scene;
class SceneObserver;
}

namespace cinder::script {

void registerSceneApi(cinder::lua::LuaApi& api, cinder::scene::Scene& scene);
std::unique_ptr<cinder::scene::SceneObserver> makeSceneObserver(lua_State* state);
std::unique_ptr<cinder::physics::ContactObserver> makeContactObserver(lua_State* state);

}
