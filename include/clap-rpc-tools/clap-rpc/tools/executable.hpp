#pragma once

#include <clap-rpc/global.hpp>

#include <memory>
#include <optional>
#include <string_view>

CLAP_RPC_BEGIN_NAMESPACE

class ExecutablePrivate;

class Executable
{
public:
    using PidType = unsigned long;
    enum class Error { None, NotFound, Permission, Unknown };
    enum class Status { Init, Exited, Terminated, Running };

    Executable();
    explicit Executable(std::string_view path, std::initializer_list<std::string_view> args = {});
    ~Executable();

    Executable(const Executable &) = delete;
    Executable &operator=(const Executable &) = delete;

    Executable(Executable &&) noexcept;
    Executable &operator=(Executable &&) noexcept;

    [[nodiscard]] bool isValid();
    [[nodiscard]] std::optional<PidType> pid() const noexcept;

    Executable &setPath(std::string_view path);
    [[nodiscard]] std::string path() const noexcept;

    Executable &setArgs(std::initializer_list<std::string_view> args);
    [[nodiscard]] const std::vector<std::string> &args() const noexcept;

    std::optional<int> updateStatus();
    [[nodiscard]] Status status() const noexcept;
    [[nodiscard]] Error error() const noexcept;

    [[nodiscard]] bool exec();
    [[nodiscard]] std::optional<int> kill() const;

private:
    std::unique_ptr<ExecutablePrivate> dPtr;
};

CLAP_RPC_END_NAMESPACE
