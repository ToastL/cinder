#pragma once

#include "gfx/rhi/Fwd.hpp"

#include "gfx/rhi/Handles.hpp"
#include "gfx/rhi/Backend.hpp"

#include <memory>

namespace cinder::gfx::pass {

class MeshPipeline {
public:
    explicit MeshPipeline(const cinder::gfx::rhi::Presenter& presenter);

    void bind(cinder::gfx::rhi::Commands cmd, const glm::mat4& viewProjection) const;
    void pushModel(cinder::gfx::rhi::Commands cmd, const glm::mat4& model) const;
    void bindTexture(cinder::gfx::rhi::Commands cmd,
                     cinder::gfx::rhi::TextureBinding texture) const {
        pipeline_->bindTexture(cmd, texture);
    }

private:
    std::unique_ptr<cinder::gfx::rhi::GraphicsPipeline> pipeline_;
};

}
