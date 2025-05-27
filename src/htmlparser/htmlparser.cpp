#include "../../inc/htmlparser.h"
#include "../../inc/url_utils.h"

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

  size_t pos = 0;
  while ((pos = html.find("<a ", pos)) != std::string::npos) {

    size_t href_pos = html.find("href=", pos);
    if (href_pos == std::string::npos) {
      pos += 3;
      continue;
    }

    href_pos += 5;

    char quote = 0;
    if (href_pos < html.length() &&
        (html[href_pos] == '"' || html[href_pos] == '\'')) {
      quote = html[href_pos];
      href_pos++;
    } else {
      pos = href_pos;
      continue;
    }

    size_t end_quote = html.find(quote, href_pos);
    if (end_quote == std::string::npos) {
      pos = href_pos;
      continue;
    }

    std::string href = html.substr(href_pos, end_quote - href_pos);
    pos = end_quote + 1;

    if (href.empty() || href[0] == '#' || href.find("javascript:") == 0 ||
        href.find("mailto:") == 0) {
      continue;
    }

    std::string absolute_url = href;
    if (!base_url.empty()) {
      absolute_url = UrlUtils::make_absolute_url(base_url, href);
    }

    links.insert(absolute_url);
  }

  return links;
}

std::string HTMLParser::extract_text(const std::string &html) {
  std::string result;
  result.reserve(html.size() / 2);

  bool in_script = false;
  bool in_style = false;
  bool in_tag = false;
  bool last_was_whitespace = true;

  std::string current_tag;

  for (size_t i = 0; i < html.size(); ++i) {
    char c = html[i];

    if (c == '<') {
      in_tag = true;
      current_tag.clear();
      current_tag += c;
      continue;
    }

    if (in_tag) {
      current_tag += c;

      if (c == '>') {
        in_tag = false;

        std::string tag_lower = current_tag;
        std::transform(tag_lower.begin(), tag_lower.end(), tag_lower.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (tag_lower.find("<script") == 0) {
          in_script = true;
        } else if (tag_lower.find("</script") == 0) {
          in_script = false;
        } else if (tag_lower.find("<style") == 0) {
          in_style = true;
        } else if (tag_lower.find("</style") == 0) {
          in_style = false;
        } else if (!in_script && !in_style) {

          if (!last_was_whitespace) {
            result += ' ';
            last_was_whitespace = true;
          }
        }
      }
      continue;
    }

    if (in_script || in_style) {
      continue;
    }

    if (std::isspace(c)) {
      if (!last_was_whitespace) {
        result += ' ';
        last_was_whitespace = true;
      }
    } else {
      result += c;
      last_was_whitespace = false;
    }
  }

  size_t start = 0;
  while (start < result.size() && std::isspace(result[start])) {
    ++start;
  }

  size_t end = result.size();
  while (end > start && std::isspace(result[end - 1])) {
    --end;
  }

  return result.substr(start, end - start);
}