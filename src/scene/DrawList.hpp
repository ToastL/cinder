#pragma once

#include <glm/mat4x4.hpp>

namespace cinder::scene {

class DrawList {
public:
    static constexpr int WHITE = 0;

    virtual ~DrawList() = default;

    virtual void sprite(int texture, float x, float y, float w, float h, float rot,
                        float r, float g, float b, float a) = 0;

    virtual void spriteRegion(int texture, float x, float y, float w, float h,
                              float sx, float sy, float sw, float sh, float rot,
                              float r, float g, float b, float a) = 0;

    virtual void mesh(int mesh, int texture, const glm::mat4& model) = 0;

    virtual void camera3d(const glm::mat4& world, float fovDegrees, float near, float far) = 0;

    virtual void camera2d(float x, float y, float zoom, float rotation) = 0;
};

}
