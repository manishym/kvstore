#pragma once

#include "IMap.h"
#include <boost/container/flat_map.hpp>
#include <map>

namespace kvstore {

template <typename K, typename V> class BoostMap : public IMap<K, V> {
public:
  BoostMap(size_t initial_size = 1000, float load_factor = 0.75f) {
    map_.reserve(initial_size);
  }

  bool insert(const K &key, const V &value) override {
    auto it = map_.find(key);
    bool inserted = it == map_.end();
    map_[key] = value;
    ordered_map_[key] = value;
    return inserted;
  }

  bool remove(const K &key) override {
    size_t count = map_.erase(key);
    ordered_map_.erase(key);
    return count > 0;
  }

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

  void clear() override {
    map_.clear();
    ordered_map_.clear();
  }

  // Ordered map operations
  typename IMap<K, V>::iterator begin() const override {
    return ordered_map_.begin();
  }

  typename IMap<K, V>::iterator end() const override {
    return ordered_map_.end();
  }

  typename IMap<K, V>::iterator lower_bound(const K &key) const override {
    return ordered_map_.lower_bound(key);
  }

  typename IMap<K, V>::iterator upper_bound(const K &key) const override {
    return ordered_map_.upper_bound(key);
  }

private:
  boost::container::flat_map<K, V> map_;
  std::map<K, V> ordered_map_;
}; 

} // namespace kvstore