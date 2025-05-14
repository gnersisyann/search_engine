#include "../../inc/url_priority.h"
#include "../../inc/url_utils.h"
#include <cstring>
#include <unordered_map>

// Ключевые слова, которые могут указывать на важные страницы
static const std::unordered_map<std::string, double> keyword_weights = {
    {"about", 1.5},   {"index", 1.2},   {"main", 1.2},    {"home", 1.1},
    {"contact", 0.8}, {"product", 1.3}, {"service", 1.3}, {"blog", 0.9},
    {"news", 1.0},    {"article", 0.9}};

double UrlPrioritizer::calculate_priority(const std::string &url, int depth,
                                          const std::string &) {
  double priority = 0.0;

  // Базовые критерии приоритета
  priority += keyword_score(url);
  priority += depth_score(depth);
  priority += domain_score(url);

  // Проверка доменных ключевых слов
  priority += domain_keyword_score(url);

  return priority;
}

double UrlPrioritizer::keyword_score(const std::string &url) {
  double score = 1.0; // Базовый приоритет

  std::string url_lower = url;
  std::transform(url_lower.begin(), url_lower.end(), url_lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  // Проверяем наличие ключевых слов в URL без регулярных выражений
  for (const auto &keyword_pair : keyword_weights) {
    std::string keyword_lower = keyword_pair.first;

    // Ищем ключевое слово как целое слово
    for (size_t pos = 0; pos < url_lower.length();) {
      pos = url_lower.find(keyword_lower, pos);
      if (pos == std::string::npos) {
        break;
      }

      // Проверяем границы слова
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

  // Домашняя страница имеет высокий приоритет
  if (url.find_last_of("/") == url.find("://") + 2) {
    score *= 1.5;
  }

  return score;
}

double UrlPrioritizer::depth_score(int depth) {
  // Чем меньше глубина, тем выше приоритет
  return std::max(0.1, 2.0 / (depth + 1.0));
}

double UrlPrioritizer::domain_score(const std::string &url) {
  // Здесь можно добавить логику для предпочтения определенных доменов
  // Например, можно повысить приоритет для URL с .org или .edu

  if (url.find(".org") != std::string::npos) {
    return 1.2;
  } else if (url.find(".edu") != std::string::npos) {
    return 1.3;
  } else if (url.find(".gov") != std::string::npos) {
    return 1.4;
  }

  return 1.0; // Базовый приоритет для остальных доменов
}

double UrlPrioritizer::domain_keyword_score(const std::string &url) {
  std::string current_domain = UrlUtils::extract_domain(url);
  double score = 0.0;

  // Проверяем ключевые слова для текущего домена
  if (config_.domain_keywords.count(current_domain) > 0) {
    for (const auto &keyword : config_.domain_keywords.at(current_domain)) {
      // URL этого же домена содержит ключевое слово - максимальный приоритет
      if (url_contains_keyword(url, keyword)) {
        score += config_.domain_keyword_weight;
      }
    }
  }

  // Проверяем кросс-доменные ключевые слова
  for (const auto &domain_entry : config_.domain_keywords) {
    // Пропускаем текущий домен, его уже обработали выше
    if (domain_entry.first == current_domain)
      continue;

    for (const auto &keyword : domain_entry.second) {
      // URL другого домена содержит ключевое слово - повышенный приоритет
      if (url_contains_keyword(url, keyword)) {
        score += config_.cross_domain_keyword_weight;
      }
    }
  }

  return score;
}

bool UrlPrioritizer::url_contains_keyword(const std::string &url,
                                          const std::string &keyword) {
  // Приводим URL и ключевое слово к нижнему регистру
  std::string url_lower = url;
  std::transform(url_lower.begin(), url_lower.end(), url_lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  std::string keyword_lower = keyword;
  std::transform(keyword_lower.begin(), keyword_lower.end(),
                 keyword_lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  // Поиск подстроки без регулярных выражений
  for (size_t pos = 0; pos < url_lower.length();) {
    pos = url_lower.find(keyword_lower, pos);
    if (pos == std::string::npos) {
      break;
    }

    // Проверяем, что ключевое слово является отдельным словом
    bool is_word_start = (pos == 0 || !std::isalnum(url_lower[pos - 1]));
    bool is_word_end = (pos + keyword_lower.length() == url_lower.length() ||
                        !std::isalnum(url_lower[pos + keyword_lower.length()]));

    if (is_word_start && is_word_end) {
      return true;
    }

    pos += 1; // Продолжаем поиск дальше
  }

  return false;
}