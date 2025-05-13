#include "../../inc/crawler_config.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp> // Требуется библиотека JSON для C++

using json = nlohmann::json;

CrawlerConfig CrawlerConfig::load_from_file(const std::string &filename) {
  CrawlerConfig config;

  try {
    // Открываем файл
    std::ifstream file(filename);
    if (!file.is_open()) {
      std::cerr << "Could not open config file: " << filename << std::endl;
      return config; // Возвращаем дефолтные значения
    }

    // Парсим JSON
    json j;
    file >> j;

    // Загружаем значения из JSON, используя значения по умолчанию при
    // отсутствии поля
    if (j.contains("thread_count"))
      config.thread_count = j["thread_count"];

    if (j.contains("db_name"))
      config.db_name = j["db_name"];

    if (j.contains("user_agent"))
      config.user_agent = j["user_agent"];

    if (j.contains("request_timeout_sec"))
      config.request_timeout_sec = j["request_timeout_sec"];

    if (j.contains("max_links"))
      config.max_links = j["max_links"];

    if (j.contains("max_retries"))
      config.max_retries = j["max_retries"];

    if (j.contains("retry_delay_sec"))
      config.retry_delay_sec = j["retry_delay_sec"];

    if (j.contains("log_filename"))
      config.log_filename = j["log_filename"];

    if (j.contains("verbose_logging"))
      config.verbose_logging = j["verbose_logging"];

    if (j.contains("domain_keywords")) {
      for (auto it = j["domain_keywords"].begin();
           it != j["domain_keywords"].end(); ++it) {
        std::string domain = it.key();
        if (it.value().is_array()) {
          for (const auto &keyword : it.value()) {
            if (keyword.is_string()) {
              config.domain_keywords[domain].push_back(
                  keyword.get<std::string>());
            }
          }
        }
      }
    }

    if (j.contains("domain_keyword_weight"))
      config.domain_keyword_weight = j["domain_keyword_weight"];

    if (j.contains("cross_domain_keyword_weight"))
      config.cross_domain_keyword_weight = j["cross_domain_keyword_weight"];

  } catch (const std::exception &e) {
    std::cerr << "Error loading config: " << e.what() << std::endl;
    // Возвращаем дефолтные значения в случае ошибки
  }

  return config;
}