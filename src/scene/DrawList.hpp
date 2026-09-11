#pragma once

#include <glm/mat4x4.hpp>

#include <string_view>

namespace cinder::scene {

class DrawList {
public:
    static constexpr int WHITE = 0;

    virtual ~DrawList() = default;

    virtual int textureHandle(std::string_view path) = 0;
    virtual int meshHandle(std::string_view name) = 0;

    virtual void background(float r, float g, float b) = 0;

    virtual void sprite(int texture, float x, float y, float w, float h, float rot,
                        float r, float g, float b, float a) = 0;

    virtual void spriteRegion(int texture, float x, float y, float w, float h,
                              float sx, float sy, float sw, float sh, float rot,
                              float r, float g, float b, float a) = 0;

    virtual void mesh(int mesh, int texture, const glm::mat4& model) = 0;

    virtual void camera3d(const glm::mat4& world, float fovDegrees, float near, float far) = 0;

    virtual void camera2d(float x, float y, float zoom, float rotation,
                          float virtualWidth, float virtualHeight) = 0;
};

}
