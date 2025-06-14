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

// Helper: Average multiple runs of timings
std::vector<double>
average_timings(const std::vector<std::vector<double>> &all_timings) {
  if (all_timings.empty())
    return {};

  std::vector<double> avg_timings(all_timings[0].size(), 0.0);
  for (const auto &timings : all_timings) {
    for (size_t i = 0; i < timings.size(); ++i) {
      avg_timings[i] += timings[i];
    }
  }

  for (auto &t : avg_timings) {
    t /= all_timings.size();
  }
  return avg_timings;
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
void benchmark_std_map(const std::string &name, int num_ops, int n_runs,
                       std::ofstream &out) {
  std::vector<std::vector<double>> all_insert_timings;
  std::vector<std::vector<double>> all_find_timings;
  std::vector<std::vector<double>> all_erase_timings;

  for (int run = 0; run < n_runs; ++run) {
    std::vector<int> keys(num_ops);
    std::iota(keys.begin(), keys.end(), 1);
    std::shuffle(keys.begin(), keys.end(),
                 std::mt19937{static_cast<unsigned long>(
                     42 + run)}); // Different seed for each run
    std::map<int, int> m;

    all_insert_timings.push_back(
        benchmark_op([&](int i) { m[keys[i]] = keys[i]; }, num_ops));

    all_find_timings.push_back(benchmark_op(
        [&](int i) {
          auto it = m.find(keys[i]);
          assert(it != m.end());
        },
        num_ops));

    all_erase_timings.push_back(
        benchmark_op([&](int i) { m.erase(keys[i]); }, num_ops));
  }

  // Average the timings across all runs
  auto avg_insert_timings = average_timings(all_insert_timings);
  auto avg_find_timings = average_timings(all_find_timings);
  auto avg_erase_timings = average_timings(all_erase_timings);

  out << name << "," << num_ops;
  write_percentiles(out, avg_insert_timings, "insert_us");
  write_percentiles(out, avg_find_timings, "find_us");
  write_percentiles(out, avg_erase_timings, "erase_us");
  out << "\n";
}

// ---------- 2. std::map with boost::fast_pool_allocator ----------
void benchmark_std_map_pool(const std::string &name, int num_ops, int n_runs,
                            std::ofstream &out) {
  using pool_alloc = boost::fast_pool_allocator<std::pair<const int, int>>;

  std::vector<std::vector<double>> all_insert_timings;
  std::vector<std::vector<double>> all_find_timings;
  std::vector<std::vector<double>> all_erase_timings;

  for (int run = 0; run < n_runs; ++run) {
    std::vector<int> keys(num_ops);
    std::iota(keys.begin(), keys.end(), 1);
    std::shuffle(keys.begin(), keys.end(),
                 std::mt19937{static_cast<unsigned long>(
                     42 + run)}); // Different seed for each run
    std::map<int, int, std::less<int>, pool_alloc> m;

    all_insert_timings.push_back(
        benchmark_op([&](int i) { m[keys[i]] = keys[i]; }, num_ops));

    all_find_timings.push_back(benchmark_op(
        [&](int i) {
          auto it = m.find(keys[i]);
          assert(it != m.end());
        },
        num_ops));

    all_erase_timings.push_back(
        benchmark_op([&](int i) { m.erase(keys[i]); }, num_ops));
  }

  // Average the timings across all runs
  auto avg_insert_timings = average_timings(all_insert_timings);
  auto avg_find_timings = average_timings(all_find_timings);
  auto avg_erase_timings = average_timings(all_erase_timings);

  out << name << "," << num_ops;
  write_percentiles(out, avg_insert_timings, "insert_us");
  write_percentiles(out, avg_find_timings, "find_us");
  write_percentiles(out, avg_erase_timings, "erase_us");
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
                                int n_runs, std::ofstream &out) {
  using Tree = set<IntrusiveNode>;

  std::vector<std::vector<double>> all_insert_timings;
  std::vector<std::vector<double>> all_find_timings;
  std::vector<std::vector<double>> all_erase_timings;

  for (int run = 0; run < n_runs; ++run) {
    std::vector<int> keys(num_ops);
    std::iota(keys.begin(), keys.end(), 1);
    std::shuffle(keys.begin(), keys.end(),
                 std::mt19937{static_cast<unsigned long>(
                     42 + run)}); // Different seed for each run

    // Pre-allocate all nodes in a slab
    std::vector<IntrusiveNode> nodes(num_ops);
    for (int i = 0; i < num_ops; ++i)
      nodes[i] = IntrusiveNode(keys[i], keys[i]);
    Tree t;

    all_insert_timings.push_back(
        benchmark_op([&](int i) { t.insert(nodes[i]); }, num_ops));

    all_find_timings.push_back(benchmark_op(
        [&](int i) {
          IntrusiveNode tmp(keys[i], 0);
          auto it = t.find(tmp);
          assert(it != t.end());
        },
        num_ops));

    all_erase_timings.push_back(
        benchmark_op([&](int i) { t.erase(nodes[i]); }, num_ops));
  }

  // Average the timings across all runs
  auto avg_insert_timings = average_timings(all_insert_timings);
  auto avg_find_timings = average_timings(all_find_timings);
  auto avg_erase_timings = average_timings(all_erase_timings);

  out << name << "," << num_ops;
  write_percentiles(out, avg_insert_timings, "insert_us");
  write_percentiles(out, avg_find_timings, "find_us");
  write_percentiles(out, avg_erase_timings, "erase_us");
  out << "\n";
}

int main() {
  int max_ops = 1'000'000;
  int min_ops = 100;
  int step = 100;
  int ops = min_ops;
  int n_runs = 5; // Number of times to run each benchmark

  std::ofstream out("map_vs_pool_vs_intrusive.csv");
  out << "MapType,OpsCount"
      << ",insert_us_avg,insert_us_min,insert_us_max,insert_us_p50,insert_us_"
         "p75,insert_us_p90,insert_us_p95,insert_us_p99"
      << ",find_us_avg,find_us_min,find_us_max,find_us_p50,find_us_p75,find_us_"
         "p90,find_us_p95,find_us_p99"
      << ",erase_us_avg,erase_us_min,erase_us_max,erase_us_p50,erase_us_p75,"
         "erase_us_p90,erase_us_p95,erase_us_p99\n";

  // Benchmark with different operation counts
  for (step = 100; step <= 100'000; step *= 10) {
    for (int num_ops = ops; num_ops < (ops * 10); num_ops += step) {
      std::cout << "Benchmarking with " << num_ops << " operations...\n";
      benchmark_std_map("std::map", num_ops, n_runs, out);
      benchmark_std_map_pool("std::map_pool", num_ops, n_runs, out);
      benchmark_intrusive_rbtree("intrusive_rbtree", num_ops, n_runs, out);
    }
    ops *= 10;
    if (ops >= max_ops)
      break;
  }

  out.close();
  std::cout << "Done! See map_vs_pool_vs_intrusive.csv\n";
  return 0;
}
