#pragma once
#include <string>
#include <unordered_map>
#include <vector>

struct CrawlerConfig {
  // Параметры потоков
  size_t thread_count = 10;

  // Параметры базы данных
  std::string db_name = "parser.db";

  // Параметры HTTP
  std::string user_agent = "MyWebCrawler/1.0";
  int request_timeout_sec = 30;

  // Параметры обхода
  size_t max_links = 1000;

  // Параметры повторных попыток
  int max_retries = 3;
  int retry_delay_sec = 5;

  // Параметры логирования
  std::string log_filename = "logs.txt";
  bool verbose_logging = true;

  // Ключевые слова для доменов (домен -> список ключевых слов)
  std::unordered_map<std::string, std::vector<std::string>> domain_keywords;

  // Вес для доменных ключевых слов (насколько увеличивать приоритет)
  double domain_keyword_weight = 3.0;

  // Вес для кросс-доменных ключевых слов
  double cross_domain_keyword_weight = 1.5;

  // Загружает конфигурацию из файла (если поддерживается)
  static CrawlerConfig load_from_file(const std::string &filename);
};