#pragma once

#include "Scene/SceneComponent.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <string>
#include <utility>

namespace cinder::Components {

class SpriteComponent final : public cinder::Scene::SceneComponent, public cinder::Reflect::PropSink {
public:
    const std::string& texture() const { return texture_; }
    const glm::vec2& size() const { return size_; }
    const glm::vec4& color() const { return color_; }

    SpriteComponent& setTexture(std::string path) {
        texture_ = std::move(path);
        textureHandle_ = -1;
        return *this;
    }
    SpriteComponent& setSize(float w, float h) { size_ = glm::vec2(w, h); return *this; }
    SpriteComponent& setColor(float r, float g, float b, float a) {
        color_ = glm::vec4(r, g, b, a);
        return *this;
    }

    void onRender(float alpha, cinder::Scene::DrawList& draws) override;

    void propChanged(const cinder::Reflect::PropDef& prop) override {
        if (prop.name() == "texture") textureHandle_ = -1;
    }

    CINDER_COMPONENT(SpriteComponent, cinder::Scene::SceneComponent) {
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
