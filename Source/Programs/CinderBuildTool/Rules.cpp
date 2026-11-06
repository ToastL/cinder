#include "CinderBuildTool/Registry.hpp"

#include <algorithm>
#include <stdexcept>

namespace cbt {
namespace {

std::vector<ModuleEntry>& modules() {
    static std::vector<ModuleEntry> list;
    return list;
}

std::vector<TargetEntry>& targets() {
    static std::vector<TargetEntry> list;
    return list;
}

template <class T>
void append(std::vector<T>& out, std::initializer_list<T> values) {
    out.insert(out.end(), values.begin(), values.end());
}

}

ModuleRules::ModuleRules(std::string name, std::filesystem::path directory)
    : name_(std::move(name)), directory_(std::move(directory)) {}

std::filesystem::path ModuleRules::resolve(const std::string& path) const {
    const std::filesystem::path value(path);
    return value.is_absolute() ? value : directory_ / value;
}

ModuleRules& ModuleRules::publicDependencies(std::initializer_list<std::string> names) {
    append(publicDependencies_, names);
    return *this;
}

ModuleRules& ModuleRules::privateDependencies(std::initializer_list<std::string> names) {
    append(privateDependencies_, names);
    return *this;
}

ModuleRules& ModuleRules::linkDependencies(std::initializer_list<std::string> names) {
    append(linkDependencies_, names);
    return *this;
}

ModuleRules& ModuleRules::publicIncludes(std::initializer_list<std::string> paths) {
    for (const std::string& path : paths) publicIncludes_.push_back(resolve(path));
    return *this;
}

ModuleRules& ModuleRules::privateIncludes(std::initializer_list<std::string> paths) {
    for (const std::string& path : paths) privateIncludes_.push_back(resolve(path));
    return *this;
}

ModuleRules& ModuleRules::publicDefines(std::initializer_list<std::string> defines) {
    append(publicDefines_, defines);
    return *this;
}

ModuleRules& ModuleRules::privateDefines(std::initializer_list<std::string> defines) {
    append(privateDefines_, defines);
    return *this;
}

ModuleRules& ModuleRules::compileFlags(std::initializer_list<std::string> flags) {
    append(compileFlags_, flags);
    return *this;
}

ModuleRules& ModuleRules::objcFlags(std::initializer_list<std::string> flags) {
    append(objcFlags_, flags);
    return *this;
}

ModuleRules& ModuleRules::frameworks(std::initializer_list<std::string> names) {
    append(frameworks_, names);
    return *this;
}

ModuleRules& ModuleRules::systemLibraries(std::initializer_list<std::string> names) {
    append(systemLibraries_, names);
    return *this;
}

ModuleRules& ModuleRules::sources(std::initializer_list<std::string> paths) {
    explicitSources_ = true;
    for (const std::string& path : paths) sources_.push_back(resolve(path));
    return *this;
}

ModuleRules& ModuleRules::exclude(std::initializer_list<std::string> paths) {
    for (const std::string& path : paths) excluded_.push_back(resolve(path));
    return *this;
}

ModuleRules& ModuleRules::headerOnly() {
    headerOnly_ = true;
    return *this;
}

ModuleRules& ModuleRules::thirdParty() {
    thirdParty_ = true;
    return *this;
}

TargetRules::TargetRules(std::string name, std::filesystem::path directory)
    : name_(std::move(name)), directory_(std::move(directory)) {}

TargetRules& TargetRules::type(TargetType value) {
    type_ = value;
    return *this;
}

TargetRules& TargetRules::modules(std::initializer_list<std::string> names) {
    append(modules_, names);
    return *this;
}

TargetRules& TargetRules::defines(std::initializer_list<std::string> values) {
    append(defines_, values);
    return *this;
}

TargetRules& TargetRules::exportSymbols() {
    exportSymbols_ = true;
    return *this;
}

TargetRules& TargetRules::prerequisites(std::initializer_list<std::string> targets) {
    append(prerequisites_, targets);
    return *this;
}

TargetRules& TargetRules::outputName(std::string value) {
    outputName_ = std::move(value);
    return *this;
}

ModuleRegistration::ModuleRegistration(const char* name, const char* file, ModuleRule rule) {
    modules().push_back(ModuleEntry{name, std::filesystem::path(file), rule});
}

TargetRegistration::TargetRegistration(const char* name, const char* file, TargetRule rule) {
    targets().push_back(TargetEntry{name, std::filesystem::path(file), rule});
}

void registerModule(const std::string& name, const std::filesystem::path& file, ModuleRule rule) {
    modules().push_back(ModuleEntry{name, file, rule});
}

const std::vector<ModuleEntry>& registeredModules() { return modules(); }

const std::vector<TargetEntry>& registeredTargets() { return targets(); }

const ModuleEntry* findModule(const std::string& name) {
    const auto& list = modules();
    auto found = std::find_if(list.begin(), list.end(), [&](const ModuleEntry& entry) { return entry.name == name; });
    return found == list.end() ? nullptr : &*found;
}

const TargetEntry* findTarget(const std::string& name) {
    const auto& list = targets();
    auto found = std::find_if(list.begin(), list.end(), [&](const TargetEntry& entry) { return entry.name == name; });
    return found == list.end() ? nullptr : &*found;
}

ModuleRules evaluate(const ModuleEntry& entry, const TargetInfo& info) {
    ModuleRules rules(entry.name, entry.file.parent_path());
    entry.rule(rules, info);
    return rules;
}

TargetRules evaluate(const TargetEntry& entry, const TargetInfo& info) {
    TargetRules rules(entry.name, entry.file.parent_path());
    entry.rule(rules, info);
    return rules;
}

}
