#pragma once

#include "gfx/rhi/Format.hpp"

#include <volk.h>

namespace cinder::gfx::vk { class VkCtx; }

namespace cinder::gfx::rhi {

class Presenter {
public:
    Presenter(const cinder::gfx::vk::VkCtx& ctx, Format colorFormat);
    ~Presenter();

    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

    void rebuild(Format colorFormat);

    const cinder::gfx::vk::VkCtx& ctx() const { return ctx_; }
    Format colorFormat() const { return colorFormat_; }
    VkRenderPass renderPass(PassKind pass) const;

private:
    void create();
    void destroy();

    const cinder::gfx::vk::VkCtx& ctx_;
    Format colorFormat_ = Format::Undefined;
    VkRenderPass scene_ = VK_NULL_HANDLE;
    VkRenderPass present_ = VK_NULL_HANDLE;
};

}
