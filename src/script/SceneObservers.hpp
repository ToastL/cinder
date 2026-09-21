#pragma once

#include <memory>

struct lua_State;

namespace cinder::physics { class ContactObserver; }
namespace cinder::scene {
class SceneObserver;
}

namespace cinder::script {

std::unique_ptr<cinder::scene::SceneObserver> makeSceneObserver(lua_State* state);
std::unique_ptr<cinder::physics::ContactObserver> makeContactObserver(lua_State* state);

}
