#include "../../inc/url_priority.h"
#include "../../inc/url_utils.h"
#include <cstring>
#include <unordered_map>


static const std::unordered_map<std::string, double> keyword_weights = {
    {"about", 1.5},   {"index", 1.2},   {"main", 1.2},    {"home", 1.1},
    {"contact", 0.8}, {"product", 1.3}, {"service", 1.3}, {"blog", 0.9},
    {"news", 1.0},    {"article", 0.9}};

double UrlPrioritizer::calculate_priority(const std::string &url, int depth,
                                          const std::string &) {
  double priority = 0.0;

  
  priority += keyword_score(url);
  priority += depth_score(depth);
  priority += domain_score(url);

  
  priority += domain_keyword_score(url);

  return priority;
}

double UrlPrioritizer::keyword_score(const std::string &url) {
  double score = 1.0; 

  std::string url_lower = url;
  std::transform(url_lower.begin(), url_lower.end(), url_lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  
  for (const auto &keyword_pair : keyword_weights) {
    std::string keyword_lower = keyword_pair.first;

    
    for (size_t pos = 0; pos < url_lower.length();) {
      pos = url_lower.find(keyword_lower, pos);
      if (pos == std::string::npos) {
        break;
      }

      
      bool is_word_start = (pos == 0 || !std::isalnum(url_lower[pos - 1]));
      bool is_word_end =
          (pos + keyword_lower.length() == url_lower.length() ||
           !std::isalnum(url_lower[pos + keyword_lower.length()]));

      if (is_word_start && is_word_end) {
        score *= keyword_pair.second;
        break;
      }

      pos += 1;
    }
  }

  
  if (url.find_last_of("/") == url.find("://") + 2) {
    score *= 1.5;
  }

  return score;
}

double UrlPrioritizer::depth_score(int depth) {
  
  return std::max(0.1, 2.0 / (depth + 1.0));
}

double UrlPrioritizer::domain_score(const std::string &url) {
  
  

  if (url.find(".org") != std::string::npos) {
    return 1.2;
  } else if (url.find(".edu") != std::string::npos) {
    return 1.3;
  } else if (url.find(".gov") != std::string::npos) {
    return 1.4;
  }

  return 1.0; 
}

double UrlPrioritizer::domain_keyword_score(const std::string &url) {
  std::string current_domain = UrlUtils::extract_domain(url);
  double score = 0.0;

  
  if (config_.domain_keywords.count(current_domain) > 0) {
    for (const auto &keyword : config_.domain_keywords.at(current_domain)) {
      
      if (url_contains_keyword(url, keyword)) {
        score += config_.domain_keyword_weight;
      }
    }
  }

  
  for (const auto &domain_entry : config_.domain_keywords) {
    
    if (domain_entry.first == current_domain)
      continue;

    for (const auto &keyword : domain_entry.second) {
      
      if (url_contains_keyword(url, keyword)) {
        score += config_.cross_domain_keyword_weight;
      }
    }
  }

  return score;
}

bool UrlPrioritizer::url_contains_keyword(const std::string &url,
                                          const std::string &keyword) {
  
  std::string url_lower = url;
  std::transform(url_lower.begin(), url_lower.end(), url_lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  std::string keyword_lower = keyword;
  std::transform(keyword_lower.begin(), keyword_lower.end(),
                 keyword_lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  
  for (size_t pos = 0; pos < url_lower.length();) {
    pos = url_lower.find(keyword_lower, pos);
    if (pos == std::string::npos) {
      break;
    }

    
    bool is_word_start = (pos == 0 || !std::isalnum(url_lower[pos - 1]));
    bool is_word_end = (pos + keyword_lower.length() == url_lower.length() ||
                        !std::isalnum(url_lower[pos + keyword_lower.length()]));

    if (is_word_start && is_word_end) {
      return true;
    }

    pos += 1; 
  }

  return false;
}