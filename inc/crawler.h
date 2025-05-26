#pragma once
#include "../libs/parallel_scheduler/parallel_scheduler.h"
#include "crawler_config.h"
#include "database.h"
#include "htmlparser.h"
#include "includes.h"
#include "metrics_collector.h"
#include "robots_parser.h"
#include "url_priority.h"
#include "url_utils.h"
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>

class Crawler {
public:
  explicit Crawler(const CrawlerConfig &config = CrawlerConfig());
  ~Crawler();
  void load_links_from_file(const std::string &filename);
  void run(size_t size);

  void print_performance_report(std::ostream &os = std::cout);
  void reset_metrics();

private:
  bool url_matches_keywords(const std::string &url);

  void process_links(size_t size);
  void process(const std::string &current_link, int depth = 0);
  void process_remaining_links();
  bool fetch_page(const std::string &url, std::string &content);
  void parse_page(const std::string &content,
                  std::unordered_set<std::string> &links, std::string &text,
                  int mode, const std::string &base_url);
  void save_to_database(const std::string &url, const std::string &text);
  bool fetch_page_with_retry(const std::string &url, std::string &content,
                             int max_retries = 3, int retry_delay_sec = 5);
  bool fetch_page_with_http_code(const std::string &url, std::string &content,
                                 long *http_code);
  void add_to_queue(const std::string &url, int depth, double priority = 0.0);

  void start_metrics_reporting();
  void stop_metrics_reporting();

  std::atomic<bool> metrics_running_{false};
  std::unique_ptr<std::thread> metrics_thread_;
  CrawlerConfig config;
  std::ofstream log_file;

  std::priority_queue<UrlItem> link_queue;

  std::unordered_map<std::string, int> url_depths;

  std::unordered_set<std::string> visited_links;
  std::unordered_set<std::string> main_links;
  std::mutex queue_mutex;
  Database db;
  HTMLParser parser;
  parallel_scheduler *scheduler;
  size_t links_size;
  std::mutex task_mutex;
  std::condition_variable task_cv;
  size_t active_tasks = 0;
  RobotsParser robots_parser;
  std::string user_agent;
  std::unordered_map<std::string,
                     std::chrono::time_point<std::chrono::steady_clock>>
      domain_last_access;
  std::mutex domain_access_mutex;
};