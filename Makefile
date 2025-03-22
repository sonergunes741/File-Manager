CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99
LDFLAGS = 
SRCS = fileManager.c fileops.c dirops.c logging.c utils.c
OBJS = $(SRCS:.c=.o)
TARGET = fileManager
.PHONY: all clean
all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJS) $(TARGET) log.txt *~ *.o