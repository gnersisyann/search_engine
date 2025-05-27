#include "../../inc/robots_parser.h"
#include "../../inc/url_utils.h"
#include <curl/curl.h>
#include <iostream>
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

static std::string trim_whitespace(const std::string &str) {
  size_t first = str.find_first_not_of(" \t\n\r");
  if (first == std::string::npos) {
    return "";
  }
  size_t last = str.find_last_not_of(" \t\n\r");
  return str.substr(first, last - first + 1);
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

    line = trim_whitespace(line);
    if (line.empty()) {
      continue;
    }

    if (line.substr(0, 10) == "User-agent" ||
        line.substr(0, 10) == "user-agent") {
      size_t colon = line.find(':');
      if (colon != std::string::npos && colon + 1 < line.size()) {
        current_agent = line.substr(colon + 1);
        current_agent = trim_whitespace(current_agent);

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
        path = trim_whitespace(path);

        if (!path.empty()) {
          robots_cache[domain][current_agent].disallow_rules.push_back(path);
        }
      }
    } else if (line.substr(0, 5) == "Allow" || line.substr(0, 5) == "allow") {
      size_t colon = line.find(':');
      if (colon != std::string::npos && colon + 1 < line.size()) {
        std::string path = line.substr(colon + 1);
        path = trim_whitespace(path);

        if (!path.empty()) {
          robots_cache[domain][current_agent].allow_rules.push_back(path);
        }
      }
    } else if (line.substr(0, 11) == "Crawl-delay" ||
               line.substr(0, 11) == "crawl-delay") {
      size_t colon = line.find(':');
      if (colon != std::string::npos && colon + 1 < line.size()) {
        std::string delay_str = line.substr(colon + 1);
        delay_str = trim_whitespace(delay_str);

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

  if (pattern.find('*') == std::string::npos) {
    return url.find(pattern) == 0;
  }

  size_t url_i = 0;
  size_t pattern_i = 0;
  size_t star_match = std::string::npos;
  size_t star_idx = std::string::npos;

  while (url_i < url.size()) {
    if (pattern_i < pattern.size() &&
        (pattern[pattern_i] == '?' || pattern[pattern_i] == url[url_i])) {
      url_i++;
      pattern_i++;
    } else if (pattern_i < pattern.size() && pattern[pattern_i] == '*') {
      star_match = url_i;
      star_idx = pattern_i;
      pattern_i++;
    } else if (star_idx != std::string::npos) {
      pattern_i = star_idx + 1;
      star_match++;
      url_i = star_match;
    } else {
      return false;
    }
  }

  while (pattern_i < pattern.size() && pattern[pattern_i] == '*') {
    pattern_i++;
  }

  return pattern_i == pattern.size();
}