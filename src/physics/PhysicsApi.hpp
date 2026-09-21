#pragma once

namespace cinder::lua { class LuaApi; }

namespace cinder::physics {

class World;

void registerPhysicsApi(cinder::lua::LuaApi& api, World& world);

}
