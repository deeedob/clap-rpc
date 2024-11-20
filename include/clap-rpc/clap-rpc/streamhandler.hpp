#pragma once

#include <clap-rpc/api/clapservice.pb.h>
#include <clap-rpc/global.hpp>
#include <clap-rpc/mpmcqueue.hpp>

#include <functional>
#include <memory>
#include <mutex>
#include <set>

CLAP_RPC_BEGIN_NAMESPACE

class Server;
class Stream;

class StreamHandler : public std::enable_shared_from_this<StreamHandler>
{
    using ClientQueue = MpMcQueue<api::ClientMessage, 256>;
    using ServerQueue = MpMcQueue<api::ServerMessage, 256>;

public:
    ~StreamHandler();
    // TODO: SMFs

    [[nodiscard]] uint64_t id() const noexcept
    {
        return mId;
    }
    [[nodiscard]] size_t numStreams() const noexcept
    {
        return mStreams.size();
    }
    void cancelAll();

    void pushMessage(api::ServerMessage &&response);
    void pushMessage(const api::ServerMessage &response);
    void writeMessage(api::ServerMessage &&message);

    void setOnReadCallback(std::function<bool(const Stream &)> &&callback);
    bool tryPop(api::ClientMessage *message);
    api::ClientMessage pop();

private:
    explicit StreamHandler(Server *server);
    void connect(std::unique_ptr<Stream> &&client);
    bool disconnect(Stream *client);

private:
    uint64_t mId = 0;
    std::set<std::unique_ptr<Stream>> mStreams;
    std::mutex mStreamMtx;

    ClientQueue mClientQueue;
    ServerQueue mServerQueue;
    std::function<bool(const Stream &)> mOnReadCallback;

    Server *mServer;

    friend class Stream;
    friend class ClapService;
};

CLAP_RPC_END_NAMESPACE
