#include "dev/ModuleBuilder.hpp"

#include "core/GameModule.hpp"
#include "dev/Process.hpp"
#include "platform/Files.hpp"
#include "serial/Json.hpp"

#include <algorithm>
#include <csignal>
#include <system_error>
#include <thread>

namespace cinder::dev {
namespace {

std::filesystem::file_time_type modified(const std::filesystem::path& path, bool& found) {
    std::error_code error;
    const auto time = std::filesystem::last_write_time(path, error);
    found = !error;
    return time;
}

std::vector<std::filesystem::path> dependencies(const std::string& depfile) {
    std::vector<std::filesystem::path> out;
    std::string token;
    bool target = true;
    for (std::size_t i = 0; i <= depfile.size(); ++i) {
        const char c = i < depfile.size() ? depfile[i] : ' ';
        if (c == '\\' && i + 1 < depfile.size() && depfile[i + 1] == ' ') {
            token.push_back(' ');
            ++i;
        } else if (c == '\\' && i + 1 < depfile.size() && (depfile[i + 1] == '\n' || depfile[i + 1] == '\r')) {
            ++i;
        } else if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            if (token.empty()) continue;
            if (target && token.back() == ':') target = false;
            else if (!target) out.emplace_back(token);
            token.clear();
        } else {
            token.push_back(c);
        }
    }
    return out;
}

bool isError(const std::string& line) { return line.find(": error:") != std::string::npos || line.find(": fatal error:") != std::string::npos; }

}

ModuleBuilder::ModuleBuilder(std::filesystem::path project, std::string name, Toolchain toolchain,
                             std::filesystem::path library, std::string config)
    : project_(std::move(project)), name_(std::move(name)), toolchain_(std::move(toolchain)),
      library_(library.empty() ? project_ / "Saved" / "Binaries" / (name_ + std::string(cinder::core::GameModule::extension()))
                               : std::move(library)),
      intermediate_(project_ / "Saved" / "Intermediate" / config) {}

std::vector<std::filesystem::path> ModuleBuilder::sources() const {
    std::vector<std::filesystem::path> out;
    std::error_code error;
    const std::filesystem::path root = project_ / "Source";
    if (!std::filesystem::is_directory(root, error)) return out;
    for (auto it = std::filesystem::recursive_directory_iterator(root, error);
         !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
        if (it->is_regular_file(error) && it->path().extension() == ".cpp") out.push_back(it->path());
    }
    std::sort(out.begin(), out.end());
    return out;
}

bool ModuleBuilder::stale(const Job& job, bool rebuildAll) const {
    if (rebuildAll) return true;
    bool found = false;
    const auto object = modified(job.object, found);
    if (!found) return true;
    const auto source = modified(job.source, found);
    if (!found || source > object) return true;

    std::string depfile;
    try {
        depfile = cinder::platform::readTextFile(job.depfile);
    } catch (const std::exception&) {
        return true;
    }
    for (const std::filesystem::path& dependency : dependencies(depfile)) {
        const auto time = modified(dependency, found);
        if (!found || time > object) return true;
    }
    return false;
}

bool ModuleBuilder::run(const std::vector<std::string>& args, std::vector<std::string>& output) {
    std::string error;
    std::unique_ptr<Process> process = Process::start(args, error);
    if (process == nullptr) {
        output.push_back(error);
        return false;
    }
    {
        const std::lock_guard lock(mutex_);
        running_.push_back(process->pid());
    }
    if (cancelled_.load()) process->terminate();

    std::string line;
    while (process->readLine(line)) output.push_back(line);
    {
        const std::lock_guard lock(mutex_);
        std::erase(running_, process->pid());
    }
    return process->wait() == 0;
}

void ModuleBuilder::cancel() {
    cancelled_.store(true);
    const std::lock_guard lock(mutex_);
    for (const pid_t pid : running_) kill(-pid, SIGTERM);
}

ModuleBuilder::Result ModuleBuilder::build(const Sink& sink) {
    Result result;
    cancelled_.store(false);
    std::filesystem::create_directories(intermediate_);
    std::filesystem::create_directories(library_.parent_path());

    const std::filesystem::path entry = intermediate_ / "ModuleEntry.cpp";
    const std::string entryText = "#include \"core/Module.hpp\"\nCINDER_MODULE_ENTRY()\n";
    bool entryCurrent = false;
    try {
        entryCurrent = cinder::platform::readTextFile(entry) == entryText;
    } catch (const std::exception&) {
    }
    if (!entryCurrent) cinder::platform::writeTextFile(entry, entryText);

    const std::filesystem::path stamp = intermediate_ / "flags.txt";
    bool rebuildAll = true;
    try {
        rebuildAll = cinder::platform::readTextFile(stamp) != toolchain_.fingerprint();
    } catch (const std::exception&) {
    }

    std::vector<Job> jobs;
    for (const std::filesystem::path& source : sources()) {
        std::string flat = std::filesystem::relative(source, project_ / "Source").string();
        std::replace(flat.begin(), flat.end(), '/', '_');
        jobs.push_back(Job{source, intermediate_ / (flat + ".o"), intermediate_ / (flat + ".d")});
    }
    if (jobs.empty()) {
        sink("no C++ sources in " + (project_ / "Source").string());
        result.ok = true;
        return result;
    }
    jobs.push_back(Job{entry, intermediate_ / "ModuleEntry.o", intermediate_ / "ModuleEntry.d"});
    writeCompileCommands(jobs);

    std::vector<const Job*> pending;
    for (const Job& job : jobs) {
        if (stale(job, rebuildAll)) pending.push_back(&job);
    }

    std::mutex reporting;
    std::atomic<std::size_t> next = 0;
    std::atomic<bool> failed = false;
    const unsigned workers = std::max(1u, std::min(std::thread::hardware_concurrency(), static_cast<unsigned>(pending.size())));
    std::vector<std::thread> threads;
    for (unsigned w = 0; w < workers; ++w) {
        threads.emplace_back([&] {
            for (std::size_t i = next++; i < pending.size() && !cancelled_.load(); i = next++) {
                const Job& job = *pending[i];
                std::vector<std::string> output;
                const bool ok = run(toolchain_.compile(job.source, job.object, job.depfile), output);
                if (!ok) {
                    failed.store(true);
                    std::error_code error;
                    std::filesystem::remove(job.object, error);
                }
                const std::lock_guard lock(reporting);
                sink("compiling " + std::filesystem::relative(job.source, project_).string());
                for (const std::string& line : output) {
                    if (isError(line)) result.errors++;
                    sink(line);
                }
                result.compiled++;
            }
        });
    }
    for (std::thread& thread : threads) thread.join();

    if (cancelled_.load()) {
        sink("build cancelled");
        return result;
    }
    if (failed.load()) {
        if (result.errors == 0) result.errors = 1;
        sink("build failed with " + std::to_string(result.errors) + (result.errors == 1 ? " error" : " errors"));
        return result;
    }
    cinder::platform::writeTextFile(stamp, toolchain_.fingerprint());

    bool found = false;
    const auto linkedAt = modified(library_, found);
    bool relink = !found || !pending.empty();
    for (const Job& job : jobs) {
        bool exists = false;
        if (!relink && modified(job.object, exists) > linkedAt) relink = true;
    }
    if (relink) {
        std::vector<std::filesystem::path> objects;
        for (const Job& job : jobs) objects.push_back(job.object);
        const std::filesystem::path temporary = library_.string() + ".tmp";
        std::vector<std::string> output;
        sink("linking " + library_.filename().string());
        const bool ok = run(toolchain_.link(objects, temporary), output);
        for (const std::string& line : output) sink(line);
        if (!ok) {
            result.errors = std::max(result.errors, 1);
            sink("link failed");
            return result;
        }
        std::filesystem::rename(temporary, library_);
        result.linked = true;
    }
    result.ok = true;
    return result;
}

void ModuleBuilder::writeCompileCommands(const std::vector<Job>& jobs) const {
    cinder::serial::JsonWriter json;
    json.beginArray();
    for (const Job& job : jobs) {
        json.beginObject();
        json.text("directory", project_.string());
        json.text("file", job.source.string());
        json.beginArray("arguments");
        for (const std::string& arg : toolchain_.compile(job.source, job.object, job.depfile)) json.item(arg);
        json.end();
        json.end();
    }
    json.end();
    cinder::platform::writeTextFile(project_ / "compile_commands.json", json.text() + "\n");
}

}
