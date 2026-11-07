#include "CinderBuildTool/Paths.hpp"

#include <cstdlib>
#include <sstream>
#include <system_error>

namespace cbt {

bool isRuleFile(const std::filesystem::path& path) {
    const std::string name = path.filename().string();
    return name.ends_with(".Build.cpp") || name.ends_with(".Target.cpp");
}

std::string findTool(const std::string& name, const char* overrideVariable) {
    if (overrideVariable != nullptr) {
        if (const char* chosen = std::getenv(overrideVariable); chosen != nullptr && *chosen != '\0') return chosen;
    }
    std::error_code error;
    if (const char* sdk = std::getenv("VULKAN_SDK"); sdk != nullptr) {
        const std::filesystem::path candidate = std::filesystem::path(sdk) / "bin" / name;
        if (std::filesystem::is_regular_file(candidate, error)) return candidate.string();
    }
    const char* path = std::getenv("PATH");
    std::stringstream folders(path != nullptr ? path : "");
    std::string folder;
    while (std::getline(folders, folder, ':')) {
        const std::filesystem::path candidate = std::filesystem::path(folder) / name;
        if (!folder.empty() && std::filesystem::is_regular_file(candidate, error)) return candidate.string();
    }
    return {};
}

}
