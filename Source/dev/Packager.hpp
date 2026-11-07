#pragma once

#include "dev/BackgroundProcess.hpp"
#include "dev/Staging.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace cinder::dev {

struct PackageTools {
    std::filesystem::path packager;

    static PackageTools fromBuild();
};

std::vector<std::string> packageArguments(const PackageTools& tools, Platform platform, Configuration configuration,
                                          const std::filesystem::path& project, const std::filesystem::path& output);

class Packager {
public:
    Packager(std::filesystem::path project, std::vector<std::string> targetPlatforms,
             PackageTools tools = PackageTools::fromBuild());
    ~Packager();

    Packager(const Packager&) = delete;
    Packager& operator=(const Packager&) = delete;

    bool targets(Platform platform) const;
    bool canPackage(Platform platform) const;
    std::string unavailableReason(Platform platform) const;
    std::filesystem::path outputDir(Platform platform) const;

    Configuration configuration() const { return configuration_; }
    void setConfiguration(Configuration configuration) { configuration_ = configuration; }

    bool busy() const { return job_.running(); }
    std::optional<Platform> current() const { return busy() ? current_ : std::nullopt; }

    bool start(Platform platform);
    void cancel();
    void update();

private:
    void finish();

    std::filesystem::path project_;
    std::vector<std::string> targetPlatforms_;
    PackageTools tools_;
    std::string name_;
    Configuration configuration_ = Configuration::Development;
    BackgroundProcess job_;
    bool cancelled_ = false;
    std::optional<Platform> current_;
    bool reported_ = true;
};

}
