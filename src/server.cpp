// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Dennis Oberst <dennis.ob@protonmail.com>

#include "logging.h"

#include <clap-rpc/api/clapservice.grpc.pb.h>
#include <clap-rpc/api/clapservice.pb.h>
#include <clap-rpc/server.hpp>
#include <clap-rpc/stream.hpp>

#include <google/protobuf/message.h>
#include <grpc/support/time.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include <algorithm>
#include <condition_variable>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <utility>

CLAP_RPC_BEGIN_NAMESPACE

using namespace std::chrono_literals;

namespace {
uint64_t toHash(const void *ptr)
{
    auto h = reinterpret_cast<uint64_t>(ptr);
    h ^= h >> 33;
    h *= 0xff'51'af'd7'ed'55'8c'cd;
    h ^= h >> 33;
    h *= 0xc4'ce'b9'fe'1a'85'ec'53;
    return h ^ (h >> 33);
}
} // namespace

class ClapService final : public api::ClapService::CallbackService
{
public:
    ClapService()
    {
        assert(startWorker() && "Couldn't start worker");
    }
    ~ClapService() override
    {
        stopWorker();
    }

    ClapService(const ClapService &) = delete;
    ClapService &operator=(const ClapService &) = delete;

    ClapService(ClapService &&) = delete;
    ClapService &operator=(ClapService &&) = delete;

    std::shared_ptr<StreamHandler> createStreamHandler(Server *server)
    {
        auto deleter = [this](StreamHandler *ptr) {
            std::shared_lock readLock(mSharedHandlersMtx);
            const auto it = std::ranges::find_if(mActiveHandlers, [ptr](const auto &v) {
                if (auto shared = v.second.lock(); !shared || shared.get() == ptr)
                    return true;
                return false;
            });
            if (it != mActiveHandlers.end()) {
                readLock.unlock();
                std::unique_lock writeLock(mSharedHandlersMtx);
                mActiveHandlers.erase(it);
            }
            delete ptr;
        };

        std::shared_ptr<StreamHandler> handler(new StreamHandler(server), deleter);
        handler->mId = toHash(handler.get());
        Log(INFO, "Registered unique plugin ID: {}", handler->mId);

        std::unique_lock writeLock(mSharedHandlersMtx);
        mActiveHandlers.insert({ handler->mId, handler });

        return handler;
    }

    bool startWorker()
    {
        if (mWorker.joinable())
            return false;

        mWorker = std::jthread([this](std::stop_token stoken) {
            Log(DEBUG, "worker thread initialized");
            // TODO: Integrate exponential backoff ? atomic_flag spin-wait?
            while (!stoken.stop_requested()) {
                std::unique_lock<std::mutex> waitMtx(mWorkerMtx);
                mWorkerCV.wait(waitMtx, stoken, [this] { return mWorkerIsReady.load(); });
                mWorkerIsReady = false;

                api::ServerMessage message;

                std::shared_lock readLock(mSharedHandlersMtx);
                for (const auto &handler : mActiveHandlers) {
                    auto sharedHandler = handler.second.lock();
                    if (!sharedHandler)
                        continue;
                    while (sharedHandler->mServerQueue.pop(&message)) {
                        sharedHandler->broadcast(std::move(message));
                        message = api::ServerMessage();
                    }
                }
            }
            Log(DEBUG, "worker thread finished");
        });

        return true;
    }

    bool stopWorker()
    {
        if (!mWorker.joinable())
            return false;
        mWorker.request_stop();
        mWorker.join();
        return true;
    }

    bool tryNotifyWorker()
    {
        bool expected = false;
        if (!mWorkerIsReady.compare_exchange_strong(expected, true, std::memory_order_acquire))
            return false;
        // TODO: This is not realtime safe as it may involve an OS call...
        // https://timur.audio/using-locks-in-real-time-audio-processing-safely
        mWorkerCV.notify_one();
        return true;
    }

protected:
    grpc::ServerBidiReactor<api::ClientMessage, api::ServerMessage> *
        EventStream(grpc::CallbackServerContext *context) override
    {
        // a new connection must provide a plugin_id
        const auto metadata = context->client_metadata();
        const auto metaPlugId = metadata.find("plugin_id");
        if (metaPlugId == metadata.end()) {
            return new Stream(context, nullptr,
                {
                    grpc::StatusCode::UNAUTHENTICATED,
                    "No plugin_id found in metadata",
                });
        }

        const auto hashId = std::stoull(std::string(metaPlugId->second.data(),
            metaPlugId->second.length()));

        std::shared_lock readLock(mSharedHandlersMtx);
        const auto it = mActiveHandlers.find(hashId);
        if (it == mActiveHandlers.end()) {
            return new Stream(context, nullptr,
                {
                    grpc::StatusCode::UNAUTHENTICATED,
                    std::format("plugin_id: '{}' not found", hashId),
                });
        }
        readLock.unlock();

        const auto sharedHandler = it->second.lock();
        auto stream = std::make_unique<Stream>(context, sharedHandler, grpc::Status::OK);
        auto *streamPtr = stream.get();
        sharedHandler->connect(std::move(stream));
        return streamPtr;
    }

private:
    std::unordered_map<uint64_t, std::weak_ptr<StreamHandler>> mActiveHandlers;
    std::shared_mutex mSharedHandlersMtx;

    std::jthread mWorker;
    std::mutex mWorkerMtx;
    std::condition_variable_any mWorkerCV;
    std::atomic<bool> mWorkerIsReady{ false };
};

static std::mutex sUniqueInstanceMtx = {};
static ServerConfig sServerConfig = {};

class ServerPrivate
{
public:
    explicit ServerPrivate()
    {
        grpc::ServerBuilder builder;
        builder.AddListeningPort(sServerConfig.addressUri, grpc::InsecureServerCredentials(),
            &selectedPort);
        builder.RegisterService(&clapService);

        server = builder.BuildAndStart();
        if (!server) {
            Log(ERROR, "Server start failed");
            return;
        }
        address = sServerConfig.addressUri.substr(0, sServerConfig.addressUri.find_last_of(':'));
        Log(INFO, "Server listening on URI: {}, Port: {}", address, selectedPort);
        running = true;
    }

    std::atomic<bool> running = false;
    std::string address;
    int selectedPort = -1;

    ClapService clapService;
    std::unique_ptr<grpc::Server> server;
};

Server::Server(PrivateTag)
    : dPtr(std::make_unique<ServerPrivate>())
{
}

Server::~Server()
{
    stop();
}

std::shared_ptr<Server> Server::uniqueInstance()
{
    std::scoped_lock<std::mutex> lock(sUniqueInstanceMtx);
    auto sharedInstance = sUniqueInstance.lock();
    if (!sharedInstance) {
        sharedInstance = std::make_shared<Server>(PrivateTag{});
        sUniqueInstance = sharedInstance;
    }
    return sharedInstance;
}

void Server::configure(ServerConfig config)
{
    sServerConfig = std::move(config);
}

bool Server::isRunning() const noexcept
{
    return dPtr->running;
}

std::string_view Server::address() const noexcept
{
    return dPtr->address;
}

int Server::port() const noexcept
{
    return dPtr->selectedPort;
}

std::string Server::uri() const
{
    return dPtr->address + ':' + std::to_string(dPtr->selectedPort);
}

bool Server::stop()
{
    bool expected = false;
    if (!dPtr->running.compare_exchange_strong(expected, true, std::memory_order_acquire))
        return false;

    dPtr->clapService.stopWorker();
    dPtr->server->Shutdown();
    Log(DEBUG, "server stopped");
    return true;
}

std::shared_ptr<StreamHandler> Server::createStreamHandler()
{
    return dPtr->clapService.createStreamHandler(this);
}

bool Server::tryNotify()
{
    return dPtr->clapService.tryNotifyWorker();
}

CLAP_RPC_END_NAMESPACE
