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

class StmtGuard {
public:
  StmtGuard(sqlite3_stmt *stmt);
  ~StmtGuard();

  sqlite3_stmt *get();
  operator sqlite3_stmt *();

private:
  sqlite3_stmt *stmt_;
  StmtGuard(const StmtGuard &) = delete;
  StmtGuard &operator=(const StmtGuard &) = delete;
};