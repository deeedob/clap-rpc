// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Dennis Oberst <dennis.ob@protonmail.com>

#include "executable.hxx"

#include <cstring>
#include <format>
#include <iostream>

#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

CLAP_RPC_BEGIN_NAMESPACE

ExecutablePrivate::~ExecutablePrivate()
{
    kill();
}

bool ExecutablePrivate::exec()
{
    if (!isValid())
        return false;
    mPid = fork();
    if (mPid < 0) {
        std::cerr << "Fork has failed. This shouldn't happen\n";
        return false;
    } else if (mPid == 0) {
        // Child process
        std::string path = absolute(mPath);
        std::vector<char *> argv;
        argv.push_back(const_cast<char *>(path.c_str()));
        for (const auto &arg : mArgs)
            argv.push_back(const_cast<char *>(arg.c_str()));
        argv.push_back(nullptr);
        execv(path.c_str(), argv.data());

        std::cerr << std::format("error code: {}, message: {}\n", errno,
            std::string(strerror(errno)));
        switch (errno) {
        case EACCES:
            mError = Executable::Error::Permission;
            break;
        case ENOENT:
            mError = Executable::Error::NotFound;
            break;
        default:
            mError = Executable::Error::Unknown;
            break;
        }
        return false;
    }
    return true;
}

std::optional<int> ExecutablePrivate::kill() const
{
    if (mPid == -1)
        return {};
    if (::kill(mPid, SIGKILL) < 0)
        return {};

    int status;
    if (waitpid(mPid, &status, 0) == -1) {
        std::cerr << "can't wait for child after kill\n";
        return {};
    }

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        return WTERMSIG(status);
    else if (WIFSTOPPED(status))
        return WSTOPSIG(status);

    std::cout << "Child killed but no status received\n";
    return {};
}

std::optional<int> ExecutablePrivate::updateStatus()
{
    if (mPid == -1)
        return {};
    int status;
    pid_t result = waitpid(mPid, &status, WNOHANG);
    if (result == mPid) {
        if (WIFEXITED(status)) {
            reset();
            mStatus = Executable::Status::Exited;
            return WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            reset();
            mStatus = Executable::Status::Terminated;
            return WTERMSIG(status);
        } else if (WIFSTOPPED(status)) {
            reset();
            mStatus = Executable::Status::Terminated;
            return WSTOPSIG(status);
        }
    } else if (result == 0) {
        mStatus = Executable::Status::Running;
        return {};
    }
    std::cerr << "WNOHANG update failed\n";
    return {};
}

void ExecutablePrivate::reset()
{
    mPid = -1;
    mError = Executable::Error::None;
}

std::optional<Executable::PidType> ExecutablePrivate::pid() const noexcept
{
    if (mPid < 0)
        return {};
    return static_cast<Executable::PidType>(mPid);
}

CLAP_RPC_END_NAMESPACE
