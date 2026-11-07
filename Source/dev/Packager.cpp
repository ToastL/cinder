#include "dev/Packager.hpp"

#include "dev/Process.hpp"

#include "platform/Assets.hpp"
#include "platform/Log.hpp"

#include <algorithm>
#include <array>
#include <csignal>
#include <cstdio>
#include <system_error>
#include <utility>

#include <sys/wait.h>
#include <unistd.h>

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

Packager::~Packager() {
    cancel();
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
    cancelled_.store(false);
    pid_ = process->pid();
    running_.store(true);
    worker_ = std::thread([this, process = std::move(process)]() mutable {
        std::string line;
        while (process->readLine(line)) {
            const std::lock_guard lock(mutex_);
            lines_.push_back(line);
        }
        {
            const std::lock_guard lock(mutex_);
            pid_ = 0;
        }
        status_ = process->wait();
        running_.store(false);
    });
    return true;
}

void Packager::cancel() {
    const std::lock_guard lock(mutex_);
    if (pid_ <= 0) return;
    cancelled_.store(true);
    kill(-pid_, SIGTERM);
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
    if (cancelled_.load()) {
        cinder::platform::logInfo("[package] %s build cancelled\n", platform.c_str());
    } else if (status_ == 0) {
        cinder::platform::logInfo("[package] %s build ready in %s\n", platform.c_str(),
                                  outputDir(current_.value_or(hostPlatform())).string().c_str());
    } else {
        cinder::platform::logError("[package] %s build failed (exit %d)\n", platform.c_str(), status_);
    }
}

}
