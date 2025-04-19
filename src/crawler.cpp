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
  std::cerr << "Creating parallel scheduler with thread count: " << thread_count
            << std::endl;
  if (!scheduler) {
    std::cerr << "Failed to create parallel scheduler" << std::endl;
    throw std::runtime_error("Failed to create parallel scheduler");
  }
  std::cerr << "Parallel scheduler created successfully" << std::endl;

  db.connect("parser.db");
  db.create_table();
  std::cerr << "Database connected and table created." << std::endl;
}

Crawler::~Crawler() {
  if (scheduler) {
    parallel_scheduler_destroy(scheduler);
  }
  scheduler = nullptr;
}

void Crawler::load_links_from_file(const std::string &filename) {
  std::cerr << "Loading links from file: " << filename << std::endl;
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Unable to open file " << filename << std::endl;
    return;
  }
  std::cerr << "File opened successfully: " << filename << std::endl;
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
    std::string::size_type pos = link.find(domain);
    if (pos == std::string::npos) {
      return false;
    }
    if (pos > 0 && link[pos - 1] != '/' && link[pos - 1] != '.') {
      return false;
    }
    return true;
  } catch (const std::exception &e) {
    std::cerr << "Error in is_valid_domain: " << e.what() << std::endl;
    return false;
  }
}

void Crawler::process(const std::string &current_link) {
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    std::cerr << "Processing link: " << current_link << std::endl;
  }
  std::string content;
  std::unordered_set<std::string> links;
  std::string text;
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    fetch_page(current_link, content);
  }
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    parse_page(content, links, text);
  }
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    save_to_database(current_link, text);
  }
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    std::cerr << "Adding links to queue. Current link count: " << links.size()
              << std::endl;
  }

  for (const auto &link : links) {
    bool valid_domain = false;
    for (const auto &main_link : main_links) {
      if (is_valid_domain(link, main_link)) {
        valid_domain = true;
        break;
      }
    }
    if (valid_domain && visited_links.find(link) == visited_links.end()) {
      {
        std::lock_guard<std::mutex> lock(queue_mutex);
        link_queue.push(link);
        visited_links.insert(link);
      }
    }
  }
}

void Crawler::process_links(size_t size) {
  //   std::cerr << "Starting to process links with size limit: " << size
  //             << std::endl;
  //   std::cerr << "Initial queue size: " << link_queue.size() << std::endl;
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
  if (visited_links.size() == size) {
    exit(0);
  }
}

void Crawler::fetch_page(const std::string &url, std::string &content) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    std::cerr << "Error: Failed to initialize CURL for URL: " << url
              << std::endl;
    return;
  }
  std::cerr << "CURL initialized successfully for URL: " << url << std::endl;

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &content);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      std::cerr << "Error: curl_easy_perform() failed for URL: " << url
                << " with error: " << curl_easy_strerror(res) << std::endl;
      curl_easy_cleanup(curl);
      return;
    }
    std::cerr << "CURL performed successfully for URL: " << url << std::endl;
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
  //   std::cerr << "Parsing page content for URL: " << content.substr(0, 50)
  //             << "..." << std::endl;
  std::cerr << "Extracted links count: " << links.size() << std::endl;
  std::cerr << "Extracted text length: " << text.size() << std::endl;
}

void Crawler::save_to_database(const std::string &url,
                               const std::string &text) {
  std::cerr << "Saving to database URL: " << url
            << " with text length: " << text.size() << std::endl;
  std::cerr << "Database state: Checking if URL is processed: " << url
            << std::endl;
  if (!db.is_url_processed(url)) {
    std::cerr << "Database state: URL not processed, inserting page."
              << std::endl;
    db.insert_page(url, text);
  } else {
    std::cerr << "Database state: URL already processed." << std::endl;
  }
}
