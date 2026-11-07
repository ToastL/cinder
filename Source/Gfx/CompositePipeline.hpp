#pragma once

#include "Gfx/RHI/Fwd.hpp"

#include "Gfx/RHI/Handles.hpp"

#include "Gfx/RHI/Backend.hpp"

#include <memory>

namespace cinder::Gfx {

class CompositePipeline {
public:
    explicit CompositePipeline(const cinder::Gfx::RHI::Presenter& presenter);

    void draw(cinder::Gfx::RHI::Commands cmd, cinder::Gfx::RHI::TextureBinding texture) const;

private:
    static constexpr uint32_t FULLSCREEN_TRIANGLE_VERTICES = 3;

    std::unique_ptr<cinder::Gfx::RHI::GraphicsPipeline> pipeline_;
};

}
