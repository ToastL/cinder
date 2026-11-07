#include "dev/BackgroundProcess.hpp"

#include "dev/Process.hpp"

#include <csignal>
#include <utility>

namespace cinder::dev {

BackgroundProcess::~BackgroundProcess() {
    cancel();
    join();
}

void BackgroundProcess::start(std::unique_ptr<Process> process) {
    join();
    status_ = 0;
    pid_ = process->pid();
    running_.store(true);
    worker_ = std::thread([this, process = std::move(process)]() mutable {
        std::string line;
        while (process->readLine(line)) {
            const std::lock_guard lock(mutex_);
            lines_.push_back(line);
        }
        {
            const std::lock_guard lock(mutex_);
            pid_ = 0;
        }
        status_ = process->wait();
        running_.store(false);
    });
}

void BackgroundProcess::cancel() {
    const std::lock_guard lock(mutex_);
    if (pid_ > 0) kill(-pid_, SIGTERM);
}

void BackgroundProcess::join() {
    if (worker_.joinable()) worker_.join();
}

std::deque<std::string> BackgroundProcess::takeLines() {
    std::deque<std::string> lines;
    const std::lock_guard lock(mutex_);
    lines.swap(lines_);
    return lines;
}

}
