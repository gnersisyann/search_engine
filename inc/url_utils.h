#pragma once
#include <algorithm>
#include <regex>
#include <string>

class UrlUtils {
public:
  static std::string normalize_url(const std::string &url);

  static std::string make_absolute_url(const std::string &base_url,
                                       const std::string &relative_url);

  static bool is_same_domain(const std::string &url, const std::string &domain);

  static std::string extract_domain(const std::string &url);
};