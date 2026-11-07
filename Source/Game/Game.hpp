#pragma once

#include "Core/Module.hpp"
#include "Components/Actors.hpp"
#include "Components/CameraComponent.hpp"
#include "Components/SpriteComponent.hpp"
#include "Components/StaticMeshComponent.hpp"
#include "Physics/ColliderComponent.hpp"
#include "Physics/RigidBodyComponent.hpp"
#include "Physics/World.hpp"
#include "Platform/Log.hpp"
#include "Scene/Actor.hpp"
#include "Scene/Component.hpp"
#include "Scene/DrawList.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneComponent.hpp"
#include "Scene/Transform.hpp"

#include <glm/glm.hpp>

#include <optional>
#include <string_view>

namespace cinder::Core { class Engine; }

namespace cinder::Game {

cinder::Core::Engine& engine();
cinder::Scene::Scene& scene();
cinder::Physics::World& world();

double time();
void quit();

bool keyDown(std::string_view key);
bool keyPressed(std::string_view key);
bool keyReleased(std::string_view key);
bool mouseDown(std::string_view button);
bool mousePressed(std::string_view button);
bool mouseReleased(std::string_view button);
glm::vec2 mousePosition();
glm::vec2 mouseDelta();
glm::vec2 scroll();
void setCursorLocked(bool locked);
bool cursorLocked();

int loadTexture(std::string_view path);
glm::ivec2 textureSize(int texture);

glm::vec2 screenSize();
glm::vec2 screenToWorld(glm::vec2 point);
void drawSprite(int texture, glm::vec2 position, glm::vec2 size, float rotation = 0.0f,
                glm::vec4 color = glm::vec4(1.0f));
void drawSpriteRegion(int texture, glm::vec2 position, glm::vec2 size, glm::vec4 region,
                      float rotation = 0.0f, glm::vec4 color = glm::vec4(1.0f));
void drawRect(glm::vec2 position, glm::vec2 size, glm::vec4 color = glm::vec4(1.0f));
int newCube(glm::vec3 color = glm::vec3(1.0f));
void drawMesh(int mesh, const glm::mat4& model, int texture = cinder::Scene::DrawList::WHITE);

std::optional<cinder::Physics::RayHit> raycast(glm::vec3 origin, glm::vec3 direction, float maxDistance = 1000.0f);

float random();
float random(float low, float high);

}
