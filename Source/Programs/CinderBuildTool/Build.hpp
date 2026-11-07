#pragma once

#include "CinderBuildTool/Executor.hpp"
#include "CinderBuildTool/Rules.hpp"
#include "CinderBuildTool/Toolchain.hpp"

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace cbt {

inline constexpr const char* ENGINE_VERSION = "0.1.0";

bool writeCompileCommands(const std::filesystem::path& file, const std::filesystem::path& directory,
                          const std::vector<Action>& actions);

struct BuildOptions {
    std::filesystem::path root;
    std::filesystem::path output;
    std::filesystem::path self;
    std::string config = "Development";
    std::string backend;
    std::vector<std::string> shaderFormats;
    int jobs = 0;
};

class Builder {
public:
    Builder(Toolchain toolchain, BuildOptions options);

    bool build(const std::vector<std::string>& targets);
    std::vector<Action> plan(const std::vector<std::string>& targets);
    bool clean();
    const std::filesystem::path& output() const { return options_.output; }
    const std::string& config() const { return options_.config; }
    const std::string& backend() const { return options_.backend; }
    const std::string& platform() const { return toolchain_.platform; }
    bool buildGame(const std::filesystem::path& project, std::filesystem::path library);
    TargetInfo info(const std::string& target) const;
    const std::string& id() const { return buildId(); }
    std::filesystem::path binaries() const;
    std::filesystem::path intermediate() const;
    const std::filesystem::path& root() const { return options_.root; }
    const std::vector<std::string>& shaderFormats() const { return options_.shaderFormats; }

private:
    struct Module {
        ModuleRules rules;
        bool firstParty = false;
    };

    struct Placement {
        std::filesystem::path objects;
        std::filesystem::path product;
        std::filesystem::path entry;
        std::string id;
    };

    void planTarget(const std::string& name, std::set<std::string>& planned, std::vector<Action>& actions);
    void planRules(const std::string& name, const TargetRules& target, const TargetInfo& targetInfo,
                   std::vector<Action>& actions, const Placement& placement);
    const Module& module(const std::string& name, const TargetInfo& info);
    std::vector<std::string> closure(const std::vector<std::string>& roots, const TargetInfo& info);
    void publicClosure(const std::string& name, const TargetInfo& info, std::set<std::string>& out);
    std::vector<std::filesystem::path> sourcesOf(const Module& module) const;
    std::vector<std::string> compileFlags(const std::string& name, const TargetInfo& info, bool positionIndependent,
                                          const std::vector<std::string>& extraDefines);
    void planShaders(std::vector<Action>& actions, std::vector<std::filesystem::path>& outputs);
    const std::string& buildId() const;
    std::string computeBuildId() const;
    TargetInfo info(const std::string& target, const std::string& id) const;
    std::vector<std::string> configFlags() const;

    Toolchain toolchain_;
    BuildOptions options_;
    mutable std::optional<std::string> buildId_;
    std::map<std::string, Module> modules_;
    std::set<std::string> shaderPlanned_;
};

}
