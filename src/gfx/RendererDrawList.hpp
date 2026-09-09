#pragma once

#include "scene/DrawList.hpp"

namespace cinder::gfx::pass {
class MeshPass;
class SpritePass;
}

namespace cinder::gfx {

class RendererDrawList : public cinder::scene::DrawList {
public:
    RendererDrawList(cinder::gfx::pass::MeshPass& mesh, cinder::gfx::pass::SpritePass& sprite)
        : mesh_(mesh), sprite_(sprite) {}

    void sprite(int texture, float x, float y, float w, float h, float rot,
                float r, float g, float b, float a) override;

    void spriteRegion(int texture, float x, float y, float w, float h,
                      float sx, float sy, float sw, float sh, float rot,
                      float r, float g, float b, float a) override;

    void mesh(int mesh, int texture, const glm::mat4& model) override;

    void camera3d(const glm::mat4& world, float fovDegrees, float near, float far) override;

    void camera2d(float x, float y, float zoom, float rotation) override;

private:
    cinder::gfx::pass::MeshPass& mesh_;
    cinder::gfx::pass::SpritePass& sprite_;
};

}
