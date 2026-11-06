#pragma once

#include "gfx/rhi/Handles.hpp"

#include <volk.h>

namespace cinder::gfx::rhi {

inline VkCommandBuffer unwrap(Commands cmd) { return static_cast<VkCommandBuffer>(cmd.handle); }

inline VkCommandBuffer unwrap(Uploads cmd) { return static_cast<VkCommandBuffer>(cmd.handle); }

inline Commands commands(VkCommandBuffer cmd) { return Commands{cmd}; }

inline Uploads uploads(VkCommandBuffer cmd) { return Uploads{cmd}; }

}
