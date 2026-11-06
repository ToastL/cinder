#pragma once

#include <filesystem>
#include <initializer_list>
#include <string>
#include <vector>

namespace cbt {

struct TargetInfo {
    std::string target;
    std::string platform;
    std::string config;
    std::string backend;
    std::string version;
    std::string abi;
    std::filesystem::path root;
    std::filesystem::path binaries;
    std::filesystem::path intermediate;
};

class ModuleRules {
public:
    ModuleRules(std::string name, std::filesystem::path directory);

    const std::string& name() const { return name_; }
    const std::filesystem::path& directory() const { return directory_; }

    ModuleRules& publicDependencies(std::initializer_list<std::string> names);
    ModuleRules& privateDependencies(std::initializer_list<std::string> names);
    ModuleRules& linkDependencies(std::initializer_list<std::string> names);
    ModuleRules& publicIncludes(std::initializer_list<std::string> paths);
    ModuleRules& privateIncludes(std::initializer_list<std::string> paths);
    ModuleRules& publicDefines(std::initializer_list<std::string> defines);
    ModuleRules& privateDefines(std::initializer_list<std::string> defines);
    ModuleRules& compileFlags(std::initializer_list<std::string> flags);
    ModuleRules& objcFlags(std::initializer_list<std::string> flags);
    ModuleRules& frameworks(std::initializer_list<std::string> names);
    ModuleRules& systemLibraries(std::initializer_list<std::string> names);
    ModuleRules& sources(std::initializer_list<std::string> paths);
    ModuleRules& exclude(std::initializer_list<std::string> paths);
    ModuleRules& headerOnly();
    ModuleRules& thirdParty();

    std::vector<std::string> publicDependencies_;
    std::vector<std::string> privateDependencies_;
    std::vector<std::string> linkDependencies_;
    std::vector<std::filesystem::path> publicIncludes_;
    std::vector<std::filesystem::path> privateIncludes_;
    std::vector<std::string> publicDefines_;
    std::vector<std::string> privateDefines_;
    std::vector<std::string> compileFlags_;
    std::vector<std::string> objcFlags_;
    std::vector<std::string> frameworks_;
    std::vector<std::string> systemLibraries_;
    std::vector<std::filesystem::path> sources_;
    std::vector<std::filesystem::path> excluded_;
    bool explicitSources_ = false;
    bool headerOnly_ = false;
    bool thirdParty_ = false;

private:
    std::filesystem::path resolve(const std::string& path) const;

    std::string name_;
    std::filesystem::path directory_;
};

enum class TargetType { Executable, GameModule };

class TargetRules {
public:
    TargetRules(std::string name, std::filesystem::path directory);

    const std::string& name() const { return name_; }
    const std::filesystem::path& directory() const { return directory_; }

    TargetRules& type(TargetType value);
    TargetRules& modules(std::initializer_list<std::string> names);
    TargetRules& defines(std::initializer_list<std::string> values);
    TargetRules& exportSymbols();
    TargetRules& outputName(std::string value);
    TargetRules& prerequisites(std::initializer_list<std::string> targets);

    TargetType type_ = TargetType::Executable;
    std::vector<std::string> modules_;
    std::vector<std::string> defines_;
    bool exportSymbols_ = false;
    std::string outputName_;
    std::vector<std::string> prerequisites_;

private:
    std::string name_;
    std::filesystem::path directory_;
};

using ModuleRule = void (*)(ModuleRules&, const TargetInfo&);
using TargetRule = void (*)(TargetRules&, const TargetInfo&);

struct ModuleRegistration {
    ModuleRegistration(const char* name, const char* file, ModuleRule rule);
};

struct TargetRegistration {
    TargetRegistration(const char* name, const char* file, TargetRule rule);
};

}

#define CINDER_RULES_CONCAT_(a, b) a##b
#define CINDER_RULES_CONCAT(a, b) CINDER_RULES_CONCAT_(a, b)

#define CINDER_MODULE(name, ...)                                                                  \
    static const ::cbt::ModuleRegistration CINDER_RULES_CONCAT(cbtModule, __LINE__)(name, __FILE__, \
                                                                                    __VA_ARGS__)

#define CINDER_TARGET(name, ...)                                                                  \
    static const ::cbt::TargetRegistration CINDER_RULES_CONCAT(cbtTarget, __LINE__)(name, __FILE__, \
                                                                                    __VA_ARGS__)
