#include <doctest/doctest.h>

#include "core/ProjectConfig.hpp"
#include "nodes/Builtins.hpp"
#include "physics/Nodes.hpp"
#include "scene/NodeTypes.hpp"
#include "scene/Scene.hpp"
#include "serial/SceneCodec.hpp"


#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using cinder::core::ProjectConfig;
using cinder::core::ProjectDescriptor;
using cinder::scene::NodeTypes;
using cinder::scene::Scene;
using cinder::serial::SceneCodec;

namespace {

const std::filesystem::path SOURCE(CINDER_SOURCE_DIR);

std::string read(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::vector<std::filesystem::path> shippedScenes() {
    std::vector<std::filesystem::path> out;
    for (const std::filesystem::path& root : {SOURCE / "Samples", SOURCE / "Tests" / "selftest"}) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
            if (entry.path().extension() == ".scene") out.push_back(entry.path());
        }
    }
    return out;
}

std::vector<std::filesystem::path> shippedDescriptors() {
    std::vector<std::filesystem::path> out;
    for (const std::filesystem::path& root : {SOURCE / "Samples", SOURCE / "Tests" / "selftest"}) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
            if (entry.path().extension() == ".cinder") out.push_back(entry.path());
        }
    }
    return out;
}

}

TEST_CASE("every shipped project descriptor and config is canonical") {
    const std::vector<std::filesystem::path> descriptors = shippedDescriptors();
    CHECK(descriptors.size() >= 3);

    for (const std::filesystem::path& file : descriptors) {
        const std::filesystem::path root = file.parent_path();
        const std::string name = root.lexically_relative(SOURCE).string();
        CAPTURE(name);

        const ProjectConfig config = ProjectConfig::load(root);
        CHECK(ProjectDescriptor::save(config.descriptor) == read(file));
        CHECK(ProjectConfig::save(config) == read(root / "Config" / "Game.ini"));
    }
}

TEST_CASE("every shipped scene is canonical") {
    NodeTypes types;
    cinder::nodes::registerBuiltins(types);
    cinder::physics::registerNodes(types);
    Scene scene{types};

    const std::vector<std::filesystem::path> scenes = shippedScenes();
    CHECK(scenes.size() >= 3);

    for (const std::filesystem::path& path : scenes) {
        const std::string name = path.lexically_relative(SOURCE).string();
        CAPTURE(name);
        const std::string text = read(path);
        SceneCodec::load(text, scene);
        CHECK(SceneCodec::save(scene) == text);
    }
}
