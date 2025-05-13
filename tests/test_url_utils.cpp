#include "catch2/catch.hpp"
#include "../inc/url_utils.h"
#include <string>

TEST_CASE("URL normalization", "[url_utils]") {
    SECTION("Basic normalization") {
        CHECK(UrlUtils::normalize_url("http://example.com") == "http://example.com/");
        CHECK(UrlUtils::normalize_url("http://example.com/") == "http://example.com/");
        CHECK(UrlUtils::normalize_url("HTTP://EXAMPLE.COM/") == "http://example.com/");
    }

    SECTION("Path normalization") {
        CHECK(UrlUtils::normalize_url("http://example.com/path/./to/page") == "http://example.com/path/to/page");
        CHECK(UrlUtils::normalize_url("http://example.com/path/../to/page") == "http://example.com/to/page");
        CHECK(UrlUtils::normalize_url("http://example.com/path/to/..") == "http://example.com/path/");
    }

    SECTION("Query parameters") {
        CHECK(UrlUtils::normalize_url("http://example.com/?a=1&b=2") == "http://example.com/?a=1&b=2");
        CHECK(UrlUtils::normalize_url("http://example.com/?b=2&a=1") == "http://example.com/?b=2&a=1");
    }

    SECTION("URL with fragments") {
        CHECK(UrlUtils::normalize_url("http://example.com/page#section") == "http://example.com/page");
    }
}

TEST_CASE("Make absolute URL", "[url_utils]") {
    SECTION("Absolute URLs") {
        CHECK(UrlUtils::make_absolute_url("http://example.com", "http://another.com") == "http://another.com");
        CHECK(UrlUtils::make_absolute_url("http://example.com/", "https://another.com/") == "https://another.com/");
    }

    SECTION("Relative URLs") {
        CHECK(UrlUtils::make_absolute_url("http://example.com", "/page") == "http://example.com/page");
        CHECK(UrlUtils::make_absolute_url("http://example.com/path/", "subpage") == "http://example.com/path/subpage");
        CHECK(UrlUtils::make_absolute_url("http://example.com/path/page.html", "../other.html") == "http://example.com/other.html");
    }
}

TEST_CASE("Extract domain", "[url_utils]") {
    SECTION("Simple domains") {
        CHECK(UrlUtils::extract_domain("http://example.com") == "example.com");
        CHECK(UrlUtils::extract_domain("https://example.com/") == "example.com");
    }

    SECTION("Subdomains") {
        CHECK(UrlUtils::extract_domain("http://sub.example.com") == "sub.example.com");
        CHECK(UrlUtils::extract_domain("https://blog.example.co.uk/") == "blog.example.co.uk");
    }

    SECTION("URLs with paths and parameters") {
        CHECK(UrlUtils::extract_domain("http://example.com/path/to/page") == "example.com");
        CHECK(UrlUtils::extract_domain("https://example.com/path?param=value") == "example.com");
    }
}

TEST_CASE("Same domain check", "[url_utils]") {
    SECTION("Exact same domain") {
        CHECK(UrlUtils::is_same_domain("http://example.com", "http://example.com/page"));
        CHECK(UrlUtils::is_same_domain("https://example.com/", "http://example.com/other"));
    }

    SECTION("Subdomain checks") {
        CHECK(UrlUtils::is_same_domain("http://example.com", "http://sub.example.com") == false);
        CHECK(UrlUtils::is_same_domain("http://blog.example.com", "http://news.example.com") == false);
    }

    SECTION("Different domains") {
        CHECK(UrlUtils::is_same_domain("http://example.com", "http://another.com") == false);
        CHECK(UrlUtils::is_same_domain("https://example.com", "https://example.org") == false);
    }
}