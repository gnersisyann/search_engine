#include <curl/curl.h>
#include <fcntl.h>
#include <gumbo.h>
#include <iostream>
#include <queue>
#include <sqlite3.h>
#include <string>
#include <unistd.h>

std::string escape_sql_string(const std::string &str) {
  std::string result;
  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '\'')
      result += "''";
    else
      result += str[i];
  }
  return result;
}

void extract_text(GumboNode *node, std::string &text) {
  if (node->type == GUMBO_NODE_TEXT)
    text += node->v.text.text;
  else if (node->type == GUMBO_NODE_ELEMENT) {
    GumboVector *children = &node->v.element.children;
    for (size_t i = 0; i < children->length; ++i)
      extract_text(static_cast<GumboNode *>(children->data[i]), text);
  }
}

void execute_sql(sqlite3 *db, const std::string &sql) {
  char *errMsg = nullptr;
  if (sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg) != SQLITE_OK) {
    std::cerr << "Ошибка SQL: " << errMsg << std::endl;
    sqlite3_free(errMsg);
  }
}

void parse_html(const std::string &html, sqlite3 *db, const std::string &url) {
  GumboOutput *output = gumbo_parse(html.c_str());

  std::string extractedText;
  extract_text(output->root, extractedText);

  gumbo_destroy_output(&kGumboDefaultOptions, output);

  extractedText = escape_sql_string(extractedText);
  std::string escaped_url = escape_sql_string(url);

  std::string insert_sql = "INSERT INTO data (text, url) VALUES ('" +
                           extractedText + "', '" + escaped_url + "');";
  execute_sql(db, insert_sql);
}

size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                     std::string *output) {
  size_t totalSize = size * nmemb;
  output->append((char *)contents, totalSize);
  return totalSize;
}

std::string fetch_html(const std::string &url) {
  CURL *curl = curl_easy_init();
  std::string html;

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
  }

  return html;
}

int main() {
  int fd = open("link.txt", O_RDONLY);
  if (fd == -1)
    return (perror("File open error"), 1);
  std::queue<std::string> links;
  char buffer[4096];
  std::string temp;

  ssize_t bytes_read;
  while ((bytes_read = read(fd, buffer, 4096)) > 0) {
    for (ssize_t i = 0; i < bytes_read; i++) {
      if (buffer[i] == '\n') {
        links.push(temp);
        temp.clear();
      } else
        temp += buffer[i];
    }
  }

  if (!temp.empty())
    links.push(temp);

  if (links.empty())
    return (perror("There is no links"), 1);

  sqlite3 *db;
  if (sqlite3_open("parser.db", &db) != SQLITE_OK)
    return (perror(sqlite3_errmsg(db)), 1);

  execute_sql(db, "CREATE TABLE IF NOT EXISTS data (text TEXT, url TEXT);");

  while (!links.empty()) {
    std::string url = links.front();
    std::string html = fetch_html(url); // Загружаем HTML по ссылке
    parse_html(html, db, url);          // Парсим HTML и сохраняем данные в базу
    links.pop();
  }

  const char *select_sql = "SELECT * FROM data;";
  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db, select_sql, -1, &stmt, 0) != SQLITE_OK)
    return (perror(sqlite3_errmsg(db)), 1);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *text =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    const char *url =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    std::cout << "Text: " << text << ", URL: " << url << std::endl;
  }
  sqlite3_finalize(stmt);

  sqlite3_close(db);

  return 0;
}
