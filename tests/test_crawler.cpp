#include "catch2/catch.hpp"
#include "../inc/crawler.h"
#include "../inc/crawler_config.h"
#include <string>
#include <fstream>

// Вспомогательная функция для создания тестового файла со ссылками
void create_test_links_file(const std::string& path, const std::vector<std::string>& links) {
    std::ofstream file(path);
    REQUIRE(file.is_open());
    
    for (const auto& link : links) {
        file << link << std::endl;
    }
    
    file.close();
}

// Вспомогательная функция для проверки существования и удаления файла
bool file_exists_and_delete(const std::string& path) {
    std::ifstream file(path);
    bool exists = file.good();
    file.close();
    if (exists) {
        std::remove(path.c_str());
    }
    return exists;
}

TEST_CASE("Crawler configuration", "[crawler]") {
    CrawlerConfig config;
    
    SECTION("Default configuration") {
        CHECK(config.thread_count == 10);
        CHECK(config.db_name == "parser.db");
        CHECK(config.max_links == 1000);
        CHECK(config.user_agent == "MyWebCrawler/1.0");
    }
    
    SECTION("Load custom configuration") {
        // Создаем тестовый файл конфигурации
        std::ofstream config_file("test_config.json");
        REQUIRE(config_file.is_open());
        
        config_file << R"({
            "thread_count": 4,
            "db_name": "test_db.db",
            "max_links": 100,
            "user_agent": "TestCrawler/1.0",
            "domain_keywords": {
                "example.com": ["test", "example"]
            }
        })";
        config_file.close();
        
        // Загружаем конфигурацию
        config = CrawlerConfig::load_from_file("test_config.json");
        
        // Проверяем значения
        CHECK(config.thread_count == 4);
        CHECK(config.db_name == "test_db.db");
        CHECK(config.max_links == 100);
        CHECK(config.user_agent == "TestCrawler/1.0");
        
        // Проверяем доменные ключевые слова
        CHECK(config.domain_keywords.size() == 1);
        CHECK(config.domain_keywords.count("example.com") == 1);
        CHECK(config.domain_keywords.at("example.com").size() == 2);
        CHECK(config.domain_keywords.at("example.com")[0] == "test");
        CHECK(config.domain_keywords.at("example.com")[1] == "example");
        
        // Удаляем тестовый файл
        std::remove("test_config.json");
    }
}

TEST_CASE("Crawler initialization", "[crawler]") {
    // Создаем конфигурацию для теста
    CrawlerConfig config;
    config.db_name = "test_crawler.db";
    config.log_filename = "test_logs.txt";
    
    SECTION("Basic initialization") {
        // Создаем экземпляр краулера
        Crawler crawler(config);
        
        // Проверяем, что файлы созданы
        CHECK(file_exists_and_delete("test_logs.txt"));
        CHECK(file_exists_and_delete("test_crawler.db"));
    }
}

TEST_CASE("Loading links from file", "[crawler]") {
    // Создаем конфигурацию для теста
    CrawlerConfig config;
    config.db_name = "test_crawler.db";
    config.log_filename = "test_logs.txt";
    
    // Создаем тестовый файл со ссылками
    std::vector<std::string> test_links = {
        "http://example.com",
        "http://test.com",
        "https://example.org"
    };
    create_test_links_file("test_links.txt", test_links);
    
    SECTION("Load links") {
        Crawler crawler(config);
        crawler.load_links_from_file("test_links.txt");
        
        // Мы не можем напрямую проверить private члены,
        // но можем проверить создание файлов и другие побочные эффекты
        CHECK(file_exists_and_delete("test_logs.txt"));
        
        // Удаляем тестовый файл со ссылками
        std::remove("test_links.txt");
    }
}

// Для более сложного тестирования crawler необходимо создавать mock объекты
// или интеграционные тесты с реальным сервером. Это выходит за рамки базового тестирования.