CROSS_COMPILE?=
CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall -Wextra -O2
TARGET=writer
SRC=writer.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f *.o $(TARGET)
