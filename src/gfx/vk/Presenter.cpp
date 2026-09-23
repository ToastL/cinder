#include "gfx/vk/Presenter.hpp"

#include "gfx/vk/VkCtx.hpp"
#include "gfx/vk/VkRenderPasses.hpp"

namespace cinder::gfx::rhi {

namespace renderPasses = cinder::gfx::vk::renderPasses;

Presenter::Presenter(const cinder::gfx::vk::VkCtx& ctx, Format colorFormat)
    : ctx_(ctx), colorFormat_(colorFormat) {
    create();
}

void Presenter::create() {
    scene_ = renderPasses::scene(ctx_, colorFormat_);
    present_ = renderPasses::present(ctx_, colorFormat_);
}

void Presenter::destroy() {
    if (present_ != VK_NULL_HANDLE) vkDestroyRenderPass(ctx_.device(), present_, nullptr);
    if (scene_ != VK_NULL_HANDLE) vkDestroyRenderPass(ctx_.device(), scene_, nullptr);
    present_ = VK_NULL_HANDLE;
    scene_ = VK_NULL_HANDLE;
}

void Presenter::rebuild(Format colorFormat) {
    destroy();
    colorFormat_ = colorFormat;
    create();
}

VkRenderPass Presenter::renderPass(PassKind pass) const {
    return pass == PassKind::Scene ? scene_ : present_;
}

Presenter::~Presenter() { destroy(); }

}
