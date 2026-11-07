#include "Components/StaticMeshComponent.hpp"

#include "Scene/DrawList.hpp"

namespace cinder::Components {

void StaticMeshComponent::onRender(float alpha, cinder::Scene::DrawList& draws) {
    if (meshHandle_ < 0) meshHandle_ = draws.meshHandle(mesh_);
    if (textureHandle_ < 0) textureHandle_ = draws.textureHandle(texture_);
    draws.mesh(meshHandle_, textureHandle_, transform().world());
}

}
