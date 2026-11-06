#pragma once

#include "components/Camera.hpp"
#include "components/Folder.hpp"
#include "components/Group.hpp"
#include "components/MeshPart.hpp"
#include "components/Sprite.hpp"
#include "core/Module.hpp"
#include "physics/Body.hpp"
#include "physics/Collider.hpp"
#include "physics/World.hpp"
#include "platform/Log.hpp"
#include "scene/DrawList.hpp"
#include "scene/Node.hpp"
#include "scene/Scene.hpp"
#include "scene/Spatial.hpp"
#include "scene/Transform.hpp"

#include <glm/glm.hpp>

#include <optional>
#include <string_view>

namespace cinder::core { class Engine; }

namespace cinder::game {

cinder::core::Engine& engine();
cinder::scene::Scene& scene();
cinder::physics::World& world();

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
void drawMesh(int mesh, const glm::mat4& model, int texture = cinder::scene::DrawList::WHITE);

std::optional<cinder::physics::RayHit> raycast(glm::vec3 origin, glm::vec3 direction, float maxDistance = 1000.0f);

float random();
float random(float low, float high);

}
