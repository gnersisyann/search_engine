#include "../../inc/htmlparser.h"
#include "../../inc/url_utils.h"
#include <regex>

HTMLParser::HTMLParser() {}
HTMLParser::~HTMLParser() {}

std::unordered_set<std::string>
HTMLParser::extract_links(const std::string &html) {
  return extract_links(html, "");
}

std::unordered_set<std::string>
HTMLParser::extract_links(const std::string &html,
                          const std::string &base_url) {
  std::unordered_set<std::string> links;

  std::regex link_regex("<a\\s+[^>]*href\\s*=\\s*['\"]([^'\"]+)['\"][^>]*>",
                        std::regex::icase);

  auto links_begin = std::sregex_iterator(html.begin(), html.end(), link_regex);
  auto links_end = std::sregex_iterator();

  for (std::sregex_iterator i = links_begin; i != links_end; ++i) {
    std::smatch match = *i;
    std::string relative_link = match[1].str();

    if (relative_link.empty() || relative_link[0] == '#' ||
        relative_link.find("javascript:") == 0 ||
        relative_link.find("mailto:") == 0) {
      continue;
    }

    try {
      std::string absolute_url = relative_link;
      if (!base_url.empty()) {
        absolute_url = UrlUtils::make_absolute_url(base_url, relative_link);
      }
      links.insert(absolute_url);
    } catch (const std::exception &) {
    }
  }

  return links;
}

std::string HTMLParser::extract_text(const std::string &html) {
  std::string text = html;

  std::regex script_regex("<script[^>]*>[\\s\\S]*?</script>",
                          std::regex::icase);
  std::regex style_regex("<style[^>]*>[\\s\\S]*?</style>", std::regex::icase);
  text = std::regex_replace(text, script_regex, "");
  text = std::regex_replace(text, style_regex, "");

  std::regex tag_regex("<[^>]*>");
  text = std::regex_replace(text, tag_regex, " ");

  std::regex whitespace_regex("[\\s]+");
  text = std::regex_replace(text, whitespace_regex, " ");

  text = std::regex_replace(text, std::regex("^\\s+|\\s+$"), "");

  return text;
}