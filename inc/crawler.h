#pragma once
#include "database.h"
#include "htmlparser.h"
#include "includes.h"
#include <mutex>

#define THREAD_COUNT 8

class Crawler {
public:
  Crawler(size_t thread_count = THREAD_COUNT);
  ~Crawler();
  void load_links_from_file(const std::string &filename);
  void process_links(size_t size);
  void process(const std::string &current_link);

private:
  void fetch_page(const std::string &url, std::string &content);
  void parse_page(const std::string &content,
                  std::unordered_set<std::string> &links, std::string &text);
  void save_to_database(const std::string &url, const std::string &text);

  std::queue<std::string> link_queue;
  std::unordered_set<std::string> visited_links;
  std::mutex queue_mutex;
  Database db;
  HTMLParser parser;
  parallel_scheduler *scheduler;
};