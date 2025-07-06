add_library(wal_lib STATIC
    wal_serializer.cpp
)
target_include_directories(wal_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
