#include <doctest/doctest.h>

#include "gfx/pass/ViewCamera.hpp"
#include "scene/View.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>

#include <array>
#include <cmath>
#include <cstddef>

using cinder::gfx::pass::ViewCamera;
using cinder::scene::View;

namespace {

ViewCamera aimed(bool orthographic) {
    glm::mat4 world = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 2.0f, 10.0f));
    world = glm::rotate(world, 0.6f, glm::vec3(0.0f, 1.0f, 0.0f));
    world = glm::rotate(world, -0.3f, glm::vec3(1.0f, 0.0f, 0.0f));

    View view;
    view.world = world;
    view.orthographic = orthographic;
    view.fovDegrees = 70.0f;
    view.nearClip = 0.5f;
    view.farClip = 40.0f;
    view.zoom = 2.0f;
    view.size = glm::vec2(64.0f, 36.0f);

    ViewCamera camera;
    camera.setView(view);
    camera.setViewSize(1600.0f, 900.0f);
    return camera;
}

glm::vec3 ndc(ViewCamera& camera, const glm::vec3& point) {
    const glm::vec4 clip = camera.viewProjection() * glm::vec4(point, 1.0f);
    return glm::vec3(clip) / clip.w;
}

}

TEST_CASE("frustum corners land on the corners of the clip volume") {
    for (const bool orthographic : {false, true}) {
        ViewCamera camera = aimed(orthographic);
        const std::array<glm::vec3, 8> corners = camera.corners();

        for (std::size_t i = 0; i < corners.size(); ++i) {
            const glm::vec3 p = ndc(camera, corners[i]);
            CHECK(std::abs(p.x) == doctest::Approx(1.0f).epsilon(1e-4));
            CHECK(std::abs(p.y) == doctest::Approx(1.0f).epsilon(1e-4));
            CHECK(p.z == doctest::Approx(i < 4 ? 0.0f : 1.0f).epsilon(1e-4));
        }
    }
}

TEST_CASE("frustum edges join each near corner to its far corner and its neighbours") {
    for (const bool orthographic : {false, true}) {
        ViewCamera camera = aimed(orthographic);
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
}

TEST_CASE("an orthographic view's near and far rectangles are the same size") {
    ViewCamera camera = aimed(true);
    const std::array<glm::vec3, 8> corners = camera.corners();
    CHECK(glm::distance(corners[0], corners[1]) == doctest::Approx(32.0f));
    CHECK(glm::distance(corners[4], corners[5]) == doctest::Approx(32.0f));
    CHECK(glm::distance(corners[1], corners[2]) == doctest::Approx(18.0f));
}

TEST_CASE("an orthographic view covers its size over its zoom, or the view itself without a size") {
    ViewCamera camera = aimed(true);
    CHECK(camera.extent().x == doctest::Approx(32.0f));
    CHECK(camera.extent().y == doctest::Approx(18.0f));

    View view = camera.view();
    view.size = glm::vec2(0.0f);
    camera.setView(view);
    CHECK(camera.size().x == doctest::Approx(1600.0f));
    CHECK(camera.extent().x == doctest::Approx(800.0f));
    CHECK(camera.extent().y == doctest::Approx(450.0f));
}

TEST_CASE("up in the world is up on the screen through either projection") {
    for (const bool orthographic : {false, true}) {
        View view;
        view.orthographic = orthographic;
        ViewCamera camera;
        camera.setView(view);
        camera.setViewSize(200.0f, 100.0f);

        CHECK(ndc(camera, glm::vec3(0.0f, 1.0f, -5.0f)).y < 0.0f);
        CHECK(ndc(camera, glm::vec3(1.0f, 0.0f, -5.0f)).x > 0.0f);
    }
}

TEST_CASE("a screen point lands where its ray meets the z = 0 plane") {
    View view;
    view.world = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 7.0f, 10.0f));
    view.orthographic = true;
    ViewCamera camera;
    camera.setView(view);
    camera.setViewSize(200.0f, 100.0f);

    const glm::vec3 centre = camera.screenToWorld(100.0f, 50.0f);
    CHECK(centre.x == doctest::Approx(5.0f));
    CHECK(centre.y == doctest::Approx(7.0f));
    CHECK(centre.z == doctest::Approx(0.0f).epsilon(1e-3));

    const glm::vec3 topLeft = camera.screenToWorld(0.0f, 0.0f);
    CHECK(topLeft.x == doctest::Approx(-95.0f));
    CHECK(topLeft.y == doctest::Approx(57.0f));

    view.orthographic = false;
    camera.setView(view);
    const glm::vec3 ahead = camera.screenToWorld(100.0f, 50.0f);
    CHECK(ahead.x == doctest::Approx(5.0f).epsilon(1e-3));
    CHECK(ahead.y == doctest::Approx(7.0f).epsilon(1e-3));
    CHECK(ahead.z == doctest::Approx(0.0f).epsilon(1e-3));

    const glm::vec3 left = camera.screenToWorld(0.0f, 50.0f);
    CHECK(left.z == doctest::Approx(0.0f).epsilon(1e-3));
    CHECK(ndc(camera, left).x == doctest::Approx(-1.0f).epsilon(1e-3));
}
