#include "../inc/crawler.h"
#include <string>

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Invalid arguments, use ./crawler [filename].txt" << std::endl;
    exit(EXIT_FAILURE);
  }
  Crawler crawler(THREAD_COUNT);
  std::string filename = argv[1];
  std::cerr << "Starting Crawler with THREAD_COUNT=" << THREAD_COUNT
            << " and MAX_LINKS=" << MAX_LINKS << std::endl;
  std::cerr << "Loading links from file: " << filename << std::endl;
  crawler.load_links_from_file(filename);
  crawler.process_links(MAX_LINKS);
}