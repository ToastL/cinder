#include <doctest/doctest.h>

#include "gfx/pass/PerspectiveCamera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include <array>
#include <cmath>
#include <cstddef>

using cinder::gfx::pass::PerspectiveCamera;

namespace {

PerspectiveCamera aimed() {
    glm::mat4 world = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 2.0f, 10.0f));
    world = glm::rotate(world, 0.6f, glm::vec3(0.0f, 1.0f, 0.0f));
    world = glm::rotate(world, -0.3f, glm::vec3(1.0f, 0.0f, 0.0f));

    PerspectiveCamera camera;
    camera.setWorld(world);
    camera.setFov(glm::radians(70.0f));
    camera.setAspect(16.0f / 9.0f);
    camera.setClip(0.5f, 40.0f);
    return camera;
}

glm::vec3 ndc(PerspectiveCamera& camera, const glm::vec3& point) {
    const glm::vec4 clip = camera.viewProjection() * glm::vec4(point, 1.0f);
    return glm::vec3(clip) / clip.w;
}

}

TEST_CASE("frustum corners land on the corners of the clip volume") {
    PerspectiveCamera camera = aimed();
    const std::array<glm::vec3, 8> corners = camera.corners();

    for (std::size_t i = 0; i < corners.size(); ++i) {
        const glm::vec3 p = ndc(camera, corners[i]);
        CHECK(std::abs(p.x) == doctest::Approx(1.0f).epsilon(1e-4));
        CHECK(std::abs(p.y) == doctest::Approx(1.0f).epsilon(1e-4));
        CHECK(p.z == doctest::Approx(i < 4 ? 0.0f : 1.0f).epsilon(1e-4));
    }
}

TEST_CASE("frustum edges join each near corner to its far corner and its neighbours") {
    PerspectiveCamera camera = aimed();
    const std::array<glm::vec3, 8> corners = camera.corners();

    for (std::size_t i = 0; i < 4; ++i) {
        const glm::vec3 front = ndc(camera, corners[i]);
        const glm::vec3 back = ndc(camera, corners[i + 4]);
        CHECK(back.x == doctest::Approx(front.x).epsilon(1e-4));
        CHECK(back.y == doctest::Approx(front.y).epsilon(1e-4));

        const glm::vec3 next = ndc(camera, corners[(i + 1) % 4]);
        const bool sameX = (front.x > 0.0f) == (next.x > 0.0f);
        const bool sameY = (front.y > 0.0f) == (next.y > 0.0f);
        CHECK(sameX != sameY);
    }
}
