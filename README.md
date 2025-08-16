# Web Crawler & Search Engine

A robust, multithreaded web crawler and search engine implemented in C++ that crawls websites, extracts and indexes content, and provides fast full-text search capabilities.

## Features

### 🕷️ Web Crawler
- **Multithreaded Architecture**: Configurable thread pool for concurrent crawling
- **Intelligent URL Prioritization**: Advanced scoring system based on keywords, depth, and domain relevance
- **Robots.txt Compliance**: Respects robots.txt rules and crawl delays
- **Duplicate Detection**: Prevents processing of duplicate URLs
- **Domain-Specific Crawling**: Focus crawling on specific domains with keyword filtering
- **Retry Logic**: Automatic retry mechanism for failed requests
- **Performance Metrics**: Real-time monitoring of crawling performance

### 🔍 Search Engine
- **Full-Text Search**: Fast content search using SQLite FTS
- **SQLite Database**: Efficient storage and indexing of crawled content
- **URL Normalization**: Consistent URL handling and deduplication

### ⚙️ Configuration
- **JSON Configuration**: Easy configuration through JSON files
- **Customizable Parameters**: Thread count, timeouts, retry policies, and more
- **Domain Keywords**: Specify keywords for targeted crawling per domain

## Architecture

### Core Components

- **Crawler Engine** ([`src/crawler/crawler.cpp`](src/crawler/crawler.cpp)): Main crawling logic with multithreading support
- **URL Utilities** ([`src/url/url_utils.cpp`](src/url/url_utils.cpp)): URL normalization and domain extraction
- **URL Prioritizer** ([`src/url/url_priority.cpp`](src/url/url_priority.cpp)): Intelligent URL scoring and prioritization
- **HTML Parser** ([`src/htmlparser/htmlparser.cpp`](src/htmlparser/htmlparser.cpp)): Link extraction and text content parsing
- **Database Layer** ([`src/database/database.cpp`](src/database/database.cpp)): SQLite integration for data persistence
- **Robots Parser** ([`src/robots_parser/robots_parser.cpp`](src/robots_parser/robots_parser.cpp)): Robots.txt compliance
- **Metrics Collector** ([`src/metrics/metrics_collector.cpp`](src/metrics/metrics_collector.cpp)): Performance monitoring
- **Searcher** ([`src/searcher/searcher.cpp`](src/searcher/searcher.cpp)): Search functionality

### Parallel Scheduler
Custom C-based thread pool implementation ([`libs/parallel_scheduler/`](libs/parallel_scheduler/)) for efficient task distribution.

## Prerequisites

### Required Libraries
```bash
# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev libsqlite3-dev libgumbo-dev libnlohmann-json3-dev

# macOS (with Homebrew)
brew install curl sqlite gumbo-parser nlohmann-json

# Fedora/RHEL
sudo dnf install libcurl-devel sqlite-devel gumbo-parser-devel json-devel
```

### Build Tools
- GCC/G++ with C++17 support
- Make
- pthread library

## Installation

1. **Clone the repository**
```bash
git clone https://github.com/yourusername/web-crawler-search-engine.git
cd web-crawler-search-engine
```

2. **Build the project**
```bash
make all
```

This will create two executables:
- `crawler` - The web crawler
- `searcher` - The search interface

## Usage

### Configuration

Create a config.json file to customize crawler behavior:

```json
{
    "thread_count": 10,
    "db_name": "web_crawler.db",
    "user_agent": "MyAdvancedCrawler/2.0",
    "request_timeout_sec": 15,
    "max_links": 1000,
    "max_retries": 5,
    "retry_delay_sec": 3,
    "log_filename": "crawler_log.txt",
    "verbose_logging": true,
    "domain_keyword_weight": 3.0,
    "cross_domain_keyword_weight": 1.5,
    "domain_keywords": {
        "example.com": ["keyword1", "keyword2"],
        "another-site.com": ["important", "content"]
    }
}
```

### Crawling

1. **Create a links file** (links.txt) with starting URLs:
```
https://example.com/
https://another-site.com/
```

2. **Run the crawler**:
```bash
# With custom config and links file
./crawler config.json links.txt

# With default settings
./crawler
```

### Searching

Search the crawled content:
```bash
./searcher web_crawler.db "search query"
```

## Configuration Options

| Parameter | Description | Default |
|-----------|-------------|---------|
| `thread_count` | Number of crawler threads | 10 |
| `db_name` | SQLite database filename | "parser.db" |
| `user_agent` | HTTP User-Agent string | "MyWebCrawler/1.0" |
| `request_timeout_sec` | HTTP request timeout | 30 |
| `max_links` | Maximum URLs to crawl | 1000 |
| `max_retries` | Retry attempts for failed requests | 3 |
| `retry_delay_sec` | Delay between retries | 5 |
| `verbose_logging` | Enable detailed logging | true |
| `domain_keywords` | Keywords for domain-specific crawling | {} |

## Performance Features

- **Real-time Metrics**: Monitor crawling speed, success rates, and bandwidth usage
- **Memory Efficient**: Optimized memory usage for large-scale crawling
- **Respectful Crawling**: Implements delays and respects robots.txt
- **Error Handling**: Robust error handling and recovery mechanisms

## Output

The crawler generates:
- **SQLite Database**: Contains all crawled URLs and extracted text content
- **Performance Report**: Detailed metrics saved to `performance_report.txt`
- **Logs**: Configurable logging output for debugging and monitoring

## Examples

### Basic Crawling
```bash
# Crawl up to 50 pages from allrecipes.com
echo "https://www.allrecipes.com/" > links.txt
./crawler
```

### Targeted Crawling with Keywords
```bash
# Focus on salad-related content from allrecipes.com
# Configure domain_keywords in config.json:
# "domain_keywords": {
#     "allrecipes.com": ["salad", "recipe"]
# }
./crawler config.json links.txt
```

### Searching
```bash
# Search for "chocolate cake" in crawled content
./searcher web_crawler.db "chocolate cake"
```

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- [libcurl](https://curl.se/libcurl/) for HTTP functionality
- [SQLite](https://www.sqlite.org/) for database operations
- [Gumbo](https://github.com/google/gumbo-parser) for HTML parsing
- [nlohmann/json](https://github.com/nlohmann/json) for JSON configuration
