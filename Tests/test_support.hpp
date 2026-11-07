#pragma once

#include "Dev/History.hpp"
#include "Dev/Selection.hpp"
#include "Nodes/Builtins.hpp"
#include "Scene/NodeTypes.hpp"
#include "Scene/Scene.hpp"

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
    cinder::Scene::NodeTypes types;
    cinder::Scene::Scene scene{types};

    SceneFixture() { cinder::Nodes::registerBuiltins(types); }
};

struct EditorFixture : SceneFixture {
    cinder::Dev::Selection selection;
    cinder::Dev::History history{scene};
};

}
