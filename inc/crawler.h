#pragma once
#include "../libs/parallel_scheduler/parallel_scheduler.h"
#include "database.h"
#include "htmlparser.h"
#include "includes.h"
#include <condition_variable>
#include <mutex>

#define THREAD_COUNT 10
#define MAX_LINKS 500

class Crawler {
public:
  Crawler(size_t thread_count = THREAD_COUNT);
  ~Crawler();
  void load_links_from_file(const std::string &filename);
  void run(size_t size = MAX_LINKS);

private:
  void process_links(size_t size = MAX_LINKS);
  void process(const std::string &current_link);
  void process_remaining_links();
  void fetch_page(const std::string &url, std::string &content);
  void parse_page(const std::string &content,
                  std::unordered_set<std::string> &links, std::string &text,
                  int mode);
  void save_to_database(const std::string &url, const std::string &text);

  std::queue<std::string> link_queue;
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
};