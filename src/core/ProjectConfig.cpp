#include "core/ProjectConfig.hpp"

#include "core/GameLoop.hpp"
#include "platform/Log.hpp"
#include "serial/IniLoad.hpp"
#include "serial/IniSave.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

namespace cinder::core {
namespace {

std::string readFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot read " + path.string());
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::filesystem::path marker(const std::filesystem::path& folder) {
    std::vector<std::filesystem::path> found;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(folder, error)) {
        if (entry.path().extension() == ".cinder" && entry.is_regular_file(error)) {
            found.push_back(entry.path());
        }
    }

    if (found.size() == 1) return found.front();
    if (found.empty()) {
        throw std::runtime_error(folder.string() + " is not a project: it has no .cinder file");
    }

    std::sort(found.begin(), found.end());
    std::string names;
    for (const std::filesystem::path& path : found) {
        if (!names.empty()) names += ", ";
        names += path.filename().string();
    }
    throw std::runtime_error(folder.string() + " has more than one .cinder file: " + names);
}

}

ProjectConfig::ProjectConfig(std::string title, int width, int height, std::string startScene,
                             int fixedHz)
    : title(std::move(title)), width(width), height(height), startScene(std::move(startScene)),
      fixedHz(fixedHz) {
    GameLoop::requirePositiveHz(fixedHz, "fixedHz");
}

std::filesystem::path ProjectConfig::root(const std::filesystem::path& argument) {
    std::error_code error;
    if (std::filesystem::is_directory(argument, error)) return std::filesystem::absolute(argument);
    if (argument.extension() == ".cinder" && std::filesystem::is_regular_file(argument, error)) {
        return std::filesystem::absolute(argument).parent_path();
    }
    throw std::runtime_error(argument.string() + " is not a project folder or a .cinder file");
}

ProjectConfig ProjectConfig::load(const std::filesystem::path& folder) {
    const std::filesystem::path file = marker(folder);
    const std::string json = readFile(file);

    ProjectConfig config;
    try {
        config.descriptor = ProjectDescriptor::parse(json);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(file.filename().string() + ": " + e.what());
    }

    const std::string& made = config.descriptor.engineAssociation;
    const std::string_view engine = ProjectDescriptor::engineVersion();
    if (!made.empty() && made != engine) {
        cinder::platform::logError("[project] %s was made with cinder %s; this is cinder %.*s\n",
                                   file.filename().string().c_str(), made.c_str(),
                                   static_cast<int>(engine.size()), engine.data());
    }

    const std::filesystem::path settings = folder / "Config" / "Game.ini";
    std::error_code error;
    if (std::filesystem::exists(settings, error)) {
        cinder::serial::IniLoad archive = cinder::serial::IniLoad::parse(readFile(settings));
        config.walk(archive);
    }
    GameLoop::requirePositiveHz(config.fixedHz, "[Game] fixedHz");
    return config;
}

std::string ProjectConfig::save(const ProjectConfig& config) {
    cinder::serial::IniSave archive;
    ProjectConfig copy = config;
    copy.walk(archive);
    return archive.text();
}

void ProjectConfig::walk(cinder::serial::Archive& ar) {
    const ProjectConfig defaults;

    if (ar.enterRecord("Game")) {
        title = ar.text("title", title, defaults.title);
        startScene = ar.text("startScene", startScene, defaults.startScene);
        fixedHz = ar.integer("fixedHz", fixedHz, defaults.fixedHz);
        ar.leaveRecord();
    }

    if (ar.enterRecord("Window")) {
        width = ar.integer("width", width, defaults.width);
        height = ar.integer("height", height, defaults.height);
        ar.leaveRecord();
    }
}

}
