#include "../../inc/crawler.h"
#include "../../inc/crawler_config.h"
#include <fstream>
#include <iostream>

CrawlerConfig config;

int main(int argc, char *argv[]) {
  try {

    if (argc > 1) {
      config = CrawlerConfig::load_from_file(argv[1]);
    }

    Crawler crawler(config);

    if (argc > 2) {
      crawler.load_links_from_file(argv[2]);
    } else {
      crawler.load_links_from_file("links.txt");
    }

    crawler.run(config.max_links);

    std::cout << "\n===== Final Performance Report =====\n";
    crawler.print_performance_report(std::cout);

    std::ofstream report_file("performance_report.txt");
    if (report_file.is_open()) {
      crawler.print_performance_report(report_file);
      std::cout << "Performance report saved to 'performance_report.txt'\n";
    }

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}