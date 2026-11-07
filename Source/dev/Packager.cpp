#include "dev/Packager.hpp"

#include "dev/Process.hpp"
#include "platform/Assets.hpp"
#include "platform/Log.hpp"

#include <algorithm>
#include <system_error>
#include <utility>

namespace cinder::dev {
namespace {

std::string projectName(const std::filesystem::path& project) {
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(project, error)) {
        if (entry.path().extension() == ".cinder") return entry.path().stem().string();
    }
    return project.filename().string();
}

}

PackageTools PackageTools::fromBuild() { return PackageTools{cinder::platform::executableDir() / "packager"}; }

std::vector<std::string> packageArguments(const PackageTools& tools, Platform platform, Configuration configuration,
                                          const std::filesystem::path& project, const std::filesystem::path& output) {
    return {tools.packager.string(), project.string(), "--platform", std::string(platformName(platform)),
            "--config", std::string(configurationName(configuration)), "--out", output.string()};
}

Packager::Packager(std::filesystem::path project, std::vector<std::string> targetPlatforms, PackageTools tools)
    : project_(std::move(project)), targetPlatforms_(std::move(targetPlatforms)), tools_(std::move(tools)),
      name_(projectName(project_)) {}

Packager::~Packager() { job_.cancel(); }

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
    job_.join();

    const std::filesystem::path output = outputDir(platform);
    const std::vector<std::string> arguments = packageArguments(tools_, platform, configuration_, project_, output);
    cinder::platform::logInfo("[package] packaging %s for %s (%s) into %s\n", name_.c_str(),
                              std::string(platformName(platform)).c_str(),
                              std::string(configurationName(configuration_)).c_str(), output.string().c_str());

    std::string error;
    std::unique_ptr<Process> process = Process::start(arguments, error);
    if (process == nullptr) {
        cinder::platform::logError("[package] %s\n", error.c_str());
        return false;
    }

    current_ = platform;
    reported_ = false;
    cancelled_ = false;
    job_.start(std::move(process));
    return true;
}

void Packager::cancel() {
    if (!busy()) return;
    cancelled_ = true;
    job_.cancel();
}

void Packager::update() {
    for (const std::string& line : job_.takeLines()) cinder::platform::logInfo("[package] %s\n", line.c_str());
    if (!busy() && !reported_) finish();
}

void Packager::finish() {
    job_.join();
    reported_ = true;
    update();
    const std::string platform(platformName(current_.value_or(hostPlatform())));
    if (cancelled_) {
        cinder::platform::logInfo("[package] %s build cancelled\n", platform.c_str());
    } else if (job_.status() == 0) {
        cinder::platform::logInfo("[package] %s build ready in %s\n", platform.c_str(),
                                  outputDir(current_.value_or(hostPlatform())).string().c_str());
    } else {
        cinder::platform::logError("[package] %s build failed (exit %d)\n", platform.c_str(), job_.status());
    }
}

}
