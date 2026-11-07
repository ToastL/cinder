#include "CinderBuildTool/Executor.hpp"

#include <cstdint>

#include "CinderBuildTool/Process.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>

namespace cbt {
namespace {

std::vector<std::filesystem::path> dependencies(const std::filesystem::path& depfile) {
    std::ifstream in(depfile);
    std::stringstream buffer;
    buffer << in.rdbuf();
    const std::string text = buffer.str();
    std::vector<std::filesystem::path> out;
    std::string token;
    bool target = true;
    for (std::size_t i = 0; i <= text.size(); ++i) {
        const char c = i < text.size() ? text[i] : ' ';
        if (c == '\\' && i + 1 < text.size() && text[i + 1] == ' ') {
            token.push_back(' ');
            ++i;
        } else if (c == '\\' && i + 1 < text.size() && (text[i + 1] == '\n' || text[i + 1] == '\r')) {
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

std::string key(const Action& action) { return action.outputs.empty() ? action.description : action.outputs.front().string(); }

}

std::string signatureOf(const Action& action) {
    std::string text = action.signature;
    for (const std::string& arg : action.command) text += arg + '\x1f';
    std::uint64_t hash = 1469598103934665603ull;
    for (const char c : text) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 1099511628211ull;
    }
    return std::to_string(hash);
}

Executor::Executor(std::filesystem::path log, int jobs) : log_(std::move(log)), jobs_(jobs) { load(); }

void Executor::load() {
    std::ifstream in(log_);
    std::string line;
    while (std::getline(in, line)) {
        const std::size_t tab = line.find('\t');
        if (tab != std::string::npos) signatures_[line.substr(tab + 1)] = line.substr(0, tab);
    }
}

void Executor::save() const {
    std::filesystem::create_directories(log_.parent_path());
    std::ofstream out(log_);
    for (const auto& [output, signature] : signatures_) out << signature << '\t' << output << '\n';
}

bool Executor::stale(const Action& action) const {
    if (action.outputs.empty()) return true;
    auto found = signatures_.find(key(action));
    if (found == signatures_.end() || found->second != signatureOf(action)) return true;

    std::error_code error;
    std::filesystem::file_time_type oldest = std::filesystem::file_time_type::max();
    for (const std::filesystem::path& output : action.outputs) {
        const auto time = std::filesystem::last_write_time(output, error);
        if (error) return true;
        oldest = std::min(oldest, time);
    }
    std::vector<std::filesystem::path> inputs = action.inputs;
    if (!action.depfile.empty()) {
        if (!std::filesystem::exists(action.depfile)) return true;
        const std::vector<std::filesystem::path> extra = dependencies(action.depfile);
        inputs.insert(inputs.end(), extra.begin(), extra.end());
    }
    for (const std::filesystem::path& input : inputs) {
        const auto time = std::filesystem::last_write_time(input, error);
        if (error || time > oldest) return true;
    }
    return false;
}

ExecutorResult Executor::run(std::vector<Action> actions) {
    ExecutorResult result;
    std::map<std::string, std::size_t> producers;
    for (std::size_t i = 0; i < actions.size(); ++i) {
        for (const std::filesystem::path& output : actions[i].outputs) producers[output.string()] = i;
    }

    std::vector<std::vector<std::size_t>> dependents(actions.size());
    std::vector<int> waiting(actions.size(), 0);
    for (std::size_t i = 0; i < actions.size(); ++i) {
        std::set<std::size_t> needs;
        for (const std::filesystem::path& input : actions[i].inputs) {
            auto found = producers.find(input.string());
            if (found != producers.end() && found->second != i) needs.insert(found->second);
        }
        for (const std::size_t need : needs) dependents[need].push_back(i);
        waiting[i] = static_cast<int>(needs.size());
    }

    std::mutex mutex;
    std::condition_variable wake;
    std::vector<std::size_t> ready;
    for (std::size_t i = 0; i < actions.size(); ++i) {
        if (waiting[i] == 0) ready.push_back(i);
    }
    std::size_t finished = 0;
    std::size_t started = 0;
    bool failed = false;
    const std::size_t total = actions.size();

    const auto work = [&] {
        std::unique_lock lock(mutex);
        while (true) {
            wake.wait(lock, [&] { return !ready.empty() || finished == total || failed; });
            if (ready.empty()) return;
            const std::size_t index = ready.back();
            ready.pop_back();
            Action& action = actions[index];
            if (failed) {
                ++finished;
                wake.notify_all();
                continue;
            }

            bool ok = true;
            bool ran = false;
            if (stale(action)) {
                ran = true;
                const std::size_t number = ++started;
                lock.unlock();
                for (const std::filesystem::path& output : action.outputs) {
                    std::filesystem::create_directories(output.parent_path());
                }
                std::string output;
                if (action.builtin) {
                    ok = action.builtin(output);
                } else {
                    std::string error;
                    std::unique_ptr<Process> process = Process::start(action.command, error);
                    if (process == nullptr) {
                        ok = false;
                        output = error + "\n";
                    } else {
                        std::string line;
                        while (process->readLine(line)) output += line + "\n";
                        ok = process->wait() == 0;
                    }
                }
                lock.lock();
                std::printf("[%zu] %s\n%s", number, action.description.c_str(), output.c_str());
                std::fflush(stdout);
                if (ok) {
                    signatures_[key(action)] = signatureOf(action);
                } else {
                    failed = true;
                    std::error_code error;
                    for (const std::filesystem::path& out : action.outputs) std::filesystem::remove(out, error);
                }
            }
            if (!ok) {
                ++finished;
                wake.notify_all();
                continue;
            }
            if (ran) ++result.ran;
            else ++result.upToDate;
            ++finished;
            if (!failed) {
                for (const std::size_t next : dependents[index]) {
                    if (--waiting[next] == 0) ready.push_back(next);
                }
            }
            wake.notify_all();
        }
    };

    const int workers = std::max(1, std::min(jobs_, static_cast<int>(actions.size())));
    std::vector<std::thread> threads;
    for (int i = 0; i < workers; ++i) threads.emplace_back(work);
    for (std::thread& thread : threads) thread.join();

    result.ok = !failed && finished == total;
    save();
    return result;
}

}
