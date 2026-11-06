#include "dev/Toolchain.hpp"

#include "platform/Assets.hpp"
#include "platform/Files.hpp"
#include "scene/PropValue.hpp"
#include "serial/Json.hpp"

#include <exception>

namespace cinder::dev {
namespace {

using cinder::scene::PropRec;
using cinder::scene::PropSeq;
using cinder::scene::PropValue;

std::string textOf(const PropRec& record, const std::string& key) {
    auto found = record.find(key);
    if (found == record.end() || !found->second.is<std::string>()) {
        throw std::runtime_error("the manifest has no " + key);
    }
    return found->second.as<std::string>();
}

std::vector<std::string> listOf(const PropRec& record, const std::string& key) {
    std::vector<std::string> out;
    auto found = record.find(key);
    if (found == record.end()) return out;
    if (!found->second.is<PropSeq>()) throw std::runtime_error(key + " is not a list");
    for (const PropValue& value : found->second.as<PropSeq>()) {
        if (!value.is<std::string>()) throw std::runtime_error(key + " holds something that is not text");
        if (!value.as<std::string>().empty()) out.push_back(value.as<std::string>());
    }
    return out;
}

}

std::filesystem::path Toolchain::defaultPath() { return cinder::platform::executableDir() / "cinder-toolchain.json"; }

std::optional<Toolchain> Toolchain::load(const std::filesystem::path& manifest, std::string& error) {
    try {
        const PropValue root = cinder::serial::parseJson(cinder::platform::readTextFile(manifest));
        if (!root.is<PropRec>()) throw std::runtime_error("the manifest is not an object");
        const PropRec& record = root.as<PropRec>();
        Toolchain toolchain;
        toolchain.compiler = textOf(record, "compiler");
        toolchain.abi = textOf(record, "abi");
        toolchain.flags = listOf(record, "flags");
        toolchain.includes = listOf(record, "includes");
        toolchain.defines = listOf(record, "defines");
        return toolchain;
    } catch (const std::exception& e) {
        error = manifest.string() + ": " + e.what();
        return std::nullopt;
    }
}

std::vector<std::string> Toolchain::compile(const std::filesystem::path& source, const std::filesystem::path& object,
                                            const std::filesystem::path& depfile) const {
    std::vector<std::string> args{compiler};
    args.insert(args.end(), flags.begin(), flags.end());
    for (const std::string& include : includes) args.push_back("-I" + include);
    for (const std::string& define : defines) args.push_back("-D" + define);
    args.push_back("-DCINDER_MODULE_ABI=\"" + abi + "\"");
    args.insert(args.end(), {"-MMD", "-MF", depfile.string(), "-c", source.string(), "-o", object.string()});
    return args;
}

std::vector<std::string> Toolchain::link(const std::vector<std::filesystem::path>& objects,
                                         const std::filesystem::path& library) const {
    std::vector<std::string> args{compiler, "-shared"};
#if defined(__APPLE__)
    args.insert(args.end(), {"-undefined", "dynamic_lookup"});
#endif
    for (const std::filesystem::path& object : objects) args.push_back(object.string());
    args.insert(args.end(), {"-o", library.string()});
    return args;
}

std::string Toolchain::fingerprint() const {
    std::string text = compiler + "\n" + abi + "\n";
    for (const auto* list : {&flags, &includes, &defines}) {
        for (const std::string& item : *list) text += item + "\n";
        text += "--\n";
    }
    return text;
}

}
