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
    // destroys this object
    if (mHandler) {
        if (!mHandler->disconnect(this))
            std::cout << std::format("Failed to disconnect\n", (void *) this);
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
    this->StartRead(&mClientMessage);
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
        std::cout << std::format("Write from response buffer, size {}\n", mServerBuffer.size());
        StartWrite(mServerMessage.get());
        return;
    }

    mServerMessage.reset();
    mIsWriting = false;
}

CLAP_RPC_END_NAMESPACE
