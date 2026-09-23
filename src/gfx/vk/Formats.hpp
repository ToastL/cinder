#pragma once

#include "gfx/rhi/Format.hpp"

#include <volk.h>

namespace cinder::gfx::rhi {

VkFormat toVk(Format format);
VkFormat toVk(VertexFormat format);
Format fromVk(VkFormat format);

}
