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
find_package(fmt REQUIRED)
find_package(CLI11 REQUIRED)

# Find SPDK
find_library(SPDK_BDEV_LIBRARY NAMES spdk_bdev)
find_library(SPDK_ENV_LIBRARY NAMES spdk_env_dpdk)
find_library(SPDK_EVENT_LIBRARY NAMES spdk_event)
find_library(SPDK_THREAD_LIBRARY NAMES spdk_thread)
find_library(SPDK_UTIL_LIBRARY NAMES spdk_util)
find_library(SPDK_JSON_LIBRARY NAMES spdk_json)

# Also find dynamic SPDK libraries
find_library(SPDK_BDEV_SO NAMES libspdk_bdev.so)
find_library(SPDK_ENV_SO NAMES libspdk_env_dpdk.so)
find_library(SPDK_EVENT_SO NAMES libspdk_event.so)
find_library(SPDK_THREAD_SO NAMES libspdk_thread.so)
find_library(SPDK_UTIL_SO NAMES libspdk_util.so)
find_library(SPDK_JSON_SO NAMES libspdk_json.so)

if(NOT SPDK_BDEV_LIBRARY OR NOT SPDK_ENV_LIBRARY OR NOT SPDK_EVENT_LIBRARY)
    message(FATAL_ERROR "Required SPDK libraries not found")
endif()

# Use the system protoc instead of any local version
set(Protobuf_PROTOC_EXECUTABLE "/usr/bin/protoc")

# Add Folly
set(FOLLY_INCLUDE_DIR "/usr/local/include")
set(FOLLY_LIBRARY "/usr/local/lib/libfolly.a")

# Set the directory for generated files
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}")
file(MAKE_DIRECTORY ${GENERATED_DIR})

# Generate protobuf files
set(PROTO_SRCS "${GENERATED_DIR}/kvstore.pb.cc")
set(PROTO_HDRS "${GENERATED_DIR}/kvstore.pb.h")

# Generate gRPC files
set(GRPC_CPP_PLUGIN $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
set(GRPC_SRCS "${GENERATED_DIR}/kvstore.grpc.pb.cc")
set(GRPC_HDRS "${GENERATED_DIR}/kvstore.grpc.pb.h")

add_custom_command(
    OUTPUT "${PROTO_SRCS}" "${PROTO_HDRS}" "${GRPC_SRCS}" "${GRPC_HDRS}"
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
    ${FOLLY_INCLUDE_DIR}
    /usr/local/include
)

# Common libraries
set(COMMON_LIBS
    gRPC::grpc++
    gRPC::grpc++_reflection
    protobuf::libprotobuf
    Threads::Threads
    Boost::container
    nlohmann_json::nlohmann_json
    ${FOLLY_LIBRARY}
    fmt::fmt
    CLI11::CLI11
    # System libraries
    uuid
    numa
    dl
    rt
    pthread
    z
)

# Add server executable
add_executable(server
    src/server/kv_server.cpp
    src/server/kv_server.h
    src/main.cpp
    src/storage/memtable_factory.cpp
    src/storage/stdmap_memtable.cpp
    src/storage/boostmap_memtable.cpp
    src/storage/skiplist_memtable.cpp
    src/wal/wal_serializer.cpp
    src/utils/file_buffer_writer.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    ${GRPC_SRCS}
    ${GRPC_HDRS}
)

add_compile_options(-fno-omit-frame-pointer -pthread)

target_link_libraries(server ${COMMON_LIBS} dl z snappy lz4 zstd pthread glog gflags iberty)
target_include_directories(server PRIVATE ${COMMON_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

# Add client executable
add_executable(client
    src/client/client.cpp
    src/wal/wal_serializer.cpp
    src/utils/file_buffer_writer.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    ${GRPC_SRCS}
    ${GRPC_HDRS}
)

target_link_libraries(client ${COMMON_LIBS} dl z snappy lz4 zstd pthread glog gflags iberty)
target_include_directories(client PRIVATE ${COMMON_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

# Test configuration
enable_testing()

# Add test executables
add_executable(kvstore_tests
    tests/unit/server_test.cpp
    tests/unit/memtable_test.cpp
    tests/unit/map_test.cpp
    tests/unit/block_device_wal_test.cpp
    tests/unit/spdk_wal_test.cpp
    tests/unit/wal_factory_test.cpp
    tests/unit/wal_serializer_test.cpp
    tests/unit/passthrough_wal_test.cpp
    tests/unit/file_buffer_writer_test.cpp
    tests/unit/spdk_buffer_writer_test.cpp
    src/storage/memtable_factory.cpp
    src/storage/stdmap_memtable.cpp
    src/storage/boostmap_memtable.cpp
    src/storage/skiplist_memtable.cpp
    src/server/kv_server.cpp
    src/wal/wal_serializer.cpp
    src/utils/file_buffer_writer.cpp
    src/utils/spdk_buffer_writer.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    ${GRPC_SRCS}
    ${GRPC_HDRS}
)

target_link_libraries(kvstore_tests ${COMMON_LIBS} GTest::GTest GTest::Main glog gflags iberty)
target_include_directories(kvstore_tests PRIVATE
    ${COMMON_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Add test to CTest
add_test(NAME kvstore_tests COMMAND kvstore_tests)

# Add SPDK WAL test executable
add_executable(test_spdk_wal
    src/test_spdk_wal.cpp
    src/wal/wal_serializer.cpp
    src/utils/file_buffer_writer.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    ${GRPC_SRCS}
    ${GRPC_HDRS}
)

target_link_libraries(test_spdk_wal ${COMMON_LIBS} dl z snappy lz4 zstd pthread glog gflags iberty)
target_include_directories(test_spdk_wal PRIVATE ${COMMON_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})

# Add coverage flags if enabled
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    target_compile_options(kvstore_tests PRIVATE --coverage)
    target_link_options(kvstore_tests PRIVATE --coverage)
endif() 