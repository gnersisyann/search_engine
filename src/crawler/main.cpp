#include "../../inc/crawler.h"


int count_lines_in_file(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Unable to open file " << filename;
    return -1;
  }
  return std::count(std::istreambuf_iterator<char>(file),
                    std::istreambuf_iterator<char>(), '\n') +
         1;
}

int main(int argc, char **argv) {

  if (argc != 2 && argc != 3) {
    std::cerr << "Invalid arguments, use ./crawler [filename].txt "
                 "[max_links]{optional}"
              << std::endl;
    exit(EXIT_FAILURE);
  }

  int links = MAX_LINKS;
  if (argc == 3) {
    links = std::stoi(argv[2]);
  }

  int line_count = count_lines_in_file(argv[1]);
  if (line_count == -1) {
    std::cerr << "Error: Unable to read file " << argv[1] << std::endl;
    exit(EXIT_FAILURE);
  }

  if (argc == 3 && links <= line_count) {
    std::cerr << "Invalid max_link count, must be greater than the number of "
                 "lines in "
              << argv[1] << std::endl;
    exit(EXIT_FAILURE);
  }

  Crawler crawler(THREAD_COUNT);
  std::string filename = argv[1];
  std::cerr << "Starting Crawler with THREAD_COUNT=" << THREAD_COUNT
            << " and MAX_LINKS=" << links << std::endl;
  crawler.load_links_from_file(filename);
  crawler.run(links);
}