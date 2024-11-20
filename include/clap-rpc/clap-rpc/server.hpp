#pragma once

#include <clap-rpc/global.hpp>
#include <clap-rpc/streamhandler.hpp>

#include <memory>
#include <string_view>

CLAP_RPC_BEGIN_NAMESPACE

class ServerPrivate;
class Server
{
public:
    explicit Server(std::string_view addressUri = "localhost:0");
    ~Server();

    Server(const Server &) = delete;
    Server(Server &&) = delete;

    Server &operator=(const Server &) = delete;
    Server &operator=(Server &&) = delete;

    [[nodiscard]] bool isRunning() const noexcept;
    [[nodiscard]] std::string_view address() const noexcept;
    [[nodiscard]] int port() const noexcept;
    [[nodiscard]] std::string uri() const;

    [[nodiscard]] std::shared_ptr<StreamHandler> createStreamHandler();

    bool stop();

private:
    bool tryNotify();

private:
    std::unique_ptr<ServerPrivate> dPtr;

    friend class StreamHandler;
};

CLAP_RPC_END_NAMESPACE
