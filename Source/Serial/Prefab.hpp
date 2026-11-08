#pragma once

#include <string>
#include <string_view>

namespace cinder::Scene {
class Actor;
class Object;
class Scene;
}

namespace cinder::Serial {

class Prefab {
public:
    static constexpr std::string_view EXTENSION = ".prefab";

    static cinder::Scene::Actor* instantiate(cinder::Scene::Scene& scene, std::string_view path,
                                             cinder::Scene::Actor* parent = nullptr);
    static const cinder::Scene::Actor* rootOf(const cinder::Scene::Object& object);
    static const cinder::Scene::Object* baseline(const cinder::Scene::Scene& scene,
                                                 const cinder::Scene::Object& object);
    static bool missing(const cinder::Scene::Scene& scene, const cinder::Scene::Actor& root);

    static std::string refusal(cinder::Scene::Scene& scene, const cinder::Scene::Actor& actor,
                               std::string_view path);
    static bool write(cinder::Scene::Scene& scene, cinder::Scene::Actor& actor, std::string_view path);
    static bool revert(cinder::Scene::Scene& scene, cinder::Scene::Actor& root);
};

}
