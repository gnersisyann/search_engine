#include "../../inc/searcher.h"

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Invalid arguments, use ./searcher [db] [query]" << std::endl;
    exit(EXIT_FAILURE);
  }
  Searcher searcher(argv[1]);
  std::vector<std::string> links = searcher.search(argv[2]);
  for (const auto &el : links)
    std::cout << el << std::endl;
}