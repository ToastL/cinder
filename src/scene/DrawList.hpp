#pragma once

#include "scene/View.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <string_view>

namespace cinder::scene {

class DrawList {
public:
    static constexpr int WHITE = 0;

    virtual ~DrawList() = default;

    virtual int textureHandle(std::string_view path) = 0;
    virtual int meshHandle(std::string_view name) = 0;

    virtual void background(float r, float g, float b) = 0;

    virtual void sprite(int texture, const glm::mat4& model, glm::vec2 size, const glm::vec4& color) = 0;

    virtual void mesh(int mesh, int texture, const glm::mat4& model) = 0;

    virtual void camera(const View& view) = 0;
};

}
