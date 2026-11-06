#include "gfx/rhi/Commands.hpp"

#include "gfx/vk/Commands.hpp"

namespace cinder::gfx::rhi {

void draw(Commands cmd, std::uint32_t vertexCount) {
    vkCmdDraw(unwrap(cmd), vertexCount, 1, 0, 0);
}

void drawIndexed(Commands cmd, std::uint32_t indexCount, std::uint32_t firstIndex) {
    vkCmdDrawIndexed(unwrap(cmd), indexCount, 1, firstIndex, 0, 0);
}

void viewport(Commands cmd, std::uint32_t width, std::uint32_t height) {
    VkViewport region{};
    region.x = 0.0f;
    region.y = 0.0f;
    region.width = static_cast<float>(width);
    region.height = static_cast<float>(height);
    region.minDepth = 0.0f;
    region.maxDepth = 1.0f;
    vkCmdSetViewport(unwrap(cmd), 0, 1, &region);
}

void scissor(Commands cmd, std::int32_t x, std::int32_t y, std::uint32_t width,
             std::uint32_t height) {
    VkRect2D region{};
    region.offset = {x, y};
    region.extent = {width, height};
    vkCmdSetScissor(unwrap(cmd), 0, 1, &region);
}

}
