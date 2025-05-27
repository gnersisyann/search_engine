#pragma once

#include "database.h"
#include "includes.h"

class Searcher {
public:
  Searcher(char *db);
  ~Searcher();
  std::vector<std::string> search(const std::string &query);

private:
  Database db;
};