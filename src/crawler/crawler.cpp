#include "../../inc/crawler.h"
#include <thread>

#define LOG(msg)                                                               \
  if (config.verbose_logging && log_file.is_open()) {                          \
    log_file << msg << std::endl;                                              \
  }

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            std::string *output) {
  size_t totalSize = size * nmemb;
  output->append((char *)contents, totalSize);
  return totalSize;
}

Crawler::Crawler(const CrawlerConfig &config) : config(config) {

  log_file.open(config.log_filename, std::ios::trunc);

  scheduler = parallel_scheduler_create(config.thread_count);
  LOG("Creating parallel scheduler with thread count: " << config.thread_count);
  if (!scheduler) {
    LOG("Failed to create parallel scheduler");
    throw std::runtime_error("Failed to create parallel scheduler");
  }
  LOG("Parallel scheduler created successfully");

  db.connect(config.db_name.c_str(), CRAWLER);
  db.create_table();
  LOG("Database connected and table created.");

  user_agent = config.user_agent;
  MetricsCollector::instance().reset();
}

Crawler::~Crawler() {
  if (scheduler) {
    parallel_scheduler_destroy(scheduler);
  }
  scheduler = nullptr;
  stop_metrics_reporting();
}

void Crawler::print_performance_report(std::ostream &os) {
  MetricsCollector::instance().print_report(os);
}

void Crawler::reset_metrics() { MetricsCollector::instance().reset(); }

void Crawler::start_metrics_reporting() {
  if (metrics_running_)
    return;

  metrics_running_ = true;
  metrics_thread_ = std::make_unique<std::thread>([this]() {
    while (metrics_running_) {
      std::this_thread::sleep_for(std::chrono::seconds(10));

      if (config.verbose_logging && log_file.is_open()) {
        MetricsCollector::instance().print_report(log_file);
      }
    }
  });
}

void Crawler::stop_metrics_reporting() {
  metrics_running_ = false;
  if (metrics_thread_ && metrics_thread_->joinable()) {
    metrics_thread_->join();
  }
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
    if (!link.empty()) {

      link = UrlUtils::normalize_url(link);

      if (visited_links.find(link) == visited_links.end()) {

        add_to_queue(link, 0, 10.0);
        main_links.insert(link);

        std::string domain = UrlUtils::extract_domain(link);
        LOG("Extracted domain from initial link: " << domain);

        url_depths[link] = 0;
      } else {
        LOG("URL already visited: " << link);
      }
    }
  }

  LOG("Loaded main domains:");
  for (const auto &link : main_links) {
    LOG(" - " << UrlUtils::extract_domain(link));
  }
}

void Crawler::add_to_queue(const std::string &url, int depth, double priority) {

  static UrlPrioritizer prioritizer(config);

  if (priority == 0.0) {
    priority = prioritizer.calculate_priority(url, depth);
  }

  link_queue.push(UrlItem(url, depth, priority));
  url_depths[url] = depth;

  LOG("Added URL with priority " << priority << ": " << url);
}

static bool is_valid_domain(const std::string &link,
                            const std::string &main_url) {
  try {

    std::string main_domain = UrlUtils::extract_domain(main_url);
    std::string link_domain = UrlUtils::extract_domain(link);

    if (link_domain.empty() || main_domain.empty()) {
      std::cerr << "Ошибка извлечения домена: " << link << " vs " << main_url
                << std::endl;
      return false;
    }

    if (link_domain == main_domain) {
      return true;
    }

    if (link_domain.size() > main_domain.size() &&
        link_domain.find(main_domain,
                         link_domain.size() - main_domain.size()) !=
            std::string::npos &&
        link_domain[link_domain.size() - main_domain.size() - 1] == '.') {
      return true;
    }

    if (main_domain.size() > link_domain.size() &&
        main_domain.find(link_domain,
                         main_domain.size() - link_domain.size()) !=
            std::string::npos &&
        main_domain[main_domain.size() - link_domain.size() - 1] == '.') {
      return true;
    }

    return false;
  } catch (const std::exception &e) {
    std::cerr << "Exception in domain check: " << e.what() << " for " << link
              << " vs " << main_url << std::endl;
    return false;
  }
}

void Crawler::run(size_t size) {
  if (size == 0)
    size = config.max_links;
  links_size = size;

  std::cout << "Starting crawler with configuration:" << std::endl;
  std::cout << "  Threads: " << config.thread_count << std::endl;
  std::cout << "  Max links: " << size << std::endl;
  std::cout << "  Database: " << config.db_name << std::endl;

  LOG("Running crawler with size limit: " << size);

  start_metrics_reporting();

  while (true) {
    process_links(size);

    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      LOG("Queue status: " << link_queue.size() << " links in queue, "
                           << visited_links.size() << " visited links out of "
                           << size << " maximum");

      if (visited_links.size() >= size) {
        LOG("Visited links limit reached. Waiting for tasks to finish...");

        {
          std::unique_lock<std::mutex> lock(task_mutex);
          task_cv.wait(lock, [this]() { return active_tasks == 0; });
        }

        LOG("Processing remaining links...");
        break;
      }

      if (link_queue.empty()) {
        LOG("No more links to process. Exiting...");
        break;
      }
    }
  }

  std::cout << "\nCrawling completed." << std::endl;
  std::cout << "Processed " << visited_links.size() << " URLs." << std::endl;
  std::cout << "Results saved to " << config.db_name << std::endl;

  std::ofstream report("performance_report.txt");
  if (report.is_open()) {
    report << "\n===== Final Performance Report =====" << std::endl;
    print_performance_report(report);
    report << "==========================================\n";
    std::cout << "Performance report saved to 'performance_report.txt'"
              << std::endl;
  }
}

bool Crawler::url_matches_keywords(const std::string &url) {

  if (config.domain_keywords.empty()) {
    return true;
  }

  std::string domain = UrlUtils::extract_domain(url);

  if (config.domain_keywords.find(domain) == config.domain_keywords.end()) {

    return true;
  }

  const auto &keywords = config.domain_keywords.at(domain);
  if (keywords.empty()) {
    return true;
  }

  for (const auto &keyword : keywords) {

    size_t domain_pos = url.find(domain);
    if (domain_pos != std::string::npos) {
      std::string path = url.substr(domain_pos + domain.length());
      if (path.find(keyword) != std::string::npos) {
        return true;
      }
    }
  }

  return false;
}

void Crawler::process(const std::string &current_link, int depth) {

  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    LOG("Processing link (depth " << depth << "): " << current_link);
  }
  MetricsCollector::instance().increment_active_threads();

  if (!robots_parser.is_allowed(user_agent, current_link)) {
    try {
      LOG("URL not allowed by robots.txt: " << current_link);
      LOG("URL not allowed by robots.txt: " << current_link);

      {
        std::lock_guard<std::mutex> lock(queue_mutex);
        visited_links.insert(current_link);
      }
      {
        std::lock_guard<std::mutex> lock(queue_mutex);
        MetricsCollector::instance().set_queue_size(link_queue.size());
        MetricsCollector::instance().set_visited_count(visited_links.size());
      }

      MetricsCollector::instance().decrement_active_threads();
    } catch (const std::exception &e) {
      LOG("Exception in robots check: " << e.what());
    }
    return;
  }

  {
    std::string domain = UrlUtils::extract_domain(current_link);
    int crawl_delay = robots_parser.get_crawl_delay(user_agent, domain);

    if (crawl_delay > 0) {
      std::lock_guard<std::mutex> lock(domain_access_mutex);

      auto now = std::chrono::steady_clock::now();

      if (domain_last_access.find(domain) != domain_last_access.end()) {
        auto last_access = domain_last_access[domain];
        auto elapsed =
            std::chrono::duration_cast<std::chrono::seconds>(now - last_access)
                .count();

        if (elapsed < crawl_delay) {

          std::this_thread::sleep_for(
              std::chrono::seconds(crawl_delay - elapsed));
        }
      }

      domain_last_access[domain] = std::chrono::steady_clock::now();
    }
  }

  std::string content;
  std::unordered_set<std::string> links;
  std::string text;

  bool fetch_success = fetch_page_with_retry(current_link, content);

  if (fetch_success) {
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

            if (url_matches_keywords(link) && link_queue.size() < links_size) {
              LOG("Adding link to queue (depth " << depth + 1 << "): " << link);
              add_to_queue(link, depth + 1);
            } else {
              LOG("Skipping URL due to keyword filter: " << link);
            }
          }
        }
      }
    }
  } else {
    LOG("Failed to fetch page: " << current_link
                                 << ", skipping link processing");
  }

  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    visited_links.insert(current_link);
    LOG("Visited links count: "
        << visited_links.size()
        << (fetch_success ? "" : " (after fetch failure)"));

    if (visited_links.size() >= links_size) {
      LOG("Visited links limit reached. Stopping processing...");
      return;
    }
  }

  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    visited_links.insert(current_link);

    MetricsCollector::instance().set_visited_count(visited_links.size());
    MetricsCollector::instance().set_queue_size(link_queue.size());

    LOG("Visited links count: "
        << visited_links.size()
        << (fetch_success ? "" : " (after fetch failure)"));

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
      UrlItem item = link_queue.top();
      link_queue.pop();
      current_link = item.url;
    }

    std::string content;
    std::unordered_set<std::string> links;
    std::string text;

    bool fetch_success = fetch_page_with_retry(current_link, content);

    if (fetch_success) {
      parse_page(content, links, text, 0, current_link);
      save_to_database(current_link, text);
    } else {
      LOG("Failed to fetch remaining page: " << current_link);
    }

    {
      std::lock_guard<std::mutex> lock(task_mutex);
      visited_links.insert(current_link);
      LOG("Processed remaining link: "
          << current_link << ", Visited links count: " << visited_links.size()
          << (fetch_success ? "" : " (fetch failed)"));
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
    int depth = 0;

    {
      std::lock_guard<std::mutex> lock(queue_mutex);
      if (link_queue.empty() || visited_links.size() >= size) {
        break;
      }

      UrlItem item = link_queue.top();
      link_queue.pop();
      current_link = item.url;
      depth = item.depth;

      LOG("Processing URL with priority " << item.priority << " and depth "
                                          << depth << ": " << current_link);
    }

    {
      std::lock_guard<std::mutex> lock(task_mutex);
      active_tasks++;
    }

    void *task_ptr =
        new std::tuple<Crawler *, std::string, int>(this, current_link, depth);

    parallel_scheduler_run(
        scheduler,
        [](void *arg) {
          auto task_data =
              std::unique_ptr<std::tuple<Crawler *, std::string, int>>(
                  static_cast<std::tuple<Crawler *, std::string, int> *>(arg));
          Crawler *crawler = std::get<0>(*task_data);
          std::string link = std::get<1>(*task_data);
          int depth = std::get<2>(*task_data);

          crawler->process(link, depth);

          {
            std::lock_guard<std::mutex> lock(crawler->task_mutex);
            crawler->active_tasks--;
            crawler->task_cv.notify_one();
          }
        },
        task_ptr);
  }

  {
    std::unique_lock<std::mutex> lock(task_mutex);
    task_cv.wait(lock, [this]() { return active_tasks == 0; });
  }
}

bool Crawler::fetch_page(const std::string &url, std::string &content) {
  long http_code;
  return fetch_page_with_http_code(url, content, &http_code);
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

bool Crawler::fetch_page_with_retry(const std::string &url,
                                    std::string &content, int max_retries,
                                    int retry_delay_sec) {

  if (max_retries <= 0)
    max_retries = config.max_retries;
  if (retry_delay_sec <= 0)
    retry_delay_sec = config.retry_delay_sec;

  long http_code = 0;
  bool success = fetch_page_with_http_code(url, content, &http_code);

  if (http_code >= 400 && http_code < 500) {
    LOG("Client error " << http_code << " for URL: " << url
                        << " - not retrying");
    return false;
  }

  if (!success) {
    for (int attempt = 1; attempt < max_retries; ++attempt) {
      LOG("Retry attempt " << attempt << " of " << (max_retries - 1)
                           << " for URL: " << url);
      std::this_thread::sleep_for(std::chrono::seconds(retry_delay_sec));

      if (fetch_page_with_http_code(url, content, &http_code)) {
        return true;
      }

      if (http_code >= 400 && http_code < 500) {
        LOG("Client error " << http_code << " on retry for URL: " << url);
        return false;
      }
    }
  }

  return success;
}

bool Crawler::fetch_page_with_http_code(const std::string &url,
                                        std::string &content, long *http_code) {
  content.clear();
  CURL *curl = curl_easy_init();
  if (!curl) {
    LOG("Error: Failed to initialize CURL for URL: " << url);
    return false;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent.c_str());
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT,
                   static_cast<long>(config.request_timeout_sec));

  CURLcode res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    LOG("Error: curl_easy_perform() failed for URL: "
        << url << " with error: " << curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    return false;
  }

  if (http_code) {
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_code);
  } else {
    long code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
  }

  curl_easy_cleanup(curl);

  LOG("HTTP response code for URL " << url << ": "
                                    << (http_code ? *http_code : 0));

  if (http_code && (*http_code >= 200 && *http_code < 400)) {
    if (content.empty()) {
      LOG("Warning: Empty content with successful HTTP code for URL: " << url);
      return false;
    }

    MetricsCollector::instance().add_bytes_downloaded(content.size());
    return true;
  }

  if (http_code) {
    if (*http_code == 404) {
      LOG("URL not found (404): " << url);
    } else if (*http_code >= 400 && *http_code < 500) {
      LOG("Client error for URL: " << url << " with HTTP code: " << *http_code);
    } else if (*http_code >= 500) {
      LOG("Server error for URL: " << url << " with HTTP code: " << *http_code);
    } else {
      LOG("Unexpected HTTP code " << *http_code << " for URL: " << url);
    }
  }

  return false;
}