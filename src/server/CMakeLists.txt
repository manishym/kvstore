add_library(kvserver_lib STATIC
    kv_server.cpp
)
target_include_directories(kvserver_lib PUBLIC \
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
