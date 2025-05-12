#include "../../inc/crawler.h"
#include <thread>

static std::ofstream log_file("logs.txt", std::ios::trunc);

#define LOG(msg)                                                               \
  if (log_file.is_open()) {                                                    \
    log_file << msg << std::endl;                                              \
  }

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            std::string *output) {
  size_t totalSize = size * nmemb;
  output->append((char *)contents, totalSize);
  return totalSize;
}

Crawler::Crawler(size_t thread_count) {
  scheduler = parallel_scheduler_create(thread_count);
  LOG("Creating parallel scheduler with thread count: " << thread_count);
  if (!scheduler) {
    LOG("Failed to create parallel scheduler");
    throw std::runtime_error("Failed to create parallel scheduler");
  }
  LOG("Parallel scheduler created successfully");

  db.connect("parser.db", CRAWLER);
  db.create_table();
  LOG("Database connected and table created.");
}

Crawler::~Crawler() {
  if (scheduler) {
    parallel_scheduler_destroy(scheduler);
  }
  scheduler = nullptr;
}

void Crawler::load_links_from_file(const std::string &filename) {
  LOG("Loading links from file: " << filename);
  std::ifstream file(filename);
  if (!file.is_open()) {
    LOG("Error: Unable to open file " << filename);
    return;
  }
  LOG("File opened successfully: " << filename);
  std::string link;
  while (std::getline(file, link)) {
    if (!link.empty() && visited_links.find(link) == visited_links.end()) {
      link_queue.push(link);
      visited_links.insert(link);
      main_links.insert(link);
    }
  }
}

static bool is_valid_domain(const std::string &link,
                            const std::string &domain) {
  try {
    return UrlUtils::is_same_domain(link, domain);
  } catch (const std::exception &e) {
    LOG("Error in is_valid_domain: " << e.what());
    return false;
  }
}

void Crawler::run(size_t size) {
  links_size = size;
  LOG("Running crawler with size limit: " << size);

  while (true) {
    process_links(size);

    {
      std::lock_guard<std::mutex> lock(queue_mutex);

      if (visited_links.size() >= size) {
        LOG("Visited links limit reached. Waiting for tasks to finish...");

        {
          std::unique_lock<std::mutex> lock(task_mutex);
          task_cv.wait(lock, [this]() { return active_tasks == 0; });
        }

        LOG("Processing remaining links...");
        for (int i = 0; i < THREAD_COUNT; ++i) {
          parallel_scheduler_run(
              scheduler,
              [](void *arg) {
                Crawler *crawler = static_cast<Crawler *>(arg);
                crawler->process_remaining_links();
              },
              this);
        }
        break;
      }

      if (link_queue.empty() && visited_links.size() < size) {
        LOG("No more links to process. Exiting...");
        break;
      }
    }
  }
}

void Crawler::process(const std::string &current_link) {
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    LOG("Processing link: " << current_link);
  }

  // Проверяем, разрешено ли посещать URL согласно robots.txt
  if (!robots_parser.is_allowed(user_agent, current_link)) {
    LOG("URL not allowed by robots.txt: " << current_link);

    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      visited_links.insert(current_link); // Помечаем URL как посещенный, чтобы
                                          // избежать повторной проверки
    }

    return;
  }

  // Соблюдаем crawl-delay для домена
  {
    std::string domain = UrlUtils::extract_domain(current_link);
    int crawl_delay = robots_parser.get_crawl_delay(user_agent, domain);

    if (crawl_delay > 0) {
      std::lock_guard<std::mutex> lock(domain_access_mutex);

      // Проверяем, когда последний раз был доступ к этому домену
      auto now = std::chrono::steady_clock::now();

      if (domain_last_access.find(domain) != domain_last_access.end()) {
        auto last_access = domain_last_access[domain];
        auto elapsed =
            std::chrono::duration_cast<std::chrono::seconds>(now - last_access)
                .count();

        if (elapsed < crawl_delay) {
          // Если время с последнего доступа меньше crawl-delay, то делаем паузу
          std::this_thread::sleep_for(
              std::chrono::seconds(crawl_delay - elapsed));
        }
      }

      // Обновляем время последнего доступа
      domain_last_access[domain] = std::chrono::steady_clock::now();
    }
  }

  std::string content;
  std::unordered_set<std::string> links;
  std::string text;

  fetch_page(current_link, content);
  parse_page(content, links, text, 1, current_link);
  save_to_database(current_link, text);

  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    LOG("Adding links to queue. Current link count: " << links.size());

    if (visited_links.size() < links_size) {
      for (const auto &link : links) {
        bool valid_domain = false;

        for (const auto &main_link : main_links) {
          if (is_valid_domain(link, main_link)) {
            valid_domain = true;
            break;
          }
        }

        if (valid_domain && visited_links.find(link) == visited_links.end()) {
          if (link_queue.size() < links_size) {
            LOG("Adding link to queue and visited: " << link);
            link_queue.push(link);
            visited_links.insert(link);
          }
        }
      }
    }

    visited_links.insert(current_link);

    LOG("Visited links count: " << visited_links.size());

    if (visited_links.size() >= links_size) {
      LOG("Visited links limit reached. Stopping processing...");
      return;
    }
  }
}

void Crawler::process_remaining_links() {
  LOG("Processing remaining links in the queue...");

  while (true) {
    std::string current_link;

    if (link_queue.empty()) {
      LOG("No more links in the queue to process.");
      break;
    }
    {
      std::lock_guard<std::mutex> lock(task_mutex);
      current_link = link_queue.front();
      link_queue.pop();
    }

    std::string content;
    std::unordered_set<std::string> links;
    std::string text;

    fetch_page(current_link, content);
    parse_page(content, links, text, 0, current_link);
    save_to_database(current_link, text);
    {
      std::lock_guard<std::mutex> lock(task_mutex);
      visited_links.insert(current_link);
      LOG("Processed remaining link: "
          << current_link << ", Visited links count: " << visited_links.size());
    }
  }

  LOG("Finished processing remaining links.");
}

void Crawler::process_links(size_t size) {
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    LOG("Starting to process links with size limit: " << size);
    LOG("Initial queue size: " << link_queue.size());
  }

  while (true) {
    std::string current_link;

    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      if (link_queue.empty() || visited_links.size() >= size) {
        break;
      }
      current_link = link_queue.front();
      link_queue.pop();
    }

    {
      std::lock_guard<std::mutex> lock(task_mutex);
      active_tasks++;
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

          {
            std::lock_guard<std::mutex> lock(crawler->task_mutex);
            crawler->active_tasks--;
            crawler->task_cv.notify_one();
          }
        },
        task_data.release());
  }

  {
    std::unique_lock<std::mutex> lock(task_mutex);
    task_cv.wait(lock, [this]() { return active_tasks == 0; });
  }
}

void Crawler::fetch_page(const std::string &url, std::string &content) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    LOG("Error: Failed to initialize CURL for URL: " << url);
    return;
  }
  LOG("CURL initialized successfully for URL: " << url);

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);

  // Устанавливаем User-Agent
  curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());

  // Следуем редиректам
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  // Устанавливаем таймаут
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    LOG("Error: curl_easy_perform() failed for URL: "
        << url << " with error: " << curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    return;
  }
  curl_easy_cleanup(curl);

  if (content.empty()) {
    LOG("Error: Empty content fetched for URL: " << url);
  }
}

void Crawler::parse_page(const std::string &content,
                         std::unordered_set<std::string> &links,
                         std::string &text, int mode,
                         const std::string &base_url) {
  if (content.empty()) {
    links.clear();
    text.clear();
    return;
  }
  if (mode) {
    links = parser.extract_links(content, base_url);
  }
  text = parser.extract_text(content);
  LOG("Extracted links count: " << links.size());
  LOG("Extracted text length: " << text.size());
}

void Crawler::save_to_database(const std::string &url,
                               const std::string &text) {
  LOG("Saving to database URL: " << url
                                 << " with text length: " << text.size());
  LOG("Database state: Checking if URL is processed: " << url);
  if (!db.is_url_processed(url)) {
    LOG("Database state: URL not processed, inserting page.");
    db.insert_page(url, text);
  } else {
    LOG("Database state: URL already processed.");
  }
}
