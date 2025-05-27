#include "../../inc/metrics_collector.h"
#include "../../inc/url_utils.h"

MetricsCollector &MetricsCollector::instance() {
  static MetricsCollector instance;
  return instance;
}

void MetricsCollector::start_timer(const std::string &operation,
                                   const std::string &url) {
  std::lock_guard<std::mutex> lock(metrics_mutex_);

  timers_[operation] = std::chrono::high_resolution_clock::now();

  if (!url.empty()) {
    active_urls_[operation] = url;
  }

  if (start_time_ == std::chrono::high_resolution_clock::time_point()) {
    start_time_ = std::chrono::high_resolution_clock::now();
  }
}

void MetricsCollector::stop_timer(const std::string &operation, bool success) {
  std::lock_guard<std::mutex> lock(metrics_mutex_);

  auto it = timers_.find(operation);
  if (it != timers_.end()) {
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                        end_time - it->second)
                        .count() /
                    1000.0;

    std::string domain;
    auto url_it = active_urls_.find(operation);
    if (url_it != active_urls_.end()) {
      try {
        domain = UrlUtils::extract_domain(url_it->second);
      } catch (...) {
        domain = "unknown";
      }
      active_urls_.erase(url_it);
    }

    auto &metric = metrics_[operation];
    metric.total_time_ms += duration;
    metric.count++;

    if (metric.count == 1 || duration < metric.min_time_ms) {
      metric.min_time_ms = duration;
    }
    if (metric.count == 1 || duration > metric.max_time_ms) {
      metric.max_time_ms = duration;
    }

    if (!success) {
      metric.error_count++;
    }

    if (!domain.empty()) {
      metric.domain_times[domain] += duration;
      metric.domain_counts[domain]++;
    }

    timers_.erase(it);
  }
}

void MetricsCollector::record_metric(const std::string &operation,
                                     double time_ms, bool success,
                                     const std::string &domain) {
  std::lock_guard<std::mutex> lock(metrics_mutex_);

  auto &metric = metrics_[operation];
  metric.total_time_ms += time_ms;
  metric.count++;

  if (metric.count == 1 || time_ms < metric.min_time_ms) {
    metric.min_time_ms = time_ms;
  }
  if (metric.count == 1 || time_ms > metric.max_time_ms) {
    metric.max_time_ms = time_ms;
  }

  if (!success) {
    metric.error_count++;
  }

  if (!domain.empty()) {
    metric.domain_times[domain] += time_ms;
    metric.domain_counts[domain]++;
  }
}

void MetricsCollector::reset() {
  std::lock_guard<std::mutex> lock(metrics_mutex_);
  metrics_.clear();
  timers_.clear();
  active_urls_.clear();
  active_threads_ = 0;
  queue_size_ = 0;
  visited_count_ = 0;
  total_bytes_downloaded_ = 0;

  start_time_ = std::chrono::high_resolution_clock::now();
}

std::unordered_map<std::string, MetricsCollector::OperationMetrics>
MetricsCollector::get_metrics() {
  std::lock_guard<std::mutex> lock(metrics_mutex_);
  return metrics_;
}

void MetricsCollector::print_report(std::ostream &os) {
  std::lock_guard<std::mutex> lock(metrics_mutex_);

  os << "\n===== Web Crawler Performance Report =====\n";

  auto now = std::chrono::high_resolution_clock::now();
  double total_runtime_sec =
      std::chrono::duration_cast<std::chrono::seconds>(now - start_time_)
          .count();

  os << "Runtime: " << std::fixed << std::setprecision(2) << total_runtime_sec
     << " seconds\n";
  os << "URLs processed: " << visited_count_ << "\n";
  os << "Active threads: " << active_threads_ << "\n";
  os << "Queue size: " << queue_size_ << "\n";
  os << "Processing rate: "
     << (total_runtime_sec > 0 ? visited_count_ / total_runtime_sec : 0)
     << " URLs/second\n";
}

void MetricsCollector::increment_active_threads() { ++active_threads_; }

void MetricsCollector::decrement_active_threads() {
  size_t current = active_threads_.load();
  if (current > 0) {
    --active_threads_;
  }
}

void MetricsCollector::set_queue_size(size_t size) { queue_size_ = size; }

void MetricsCollector::set_visited_count(size_t count) {
  visited_count_ = count;
}

double MetricsCollector::get_urls_per_second() {
  auto now = std::chrono::high_resolution_clock::now();
  double total_runtime_sec =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_)
          .count() /
      1000.0;
  return total_runtime_sec > 0 ? visited_count_ / total_runtime_sec : 0;
}

double MetricsCollector::get_bandwidth_usage() {
  auto now = std::chrono::high_resolution_clock::now();
  double total_runtime_sec =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_)
          .count() /
      1000.0;
  return total_runtime_sec > 0
             ? (total_bytes_downloaded_ / 1024.0) / total_runtime_sec
             : 0;
}

void MetricsCollector::add_bytes_downloaded(size_t bytes) {
  total_bytes_downloaded_ += bytes;
}