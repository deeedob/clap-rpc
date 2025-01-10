// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Dennis Oberst <dennis.ob@protonmail.com>

#include "executable.hxx"

#include <algorithm>

CLAP_RPC_BEGIN_NAMESPACE

Executable::Executable()
    : dPtr(std::make_unique<ExecutablePrivate>())
{
}

Executable::Executable(std::string_view path, std::initializer_list<std::string_view> args)
    : dPtr(std::make_unique<ExecutablePrivate>(path, args))
{
}

Executable::~Executable() = default;

Executable::Executable(Executable &&) noexcept = default;

Executable &Executable::operator=(Executable &&) noexcept = default;

bool Executable::isValid()
{
    return dPtr->isValid();
}

bool Executable::exec()
{
    return dPtr->exec();
}

std::optional<int> Executable::kill() const
{
    return dPtr->kill();
}

Executable &Executable::setPath(std::string_view path)
{
    if (dPtr->mPath != path)
        dPtr->mPath = std::filesystem::path(path);
    return *this;
}

Executable &Executable::setArgs(std::initializer_list<std::string_view> args)
{
    if (!std::ranges::equal(dPtr->mArgs, args))
        dPtr->mArgs = std::vector<std::string>(args.begin(), args.end());
    return *this;
}

std::optional<Executable::PidType> Executable::pid() const noexcept
{
    return dPtr->pid();
}

std::string Executable::path() const noexcept
{
    return dPtr->mPath;
}

const std::vector<std::string> &Executable::args() const noexcept
{
    return dPtr->mArgs;
}

std::optional<int> Executable::updateStatus()
{
    return dPtr->updateStatus();
}

Executable::Status Executable::status() const noexcept
{
    return dPtr->mStatus;
}

Executable::Error Executable::error() const noexcept
{
    return dPtr->mError;
}


CLAP_RPC_END_NAMESPACE
