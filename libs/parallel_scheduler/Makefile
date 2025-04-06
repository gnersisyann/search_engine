NAME = libparallel_scheduler.a
CC = cc
AR = ar crs
CFLAGS = -Wall -Wextra -pthread
SRC = parallel_scheduler.c
HEADER = parallel_scheduler.h
OBJ = $(SRC:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(AR) $@ $^

%.o: %.c $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re