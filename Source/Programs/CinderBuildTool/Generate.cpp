#include "CinderBuildTool/Generate.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace cbt {
namespace {

const char* CONFIGS[] = {"Debug", "Development", "Shipping"};

std::string objectId(const std::string& key) {
    std::uint64_t a = 1469598103934665603ull;
    std::uint64_t b = 0x9e3779b97f4a7c15ull;
    for (const char c : key) {
        a = (a ^ static_cast<unsigned char>(c)) * 1099511628211ull;
        b = (b ^ static_cast<unsigned char>(c)) * 0x100000001b3ull + 7;
    }
    char text[25];
    std::snprintf(text, sizeof(text), "%016llX%08X", static_cast<unsigned long long>(a), static_cast<unsigned>(b >> 32));
    return text;
}

std::string quote(const std::string& value) {
    std::string out = "\"";
    for (const char c : value) {
        if (c == '"' || c == '\\') out.push_back('\\');
        out.push_back(c);
    }
    return out + "\"";
}

std::string xml(const std::string& value) {
    std::string out;
    for (const char c : value) {
        if (c == '&') out += "&amp;";
        else if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '"') out += "&quot;";
        else out.push_back(c);
    }
    return out;
}

std::string fileType(const std::filesystem::path& path) {
    const std::string extension = path.extension().string();
    if (extension == ".cpp" || extension == ".cc") return "sourcecode.cpp.cpp";
    if (extension == ".hpp" || extension == ".hh" || extension == ".inl") return "sourcecode.cpp.h";
    if (extension == ".h") return "sourcecode.c.h";
    if (extension == ".c") return "sourcecode.c.c";
    if (extension == ".mm") return "sourcecode.cpp.objcpp";
    if (extension == ".m") return "sourcecode.c.objc";
    if (extension == ".md") return "net.daringfireball.markdown";
    if (extension == ".sh") return "text.script.sh";
    return "text";
}

bool browsable(const std::filesystem::path& path) {
    static const std::set<std::string> extensions{".cpp", ".hpp", ".h", ".c", ".mm", ".m", ".inl", ".hlsl"};
    return extensions.count(path.extension().string()) != 0;
}

bool writeFile(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << text;
    return static_cast<bool>(out);
}

struct Project {
    std::map<std::string, std::string> sections;

    void add(const std::string& isa, const std::string& id, const std::string& body) {
        sections[isa] += "\t\t" + id + " = {\n\t\t\tisa = " + isa + ";\n" + body + "\t\t};\n";
    }
};

struct Group {
    std::map<std::string, Group> children;
    std::vector<std::filesystem::path> files;
};

std::string emitGroup(Project& project, const std::string& key, const std::string& name, const Group& group,
                      std::map<std::filesystem::path, std::string>& refs) {
    std::string children;
    for (const auto& [childName, child] : group.children) {
        children += "\t\t\t\t" + emitGroup(project, key + "/" + childName, childName, child, refs) + ",\n";
    }
    std::vector<std::filesystem::path> files = group.files;
    std::sort(files.begin(), files.end());
    for (const std::filesystem::path& file : files) {
        const std::string id = objectId("file:" + file.string());
        refs[file] = id;
        project.add("PBXFileReference", id,
                    "\t\t\tlastKnownFileType = " + fileType(file) + ";\n\t\t\tname = " + quote(file.filename().string())
                            + ";\n\t\t\tpath = " + quote(file.string()) + ";\n\t\t\tsourceTree = \"<absolute>\";\n");
        children += "\t\t\t\t" + id + ",\n";
    }
    const std::string id = objectId("group:" + key);
    project.add("PBXGroup", id,
                "\t\t\tchildren = (\n" + children + "\t\t\t);\n" + (name.empty() ? "" : "\t\t\tname = " + quote(name) + ";\n")
                        + "\t\t\tsourceTree = \"<group>\";\n");
    return id;
}

std::string configurationList(Project& project, const std::string& owner,
                              const std::map<std::string, std::string>& settings) {
    std::string configurations;
    for (const char* config : CONFIGS) {
        const std::string id = objectId(owner + ":config:" + config);
        std::string body = "\t\t\tbuildSettings = {\n";
        for (const auto& [key, value] : settings) {
            std::string expanded = value;
            for (std::size_t at; (at = expanded.find("@CONFIG@")) != std::string::npos;) expanded.replace(at, 8, config);
            body += "\t\t\t\t" + key + " = " + expanded + ";\n";
        }
        body += "\t\t\t};\n\t\t\tname = " + std::string(config) + ";\n";
        project.add("XCBuildConfiguration", id, body);
        configurations += "\t\t\t\t" + id + ",\n";
    }
    const std::string id = objectId(owner + ":configurations");
    project.add("XCConfigurationList", id,
                "\t\t\tbuildConfigurations = (\n" + configurations
                        + "\t\t\t);\n\t\t\tdefaultConfigurationIsVisible = 0;\n\t\t\tdefaultConfigurationName = Development;\n");
    return id;
}

std::string list(const std::vector<std::string>& values) {
    std::string out = "(\n";
    for (const std::string& value : values) out += "\t\t\t\t\t" + quote(value) + ",\n";
    return out + "\t\t\t\t)";
}

std::string scheme(const std::string& target, const std::string& id, const std::filesystem::path& binaries,
                   const std::filesystem::path& root, const std::vector<std::string>& arguments) {
    const std::string reference = "<BuildableReference BuildableIdentifier = \"primary\" BlueprintIdentifier = \"" + id
            + "\" BuildableName = \"" + target + "\" BlueprintName = \"" + target
            + "\" ReferencedContainer = \"container:Cinder.xcodeproj\"/>";
    std::string args;
    for (const std::string& argument : arguments) {
        args += "         <CommandLineArgument argument = \"" + xml(argument) + "\" isEnabled = \"YES\"/>\n";
    }
    const auto runnable = [&](const char* config) {
        return "      <PathRunnable runnableDebuggingMode = \"0\" FilePath = \"" + xml((binaries / config / target).string()) + "\"/>\n";
    };
    const std::string working = "useCustomWorkingDirectory = \"YES\" customWorkingDirectory = \"" + xml(root.string()) + "\"";
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Scheme LastUpgradeVersion = \"2600\" version = \"1.7\">\n"
           "   <BuildAction parallelizeBuildables = \"YES\" buildImplicitDependencies = \"YES\">\n      <BuildActionEntries>\n"
           "         <BuildActionEntry buildForTesting = \"YES\" buildForRunning = \"YES\" buildForProfiling = \"YES\" "
           "buildForArchiving = \"YES\" buildForAnalyzing = \"YES\">\n            "
           + reference + "\n         </BuildActionEntry>\n      </BuildActionEntries>\n   </BuildAction>\n"
           "   <LaunchAction buildConfiguration = \"Debug\" selectedDebuggerIdentifier = \"Xcode.DebuggerFoundation.Debugger.LLDB\" "
           "selectedLauncherIdentifier = \"Xcode.DebuggerFoundation.Launcher.LLDB\" launchStyle = \"0\" "
           + working + " ignoresPersistentStateOnLaunch = \"NO\" debugDocumentVersioning = \"YES\" allowLocationSimulation = \"YES\">\n"
           + runnable("Debug") + "      <MacroExpansion>\n         " + reference + "\n      </MacroExpansion>\n"
           + (args.empty() ? "" : "      <CommandLineArguments>\n" + args + "      </CommandLineArguments>\n")
           + "   </LaunchAction>\n"
           "   <ProfileAction buildConfiguration = \"Development\" shouldUseLaunchSchemeArgsEnv = \"YES\" "
           + working + " debugDocumentVersioning = \"YES\">\n" + runnable("Development") + "   </ProfileAction>\n"
           "   <AnalyzeAction buildConfiguration = \"Debug\"/>\n"
           "   <ArchiveAction buildConfiguration = \"Shipping\" revealArchiveInOrganizer = \"YES\"/>\n</Scheme>\n";
}

}

bool generateCompileCommands(Builder& builder, const std::vector<std::string>& targets) {
    const std::filesystem::path file = builder.root() / "compile_commands.json";
    std::vector<Action> actions = builder.plan(targets);
    const std::filesystem::path programs = builder.root() / "Source" / "Programs";
    std::vector<std::filesystem::path> tool;
    for (const char* folder : {"Source", "Tests"}) {
        std::error_code error;
        for (auto it = std::filesystem::recursive_directory_iterator(builder.root() / folder, error);
             !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
            const std::string name = it->path().filename().string();
            const bool rule = name.ends_with(".Build.cpp") || name.ends_with(".Target.cpp");
            if (it->is_regular_file() && (rule || (it->path().parent_path() == programs / "CinderBuildTool" && it->path().extension() == ".cpp"))) {
                tool.push_back(it->path());
            }
        }
    }
    for (const std::filesystem::path& source : tool) {
        actions.push_back(Action{"compile " + source.filename().string(),
                                 {"c++", "-std=c++20", "-I" + programs.string(), "-c", source.string()}, {}, {source}, {}, {}, {}});
    }
    writeCompileCommands(file, builder.root(), actions);
    std::printf("cbt: wrote %s\n", file.string().c_str());
    return true;
}

bool generateXcode(Builder& builder, const std::vector<std::string>& targets, const std::filesystem::path& cbt) {
    const std::filesystem::path root = builder.root();
    const std::filesystem::path bundle = builder.output() / "Cinder.xcodeproj";
    const std::filesystem::path binaries = builder.binaries().parent_path();
    const std::vector<Action> actions = builder.plan(targets);

    std::set<std::filesystem::path> indexed;
    std::set<std::string> includes;
    std::set<std::string> defines;
    for (const Action& action : actions) {
        if (!action.description.starts_with("compile ")) continue;
        const auto found = std::find(action.command.begin(), action.command.end(), "-c");
        if (found == action.command.end() || found + 1 == action.command.end()) continue;
        const std::filesystem::path source = *(found + 1);
        const std::string relative = source.lexically_relative(root).generic_string();
        if (relative.starts_with("Source/ThirdParty/") || relative.starts_with("..") || relative.starts_with("Intermediate/")) continue;
        indexed.insert(source);
        for (std::size_t i = 0; i + 1 < action.command.size(); ++i) {
            const std::string& arg = action.command[i];
            if (arg == "-isystem") includes.insert(action.command[i + 1]);
            else if (arg.starts_with("-I")) includes.insert(arg.substr(2));
            else if (arg.starts_with("-D") && arg.find_first_of("\"\\ ") == std::string::npos) defines.insert(arg.substr(2));
        }
    }

    Group main;
    for (const char* folder : {"Source", "Tests", "Shaders", "Samples"}) {
        std::error_code error;
        for (auto it = std::filesystem::recursive_directory_iterator(root / folder, error);
             !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
            const std::string leaf = it->path().filename().string();
            if (it->is_directory() && (leaf == "Saved" || leaf == "Content" || it->path() == root / "Source" / "ThirdParty")) {
                it.disable_recursion_pending();
                continue;
            }
            if (!it->is_regular_file() || !browsable(it->path())) continue;
            Group* group = &main;
            for (const auto& part : it->path().parent_path().lexically_relative(root)) group = &group->children[part.string()];
            group->files.push_back(it->path());
        }
    }
    for (const char* file : {"CLAUDE.md", "README.md", "TODO.md", "Setup.sh"}) {
        if (std::filesystem::exists(root / file)) main.files.push_back(root / file);
    }

    Project project;
    std::map<std::filesystem::path, std::string> refs;
    const std::string mainGroup = emitGroup(project, "", "", main, refs);

    std::vector<std::string> targetIds;
    std::map<std::string, std::string> legacy;
    for (const std::string& target : targets) {
        const std::string id = objectId("target:" + target);
        legacy[target] = id;
        targetIds.push_back(id);
        const char* path = std::getenv("PATH");
        const std::string arguments = "xcode --output " + quote(builder.output().string()) + " --backend " + builder.backend()
                + (path != nullptr && *path != '\0' ? " --path " + quote(path) : std::string())
                + " $(TARGET_NAME) $(CONFIGURATION) $(ACTION)";
        const std::string configurations = configurationList(project, "target:" + target, {{"PRODUCT_NAME", quote(target)}});
        project.add("PBXLegacyTarget", id,
                    "\t\t\tbuildArgumentsString = " + quote(arguments) + ";\n\t\t\tbuildConfigurationList = " + configurations
                            + ";\n\t\t\tbuildPhases = (\n\t\t\t);\n\t\t\tbuildToolPath = " + quote(cbt.string())
                            + ";\n\t\t\tbuildWorkingDirectory = " + quote(root.string())
                            + ";\n\t\t\tdependencies = (\n\t\t\t);\n\t\t\tname = " + quote(target)
                            + ";\n\t\t\tpassBuildSettingsInEnvironment = 0;\n\t\t\tproductName = " + quote(target) + ";\n");
    }

    std::string buildFiles;
    for (const std::filesystem::path& source : indexed) {
        const auto ref = refs.find(source);
        if (ref == refs.end()) continue;
        const std::string id = objectId("build:" + source.string());
        project.add("PBXBuildFile", id, "\t\t\tfileRef = " + ref->second + ";\n");
        buildFiles += "\t\t\t\t" + id + ",\n";
    }
    for (const auto& [file, ref] : refs) {
        const std::string relative = file.lexically_relative(root).generic_string();
        if (!relative.starts_with("Samples/") || file.extension() != ".cpp") continue;
        const std::string id = objectId("build:" + file.string());
        project.add("PBXBuildFile", id, "\t\t\tfileRef = " + ref + ";\n");
        buildFiles += "\t\t\t\t" + id + ",\n";
    }
    const std::string sources = objectId("index:sources");
    project.add("PBXSourcesBuildPhase", sources,
                "\t\t\tbuildActionMask = 2147483647;\n\t\t\tfiles = (\n" + buildFiles
                        + "\t\t\t);\n\t\t\trunOnlyForDeploymentPostprocessing = 0;\n");
    const std::string index = objectId("target:Index");
    targetIds.push_back(index);
    const std::string indexConfigurations = configurationList(
            project, "target:Index",
            {{"PRODUCT_NAME", "Index"},
             {"CLANG_CXX_LANGUAGE_STANDARD", quote("c++20")},
             {"CLANG_ENABLE_OBJC_ARC", "YES"},
             {"GCC_PREPROCESSOR_DEFINITIONS", list({defines.begin(), defines.end()})},
             {"HEADER_SEARCH_PATHS", list({includes.begin(), includes.end()})},
             {"USE_HEADERMAP", "NO"},
             {"CODE_SIGNING_ALLOWED", "NO"}});
    project.add("PBXNativeTarget", index,
                "\t\t\tbuildConfigurationList = " + indexConfigurations + ";\n\t\t\tbuildPhases = (\n\t\t\t\t" + sources
                        + ",\n\t\t\t);\n\t\t\tbuildRules = (\n\t\t\t);\n\t\t\tdependencies = (\n\t\t\t);\n\t\t\tname = Index;\n"
                          "\t\t\tproductName = Index;\n\t\t\tproductType = \"com.apple.product-type.tool\";\n");

    const std::string projectConfigurations =
            configurationList(project, "project",
                              {{"SDKROOT", "macosx"},
                               {"SYMROOT", quote((builder.output() / "Intermediate" / "Xcode" / "Build").string())},
                               {"OBJROOT", quote((builder.output() / "Intermediate" / "Xcode" / "Objects").string())}});
    std::string targetList;
    for (const std::string& id : targetIds) targetList += "\t\t\t\t" + id + ",\n";
    const std::string rootObject = objectId("project");
    project.add("PBXProject", rootObject,
                "\t\t\tattributes = {\n\t\t\t\tBuildIndependentTargetsInParallel = 1;\n\t\t\t\tLastUpgradeCheck = 2600;\n\t\t\t};\n"
                "\t\t\tbuildConfigurationList = " + projectConfigurations
                        + ";\n\t\t\tcompatibilityVersion = \"Xcode 14.0\";\n\t\t\tdevelopmentRegion = en;\n\t\t\thasScannedForEncodings = 0;\n"
                          "\t\t\tknownRegions = (\n\t\t\t\ten,\n\t\t\t);\n\t\t\tmainGroup = " + mainGroup
                        + ";\n\t\t\tprojectDirPath = \"\";\n\t\t\tprojectRoot = \"\";\n\t\t\ttargets = (\n"
                        + targetList + "\t\t\t);\n");

    std::string text = "// !$*UTF8*$!\n{\n\tarchiveVersion = 1;\n\tclasses = {\n\t};\n\tobjectVersion = 56;\n\tobjects = {\n";
    for (const auto& [isa, body] : project.sections) text += "\n/* Begin " + isa + " section */\n" + body + "/* End " + isa + " section */\n";
    text += "\t};\n\trootObject = " + rootObject + ";\n}\n";

    std::error_code error;
    std::filesystem::remove_all(bundle / "xcshareddata" / "xcschemes", error);
    bool ok = writeFile(bundle / "project.pbxproj", text);
    ok = writeFile(bundle / "project.xcworkspace" / "contents.xcworkspacedata",
                   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Workspace version = \"1.0\">\n"
                   "   <FileRef location = \"self:\">\n   </FileRef>\n</Workspace>\n") && ok;
    ok = writeFile(bundle / "project.xcworkspace" / "xcshareddata" / "WorkspaceSettings.xcsettings",
                   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
                   "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n<plist version=\"1.0\">\n<dict>\n"
                   "\t<key>IDEWorkspaceSharedSettings_AutocreateContextsIfNeeded</key>\n\t<false/>\n</dict>\n</plist>\n") && ok;
    const std::string sample = (root / "Samples" / "sandbox3d").string();
    for (const auto& [target, id] : legacy) {
        std::vector<std::string> arguments;
        if (target == "editor" || target == "player" || target == "packager") arguments.push_back(sample);
        ok = writeFile(bundle / "xcshareddata" / "xcschemes" / (target + ".xcscheme"), scheme(target, id, binaries, root, arguments)) && ok;
    }
    std::printf("cbt: wrote %s (%zu targets, %zu indexed sources)\n", bundle.string().c_str(), legacy.size(), indexed.size());
    return ok;
}

}
