#pragma once
#include "includes.h"

#define CRAWLER 1
#define SEARCHER 0

class Database {
public:
  void connect(const std::string &db_name, int mode);
  void create_table();
  bool is_url_processed(const std::string &url);
  void insert_page(const std::string &url, const std::string &text);
  sqlite3 *get_db();
  ~Database();

private:
  sqlite3 *db;
};