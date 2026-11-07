#include "Gfx/Pass/MeshPass.hpp"

#include "Gfx/RHI/Commands.hpp"

#include "Gfx/Pass/Overflow.hpp"
#include "Gfx/RHI/Backend.hpp"

namespace cinder::Gfx::Pass {

using cinder::Gfx::Asset::Mesh;

MeshPass::MeshPass(const cinder::Gfx::RHI::Ctx& ctx, cinder::Gfx::Asset::Assets& assets,
                   const MeshPipeline& pipeline)
    : ctx_(ctx), assets_(assets), pipeline_(pipeline),
      drawMesh_(MAX_DRAWS), drawTexture_(MAX_DRAWS), drawModel_(MAX_DRAWS, glm::mat4(1.0f)) {
    meshes_.push_back(Mesh::cube(ctx, 1.0f, 1.0f, 1.0f));
    named_.emplace("cube", 0);
    meshes_.push_back(Mesh::sphere(ctx, 1.0f, 1.0f, 1.0f));
    named_.emplace("sphere", 1);
    meshes_.push_back(Mesh::capsule(ctx, 1.0f, 1.0f, 1.0f));
    named_.emplace("capsule", 2);
}

int MeshPass::meshNamed(std::string_view name) const {
    auto found = named_.find(std::string(name));
    return found == named_.end() ? -1 : found->second;
}

void MeshPass::beginFrame() { drawCount_ = 0; }

bool MeshPass::accept(int mesh) {
    if (mesh < 0 || static_cast<std::size_t>(mesh) >= meshes_.size()) return false;
    if (drawCount_ >= MAX_DRAWS) {
        overflowWarned_ = warnOverflow(overflowWarned_, "mesh queue", MAX_DRAWS, "draws");
        return false;
    }
    return true;
}

void MeshPass::submit(int mesh, int texture, const glm::mat4& model) {
    if (!accept(mesh)) return;
    drawMesh_[drawCount_] = mesh;
    drawTexture_[drawCount_] = texture;
    drawModel_[drawCount_] = model;
    drawCount_++;
}

int MeshPass::addCube(float r, float g, float b) {
    meshes_.push_back(Mesh::cube(ctx_, r, g, b));
    return static_cast<int>(meshes_.size()) - 1;
}

void MeshPass::record(cinder::Gfx::RHI::Commands cmd, uint32_t frameInFlight,
                      const glm::mat4& viewProjection) {
    if (drawCount_ == 0) return;

    pipeline_.bind(cmd, viewProjection);

    int boundMesh = -1;
    int boundTexture = -1;

    for (uint32_t i = 0; i < drawCount_; ++i) {
        const int mesh = drawMesh_[i];
        if (mesh != boundMesh) {
            meshes_[static_cast<std::size_t>(mesh)].bind(cmd);
            boundMesh = mesh;
        }
        if (drawTexture_[i] != boundTexture) {
            pipeline_.bindTexture(cmd, assets_.get(drawTexture_[i]).binding());
            boundTexture = drawTexture_[i];
        }
        pipeline_.pushModel(cmd, drawModel_[i]);
        cinder::Gfx::RHI::drawIndexed(cmd, meshes_[static_cast<std::size_t>(mesh)].indexCount(), 0);
    }
}

}
