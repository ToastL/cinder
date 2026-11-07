#pragma once

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <sys/types.h>

namespace cinder::dev {

class Process;

class BackgroundProcess {
public:
    BackgroundProcess() = default;
    ~BackgroundProcess();

    BackgroundProcess(const BackgroundProcess&) = delete;
    BackgroundProcess& operator=(const BackgroundProcess&) = delete;

    void start(std::unique_ptr<Process> process);
    bool running() const { return running_.load(); }
    void cancel();
    void join();
    std::deque<std::string> takeLines();
    int status() const { return status_; }

private:
    std::thread worker_;
    std::atomic<bool> running_ = false;
    std::mutex mutex_;
    std::deque<std::string> lines_;
    pid_t pid_ = 0;
    int status_ = 0;
};

}
