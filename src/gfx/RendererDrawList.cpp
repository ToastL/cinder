#include "gfx/RendererDrawList.hpp"

#include "gfx/Renderer.hpp"
#include "gfx/pass/MeshPass.hpp"
#include "gfx/pass/SpritePass.hpp"
#include "platform/Assets.hpp"
#include "platform/Log.hpp"

#include <exception>
#include <glm/trigonometric.hpp>

namespace cinder::gfx {

int RendererDrawList::textureHandle(std::string_view path) {
    if (path.empty()) return WHITE;

    const std::string key = cinder::platform::resolveAsset(path).string();
    if (missingTextures_.count(key) != 0) return WHITE;

    try {
        return renderer_.assets().load(key);
    } catch (const std::exception& e) {
        missingTextures_.insert(key);
        cinder::platform::logError("[gfx] %s\n", e.what());
        return WHITE;
    }
}

int RendererDrawList::meshHandle(std::string_view name) {
    if (name.empty()) name = "cube";

    const int mesh = mesh_.meshNamed(name);
    if (mesh >= 0) return mesh;

    if (unknownMeshes_.insert(std::string(name)).second) {
        cinder::platform::logError("[gfx] unknown mesh \"%.*s\", drawing a cube\n",
                                   static_cast<int>(name.size()), name.data());
    }
    return mesh_.meshNamed("cube");
}

void RendererDrawList::background(float r, float g, float b) { renderer_.setClearColor(r, g, b); }

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

void RendererDrawList::camera2d(float x, float y, float zoom, float rotation,
                                float virtualWidth, float virtualHeight) {
    sprite_.setVirtualSize(virtualWidth, virtualHeight);
    cinder::gfx::pass::OrthographicCamera& camera = sprite_.camera();
    camera.setPosition(x, y);
    camera.setZoom(zoom);
    camera.setRotation(rotation);
}

}
