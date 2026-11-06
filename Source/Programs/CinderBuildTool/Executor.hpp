#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace cbt {

struct Action {
    std::string description;
    std::vector<std::string> command;
    std::function<bool(std::string&)> builtin;
    std::vector<std::filesystem::path> inputs;
    std::vector<std::filesystem::path> outputs;
    std::filesystem::path depfile;
    std::string signature;
};

struct ExecutorResult {
    bool ok = true;
    int ran = 0;
    int upToDate = 0;
};

class Executor {
public:
    Executor(std::filesystem::path log, int jobs);

    ExecutorResult run(std::vector<Action> actions);

private:
    bool stale(const Action& action) const;
    void load();
    void save() const;

    std::filesystem::path log_;
    int jobs_;
    std::map<std::string, std::string> signatures_;
};

std::string signatureOf(const Action& action);

}
