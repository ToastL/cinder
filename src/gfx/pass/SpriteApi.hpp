#pragma once

namespace cinder::lua { class LuaApi; }

namespace cinder::gfx::pass {

class SpritePass;

void registerSpriteApi(cinder::lua::LuaApi& api, SpritePass& pass);

}
