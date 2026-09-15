#include <doctest/doctest.h>

#include "platform/Assets.hpp"
#include "platform/Log.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using cinder::platform::contentPath;
using cinder::platform::sourcePath;

namespace {

struct Project {
    std::filesystem::path root = std::filesystem::temp_directory_path() / "cinder_assets_test";

    Project() {
        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "Content" / "Textures");
        std::filesystem::create_directories(root / "Source");
        std::ofstream(root / "Content" / "Textures" / "Box.png") << "png";
        cinder::platform::setProjectRoot(root);
    }

    ~Project() { std::filesystem::remove_all(root); }
};

struct Sink {
    std::vector<std::string> lines;

    Sink() {
        cinder::platform::setLogSink([this](cinder::platform::LogLevel, std::string_view text) {
            lines.emplace_back(text);
        });
    }

    ~Sink() { cinder::platform::setLogSink(nullptr); }

    Sink(const Sink&) = delete;
    Sink& operator=(const Sink&) = delete;
};

}

TEST_CASE("content and source paths resolve under their own folders") {
    Project project;
    const std::filesystem::path box = project.root / "Content" / "Textures" / "Box.png";

    CHECK(contentPath("Textures/Box.png") == box);
    CHECK(contentPath("./Textures/Box.png") == box);
    CHECK(contentPath("Scenes/../Textures/Box.png") == box);
    CHECK(sourcePath("bobber.lua") == project.root / "Source" / "bobber.lua");
}

TEST_CASE("paths that leave their folder are rejected") {
    Project project;

    CHECK_THROWS_AS(contentPath("../secret.png"), std::runtime_error);
    CHECK_THROWS_AS(contentPath("Textures/../../secret.png"), std::runtime_error);
    CHECK_THROWS_AS(contentPath("/etc/hosts"), std::runtime_error);
    CHECK_THROWS_AS(sourcePath("../Content/Textures/Box.png"), std::runtime_error);
}

TEST_CASE("a path whose case differs from the disk warns") {
    Project project;
    Sink sink;

    contentPath("Textures/Box.png");
    CHECK(sink.lines.empty());

    const bool insensitive =
            std::filesystem::exists(project.root / "Content" / "textures" / "box.png");
    contentPath("textures/box.png");
    CHECK(sink.lines.size() == (insensitive ? 1u : 0u));
}
