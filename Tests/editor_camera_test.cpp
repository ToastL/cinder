#include <doctest/doctest.h>

#include "Dev/EditorCamera.hpp"
#include "Gfx/Pass/ViewCamera.hpp"
#include "Components/Actors.hpp"
#include "Components/Builtins.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/Scene.hpp"
#include "Scene/Transform.hpp"

#include <glm/trigonometric.hpp>
#include <glm/vec4.hpp>

#include <array>
#include <cmath>
#include <cstddef>

using Camera = cinder::Components::CameraComponent;
using cinder::Dev::EditorCamera;
using cinder::Gfx::Pass::ViewCamera;
using cinder::Scene::ClassRegistry;
using cinder::Scene::Scene;

namespace {

const glm::vec2 PANEL(400.0f, 300.0f);

struct World {
    ClassRegistry types;
    Scene scene{types};
    Camera* camera = nullptr;

    World() {
        cinder::Components::registerBuiltins(types);
        camera = &scene.spawn<cinder::Components::CameraActor>()->camera();
        camera->transform().setPosition(10.0f, 20.0f, 30.0f);
        camera->transform().setRotation(glm::radians(-20.0f), glm::radians(35.0f), 0.0f);
    }

    std::array<glm::vec3, 8> corners() {
        ViewCamera seen;
        seen.setView(camera->view());
        seen.setViewSize(PANEL.x, PANEL.y);
        return seen.corners();
    }
};

glm::vec3 ndc(ViewCamera& camera, const glm::vec3& point) {
    const glm::vec4 clip = camera.viewProjection() * glm::vec4(point, 1.0f);
    return glm::vec3(clip) / clip.w;
}

}

TEST_CASE("an editor camera seeded from a perspective camera sees exactly its frame") {
    World world;
    EditorCamera editor;
    editor.seed(world.camera, PANEL);

    const std::array<glm::vec3, 8> corners = world.corners();
    for (std::size_t i = 0; i < corners.size(); ++i) {
        const glm::vec3 p = ndc(editor.camera(), corners[i]);
        CHECK(std::abs(p.x) == doctest::Approx(1.0f).epsilon(1e-3));
        CHECK(std::abs(p.y) == doctest::Approx(1.0f).epsilon(1e-3));
        CHECK(p.z == doctest::Approx(i < 4 ? 0.0f : 1.0f).epsilon(1e-3));
    }
}

TEST_CASE("an editor camera seeded from an orthographic camera frames its rectangle and sees past it") {
    World world;
    world.camera->setProjection(Camera::Projection::Orthographic).setVirtualSize(PANEL.x, PANEL.y).setZoom(2.0f);

    EditorCamera editor;
    editor.seed(world.camera, PANEL);
    CHECK_FALSE(editor.view().orthographic);

    const std::array<glm::vec3, 8> corners = world.corners();
    const glm::vec3 eye(editor.view().world[3]);
    const glm::vec3 forward = -glm::vec3(editor.view().world[2]);
    for (std::size_t i = 0; i < 4; ++i) {
        const glm::vec3 p = ndc(editor.camera(), corners[i]);
        CHECK(std::abs(p.x) == doctest::Approx(1.0f).epsilon(1e-3));
        CHECK(std::abs(p.y) == doctest::Approx(1.0f).epsilon(1e-3));
        CHECK(p.z > 0.0f);

        CHECK(glm::dot(corners[i + 4] - eye, forward) < editor.view().farClip);
    }
}
