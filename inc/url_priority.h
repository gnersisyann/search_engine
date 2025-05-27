#pragma once
#include "crawler_config.h"
#include <functional>
#include <string>

struct UrlItem {
  std::string url;
  int depth;
  double priority;

  UrlItem(const std::string &url, int depth = 0, double priority = 0.0)
      : url(url), depth(depth), priority(priority) {}

  bool operator<(const UrlItem &other) const {
    return priority < other.priority;
  }
};

class UrlPrioritizer {
public:
  UrlPrioritizer(const CrawlerConfig &config) : config_(config) {}

  double calculate_priority(const std::string &url, int depth,
                            const std::string &content = "");

private:
  const CrawlerConfig &config_;

  double keyword_score(const std::string &url);
  double depth_score(int depth);
  double domain_score(const std::string &url);
  double domain_keyword_score(const std::string &url);
  bool url_contains_keyword(const std::string &url, const std::string &keyword);
};