#pragma once

#include "scene/Component.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace cinder::components {

class SpriteRenderer final : public cinder::scene::Component {
public:
    int texture() const { return texture_; }
    const glm::vec2& size() const { return size_; }
    const glm::vec4& color() const { return color_; }

    SpriteRenderer& setTexture(int texture) { texture_ = texture; return *this; }
    SpriteRenderer& setSize(float w, float h) { size_ = glm::vec2(w, h); return *this; }
    SpriteRenderer& setColor(float r, float g, float b, float a) {
        color_ = glm::vec4(r, g, b, a);
        return *this;
    }

    void onRender(float alpha, cinder::scene::DrawList& draws) override;

    CINDER_COMPONENT(SpriteRenderer, cinder::scene::Component) {
        CINDER_PROP(texture_);
        CINDER_PROP(size_);
        CINDER_PROP(color_);
    }

private:
    int texture_ = 0;
    glm::vec2 size_{32.0f, 32.0f};
    glm::vec4 color_{1.0f, 1.0f, 1.0f, 1.0f};
};

}
