#include "nodes/MeshPart.hpp"

#include "scene/DrawList.hpp"

namespace cinder::nodes {

void MeshPart::onRender(float alpha, cinder::scene::DrawList& draws) {
    if (meshHandle_ < 0) meshHandle_ = draws.meshHandle(mesh_);
    if (textureHandle_ < 0) textureHandle_ = draws.textureHandle(texture_);
    draws.mesh(meshHandle_, textureHandle_, transform()->world());
}

}
