#pragma once

#include "Scene/Transform.hpp"

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace cinder::Reflect { class PropDef; }

namespace cinder::Scene {
class Node;
class NodeTypes;
class Scene;
}

namespace cinder::Serial {

class Archive;

class SceneCodec {
public:
    static constexpr int VERSION = 4;
    static constexpr int OLDEST = 4;

    static std::string save(cinder::Scene::Scene& scene);
    static void load(const std::string& source, cinder::Scene::Scene& scene);
    static void saveToFile(const std::filesystem::path& path, cinder::Scene::Scene& scene);
    static void loadFromFile(const std::filesystem::path& path, cinder::Scene::Scene& scene);

private:
    explicit SceneCodec(cinder::Scene::Scene& scene);

    void walk(Archive& ar);
    void nodes(Archive& ar, const std::vector<cinder::Scene::Node*>& list, cinder::Scene::Node* parent);
    void node(Archive& ar, cinder::Scene::Node* existing, std::string_view type,
              cinder::Scene::Node* parent);
    cinder::Scene::Node* insert(std::unique_ptr<cinder::Scene::Node> node, int id, cinder::Scene::Node* parent);
    void emit(Archive& ar, const std::vector<cinder::Reflect::PropDef>& defs,
              void* target, const void* base);

    cinder::Scene::Scene& scene_;
    cinder::Scene::NodeTypes& types_;
    cinder::Scene::Transform blank_{nullptr};
    float values_[4]{};
    float fallback_[4]{};
    std::set<std::string, std::less<>> reported_;
};

}
