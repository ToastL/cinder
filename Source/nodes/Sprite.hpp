#pragma once

#include "scene/Spatial.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <string>
#include <utility>

namespace cinder::components {

class Sprite final : public cinder::scene::Spatial, public cinder::reflect::PropSink {
public:
    const std::string& texture() const { return texture_; }
    const glm::vec2& size() const { return size_; }
    const glm::vec4& color() const { return color_; }

    Sprite& setTexture(std::string path) {
        texture_ = std::move(path);
        textureHandle_ = -1;
        return *this;
    }
    Sprite& setSize(float w, float h) { size_ = glm::vec2(w, h); return *this; }
    Sprite& setColor(float r, float g, float b, float a) {
        color_ = glm::vec4(r, g, b, a);
        return *this;
    }

    void onRender(float alpha, cinder::scene::DrawList& draws) override;

    void propChanged(const cinder::reflect::PropDef& prop) override {
        if (prop.name() == "texture") textureHandle_ = -1;
    }

    CINDER_NODE(Sprite, cinder::scene::Spatial) {
        CINDER_PROP(texture_);
        CINDER_PROP(size_);
        CINDER_PROP_COLOR(color_);
    }

private:
    std::string texture_;
    glm::vec2 size_{32.0f, 32.0f};
    glm::vec4 color_{1.0f, 1.0f, 1.0f, 1.0f};
    int textureHandle_ = -1;
};

}
