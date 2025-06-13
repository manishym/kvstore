#include <algorithm>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <vector>

#include <boost/intrusive/set.hpp>
#include <boost/pool/pool_alloc.hpp>

using namespace boost::intrusive;

// Helper: Time and percentiles
template <typename Func> std::vector<double> benchmark_op(Func &&f, int ops) {
  std::vector<double> timings(ops);
  for (int i = 0; i < ops; ++i) {
    auto t1 = std::chrono::high_resolution_clock::now();
    f(i);
    auto t2 = std::chrono::high_resolution_clock::now();
    timings[i] = std::chrono::duration<double, std::micro>(t2 - t1).count();
  }
  return timings;
}

void write_percentiles(std::ofstream &out, const std::vector<double> &data,
                       const std::string &prefix) {
  std::vector<double> sorted = data;
  std::sort(sorted.begin(), sorted.end());
  auto get_pct = [&](double pct) {
    size_t idx = static_cast<size_t>(pct * sorted.size());
    if (idx >= sorted.size())
      idx = sorted.size() - 1;
    return sorted[idx];
  };
  double avg =
      std::accumulate(sorted.begin(), sorted.end(), 0.0) / sorted.size();
  out << "," << avg << "," << sorted.front() << "," << sorted.back() << ","
      << get_pct(0.50) << "," << get_pct(0.75) << "," << get_pct(0.90) << ","
      << get_pct(0.95) << "," << get_pct(0.99);
}

// ---------- 1. std::map (default allocator) ----------
void benchmark_std_map(const std::string &name, int num_ops,
                       std::ofstream &out) {
  std::vector<int> keys(num_ops);
  std::iota(keys.begin(), keys.end(), 1);
  std::shuffle(keys.begin(), keys.end(), std::mt19937{42});
  std::map<int, int> m;

  auto insert_timings =
      benchmark_op([&](int i) { m[keys[i]] = keys[i]; }, num_ops);

  auto find_timings = benchmark_op(
      [&](int i) {
        auto it = m.find(keys[i]);
        assert(it != m.end());
      },
      num_ops);

  auto erase_timings = benchmark_op([&](int i) { m.erase(keys[i]); }, num_ops);

  out << name << "," << num_ops;
  write_percentiles(out, insert_timings, "insert_us");
  write_percentiles(out, find_timings, "find_us");
  write_percentiles(out, erase_timings, "erase_us");
  out << "\n";
}

// ---------- 2. std::map with boost::fast_pool_allocator ----------
void benchmark_std_map_pool(const std::string &name, int num_ops,
                            std::ofstream &out) {
  using pool_alloc = boost::fast_pool_allocator<std::pair<const int, int>>;
  std::vector<int> keys(num_ops);
  std::iota(keys.begin(), keys.end(), 1);
  std::shuffle(keys.begin(), keys.end(), std::mt19937{42});
  std::map<int, int, std::less<int>, pool_alloc> m;

  auto insert_timings =
      benchmark_op([&](int i) { m[keys[i]] = keys[i]; }, num_ops);

  auto find_timings = benchmark_op(
      [&](int i) {
        auto it = m.find(keys[i]);
        assert(it != m.end());
      },
      num_ops);

  auto erase_timings = benchmark_op([&](int i) { m.erase(keys[i]); }, num_ops);

  out << name << "," << num_ops;
  write_percentiles(out, insert_timings, "insert_us");
  write_percentiles(out, find_timings, "find_us");
  write_percentiles(out, erase_timings, "erase_us");
  out << "\n";
}

// ---------- 3. boost::intrusive::rbtree (pre-allocated nodes) ----------
struct IntrusiveNode
    : public set_base_hook<optimize_size<true>, link_mode<normal_link>> {
  int key;
  int value;

  IntrusiveNode(int k = 0, int v = 0) : key(k), value(v) {}
  bool operator<(const IntrusiveNode &other) const { return key < other.key; }
  bool operator==(const IntrusiveNode &other) const { return key == other.key; }
};

void benchmark_intrusive_rbtree(const std::string &name, int num_ops,
                                std::ofstream &out) {
  using Tree = set<IntrusiveNode>;
  std::vector<int> keys(num_ops);
  std::iota(keys.begin(), keys.end(), 1);
  std::shuffle(keys.begin(), keys.end(), std::mt19937{42});

  // Pre-allocate all nodes in a slab
  std::vector<IntrusiveNode> nodes(num_ops);
  for (int i = 0; i < num_ops; ++i)
    nodes[i] = IntrusiveNode(keys[i], keys[i]);
  Tree t;

  // Insert
  auto insert_timings =
      benchmark_op([&](int i) { t.insert(nodes[i]); }, num_ops);

  // Find
  auto find_timings = benchmark_op(
      [&](int i) {
        IntrusiveNode tmp(keys[i], 0);
        auto it = t.find(tmp);
        assert(it != t.end());
      },
      num_ops);

  // Erase
  auto erase_timings = benchmark_op([&](int i) { t.erase(nodes[i]); }, num_ops);

  out << name << "," << num_ops;
  write_percentiles(out, insert_timings, "insert_us");
  write_percentiles(out, find_timings, "find_us");
  write_percentiles(out, erase_timings, "erase_us");
  out << "\n";
}

int main() {
  int num_ops = 1'000'000'0;

  std::ofstream out("map_vs_pool_vs_intrusive.csv");
  out << "MapType,TotalOps"
      << ",insert_us_avg,insert_us_min,insert_us_max,insert_us_p50,insert_us_"
         "p75,insert_us_p90,insert_us_p95,insert_us_p99"
      << ",find_us_avg,find_us_min,find_us_max,find_us_p50,find_us_p75,find_us_"
         "p90,find_us_p95,find_us_p99"
      << ",erase_us_avg,erase_us_min,erase_us_max,erase_us_p50,erase_us_p75,"
         "erase_us_p90,erase_us_p95,erase_us_p99\n";

  benchmark_std_map("std::map", num_ops, out);
  benchmark_std_map_pool("std::map_pool", num_ops, out);
  benchmark_intrusive_rbtree("intrusive_rbtree", num_ops, out);

  out.close();
  std::cout << "Done! See map_vs_pool_vs_intrusive.csv\n";
  return 0;
}
