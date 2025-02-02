<div align="center">

# clap-rpc

Easily add **R**emote **P**rocedure **C**all(s) to your
[*CLAP*](https://github.com/free-audio/clap) audio plugin(s).

</div>

## Motivation

I wanted a fast and reliable way to enable out-of-process communication for
plugins. This library allows any client to connect and start interacting with
the plugin, whether on the same machine or over a private network. It provides
an efficient and flexible way for bidirectional communication between a
plugin instance and its clients, which can be written in any of the [supported
client-languages](https://grpc.io/docs/languages/)

## Introduction

The [*protobuf*](https://protobuf.dev/) messages in
[*clapservice.proto*](api/clapservice.proto) defines the exposed interface. In
combination with [gRPC](https://grpc.io/), we exchange messages between clients
and plugins through (long-lived) streams. A client can connect to any plugin in
the CLAP bundle. The server runs on the plugin side and should be shared across
plugin instances.

## Prerequisite

[gRPC](https://github.com/grpc/grpc/blob/master/BUILDING.md) is a required
dependency of this project.

## Client Implementations

- [qtcleveraudioplugin](https://code.qt.io/cgit/playground/qtcleveraudioplugin.git/about/)
