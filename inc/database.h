#pragma once
#include "includes.h"

class Database {
public:
  void connect(const std::string &db_name);
  void create_table();
  bool is_url_processed(const std::string &url);
  void insert_page(const std::string &url, const std::string &text);
  ~Database();

private:
  sqlite3 *db;
};