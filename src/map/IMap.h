#pragma once

#include <iterator>
#include <map>
#include <memory>
#include <string>

namespace kvstore {

template <typename K, typename V> class IMap {
public:
  // Use a more generic iterator type that works with both std::map and
  // boost::container::flat_map
  using value_type = std::pair<const K, V>;
  using iterator = typename std::map<K, V>::const_iterator;

  virtual ~IMap() = default;

  // Core map operations
  virtual bool insert(const K &key, const V &value) = 0;
  virtual bool remove(const K &key) = 0;
  virtual bool get(const K &key, V &value) const = 0;
  virtual bool contains(const K &key) const = 0;
  virtual size_t size() const = 0;
  virtual void clear() = 0;

  // Ordered map operations
  virtual iterator begin() const = 0;
  virtual iterator end() const = 0;
  virtual iterator lower_bound(const K &key) const = 0;
  virtual iterator upper_bound(const K &key) const = 0;
};

} // namespace kvstore