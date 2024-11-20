#include <algorithm>
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

void StreamHandler::writeMessage(api::ServerMessage &&message)
{
    auto smessage = std::make_shared<api::ServerMessage>(std::move(message));
    for (const auto &stream : mStreams)
        stream->StartSharedWrite(smessage);
}

void StreamHandler::cancelAll()
{
    std::unique_lock<std::mutex> guard(mStreamMtx);
    for (const auto &s : mStreams)
        s->Cancel();
}

void StreamHandler::setOnReadCallback(std::function<bool(const Stream &)> &&callback)
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
    std::unique_lock<std::mutex> lock(mStreamMtx);
    std::cout << std::format("connected: {}\n", (void *) client.get());
    mStreams.emplace(std::move(client));
}

bool StreamHandler::disconnect(Stream *client)
{
    const auto it = std::ranges::find_if(mStreams,
        [client](const auto &p) { return p.get() == client; });
    if (it == mStreams.end())
        return false;
    std::unique_lock<std::mutex> guard(mStreamMtx);
    mStreams.erase(it);
    std::cout << std::format("disconnected: {}\n", (void *) client);
    return true;
}

CLAP_RPC_END_NAMESPACE
