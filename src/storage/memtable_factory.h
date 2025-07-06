#ifndef MEMTABLE_FACTORY_H
#define MEMTABLE_FACTORY_H

#include "memtable.h"
#include <memory>
#include <nlohmann/json.hpp>

std::shared_ptr<MemTable> createMemTable(const nlohmann::json& config);

#endif // MEMTABLE_FACTORY_H
