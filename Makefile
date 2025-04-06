CRAWLER = crawler
SEARCH = search
CC = g++
HEADER = inc/includes.h
CFLAGS = -Wall -Wextra -Ilibs/parallel_scheduler -Iinc
LDFLAGS = -Llibs/parallel_scheduler -lparallel_scheduler -pthread -lgumbo -lcurl -lsqlite3
PARALLEL_SCHEDULER_PATH = libs/parallel_scheduler
SRC_CRAWLER = src/crawler.cpp src/database.cpp src/htmlparser.cpp
SRC_SEARCH = src/main.cpp src/database.cpp src/htmlparser.cpp
OBJ_CRAWLER = $(SRC_CRAWLER:.cpp=.o)
OBJ_SEARCH = $(SRC_SEARCH:.cpp=.o)

all: subsystems $(CRAWLER) $(SEARCH)

subsystems:
	$(MAKE) -C $(PARALLEL_SCHEDULER_PATH)

$(CRAWLER): $(OBJ_CRAWLER)
	$(CC) $^ $(LDFLAGS) -o $@

$(SEARCH): $(OBJ_SEARCH)
	$(CC) $^ $(LDFLAGS) -o $@

%.o: %.cpp $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(MAKE) -C $(PARALLEL_SCHEDULER_PATH) clean
	rm -f $(OBJ_CRAWLER) $(OBJ_SEARCH)

fclean: clean
	$(MAKE) -C $(PARALLEL_SCHEDULER_PATH) fclean
	rm -f $(CRAWLER) $(SEARCH)

re: fclean all

.PHONY: all clean fclean re subsystems