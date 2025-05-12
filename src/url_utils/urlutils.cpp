#include "../../inc/url_utils.h"

std::string UrlUtils::normalize_url(const std::string &url) {
  std::string normalized = url;

  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  if (normalized.find("http://") != 0 && normalized.find("https://") != 0) {
    normalized = "http://" + normalized;
  }

  size_t fragment_pos = normalized.find('#');
  if (fragment_pos != std::string::npos) {
    normalized = normalized.substr(0, fragment_pos);
  }

  if (normalized.size() > 8 && normalized.back() == '/' &&
      std::count(normalized.begin() + 8, normalized.end(), '/') == 1) {
    normalized.pop_back();
  }

  static const std::vector<std::string> index_files = {
      "/index.html", "/index.php", "/index.htm", "/default.html"};

  for (const auto &index : index_files) {
    if (normalized.size() >= index.size() &&
        normalized.substr(normalized.size() - index.size()) == index) {
      normalized = normalized.substr(0, normalized.size() - index.size());
      if (normalized.back() != '/') {
        normalized += '/';
      }
      break;
    }
  }

  std::regex multi_slash("/{2,}");
  normalized = std::regex_replace(normalized, multi_slash, "/");

  return normalized;
}

std::string UrlUtils::make_absolute_url(const std::string &base_url,
                                        const std::string &relative_url) {
  if (relative_url.find("http://") == 0 || relative_url.find("https://") == 0) {
    return normalize_url(relative_url);
  }

  if (relative_url.size() >= 2 && relative_url[0] == '/' &&
      relative_url[1] == '/') {
    size_t proto_end = base_url.find("://");
    if (proto_end != std::string::npos) {
      std::string protocol = base_url.substr(0, proto_end);
      return normalize_url(protocol + ":" + relative_url);
    }
    return normalize_url("http:" + relative_url);
  }

  std::string base = normalize_url(base_url);

  if (!relative_url.empty() && relative_url[0] == '/') {
    size_t proto_end = base.find("://");
    if (proto_end == std::string::npos) {
      return normalize_url(base + relative_url);
    }

    proto_end += 3;
    size_t domain_end = base.find('/', proto_end);
    if (domain_end == std::string::npos) {
      return normalize_url(base + relative_url);
    } else {
      return normalize_url(base.substr(0, domain_end) + relative_url);
    }
  }

  size_t last_slash = base.find_last_of('/');
  if (last_slash != std::string::npos && last_slash > 8) {
    base = base.substr(0, last_slash + 1);
  } else if (base.back() != '/') {
    base += '/';
  }

  return normalize_url(base + relative_url);
}

std::string UrlUtils::extract_domain(const std::string &url) {
  size_t domain_start = url.find("://");
  if (domain_start == std::string::npos) {
    domain_start = 0;
  } else {
    domain_start += 3;
  }

  size_t domain_end = url.find('/', domain_start);
  if (domain_end == std::string::npos) {
    domain_end = url.size();
  }

  return url.substr(domain_start, domain_end - domain_start);
}

bool UrlUtils::is_same_domain(const std::string &url,
                              const std::string &domain) {
  std::string url_domain = extract_domain(url);

  if (url_domain == domain) {
    return true;
  }

  if (url_domain.size() > domain.size() + 1 &&
      url_domain.substr(url_domain.size() - domain.size() - 1) ==
          ("." + domain)) {
    return true;
  }

  return false;
}