#include <algorithm>
#include <boost/container/flat_map.hpp>
#include <boost/container/map.hpp>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <vector>

// --------- Place this FIRST: Helper for timing and percentiles ---------
template <typename Func> std::vector<double> benchmark_op(Func &&f, int ops) {
  std::vector<double> timings(ops);
  for (int i = 0; i < ops; ++i) {
    auto t1 = std::chrono::high_resolution_clock::now();
    f(i);
    auto t2 = std::chrono::high_resolution_clock::now();
    timings[i] =
        std::chrono::duration<double, std::micro>(t2 - t1).count(); // us
  }
  return timings;
}
void write_percentile_headers(std::ofstream &out, const std::string &prefix) {
  out << "," << prefix << "_avg"
      << "," << prefix << "_min"
      << "," << prefix << "_max"
      << "," << prefix << "_p50"
      << "," << prefix << "_p75"
      << "," << prefix << "_p90"
      << "," << prefix << "_p95"
      << "," << prefix << "_p99";
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

// --------- Map Benchmark Template (tree-based) -----------
template <typename MapType>
void benchmark_map(const std::string &name, int num_ops, std::ofstream &out) {
  std::vector<int> keys(num_ops);
  std::iota(keys.begin(), keys.end(), 1); // Unique keys 1...num_ops
  std::shuffle(keys.begin(), keys.end(), std::mt19937{42}); // Random order

  MapType m;

  // Insert
  auto insert_timings =
      benchmark_op([&](int i) { m[keys[i]] = keys[i]; }, num_ops);

  // Find
  auto find_timings = benchmark_op(
      [&](int i) {
        auto it = m.find(keys[i]);
        assert(it != m.end());
      },
      num_ops);

  // Erase
  auto erase_timings = benchmark_op([&](int i) { m.erase(keys[i]); }, num_ops);

  // CSV: MapType,OpsCount,insert_us_avg,min,max,p50,p75,p90,p95,p99,...
  out << name << "," << num_ops;
  write_percentiles(out, insert_timings, "insert_us");
  write_percentiles(out, find_timings, "find_us");
  write_percentiles(out, erase_timings, "erase_us");
  out << "\n";
}

// --------- Flat Map Specialization with Pre-allocation -----------
void benchmark_flat_map(const std::string &name, int num_ops,
                        std::ofstream &out) {
  std::vector<int> keys(num_ops);
  std::iota(keys.begin(), keys.end(), 1);
  std::shuffle(keys.begin(), keys.end(), std::mt19937{42});

  boost::container::flat_map<int, int> m;
  m.reserve(num_ops); // Pre-allocate storage

  // Insert
  auto insert_timings = benchmark_op(
      [&](int i) { m.insert(std::make_pair(keys[i], keys[i])); }, num_ops);

  // Find
  auto find_timings = benchmark_op(
      [&](int i) {
        auto it = m.find(keys[i]);
        assert(it != m.end());
      },
      num_ops);

  // Erase
  auto erase_timings = benchmark_op([&](int i) { m.erase(keys[i]); }, num_ops);

  out << name << "," << num_ops;
  write_percentiles(out, insert_timings, "insert_us");
  write_percentiles(out, find_timings, "find_us");
  write_percentiles(out, erase_timings, "erase_us");
  out << "\n";
}

// ----------- Main -----------
int main() {
  std::ofstream out("cpp_map_percentile_bench.csv");
  out << "MapType,OpsCount";
  write_percentile_headers(out, "insert_us");
  write_percentile_headers(out, "find_us");
  write_percentile_headers(out, "erase_us");
  out << "\n";

  // Benchmark with different operation counts
  for (int num_ops = 100'000; num_ops <= 1'000'000; num_ops += 100'000) {
    std::cout << "Benchmarking with " << num_ops << " operations...\n";

    // Tree-based maps
    benchmark_map<std::map<int, int>>("std::map", num_ops, out);
    benchmark_map<boost::container::map<int, int>>("boost::map", num_ops, out);

    // Flat_map with pre-allocation
    benchmark_flat_map("boost::flat_map", num_ops, out);
  }

  out.close();
  std::cout << "Done! Results in cpp_map_percentile_bench.csv\n";
  return 0;
}
