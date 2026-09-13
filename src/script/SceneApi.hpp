#pragma once

struct lua_State;

namespace cinder::lua { class LuaApi; }
namespace cinder::scene { class Scene; }

namespace cinder::script {

void registerSceneApi(cinder::lua::LuaApi& api, cinder::scene::Scene& scene);
void listenForAttributes(lua_State* state, cinder::scene::Scene& scene);

}
