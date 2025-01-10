#include "logging.h"

#include <clap-rpc/server.hpp>
#include <clap-rpc/stream.hpp>
#include <clap-rpc/streamhandler.hpp>

#include <thread>

CLAP_RPC_BEGIN_NAMESPACE

StreamHandler::StreamHandler(Server *server)
    : mOnReadCallback([](const Stream &) { return false; }), mServer(server)
{
}

StreamHandler::~StreamHandler()
{
    // On successfull shutdown with a single stream left the deleter for the
    // StreamHandler may interfere with a prior disconnect call as it would
    // release the last shared_ptr instance of this but still hold the
    // mStreamMtx.
    std::shared_lock<std::shared_mutex> lock(mSharedStreamsMtx, std::defer_lock);
    if (lock.try_lock())
        cancelAll();
}

void StreamHandler::pushMessage(api::ServerMessage &&response)
{
    mServerQueue.push(std::move(response));
    mServer->tryNotify();
}

void StreamHandler::pushMessage(const api::ServerMessage &response)
{
    mServerQueue.push(response);
    mServer->tryNotify();
}

void StreamHandler::broadcast(api::ServerMessage &&message)
{
    auto smessage = std::make_shared<api::ServerMessage>(std::move(message));
    std::shared_lock<std::shared_mutex> lock(mSharedStreamsMtx);
    for (const auto &stream : mStreams)
        stream->StartSharedWrite(smessage);
}

void StreamHandler::cancelAll() const
{
    std::shared_lock<std::shared_mutex> lock(mSharedStreamsMtx);
    for (const auto &s : mStreams)
        s->Cancel();
}

void StreamHandler::setInterceptor(std::function<bool(const Stream &)> &&callback)
{
    mOnReadCallback = std::move(callback);
}

bool StreamHandler::tryPop(api::ClientMessage *message)
{
    return mClientQueue.pop(message);
}

api::ClientMessage StreamHandler::pop()
{
    // TODO: exponential backoff?
    api::ClientMessage message;
    while (!tryPop(&message))
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    return message;
}

void StreamHandler::connect(std::unique_ptr<Stream> &&client)
{
    std::unique_lock<std::shared_mutex> lock(mSharedStreamsMtx);
    Log(INFO, "connected: {}", (void *) client.get());
    mStreams.emplace(std::move(client));
}

bool StreamHandler::disconnect(Stream *client)
{
    std::unique_lock guard(mSharedStreamsMtx);
    const auto it = std::ranges::find_if(mStreams,
        [client](const auto &p) { return p.get() == client; });
    if (it == mStreams.end())
        return false;
    mStreams.erase(it);
    Log(INFO, "disconnected: {}", (void *) client);
    return true;
}

CLAP_RPC_END_NAMESPACE
