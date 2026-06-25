CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror
LDLIBS = -lncurses
TARGET = game_of_life
SRC = game_of_life.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDLIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
