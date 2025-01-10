// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Dennis Oberst <dennis.ob@protonmail.com>

#pragma once

#include <clap-rpc/api/clapservice.grpc.pb.h>
#include <clap-rpc/global.hpp>

#include <clap/clap.h>

CLAP_RPC_BEGIN_NAMESPACE

class TransportWatcher
{
public:
    TransportWatcher()
        : mRef(mResponse.mutable_event()->mutable_event()->mutable_transport())
    {
    }

    bool enabled() const noexcept
    {
        return mEnabled;
    }
    void setEnabled(bool value)
    {
        mEnabled = value;
    }

    bool update(const clap_event_transport *other);
    const api::ServerMessage &message() const &
    {
        return mResponse;
    }

    [[nodiscard]] bool isPlaying() const noexcept;
    [[nodiscard]] bool isRecording() const noexcept;
    [[nodiscard]] bool isLoopActive() const noexcept;
    [[nodiscard]] bool isWithinPreRoll() const noexcept;

    bool equalsPosition(const clap_event_transport *other) const;
    bool equalsTempo(const clap_event_transport *other) const;
    bool equalsLoop(const clap_event_transport *other) const;
    bool equalsTimeSignature(const clap_event_transport *other) const;

private:
    std::atomic<bool> mEnabled = false;
    api::ServerMessage mResponse;
    api::event::Transport *mRef;
};

inline bool TransportWatcher::update(const clap_event_transport *other)
{
    if (!other || !mEnabled)
        return false;

    uint32_t updatedFields = 0;
    if (mRef->flags() != other->flags) {
        mRef->set_flags(other->flags);
        updatedFields |= 1u;
    }
    if (!equalsPosition(other)) {
        mRef->mutable_position()->set_beats(other->song_pos_beats);
        mRef->mutable_position()->set_seconds(other->song_pos_seconds);
        updatedFields |= 2u;
    }
    if (!equalsTempo(other)) {
        mRef->mutable_tempo()->set_value(other->tempo);
        mRef->mutable_tempo()->set_increment(other->tempo_inc);
        updatedFields |= 4u;
    }
    if (!equalsLoop(other)) {
        mRef->mutable_loop()->set_start_beats(other->loop_start_beats);
        mRef->mutable_loop()->set_end_beats(other->loop_end_beats);
        mRef->mutable_loop()->set_start_seconds(other->loop_start_seconds);
        mRef->mutable_loop()->set_end_seconds(other->loop_end_seconds);
        updatedFields |= 8u;
    }
    if (!equalsTimeSignature(other)) {
        mRef->mutable_time_signature()->set_numerator(other->tsig_num);
        mRef->mutable_time_signature()->set_denominator(other->tsig_denom);
        updatedFields |= 16u;
    }

    // TODO: Decide to make this configurable.
    const auto nFields = __builtin_popcount(updatedFields);
    return nFields > 0;
}

inline bool TransportWatcher::isPlaying() const noexcept
{
    return (mRef->flags() & CLAP_TRANSPORT_IS_PLAYING) != 0;
}

inline bool TransportWatcher::isRecording() const noexcept
{
    return (mRef->flags() & CLAP_TRANSPORT_IS_RECORDING) != 0;
}

inline bool TransportWatcher::isLoopActive() const noexcept
{
    return (mRef->flags() & CLAP_TRANSPORT_IS_LOOP_ACTIVE) != 0;
}

inline bool TransportWatcher::isWithinPreRoll() const noexcept
{
    return (mRef->flags() & CLAP_TRANSPORT_IS_WITHIN_PRE_ROLL) != 0;
}

inline bool TransportWatcher::equalsPosition(const clap_event_transport *other) const
{
    return other && mRef->mutable_position()->beats() == other->song_pos_beats
        && mRef->mutable_position()->seconds() == other->song_pos_seconds;
}
inline bool TransportWatcher::equalsTempo(const clap_event_transport *other) const
{
    return other && mRef->mutable_tempo()->value() == other->tempo
        && mRef->mutable_tempo()->increment() == other->tempo_inc;
}

inline bool TransportWatcher::equalsLoop(const clap_event_transport *other) const
{
    return other && mRef->mutable_loop()->start_beats() == other->loop_start_beats
        && mRef->mutable_loop()->end_beats() == other->loop_end_beats
        && mRef->mutable_loop()->start_seconds() == other->loop_start_seconds
        && mRef->mutable_loop()->end_seconds() == other->loop_end_seconds;
}

inline bool TransportWatcher::equalsTimeSignature(const clap_event_transport *other) const
{
    return other && mRef->mutable_time_signature()->numerator() == other->tsig_num
        && mRef->mutable_time_signature()->denominator() == other->tsig_denom;
}

CLAP_RPC_END_NAMESPACE
