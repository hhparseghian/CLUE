CC      = gcc
CFLAGS  = -std=c99 -Wall $(shell pkg-config --cflags clue)
LDFLAGS = $(shell pkg-config --libs clue)
TARGET  = myapp

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: clean
