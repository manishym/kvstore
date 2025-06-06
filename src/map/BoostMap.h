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
    return map_.insert(std::make_pair(key, value)).second;
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
  typename IMap<K, V>::iterator begin() const override {
    // Convert boost::container::flat_map iterator to std::map iterator
    std::map<K, V> temp_map(map_.begin(), map_.end());
    return temp_map.begin();
  }

  typename IMap<K, V>::iterator end() const override {
    std::map<K, V> temp_map(map_.begin(), map_.end());
    return temp_map.end();
  }

  typename IMap<K, V>::iterator lower_bound(const K &key) const override {
    std::map<K, V> temp_map(map_.begin(), map_.end());
    return temp_map.lower_bound(key);
  }

  typename IMap<K, V>::iterator upper_bound(const K &key) const override {
    std::map<K, V> temp_map(map_.begin(), map_.end());
    return temp_map.upper_bound(key);
  }

private:
  boost::container::flat_map<K, V> map_;
};

} // namespace kvstore