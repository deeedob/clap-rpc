if(UNIX)
    if(APPLE)
        set(CLAP_SYSTEM_PATH "/Library/Audio/Plug-Ins/CLAP")
        set(CLAP_USER_PATH "$ENV{HOME}/Library/Audio/Plug-Ins/CLAP")
    else()
        set(CLAP_SYSTEM_PATH "/usr/lib/clap")
        set(CLAP_USER_PATH "$ENV{HOME}/.clap")
    endif()
elseif(WIN32)
    set(CLAP_SYSTEM_PATH "$ENV{COMMONPROGRAMFILES}\\CLAP")
    set(CLAP_USER_PATH "$ENV{LOCALAPPDATA}\\Programs\\Common\\CLAP")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

function(clap_symlink TARGET_NAME)
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

    cmake_parse_arguments(PARSE_ARG "" "GUI_TARGET" "" ${ARGN})
    if (PARSE_ARG_GUI_TARGET)
        # Symlink the GUI executable inside the target directory
        add_custom_command(
            TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                    "$<TARGET_FILE:${PARSE_ARG_GUI_TARGET}>"
                    "${CLAP_USER_PATH}/${TARGET_NAME}/$<TARGET_FILE_NAME:${PARSE_ARG_GUI_TARGET}>"
        )

        # Add a command to output a message indicating the GUI symlink was created
        add_custom_command(
            TARGET create_symlink_${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo
                    "Symlink for GUI target created: ${CLAP_USER_PATH}/${TARGET_NAME}/$<TARGET_FILE_NAME:${PARSE_ARG_GUI_TARGET}>"
        )
    endif()
endfunction()
