#include "../../inc/searcher.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Invalid arguments, use ./searcher [query]" << std::endl;
    exit(EXIT_FAILURE);
  }
  Searcher searcher;
  std::vector<std::string> links = searcher.search(argv[1]);
  for (const auto &el : links)
    std::cout << el << std::endl;
}