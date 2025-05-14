CC                  = g++
CFLAGS              = -g -Wall -Wextra -O0
RM                  = rm -f

NAME                = crawler
SEARCHER            = searcher
OTHER               = logs.txt \
                      parser.db \
                      performance_report.txt

LIBS_DIR            = libs/parallel_scheduler
LIBS_FILE           = $(LIBS_DIR)/libparallel_scheduler.a
CFLAGS              += -I$(LIBS_DIR) -Iinc
LDFLAGS             = -L$(LIBS_DIR) -lparallel_scheduler -pthread -lgumbo -lcurl -lsqlite3
MAKE_LIB            = make -C

SRC_DIR             = src
CRAWLER_SRC_DIR     = $(SRC_DIR)/crawler
SEARCHER_SRC_DIR    = $(SRC_DIR)/searcher
DATABASE_SRC_DIR    = $(SRC_DIR)/database
HTMLPARSER_SRC_DIR  = $(SRC_DIR)/htmlparser
METRICS_SRC_DIR     = $(SRC_DIR)/metrics
URL_SRC_DIR         = $(SRC_DIR)/url
ROBOTS_SRC_DIR      = $(SRC_DIR)/robots_parser

OBJ_DIR             = obj
CRAWLER_OBJ_DIR     = $(OBJ_DIR)/crawler
SEARCHER_OBJ_DIR    = $(OBJ_DIR)/searcher
DATABASE_OBJ_DIR    = $(OBJ_DIR)/database
HTMLPARSER_OBJ_DIR  = $(OBJ_DIR)/htmlparser
METRICS_OBJ_DIR     = $(OBJ_DIR)/metrics
URL_OBJ_DIR         = $(OBJ_DIR)/url
ROBOTS_OBJ_DIR      = $(OBJ_DIR)/robots_parser

CRAWLER_SRC         = $(CRAWLER_SRC_DIR)/main.cpp \
                      $(CRAWLER_SRC_DIR)/crawler.cpp \
                      $(CRAWLER_SRC_DIR)/crawler_config.cpp \
                      $(DATABASE_SRC_DIR)/database.cpp \
                      $(HTMLPARSER_SRC_DIR)/htmlparser.cpp \
                      $(METRICS_SRC_DIR)/metrics_collector.cpp \
                      $(URL_SRC_DIR)/url_priority.cpp \
                      $(URL_SRC_DIR)/url_utils.cpp \
                      $(ROBOTS_SRC_DIR)/robots_parser.cpp

SEARCHER_SRC        = $(SEARCHER_SRC_DIR)/main.cpp \
                      $(SEARCHER_SRC_DIR)/searcher.cpp \
                      $(DATABASE_SRC_DIR)/database.cpp \
                      $(URL_SRC_DIR)/url_utils.cpp

# Тестовые файлы
TEST_DIR            = tests
TEST_SRC            = $(TEST_DIR)/test_main.cpp \
                      $(TEST_DIR)/test_crawler.cpp \
                      $(TEST_DIR)/test_url_utils.cpp
TEST_OBJ            = $(TEST_SRC:%.cpp=$(OBJ_DIR)/%.o)
TEST_TARGET         = run_tests

CRAWLER_OBJ         = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(CRAWLER_SRC))
SEARCHER_OBJ        = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SEARCHER_SRC))

all: $(NAME) $(SEARCHER)

$(NAME): $(LIBS_FILE) $(CRAWLER_OBJ)
	$(CC) $(CFLAGS) $(CRAWLER_OBJ) $(LDFLAGS) -o $@

$(SEARCHER): $(LIBS_FILE) $(SEARCHER_OBJ)
	$(CC) $(CFLAGS) $(SEARCHER_OBJ) $(LDFLAGS) -o $@

# Правило для запуска crawler с конфигурационным файлом
run: $(NAME)
	./$(NAME) config.json links.txt

# Правило для тестов
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(LIBS_FILE) $(filter-out $(OBJ_DIR)/$(CRAWLER_SRC_DIR)/main.o, $(CRAWLER_OBJ)) $(TEST_OBJ)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/$(TEST_DIR)/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Itests -c $< -o $@

$(LIBS_FILE):
	$(MAKE_LIB) $(LIBS_DIR)

clean:
	$(MAKE_LIB) $(LIBS_DIR) clean
	$(RM) -rf $(OBJ_DIR) $(OTHER)

fclean: clean
	$(MAKE_LIB) $(LIBS_DIR) fclean
	$(RM) $(NAME) $(SEARCHER) $(TEST_TARGET) *.db *_log.txt

re: fclean all

.PHONY: all clean fclean re test run