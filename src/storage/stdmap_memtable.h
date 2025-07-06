#ifndef STDMAP_MEMTABLE_H
#define STDMAP_MEMTABLE_H

#include "memtable.h"
#include <map>
#include <mutex>
#include <atomic>

class StdMapMemTable : public MemTable {
public:
  void put(const std::string& key, const std::string& value) override;
  void del(const std::string& key) override;
  std::optional<std::string> get(const std::string& key) const override;
  size_t size() const override;
  void markImmutable() override { immutable_.store(true); }
  bool isImmutable() const override { return immutable_.load(); }

private:
  mutable std::mutex mutex_;
  std::map<std::string, std::string> map_;
  std::atomic<bool> immutable_{false};
};

#endif // STDMAP_MEMTABLE_H
