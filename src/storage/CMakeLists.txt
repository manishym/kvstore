add_library(storage_lib STATIC
    memtable_factory.cpp
    stdmap_memtable.cpp
    boostmap_memtable.cpp
    skiplist_memtable.cpp
)
target_include_directories(storage_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
