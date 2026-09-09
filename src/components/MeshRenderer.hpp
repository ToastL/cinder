#pragma once

#include "scene/Component.hpp"

namespace cinder::components {

class MeshRenderer final : public cinder::scene::Component {
public:
    int mesh() const { return mesh_; }
    int texture() const { return texture_; }

    MeshRenderer& setMesh(int mesh) { mesh_ = mesh; return *this; }
    MeshRenderer& setTexture(int texture) { texture_ = texture; return *this; }

    void onRender(float alpha, cinder::scene::DrawList& draws) override;

    CINDER_COMPONENT(MeshRenderer, cinder::scene::Component) {
        CINDER_PROP(mesh_);
        CINDER_PROP(texture_);
    }

private:
    int mesh_ = 0;
    int texture_ = 0;
};

}
