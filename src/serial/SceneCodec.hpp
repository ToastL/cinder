#pragma once

#include "scene/Transform.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace cinder::reflect { class PropDef; }

namespace cinder::scene {
class Node;
class NodeTypes;
class Scene;
}

namespace cinder::serial {

class Archive;

class SceneCodec {
public:
    static constexpr int VERSION = 4;
    static constexpr int OLDEST = 4;

    static std::string save(cinder::scene::Scene& scene);
    static void load(const std::string& source, cinder::scene::Scene& scene);
    static void saveToFile(const std::filesystem::path& path, cinder::scene::Scene& scene);
    static void loadFromFile(const std::filesystem::path& path, cinder::scene::Scene& scene);

private:
    explicit SceneCodec(cinder::scene::Scene& scene);

    void walk(Archive& ar);
    void nodes(Archive& ar, const std::vector<cinder::scene::Node*>& list, cinder::scene::Node* parent);
    void node(Archive& ar, cinder::scene::Node* existing, std::string_view type,
              cinder::scene::Node* parent);
    cinder::scene::Node* insert(std::string_view type, int id, cinder::scene::Node* parent);
    void emit(Archive& ar, const std::vector<cinder::reflect::PropDef>& defs,
              void* target, const void* base);

    cinder::scene::Scene& scene_;
    cinder::scene::NodeTypes& types_;
    cinder::scene::Transform blank_{nullptr};
    float values_[4]{};
    float fallback_[4]{};
};

}
