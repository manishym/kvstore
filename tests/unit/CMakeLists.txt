cmake_minimum_required(VERSION 3.10)
project(KeyValueStoreTests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set project root
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

# Find required packages
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# Generate proto files
set(PROTO_FILES
    ${PROJECT_ROOT}/proto/kvstore.proto
)

# Generate gRPC files
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/kvstore.pb.cc"
           "${CMAKE_CURRENT_BINARY_DIR}/kvstore.pb.h"
           "${CMAKE_CURRENT_BINARY_DIR}/kvstore.grpc.pb.cc"
           "${CMAKE_CURRENT_BINARY_DIR}/kvstore.grpc.pb.h"
    COMMAND protobuf::protoc
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
         -I "${PROJECT_ROOT}/proto"
         --plugin=protoc-gen-grpc="$<TARGET_FILE:gRPC::grpc_cpp_plugin>"
         "${PROJECT_ROOT}/proto/kvstore.proto"
    DEPENDS "${PROJECT_ROOT}/proto/kvstore.proto"
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_ROOT}
    ${PROJECT_ROOT}/src
    ${Protobuf_INCLUDE_DIRS}
    ${GRPC_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
)

# Add the test executable
add_executable(kvstore_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/server_test.cpp
    ${PROJECT_ROOT}/src/server.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/kvstore.pb.cc"
    "${CMAKE_CURRENT_BINARY_DIR}/kvstore.grpc.pb.cc"
)

# Add the server executable
add_executable(kvstore_server
    ${PROJECT_ROOT}/src/server_main.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/kvstore.pb.cc"
    "${CMAKE_CURRENT_BINARY_DIR}/kvstore.grpc.pb.cc"
)

# Link libraries for the test executable
target_link_libraries(kvstore_tests
    GTest::GTest
    GTest::Main
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    Threads::Threads
)

# Link libraries for the server executable
target_link_libraries(kvstore_server
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    Threads::Threads
)

# Enable testing
enable_testing()
add_test(NAME kvstore_tests COMMAND kvstore_tests)

# Add coverage flags if enabled
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    target_compile_options(kvstore_tests PRIVATE --coverage)
    target_link_options(kvstore_tests PRIVATE --coverage)
endif() 