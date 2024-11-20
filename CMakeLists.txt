cmake_minimum_required(VERSION 3.27)

project(
    clap-rpc
    VERSION 0.1
    LANGUAGES CXX
)

message(
    STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}, v${CMAKE_CXX_COMPILER_VERSION}"
)

# OPTIONS
option(clap-rpc_BUILD_TESTS "Build tests" OFF)
option(clap-rpc_BUILD_EXAMPLES "Build examples" OFF)
option(WARNINGS_ARE_ERRORS "Error on Warning" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ${WARNINGS_ARE_ERRORS})

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_OBJC_VISIBILITY_PRESET hidden)
set(CMAKE_OBJCXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

include(cmake/utils.cmake)
include(cmake/initialize_proto.cmake)
include(cmake/clap_autolink.cmake)

# Library targets
add_library(clap-rpc STATIC)
target_sources(clap-rpc
    PRIVATE
        src/server.cpp
        src/stream.cpp
        src/streamhandler.cpp
    PUBLIC FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}/include/clap-rpc
    FILES
        include/clap-rpc/clap-rpc/global.hpp
        include/clap-rpc/clap-rpc/server.hpp
        include/clap-rpc/clap-rpc/stream.hpp
        include/clap-rpc/clap-rpc/streamhandler.hpp
)

add_library(clap-rpc-tools STATIC)
target_sources(clap-rpc-tools
    PRIVATE
        src/tools/executable.hxx
        src/tools/executable.cpp
    PUBLIC FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}/include/clap-rpc-tools
    FILES
        include/clap-rpc-tools/clap-rpc/tools/executable.hpp
        include/clap-rpc-tools/clap-rpc/tools/transportwatcher.hpp
)

if(UNIX)
    target_sources(clap-rpc-tools PRIVATE src/tools/executable_unix.cpp)
elseif(WIN32)
    message(FATAL_ERROR "Not implemented!")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

add_library(clap::rpc ALIAS clap-rpc)
add_library(clap::rpc::tools ALIAS clap-rpc-tools)

target_link_libraries(clap-rpc PUBLIC clap protobuf::libprotobuf gRPC::grpc++)
target_link_libraries(clap-rpc-tools PUBLIC clap-rpc)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
add_subdirectory(3rdparty/clap/)

# Protobuf generation
initialize_proto(clap-rpc
    IMPORT_DIR
        "api"
    GRPC
        "api/clapservice.proto"
    PROTO
        "api/clapservice.proto"
        "api/plugin.proto"
        "api/events.proto"
        "api/host.proto"
        "api/gui.proto"
)

# Warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
        clap-rpc
        PRIVATE -Wall
                -Wshadow
                -Wnon-virtual-dtor
                -Wcast-align
                -Wunused
                -Wpedantic
                -Wsign-conversion
                -Wnull-dereference
                -Wdouble-promotion
                -Wformat=2
                -Wimplicit-fallthrough
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(clap-rpc PRIVATE /W4 /WX /wd4100 /wd4702)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sanitizers-cmake/cmake"
                      ${CMAKE_MODULE_PATH}
)
find_package(Sanitizers REQUIRED)
add_sanitizers(clap-rpc)

if(${clap-rpc_BUILD_TESTS})
    add_subdirectory(tests/)
endif()

if(${clap-rpc_BUILD_EXAMPLES})
    # add_subdirectory(examples/)
endif()
