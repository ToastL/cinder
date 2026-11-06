#pragma once

#include "gfx/rhi/Fwd.hpp"

#include "gfx/rhi/Handles.hpp"

#include "gfx/rhi/Backend.hpp"

#include <memory>

namespace cinder::gfx {

class CompositePipeline {
public:
    explicit CompositePipeline(const cinder::gfx::rhi::Presenter& presenter);

    void draw(cinder::gfx::rhi::Commands cmd, cinder::gfx::rhi::TextureBinding texture) const;

private:
    static constexpr uint32_t FULLSCREEN_TRIANGLE_VERTICES = 3;

    std::unique_ptr<cinder::gfx::rhi::GraphicsPipeline> pipeline_;
};

}
