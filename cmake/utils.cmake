# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Dennis Oberst <dennis.ob@protonmail.com>

function(add_test_executable name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain)

    set(flags)
    set(args)
    set(listArgs DEPENDENCIES)
    cmake_parse_arguments(
        arg
        "${flags}"
        "${args}"
        "${listArgs}"
        ${ARGN}
    )

    # Optional libraries to link against (if provided)
    if(arg_DEPENDENCIES)
        target_link_libraries(${name} PUBLIC ${arg_DEPENDENCIES})
        add_dependencies(${name} ${arg_DEPENDENCIES})
    endif()
    catch_discover_tests(${name})
    message(STATUS "Added test executable: ${name}")
endfunction()

function(print_target_info target_name)
    get_target_property(target_type ${target_name} TYPE)
    get_target_property(target_sources ${target_name} SOURCES)
    get_target_property(target_include_dirs ${target_name} INCLUDE_DIRECTORIES)
    get_target_property(target_compile_options ${target_name} COMPILE_OPTIONS)
    get_target_property(
        target_compile_definitions ${target_name} COMPILE_DEFINITIONS
    )
    get_target_property(target_link_libraries ${target_name} LINK_LIBRARIES)
    get_target_property(
        target_dependencies ${target_name} INTERFACE_LINK_LIBRARIES
    )

    message(STATUS "Target Type: ${target_type}")
    message(STATUS "Sources: ${target_sources}")
    message(STATUS "Include Directories: ${target_include_dirs}")
    message(STATUS "Compile Options: ${target_compile_options}")
    message(STATUS "Compile Definitions: ${target_compile_definitions}")
    message(STATUS "Link Libraries: ${target_link_libraries}")
    message(STATUS "Dependencies: ${target_dependencies}")
endfunction()

