NAME=words

CC=gcc
FLAGS=-Wall -Werror -Wextra -std=c99 -Wvla
DEBUG_FLAGS= -fsanitize=address,undefined -g
SRC=*.c
LIBS=
OBJ=
RM =rm -rf

all: $(NAME)

$(NAME): $(SRC)
	$(CC) $(FLAGS) $(LIBS) $(SRC) -o $(NAME)

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re: fclean all

run: re
	./$(NAME) test

debug: fclean
	$(CC) $(FLAGS) $(DEBUG_FLAGS) $(LIBS) $(SRC) -o $(NAME)
