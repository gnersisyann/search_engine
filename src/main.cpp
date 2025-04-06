#include "../inc/crawler.h"
#include "../inc/database.h"
#include "../inc/htmlparser.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Invalid arguments, use ./crawler [filename].txt";
    exit(EXIT_FAILURE);
  }
}