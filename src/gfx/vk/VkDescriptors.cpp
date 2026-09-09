#include "gfx/vk/VkDescriptors.hpp"

#include "gfx/vk/VkCtx.hpp"
#include "gfx/vk/VkUtil.hpp"

namespace cinder::gfx::vk::descriptors {

VkDescriptorPool pool(const VkCtx& ctx, uint32_t maxSets) {
    VkDescriptorPoolSize size{};
    size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    size.descriptorCount = maxSets;

    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = 1;
    info.pPoolSizes = &size;
    info.maxSets = maxSets;

    VkDescriptorPool handle = VK_NULL_HANDLE;
    check(vkCreateDescriptorPool(ctx.device(), &info, nullptr, &handle), "vkCreateDescriptorPool");
    return handle;
}

VkDescriptorSet allocate(const VkCtx& ctx, VkDescriptorPool pool, VkDescriptorSetLayout layout) {
    VkDescriptorSetAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.descriptorPool = pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;

    VkDescriptorSet set = VK_NULL_HANDLE;
    check(vkAllocateDescriptorSets(ctx.device(), &info, &set), "vkAllocateDescriptorSets");
    return set;
}

void writeCombinedImageSampler(const VkCtx& ctx, VkDescriptorSet set,
                               VkImageView view, VkSampler sampler) {
    VkDescriptorImageInfo image{};
    image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image.imageView = view;
    image.sampler = sampler;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &image;

    vkUpdateDescriptorSets(ctx.device(), 1, &write, 0, nullptr);
}

}
