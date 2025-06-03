cmake_minimum_required(VERSION 3.10)
project(SpdkKeyValueStore)

set(CMAKE_CXX_STANDARD 14)

# Add gRPC as a subdirectory
add_subdirectory(third_party/grpc)

# Find required packages
find_package(Protobuf REQUIRED)

# Set proto file path
set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/proto/kvstore.proto")

# Set generated files directory
set(GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})

# Generate protobuf and gRPC files
add_custom_command(
    OUTPUT 
        "${GENERATED_DIR}/kvstore.pb.cc"
        "${GENERATED_DIR}/kvstore.pb.h"
        "${GENERATED_DIR}/kvstore.grpc.pb.cc"
        "${GENERATED_DIR}/kvstore.grpc.pb.h"
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --grpc_out="${GENERATED_DIR}"
         --cpp_out="${GENERATED_DIR}"
         -I "${CMAKE_CURRENT_SOURCE_DIR}/proto"
         --plugin=protoc-gen-grpc="${_gRPC_CPP_PLUGIN_EXECUTABLE}"
         "${PROTO_FILE}"
    DEPENDS "${PROTO_FILE}"
)

# Include directories
include_directories(
    ${GENERATED_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/proto
)

# Server executable
add_executable(server 
    src/server.cpp
    "${GENERATED_DIR}/kvstore.pb.cc"
    "${GENERATED_DIR}/kvstore.grpc.pb.cc"
)
target_link_libraries(server 
    gRPC::grpc++ 
    gRPC::grpc++_reflection 
    protobuf::libprotobuf
)

# Client executable
add_executable(client 
    src/client.cpp
    "${GENERATED_DIR}/kvstore.pb.cc"
    "${GENERATED_DIR}/kvstore.grpc.pb.cc"
)
target_link_libraries(client 
    gRPC::grpc++ 
    gRPC::grpc++_reflection 
    protobuf::libprotobuf
) 