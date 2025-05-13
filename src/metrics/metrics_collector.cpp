#include "../../inc/metrics_collector.h"
#include "../../inc/url_utils.h"

MetricsCollector &MetricsCollector::instance() {
  static MetricsCollector instance;
  return instance;
}

void MetricsCollector::start_timer(const std::string &operation,
                                   const std::string &url) {
  std::lock_guard<std::mutex> lock(metrics_mutex_);

  // Запоминаем время начала операции
  timers_[operation] = std::chrono::high_resolution_clock::now();

  // Если указан URL, сохраняем его для этой операции
  if (!url.empty()) {
    active_urls_[operation] = url;
  }

  // Если это первый таймер, инициализируем общее время начала
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

    // Получаем URL и домен, если они были сохранены
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

    // Обновляем метрики
    auto &metric = metrics_[operation];
    metric.total_time_ms += duration;
    metric.count++;

    // Обновляем мин/макс
    if (metric.count == 1 || duration < metric.min_time_ms) {
      metric.min_time_ms = duration;
    }
    if (metric.count == 1 || duration > metric.max_time_ms) {
      metric.max_time_ms = duration;
    }

    // Если не успешно, увеличиваем счетчик ошибок
    if (!success) {
      metric.error_count++;
    }

    // Обновляем доменную статистику
    if (!domain.empty()) {
      metric.domain_times[domain] += duration;
      metric.domain_counts[domain]++;
    }

    // Удаляем таймер из активных
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

  // Обновляем мин/макс
  if (metric.count == 1 || time_ms < metric.min_time_ms) {
    metric.min_time_ms = time_ms;
  }
  if (metric.count == 1 || time_ms > metric.max_time_ms) {
    metric.max_time_ms = time_ms;
  }

  // Если не успешно, увеличиваем счетчик ошибок
  if (!success) {
    metric.error_count++;
  }

  // Обновляем доменную статистику
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
  start_time_ = std::chrono::high_resolution_clock::time_point();
}

std::unordered_map<std::string, MetricsCollector::OperationMetrics>
MetricsCollector::get_metrics() {
  std::lock_guard<std::mutex> lock(metrics_mutex_);
  return metrics_;
}

void MetricsCollector::print_report(std::ostream &os) {
  std::lock_guard<std::mutex> lock(metrics_mutex_);

  os << "\n===== Web Crawler Performance Report =====\n";

  // Общая статистика
  auto now = std::chrono::high_resolution_clock::now();
  double total_runtime_sec =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_)
          .count() /
      1000.0;

  os << "Runtime: " << std::fixed << std::setprecision(2) << total_runtime_sec
     << " seconds\n";
  os << "URLs processed: " << visited_count_ << "\n";
  os << "Active threads: " << active_threads_ << "\n";
  os << "Queue size: " << queue_size_ << "\n";
  os << "Processing rate: "
     << (total_runtime_sec > 0 ? visited_count_ / total_runtime_sec : 0)
     << " URLs/second\n";

  os << "\n----- Operation Statistics -----\n";
  os << std::setw(25) << "Operation" << " | " << std::setw(10) << "Count"
     << " | " << std::setw(10) << "Avg (ms)" << " | " << std::setw(10)
     << "Min (ms)" << " | " << std::setw(10) << "Max (ms)" << " | "
     << std::setw(10) << "Errors" << " | " << "Success Rate" << "\n";
  os << std::string(100, '-') << "\n";

  for (const auto &metric_pair : metrics_) {
    const auto &name = metric_pair.first;
    const auto &metric = metric_pair.second;

    double avg_time =
        metric.count > 0 ? metric.total_time_ms / metric.count : 0;
    double success_rate =
        metric.count > 0
            ? 100.0 * (metric.count - metric.error_count) / metric.count
            : 0;

    os << std::setw(25) << name << " | " << std::setw(10) << metric.count
       << " | " << std::setw(10) << std::fixed << std::setprecision(2)
       << avg_time << " | " << std::setw(10) << std::fixed
       << std::setprecision(2) << metric.min_time_ms << " | " << std::setw(10)
       << std::fixed << std::setprecision(2) << metric.max_time_ms << " | "
       << std::setw(10) << metric.error_count << " | " << std::fixed
       << std::setprecision(2) << success_rate << "%\n";
  }

  // Топ-5 самых медленных доменов
  os << "\n----- Top 5 Slowest Domains (HTTP Requests) -----\n";

  std::vector<std::pair<std::string, double>> domain_avg_times;
  auto http_metrics_it = metrics_.find("HTTP Request");

  if (http_metrics_it != metrics_.end() &&
      !http_metrics_it->second.domain_times.empty()) {
    const auto &http_metrics = http_metrics_it->second;

    for (const auto &domain_pair : http_metrics.domain_times) {
      const auto &domain = domain_pair.first;
      double total_time = domain_pair.second;
      size_t count = http_metrics.domain_counts.at(domain);
      double avg_time = count > 0 ? total_time / count : 0;

      domain_avg_times.push_back({domain, avg_time});
    }

    // Сортировка по среднему времени (убывание)
    std::sort(domain_avg_times.begin(), domain_avg_times.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    // Вывод топ-5 или меньше, если доменов меньше 5
    size_t domains_to_show = std::min(domain_avg_times.size(), size_t(5));

    os << std::setw(30) << "Domain" << " | " << std::setw(10) << "Requests"
       << " | " << std::setw(15) << "Avg Time (ms)" << "\n";
    os << std::string(60, '-') << "\n";

    for (size_t i = 0; i < domains_to_show; ++i) {
      const auto &domain = domain_avg_times[i].first;
      double avg_time = domain_avg_times[i].second;
      size_t count = http_metrics.domain_counts.at(domain);

      os << std::setw(30) << domain << " | " << std::setw(10) << count << " | "
         << std::setw(15) << std::fixed << std::setprecision(2) << avg_time
         << "\n";
    }
  } else {
    os << "No HTTP request data available.\n";
  }

  os << "==========================================\n";
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
             : 0; // КБ/с
}

void MetricsCollector::add_bytes_downloaded(size_t bytes) {
  total_bytes_downloaded_ += bytes;
}