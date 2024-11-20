# https://github.com/protocolbuffers/protobuf/blob/main/docs/cmake_protobuf_generate.md

function(initialize_proto target)

    cmake_parse_arguments(args "" "IMPORT_DIR" "PROTO;GRPC" ${ARGN})
    if(NOT args_IMPORT_DIR)
        message(FATAL_ERROR "IMPORT_DIR argument is required.")
    endif()

    set(PROTO_OUT "${CMAKE_CURRENT_BINARY_DIR}/include/clap-rpc/api")
    set(PROTO_OUT_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/include")

    target_include_directories(
        ${target} PUBLIC "$<BUILD_INTERFACE:${PROTO_OUT_INCLUDE}>"
    )

    set(generated_files "")
    if(args_PROTO)
        protobuf_generate(
            TARGET
                ${target}
            PROTOS
                ${args_PROTO}
            PROTOC_OUT_DIR
                ${PROTO_OUT}
            IMPORT_DIRS
                ${args_IMPORT_DIR}
        )
    endif()

    if(args_GRPC)
        protobuf_generate(
            TARGET
                ${target}
            PROTOS
                ${args_GRPC}
            PROTOC_OUT_DIR
                ${PROTO_OUT}
            IMPORT_DIRS
                ${args_IMPORT_DIR}
            LANGUAGE
                grpc
            GENERATE_EXTENSIONS
                .grpc.pb.h
                .grpc.pb.cc
            PLUGIN
                "protoc-gen-grpc=\$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
        )
    endif()

    message(STATUS "Generated proto interface for target ${target}")
endfunction()

