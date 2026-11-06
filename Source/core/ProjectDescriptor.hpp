#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace cinder::core {

struct ProjectDescriptor {
    static constexpr int FILE_VERSION = 1;

    struct Module {
        std::string name;
        std::string type = "Runtime";
        std::string loadingPhase = "Default";
        std::vector<std::string> additionalDependencies;
    };

    struct Plugin {
        std::string name;
        bool enabled = true;
        bool optional = false;
        std::string description;
        std::vector<std::string> platformAllowList;
        std::vector<std::string> platformDenyList;
        std::vector<std::string> targetAllowList;
        std::vector<std::string> targetDenyList;
    };

    using BuildSteps = std::map<std::string, std::vector<std::string>>;

    std::string engineAssociation;
    std::string category;
    std::string description;
    bool disableEnginePluginsByDefault = false;
    std::vector<Module> modules;
    std::vector<Plugin> plugins;
    std::vector<std::string> additionalRootDirectories;
    std::vector<std::string> additionalPluginDirectories;
    std::vector<std::string> targetPlatforms;
    BuildSteps preBuildSteps;
    BuildSteps postBuildSteps;

    static std::string_view engineVersion();
    static ProjectDescriptor parse(std::string_view json);
    static std::string save(const ProjectDescriptor& descriptor);
};

}
