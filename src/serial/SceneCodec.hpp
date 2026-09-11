#pragma once

#include "scene/Transform.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace cinder::reflect { class PropDef; }

namespace cinder::scene {
class Actor;
class Component;
class Components;
class Scene;
}

namespace cinder::serial {

class Archive;

class SceneCodec {
public:
    static constexpr int VERSION = 2;
    static constexpr int OLDEST = 2;

    static std::string save(cinder::scene::Scene& scene);
    static void load(const std::string& source, cinder::scene::Scene& scene);
    static void saveToFile(const std::filesystem::path& path, cinder::scene::Scene& scene);
    static void loadFromFile(const std::filesystem::path& path, cinder::scene::Scene& scene);

private:
    explicit SceneCodec(cinder::scene::Scene& scene);

    void walk(Archive& ar, int version);
    void actors(Archive& ar, const std::vector<cinder::scene::Actor*>& list,
                cinder::scene::Actor* parent, int version);
    void actor(Archive& ar, cinder::scene::Actor* actor, cinder::scene::Actor* parent, int version);
    void components(Archive& ar, cinder::scene::Actor& actor, int version);
    void component(Archive& ar, cinder::scene::Actor& actor, cinder::scene::Component* existing,
                   std::string_view type, int version);

    cinder::scene::Actor* spawn(int id, const std::string& name, cinder::scene::Actor* parent);
    void emit(Archive& ar, const std::vector<cinder::reflect::PropDef>& defs,
              void* target, const void* base);

    cinder::scene::Scene& scene_;
    cinder::scene::Components& types_;
    cinder::scene::Transform blank_{nullptr};
    float values_[4]{};
    float fallback_[4]{};
};

}
