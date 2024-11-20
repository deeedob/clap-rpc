#include <clap-rpc/api/clapservice.grpc.pb.h>
#include <clap-rpc/api/clapservice.pb.h>
#include <clap-rpc/server.hpp>
#include <clap-rpc/stream.hpp>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>

CLAP_RPC_BEGIN_NAMESPACE

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

    void shutdown()
    {
        stopWorker();
        for (const auto &handler : mActiveHandlers)
            handler.second.lock()->cancelAll();
        mActiveHandlers.clear();
    }

    std::shared_ptr<StreamHandler> createStreamHandler(Server *server)
    {
        std::shared_ptr<StreamHandler> handler(new StreamHandler(server),
            [this](StreamHandler *ptr) {
                const auto it = std::ranges::find_if(mActiveHandlers, [ptr](const auto &v) {
                    const auto shared = v.second.lock();
                    if (!shared)
                        return true;
                    if (shared.get() == ptr)
                        return true;
                    return false;
                });
                if (it != mActiveHandlers.end()) {
                    std::unique_lock<std::mutex> lock(mActiveHandlersMtx);
                    std::cerr << std::format("erasing leftover handler with id: {}\n", ptr->id());
                    mActiveHandlers.erase(it);
                }
                delete ptr;
            });
        handler->mId = toHash(handler.get());
        std::cout << std::format("Registered stream-handler ID: {}\n", handler->mId);

        std::unique_lock<std::mutex> lock(mActiveHandlersMtx);
        const auto ok = mActiveHandlers.insert({ handler->mId, handler }).second;
        assert(ok && "Failed to register stream handler");
        return handler;
    }

    bool startWorker()
    {
        if (mWorker.joinable())
            return false;

        mWorker = std::jthread([this](std::stop_token stoken) {
            std::cout << "worker initialized\n";
            // TODO: Integrate exponential backoff
            while (!stoken.stop_requested()) {
                std::unique_lock<std::mutex> lock(mWorkerMtx);
                mWorkerCV.wait(lock, stoken, [this] { return mWorkerIsReady.load(); });
                mWorkerIsReady = false;

                api::ServerMessage message;
                for (const auto &handler : mActiveHandlers) {
                    auto sharedHandler = handler.second.lock();
                    while (sharedHandler->mServerQueue.pop(&message)) {
                        sharedHandler->writeMessage(std::move(message));
                        message = api::ServerMessage();
                    }
                }
            }
            std::cout << "worker finished\n";
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
        if (mWorkerIsReady)
            return false;
        mWorkerIsReady = true;
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
        const auto it = mActiveHandlers.find(hashId);
        if (it == mActiveHandlers.end()) {
            return new Stream(context, nullptr,
                {
                    grpc::StatusCode::UNAUTHENTICATED,
                    std::format("plugin_id: '{}' not found", hashId),
                });
        }

        const auto sharedHandler = it->second.lock();
        auto stream = std::make_unique<Stream>(context, sharedHandler, grpc::Status::OK);
        auto *streamPtr = stream.get();
        sharedHandler->connect(std::move(stream));
        return streamPtr;
    }

private:
    std::unordered_map<uint64_t, std::weak_ptr<StreamHandler>> mActiveHandlers;
    std::mutex mActiveHandlersMtx;

    std::jthread mWorker;
    std::mutex mWorkerMtx;
    std::condition_variable_any mWorkerCV;
    std::atomic<bool> mWorkerIsReady{ false };
};

class ServerPrivate
{
public:
    bool running = false;
    std::string address;
    int selectedPort = -1;

    ClapService clapService;
    std::unique_ptr<grpc::Server> server;
    std::mutex serverMtx;
};

Server::Server(std::string_view addressUri)
    : dPtr(std::make_unique<ServerPrivate>())
{
    grpc::ServerBuilder builder;
    builder.AddListeningPort(addressUri.data(), grpc::InsecureServerCredentials(),
        &dPtr->selectedPort);
    builder.RegisterService(&dPtr->clapService);

    dPtr->server = builder.BuildAndStart();
    if (!dPtr->server) {
        std::cerr << "Failed to start server\n";
        return;
    }
    dPtr->address = addressUri.substr(0, addressUri.find_last_of(':'));
    std::cout << std::format("Server listening on URI: {}, Port: {}\n", dPtr->address,
        dPtr->selectedPort);
    dPtr->running = true;
}

Server::~Server()
{
    stop();
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
    if (!dPtr->running)
        return false;
    dPtr->clapService.shutdown();
    dPtr->server->Shutdown();
    dPtr->running = false;
    std::cout << "server stopped\n";
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
