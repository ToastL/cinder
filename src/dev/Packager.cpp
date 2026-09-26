#include "dev/Packager.hpp"

#include "platform/Log.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <system_error>
#include <utility>

#include <sys/wait.h>

namespace cinder::dev {

namespace {

constexpr std::array<Platform, 5> PLATFORMS{Platform::MacOS, Platform::Linux, Platform::Windows, Platform::IOS,
                                            Platform::Android};

std::string quote(const std::filesystem::path& path) {
    std::string out = "'";
    for (const char c : path.string()) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out += c;
        }
    }
    return out + "'";
}

std::string define(std::string_view name, const std::filesystem::path& value) {
    return " " + quote("-D" + std::string(name) + "=" + value.string());
}

std::string projectName(const std::filesystem::path& project) {
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(project, error)) {
        if (entry.path().extension() == ".cinder") return entry.path().stem().string();
    }
    return project.filename().string();
}

}

std::span<const Platform> platforms() { return PLATFORMS; }

std::string_view platformName(Platform platform) {
    switch (platform) {
        case Platform::MacOS: return "macOS";
        case Platform::Linux: return "Linux";
        case Platform::Windows: return "Windows";
        case Platform::IOS: return "iOS";
        case Platform::Android: return "Android";
    }
    return "";
}

Platform hostPlatform() {
#if defined(__APPLE__)
    return Platform::MacOS;
#elif defined(_WIN32)
    return Platform::Windows;
#else
    return Platform::Linux;
#endif
}

bool platformSupported(Platform platform) {
    return platform == hostPlatform() && (platform == Platform::MacOS || platform == Platform::Linux);
}

PackageTools PackageTools::fromBuild() {
    return PackageTools{CINDER_CMAKE_COMMAND, CINDER_BUILD_DIR, CINDER_PLAYER_PATH, CINDER_PACKAGE_SCRIPT,
                        CINDER_ENGINE_DIR,    CINDER_SHADER_DIR};
}

std::string packageCommand(const PackageTools& tools, Platform platform, const std::filesystem::path& project,
                           const std::filesystem::path& output) {
    const std::string cmake = quote(tools.cmake);
    return "(" + cmake + " --build " + quote(tools.buildDir) + " --target player && " + cmake
            + define("PLAYER", tools.player) + define("HOST_PLATFORM", std::string(platformName(platform)))
            + define("ENGINE_DIR", tools.engineDir) + define("SHADER_DIR", tools.shaderDir)
            + define("PROJECT_DIR", project) + define("OUT_DIR", output) + " -P " + quote(tools.script) + ") 2>&1";
}

Packager::Packager(std::filesystem::path project, std::vector<std::string> targetPlatforms, PackageTools tools)
    : project_(std::move(project)), targetPlatforms_(std::move(targetPlatforms)), tools_(std::move(tools)),
      name_(projectName(project_)) {}

Packager::~Packager() {
    if (worker_.joinable()) worker_.join();
}

bool Packager::targets(Platform platform) const {
    return targetPlatforms_.empty()
            || std::find(targetPlatforms_.begin(), targetPlatforms_.end(), platformName(platform))
                    != targetPlatforms_.end();
}

bool Packager::canPackage(Platform platform) const { return platformSupported(platform) && targets(platform); }

std::string Packager::unavailableReason(Platform platform) const {
    const std::string name(platformName(platform));
    if (!platformSupported(platform)) {
        if (platform == Platform::MacOS || platform == Platform::Linux || platform == Platform::Windows) {
            return name + " builds must be packaged on " + name;
        }
        return name + " is not supported yet";
    }
    if (!targets(platform)) return "The project's targetPlatforms does not list " + name;
    return {};
}

std::filesystem::path Packager::outputDir(Platform platform) const {
    return project_ / "Saved" / "Builds" / std::string(platformName(platform)) / name_;
}

bool Packager::start(Platform platform) {
    if (busy()) return false;
    if (!canPackage(platform)) {
        cinder::platform::logError("[package] %s\n", unavailableReason(platform).c_str());
        return false;
    }
    if (worker_.joinable()) worker_.join();

    const std::filesystem::path output = outputDir(platform);
    const std::string command = packageCommand(tools_, platform, project_, output);
    cinder::platform::logInfo("[package] packaging %s for %s into %s\n", name_.c_str(),
                              std::string(platformName(platform)).c_str(), output.string().c_str());

    current_ = platform;
    reported_ = false;
    running_.store(true);
    worker_ = std::thread([this, command] {
        int status = -1;
        if (FILE* pipe = popen(command.c_str(), "r")) {
            std::array<char, 512> buffer{};
            std::string line;
            while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
                line += buffer.data();
                if (line.empty() || line.back() != '\n') continue;
                line.pop_back();
                const std::lock_guard lock(mutex_);
                lines_.push_back(std::move(line));
                line.clear();
            }
            if (!line.empty()) {
                const std::lock_guard lock(mutex_);
                lines_.push_back(std::move(line));
            }
            const int wait = pclose(pipe);
            status = WIFEXITED(wait) ? WEXITSTATUS(wait) : -1;
        }
        status_ = status;
        running_.store(false);
    });
    return true;
}

void Packager::update() {
    std::deque<std::string> lines;
    {
        const std::lock_guard lock(mutex_);
        lines.swap(lines_);
    }
    for (const std::string& line : lines) cinder::platform::logInfo("[package] %s\n", line.c_str());
    if (!busy() && !reported_) finish();
}

void Packager::finish() {
    if (worker_.joinable()) worker_.join();
    reported_ = true;
    update();
    const std::string platform(platformName(current_.value_or(hostPlatform())));
    if (status_ == 0) {
        cinder::platform::logInfo("[package] %s build ready in %s\n", platform.c_str(),
                                  outputDir(current_.value_or(hostPlatform())).string().c_str());
    } else {
        cinder::platform::logError("[package] %s build failed (exit %d)\n", platform.c_str(), status_);
    }
}

}
