#include "memtable_factory.h"
#include "stdmap_memtable.h"
#include "boostmap_memtable.h"
#include "skiplist_memtable.h"

std::shared_ptr<MemTable> createMemTable(const nlohmann::json& config) {
  std::string type = "skiplist";
  if (config.contains("map_type") && config["map_type"].is_string()) {
    type = config["map_type"].get<std::string>();
  }
  if (type == "std") {
    return std::make_shared<StdMapMemTable>();
  } else if (type == "boost") {
    return std::make_shared<BoostMapMemTable>();
  }
  return std::make_shared<SkipListMemTable>();
}
