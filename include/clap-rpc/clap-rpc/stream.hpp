// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Dennis Oberst <dennis.ob@protonmail.com>

#pragma once

#include <clap-rpc/api/clapservice.pb.h>
#include <clap-rpc/global.hpp>
#include <clap-rpc/streamhandler.hpp>

#include <grpcpp/support/server_callback.h>

#include <queue>
#include <atomic>
#include <memory>

CLAP_RPC_BEGIN_NAMESPACE

class Stream final : public grpc::ServerBidiReactor<api::ClientMessage, api::ServerMessage>
{
public:
    explicit Stream(grpc::CallbackServerContext *context, std::shared_ptr<StreamHandler> handler,
        grpc::Status status);

    void StartSharedWrite(std::shared_ptr<api::ServerMessage> response);
    void Cancel() const;

    const api::ClientMessage &clientMessage() const &
    {
        return mClientMessage;
    }

protected:
    void OnDone() override;
    void OnCancel() override;
    void OnReadDone(bool ok) override;
    void OnWriteDone(bool ok) override;

private:
    api::ClientMessage mClientMessage;

    std::shared_ptr<api::ServerMessage> mServerMessage;
    std::queue<std::shared_ptr<api::ServerMessage>> mServerBuffer;
    std::atomic<bool> mIsWriting = false;

    grpc::CallbackServerContext *mContext;
    std::shared_ptr<StreamHandler> mHandler;
};

CLAP_RPC_END_NAMESPACE
