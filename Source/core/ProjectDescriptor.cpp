#include "core/ProjectDescriptor.hpp"

#include "serial/Json.hpp"

#include <cstdint>
#include <stdexcept>
#include <utility>

#ifndef CINDER_VERSION
#define CINDER_VERSION "0.0.0"
#endif

namespace cinder::core {
namespace {

using cinder::scene::PropRec;
using cinder::scene::PropSeq;
using cinder::scene::PropValue;
using cinder::serial::JsonWriter;

[[noreturn]] void mustBe(std::string_view label, const char* type) {
    throw std::runtime_error("\"" + std::string(label) + "\" must be " + type);
}

const PropValue* member(const PropRec& object, const char* key) {
    const auto found = object.find(key);
    return found == object.end() ? nullptr : &found->second;
}

std::string textOf(const PropValue* value, std::string_view label, std::string fallback) {
    if (value == nullptr) return fallback;
    if (!value->is<std::string>()) mustBe(label, "a string");
    return value->as<std::string>();
}

bool flagOf(const PropValue* value, std::string_view label, bool fallback) {
    if (value == nullptr) return fallback;
    if (!value->is<bool>()) mustBe(label, "true or false");
    return value->as<bool>();
}

std::vector<std::string> textsOf(const PropValue* value, std::string_view label) {
    std::vector<std::string> out;
    if (value == nullptr) return out;
    if (!value->is<PropSeq>()) mustBe(label, "a list of strings");
    for (const PropValue& item : value->as<PropSeq>()) {
        if (!item.is<std::string>()) mustBe(label, "a list of strings");
        out.push_back(item.as<std::string>());
    }
    return out;
}

std::vector<const PropRec*> objectsOf(const PropValue* value, std::string_view label) {
    std::vector<const PropRec*> out;
    if (value == nullptr) return out;
    if (!value->is<PropSeq>()) mustBe(label, "a list of objects");
    for (const PropValue& item : value->as<PropSeq>()) {
        if (!item.is<PropRec>()) mustBe(label, "a list of objects");
        out.push_back(&item.as<PropRec>());
    }
    return out;
}

ProjectDescriptor::BuildSteps stepsOf(const PropValue* value, std::string_view label) {
    ProjectDescriptor::BuildSteps out;
    if (value == nullptr) return out;
    if (!value->is<PropRec>()) mustBe(label, "an object of platform lists");
    for (const auto& [platform, commands] : value->as<PropRec>()) {
        out.emplace(platform, textsOf(&commands, std::string(label) + "." + platform));
    }
    return out;
}

std::string nameOf(const PropRec& entry, const std::string& list) {
    std::string name = textOf(member(entry, "name"), list + ".name", "");
    if (name.empty()) throw std::runtime_error("every entry in \"" + list + "\" needs a \"name\"");
    return name;
}

void writeTexts(JsonWriter& json, std::string_view key, const std::vector<std::string>& values) {
    if (values.empty()) return;
    json.beginArray(key);
    for (const std::string& value : values) json.item(value);
    json.end();
}

void writeSteps(JsonWriter& json, std::string_view key, const ProjectDescriptor::BuildSteps& steps) {
    if (steps.empty()) return;
    json.beginObject(key);
    for (const auto& [platform, commands] : steps) writeTexts(json, platform, commands);
    json.end();
}

}

std::string_view ProjectDescriptor::engineVersion() { return CINDER_VERSION; }

ProjectDescriptor ProjectDescriptor::parse(std::string_view json) {
    const PropValue document = cinder::serial::parseJson(json);
    if (!document.is<PropRec>()) throw std::runtime_error("a project descriptor must be a JSON object");
    const PropRec& root = document.as<PropRec>();

    const PropValue* version = member(root, "fileVersion");
    if (version == nullptr || !version->is<std::int64_t>()) mustBe("fileVersion", "a whole number");
    if (version->as<std::int64_t>() != FILE_VERSION) {
        throw std::runtime_error("fileVersion " + std::to_string(version->as<std::int64_t>())
                                 + " is not supported (this build reads "
                                 + std::to_string(FILE_VERSION) + ")");
    }

    ProjectDescriptor out;
    out.engineAssociation = textOf(member(root, "engineAssociation"), "engineAssociation", "");
    out.category = textOf(member(root, "category"), "category", "");
    out.description = textOf(member(root, "description"), "description", "");
    out.disableEnginePluginsByDefault = flagOf(member(root, "disableEnginePluginsByDefault"),
                                               "disableEnginePluginsByDefault", false);

    for (const PropRec* entry : objectsOf(member(root, "modules"), "modules")) {
        Module module;
        module.name = nameOf(*entry, "modules");
        module.type = textOf(member(*entry, "type"), "modules.type", module.type);
        module.loadingPhase = textOf(member(*entry, "loadingPhase"), "modules.loadingPhase",
                                     module.loadingPhase);
        module.additionalDependencies = textsOf(member(*entry, "additionalDependencies"),
                                                "modules.additionalDependencies");
        out.modules.push_back(std::move(module));
    }

    for (const PropRec* entry : objectsOf(member(root, "plugins"), "plugins")) {
        Plugin plugin;
        plugin.name = nameOf(*entry, "plugins");
        plugin.enabled = flagOf(member(*entry, "enabled"), "plugins.enabled", plugin.enabled);
        plugin.optional = flagOf(member(*entry, "optional"), "plugins.optional", plugin.optional);
        plugin.description = textOf(member(*entry, "description"), "plugins.description", "");
        plugin.platformAllowList = textsOf(member(*entry, "platformAllowList"),
                                           "plugins.platformAllowList");
        plugin.platformDenyList = textsOf(member(*entry, "platformDenyList"),
                                          "plugins.platformDenyList");
        plugin.targetAllowList = textsOf(member(*entry, "targetAllowList"), "plugins.targetAllowList");
        plugin.targetDenyList = textsOf(member(*entry, "targetDenyList"), "plugins.targetDenyList");
        out.plugins.push_back(std::move(plugin));
    }

    out.additionalRootDirectories = textsOf(member(root, "additionalRootDirectories"),
                                            "additionalRootDirectories");
    out.additionalPluginDirectories = textsOf(member(root, "additionalPluginDirectories"),
                                              "additionalPluginDirectories");
    out.targetPlatforms = textsOf(member(root, "targetPlatforms"), "targetPlatforms");
    out.preBuildSteps = stepsOf(member(root, "preBuildSteps"), "preBuildSteps");
    out.postBuildSteps = stepsOf(member(root, "postBuildSteps"), "postBuildSteps");
    return out;
}

std::string ProjectDescriptor::save(const ProjectDescriptor& descriptor) {
    JsonWriter json;
    json.beginObject();
    json.integer("fileVersion", FILE_VERSION);
    json.text("engineAssociation", descriptor.engineAssociation);
    json.text("category", descriptor.category);
    json.text("description", descriptor.description);
    if (descriptor.disableEnginePluginsByDefault) json.flag("disableEnginePluginsByDefault", true);

    if (!descriptor.modules.empty()) {
        json.beginArray("modules");
        for (const Module& module : descriptor.modules) {
            json.beginObject();
            json.text("name", module.name);
            json.text("type", module.type);
            json.text("loadingPhase", module.loadingPhase);
            writeTexts(json, "additionalDependencies", module.additionalDependencies);
            json.end();
        }
        json.end();
    }

    if (!descriptor.plugins.empty()) {
        json.beginArray("plugins");
        for (const Plugin& plugin : descriptor.plugins) {
            json.beginObject();
            json.text("name", plugin.name);
            json.flag("enabled", plugin.enabled);
            if (plugin.optional) json.flag("optional", true);
            if (!plugin.description.empty()) json.text("description", plugin.description);
            writeTexts(json, "platformAllowList", plugin.platformAllowList);
            writeTexts(json, "platformDenyList", plugin.platformDenyList);
            writeTexts(json, "targetAllowList", plugin.targetAllowList);
            writeTexts(json, "targetDenyList", plugin.targetDenyList);
            json.end();
        }
        json.end();
    }

    writeTexts(json, "additionalRootDirectories", descriptor.additionalRootDirectories);
    writeTexts(json, "additionalPluginDirectories", descriptor.additionalPluginDirectories);
    writeTexts(json, "targetPlatforms", descriptor.targetPlatforms);
    writeSteps(json, "preBuildSteps", descriptor.preBuildSteps);
    writeSteps(json, "postBuildSteps", descriptor.postBuildSteps);
    json.end();
    return json.text();
}

}
