#pragma once

#include <functional>

namespace cinder::gfx { class Renderer; }
namespace cinder::lua { class LuaApi; }
namespace cinder::platform { class Input; }

namespace cinder::script {

struct RuntimeContext {
    cinder::gfx::Renderer* renderer;
    cinder::platform::Input* input;
    std::function<void()>* quit;
};

void registerRuntimeApi(cinder::lua::LuaApi& api, RuntimeContext& context);

}
