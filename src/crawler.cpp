#include "../inc/crawler.h"
#include <fstream>
#include <memory>

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            std::string *output) {
  size_t totalSize = size * nmemb;
  output->append((char *)contents, totalSize);
  return totalSize;
}

Crawler::Crawler(size_t thread_count) {
  scheduler = parallel_scheduler_create(thread_count);
}

Crawler::~Crawler() {
  if (scheduler)
    parallel_scheduler_destroy(scheduler);
  scheduler = nullptr;
}

void Crawler::load_links_from_file(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "File open error: " << filename << "\n";
    return;
  }
  std::string link;
  while (std::getline(file, link)) {
    if (!link.empty() && visited_links.find(link) == visited_links.end()) {
      link_queue.push(link);
      visited_links.insert(link);
    }
  }
}

void Crawler::process(const std::string &current_link) {
  std::string content;
  std::unordered_set<std::string> links;
  std::string text;

  fetch_page(current_link, content);

  parse_page(content, links, text);

  save_to_database(current_link, text);

  for (const auto &link : links) {
    if (visited_links.find(link) == visited_links.end()) {
      link_queue.push(link);
      visited_links.insert(link);
    }
  }
}

void Crawler::process_links(size_t size) {
  while (!link_queue.empty() && visited_links.size() < size) {
    std::string current_link;

    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      if (link_queue.empty()) {
        break;
      }
      current_link = link_queue.front();
      link_queue.pop();
    }

    auto task_data =
        std::make_unique<std::pair<Crawler *, std::string>>(this, current_link);

    parallel_scheduler_run(
        scheduler,
        [](void *arg) {
          auto task_data = std::unique_ptr<std::pair<Crawler *, std::string>>(
              static_cast<std::pair<Crawler *, std::string> *>(arg));
          Crawler *crawler = task_data->first;
          std::string link = task_data->second;

          crawler->process(link);
        },
        task_data.release());
  }
}

void Crawler::fetch_page(const std::string &url, std::string &content) {
  CURL *curl = curl_easy_init();

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
  };
}

void Crawler::parse_page(const std::string &content,
                         std::unordered_set<std::string> &links,
                         std::string &text) {
  if (content.empty()) {
    links.clear();
    text.clear();
    return;
  }
  links = parser.extract_links(content);
  text = parser.extract_text(content);
}

void Crawler::save_to_database(const std::string &url,
                               const std::string &text) {
  if (!db.is_url_processed(url)) {
    db.insert_page(url, text);
  }
}
