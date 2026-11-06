#include "components/Sprite.hpp"

#include "scene/DrawList.hpp"

namespace cinder::components {

void Sprite::onRender(float alpha, cinder::scene::DrawList& draws) {
    if (textureHandle_ < 0) textureHandle_ = draws.textureHandle(texture_);
    draws.sprite(textureHandle_, transform()->world(), size_, color_);
}

}
