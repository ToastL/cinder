#pragma once

#include "Gfx/RHI/Fwd.hpp"

#include "Gfx/RHI/Handles.hpp"
#include "Gfx/RHI/Backend.hpp"

#include <memory>

namespace cinder::Gfx::Pass {

class MeshPipeline {
public:
    explicit MeshPipeline(const cinder::Gfx::RHI::Presenter& presenter);

    void bind(cinder::Gfx::RHI::Commands cmd, const glm::mat4& viewProjection) const;
    void pushModel(cinder::Gfx::RHI::Commands cmd, const glm::mat4& model) const;
    void bindTexture(cinder::Gfx::RHI::Commands cmd,
                     cinder::Gfx::RHI::TextureBinding texture) const {
        pipeline_->bindTexture(cmd, texture);
    }

private:
    std::unique_ptr<cinder::Gfx::RHI::GraphicsPipeline> pipeline_;
};

}
