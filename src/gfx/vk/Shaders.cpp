#include "gfx/vk/Shaders.hpp"

#include "gfx/vk/VkCtx.hpp"
#include "gfx/vk/VkUtil.hpp"
#include "platform/Assets.hpp"

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace cinder::gfx::vk::shaders {

VkShaderModule fromFile(const VkCtx& ctx, std::string_view name) {
    const std::filesystem::path path = cinder::platform::shaderPath(std::string(name) + ".spv");

    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) throw std::runtime_error("Cannot read shader " + path.string());

    const std::streamsize size = in.tellg();
    if (size <= 0 || size % 4 != 0) {
        throw std::runtime_error("Not a SPIR-V module: " + path.string());
    }

    std::vector<uint32_t> code(static_cast<std::size_t>(size) / 4);
    in.seekg(0);
    in.read(reinterpret_cast<char*>(code.data()), size);

    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = static_cast<std::size_t>(size);
    info.pCode = code.data();

    VkShaderModule handle = VK_NULL_HANDLE;
    check(vkCreateShaderModule(ctx.device(), &info, nullptr, &handle), "vkCreateShaderModule");
    return handle;
}

}
