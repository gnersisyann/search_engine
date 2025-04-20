CC                  = g++
CFLAGS              = -Wall -Wextra -O3
RM                  = rm -f

NAME                = crawler
SEARCHER            = searcher
OTHER				= logs.txt \
						parser.db

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

OBJ_DIR             = obj
CRAWLER_OBJ_DIR     = $(OBJ_DIR)/crawler
SEARCHER_OBJ_DIR    = $(OBJ_DIR)/searcher
DATABASE_OBJ_DIR    = $(OBJ_DIR)/database
HTMLPARSER_OBJ_DIR  = $(OBJ_DIR)/htmlparser

CRAWLER_SRC         = $(CRAWLER_SRC_DIR)/main.cpp \
                      $(CRAWLER_SRC_DIR)/crawler.cpp \
                      $(DATABASE_SRC_DIR)/database.cpp \
                      $(HTMLPARSER_SRC_DIR)/htmlparser.cpp

SEARCHER_SRC        = $(SEARCHER_SRC_DIR)/main.cpp \
                      $(SEARCHER_SRC_DIR)/searcher.cpp \
                      $(DATABASE_SRC_DIR)/database.cpp

CRAWLER_OBJ         = $(CRAWLER_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
SEARCHER_OBJ        = $(SEARCHER_SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME) $(SEARCHER)

$(NAME): $(LIBS_FILE) $(CRAWLER_OBJ)
	$(CC) $(CFLAGS) $(CRAWLER_OBJ) $(LDFLAGS) -o $@

$(SEARCHER): $(LIBS_FILE) $(SEARCHER_OBJ)
	$(CC) $(CFLAGS) $(SEARCHER_OBJ) $(LDFLAGS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBS_FILE):
	$(MAKE_LIB) $(LIBS_DIR)

clean:
	$(MAKE_LIB) $(LIBS_DIR) clean
	$(RM) -rf $(OBJ_DIR) $(OTHER)

fclean: clean
	$(MAKE_LIB) $(LIBS_DIR) fclean
	$(RM) $(NAME) $(SEARCHER)

re: fclean all

.PHONY: all clean fclean re