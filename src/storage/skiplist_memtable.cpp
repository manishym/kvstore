#include "skiplist_memtable.h"

SkipListMemTable::SkipListMemTable() : list_(SkipList::createInstance()) {}

void SkipListMemTable::put(const std::string& key, const std::string& value) {
  Accessor accessor(list_);
  KeyValue kv = std::make_pair(key, value);
  auto it = accessor.find(kv);
  if (it != accessor.end()) {
    accessor.erase(kv);
  }
  accessor.insert(kv);
}

void SkipListMemTable::del(const std::string& key) {
  Accessor accessor(list_);
  KeyValue kv = std::make_pair(key, "");
  accessor.erase(kv);
}

std::optional<std::string> SkipListMemTable::get(const std::string& key) const {
  Accessor accessor(list_);
  KeyValue kv = std::make_pair(key, "");
  auto it = accessor.find(kv);
  if (it != accessor.end()) {
    return it->second;
  }
  return std::nullopt;
}

size_t SkipListMemTable::size() const {
  return list_->size();
}
