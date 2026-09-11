#include <doctest/doctest.h>

#include "components/Builtins.hpp"
#include "scene/Components.hpp"
#include "scene/Scene.hpp"
#include "script/Behaviour.hpp"
#include "serial/SceneCodec.hpp"

#include <lua.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using cinder::scene::Components;
using cinder::scene::Scene;
using cinder::script::Behaviour;
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
    for (const std::filesystem::path& root : {SOURCE / "samples", SOURCE / "tests" / "selftest"}) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
            if (entry.path().extension() == ".scene") out.push_back(entry.path());
        }
    }
    return out;
}

}

TEST_CASE("every shipped scene is canonical") {
    lua_State* state = luaL_newstate();
    {
        Components types;
        cinder::components::registerBuiltins(types);
        types.add<Behaviour>("Behaviour", [state] { return std::make_unique<Behaviour>(state); });
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
    lua_close(state);
}
