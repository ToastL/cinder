#include "gfx/RendererDrawList.hpp"

#include "gfx/pass/MeshPass.hpp"
#include "gfx/pass/SpritePass.hpp"

#include <glm/trigonometric.hpp>

namespace cinder::gfx {

void RendererDrawList::sprite(int texture, float x, float y, float w, float h, float rot,
                              float r, float g, float b, float a) {
    sprite_.draw(texture, x, y, w, h, rot, r, g, b, a);
}

void RendererDrawList::spriteRegion(int texture, float x, float y, float w, float h,
                                    float sx, float sy, float sw, float sh, float rot,
                                    float r, float g, float b, float a) {
    sprite_.drawRegion(texture, x, y, w, h, sx, sy, sw, sh, rot, r, g, b, a);
}

void RendererDrawList::mesh(int mesh, int texture, const glm::mat4& model) {
    mesh_.submit(mesh, texture, model);
}

void RendererDrawList::camera3d(const glm::mat4& world, float fovDegrees, float near, float far) {
    cinder::gfx::pass::PerspectiveCamera& camera = mesh_.camera();
    camera.setWorld(world);
    camera.setFov(glm::radians(fovDegrees));
    camera.setClip(near, far);
}

void RendererDrawList::camera2d(float x, float y, float zoom, float rotation) {
    cinder::gfx::pass::OrthographicCamera& camera = sprite_.camera();
    camera.setPosition(x, y);
    camera.setZoom(zoom);
    camera.setRotation(rotation);
}

}
