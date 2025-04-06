#include "../inc/htmlparser.h"
#include <cstring>
#include <gumbo.h>

static void find_links(GumboNode *node,
                       std::unordered_set<std::string> &links) {
  if (node->type != GUMBO_NODE_ELEMENT) {
    return;
  }

  GumboElement *element = &node->v.element;

  if (element->tag == GUMBO_TAG_A) {
    GumboAttribute *href = gumbo_get_attribute(&element->attributes, "href");
    if (href && href->value && std::strlen(href->value) > 0) {
      links.insert(href->value);
    }
  }

  for (unsigned int i = 0; i < element->children.length; ++i) {
    find_links(static_cast<GumboNode *>(element->children.data[i]), links);
  }
}

static void find_text(GumboNode *node, std::string &text) {
  if (node->type == GUMBO_NODE_TEXT) {
    text += node->v.text.text;
    text += " ";
  } else if (node->type == GUMBO_NODE_ELEMENT) {
    GumboElement *element = &node->v.element;
    if (element->tag == GUMBO_TAG_SCRIPT || element->tag == GUMBO_TAG_STYLE) {
      return;
    }
    for (unsigned int i = 0; i < element->children.length; ++i) {
      find_text(static_cast<GumboNode *>(element->children.data[i]), text);
    }
  }
}

std::unordered_set<std::string>
HTMLParser::extract_links(const std::string &html) {
  if (html.empty())
    return {};
  GumboOutput *gumbo = gumbo_parse(html.c_str());
  std::unordered_set<std::string> links;

  find_links(gumbo->root, links);

  gumbo_destroy_output(&kGumboDefaultOptions, gumbo);

  return links;
}
std::string HTMLParser::extract_text(const std::string &html) {
  if (html.empty())
    return {};
  GumboOutput *gumbo = gumbo_parse(html.c_str());
  std::string text;

  find_text(gumbo->root, text);

  gumbo_destroy_output(&kGumboDefaultOptions, gumbo);

  return text;
}