#pragma once
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class MetricsCollector {
public:
  struct OperationMetrics {
    double total_time_ms = 0;
    size_t count = 0;
    double min_time_ms = 0;
    double max_time_ms = 0;
    size_t error_count = 0;

    std::unordered_map<std::string, double> domain_times;
    std::unordered_map<std::string, size_t> domain_counts;
  };

  static MetricsCollector &instance();

  void start_timer(const std::string &operation, const std::string &url = "");

  void stop_timer(const std::string &operation, bool success = true);

  void record_metric(const std::string &operation, double time_ms,
                     bool success = true, const std::string &domain = "");

  void reset();

  std::unordered_map<std::string, OperationMetrics> get_metrics();

  void print_report(std::ostream &os = std::cout);

  void increment_active_threads();
  void decrement_active_threads();
  void set_queue_size(size_t size);
  void set_visited_count(size_t count);

  double get_urls_per_second();
  double get_bandwidth_usage();

  void add_bytes_downloaded(size_t bytes);

private:
  MetricsCollector() = default;
  ~MetricsCollector() = default;
  MetricsCollector(const MetricsCollector &) = delete;
  MetricsCollector &operator=(const MetricsCollector &) = delete;

  std::mutex metrics_mutex_;
  std::unordered_map<std::string, OperationMetrics> metrics_;
  std::unordered_map<std::string,
                     std::chrono::high_resolution_clock::time_point>
      timers_;
  std::unordered_map<std::string, std::string> active_urls_;

  std::atomic<size_t> active_threads_{0};
  std::atomic<size_t> queue_size_{0};
  std::atomic<size_t> visited_count_{0};
  std::chrono::high_resolution_clock::time_point start_time_;
  std::atomic<size_t> total_bytes_downloaded_{0};
};

#define MEASURE_TIME(operation)                                                \
  class ScopedTimer {                                                          \
  public:                                                                      \
    ScopedTimer(const std::string &op) : op_(op) {                             \
      MetricsCollector::instance().start_timer(op_);                           \
    }                                                                          \
    ~ScopedTimer() { MetricsCollector::instance().stop_timer(op_); }           \
                                                                               \
  private:                                                                     \
    std::string op_;                                                           \
  } timer_##__LINE__(operation);