// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Dennis Oberst <dennis.ob@protonmail.com>

#pragma once

#include <clap-rpc/global.hpp>
#include <clap-rpc/streamhandler.hpp>

#include <memory>
#include <string>
#include <string_view>

CLAP_RPC_BEGIN_NAMESPACE

struct ServerConfig
{
    std::string addressUri = "localhost:0";
};

class ServerPrivate;
class Server
{
    struct PrivateTag
    {
        explicit PrivateTag() = default;
    };

public:
    explicit Server(PrivateTag);
    ~Server();

    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;

    Server(Server &&) = delete;
    Server &operator=(Server &&) = delete;

    static std::shared_ptr<Server> uniqueInstance();
    static void configure(ServerConfig config);

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
    inline static std::weak_ptr<Server> sUniqueInstance;

    friend class StreamHandler;
};

CLAP_RPC_END_NAMESPACE
