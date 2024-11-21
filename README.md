<div align="center">

# clap-rpc

Easily add **R**emote **P**rocedure **C**all(s) to your
[*CLAP*](https://github.com/free-audio/clap) audio plugin(s).

</div>

## Motivation

I wanted a fast and reliable way to enable out-of-process communication for
plugins. This setup allows any executable to connect as a client and start
interacting with the plugin — whether on the same machine or over a private
network. It provides the flexibility to communicate freely with these clients,
which can be written in any of the many [supported
languages](https://grpc.io/docs/languages/)

## Introduction

The [*protobuf*](https://protobuf.dev/) messages in
[*clapservice.proto*](api/clapservice.proto) defines the exposed interface. In
combination with [gRPC](https://grpc.io/), we exchange messages between clients
and plugins through (long-lived) streams. A client can connect to any plugin in
the CLAP bundle. The server runs on the plugin side and should be shared across
plugin instances.


### API Example

```cpp
#include <clap-rpc/server.hpp>

std::unique_ptr<clap::rpc::Server> Server;
inline static size_t PluginCount = 0;

Plugin::Plugin(const clap_host *host)
{
    ++PluginCount;
    if (!Server) // Share the server across plugin instances within the same process
        Server = std::make_unique<clap::rpc::Server>("localhost:50552");
    mStreamHandler = Server->createStreamHandler(); // access streams
}
Plugin::~Plugin()
{
    if (--PluginCount == 0)
        Server.reset();
}

bool activate(double sampleRate, uint32_t minFrames, uint32_t maxFrames) noexcept override
{
    api::ServerMessage message;
    message.mutable_plugin()->set_plugin_api(api::plugin::Server::ACTIVATE);
    auto *act = message.mutable_plugin()->mutable_args()->mutable_activate();
    act->set_sample_rate(sampleRate);
    act->set_min_frames_count(minFrames);
    act->set_max_frames_count(maxFrames);
    mStreamHandler->pushMessage(std::move(message));
    return true;
}
```

## Prerequisite

[gRPC](https://github.com/grpc/grpc/blob/master/BUILDING.md) is a required
dependency of this project.

## Client Implementations

- [qtcleveraudioplugin](https://code.qt.io/cgit/playground/qtcleveraudioplugin.git/about/)
