#pragma once

namespace cinder::lua { class LuaApi; }
namespace cinder::scene { class Scene; }

namespace cinder::script {

void registerSceneApi(cinder::lua::LuaApi& api, cinder::scene::Scene& scene);

}
