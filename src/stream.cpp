// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Dennis Oberst <dennis.ob@protonmail.com>

#include "logging.h"

#include <clap-rpc/stream.hpp>

#include <grpcpp/server_context.h>

CLAP_RPC_BEGIN_NAMESPACE

Stream::Stream(grpc::CallbackServerContext *context, std::shared_ptr<StreamHandler> handler,
    grpc::Status status)
    : mContext(context), mHandler(std::move(handler))
{
    if (!mHandler || !status.ok()) {
        Finish(std::move(status));
        return;
    }
    StartRead(&mClientMessage);
}

void Stream::StartSharedWrite(std::shared_ptr<api::ServerMessage> response)
{
    bool expected = false;
    if (mIsWriting.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
        mServerMessage = std::move(response);
        StartWrite(mServerMessage.get());
    } else {
        mServerBuffer.emplace(std::move(response));
    }
}

void Stream::Cancel() const
{
    mContext->TryCancel();
}

void Stream::OnDone()
{
    Log(INFO, "stream done: {}", (void *) this);
    if (mHandler) { // in case of early Finish
        if (!mHandler->disconnect(this))
            Log(ERROR, "Failed to disconnect: {}", (void *) this);
    } else {
        delete this;
    }
}

void Stream::OnCancel()
{
    Finish(grpc::Status::CANCELLED);
}

void Stream::OnReadDone(bool ok)
{
    if (!ok) {
        if (mContext->IsCancelled())
            return;
        Finish(grpc::Status::OK);
        return;
    }
    if (!mHandler->mOnReadCallback(*this))
        mHandler->mClientQueue.push(std::move(mClientMessage));
    StartRead(&mClientMessage);
}

void Stream::OnWriteDone(bool ok)
{
    if (!ok) {
        if (mContext->IsCancelled())
            return;
        Finish(grpc::Status::OK);
        return;
    }

    if (!mServerBuffer.empty()) {
        mServerMessage = std::move(mServerBuffer.front());
        mServerBuffer.pop();
        StartWrite(mServerMessage.get());
        return;
    }

    mServerMessage.reset();
    mIsWriting = false;
}

CLAP_RPC_END_NAMESPACE
