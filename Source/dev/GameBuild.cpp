#include "dev/GameBuild.hpp"

#include "platform/Assets.hpp"

namespace cinder::dev {
namespace {

std::string_view platformFolder() {
#if defined(__APPLE__)
    return "macOS";
#else
    return "Linux";
#endif
}

}

std::string_view GameBuild::defaultBackend() {
#if defined(CINDER_BACKEND_MTL)
    return "mtl";
#else
    return "vk";
#endif
}

std::optional<GameBuild> GameBuild::locate() {
    const std::filesystem::path folder = cinder::platform::executableDir();
    if (folder.empty()) return std::nullopt;
    const std::filesystem::path cbt = folder.parent_path() / "cbt";
    std::error_code error;
    if (!std::filesystem::is_regular_file(cbt, error)) return std::nullopt;
    return GameBuild{cbt, folder.parent_path().parent_path().parent_path(), folder.filename().string(),
                     std::string(defaultBackend())};
}

std::filesystem::path GameBuild::binaries() const { return output / "Binaries" / std::string(platformFolder()) / config; }

std::vector<std::string> GameBuild::game(const std::filesystem::path& project, const std::filesystem::path& library) const {
    std::vector<std::string> argv{cbt.string(), "game", project.string(), "--config", config, "--backend", backend,
                                  "--output", output.string()};
    if (!library.empty()) argv.insert(argv.end(), {"--library", library.string()});
    return argv;
}

std::vector<std::string> GameBuild::build(const std::string& target) const {
    return {cbt.string(), "build", target, "--config", config, "--backend", backend, "--output", output.string()};
}

}
