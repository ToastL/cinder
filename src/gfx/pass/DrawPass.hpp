#pragma once

#include <glm/mat4x4.hpp>
#include <volk.h>

namespace cinder::lua { class LuaApi; }

namespace cinder::gfx::pass {

class DrawPass {
public:
    virtual ~DrawPass() = default;

    virtual void beginFrame() = 0;
    virtual void record(VkCommandBuffer cmd, uint32_t frameInFlight, const glm::mat4& viewProjection) = 0;
    virtual void registerApi(cinder::lua::LuaApi& api) = 0;
};

}
