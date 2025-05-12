#include "../../inc/robots_parser.h"
#include "../../inc/url_utils.h"
#include <curl/curl.h>
#include <iostream>
#include <regex>
#include <sstream>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            std::string *output) {
  size_t totalSize = size * nmemb;
  output->append((char *)contents, totalSize);
  return totalSize;
}

RobotsParser::RobotsParser() {}
RobotsParser::~RobotsParser() {}

bool RobotsParser::is_allowed(const std::string &user_agent,
                              const std::string &url) {
  std::string domain = UrlUtils::extract_domain(url);
  if (domain.empty()) {
    return true;
  }

  std::string robots_url;
  if (url.find("https://") == 0) {
    robots_url = "https://" + domain + "/robots.txt";
  } else {
    robots_url = "http://" + domain + "/robots.txt";
  }

  {
    std::lock_guard<std::mutex> lock(cache_mutex);
    if (robots_cache.find(domain) == robots_cache.end()) {
      fetch_robots_txt(domain);
    }
  }

  std::string path = url.substr(url.find(domain) + domain.size());
  if (path.empty()) {
    path = "/";
  }

  bool allowed = true;
  {
    std::lock_guard<std::mutex> lock(cache_mutex);

    if (robots_cache[domain].find(user_agent) != robots_cache[domain].end()) {
      auto &rules = robots_cache[domain][user_agent];

      for (const auto &allow_rule : rules.allow_rules) {
        if (matches_pattern(path, allow_rule)) {
          return true;
        }
      }

      for (const auto &disallow_rule : rules.disallow_rules) {
        if (matches_pattern(path, disallow_rule)) {
          allowed = false;
          break;
        }
      }
    } else if (robots_cache[domain].find("*") != robots_cache[domain].end()) {
      auto &rules = robots_cache[domain]["*"];

      for (const auto &allow_rule : rules.allow_rules) {
        if (matches_pattern(path, allow_rule)) {
          return true;
        }
      }

      for (const auto &disallow_rule : rules.disallow_rules) {
        if (matches_pattern(path, disallow_rule)) {
          allowed = false;
          break;
        }
      }
    }
  }

  return allowed;
}

int RobotsParser::get_crawl_delay(const std::string &user_agent,
                                  const std::string &domain) {
  std::lock_guard<std::mutex> lock(cache_mutex);

  if (robots_cache.find(domain) == robots_cache.end()) {
    fetch_robots_txt(domain);
  }

  if (robots_cache[domain].find(user_agent) != robots_cache[domain].end()) {
    return robots_cache[domain][user_agent].crawl_delay;
  } else if (robots_cache[domain].find("*") != robots_cache[domain].end()) {
    return robots_cache[domain]["*"].crawl_delay;
  }

  return 0;
}

void RobotsParser::fetch_robots_txt(const std::string &domain) {
  std::string robots_url = "http://" + domain + "/robots.txt";

  CURL *curl = curl_easy_init();
  if (!curl) {
    std::cerr << "Failed to initialize CURL for robots.txt" << std::endl;
    return;
  }

  std::string content;
  curl_easy_setopt(curl, CURLOPT_URL, robots_url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
  CURLcode res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    robots_url = "https://" + domain + "/robots.txt";
    curl_easy_setopt(curl, CURLOPT_URL, robots_url.c_str());
    content.clear();
    res = curl_easy_perform(curl);
  }

  curl_easy_cleanup(curl);

  if (res != CURLE_OK || content.empty()) {
    robots_cache[domain]["*"] = RobotsData();
    return;
  }

  std::istringstream stream(content);
  std::string line;
  std::string current_agent = "*";

  while (std::getline(stream, line)) {
    size_t comment_pos = line.find('#');
    if (comment_pos != std::string::npos) {
      line = line.substr(0, comment_pos);
    }

    line = std::regex_replace(line, std::regex("^\\s+|\\s+$"), "");

    if (line.empty()) {
      continue;
    }

    if (line.substr(0, 10) == "User-agent" ||
        line.substr(0, 10) == "user-agent") {
      size_t colon = line.find(':');
      if (colon != std::string::npos && colon + 1 < line.size()) {
        current_agent = line.substr(colon + 1);
        current_agent =
            std::regex_replace(current_agent, std::regex("^\\s+|\\s+$"), "");

        if (robots_cache[domain].find(current_agent) ==
            robots_cache[domain].end()) {
          robots_cache[domain][current_agent] = RobotsData();
        }
      }
    } else if (line.substr(0, 8) == "Disallow" ||
               line.substr(0, 8) == "disallow") {
      size_t colon = line.find(':');
      if (colon != std::string::npos && colon + 1 < line.size()) {
        std::string path = line.substr(colon + 1);
        path = std::regex_replace(path, std::regex("^\\s+|\\s+$"), "");

        if (!path.empty()) {
          robots_cache[domain][current_agent].disallow_rules.push_back(path);
        }
      }
    } else if (line.substr(0, 5) == "Allow" || line.substr(0, 5) == "allow") {
      size_t colon = line.find(':');
      if (colon != std::string::npos && colon + 1 < line.size()) {
        std::string path = line.substr(colon + 1);
        path = std::regex_replace(path, std::regex("^\\s+|\\s+$"), "");

        if (!path.empty()) {
          robots_cache[domain][current_agent].allow_rules.push_back(path);
        }
      }
    } else if (line.substr(0, 11) == "Crawl-delay" ||
               line.substr(0, 11) == "crawl-delay") {
      size_t colon = line.find(':');
      if (colon != std::string::npos && colon + 1 < line.size()) {
        std::string delay_str = line.substr(colon + 1);
        delay_str =
            std::regex_replace(delay_str, std::regex("^\\s+|\\s+$"), "");

        try {
          int delay = std::stoi(delay_str);
          robots_cache[domain][current_agent].crawl_delay = delay;
        } catch (...) {
        }
      }
    }
  }
}

bool RobotsParser::matches_pattern(const std::string &url,
                                   const std::string &pattern) {
  if (pattern.find('*') != std::string::npos) {
    std::string regex_pattern =
        std::regex_replace(pattern, std::regex("\\*"), ".*");
    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\+"), "\\+");
    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\?"), "\\?");
    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\."), "\\.");
    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\("), "\\(");
    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\)"), "\\)");
    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\["), "\\[");
    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\]"), "\\]");
    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\{"), "\\{");
    regex_pattern = std::regex_replace(regex_pattern, std::regex("\\}"), "\\}");

    regex_pattern = "^" + regex_pattern;

    try {
      std::regex re(regex_pattern);
      return std::regex_match(url, re);
    } catch (...) {
      return url.find(pattern) == 0;
    }
  } else {
    return url.find(pattern) == 0;
  }
}