cmake_minimum_required(VERSION 3.10)
project(kvstore)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS container)
find_package(nlohmann_json REQUIRED)

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

# Common include directories
set(COMMON_INCLUDE_DIRS
    ${GENERATED_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Protobuf_INCLUDE_DIRS}
    ${gRPC_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)

# Common libraries
set(COMMON_LIBS
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    Threads::Threads
    Boost::container
    nlohmann_json::nlohmann_json
)

# Add server executable
add_executable(server 
    src/server.cpp 
    src/server_impl.h
    src/server_main.cpp
    ${PROTO_SRCS} 
    ${PROTO_HDRS} 
    ${GRPC_SRCS} 
    ${GRPC_HDRS}
)

target_link_libraries(server ${COMMON_LIBS})
target_include_directories(server PRIVATE ${COMMON_INCLUDE_DIRS})

# Add client executable
add_executable(client 
    src/client.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    ${GRPC_SRCS}
    ${GRPC_HDRS}
)

target_link_libraries(client ${COMMON_LIBS})
target_include_directories(client PRIVATE ${COMMON_INCLUDE_DIRS})

# Test configuration
enable_testing()

# Add test executables
add_executable(kvstore_tests
    tests/unit/server_test.cpp
    tests/unit/map_test.cpp
    src/server.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    ${GRPC_SRCS}
    ${GRPC_HDRS}
)

target_link_libraries(kvstore_tests
    ${COMMON_LIBS}
    GTest::GTest
    GTest::Main
)

target_include_directories(kvstore_tests PRIVATE
    ${COMMON_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Add test to CTest
add_test(NAME kvstore_tests COMMAND kvstore_tests)

# Add coverage flags if enabled
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    target_compile_options(kvstore_tests PRIVATE --coverage)
    target_link_options(kvstore_tests PRIVATE --coverage)
endif() 