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
    double total_time_ms = 0; // общее время операции в мс
    size_t count = 0;         // количество операций
    double min_time_ms = 0; // минимальное время операции
    double max_time_ms = 0; // максимальное время операции
    size_t error_count = 0; // количество ошибок

    // Для доменной статистики
    std::unordered_map<std::string, double> domain_times;
    std::unordered_map<std::string, size_t> domain_counts;
  };

  // Получение экземпляра синглтона
  static MetricsCollector &instance();

  // Запуск измерения времени для операции
  void start_timer(const std::string &operation, const std::string &url = "");

  // Остановка таймера и запись результата
  void stop_timer(const std::string &operation, bool success = true);

  // Прямая запись метрики
  void record_metric(const std::string &operation, double time_ms,
                     bool success = true, const std::string &domain = "");

  // Сброс всех метрик
  void reset();

  // Получение агрегированных метрик
  std::unordered_map<std::string, OperationMetrics> get_metrics();

  // Печать отчета
  void print_report(std::ostream &os = std::cout);

  // Счетчики активности
  void increment_active_threads();
  void decrement_active_threads();
  void set_queue_size(size_t size);
  void set_visited_count(size_t count);

  // Получение статистики производительности
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

  // Общая статистика
  std::atomic<size_t> active_threads_{0};
  std::atomic<size_t> queue_size_{0};
  std::atomic<size_t> visited_count_{0};
  std::chrono::high_resolution_clock::time_point start_time_;
  std::atomic<size_t> total_bytes_downloaded_{0};
};

#define MEASURE_TIME(operation) \
    class TimerGuard { \
    public: \
        TimerGuard(const std::string& op) : op_(op) { \
            MetricsCollector::instance().start_timer(op_); \
        } \
        ~TimerGuard() { \
            MetricsCollector::instance().stop_timer(op_); \
        } \
    private: \
        std::string op_; \
    } timer_guard_##__LINE__(operation);