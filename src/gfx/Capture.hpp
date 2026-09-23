#pragma once

#include "gfx/rhi/Format.hpp"

#include <volk.h>

#include <string>

namespace cinder::gfx::vk { class VkCtx; }

namespace cinder::gfx {

class RenderTarget;

void captureTarget(const cinder::gfx::vk::VkCtx& ctx, const RenderTarget& target,
                   cinder::gfx::rhi::Format format, const std::string& path);
void writeCapture(const std::string& path, uint32_t width, uint32_t height, const void* pixels,
                  cinder::gfx::rhi::Format format);

}
