#pragma once

#include "dev/History.hpp"
#include "dev/Selection.hpp"
#include "nodes/Builtins.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/Scene.hpp"

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
    cinder::scene::NodeTypes types;
    cinder::scene::Scene scene{types};

    SceneFixture() { cinder::nodes::registerBuiltins(types); }
};

struct EditorFixture : SceneFixture {
    cinder::dev::Selection selection;
    cinder::dev::History history{scene};
};

}
