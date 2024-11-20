#include <catch2/catch_test_macros.hpp>
#include <clap-rpc/server.hpp>

TEST_CASE("StartStop", "[server]")
{
    auto server = std::make_unique<clap::rpc::Server>("localhost:65187");
    REQUIRE(server->isRunning());
    REQUIRE(server->address() == "localhost");
    REQUIRE(server->port() == 65187);
    REQUIRE(server->uri() == "localhost:65187");
    server->stop();
    REQUIRE(!server->isRunning());
}

TEST_CASE("CreateStreamHandler", "[server]")
{
    auto server = std::make_unique<clap::rpc::Server>();
    REQUIRE(server->isRunning());
    auto handler = server->createStreamHandler();
    server.reset();
}

