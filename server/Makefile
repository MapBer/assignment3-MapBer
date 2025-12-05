CROSS_COMPILE?=
CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall -Wextra -O2
TARGET=aesdsocket
SRC=aesdsocket.c

default: all
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -DDAEMON -o $(TARGET) $(SRC)

clean:
	rm -f *.o $(TARGET)