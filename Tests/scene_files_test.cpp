#include <doctest/doctest.h>

#include "Core/ProjectConfig.hpp"
#include "Components/Builtins.hpp"
#include "Physics/Components.hpp"
#include "Platform/Assets.hpp"
#include "Scene/ClassRegistry.hpp"
#include "Scene/PrefabCache.hpp"
#include "Scene/Scene.hpp"
#include "Serial/SceneCodec.hpp"


#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using cinder::Core::ProjectConfig;
using cinder::Core::ProjectDescriptor;
using cinder::Scene::ClassRegistry;
using cinder::Scene::Scene;
using cinder::Serial::SceneCodec;

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
        for (auto it = std::filesystem::recursive_directory_iterator(root); it != std::filesystem::recursive_directory_iterator(); ++it) {
            if (it->is_directory() && it->path().filename() == "Saved") it.disable_recursion_pending();
            else if (it->path().extension() == ".scene" || it->path().extension() == ".prefab") out.push_back(it->path());
        }
    }
    return out;
}

std::vector<std::filesystem::path> shippedDescriptors() {
    std::vector<std::filesystem::path> out;
    for (const std::filesystem::path& root : {SOURCE / "Samples", SOURCE / "Tests" / "selftest"}) {
        for (auto it = std::filesystem::recursive_directory_iterator(root); it != std::filesystem::recursive_directory_iterator(); ++it) {
            if (it->is_directory() && it->path().filename() == "Saved") it.disable_recursion_pending();
            else if (it->path().extension() == ".cinder") out.push_back(it->path());
        }
    }
    return out;
}

std::filesystem::path projectOf(const std::filesystem::path& file) {
    for (std::filesystem::path folder = file.parent_path(); !folder.empty() && folder != folder.parent_path();
         folder = folder.parent_path()) {
        for (const auto& entry : std::filesystem::directory_iterator(folder)) {
            if (entry.path().extension() == ".cinder") return folder;
        }
    }
    return file.parent_path();
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

TEST_CASE("every shipped scene and prefab is canonical") {
    ClassRegistry types;
    cinder::Components::registerBuiltins(types);
    cinder::Physics::registerComponents(types);
    Scene scene{types};

    const std::vector<std::filesystem::path> scenes = shippedScenes();
    CHECK(scenes.size() >= 3);

    for (const std::filesystem::path& path : scenes) {
        const std::string name = path.lexically_relative(SOURCE).string();
        CAPTURE(name);
        const std::string text = read(path);
        cinder::Platform::setProjectRoot(projectOf(path));
        types.prefabs().clear();
        SceneCodec::load(text, scene);
        CHECK(SceneCodec::save(scene) == text);
    }
}
