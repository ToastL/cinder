#pragma once

#include "Dev/History.hpp"
#include "Dev/Selection.hpp"
#include "Components/Builtins.hpp"
#include "Physics/Components.hpp"
#include "Scene/Actor.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneComponent.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

#include <unistd.h>

namespace testing {

inline std::filesystem::path uniqueTempDir(std::string_view prefix) {
    std::string pattern = (std::filesystem::temp_directory_path() / (std::string(prefix) + "-XXXXXX")).string();
    if (mkdtemp(pattern.data()) == nullptr) throw std::runtime_error("could not create a temporary folder");
    return std::filesystem::path(pattern);
}

struct SceneFixture {
    cinder::Scene::ClassRegistry types;
    cinder::Scene::Scene scene{types};

    SceneFixture() {
        cinder::Components::registerBuiltins(types);
        cinder::Physics::registerComponents(types);
    }

    cinder::Scene::Actor* group(cinder::Scene::Actor* parent = nullptr) {
        cinder::Scene::Actor* actor = scene.spawn<cinder::Scene::Actor>(parent);
        actor->addComponent<cinder::Scene::SceneComponent>();
        return actor;
    }
};

struct EditorFixture : SceneFixture {
    cinder::Dev::Selection selection;
    cinder::Dev::History history{scene};
};

}
