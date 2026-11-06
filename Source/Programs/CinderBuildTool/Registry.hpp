#pragma once

#include "CinderBuildTool/Rules.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace cbt {

struct ModuleEntry {
    std::string name;
    std::filesystem::path file;
    ModuleRule rule;
};

struct TargetEntry {
    std::string name;
    std::filesystem::path file;
    TargetRule rule;
};

const std::vector<ModuleEntry>& registeredModules();
const std::vector<TargetEntry>& registeredTargets();
const ModuleEntry* findModule(const std::string& name);
void registerModule(const std::string& name, const std::filesystem::path& file, ModuleRule rule);
const TargetEntry* findTarget(const std::string& name);
ModuleRules evaluate(const ModuleEntry& entry, const TargetInfo& info);
TargetRules evaluate(const TargetEntry& entry, const TargetInfo& info);

}
