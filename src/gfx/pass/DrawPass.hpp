#pragma once

#include <volk.h>

namespace cinder::lua { class LuaApi; }

namespace cinder::gfx::pass {

class DrawPass {
public:
    virtual ~DrawPass() = default;

    virtual void beginFrame() = 0;
    virtual void record(VkCommandBuffer cmd, uint32_t frameInFlight) = 0;
    virtual void registerApi(cinder::lua::LuaApi& api) = 0;
    virtual void resize(int width, int height) = 0;
};

}
