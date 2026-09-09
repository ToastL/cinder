#include "gfx/pass/MeshPass.hpp"

#include "gfx/pass/Overflow.hpp"
#include "gfx/vk/VkCtx.hpp"
#include "lua/LuaApi.hpp"
#include "scene/DrawList.hpp"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace cinder::gfx::pass {

using cinder::gfx::asset::Mesh;
using cinder::lua::LuaApi;

namespace {

int newCube(lua_State* state) {
    MeshPass& pass = *LuaApi::context<MeshPass>(state);
    lua_pushinteger(state, pass.addCube(LuaApi::optFloat(state, 1, 1.0f),
                                        LuaApi::optFloat(state, 2, 1.0f),
                                        LuaApi::optFloat(state, 3, 1.0f)));
    return 1;
}

int drawMesh(lua_State* state) {
    MeshPass& pass = *LuaApi::context<MeshPass>(state);

    const int mesh = static_cast<int>(lua_tointeger(state, 1));
    const float x = static_cast<float>(lua_tonumber(state, 2));
    const float y = static_cast<float>(lua_tonumber(state, 3));
    const float z = static_cast<float>(lua_tonumber(state, 4));
    const float sx = LuaApi::optFloat(state, 5, 1.0f);
    const float sy = LuaApi::optFloat(state, 6, 1.0f);
    const float sz = LuaApi::optFloat(state, 7, 1.0f);
    const float rx = LuaApi::optFloat(state, 8, 0.0f);
    const float ry = LuaApi::optFloat(state, 9, 0.0f);
    const float rz = LuaApi::optFloat(state, 10, 0.0f);
    const int texture = LuaApi::optInt(state, 11, cinder::scene::DrawList::WHITE);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
    model = glm::rotate(model, ry, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, rx, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, rz, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(sx, sy, sz));

    pass.submit(mesh, texture, model);
    return 0;
}

}

MeshPass::MeshPass(const cinder::gfx::vk::VkCtx& ctx, cinder::gfx::asset::Assets& assets,
                   const MeshPipeline& pipeline)
    : ctx_(ctx), assets_(assets), pipeline_(pipeline),
      drawMesh_(MAX_DRAWS), drawTexture_(MAX_DRAWS), drawModel_(MAX_DRAWS, glm::mat4(1.0f)) {
    meshes_.push_back(Mesh::cube(ctx, 1.0f, 1.0f, 1.0f));
}

void MeshPass::beginFrame() { drawCount_ = 0; }

bool MeshPass::accept(int mesh) {
    if (mesh < 0 || static_cast<std::size_t>(mesh) >= meshes_.size()) return false;
    if (drawCount_ >= MAX_DRAWS) {
        overflowWarned_ = warnOverflow(overflowWarned_, "mesh queue", "4096");
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

void MeshPass::record(VkCommandBuffer cmd, uint32_t frameInFlight) {
    if (drawCount_ == 0) return;

    pipeline_.bind(cmd, camera_.viewProjection());

    int boundMesh = -1;
    int boundTexture = -1;

    for (uint32_t i = 0; i < drawCount_; ++i) {
        const int mesh = drawMesh_[i];
        if (mesh != boundMesh) {
            meshes_[static_cast<std::size_t>(mesh)].bind(cmd);
            boundMesh = mesh;
        }
        if (drawTexture_[i] != boundTexture) {
            const VkDescriptorSet set = assets_.get(drawTexture_[i]).descriptorSet();
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.layout(),
                                    0, 1, &set, 0, nullptr);
            boundTexture = drawTexture_[i];
        }
        pipeline_.pushModel(cmd, drawModel_[i]);
        vkCmdDrawIndexed(cmd, meshes_[static_cast<std::size_t>(mesh)].indexCount(), 1, 0, 0, 0);
    }
}

void MeshPass::registerApi(LuaApi& api) {
    api.bind("newCube", newCube, this);
    api.bind("drawMesh", drawMesh, this);
}

void MeshPass::resize(int width, int height) {
    camera_.setAspect(static_cast<float>(width) / static_cast<float>(std::max(1, height)));
}

}
