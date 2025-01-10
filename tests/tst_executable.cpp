// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Dennis Oberst <dennis.ob@protonmail.com>

#include <catch2/catch_test_macros.hpp>
#include <clap-rpc/tools/executable.hpp>

#include <filesystem>
#include <fstream>
#include <thread>
#include <iostream>

namespace fs = std::filesystem;

class FileWriter
{
public:
    FileWriter(std::string_view path_)
        : path(path_)
    {
        file.open(path);
        file.flush();
    }

    ~FileWriter()
    {
        std::filesystem::remove(path);
    }

    void addPermission()
    {
        fs::permissions(path,
            fs::perms::owner_all | fs::perms::group_all, fs::perm_options::add);
    }

    void writeSimple()
    {
        file << "#!/bin/bash\n";
        file << "echo 'Hello World'";
        file.flush();
    }

    void writeDelayed(int sleep, int exit)
    {
        file << "#!/bin/bash\n";
        file << "echo 'write Delayed'\n";
        file << "sleep " << sleep << "\n";
        file << "exit " << exit << "\n";
        file.flush();
    }

    void writeTrap(int exit)
    {
        file << "#!/bin/bash\n";
        file << "handle_sigterm() { echo 'Received SIGTERM signal'; exit " << exit << "; }\n";
        file << "trap 'handle_sigterm' SIGTERM\n";
        file << "while true; do sleep 1; done\n";
        file.flush();
    }

    std::string path;
    std::ofstream file;
};

TEST_CASE("basic", "[executable]")
{
    using namespace clap::rpc;
    FileWriter data("basic.sh");
    data.writeSimple();

    Executable ex(data.path);
    REQUIRE(!ex.isValid());
    REQUIRE(!ex.pid());
    REQUIRE(ex.path() == data.path);
    REQUIRE(ex.status() == Executable::Status::Init);

    data.file.close();
    data.addPermission();

    REQUIRE(ex.exec());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(ex.updateStatus());
    REQUIRE(ex.status() == Executable::Status::Exited);
    REQUIRE(!ex.kill());
}

TEST_CASE("waitForExit", "[executable]")
{
    using namespace clap::rpc;
    FileWriter data("delayed.sh");
    data.writeDelayed(4, 100);
    data.addPermission();

    Executable ex(data.path);

    REQUIRE(ex.exec());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE(!ex.updateStatus());
    REQUIRE(ex.status() == Executable::Status::Running);

    REQUIRE(ex.kill());
}
