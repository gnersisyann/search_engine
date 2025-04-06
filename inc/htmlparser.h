#pragma once
#include "includes.h"

class HTMLParser {
public:
  std::unordered_set<std::string> extract_links(const std::string &html);
  std::string extract_text(const std::string &html);
};