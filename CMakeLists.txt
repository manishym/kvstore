cmake_minimum_required(VERSION 3.10)
project(kvstore)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS container program_options)
find_package(PkgConfig REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(fmt REQUIRED)
find_package(CLI11 REQUIRED)

# Folly
set(FOLLY_INCLUDE_DIR "/usr/local/include")
set(FOLLY_LIBRARY "/usr/local/lib/libfolly.a")

# Protobuf / gRPC codegen
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}")
file(MAKE_DIRECTORY ${GENERATED_DIR})
set(Protobuf_PROTOC_EXECUTABLE "/usr/bin/protoc")
set(GRPC_CPP_PLUGIN $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/proto/kvstore.proto")
set(PROTO_SRCS "${GENERATED_DIR}/kvstore.pb.cc")
set(PROTO_HDRS "${GENERATED_DIR}/kvstore.pb.h")
set(GRPC_SRCS "${GENERATED_DIR}/kvstore.grpc.pb.cc")
set(GRPC_HDRS "${GENERATED_DIR}/kvstore.grpc.pb.h")

add_custom_command(
  OUTPUT ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS}
  COMMAND ${Protobuf_PROTOC_EXECUTABLE}
  ARGS --grpc_out "${GENERATED_DIR}"
       --cpp_out "${GENERATED_DIR}"
       -I "${CMAKE_CURRENT_SOURCE_DIR}/proto"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       "${PROTO_FILE}"
  DEPENDS "${PROTO_FILE}"
)

# Include directories
set(COMMON_INCLUDE_DIRS
  ${GENERATED_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${Protobuf_INCLUDE_DIRS}
  ${gRPC_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${FOLLY_INCLUDE_DIR}
  ${SPDK_INCLUDE_DIRS}
)

# Get static linker flags for SPDK + DPDK
execute_process(COMMAND pkg-config --static --libs spdk_bdev spdk_env_dpdk OUTPUT_VARIABLE SPDK_STATIC_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)

# Common libraries
set(COMMON_LIBS
  gRPC::grpc++
  gRPC::grpc++_reflection
  protobuf::libprotobuf
  Threads::Threads
  ${FOLLY_LIBRARY}
  Boost::container
  Boost::program_options
  nlohmann_json::nlohmann_json
  double-conversion
  fmt::fmt
  CLI11::CLI11
  ${SPDK_STATIC_LIBS}
  uuid
  isal
  z
  snappy
  lz4
  zstd
  glog
  gflags
  iberty
  dl
  pthread
  rt
)

add_compile_options(-fno-omit-frame-pointer -pthread)

# Server
add_executable(server
  src/server/kv_server.cpp
  src/main.cpp
  src/storage/memtable_factory.cpp
  src/storage/stdmap_memtable.cpp
  src/storage/boostmap_memtable.cpp
  src/storage/skiplist_memtable.cpp
  src/wal/spdk_wal.cpp
  src/wal/wal_serializer.cpp
  src/utils/file_buffer_writer.cpp
  ${PROTO_SRCS} ${PROTO_HDRS}
  ${GRPC_SRCS} ${GRPC_HDRS}
)

target_link_libraries(server ${COMMON_LIBS})
target_include_directories(server PRIVATE ${COMMON_INCLUDE_DIRS})

# Client
add_executable(client
  src/client/client.cpp
  src/wal/spdk_wal.cpp
  src/wal/wal_serializer.cpp
  src/utils/file_buffer_writer.cpp
  ${PROTO_SRCS} ${PROTO_HDRS}
  ${GRPC_SRCS} ${GRPC_HDRS}
)

target_link_libraries(client ${COMMON_LIBS})
target_include_directories(client PRIVATE ${COMMON_INCLUDE_DIRS})

# Tests
enable_testing()
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
  src/wal/spdk_wal.cpp
  src/storage/memtable_factory.cpp
  src/storage/stdmap_memtable.cpp
  src/storage/boostmap_memtable.cpp
  src/storage/skiplist_memtable.cpp
  src/server/kv_server.cpp
  src/wal/wal_serializer.cpp
  src/utils/file_buffer_writer.cpp
  src/utils/spdk_buffer_writer.cpp
  ${PROTO_SRCS} ${PROTO_HDRS}
  ${GRPC_SRCS} ${GRPC_HDRS}
)

target_link_libraries(kvstore_tests ${COMMON_LIBS} GTest::GTest GTest::Main)
target_include_directories(kvstore_tests PRIVATE ${COMMON_INCLUDE_DIRS})
add_test(NAME kvstore_tests COMMAND kvstore_tests)

# Coverage
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  target_compile_options(kvstore_tests PRIVATE --coverage)
  target_link_options(kvstore_tests PRIVATE --coverage)
endif()
