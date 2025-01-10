cmake_minimum_required(VERSION 3.27)

project(
    clap-rpc
    VERSION 0.1
    LANGUAGES CXX
)

# OPTIONS
option(clap-rpc_BUILD_TESTS "Build tests" OFF)
option(clap-rpc_BUILD_EXAMPLES "Build examples" OFF)
option(WARNINGS_ARE_ERRORS "Error on Warning" OFF)
option(BUILD_SHARED_LIBS "Build libraries as shared" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COMPILE_WARNING_AS_ERROR ${WARNINGS_ARE_ERRORS})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(clap-rpc)
add_library(clap::rpc ALIAS clap-rpc)
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

add_library(clap-rpc-tools)
add_library(clap::rpc::tools ALIAS clap-rpc-tools)
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
    message(WARNING "Windows support is untested")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
add_subdirectory(3rdparty/clap)

target_link_libraries(clap-rpc PUBLIC clap protobuf::libprotobuf gRPC::grpc++)
target_link_libraries(clap-rpc-tools PUBLIC clap-rpc)

target_include_directories(clap-rpc
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:include>
)

target_include_directories(clap-rpc-tools
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:include>
)

# Protobuf generation
include(cmake/initialize_proto.cmake)
initialize_proto(clap-rpc
    IMPORT_DIR
        "api"
    GRPC
        "api/clapservice.proto"
    PROTO
        "api/clapservice.proto"
        "api/plugin.proto"
        "api/event.proto"
        "api/host.proto"
        "api/gui.proto"
)

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

if(${clap-rpc_BUILD_TESTS})
    include(cmake/utils.cmake)
    add_subdirectory(tests/)
endif()

if(${clap-rpc_BUILD_EXAMPLES})
    # add_subdirectory(examples/)
endif()

include(GNUInstallDirs)
install(TARGETS clap-rpc clap-rpc-tools
    EXPORT clap-rpc-config
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT clap-rpc-config
    NAMESPACE clap::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clap-rpc
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/clap-rpc-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/clap-rpc-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clap-rpc
)
export(EXPORT clap-rpc-config
    NAMESPACE clap::
    FILE ${CMAKE_BINARY_DIR}/clap-rpc-config.cmake
)
