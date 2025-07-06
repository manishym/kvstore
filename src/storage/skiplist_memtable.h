#ifndef SKIPLIST_MEMTABLE_H
#define SKIPLIST_MEMTABLE_H

#include "memtable.h"
#include <folly/ConcurrentSkipList.h>
#include <atomic>
#include <memory>

class SkipListMemTable : public MemTable {
public:
  using KeyValue = std::pair<std::string, std::string>;
  struct KeyValueComparator {
    bool operator()(const KeyValue& a, const KeyValue& b) const {
      return a.first < b.first;
    }
  };
  using SkipList = folly::ConcurrentSkipList<KeyValue, KeyValueComparator>;
  using Accessor = SkipList::Accessor;

  SkipListMemTable();

  void put(const std::string& key, const std::string& value) override;
  void del(const std::string& key) override;
  std::optional<std::string> get(const std::string& key) const override;
  size_t size() const override;
  void markImmutable() override { immutable_.store(true); }
  bool isImmutable() const override { return immutable_.load(); }

private:
  std::shared_ptr<SkipList> list_;
  std::atomic<bool> immutable_{false};
};

#endif // SKIPLIST_MEMTABLE_H
