#pragma once
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class RobotsParser {
public:
  RobotsParser();
  ~RobotsParser();

  bool is_allowed(const std::string &user_agent, const std::string &url);

  int get_crawl_delay(const std::string &user_agent, const std::string &domain);

private:
  struct RobotsData {
    std::vector<std::string> allow_rules;
    std::vector<std::string> disallow_rules;
    int crawl_delay = 0;
  };

  void fetch_robots_txt(const std::string &domain);

  bool matches_pattern(const std::string &url, const std::string &pattern);

  std::unordered_map<std::string, std::unordered_map<std::string, RobotsData>>
      robots_cache;

  std::mutex cache_mutex;
};