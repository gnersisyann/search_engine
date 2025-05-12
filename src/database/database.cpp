#include "../../inc/database.h"
#include "../../inc/url_utils.h"

StmtGuard::StmtGuard(sqlite3_stmt *stmt) : stmt_(stmt) {}

StmtGuard::~StmtGuard() {
  if (stmt_) {
    sqlite3_finalize(stmt_);
  }
}

sqlite3_stmt *StmtGuard::get() { return stmt_; }

StmtGuard::operator sqlite3_stmt *() { return stmt_; }

void Database::connect(const std::string &db_name, int mode) {
  if (mode == CRAWLER)
    std::filesystem::remove(db_name.c_str());
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
  std::string normalized_url = UrlUtils::normalize_url(url);
  sqlite3_stmt *raw_stmt;
  std::string sql = "SELECT 1 FROM pages WHERE url = ? LIMIT 1;";
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &raw_stmt, nullptr) !=
      SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
    return false;
  }
  StmtGuard stmt(raw_stmt);

  if (sqlite3_bind_text(stmt, 1, normalized_url.c_str(), -1, SQLITE_STATIC) !=
      SQLITE_OK) {
    std::cerr << "Failed to bind URL: " << sqlite3_errmsg(db) << "\n";
    return false;
  }
  int result = sqlite3_step(stmt);
  bool exists = false;

  if (result == SQLITE_ROW)
    exists = true;
  else if (result != SQLITE_DONE)
    std::cerr << "Error executing query: " << sqlite3_errmsg(db) << "\n";

  return exists;
}

void Database::insert_page(const std::string &url, const std::string &text) {
  if (!db) {
    std::cerr << "Database is not connected." << std::endl;
    return;
  }
  std::string normalized_url = UrlUtils::normalize_url(url);
  sqlite3_stmt *raw_stmt;
  std::string sql = "INSERT INTO pages (url, content) VALUES (?, ?);";
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &raw_stmt, nullptr) !=
      SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
    return;
  }
  StmtGuard stmt(raw_stmt);

  if (sqlite3_bind_text(stmt, 1, normalized_url.c_str(), -1, SQLITE_STATIC) !=
      SQLITE_OK) {
    std::cerr << "Failed to bind URL: " << sqlite3_errmsg(db) << "\n";
    return;
  }
  if (sqlite3_bind_text(stmt, 2, text.c_str(), -1, SQLITE_STATIC) !=
      SQLITE_OK) {
    std::cerr << "Failed to bind content: " << sqlite3_errmsg(db) << "\n";
    return;
  }

  int result = sqlite3_step(stmt);
  if (result != SQLITE_DONE)
    std::cerr << "Error executing query: " << sqlite3_errmsg(db) << "\n";

  return;
}
Database::~Database() {
  if (db)
    sqlite3_close(db);
}
sqlite3 *Database::get_db() { return db; }