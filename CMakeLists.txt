cmake_minimum_required(VERSION 3.10)
project(kvstore)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

# Set the directory for generated files
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${GENERATED_DIR})

# Generate protobuf files
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/kvstore.proto)

# Generate gRPC files
set(GRPC_CPP_PLUGIN $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
set(GRPC_SRCS "${GENERATED_DIR}/kvstore.grpc.pb.cc")
set(GRPC_HDRS "${GENERATED_DIR}/kvstore.grpc.pb.h")

add_custom_command(
    OUTPUT "${GRPC_SRCS}" "${GRPC_HDRS}"
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --grpc_out "${GENERATED_DIR}"
        --cpp_out "${GENERATED_DIR}"
        -I "${CMAKE_CURRENT_SOURCE_DIR}/proto"
        --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
        "${CMAKE_CURRENT_SOURCE_DIR}/proto/kvstore.proto"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/proto/kvstore.proto"
)

# Add executable
add_executable(server 
    src/server.cpp 
    src/server_impl.h
    src/server_main.cpp
    ${PROTO_SRCS} 
    ${PROTO_HDRS} 
    ${GRPC_SRCS} 
    ${GRPC_HDRS}
)

# Link libraries
target_link_libraries(server
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
)

# Include directories
target_include_directories(server PRIVATE
    ${GENERATED_DIR}/proto
    ${GENERATED_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${gRPC_INCLUDE_DIRS}
)

# Client executable
add_executable(client 
    src/client.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    ${GRPC_SRCS}
    ${GRPC_HDRS}
)

target_link_libraries(client 
    gRPC::grpc++ 
    gRPC::grpc++_reflection 
    protobuf::libprotobuf
)

# Include directories for client
target_include_directories(client PRIVATE
    ${GENERATED_DIR}/proto
    ${GENERATED_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${gRPC_INCLUDE_DIRS}
) 