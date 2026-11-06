#pragma once

#include "scene/DrawList.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace cinder::gfx::pass {
class MeshPass;
class SpritePass;
}

namespace cinder::gfx {

class Renderer;

class RendererDrawList : public cinder::scene::DrawList {
public:
    RendererDrawList(Renderer& renderer, cinder::gfx::pass::MeshPass& mesh,
                     cinder::gfx::pass::SpritePass& sprite)
        : renderer_(renderer), mesh_(mesh), sprite_(sprite) {}

    int textureHandle(std::string_view path) override;
    int meshHandle(std::string_view name) override;

    void background(float r, float g, float b) override;

    void sprite(int texture, const glm::mat4& model, glm::vec2 size, const glm::vec4& color) override;

    void mesh(int mesh, int texture, const glm::mat4& model) override;

    void camera(const cinder::scene::View& view) override;

private:
    Renderer& renderer_;
    cinder::gfx::pass::MeshPass& mesh_;
    cinder::gfx::pass::SpritePass& sprite_;
    std::unordered_map<std::string, int> textures_;
    std::unordered_set<std::string> unknownMeshes_;
};

}
