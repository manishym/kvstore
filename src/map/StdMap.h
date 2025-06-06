#pragma once

#include "IMap.h"
#include <map>

namespace kvstore {

template <typename K, typename V> class StdMap : public IMap<K, V> {
public:
  StdMap(size_t initial_size = 1000) {
    // std::map doesn't support reserve
  }

  bool insert(const K &key, const V &value) override {
    return map_.insert({key, value}).second;
  }

  bool remove(const K &key) override { return map_.erase(key) > 0; }

  bool get(const K &key, V &value) const override {
    auto it = map_.find(key);
    if (it != map_.end()) {
      value = it->second;
      return true;
    }
    return false;
  }

  bool contains(const K &key) const override {
    return map_.find(key) != map_.end();
  }

  size_t size() const override { return map_.size(); }

  void clear() override { map_.clear(); }

  // Ordered map operations
  typename IMap<K, V>::iterator begin() const override { return map_.begin(); }
  typename IMap<K, V>::iterator end() const override { return map_.end(); }
  typename IMap<K, V>::iterator lower_bound(const K &key) const override {
    return map_.lower_bound(key);
  }
  typename IMap<K, V>::iterator upper_bound(const K &key) const override {
    return map_.upper_bound(key);
  }

private:
  std::map<K, V> map_;
};

} // namespace kvstore