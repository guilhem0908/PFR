TARGET = PFR

CC = gcc
CFLAGS = -std=c11 -Wall

SRCS = main.c file_operations.c image_process.c cluster.c
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

distclean: clean

.PHONY: clean distclean