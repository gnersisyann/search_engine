#include "../../inc/database.h"

void Database::connect(const std::string &db_name) {
  if (sqlite3_open(db_name.c_str(), &db) != SQLITE_OK) {
    std::cerr << "SQLite3 connection error: " << sqlite3_errmsg(db) << "\n";
    sqlite3_close(db);
    db = nullptr;
  }
}
void Database::create_table() {
  if (!db) {
    std::cerr << "Database is not connected." << std::endl;
    return;
  }
  char *err_msg = nullptr;
  std::string sql = "CREATE TABLE IF NOT EXISTS pages ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "url TEXT UNIQUE,"
                    "content TEXT );";
  if (sqlite3_exec(db, sql.c_str(), nullptr, 0, &err_msg) != SQLITE_OK) {
    std::cerr << "SQL error: " << err_msg << "\n";
    sqlite3_free(err_msg);
  }
}

bool Database::is_url_processed(const std::string &url) {
  if (!db) {
    std::cerr << "Database is not connected." << std::endl;
    return false;
  }
  sqlite3_stmt *stmt;
  std::string sql = "SELECT 1 FROM pages WHERE url = ? LIMIT 1;";
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
    return false;
  }
  if (sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) {
    std::cerr << "Failed to bind URL: " << sqlite3_errmsg(db) << "\n";
    sqlite3_finalize(stmt);
    return false;
  }
  int result = sqlite3_step(stmt);
  if (result != SQLITE_DONE) {
    std::cerr << "Error executing query: " << sqlite3_errmsg(db) << "\n";
  }
  sqlite3_finalize(stmt);
  return result == SQLITE_ROW;
}
void Database::insert_page(const std::string &url, const std::string &text) {
  if (!db) {
    std::cerr << "Database is not connected." << std::endl;
    return;
  }
  sqlite3_stmt *stmt;
  std::string sql = "INSERT INTO pages (url, content) VALUES (?, ?);";
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
    return;
  }
  if (sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) {
    std::cerr << "Failed to bind URL: " << sqlite3_errmsg(db) << "\n";
    sqlite3_finalize(stmt);
    return;
  }
  if (sqlite3_bind_text(stmt, 2, text.c_str(), -1, SQLITE_STATIC) !=
      SQLITE_OK) {
    std::cerr << "Failed to bind URL: " << sqlite3_errmsg(db) << "\n";
    sqlite3_finalize(stmt);
    return;
  }

  int result = sqlite3_step(stmt);
  if (result != SQLITE_DONE) {
    std::cerr << "Error executing query: " << sqlite3_errmsg(db) << "\n";
  }
  sqlite3_finalize(stmt);
  return;
}
Database::~Database() {
  if (db)
    sqlite3_close(db);
}