#ifndef MEMTABLE_H
#define MEMTABLE_H

#include <optional>
#include <string>

class MemTable {
public:
  virtual ~MemTable() = default;
  virtual void put(const std::string& key, const std::string& value) = 0;
  virtual void del(const std::string& key) = 0;
  virtual std::optional<std::string> get(const std::string& key) const = 0;
  virtual size_t size() const = 0;
  virtual void markImmutable() = 0;
  virtual bool isImmutable() const = 0;
};

#endif // MEMTABLE_H
