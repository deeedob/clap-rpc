# SPDX-License-Identifier: MIT
# Copyright (c) 2025 Dennis Oberst <dennis.ob@protonmail.com>

function(clap_symlink TARGET_NAME)
    if(UNIX)
        if(APPLE)
            set(CLAP_USER_PATH "$ENV{HOME}/Library/Audio/Plug-Ins/CLAP")
        else()
            set(CLAP_USER_PATH "$ENV{HOME}/.clap")
        endif()
    elseif(WIN32)
        set(CLAP_USER_PATH "$ENV{LOCALAPPDATA}\\Programs\\Common\\CLAP")
    else()
        message(FATAL_ERROR "Unsupported platform")
    endif()
    # Create directory
    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${CLAP_USER_PATH}/${TARGET_NAME}"
    )

    # Create a custom command to create the symlink
    set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".clap" PREFIX "")
    add_custom_command(
        TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
                "$<TARGET_FILE:${TARGET_NAME}>"
                "${CLAP_USER_PATH}/${TARGET_NAME}/$<TARGET_FILE_NAME:${TARGET_NAME}>"
    )

    add_custom_target(
        create_symlink_${TARGET_NAME} ALL DEPENDS ${TARGET_NAME}
        COMMENT "Creating symlink for ${TARGET_NAME}"
    )

    add_dependencies(create_symlink_${TARGET_NAME} ${TARGET_NAME})

    add_custom_command(
        TARGET create_symlink_${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo
                "Symlink created: ${CLAP_USER_PATH}/$<TARGET_FILE_NAME:${TARGET_NAME}>"
    )

    set(options)
    set(oneValueArgs)
    set(multiValueArgs GUI_TARGETS)
    cmake_parse_arguments(PARSE_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(PARSE_ARG_GUI_TARGETS)
        foreach(GUI_TARGET ${PARSE_ARG_GUI_TARGETS})
            message(STATUS "GUI TARGET: ${GUI_TARGET}")

            add_custom_command(
                TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E create_symlink
                        "$<TARGET_FILE:${GUI_TARGET}>"
                        "${CLAP_USER_PATH}/${TARGET_NAME}/$<TARGET_FILE_NAME:${GUI_TARGET}>"
            )

            add_custom_command(
                TARGET create_symlink_${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo
                        "Symlink for GUI target ${GUI_TARGET} created: ${CLAP_USER_PATH}/${TARGET_NAME}/$<TARGET_FILE_NAME:${GUI_TARGET}>"
            )
        endforeach()
    endif()
endfunction()
