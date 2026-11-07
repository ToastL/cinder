#pragma once

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include <sys/types.h>

namespace cinder::Dev {

class Process {
public:
    static std::unique_ptr<Process> start(const std::vector<std::string>& argv, std::string& error);
    static std::unique_ptr<Process> shell(const std::string& command, std::string& error);

    ~Process();

    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    pid_t pid() const { return pid_; }
    bool readLine(std::string& line);
    int wait();
    void terminate();

private:
    Process(pid_t pid, std::FILE* output) : pid_(pid), output_(output) {}

    pid_t pid_;
    std::FILE* output_;
    bool reaped_ = false;
    int status_ = -1;
};

}
