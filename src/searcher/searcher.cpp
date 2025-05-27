#include "../../inc/searcher.h"

Searcher::Searcher(char *database) { db.connect(database, SEARCHER); }

Searcher::~Searcher() {}

std::vector<std::string> Searcher::search(const std::string &query) {
  std::vector<std::string> results;

  if (query.empty()) {
    std::cerr << "Search query is empty." << std::endl;
    return results;
  }

  sqlite3_stmt *stmt;
  std::string sql = "SELECT url FROM pages WHERE content LIKE ?;";
  if (sqlite3_prepare_v2(db.get_db(), sql.c_str(), -1, &stmt, nullptr) !=
      SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db.get_db())
              << "\n";
    return results;
  }

  std::string like_query = "%" + query + "%";
  if (sqlite3_bind_text(stmt, 1, like_query.c_str(), -1, SQLITE_STATIC) !=
      SQLITE_OK) {
    std::cerr << "Failed to bind query: " << sqlite3_errmsg(db.get_db())
              << "\n";
    sqlite3_finalize(stmt);
    return results;
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *url =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    if (url) {
      results.push_back(url);
    }
  }

  sqlite3_finalize(stmt);
  return results;
}