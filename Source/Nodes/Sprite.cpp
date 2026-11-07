#include "Nodes/Sprite.hpp"

#include "Scene/DrawList.hpp"

namespace cinder::Nodes {

void Sprite::onRender(float alpha, cinder::Scene::DrawList& draws) {
    if (textureHandle_ < 0) textureHandle_ = draws.textureHandle(texture_);
    draws.sprite(textureHandle_, transform()->world(), size_, color_);
}

}
