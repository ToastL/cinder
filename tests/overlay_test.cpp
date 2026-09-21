#include <doctest/doctest.h>

#include "gfx/Overlay.hpp"

#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

class Overlay final : public cinder::gfx::Overlay {
public:
    void beginFrame() override {}
    void record(VkCommandBuffer) override {}
    void discardFrame() override {}
    void setMinImageCount(uint32_t) override {}

    VkDescriptorSet addTexture(VkImageView) override {
        return reinterpret_cast<VkDescriptorSet>(++next_);
    }

    void removeTexture(VkDescriptorSet texture) override { removed.push_back(texture); }

    std::vector<VkDescriptorSet> removed;

private:
    std::uintptr_t next_ = 0;
};

}

TEST_CASE("overlay texture ownership follows moves and unregisters each handle once") {
    Overlay overlay;
    VkDescriptorSet first;
    VkDescriptorSet second;
    {
        cinder::gfx::OverlayTexture texture(overlay, VK_NULL_HANDLE);
        first = texture.handle();
        cinder::gfx::OverlayTexture moved(std::move(texture));
        CHECK(texture.handle() == VK_NULL_HANDLE);
        CHECK(moved.handle() == first);
        CHECK(overlay.removed.empty());

        cinder::gfx::OverlayTexture replacement(overlay, VK_NULL_HANDLE);
        second = replacement.handle();
        replacement = std::move(moved);
        REQUIRE(overlay.removed.size() == 1);
        CHECK(overlay.removed[0] == second);
        CHECK(moved.handle() == VK_NULL_HANDLE);
        CHECK(replacement.handle() == first);

        cinder::gfx::OverlayTexture empty;
        empty = std::move(texture);
    }
    REQUIRE(overlay.removed.size() == 2);
    CHECK(overlay.removed[1] == first);
}

TEST_CASE("overlay registrations are released when target setup unwinds") {
    Overlay overlay;
    VkDescriptorSet handle;
    try {
        cinder::gfx::OverlayTexture texture(overlay, VK_NULL_HANDLE);
        handle = texture.handle();
        throw std::runtime_error("later target setup failed");
    } catch (const std::runtime_error&) {
    }
    REQUIRE(overlay.removed.size() == 1);
    CHECK(overlay.removed[0] == handle);
}
