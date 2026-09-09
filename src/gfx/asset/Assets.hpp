#pragma once

#include "gfx/asset/Texture.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace cinder::gfx::asset {

class Assets {
public:
    static constexpr uint32_t MAX_TEXTURES = 256;

    Assets(const cinder::gfx::vk::VkCtx& ctx, VkDescriptorSetLayout descriptorSetLayout);
    ~Assets();

    Assets(const Assets&) = delete;
    Assets& operator=(const Assets&) = delete;

    int load(const std::string& path);
    const Texture& get(int id) const;

private:
    int registerTexture(Texture texture);

    const cinder::gfx::vk::VkCtx& ctx_;
    VkDescriptorSetLayout layout_;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
    std::vector<Texture> textures_;
    std::unordered_map<std::string, int> byPath_;
};

}
