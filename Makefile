CRAWLER = crawler
SEARCH = search
CC = g++
HEADER = #inc/header.h
CFLAGS = -Wall -Wextra -Ilibs/parallel_scheduler
LDFLAGS = -Llibs/parallel_scheduler -lparallel_scheduler -pthread -lgumbo -lcurl -lsqlite3
PARALLEL_SCHEDULER_PATH = libs/parallel_scheduler
SRC_CRAWLER = src/crawler.c
SRC_SEARCH = src/search.c
OBJ_CRAWLER = $(SRC_CRAWLER:.c=.o)
OBJ_SEARCH = $(SRC_SEARCH:.c=.o)

all: subsystems $(CRAWLER) #$(SEARCH)

subsystems:
	$(MAKE) -C $(PARALLEL_SCHEDULER_PATH)

$(CRAWLER): $(OBJ_CRAWLER)
	$(CC) $^ $(LDFLAGS) -o $@

#$(SEARCH): $(OBJ_SEARCH)
#	$(CC) $^ $(LDFLAGS) -o $@

%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(MAKE) -C $(PARALLEL_SCHEDULER_PATH) clean
	rm -f $(OBJ_CRAWLER) $(OBJ_SEARCH)

fclean: clean
	$(MAKE) -C $(PARALLEL_SCHEDULER_PATH) fclean
	rm -f $(CRAWLER) $(SEARCH)

re: fclean all

.PHONY: all clean fclean re subsystems