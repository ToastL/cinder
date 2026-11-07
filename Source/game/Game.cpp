#include "game/Game.hpp"

#include "core/Engine.hpp"
#include "gfx/Renderer.hpp"
#include "gfx/pass/MeshPass.hpp"
#include "gfx/pass/SpritePass.hpp"
#include "platform/Assets.hpp"
#include "platform/Input.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <random>
#include <stdexcept>
#include <string>

namespace cinder::game {
namespace {

glm::mat4 placed(glm::vec2 position, glm::vec2 size, float rotation) {
    const glm::mat4 centered = glm::translate(
            glm::mat4(1.0f), glm::vec3(position.x + size.x * 0.5f, position.y + size.y * 0.5f, 0.0f));
    return glm::rotate(centered, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
}

cinder::platform::Input& input() { return engine().input(); }

std::mt19937& generator() {
    static std::mt19937 engine{std::random_device{}()};
    return engine;
}

}

cinder::core::Engine& engine() {
    cinder::core::Engine* current = cinder::core::Engine::current();
    if (current == nullptr) throw std::logic_error("no engine is running");
    return *current;
}

cinder::scene::Scene& scene() { return engine().scene(); }

cinder::physics::World& world() { return engine().physics(); }

double time() { return scene().time(); }

void quit() { engine().quit(); }

bool keyDown(std::string_view key) { return input().keyDown(key); }

bool keyPressed(std::string_view key) { return input().keyPressed(key); }

bool keyReleased(std::string_view key) { return input().keyReleased(key); }

bool mouseDown(std::string_view button) { return input().mouseDown(button); }

bool mousePressed(std::string_view button) { return input().mousePressed(button); }

bool mouseReleased(std::string_view button) { return input().mouseReleased(button); }

glm::vec2 mousePosition() {
    return glm::vec2(static_cast<float>(input().mouseX()), static_cast<float>(input().mouseY()));
}

glm::vec2 mouseDelta() {
    return glm::vec2(static_cast<float>(input().mouseDeltaX()), static_cast<float>(input().mouseDeltaY()));
}

glm::vec2 scroll() {
    return glm::vec2(static_cast<float>(input().scrollX()), static_cast<float>(input().scrollY()));
}

void setCursorLocked(bool locked) { input().setCursorLocked(locked); }

bool cursorLocked() { return input().cursorLocked(); }

int loadTexture(std::string_view path) {
    return engine().renderer().assets().load(cinder::platform::contentPath(path).string());
}

glm::ivec2 textureSize(int texture) {
    const auto& found = engine().renderer().assets().get(texture);
    return glm::ivec2(static_cast<int>(found.width()), static_cast<int>(found.height()));
}

glm::vec2 screenSize() { return engine().renderer().camera().size(); }

glm::vec2 screenToWorld(glm::vec2 point) {
    const glm::vec3 world = engine().renderer().camera().screenToWorld(point.x, point.y);
    return glm::vec2(world.x, world.y);
}

void drawSprite(int texture, glm::vec2 position, glm::vec2 size, float rotation, glm::vec4 color) {
    engine().renderer().sprites().draw(texture, placed(position, size, rotation), size, color);
}

void drawSpriteRegion(int texture, glm::vec2 position, glm::vec2 size, glm::vec4 region, float rotation,
                      glm::vec4 color) {
    engine().renderer().sprites().drawRegion(texture, placed(position, size, rotation), size, region, color);
}

void drawRect(glm::vec2 position, glm::vec2 size, glm::vec4 color) {
    engine().renderer().sprites().draw(cinder::scene::DrawList::WHITE, placed(position, size, 0.0f), size, color);
}

int newCube(glm::vec3 color) { return engine().renderer().meshes().addCube(color.r, color.g, color.b); }

void drawMesh(int mesh, const glm::mat4& model, int texture) {
    engine().renderer().meshes().submit(mesh, texture, model);
}

std::optional<cinder::physics::RayHit> raycast(glm::vec3 origin, glm::vec3 direction, float maxDistance) {
    cinder::physics::RayHit hit;
    if (!world().raycast(origin, direction, maxDistance, hit)) return std::nullopt;
    return hit;
}

float random() { return std::uniform_real_distribution<float>(0.0f, 1.0f)(generator()); }

float random(float low, float high) { return std::uniform_real_distribution<float>(low, high)(generator()); }

}
