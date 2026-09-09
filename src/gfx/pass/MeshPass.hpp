#pragma once

#include "gfx/asset/Assets.hpp"
#include "gfx/asset/Mesh.hpp"
#include "gfx/pass/DrawPass.hpp"
#include "gfx/pass/MeshPipeline.hpp"
#include "gfx/pass/PerspectiveCamera.hpp"

#include <vector>

namespace cinder::gfx::pass {

class MeshPass : public DrawPass {
public:
    static constexpr uint32_t MAX_DRAWS = 4096;

    MeshPass(const cinder::gfx::vk::VkCtx& ctx, cinder::gfx::asset::Assets& assets,
             const MeshPipeline& pipeline);

    void beginFrame() override;
    void record(VkCommandBuffer cmd, uint32_t frameInFlight) override;
    void registerApi(cinder::lua::LuaApi& api) override;
    void resize(int width, int height) override;

    void submit(int mesh, int texture, const glm::mat4& model);
    int addCube(float r, float g, float b);

    PerspectiveCamera& camera() { return camera_; }

private:
    bool accept(int mesh);

    const cinder::gfx::vk::VkCtx& ctx_;
    cinder::gfx::asset::Assets& assets_;
    const MeshPipeline& pipeline_;

    std::vector<cinder::gfx::asset::Mesh> meshes_;
    std::vector<int> drawMesh_;
    std::vector<int> drawTexture_;
    std::vector<glm::mat4> drawModel_;
    uint32_t drawCount_ = 0;
    bool overflowWarned_ = false;

    PerspectiveCamera camera_;
};

}
