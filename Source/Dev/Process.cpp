#include "Dev/Process.hpp"

#include <array>
#include <cerrno>
#include <csignal>
#include <cstring>

#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

namespace cinder::Dev {

namespace {

bool openPipe(int (&fds)[2]) {
#if defined(__APPLE__)
    if (pipe(fds) != 0) return false;
    fcntl(fds[0], F_SETFD, FD_CLOEXEC);
    fcntl(fds[1], F_SETFD, FD_CLOEXEC);
    return true;
#else
    return pipe2(fds, O_CLOEXEC) == 0;
#endif
}

}

std::unique_ptr<Process> Process::start(const std::vector<std::string>& argv, std::string& error) {
    if (argv.empty()) {
        error = "no program to run";
        return nullptr;
    }

    int fds[2];
    if (!openPipe(fds)) {
        error = "could not open a pipe";
        return nullptr;
    }

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, fds[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, fds[1], STDERR_FILENO);
    posix_spawn_file_actions_addclose(&actions, fds[0]);
    posix_spawn_file_actions_addclose(&actions, fds[1]);
#if defined(__APPLE__)
    posix_spawn_file_actions_addinherit_np(&actions, STDIN_FILENO);
#endif

    posix_spawnattr_t attributes;
    posix_spawnattr_init(&attributes);
    sigset_t defaults;
    sigfillset(&defaults);
    sigset_t unblocked;
    sigemptyset(&unblocked);
    short flags = POSIX_SPAWN_SETPGROUP | POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK;
#if defined(__APPLE__)
    flags |= POSIX_SPAWN_CLOEXEC_DEFAULT;
#endif
    posix_spawnattr_setflags(&attributes, flags);
    posix_spawnattr_setpgroup(&attributes, 0);
    posix_spawnattr_setsigdefault(&attributes, &defaults);
    posix_spawnattr_setsigmask(&attributes, &unblocked);

    std::vector<std::string> owned = argv;
    std::vector<char*> args;
    for (std::string& arg : owned) args.push_back(arg.data());
    args.push_back(nullptr);

    pid_t pid = 0;
    const int spawned = posix_spawnp(&pid, args[0], &actions, &attributes, args.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    posix_spawnattr_destroy(&attributes);
    close(fds[1]);
    if (spawned != 0) {
        close(fds[0]);
        error = "could not start " + argv[0] + ": " + std::strerror(spawned);
        return nullptr;
    }

    std::FILE* output = fdopen(fds[0], "r");
    if (output == nullptr) {
        close(fds[0]);
        kill(-pid, SIGTERM);
        waitpid(pid, nullptr, 0);
        error = "could not read the output of " + argv[0];
        return nullptr;
    }
    return std::unique_ptr<Process>(new Process(pid, output));
}

std::unique_ptr<Process> Process::shell(const std::string& command, std::string& error) {
    return start({"/bin/sh", "-c", command}, error);
}

Process::~Process() {
    if (!reaped_) {
        terminate();
        wait();
    }
    if (output_ != nullptr) std::fclose(output_);
}

bool Process::readLine(std::string& line) {
    line.clear();
    std::array<char, 512> buffer{};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), output_)) {
        line += buffer.data();
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
            return true;
        }
    }
    return !line.empty();
}

int Process::wait() {
    if (reaped_) return status_;
    int raw = 0;
    pid_t done = -1;
    do {
        done = waitpid(pid_, &raw, 0);
    } while (done < 0 && errno == EINTR);
    reaped_ = true;
    status_ = done == pid_ && WIFEXITED(raw) ? WEXITSTATUS(raw) : -1;
    return status_;
}

void Process::terminate() {
    if (!reaped_) kill(-pid_, SIGTERM);
}

}
