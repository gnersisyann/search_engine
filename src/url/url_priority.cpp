#include "../../inc/url_priority.h"
#include "../../inc/url_utils.h"
#include <regex>
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

  // Проверяем наличие ключевых слов в URL
  for (const auto &keyword_pair : keyword_weights) {
    std::regex keyword_regex("\\b" + keyword_pair.first + "\\b",
                             std::regex::icase);
    if (std::regex_search(url, keyword_regex)) {
      score *= keyword_pair.second;
    }
  }

  // Домашняя страница имеет высокий приоритет
  if (url.find_last_of("/") == url.find("://") + 2) {
    score *= 1.5; // Домашняя страница (например, example.com/)
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
  // Приводим URL и ключевое слово к нижнему регистру для регистронезависимого
  // сравнения
  std::string url_lower = url;
  std::transform(url_lower.begin(), url_lower.end(), url_lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  std::string keyword_lower = keyword;
  std::transform(keyword_lower.begin(), keyword_lower.end(),
                 keyword_lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  // Используем regex для поиска слова как отдельного компонента
  std::regex word_regex("\\b" + keyword_lower + "\\b");
  return std::regex_search(url_lower, word_regex);
}