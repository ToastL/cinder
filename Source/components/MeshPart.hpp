#pragma once

#include "scene/Spatial.hpp"

#include <string>
#include <utility>

namespace cinder::components {

class MeshPart final : public cinder::scene::Spatial, public cinder::reflect::PropSink {
public:
    const std::string& mesh() const { return mesh_; }
    const std::string& texture() const { return texture_; }

    MeshPart& setMesh(std::string name) {
        mesh_ = std::move(name);
        meshHandle_ = -1;
        return *this;
    }
    MeshPart& setTexture(std::string path) {
        texture_ = std::move(path);
        textureHandle_ = -1;
        return *this;
    }

    void onRender(float alpha, cinder::scene::DrawList& draws) override;

    void propChanged(const cinder::reflect::PropDef& prop) override {
        if (prop.name() == "mesh") meshHandle_ = -1;
        if (prop.name() == "texture") textureHandle_ = -1;
    }

    CINDER_NODE(MeshPart, cinder::scene::Spatial) {
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
