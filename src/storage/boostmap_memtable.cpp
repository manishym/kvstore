#include "boostmap_memtable.h"

void BoostMapMemTable::put(const std::string& key, const std::string& value) {
  std::lock_guard<std::mutex> lock(mutex_);
  map_[key] = value;
}

void BoostMapMemTable::del(const std::string& key) {
  std::lock_guard<std::mutex> lock(mutex_);
  map_.erase(key);
}

std::optional<std::string> BoostMapMemTable::get(const std::string& key) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = map_.find(key);
  if (it != map_.end()) {
    return it->second;
  }
  return std::nullopt;
}

size_t BoostMapMemTable::size() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return map_.size();
}
