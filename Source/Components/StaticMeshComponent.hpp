#pragma once

#include "Scene/SceneComponent.hpp"

#include <string>
#include <utility>

namespace cinder::Components {

class StaticMeshComponent final : public cinder::Scene::SceneComponent, public cinder::Reflect::PropSink {
public:
    const std::string& mesh() const { return mesh_; }
    const std::string& texture() const { return texture_; }

    StaticMeshComponent& setMesh(std::string name) {
        mesh_ = std::move(name);
        meshHandle_ = -1;
        return *this;
    }
    StaticMeshComponent& setTexture(std::string path) {
        texture_ = std::move(path);
        textureHandle_ = -1;
        return *this;
    }

    void onRender(float alpha, cinder::Scene::DrawList& draws) override;

    void propChanged(const cinder::Reflect::PropDef& prop) override {
        if (prop.name() == "mesh") meshHandle_ = -1;
        if (prop.name() == "texture") textureHandle_ = -1;
    }

    CINDER_COMPONENT(StaticMeshComponent, cinder::Scene::SceneComponent) {
        CINDER_PROP(mesh_);
        CINDER_PROP(texture_);
    }

private:
    std::string mesh_ = "cube";
    std::string texture_;
    int meshHandle_ = -1;
    int textureHandle_ = -1;
};

}
