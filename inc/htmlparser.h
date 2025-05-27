#pragma once
#include <string>
#include <unordered_set>

class HTMLParser {
public:
    HTMLParser();
    ~HTMLParser();
    
    std::unordered_set<std::string> extract_links(const std::string& html);
    
    std::unordered_set<std::string> extract_links(const std::string& html, const std::string& base_url);
    
    std::string extract_text(const std::string& html);
};