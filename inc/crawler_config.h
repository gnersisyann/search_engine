#pragma once
#include <string>
#include <unordered_map>
#include <vector>

struct CrawlerConfig {
  size_t thread_count = 10;

  std::string db_name = "parser.db";

  std::string user_agent = "MyWebCrawler/1.0";
  int request_timeout_sec = 30;

  size_t max_links = 1000;

  int max_retries = 3;
  int retry_delay_sec = 5;

  std::string log_filename = "logs.txt";
  bool verbose_logging = true;

  std::unordered_map<std::string, std::vector<std::string>> domain_keywords;

  double domain_keyword_weight = 3.0;

  double cross_domain_keyword_weight = 1.5;

  static CrawlerConfig load_from_file(const std::string &filename);
};