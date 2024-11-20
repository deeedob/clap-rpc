find_package(Catch2 3 QUIET)

if (NOT Catch2_FOUND)
    include(FetchContent)
    message(STATUS "Catch2 not found, fetching it now...")
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras) # ctest
endif()

include(CTest)
include(Catch)

add_test_executable(tst_server DEPENDENCIES clap::rpc)
add_test_executable(tst_executable DEPENDENCIES clap::rpc::tools)
