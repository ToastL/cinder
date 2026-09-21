#include "gfx/Overlay.hpp"

#include <utility>

namespace cinder::gfx {

OverlayTexture::OverlayTexture(Overlay& overlay, VkImageView view)
    : overlay_(&overlay), handle_(overlay.addTexture(view)) {}

OverlayTexture::~OverlayTexture() { release(); }

OverlayTexture::OverlayTexture(OverlayTexture&& other) noexcept
    : overlay_(std::exchange(other.overlay_, nullptr)),
      handle_(std::exchange(other.handle_, VK_NULL_HANDLE)) {}

OverlayTexture& OverlayTexture::operator=(OverlayTexture&& other) noexcept {
    if (this == &other) return *this;
    release();
    overlay_ = std::exchange(other.overlay_, nullptr);
    handle_ = std::exchange(other.handle_, VK_NULL_HANDLE);
    return *this;
}

void OverlayTexture::release() {
    if (overlay_ != nullptr) overlay_->removeTexture(handle_);
    overlay_ = nullptr;
    handle_ = VK_NULL_HANDLE;
}

}
